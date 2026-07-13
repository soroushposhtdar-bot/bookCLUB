// =============================================================================
//  CartViewModel.cpp
// =============================================================================
#include "viewmodels/user/CartViewModel.h"
#include "services/CartService.h"

namespace bookclub::client {

CartViewModel::CartViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void CartViewModel::setCartService(CartService* s) {
    if (m_cartService == s) return;
    if (m_cartService) disconnect(m_cartService, nullptr, this, nullptr);
    m_cartService = s;
    if (m_cartService) {
        connect(m_cartService, &CartService::cartChanged, this, &CartViewModel::itemsChanged);
        connect(m_cartService, &CartService::checkoutSucceeded, this, [this](const QStringList& ids){
            m_lastPurchased = ids;
            emit itemsChanged();
            emit checkoutSucceeded(ids);
        });
        connect(m_cartService, &CartService::checkoutFailed, this, &CartViewModel::checkoutFailed);
    }
    emit cartServiceChanged();
    emit itemsChanged();
}

QList<QObject*> CartViewModel::items() const {
    return m_cartService ? m_cartService->items() : QList<QObject*>{};
}
int CartViewModel::itemCount() const {
    return m_cartService ? m_cartService->itemCount() : 0;
}
double CartViewModel::subtotal() const {
    return m_cartService ? m_cartService->subtotal() : 0.0;
}
double CartViewModel::discountTotal() const {
    return m_cartService ? m_cartService->discountTotal() : 0.0;
}
double CartViewModel::total() const {
    return m_cartService ? m_cartService->total() : 0.0;
}
QString CartViewModel::subtotalText() const {
    return m_cartService ? m_cartService->subtotalText() : QStringLiteral("$0.00");
}
QString CartViewModel::discountText() const {
    return m_cartService ? m_cartService->discountText() : QStringLiteral("-$0.00");
}
QString CartViewModel::totalText() const {
    return m_cartService ? m_cartService->totalText() : QStringLiteral("$0.00");
}
QString CartViewModel::savingsText() const {
    return m_cartService ? m_cartService->savingsText() : QString{};
}

void CartViewModel::removeItem(const QString& bookId) {
    if (m_cartService) m_cartService->remove(bookId);
}

void CartViewModel::clear() {
    if (m_cartService) m_cartService->clear();
}

void CartViewModel::checkout() {
    if (!m_cartService) return;
    m_pending = PendingOp::Checkout;
    beginAsync(700);   // simulate order-creation round-trip
}

void CartViewModel::onAsyncReady() {
    if (m_pending == PendingOp::Checkout) {
        if (m_cartService) m_cartService->checkout();
        m_pending = PendingOp::None;
    }
    finishAsync();
}

} // namespace bookclub::client
