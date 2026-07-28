#pragma once

#include <QObject>
#include <QString>

namespace bookclub::common {

class CartItem : public QObject {
    Q_OBJECT
public:
    explicit CartItem(QObject* parent = nullptr);
    CartItem(const QString& bookId, QObject* parent = nullptr);
    ~CartItem() override = default;

    const QString& bookId() const;
    const QString& bookTitle() const;
    double unitPrice() const;
    double discountedUnitPrice() const;
    int quantity() const;
    double lineTotal() const;

    void setBookId(const QString& bookId);
    void setBookTitle(const QString& title);
    void setUnitPrice(double price);
    void setDiscountedUnitPrice(double price);
    void setQuantity(int quantity);

signals:
    void itemChanged();

private:
    QString m_bookId;
    QString m_bookTitle;
    double m_unitPrice = 0.0;
    double m_discountedUnitPrice = 0.0;
    int m_quantity = 1;
};

} // namespace bookclub::common
