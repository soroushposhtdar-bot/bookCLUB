// common/Interfaces/IReviewRepository.cpp
//
// Concrete `IReviewRepository` backed by the shared SQLite database.
// Fixes from the previous version:
//   - Use shared "bookclub_shared" connection.
//   - Helper functions no longer take `const QSqlQuery&` and call non-const
//     `next()` on it.

#include "common/Interfaces/IReviewRepository.h"
#include "common/Models/Review.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
QSqlDatabase sharedDatabase()
{
    auto db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }
    db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
    db.setDatabaseName("bookclub.db");
    db.open();
    return db;
}

QSqlQuery runQuery(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery query(sharedDatabase());
    query.prepare(sql);
    for (const auto& p : params) {
        query.addBindValue(p);
    }
    query.exec();
    return query;
}

bool execOk(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery q = runQuery(sql, params);
    return q.lastError().type() == QSqlError::NoError;
}
} // namespace

// ========== Implementation ==========
class ReviewRepositoryImpl : public IReviewRepository {
public:
    ReviewRepositoryImpl() = default;
    ~ReviewRepositoryImpl() override = default;

    Review* findById(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Reviews WHERE id = ?", {id});
        if (query.next()) {
            return createReviewFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Review*> findByBook(const QString& bookId) const override
    {
        QVector<Review*> reviews;
        QSqlQuery query = runQuery(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (query.next()) {
            reviews.append(createReviewFromCurrentRecord(query));
        }
        return reviews;
    }

    QVector<Review*> findByUser(const QString& userId) const override
    {
        QVector<Review*> reviews;
        QSqlQuery query = runQuery(
            "SELECT * FROM Reviews WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            reviews.append(createReviewFromCurrentRecord(query));
        }
        return reviews;
    }

    bool save(Review* review) override
    {
        if (!review) return false;
        if (review->id().isEmpty()) {
            review->setId(IdGenerator::generateUuid());
        }

        const QString sql = R"(
            INSERT INTO Reviews (
                id, bookId, userId, userDisplayName, text, stars,
                createdAt, updatedAt, isEdited
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        const bool ok = execOk(sql, {
            review->id(),
            review->bookId(),
            review->userId(),
            review->userDisplayName(),
            review->text(),
            review->stars(),
            QDateTime::currentDateTime(),
            QDateTime::currentDateTime(),
            0
        });

        if (ok) updateBookRating(review->bookId());
        return ok;
    }

    bool update(Review* review) override
    {
        if (!review || review->id().isEmpty()) return false;

        const QString sql = R"(
            UPDATE Reviews SET
                text = ?,
                stars = ?,
                updatedAt = ?,
                isEdited = 1
            WHERE id = ?
        )";

        const bool ok = execOk(sql, {
            review->text(),
            review->stars(),
            QDateTime::currentDateTime(),
            review->id()
        });

        if (ok) updateBookRating(review->bookId());
        return ok;
    }

    bool remove(const QString& id) override
    {
        QString bookId;
        {
            QSqlQuery findQuery = runQuery("SELECT bookId FROM Reviews WHERE id = ?", {id});
            if (findQuery.next()) {
                bookId = findQuery.value(0).toString();
            }
        }
        const bool ok = execOk("DELETE FROM Reviews WHERE id = ?", {id});
        if (ok && !bookId.isEmpty()) {
            updateBookRating(bookId);
        }
        return ok;
    }

    double averageRating(const QString& bookId) const override
    {
        QSqlQuery query = runQuery(
            "SELECT AVG(stars) FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (query.next()) {
            return query.value(0).toDouble();
        }
        return 0.0;
    }

    int ratingCount(const QString& bookId) const override
    {
        QSqlQuery query = runQuery(
            "SELECT COUNT(*) FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

private:
    Review* createReviewFromCurrentRecord(QSqlQuery& query) const
    {
        QSqlRecord rec = query.record();
        auto* review = new Review;
        review->setId(rec.value("id").toString());
        review->setBookId(rec.value("bookId").toString());
        review->setUserId(rec.value("userId").toString());
        review->setUserDisplayName(rec.value("userDisplayName").toString());
        review->setText(rec.value("text").toString());
        review->setStars(rec.value("stars").toInt());
        review->setCreatedAt(rec.value("createdAt").toDateTime());
        review->setUpdatedAt(rec.value("updatedAt").toDateTime());
        review->setEdited(rec.value("isEdited").toInt() == 1);
        return review;
    }

    void updateBookRating(const QString& bookId) const
    {
        QSqlQuery q = runQuery(
            "SELECT AVG(stars), COUNT(*) FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (q.next()) {
            const double avg = q.value(0).toDouble();
            const int cnt = q.value(1).toInt();
            execOk(
                "UPDATE Books SET averageRating = ?, ratingCount = ? WHERE id = ?",
                {avg, cnt, bookId}
            );
        }
    }
};

// ========== Factory ==========
IReviewRepository* createReviewRepository() {
    static ReviewRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
