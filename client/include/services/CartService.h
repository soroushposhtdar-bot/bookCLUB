#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>
#include <QJsonObject>

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
    // v15c: sum of per-item discounts (basePrice - discountedUnitPrice) * quantity
    // across all cart items. This is the "purchase profit" — how much the user
    // saves from per-book discounts, BEFORE any promo code. It's distinct from
    // `discountTotal` which only reflects promo-code discounts.
    Q_PROPERTY(double  purchaseProfit READ purchaseProfit NOTIFY cartChanged)
    Q_PROPERTY(QString purchaseProfitText READ purchaseProfitText NOTIFY cartChanged)

public:
    explicit CartService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op (server-backed)

    int itemCount() const;
    double subtotal() const;
    double discountTotal() const;
    double total() const;
    QString subtotalText() const;
    QString discountText() const;
    QString totalText() const;
    QString savingsText() const;
    // v15c: per-item discount sum (purchase profit).
    double purchaseProfit() const;
    QString purchaseProfitText() const;

    Q_INVOKABLE QList<QObject*> items() const;
    Q_INVOKABLE bool isInCart(const QString& bookId) const;
    Q_INVOKABLE bool isEmpty() const;
    Q_INVOKABLE void add(const QString& bookId);
    Q_INVOKABLE void remove(const QString& bookId);
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool checkout();
    Q_INVOKABLE void applyDiscount(const QString& code);  // v11: promo code
    Q_INVOKABLE void refresh();          // Issue 12
    Q_INVOKABLE void setQuantity(const QString& bookId, int quantity);  // Issue 12

signals:
    void cartChanged();
    void checkoutSucceeded(const QStringList& purchasedBookIds);
    void checkoutFailed(const QString& error);

private:
    // Local cache of cart state (refreshed from server)
    mutable QJsonObject m_cartData;

    void refreshFromServer() const;
    QList<QObject*> parseItems(const QJsonArray& arr) const;
};

} // namespace bookclub::client
