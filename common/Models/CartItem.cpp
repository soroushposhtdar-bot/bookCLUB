#include "common/Models/CartItem.h"

namespace bookclub::common {

CartItem::CartItem(QObject* parent) : QObject(parent) {}

CartItem::CartItem(const QString& bookId, QObject* parent)
    : QObject(parent), m_bookId(bookId) {}

const QString& CartItem::bookId() const { return m_bookId; }
const QString& CartItem::bookTitle() const { return m_bookTitle; }
double CartItem::unitPrice() const { return m_unitPrice; }
double CartItem::discountedUnitPrice() const { return m_discountedUnitPrice; }
int CartItem::quantity() const { return m_quantity; }

double CartItem::lineTotal() const {
    return (m_unitPrice - m_discountedUnitPrice) * m_quantity;
}

void CartItem::setBookId(const QString& bookId) {
    if (m_bookId != bookId) {
        m_bookId = bookId;
        emit itemChanged();
    }
}

void CartItem::setBookTitle(const QString& title) {
    if (m_bookTitle != title) {
        m_bookTitle = title;
        emit itemChanged();
    }
}

void CartItem::setUnitPrice(double price) {
    if (qFuzzyCompare(m_unitPrice, price)) return;
    m_unitPrice = price;
    emit itemChanged();
}

void CartItem::setDiscountedUnitPrice(double price) {
    if (qFuzzyCompare(m_discountedUnitPrice, price)) return;
    m_discountedUnitPrice = price;
    emit itemChanged();
}

void CartItem::setQuantity(int quantity) {
    if (quantity < 1) quantity = 1;
    if (m_quantity != quantity) {
        m_quantity = quantity;
        emit itemChanged();
    }
}

} // namespace bookclub::common
