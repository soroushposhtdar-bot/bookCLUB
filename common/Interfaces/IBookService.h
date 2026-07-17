#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include "common/AppEnums.h"

namespace bookclub::common {

class Book;
class Review;
class Cart;
class Order;
class Discount;
class StudySession;
class PublisherStats;

class IBookRepository;
class IUserRepository;
class IOrderRepository;
class IReviewRepository;

class IBookService {
public:
    virtual ~IBookService() = default;

    virtual QVector<Book*> listFeaturedBooks() const = 0;
    virtual QVector<Book*> listNewBooks() const = 0;
    virtual QVector<Book*> listBestSellers() const = 0;
    virtual QVector<Book*> listFreeBooks() const = 0;
    virtual QVector<Book*> listRecommendedBooks(const QStringList& favoriteGenres) const = 0;
    virtual QVector<Book*> searchBooks(const QString& keyword) const = 0;
    virtual QVector<Book*> searchBooksByField(const QString& keyword, SearchField field) const = 0;

    virtual bool createBook(Book* book) = 0;
    virtual bool updateBook(Book* book) = 0;
    virtual bool deactivateBook(const QString& bookId) = 0;
    virtual bool activateBook(const QString& bookId) = 0;
    virtual bool applyDiscount(const QString& bookId, Discount* discount) = 0;
    virtual bool clearDiscount(const QString& bookId) = 0;

    virtual bool addReview(Review* review) = 0;
    virtual bool updateReview(Review* review) = 0;
    virtual bool removeReview(const QString& reviewId) = 0;

    virtual bool purchaseCart(Cart* cart, Order** createdOrder) = 0;
    virtual QVector<StudySession*> listActiveSessions(const QString& bookId) const = 0;
    virtual PublisherStats* publisherStats(const QString& publisherId) const = 0;
};

IBookService* createBookService(IBookRepository* bookRepo, IUserRepository* userRepo, IOrderRepository* orderRepo, IReviewRepository* reviewRepo);

} // namespace bookclub::common
