#pragma once

#include <QObject>
#include <QVector>
#include <QDateTime>

#include "common/Models/CartItem.h"

namespace bookclub::common {

class Cart : public QObject {
    Q_OBJECT
public:
    explicit Cart(QObject* parent = nullptr);
    ~Cart() override = default;

    const QString& userId() const;
    const QVector<CartItem*>& items() const;
    double subtotal() const;
    double discountTotal() const;
    double total() const;
    int itemCount() const;
    bool isEmpty() const;

    void setUserId(const QString& userId);
    void addItem(CartItem* item);
    void removeItem(const QString& bookId);
    void clear();
    void setDiscountTotal(double amount);
    void recalculateTotal();

signals:
    void cartChanged();
    void totalsChanged();

private:
    QString m_userId;
    QVector<CartItem*> m_items;
    double m_discountTotal = 0.0;
    double m_subtotal = 0.0;
    double m_total = 0.0;
};

} // namespace bookclub::common
