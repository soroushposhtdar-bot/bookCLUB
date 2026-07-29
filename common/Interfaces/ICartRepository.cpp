// common/Interfaces/ICartRepository.cpp
//
// SQLite-backed ICartRepository. Cart items persist across sessions.
#include "common/Interfaces/ICartRepository.h"
#include "common/Models/Cart.h"
#include "common/Models/CartItem.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

class CartRepositoryImpl : public ICartRepository {
public:
    Cart* getOrCreateForUser(const QString& userId) override
    {
        auto* cart = new Cart;
        cart->setUserId(userId);

        // Ensure the cart row exists.
        DbConnection::execOk(
            "INSERT OR IGNORE INTO Carts (userId, updatedAt) VALUES (?, ?)",
            {userId, QDateTime::currentDateTime()}
        );

        // Load items joined with Books for title + price.
        auto q = DbConnection::run(
            "SELECT ci.bookId, ci.quantity, b.title, b.basePrice, b.discountValue "
            "FROM CartItems ci JOIN Books b ON b.id = ci.bookId "
            "WHERE ci.userId = ?",
            {userId}
        );
        while (q.next()) {
            auto* item = new CartItem(q.value(0).toString());
            item->setBookTitle(q.value(2).toString());
            item->setUnitPrice(q.value(3).toDouble());
            item->setDiscountedUnitPrice(q.value(3).toDouble() - q.value(4).toDouble());
            item->setQuantity(q.value(1).toInt());
            cart->addItem(item);
        }
        cart->recalculateTotal();
        return cart;
    }

    bool save(const QString& userId, const QVector<CartItem*>& items) override
    {
        if (!clear(userId)) return false;
        bool allOk = true;
        for (const CartItem* item : items) {
            if (!addItem(userId, item->bookId(), item->quantity())) allOk = false;
        }
        DbConnection::execOk(
            "UPDATE Carts SET updatedAt = ? WHERE userId = ?",
            {QDateTime::currentDateTime(), userId}
        );
        return allOk;
    }

    bool addItem(const QString& userId, const QString& bookId, int quantity) override
    {
        // v15: SET quantity = ? (not quantity + ?) so re-adding a book
        // that's already in the cart does NOT bump the quantity past 1.
        // Each book can only be in the cart once, with quantity = 1.
        const bool ok = DbConnection::execOk(
            "INSERT INTO CartItems (id, userId, bookId, quantity, addedAt) "
            "VALUES (?, ?, ?, ?, ?) "
            "ON CONFLICT(userId, bookId) DO UPDATE SET quantity = ?",
            {IdGenerator::generateUuid(), userId, bookId, quantity,
             QDateTime::currentDateTime(), quantity}
        );
        DbConnection::execOk(
            "UPDATE Carts SET updatedAt = ? WHERE userId = ?",
            {QDateTime::currentDateTime(), userId}
        );
        return ok;
    }

    bool updateItemQuantity(const QString& userId, const QString& bookId, int quantity) override
    {
        if (quantity <= 0) return removeItem(userId, bookId);
        return DbConnection::execOk(
            "UPDATE CartItems SET quantity = ? WHERE userId = ? AND bookId = ?",
            {quantity, userId, bookId}
        );
    }

    bool removeItem(const QString& userId, const QString& bookId) override
    {
        const bool ok = DbConnection::execOk(
            "DELETE FROM CartItems WHERE userId = ? AND bookId = ?",
            {userId, bookId}
        );
        DbConnection::execOk(
            "UPDATE Carts SET updatedAt = ? WHERE userId = ?",
            {QDateTime::currentDateTime(), userId}
        );
        return ok;
    }

    bool clear(const QString& userId) override
    {
        const bool ok = DbConnection::execOk(
            "DELETE FROM CartItems WHERE userId = ?", {userId}
        );
        DbConnection::execOk(
            "UPDATE Carts SET updatedAt = ? WHERE userId = ?",
            {QDateTime::currentDateTime(), userId}
        );
        return ok;
    }
};

ICartRepository* createCartRepository() {
    static CartRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
