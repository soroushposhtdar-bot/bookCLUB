// common/interfaces/IReviewRepository.cpp
#include "common/interfaces/IReviewRepository.h"
#include "common/models/Review.h"
#include "common/utils/IdGenerator.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

class ReviewRepositoryImpl : public IReviewRepository {
public:
    ReviewRepositoryImpl() = default;
    ~ReviewRepositoryImpl() override = default;

    Review* findById(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT * FROM Reviews WHERE id = ?", {id});
        if (query.next()) {
            return createReviewFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Review*> findByBook(const QString& bookId) const override {
        QVector<Review*> reviews;
        QSqlQuery query = getQuery(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (query.next()) {
            reviews.append(createReviewFromCurrentRecord(query));
        }
        return reviews;
    }

    QVector<Review*> findByUser(const QString& userId) const override {
        QVector<Review*> reviews;
        QSqlQuery query = getQuery(
            "SELECT * FROM Reviews WHERE userId = ? ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            reviews.append(createReviewFromCurrentRecord(query));
        }
        return reviews;
    }

    bool save(Review* review) override {
        if (!review) return false;
        if (review->id().isEmpty()) {
            review->setId(IdGenerator::generateUuid());
        }

        QString sql = R"(
            INSERT INTO Reviews (
                id, bookId, userId, userDisplayName, text, stars,
                createdAt, updatedAt, isEdited
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getQuery(sql, {
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

        if (query.lastError().type() == QSqlError::NoError) {
            updateBookRating(review->bookId());
            return true;
        }
        return false;
    }

    bool update(Review* review) override {
        if (!review || review->id().isEmpty()) return false;

        QString sql = R"(
            UPDATE Reviews SET
                text = ?,
                stars = ?,
                updatedAt = ?,
                isEdited = 1
            WHERE id = ?
        )";

        QSqlQuery query = getQuery(sql, {
            review->text(),
            review->stars(),
            QDateTime::currentDateTime(),
            review->id()
        });

        if (query.lastError().type() == QSqlError::NoError) {
            updateBookRating(review->bookId());
            return true;
        }
        return false;
    }

    bool remove(const QString& id) override {
        // Get bookId before deletion to update rating
        QString bookId;
        QSqlQuery findQuery = getQuery("SELECT bookId FROM Reviews WHERE id = ?", {id});
        if (findQuery.next()) {
            bookId = findQuery.value(0).toString();
        }

        QSqlQuery query = getQuery("DELETE FROM Reviews WHERE id = ?", {id});
        bool ok = query.lastError().type() == QSqlError::NoError;
        if (ok && !bookId.isEmpty()) {
            updateBookRating(bookId);
        }
        return ok;
    }

    double averageRating(const QString& bookId) const override {
        QSqlQuery query = getQuery(
            "SELECT AVG(stars) FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (query.next()) {
            return query.value(0).toDouble();
        }
        return 0.0;
    }

    int ratingCount(const QString& bookId) const override {
        QSqlQuery query = getQuery(
            "SELECT COUNT(*) FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (query.next()) {
            return query.value(0).toInt();
        }
        return 0;
    }

private:
    // ====== Helper methods ======
    QSqlQuery getQuery(const QString& sql, const QVariantList& params = {}) const {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);
        query.prepare(sql);
        for (const auto& param : params) {
            query.addBindValue(param);
        }
        query.exec();
        return query;
    }

    QSqlDatabase getDatabase() const {
        static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if (!db.isOpen()) {
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        return db;
    }

    Review* createReviewFromCurrentRecord(const QSqlQuery& query) const {
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

    void updateBookRating(const QString& bookId) const {
        QSqlQuery query = getQuery(
            "SELECT AVG(stars) as avg, COUNT(*) as cnt FROM Reviews WHERE bookId = ?",
            {bookId}
        );
        if (query.next()) {
            double avg = query.value("avg").toDouble();
            int cnt = query.value("cnt").toInt();
            getQuery(
                "UPDATE Books SET averageRating = ?, ratingCount = ? WHERE id = ?",
                {avg, cnt, bookId}
            );
        }
    }
};

// ========== Factory function ==========
IReviewRepository* createReviewRepository() {
    static ReviewRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
