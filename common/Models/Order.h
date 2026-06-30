#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>

namespace bookclub::common {

class OrderItem : public QObject {
    Q_OBJECT
public:
    explicit OrderItem(QObject* parent = nullptr);
    ~OrderItem() override = default;

    const QString& bookId() const;
    const QString& title() const;
    double unitPrice() const;
    int quantity() const;

    void setBookId(const QString& bookId);
    void setTitle(const QString& title);
    void setUnitPrice(double price);
    void setQuantity(int quantity);

private:
    QString m_bookId;
    QString m_title;
    double m_unitPrice = 0.0;
    int m_quantity = 1;
};

class Order : public QObject {
    Q_OBJECT
public:
    explicit Order(QObject* parent = nullptr);
    Order(const QString& id, QObject* parent = nullptr);
    ~Order() override = default;

    const QString& id() const;
    const QString& userId() const;
    const QVector<OrderItem*>& items() const;
    const QDateTime& createdAt() const;
    double subtotal() const;
    double discountTotal() const;
    double finalTotal() const;
    bool isPaid() const;
    bool isCompleted() const;

    void setId(const QString& id);
    void setUserId(const QString& userId);
    void setCreatedAt(const QDateTime& createdAt);
    void setSubtotal(double subtotal);
    void setDiscountTotal(double discountTotal);
    void setFinalTotal(double finalTotal);
    void setPaid(bool paid);
    void setCompleted(bool completed);
    void addItem(OrderItem* item);

signals:
    void orderChanged();
    void paymentStatusChanged();

private:
    QString m_id;
    QString m_userId;
    QVector<OrderItem*> m_items;
    QDateTime m_createdAt;
    double m_subtotal = 0.0;
    double m_discountTotal = 0.0;
    double m_finalTotal = 0.0;
    bool m_paid = false;
    bool m_completed = false;
};

} // namespace bookclub::common
