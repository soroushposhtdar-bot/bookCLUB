#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class CartController : public QObject {
    Q_OBJECT
public:
    explicit CartController(QObject* parent = nullptr);
    ~CartController() override = default;

    void addBook(const QString& bookId);
    void removeBook(const QString& bookId);
    void updateQuantity(const QString& bookId, int quantity);
    void clearCart();
    void applyDiscountCode(const QString& code);
    void checkout();

signals:
    void cartChanged();
    void totalsChanged();
    void checkoutStarted();
    void checkoutSucceeded(const QString& orderId);
    void checkoutFailed(const QString& message);

private:
    QString m_discountCode;
};

} // namespace bookclub::client
