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

// Include full service headers so MOC sees complete types for Q_PROPERTY pointers.
#include "services/CartService.h"
#include "services/LibraryService.h"

namespace bookclub::client {


class CartViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(CartService* cartService READ cartService WRITE setCartService NOTIFY cartServiceChanged)

    Q_PROPERTY(QList<QObject*> items READ items NOTIFY itemsChanged)
    Q_PROPERTY(int itemCount     READ itemCount     NOTIFY itemsChanged)
    Q_PROPERTY(double subtotal   READ subtotal      NOTIFY itemsChanged)
    Q_PROPERTY(double discountTotal READ discountTotal NOTIFY itemsChanged)
    Q_PROPERTY(double purchaseProfit READ purchaseProfit NOTIFY itemsChanged)
    Q_PROPERTY(QString purchaseProfitText READ purchaseProfitText NOTIFY itemsChanged)
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
    double purchaseProfit() const;
    QString purchaseProfitText() const;
    double total() const;
    QString subtotalText() const;
    QString discountText() const;
    QString totalText() const;
    QString savingsText() const;
    bool isEmpty() const { return m_cartService ? m_cartService->isEmpty() : true; }

    Q_INVOKABLE void removeItem(const QString& bookId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void checkout();
    Q_INVOKABLE void setQuantity(const QString& bookId, int quantity) { if (m_cartService) m_cartService->setQuantity(bookId, quantity); }  // Issue 12
    Q_INVOKABLE void refresh() { if (m_cartService) m_cartService->refresh(); }  // Issue 12

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
