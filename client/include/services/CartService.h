// =============================================================================
//  CartService.h
// =============================================================================
//  Mocked shopping cart for the Regular User role.
//
//  Real-backend mapping (see common/Network/Protocol.h):
//      cart()               → REQ_CART_GET        → RES_CART
//      addToCart(bookId)    → REQ_CART_ADD        → RES_CART
//      removeFromCart(bookId) → REQ_CART_REMOVE   → RES_CART
//      clearCart()          → REQ_CART_CLEAR      → RES_CART
//      checkout()           → REQ_CART_CHECKOUT   → RES_ORDER
//
//  In the mock, the cart is an in-memory list of MockCartItem rows. After a
//  successful checkout, purchased books move into the user's library (handled
//  by MockDataStore::addPurchase) and the cart clears itself.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>

#include "services/MockDataStore.h"
#include "services/MockTypes.h"

namespace bookclub::client {

class CartService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int     itemCount      READ itemCount      NOTIFY cartChanged)
    Q_PROPERTY(double  subtotal       READ subtotal       NOTIFY cartChanged)
    Q_PROPERTY(double  discountTotal  READ discountTotal  NOTIFY cartChanged)
    Q_PROPERTY(double  total          READ total          NOTIFY cartChanged)
    Q_PROPERTY(QString subtotalText   READ subtotalText   NOTIFY cartChanged)
    Q_PROPERTY(QString discountText   READ discountText   NOTIFY cartChanged)
    Q_PROPERTY(QString totalText      READ totalText      NOTIFY cartChanged)
    Q_PROPERTY(QString savingsText    READ savingsText    NOTIFY cartChanged)

public:
    explicit CartService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    int itemCount() const { return m_items.size(); }
    double subtotal() const;
    double discountTotal() const;
    double total() const;
    QString subtotalText() const;
    QString discountText() const;
    QString totalText() const;
    QString savingsText() const;

    // Returns the current cart rows as QObject* (CartItemDto is declared in
    // the .cpp via a small inline DTO; QML reads it through Q_PROPERTY).
    Q_INVOKABLE QList<QObject*> items() const;

    Q_INVOKABLE bool isInCart(const QString& bookId) const;
    Q_INVOKABLE void add(const QString& bookId);
    Q_INVOKABLE void remove(const QString& bookId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool checkout();

signals:
    void cartChanged();
    void checkoutSucceeded(const QStringList& purchasedBookIds);
    void checkoutFailed(const QString& error);

private:
    MockDataStore* m_store = nullptr;
    QList<MockCartItem> m_items;

    void _rebuildFromBooks();   // refresh cover/price from the store
};

} // namespace bookclub::client
