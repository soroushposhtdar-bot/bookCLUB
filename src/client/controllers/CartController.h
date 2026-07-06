// src/client/controllers/CartController.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class CartController : public QObject {
    Q_OBJECT
public:
    explicit CartController(QObject* parent = nullptr);
    ~CartController() override;

    void addBook(const QString& bookId);
    void addBookWithQuantity(const QString& bookId, int quantity);
    void removeBook(const QString& bookId);
    void updateQuantity(const QString& bookId, int quantity);
    void clearCart();
    void applyDiscountCode(const QString& code);
    void checkout();

    // ---- Accessors ----
    double getTotal() const;
    double getSubtotal() const;
    double getDiscountTotal() const;
    int getItemCount() const;
    QJsonArray getItems() const;

signals:
    void cartChanged();
    void totalsChanged();
    void checkoutStarted();
    void checkoutSucceeded(const QString& orderId);
    void checkoutFailed(const QString& message);
    void errorOccurred(const QString& message);

private:
    void handleAddToCartResponse(const common::Message& response);
    void handleRemoveFromCartResponse(const common::Message& response);
    void handleGetCartResponse(const common::Message& response);
    void handleCheckoutResponse(const common::Message& response);
    void handleApplyDiscountResponse(const common::Message& response);

    QJsonObject m_cartData;
    QString m_discountCode;
};

} // namespace bookclub::client
