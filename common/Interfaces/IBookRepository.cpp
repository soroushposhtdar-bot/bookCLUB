// common/Interfaces/IBookRepository.cpp
//
// Concrete `IBookRepository` backed by the shared SQLite database.
// Fixes from the previous version:
//   - Removed calls to `Book::setActive(bool)` (which does not exist);
//     we now route through `Book::activate()` / `Book::deactivate()`.
//   - Stopped calling non-const `QSqlQuery::next()` on a `const QSqlQuery&`.
//   - Replaced the private `QSqlDatabase` connection with the shared
//     "bookclub_shared" connection used across repositories.
//   - `searchByGenreIds` no longer prepares the same query N times; it
//     uses one query with an OR'd LIKE clause.

#include "common/Interfaces/IBookRepository.h"
#include "common/Models/Book.h"
#include "common/Models/Review.h"
#include "common/Models/Discount.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
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

QString genreIdsToJson(const QStringList& genreIds)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(genreIds)).toJson(QJsonDocument::Compact)
    );
}

QStringList genreIdsFromJson(const QString& json)
{
    QStringList result;
    if (json.isEmpty()) return result;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return result;
    for (const auto& v : doc.array()) {
        result.append(v.toString());
    }
    return result;
}
} // namespace

// ========== Implementation ==========
class BookRepositoryImpl : public IBookRepository {
public:
    BookRepositoryImpl() = default;
    ~BookRepositoryImpl() override = default;

    Book* findById(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Books WHERE id = ?", {id});
        return createBookFromQuery(query);
    }

