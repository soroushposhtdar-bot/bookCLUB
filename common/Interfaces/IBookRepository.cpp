// common/Interfaces/IBookRepository.cpp
//
// SQLite-backed IBookRepository.
// Uses common::DbConnection for all DB access.
//
// Full CRUD: save (INSERT), update, remove, activate, deactivate,
// findById, findAll, findByPublisher, searchByTitle/Author/PublisherName/GenreIds,
// reviewsOf, attachReview, attachDiscount.
#include "common/Interfaces/IBookRepository.h"
#include "common/Models/Book.h"
#include "common/Models/Review.h"
#include "common/Models/Discount.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
QStringList genreIdsOfBook(const QString& bookId)
{
    QStringList ids;
    auto q = DbConnection::run(
        "SELECT g.id FROM Genres g "
        "JOIN BookGenres bg ON bg.genreId = g.id "
        "WHERE bg.bookId = ?",
        {bookId}
    );
    while (q.next()) ids.append(q.value(0).toString());
    return ids;
}

Book* bookFromCurrentRecord(QSqlQuery& query)
{
    QSqlRecord rec = query.record();
    auto* book = new Book;
    book->setId(rec.value("id").toString());
    book->setTitle(rec.value("title").toString());
    book->setAuthorName(rec.value("authorName").toString());
    book->setPublisherId(rec.value("publisherId").toString());
    book->setDescription(rec.value("description").toString());
    book->setCoverImagePath(rec.value("coverImagePath").toString());
    book->setPdfFilePath(rec.value("pdfFilePath").toString());
    book->setBasePrice(rec.value("basePrice").toDouble());
    book->setDiscountValue(rec.value("discountValue").toDouble());
    book->setAverageRating(rec.value("averageRating").toDouble());
    book->setRatingCount(rec.value("ratingCount").toInt());
    book->setTotalSales(rec.value("totalSales").toInt());
    book->setStockCount(rec.value("stockCount").toInt());
    book->setCreatedAt(rec.value("createdAt").toDateTime());
    book->setUpdatedAt(rec.value("updatedAt").toDateTime());
    book->setVisibility(static_cast<BookVisibility>(rec.value("visibility").toInt()));
    book->setAvailability(static_cast<BookAvailability>(rec.value("availability").toInt()));
    if (rec.value("isActive").toInt() == 0) book->deactivate();
    book->setGenreIds(genreIdsOfBook(book->id()));
    return book;
}

bool syncBookGenres(const QString& bookId, const QStringList& genreIds)
{
    DbConnection::execOk("DELETE FROM BookGenres WHERE bookId = ?", {bookId});
    for (const QString& gid : genreIds) {
        DbConnection::execOk(
            "INSERT OR IGNORE INTO BookGenres (bookId, genreId) VALUES (?, ?)",
            {bookId, gid}
        );
    }
    return true;
}
} // namespace

