// common/Interfaces/ICartRepository.h
//
// Persistent cart repository. One cart per user; cart items stored as rows.
#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class Cart;
class CartItem;

class ICartRepository {
public:
    virtual ~ICartRepository() = default;

    virtual Cart* getOrCreateForUser(const QString& userId) = 0;
    virtual bool save(const QString& userId, const QVector<CartItem*>& items) = 0;
    virtual bool addItem(const QString& userId, const QString& bookId, int quantity) = 0;
    virtual bool updateItemQuantity(const QString& userId, const QString& bookId, int quantity) = 0;
    virtual bool removeItem(const QString& userId, const QString& bookId) = 0;
    virtual bool clear(const QString& userId) = 0;
};

ICartRepository* createCartRepository();

} // namespace bookclub::common
