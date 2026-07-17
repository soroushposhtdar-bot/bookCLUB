// common/interfaces/IBookService.cpp
#include "common/Interfaces/IBookService.h"
#include "common/Models/Book.h"
#include "common/Models/Review.h"
#include "common/Models/Cart.h"
#include "common/Models/Order.h"
#include "common/Models/Discount.h"
#include "common/Models/StudySession.h"
#include "common/Models/PublisherStats.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IOrderRepository.h"
#include "common/Interfaces/IReviewRepository.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include <QDebug>

namespace bookclub::common {

class BookServiceImpl : public IBookService {
public:
    explicit BookServiceImpl(
        IBookRepository* bookRepo,
        IUserRepository* userRepo,
        IOrderRepository* orderRepo,
        IReviewRepository* reviewRepo
    ) : m_bookRepo(bookRepo),
        m_userRepo(userRepo),
        m_orderRepo(orderRepo),
        m_reviewRepo(reviewRepo)
    {
        Q_ASSERT(m_bookRepo && m_userRepo && m_orderRepo && m_reviewRepo);
    }

    ~BookServiceImpl() override = default;

    QVector<Book*> listFeaturedBooks() const override {
        // For demo, return most popular books based on sales
        QVector<Book*> all = m_bookRepo->findAll();
        std::sort(all.begin(), all.end(), [](Book* a, Book* b) {
            return a->totalSales() > b->totalSales();
        });
        if (all.size() > 10) all.resize(10);
        return all;
    }

    QVector<Book*> listNewBooks() const override {
        QVector<Book*> all = m_bookRepo->findAll();
        std::sort(all.begin(), all.end(), [](Book* a, Book* b) {
            return a->createdAt() > b->createdAt();
        });
        if (all.size() > 10) all.resize(10);
        return all;
    }

    QVector<Book*> listBestSellers() const override {
        QVector<Book*> all = m_bookRepo->findAll();
        std::sort(all.begin(), all.end(), [](Book* a, Book* b) {
            return a->totalSales() > b->totalSales();
        });
        if (all.size() > 5) all.resize(5);
        return all;
    }

    QVector<Book*> listFreeBooks() const override {
        QVector<Book*> all = m_bookRepo->findAll();
        QVector<Book*> free;
        for (Book* book : all) {
            if (book->isFree() && book->isActive()) {
                free.append(book);
            }
        }
        return free;
    }

    QVector<Book*> listRecommendedBooks(const QStringList& favoriteGenres) const override {
        if (favoriteGenres.isEmpty()) {
            return listFeaturedBooks();
        }

        QVector<Book*> all = m_bookRepo->findAll();
        QVector<Book*> recommended;
        for (Book* book : all) {
            if (!book->isActive()) continue;
            for (const QString& genre : favoriteGenres) {
                if (book->genreIds().contains(genre)) {
                    recommended.append(book);
                    break;
                }
            }
        }
        // Sort by rating
        std::sort(recommended.begin(), recommended.end(), [](Book* a, Book* b) {
            return a->averageRating() > b->averageRating();
        });
        if (recommended.size() > 20) recommended.resize(20);
        return recommended;
    }

    QVector<Book*> searchBooks(const QString& keyword) const override {
        return m_bookRepo->searchByTitle(keyword);
    }

    QVector<Book*> searchBooksByField(const QString& keyword, SearchField field) const override {
        switch (field) {
            case SearchField::Title:
                return m_bookRepo->searchByTitle(keyword);
            case SearchField::Author:
                return m_bookRepo->searchByAuthor(keyword);
            case SearchField::Publisher:
                return m_bookRepo->searchByPublisherName(keyword);
            case SearchField::Genre:
                // For genre, we need to find genre ID first
                // For simplicity, just return all
                return m_bookRepo->findAll();
            case SearchField::All:
            default:
                return searchBooks(keyword);
        }
    }

    bool createBook(Book* book) override {
        if (!book) return false;
        return m_bookRepo->save(book);
    }

    bool updateBook(Book* book) override {
        if (!book) return false;
        return m_bookRepo->update(book);
    }

