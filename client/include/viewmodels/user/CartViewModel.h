// =============================================================================
//  CartViewModel.h
// =============================================================================
//  MVVM view-model for the Shopping Cart page.
//
//  Re-exposes CartService state as Q_PROPERTY for binding and orchestrates
//  checkout. On checkout success, emits checkoutSucceeded so the router can
//  push the success page (which then routes the user to their library).
// =============================================================================
#ifndef CARTVIEWMODEL_H
#define CARTVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class CartService;
class LibraryService;

class CartViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(CartService* cartService READ cartService WRITE setCartService NOTIFY cartServiceChanged)

    Q_PROPERTY(QList<QObject*> items READ items NOTIFY itemsChanged)
    Q_PROPERTY(int itemCount     READ itemCount     NOTIFY itemsChanged)
    Q_PROPERTY(double subtotal   READ subtotal      NOTIFY itemsChanged)
    Q_PROPERTY(double discountTotal READ discountTotal NOTIFY itemsChanged)
    Q_PROPERTY(double total      READ total         NOTIFY itemsChanged)
    Q_PROPERTY(QString subtotalText READ subtotalText NOTIFY itemsChanged)
    Q_PROPERTY(QString discountText  READ discountText  NOTIFY itemsChanged)
    Q_PROPERTY(QString totalText     READ totalText     NOTIFY itemsChanged)
    Q_PROPERTY(QString savingsText   READ savingsText   NOTIFY itemsChanged)
    Q_PROPERTY(bool isEmpty      READ isEmpty       NOTIFY itemsChanged)

public:
    explicit CartViewModel(QObject* parent = nullptr);

    CartService* cartService() const { return m_cartService; }
    void setCartService(CartService* s);

    QList<QObject*> items() const;
    int itemCount() const;
    double subtotal() const;
    double discountTotal() const;
    double total() const;
    QString subtotalText() const;
    QString discountText() const;
    QString totalText() const;
    QString savingsText() const;
    bool isEmpty() const { return itemCount() == 0; }

    Q_INVOKABLE void removeItem(const QString& bookId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void checkout();

signals:
    void cartServiceChanged();
    void itemsChanged();
    void checkoutSucceeded(const QStringList& purchasedBookIds);
    void checkoutFailed(const QString& error);

protected:
    void onAsyncReady() override;

private:
    CartService* m_cartService = nullptr;
    enum class PendingOp { None, Checkout };
    PendingOp m_pending = PendingOp::None;
    QStringList m_lastPurchased;
};

} // namespace bookclub::client

#endif // CARTVIEWMODEL_H