    QVector<Book*> findAll() const override
    {
        QVector<Book*> books;
        QSqlQuery query = runQuery("SELECT * FROM Books WHERE isActive = 1 ORDER BY title");
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> findByPublisher(const QString& publisherId) const override
    {
        QVector<Book*> books;
        QSqlQuery query = runQuery(
            "SELECT * FROM Books WHERE publisherId = ? ORDER BY createdAt DESC",
            {publisherId}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByTitle(const QString& title) const override
    {
        QVector<Book*> books;
        QSqlQuery query = runQuery(
            "SELECT * FROM Books WHERE title LIKE ? AND isActive = 1",
            {"%" + title + "%"}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByAuthor(const QString& author) const override
    {
        QVector<Book*> books;
        QSqlQuery query = runQuery(
            "SELECT * FROM Books WHERE authorName LIKE ? AND isActive = 1",
            {"%" + author + "%"}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByPublisherName(const QString& publisherName) const override
    {
        QVector<Book*> books;
        QSqlQuery query = runQuery(
            "SELECT b.* FROM Books b "
            "JOIN Users u ON b.publisherId = u.id "
            "WHERE u.displayName LIKE ? AND b.isActive = 1",
            {"%" + publisherName + "%"}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByGenreIds(const QStringList& genreIds) const override
    {
        QVector<Book*> books;
        if (genreIds.isEmpty()) return books;

        // genreIds is stored as JSON in Books.genreIds. We OR a LIKE clause
        // for each requested id and deduplicate via a QSet.
        QStringList orClauses;
        QVariantList binds;
        for (const QString& g : genreIds) {
            orClauses << "genreIds LIKE ?";
            binds << QString("%\"%1\"%").arg(g);
        }
        const QString sql = QString(
            "SELECT * FROM Books WHERE isActive = 1 AND (%1)"
        ).arg(orClauses.join(" OR "));

        QSqlQuery query = runQuery(sql, binds);
        QSet<QString> seen;
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (!book) continue;
            if (seen.contains(book->id())) {
                delete book;
                continue;
            }
            seen.insert(book->id());
            books.append(book);
        }
        return books;
    }

    bool save(Book* book) override
    {
        if (!book) return false;
        if (book->id().isEmpty()) {
            book->setId(IdGenerator::generateUuid());
        }
        if (!book->createdAt().isValid()) {
            book->setCreatedAt(QDateTime::currentDateTime());
        }
        book->setUpdatedAt(QDateTime::currentDateTime());

        const QString sql = R"(
            INSERT INTO Books (
                id, title, authorName, publisherId, genreIds, description,
                coverImagePath, pdfFilePath, basePrice, discountValue,
                averageRating, ratingCount, totalSales, stockCount,
                isActive, visibility, availability, createdAt, updatedAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        return execOk(sql, {
            book->id(),
            book->title(),
            book->authorName(),
            book->publisherId(),
            genreIdsToJson(book->genreIds()),
            book->description(),
            book->coverImagePath(),
            book->pdfFilePath(),
            book->basePrice(),
            book->discountValue(),
            book->averageRating(),
            book->ratingCount(),
            book->totalSales(),
            book->stockCount(),
            book->isActive() ? 1 : 0,
            static_cast<int>(book->visibility()),
            static_cast<int>(book->availability()),
            book->createdAt(),
            book->updatedAt()
        });
    }

    bool update(Book* book) override
    {
        if (!book || book->id().isEmpty()) return false;

        const QString sql = R"(
            UPDATE Books SET
                title = ?,
                authorName = ?,
                publisherId = ?,
                genreIds = ?,
                description = ?,
                coverImagePath = ?,
                pdfFilePath = ?,
                basePrice = ?,
                discountValue = ?,
                averageRating = ?,
                ratingCount = ?,
                totalSales = ?,
                stockCount = ?,
                isActive = ?,
                visibility = ?,
                availability = ?,
                updatedAt = ?
            WHERE id = ?
        )";

        return execOk(sql, {
            book->title(),
            book->authorName(),
            book->publisherId(),
            genreIdsToJson(book->genreIds()),
            book->description(),
            book->coverImagePath(),
            book->pdfFilePath(),
            book->basePrice(),
            book->discountValue(),
            book->averageRating(),
            book->ratingCount(),
            book->totalSales(),
            book->stockCount(),
            book->isActive() ? 1 : 0,
            static_cast<int>(book->visibility()),
            static_cast<int>(book->availability()),
            QDateTime::currentDateTime(),
            book->id()
        });
    }

    bool remove(const QString& id) override
    {
        return execOk("DELETE FROM Books WHERE id = ?", {id});
    }

    bool activate(const QString& id) override
    {
        return execOk(
            "UPDATE Books SET isActive = 1, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
    }

    bool deactivate(const QString& id) override
    {
        return execOk(
            "UPDATE Books SET isActive = 0, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
    }

    bool attachReview(Review* review) override
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

        if (ok) {
            updateBookRating(review->bookId());
        }
        return ok;
    }

    bool attachDiscount(Discount* discount) override
    {
        if (!discount) return false;
        if (discount->id().isEmpty()) {
            discount->setId(IdGenerator::generateUuid());
        }

        const QString sql = R"(
            INSERT INTO Discounts (
                id, bookId, type, value, startsAt, endsAt, isActive
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        )";

        return execOk(sql, {
            discount->id(),
            discount->bookId(),
            static_cast<int>(discount->type()),
            discount->value(),
            discount->startsAt(),
            discount->endsAt(),
            discount->isActive() ? 1 : 0
        });
    }

    QVector<Review*> reviewsOf(const QString& bookId) const override
    {
        QVector<Review*> reviews;
        QSqlQuery query = runQuery(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (query.next()) {
            Review* review = createReviewFromCurrentRecord(query);
            if (review) reviews.append(review);
        }
        return reviews;
    }

private:
    Book* createBookFromQuery(QSqlQuery& query) const
    {
        if (!query.next()) return nullptr;
        return createBookFromCurrentRecord(query);
    }

    Book* createBookFromCurrentRecord(QSqlQuery& query) const
    {
        QSqlRecord rec = query.record();
        auto* book = new Book;
        book->setId(rec.value("id").toString());
        book->setTitle(rec.value("title").toString());
        book->setAuthorName(rec.value("authorName").toString());
        book->setPublisherId(rec.value("publisherId").toString());
        book->setGenreIds(genreIdsFromJson(rec.value("genreIds").toString()));
        book->setDescription(rec.value("description").toString());
        book->setCoverImagePath(rec.value("coverImagePath").toString());
        book->setPdfFilePath(rec.value("pdfFilePath").toString());
        book->setBasePrice(rec.value("basePrice").toDouble());
        book->setDiscountValue(rec.value("discountValue").toDouble());
        book->setAverageRating(rec.value("averageRating").toDouble());
        book->setRatingCount(rec.value("ratingCount").toInt());
        book->setTotalSales(rec.value("totalSales").toInt());
        book->setStockCount(rec.value("stockCount").toInt());

        // Book has activate()/deactivate() but no setActive(bool) — route accordingly.
        if (rec.value("isActive").toInt() == 1) {
            book->activate();
        } else {
            book->deactivate();
        }

        book->setVisibility(static_cast<BookVisibility>(rec.value("visibility").toInt()));
        book->setAvailability(static_cast<BookAvailability>(rec.value("availability").toInt()));
        book->setCreatedAt(rec.value("createdAt").toDateTime());
        book->setUpdatedAt(rec.value("updatedAt").toDateTime());
        book->recalculateSellingPrice();
        return book;
    }

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
IBookRepository* createBookRepository() {
    static BookRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
