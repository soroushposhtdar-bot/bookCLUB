// common/interfaces/IBookRepository.cpp
#include "common/interfaces/IBookRepository.h"
#include "common/models/Book.h"
#include "common/models/Review.h"
#include "common/models/Discount.h"
#include "common/utils/IdGenerator.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace bookclub::common {

// ========== کلاس پیاده‌ساز ==========
class BookRepositoryImpl : public IBookRepository {
public:
    BookRepositoryImpl() = default;
    ~BookRepositoryImpl() override = default;

    Book* findById(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT * FROM Books WHERE id = ?", {id});
        return createBookFromQuery(query);
    }

    QVector<Book*> findAll() const override {
        QVector<Book*> books;
        QSqlQuery query = getQuery("SELECT * FROM Books WHERE isActive = 1 ORDER BY title");
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> findByPublisher(const QString& publisherId) const override {
        QVector<Book*> books;
        QSqlQuery query = getQuery(
            "SELECT * FROM Books WHERE publisherId = ? ORDER BY createdAt DESC",
            {publisherId}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByTitle(const QString& title) const override {
        QVector<Book*> books;
        QString pattern = "%" + title + "%";
        QSqlQuery query = getQuery(
            "SELECT * FROM Books WHERE title LIKE ? AND isActive = 1",
            {pattern}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByAuthor(const QString& author) const override {
        QVector<Book*> books;
        QString pattern = "%" + author + "%";
        QSqlQuery query = getQuery(
            "SELECT * FROM Books WHERE authorName LIKE ? AND isActive = 1",
            {pattern}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByPublisherName(const QString& publisherName) const override {
        QVector<Book*> books;
        QString pattern = "%" + publisherName + "%";
        QSqlQuery query = getQuery(
            "SELECT b.* FROM Books b "
            "JOIN Users u ON b.publisherId = u.id "
            "WHERE u.displayName LIKE ? AND b.isActive = 1",
            {pattern}
        );
        while (query.next()) {
            Book* book = createBookFromCurrentRecord(query);
            if (book) books.append(book);
        }
        return books;
    }

    QVector<Book*> searchByGenreIds(const QStringList& genreIds) const override {
        QVector<Book*> books;
        if (genreIds.isEmpty()) return books;

        // Build IN clause
        QStringList placeholders;
        for (int i = 0; i < genreIds.size(); ++i) {
            placeholders << "?";
        }
        QString sql = QString(
            "SELECT * FROM Books WHERE genreIds LIKE ? AND isActive = 1"
        );

        // Since we store genreIds as JSON array in SQLite, we need to search in JSON
        // For simplicity, we'll use LIKE on the JSON string
        for (const QString& genreId : genreIds) {
            QString pattern = "%\"" + genreId + "\"%";
            QSqlQuery query = getQuery(sql, {pattern});
            while (query.next()) {
                Book* book = createBookFromCurrentRecord(query);
                if (book) books.append(book);
            }
        }
        return books;
    }

    bool save(Book* book) override {
        if (!book) return false;

        if (book->id().isEmpty()) {
            book->setId(IdGenerator::generateUuid());
        }

        QString sql = R"(
            INSERT INTO Books (
                id, title, authorName, publisherId, genreIds, description,
                coverImagePath, pdfFilePath, basePrice, discountValue,
                averageRating, ratingCount, totalSales, stockCount,
                isActive, visibility, availability, createdAt, updatedAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QJsonArray genreArray = QJsonArray::fromStringList(book->genreIds());
        QJsonDocument genreDoc(genreArray);

        QSqlQuery query = getQuery(sql, {
            book->id(),
            book->title(),
            book->authorName(),
            book->publisherId(),
            QString(genreDoc.toJson(QJsonDocument::Compact)),
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
            QDateTime::currentDateTime()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    bool update(Book* book) override {
        if (!book || book->id().isEmpty()) return false;

        QString sql = R"(
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

        QJsonArray genreArray = QJsonArray::fromStringList(book->genreIds());
        QJsonDocument genreDoc(genreArray);

        QSqlQuery query = getQuery(sql, {
            book->title(),
            book->authorName(),
            book->publisherId(),
            QString(genreDoc.toJson(QJsonDocument::Compact)),
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

        return query.lastError().type() == QSqlError::NoError;
    }

    bool remove(const QString& id) override {
        QSqlQuery query = getQuery("DELETE FROM Books WHERE id = ?", {id});
        return query.lastError().type() == QSqlError::NoError;
    }

    bool activate(const QString& id) override {
        QSqlQuery query = getQuery(
            "UPDATE Books SET isActive = 1, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
        return query.lastError().type() == QSqlError::NoError;
    }

    bool deactivate(const QString& id) override {
        QSqlQuery query = getQuery(
            "UPDATE Books SET isActive = 0, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
        return query.lastError().type() == QSqlError::NoError;
    }

    bool attachReview(Review* review) override {
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
            // Update book's average rating
            updateBookRating(review->bookId());
            return true;
        }
        return false;
    }

    bool attachDiscount(Discount* discount) override {
        if (!discount) return false;
        if (discount->id().isEmpty()) {
            discount->setId(IdGenerator::generateUuid());
        }

        QString sql = R"(
            INSERT INTO Discounts (
                id, bookId, type, value, startsAt, endsAt, isActive
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getQuery(sql, {
            discount->id(),
            discount->bookId(),
            static_cast<int>(discount->type()),
            discount->value(),
            discount->startsAt(),
            discount->endsAt(),
            discount->isActive() ? 1 : 0
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    QVector<Review*> reviewsOf(const QString& bookId) const override {
        QVector<Review*> reviews;
        QSqlQuery query = getQuery(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (query.next()) {
            Review* review = createReviewFromQuery(query);
            if (review) reviews.append(review);
        }
        return reviews;
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

    Book* createBookFromQuery(const QSqlQuery& query) const {
        if (!query.next()) return nullptr;
        return createBookFromCurrentRecord(query);
    }

    Book* createBookFromCurrentRecord(const QSqlQuery& query) const {
        QSqlRecord rec = query.record();
        auto* book = new Book;
        book->setId(rec.value("id").toString());
        book->setTitle(rec.value("title").toString());
        book->setAuthorName(rec.value("authorName").toString());
        book->setPublisherId(rec.value("publisherId").toString());

        // Parse genreIds from JSON
        QString genreJson = rec.value("genreIds").toString();
        if (!genreJson.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(genreJson.toUtf8());
            if (doc.isArray()) {
                QStringList genres;
                for (const auto& val : doc.array()) {
                    genres.append(val.toString());
                }
                book->setGenreIds(genres);
            }
        }

        book->setDescription(rec.value("description").toString());
        book->setCoverImagePath(rec.value("coverImagePath").toString());
        book->setPdfFilePath(rec.value("pdfFilePath").toString());
        book->setBasePrice(rec.value("basePrice").toDouble());
        book->setDiscountValue(rec.value("discountValue").toDouble());
        book->setAverageRating(rec.value("averageRating").toDouble());
        book->setRatingCount(rec.value("ratingCount").toInt());
        book->setTotalSales(rec.value("totalSales").toInt());
        book->setStockCount(rec.value("stockCount").toInt());
        book->setActive(rec.value("isActive").toInt() == 1);
        book->setVisibility(static_cast<BookVisibility>(rec.value("visibility").toInt()));
        book->setAvailability(static_cast<BookAvailability>(rec.value("availability").toInt()));
        book->setCreatedAt(rec.value("createdAt").toDateTime());
        book->setUpdatedAt(rec.value("updatedAt").toDateTime());
        book->recalculateSellingPrice();
        return book;
    }

    Review* createReviewFromQuery(const QSqlQuery& query) const {
        if (!query.next()) return nullptr;
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
IBookRepository* createBookRepository() {
    static BookRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