// ============== Implementation ==============
class BookRepositoryImpl : public IBookRepository {
public:
    Book* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Books WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return bookFromCurrentRecord(q);
    }

    QVector<Book*> findAll() const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run("SELECT * FROM Books WHERE isActive = 1 ORDER BY createdAt DESC");
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    // BUG FIX (admin books hidden): returns ALL books regardless of isActive
    // status. Used by the admin books page so the admin can see pending,
    // inactive, and removed books too.
    QVector<Book*> findAllIncludingInactive() const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run("SELECT * FROM Books ORDER BY createdAt DESC");
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    QVector<Book*> findByPublisher(const QString& publisherId) const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run(
            "SELECT * FROM Books WHERE publisherId = ? ORDER BY createdAt DESC",
            {publisherId}
        );
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    QVector<Book*> searchByTitle(const QString& title) const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run(
            "SELECT * FROM Books WHERE isActive = 1 AND title LIKE ? ORDER BY title",
            {"%" + title + "%"}
        );
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    QVector<Book*> searchByAuthor(const QString& author) const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run(
            "SELECT * FROM Books WHERE isActive = 1 AND authorName LIKE ? ORDER BY title",
            {"%" + author + "%"}
        );
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    QVector<Book*> searchByPublisherName(const QString& publisherName) const override
    {
        QVector<Book*> books;
        auto q = DbConnection::run(
            "SELECT b.* FROM Books b "
            "JOIN Users u ON u.id = b.publisherId "
            "WHERE b.isActive = 1 AND u.displayName LIKE ? "
            "ORDER BY b.title",
            {"%" + publisherName + "%"}
        );
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    QVector<Book*> searchByGenreIds(const QStringList& genreIds) const override
    {
        QVector<Book*> books;
        if (genreIds.isEmpty()) return books;
        QStringList placeholders;
        for (int i = 0; i < genreIds.size(); ++i) placeholders << "?";
        const QString sql = QStringLiteral(
            "SELECT DISTINCT b.* FROM Books b "
            "JOIN BookGenres bg ON bg.bookId = b.id "
            "WHERE b.isActive = 1 AND bg.genreId IN (%1) "
            "ORDER BY b.title"
        ).arg(placeholders.join(","));
        auto q = DbConnection::run(sql, QVariantList(genreIds.begin(), genreIds.end()));
        while (q.next()) books.append(bookFromCurrentRecord(q));
        return books;
    }

    bool save(Book* book) override
    {
        if (!book) return false;
        if (book->id().isEmpty()) book->setId(IdGenerator::generateUuid());
        if (!book->createdAt().isValid()) book->setCreatedAt(QDateTime::currentDateTime());
        book->setUpdatedAt(QDateTime::currentDateTime());

        const QString sql = QStringLiteral(
            "INSERT INTO Books ("
            "  id, title, authorName, publisherId, description, coverImagePath,"
            "  pdfFilePath, basePrice, discountValue, averageRating, ratingCount,"
            "  totalSales, stockCount, isActive, visibility, availability,"
            "  createdAt, updatedAt"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

        const bool ok = DbConnection::execOk(sql, {
            book->id(), book->title(), book->authorName(), book->publisherId(),
            book->description(), book->coverImagePath(), book->pdfFilePath(),
            book->basePrice(), book->discountValue(),
            book->averageRating(), book->ratingCount(),
            book->totalSales(), book->stockCount(),
            book->isActive() ? 1 : 0,
            static_cast<int>(book->visibility()),
            static_cast<int>(book->availability()),
            book->createdAt(), book->updatedAt()
        });

        if (!ok) {
            LOG_ERROR("Failed to save book: " + book->title()
                      + " | " + DbConnection::lastErrorText());
            return false;
        }
        return syncBookGenres(book->id(), book->genreIds());
    }

    bool update(Book* book) override
    {
        if (!book || book->id().isEmpty()) return false;
        book->setUpdatedAt(QDateTime::currentDateTime());

        const QString sql = QStringLiteral(
            "UPDATE Books SET "
            "  title=?, authorName=?, publisherId=?, description=?, coverImagePath=?,"
            "  pdfFilePath=?, basePrice=?, discountValue=?, averageRating=?,"
            "  ratingCount=?, totalSales=?, stockCount=?, isActive=?, visibility=?,"
            "  availability=?, updatedAt=? "
            "WHERE id=?"
        );

        const bool ok = DbConnection::execOk(sql, {
            book->title(), book->authorName(), book->publisherId(),
            book->description(), book->coverImagePath(), book->pdfFilePath(),
            book->basePrice(), book->discountValue(),
            book->averageRating(), book->ratingCount(),
            book->totalSales(), book->stockCount(),
            book->isActive() ? 1 : 0,
            static_cast<int>(book->visibility()),
            static_cast<int>(book->availability()),
            book->updatedAt(),
            book->id()
        });

        if (!ok) {
            LOG_ERROR("Failed to update book: " + book->id());
            return false;
        }
        return syncBookGenres(book->id(), book->genreIds());
    }

    bool remove(const QString& id) override
    {
        // Soft delete: keep the row so purchased books remain accessible.
        return DbConnection::execOk(
            "UPDATE Books SET isActive = 0, availability = 2, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
    }

    bool activate(const QString& id) override
    {
        return DbConnection::execOk(
            "UPDATE Books SET isActive = 1, availability = 0, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
    }

    bool deactivate(const QString& id) override
    {
        return DbConnection::execOk(
            "UPDATE Books SET isActive = 0, availability = 1, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
    }

    bool attachReview(Review* review) override
    {
        // Reviews are owned by IReviewRepository; this is a no-op kept for
        // interface compatibility. The book's averageRating is recalculated
        // by IReviewRepository::save() via recalculateBookRating().
        Q_UNUSED(review);
        return true;
    }

    bool attachDiscount(Discount* discount) override
    {
        if (!discount) return false;
        // Delegated to IDiscountRepository; here we just apply the immediate
        // discountValue to the Books row so price queries are correct.
        return DbConnection::execOk(
            "UPDATE Books SET discountValue = ?, updatedAt = ? WHERE id = ?",
            {discount->value(), QDateTime::currentDateTime(), discount->bookId()}
        );
    }

    QVector<Review*> reviewsOf(const QString& bookId) const override
    {
        QVector<Review*> reviews;
        auto q = DbConnection::run(
            "SELECT * FROM Reviews WHERE bookId = ? ORDER BY createdAt DESC",
            {bookId}
        );
        while (q.next()) {
            auto* r = new Review;
            r->setId(q.value("id").toString());
            r->setBookId(q.value("bookId").toString());
            r->setUserId(q.value("userId").toString());
            r->setUserDisplayName(q.value("userDisplayName").toString());
            r->setText(q.value("text").toString());
            r->setStars(q.value("stars").toInt());
            r->setEdited(q.value("isEdited").toInt() == 1);
            r->setCreatedAt(q.value("createdAt").toDateTime());
            r->setUpdatedAt(q.value("updatedAt").toDateTime());
            reviews.append(r);
        }
        return reviews;
    }
};

// ============== Factory ==============
IBookRepository* createBookRepository() {
    static BookRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
