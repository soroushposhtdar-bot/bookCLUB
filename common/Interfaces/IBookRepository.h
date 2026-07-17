#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace bookclub::common {

class Book;
class Review;
class Discount;

class IBookRepository {
public:
    virtual ~IBookRepository() = default;

    virtual Book* findById(const QString& id) const = 0;
    virtual QVector<Book*> findAll() const = 0;
    virtual QVector<Book*> findByPublisher(const QString& publisherId) const = 0;
    virtual QVector<Book*> searchByTitle(const QString& title) const = 0;
    virtual QVector<Book*> searchByAuthor(const QString& author) const = 0;
    virtual QVector<Book*> searchByPublisherName(const QString& publisherName) const = 0;
    virtual QVector<Book*> searchByGenreIds(const QStringList& genreIds) const = 0;
    virtual bool save(Book* book) = 0;
    virtual bool update(Book* book) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool activate(const QString& id) = 0;
    virtual bool deactivate(const QString& id) = 0;
    virtual bool attachReview(Review* review) = 0;
    virtual bool attachDiscount(Discount* discount) = 0;
    virtual QVector<Review*> reviewsOf(const QString& bookId) const = 0;
};

IBookRepository* createBookRepository();

} // namespace bookclub::common
