#include "common/Models/Order.h"

namespace bookclub::common {

// ---- OrderItem ----
OrderItem::OrderItem(QObject* parent) : QObject(parent) {}

const QString& OrderItem::bookId() const { return m_bookId; }
const QString& OrderItem::title() const { return m_title; }
double OrderItem::unitPrice() const { return m_unitPrice; }
int OrderItem::quantity() const { return m_quantity; }

void OrderItem::setBookId(const QString& bookId) { m_bookId = bookId; }
void OrderItem::setTitle(const QString& title) { m_title = title; }
void OrderItem::setUnitPrice(double price) { m_unitPrice = price; }
void OrderItem::setQuantity(int quantity) {
    if (quantity < 1) quantity = 1;
    m_quantity = quantity;
}

// ---- Order ----
Order::Order(QObject* parent) : QObject(parent) {}

Order::Order(const QString& id, QObject* parent) : QObject(parent), m_id(id) {}

Order::~Order() {
    qDeleteAll(m_items);
}

const QString& Order::id() const { return m_id; }
const QString& Order::userId() const { return m_userId; }
const QVector<OrderItem*>& Order::items() const { return m_items; }
const QDateTime& Order::createdAt() const { return m_createdAt; }
double Order::subtotal() const { return m_subtotal; }
double Order::discountTotal() const { return m_discountTotal; }
double Order::finalTotal() const { return m_finalTotal; }
bool Order::isPaid() const { return m_paid; }
bool Order::isCompleted() const { return m_completed; }

void Order::setId(const QString& id) { m_id = id; }
void Order::setUserId(const QString& userId) { m_userId = userId; }
void Order::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
void Order::setSubtotal(double subtotal) {
    m_subtotal = subtotal;
    emit orderChanged();
}
void Order::setDiscountTotal(double discountTotal) {
    m_discountTotal = discountTotal;
    emit orderChanged();
}
void Order::setFinalTotal(double finalTotal) {
    m_finalTotal = finalTotal;
    emit orderChanged();
}

void Order::setPaid(bool paid) {
    if (m_paid != paid) {
        m_paid = paid;
        emit paymentStatusChanged();
        emit orderChanged();
    }
}

void Order::setCompleted(bool completed) {
    if (m_completed != completed) {
        m_completed = completed;
        emit orderChanged();
    }
}

void Order::addItem(OrderItem* item) {
    if (item) {
        m_items.append(item);
        emit orderChanged();
    }
}

} // namespace bookclub::common
