#include "common/Models/Cart.h"
#include <algorithm>

namespace bookclub::common {

Cart::Cart(QObject* parent) : QObject(parent) {}

Cart::~Cart() {
    qDeleteAll(m_items);
}

const QString& Cart::userId() const { return m_userId; }
const QVector<CartItem*>& Cart::items() const { return m_items; }
double Cart::subtotal() const { return m_subtotal; }
double Cart::discountTotal() const { return m_discountTotal; }
double Cart::total() const { return m_total; }
int Cart::itemCount() const { return m_items.size(); }
bool Cart::isEmpty() const { return m_items.isEmpty(); }

void Cart::setUserId(const QString& userId) {
    if (m_userId != userId) {
        m_userId = userId;
        emit cartChanged();
    }
}

void Cart::addItem(CartItem* item) {
    if (!item) return;

    // Check if book already exists
    for (CartItem* existing : m_items) {
        if (existing->bookId() == item->bookId()) {
            existing->setQuantity(existing->quantity() + item->quantity());
            delete item;
            recalculateTotal();
            emit cartChanged();
            return;
        }
    }

    m_items.append(item);
    connect(item, &CartItem::itemChanged, this, [this]() {
        recalculateTotal();
        emit cartChanged();
    });
    recalculateTotal();
    emit cartChanged();
}

void Cart::removeItem(const QString& bookId) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->bookId() == bookId) {
            delete m_items[i];
            m_items.removeAt(i);
            recalculateTotal();
            emit cartChanged();
            return;
        }
    }
}

void Cart::clear() {
    qDeleteAll(m_items);
    m_items.clear();
    m_subtotal = 0.0;
    m_discountTotal = 0.0;
    m_total = 0.0;
    emit cartChanged();
    emit totalsChanged();
}

void Cart::setDiscountTotal(double amount) {
    if (qFuzzyCompare(m_discountTotal, amount)) return;
    m_discountTotal = amount;
    recalculateTotal();
}

void Cart::recalculateTotal() {
    double subtotal = 0.0;
    for (const CartItem* item : m_items) {
        subtotal += item->unitPrice() * item->quantity();
    }
    m_subtotal = subtotal;
    m_total = m_subtotal - m_discountTotal;
    if (m_total < 0.0) m_total = 0.0;
    emit totalsChanged();
}

} // namespace bookclub::common
