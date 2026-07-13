// =============================================================================
//  CartService.cpp
// =============================================================================
#include "services/CartService.h"
#include "services/CartItemDto.h"
#include <QDateTime>

namespace bookclub::client {

CartService::CartService(QObject* parent)
    : QObject(parent)
{}

void CartService::setDataStore(MockDataStore* store) {
    if (m_store == store) return;
    m_store = store;
    emit cartChanged();
}

double CartService::subtotal() const {
    double s = 0;
    for (const auto& it : m_items) s += it.basePrice;
    return s;
}

double CartService::discountTotal() const {
    double s = 0;
    for (const auto& it : m_items) s += it.discountAmount;
    return s;
}

double CartService::total() const {
    double s = 0;
    for (const auto& it : m_items) s += it.unitPrice;
    return s;
}

QString CartService::subtotalText() const {
    return QStringLiteral("$%1").arg(subtotal(), 0, 'f', 2);
}
QString CartService::discountText() const {
    return QStringLiteral("-$%1").arg(discountTotal(), 0, 'f', 2);
}
QString CartService::totalText() const {
    return QStringLiteral("$%1").arg(total(), 0, 'f', 2);
}
QString CartService::savingsText() const {
    if (discountTotal() <= 0.0) return QStringLiteral("You're paying full price.");
    return QStringLiteral("You're saving $%1 on this order.").arg(discountTotal(), 0, 'f', 2);
}

QList<QObject*> CartService::items() const {
    QList<QObject*> out;
    for (const auto& it : m_items) {
        auto dto = new CartItemDto;
        dto->m_bookId = it.bookId;
        dto->m_title = it.title;
        dto->m_authorName = it.authorName;
        dto->m_coverColor = it.coverColor;
        dto->m_coverAccent = it.coverAccent;
        dto->m_unitPrice = it.unitPrice;
        dto->m_basePrice = it.basePrice;
        dto->m_discountAmount = it.discountAmount;
        dto->m_quantity = it.quantity;
        out.append(dto);
    }
    return out;
}

bool CartService::isInCart(const QString& bookId) const {
    for (const auto& it : m_items) if (it.bookId == bookId) return true;
    return false;
}

void CartService::add(const QString& bookId) {
    if (!m_store) return;
    if (isInCart(bookId)) return;
    MockBook b = m_store->bookById(bookId);
    if (b.id.isEmpty()) return;

    MockCartItem it;
    it.bookId = b.id;
    it.title = b.title;
    it.authorName = b.authorName;
    it.coverColor = b.coverColor;
    it.coverAccent = b.coverAccent;
    it.basePrice = b.basePrice;
    it.unitPrice = b.price;
    it.discountAmount = b.basePrice - b.price;
    it.quantity = 1;
    m_items.append(it);
    emit cartChanged();
}

void CartService::remove(const QString& bookId) {
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].bookId == bookId) {
            m_items.removeAt(i);
            emit cartChanged();
            return;
        }
    }
}

void CartService::clear() {
    if (m_items.isEmpty()) return;
    m_items.clear();
    emit cartChanged();
}

bool CartService::checkout() {
    if (m_items.isEmpty()) {
        emit checkoutFailed(QStringLiteral("Your cart is empty."));
        return false;
    }

    QStringList bookIds;
    for (const auto& it : m_items) bookIds.append(it.bookId);

    // Real backend: REQ_CART_CHECKOUT → server creates an order, marks books
    // as purchased, broadcasts a SaleRegistered notification to the publisher,
    // and replies with the order id. Here we just record the purchase.
    m_store->addPurchase(bookIds, total(), discountTotal());

    // The user gets a "thanks for your purchase" notification.
    MockNotification n;
    n.id = QStringLiteral("n_%1").arg(QDateTime::currentMSecsSinceEpoch());
    n.type = QStringLiteral("SaleRegistered");
    n.title = QStringLiteral("Purchase complete");
    n.body = QStringLiteral("%1 book(s) added to your library.").arg(bookIds.size());
    n.bookId = bookIds.first();
    n.createdAt = QDateTime::currentDateTime();
    m_store->addNotification(n);

    QStringList purchased = bookIds;
    m_items.clear();
    emit cartChanged();
    emit checkoutSucceeded(purchased);
    return true;
}

void CartService::_rebuildFromBooks() {
    // Reserved for future use — re-pulls cover/price from the store after
    // a catalog refresh. Currently checkout/add already read fresh data.
}

} // namespace bookclub::client