    bool deactivateBook(const QString& bookId) override {
        return m_bookRepo->deactivate(bookId);
    }

    bool activateBook(const QString& bookId) override {
        return m_bookRepo->activate(bookId);
    }

    bool applyDiscount(const QString& bookId, Discount* discount) override {
        if (!discount) return false;
        Book* book = m_bookRepo->findById(bookId);
        if (!book) return false;
        discount->setBookId(bookId);
        bool ok = m_bookRepo->attachDiscount(discount);
        delete book;
        return ok;
    }

    bool clearDiscount(const QString& bookId) override {
        // Delete discount from database
        // For simplicity, we'll just update the book's discount value
        Book* book = m_bookRepo->findById(bookId);
        if (!book) return false;
        book->clearDiscount();
        bool ok = m_bookRepo->update(book);
        delete book;
        return ok;
    }

    bool addReview(Review* review) override {
        if (!review) return false;
        return m_reviewRepo->save(review);
    }

    bool updateReview(Review* review) override {
        if (!review) return false;
        return m_reviewRepo->update(review);
    }

    bool removeReview(const QString& reviewId) override {
        return m_reviewRepo->remove(reviewId);
    }

    bool purchaseCart(Cart* cart, Order** createdOrder) override {
        if (!cart || cart->isEmpty()) return false;

        auto* order = new Order;
        order->setId(IdGenerator::generateUuid());
        order->setUserId(cart->userId());
        order->setSubtotal(cart->subtotal());
        order->setDiscountTotal(cart->discountTotal());
        order->setFinalTotal(cart->total());
        order->setPaid(false);
        order->setCompleted(false);

        // Add items from cart
        for (const auto* cartItem : cart->items()) {
            auto* orderItem = new OrderItem;
            orderItem->setBookId(cartItem->bookId());
            orderItem->setTitle(cartItem->bookTitle());
            orderItem->setUnitPrice(cartItem->unitPrice());
            orderItem->setQuantity(cartItem->quantity());
            order->addItem(orderItem);
        }

        // Note: we deliberately do NOT persist the order here.
        // CartRequestHandler::handleCheckout is responsible for marking
        // the order paid/completed and calling m_orderRepo->save once.
        *createdOrder = order;
        return true;
    }

    QVector<StudySession*> listActiveSessions(const QString& bookId) const override {
        // For demo, return empty list (this will be implemented in bonus section)
        return QVector<StudySession*>();
    }

    PublisherStats* publisherStats(const QString& publisherId) const override {
        auto* stats = new PublisherStats;
        stats->setPublisherId(publisherId);

        QVector<Book*> books = m_bookRepo->findByPublisher(publisherId);
        int totalSales = 0;
        double totalRevenue = 0.0;

        QVector<BookStatItem*> bookStats;
        for (Book* book : books) {
            auto* item = new BookStatItem;
            item->setBookId(book->id());
            item->setTitle(book->title());
            item->setSalesCount(book->totalSales());
            item->setRevenue(book->totalSales() * book->price());
            item->setAverageRating(book->averageRating());
            bookStats.append(item);

            totalSales += book->totalSales();
            totalRevenue += book->totalSales() * book->price();
        }

        stats->setBookStats(bookStats);
        stats->setTotalBooks(books.size());
        stats->setTotalSales(totalSales);
        stats->setTotalRevenue(totalRevenue);
        stats->setUpdatedAt(QDateTime::currentDateTime());

        qDeleteAll(books);
        return stats;
    }

private:
    IBookRepository* m_bookRepo;
    IUserRepository* m_userRepo;
    IOrderRepository* m_orderRepo;
    IReviewRepository* m_reviewRepo;
};

// ========== Factory function ==========
IBookService* createBookService(
    IBookRepository* bookRepo,
    IUserRepository* userRepo,
    IOrderRepository* orderRepo,
    IReviewRepository* reviewRepo
) {
    static BookServiceImpl service(bookRepo, userRepo, orderRepo, reviewRepo);
    return &service;
}

} // namespace bookclub::common
