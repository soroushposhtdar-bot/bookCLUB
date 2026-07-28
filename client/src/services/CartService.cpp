#include "services/CartService.h"
#include "services/NetworkService.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QLocale>
#include "services/CartItemDto.h"

namespace bookclub::client {

CartService::CartService(QObject* parent) : QObject(parent) {}

void CartService::refreshFromServer() const {
    auto resp = NetworkService::instance().sendRequest(common::Command::GetCart);
    if (resp.isSuccess()) {
        m_cartData = resp.payload;
    }
}

QList<QObject*> CartService::parseItems(const QJsonArray& arr) const {
    QList<QObject*> result;
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        auto* item = new CartItemDto();
        item->fromJson(v.toObject());
        result.append(item);
    }
    return result;
}

int CartService::itemCount() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    // If itemCount is 0 but items array is non-empty, use array length
    int count = m_cartData.value("itemCount").toInt();
    if (count == 0) {
        count = m_cartData.value("items").toArray().size();
    }
    return count;
}

double CartService::subtotal() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    return m_cartData.value("subtotal").toDouble();
}

double CartService::discountTotal() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    return m_cartData.value("discountTotal").toDouble();
}

double CartService::total() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    return m_cartData.value("total").toDouble();
}

QString CartService::subtotalText() const {
    return QLocale().toString(subtotal(), 'f', 2);
}

QString CartService::discountText() const {
    return QLocale().toString(discountTotal(), 'f', 2);
}

QString CartService::totalText() const {
    return QLocale().toString(total(), 'f', 2);
}

QString CartService::savingsText() const {
    return QLocale().toString(discountTotal(), 'f', 2);
}

// v15c: purchaseProfit — sum of (basePrice - discountedUnitPrice) * quantity
// across all cart items. This is the "purchase profit" from per-book
// discounts. Example: two $60 books, one discounted 10% ($6 off) and the
// other 20% ($12 off) → purchaseProfit = $18.
//
// The server's cartToJson enriches each item with `basePrice` and
// `discountValue` (absolute amount), so we compute:
//   perItemProfit = discountValue * quantity
// (discountValue = basePrice - discountedUnitPrice, already absolute).
double CartService::purchaseProfit() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    double profit = 0.0;
    const QJsonArray items = m_cartData.value("items").toArray();
    for (const auto& v : items) {
        const QJsonObject item = v.toObject();
        const double basePrice = item.value("basePrice").toDouble();
        const double discountValue = item.value("discountValue").toDouble();
        const int quantity = item.value("quantity").toInt(1);
        // Per-item profit = discountValue * quantity.
        // We use discountValue (absolute) when available; otherwise fall
        // back to basePrice - discountedUnitPrice.
        if (discountValue > 0.0) {
            profit += discountValue * quantity;
        } else if (basePrice > 0.0) {
            const double discounted = item.value("discountedUnitPrice").toDouble(basePrice);
            profit += (basePrice - discounted) * quantity;
        }
    }
    return profit;
}

QString CartService::purchaseProfitText() const {
    return QLocale().toString(purchaseProfit(), 'f', 2);
}

QList<QObject*> CartService::items() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    // Parse fresh DTOs from cached server data. The old DTOs (from the
    // previous call) are QML-owned and will be GC'd when QML releases them.
    return parseItems(m_cartData.value("items").toArray());
}

bool CartService::isEmpty() const {
    if (m_cartData.isEmpty()) refreshFromServer();
    // Use the actual items array length, not the itemCount field
    // (which may be stale or not set by the server).
    return m_cartData.value("items").toArray().size() == 0;
}

bool CartService::isInCart(const QString& bookId) const {
    if (m_cartData.isEmpty()) refreshFromServer();
    const QJsonArray items = m_cartData.value("items").toArray();
    for (const auto& v : items) {
        if (v.toObject().value("bookId").toString() == bookId) return true;
    }
    return false;
}

void CartService::add(const QString& bookId) {
    QJsonObject p;
    p["bookId"] = bookId;
    p["quantity"] = 1;
    auto resp = NetworkService::instance().sendRequest(common::Command::AddToCart, p);
    if (resp.isSuccess()) {
        m_cartData = resp.payload;
        emit cartChanged();
    } else if (resp.status == common::Status::Conflict) {
        // v15: Server rejected because the user already owns this book.
        // Emit checkoutFailed with a user-friendly message so the UI can
        // show a toast instead of silently failing.
        emit checkoutFailed(QStringLiteral("You already own this book."));
        // Force-refresh the cart cache so any stale "in cart" state is
        // cleared — the book can't be in the cart if the user owns it.
        refresh();
    }
}

void CartService::remove(const QString& bookId) {
    QJsonObject p;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::RemoveFromCart, p);
    if (resp.isSuccess()) {
        m_cartData = resp.payload;
        emit cartChanged();
    }
}

void CartService::clear() {
    // Issue: use the server's ClearCart command instead of N RemoveFromCart
    // round-trips. Falls back to the old loop only if ClearCart fails.
    auto resp = NetworkService::instance().sendRequest(common::Command::ClearCart);
    if (resp.isSuccess()) {
        m_cartData = QJsonObject();
        emit cartChanged();
        return;
    }
    // Fallback: remove items one by one (for older servers without ClearCart).
    if (m_cartData.isEmpty()) refreshFromServer();
    const QJsonArray items = m_cartData.value("items").toArray();
    for (const auto& v : items) {
        QJsonObject p;
        p["bookId"] = v.toObject().value("bookId").toString();
        NetworkService::instance().sendRequest(common::Command::RemoveFromCart, p);
    }
    m_cartData = QJsonObject();
    emit cartChanged();
}

bool CartService::checkout() {
    auto resp = NetworkService::instance().sendRequest(common::Command::Checkout);
    if (resp.isSuccess()) {
        // v15: Force a server-side refresh instead of just clearing the
        // local cache. The server already cleared the cart in the DB, but
        // the local m_cartData might have been re-populated by a concurrent
        // read (e.g. the dashboard's cart badge timer). Calling refresh()
        // ensures every binding — cart badge, cart page, book detail's
        // "in cart" state — sees an empty cart immediately.
        m_cartData = QJsonObject();
        refreshFromServer();
        // Server now returns orderId + finalTotal + purchasedBookIds (array).
        QStringList purchasedIds;
        const QJsonArray ids = resp.payload.value("purchasedBookIds").toArray();
        for (const QJsonValue& v : ids) {
            purchasedIds.append(v.toString());
        }
        emit cartChanged();
        emit checkoutSucceeded(purchasedIds);
        return true;
    } else {
        emit checkoutFailed(resp.errorMessage);
        return false;
    }
}

// v11: Apply a promo/discount code to the cart.
void CartService::applyDiscount(const QString& code) {
    QJsonObject payload;
    payload["discountCode"] = code;
    auto resp = NetworkService::instance().sendRequest(common::Command::ApplyDiscount, payload);
    // Update local cache from response (server returns updated cart)
    if (resp.isSuccess()) {
        m_cartData = resp.payload;
        emit cartChanged();
    }
}

// Issue 12 — public refresh so the CartPage can force-reload from the
// server after navigation (the cache is otherwise only refreshed on
// mutation or first access).
void CartService::refresh() {
    m_cartData = QJsonObject();
    refreshFromServer();
    emit cartChanged();
}

// Issue 12 — set quantity for an item. The server's AddToCart handler
// UPSERTs (bumps quantity on conflict), so we remove + re-add with the
// desired count to set an absolute quantity.
//
// BUG FIX (Issue 14): previously, if the RemoveFromCart succeeded but
// the subsequent AddToCart failed (network blip, server 500), the
// function returned without restoring the item — the cart was missing
// the book, but no `cartChanged` was emitted, so the UI still showed
// the item until the next refresh, at which point it vanished. We now
// re-send AddToCart with the original quantity (captured before the
// remove) to restore state, then emit `cartChanged()` so the UI
// re-syncs immediately.
void CartService::setQuantity(const QString& bookId, int quantity) {
    // v15: Each book can only be purchased once, so cap the quantity at 1.
    // If the user tries to increase past 1, ignore it. If they set to 0
    // or below, remove the item.
    if (quantity <= 0) {
        remove(bookId);
        return;
    }
    if (quantity > 1) {
        quantity = 1;  // enforce single-copy-per-book
    }

    // Capture the original quantity so we can roll back if the re-add fails.
    int originalQuantity = 0;
    const QJsonArray origItems = m_cartData.value("items").toArray();
    for (const auto& v : origItems) {
        const QJsonObject o = v.toObject();
        if (o.value("bookId").toString() == bookId) {
            originalQuantity = o.value("quantity").toInt(1);
            break;
        }
    }

    QJsonObject rmP; rmP["bookId"] = bookId;
    auto rmResp = NetworkService::instance().sendRequest(common::Command::RemoveFromCart, rmP);
    if (!rmResp.isSuccess()) return;

    QJsonObject addP; addP["bookId"] = bookId; addP["quantity"] = quantity;
    auto addResp = NetworkService::instance().sendRequest(common::Command::AddToCart, addP);
    if (addResp.isSuccess()) {
        m_cartData = addResp.payload;
        emit cartChanged();
    } else if (originalQuantity > 0) {
        // Rollback: re-add the item with its original quantity so the
        // cart stays consistent with what the user saw before. The UI
        // will re-sync via cartChanged(); the user can retry.
        QJsonObject restoreP; restoreP["bookId"] = bookId; restoreP["quantity"] = originalQuantity;
        auto restoreResp = NetworkService::instance().sendRequest(common::Command::AddToCart, restoreP);
        if (restoreResp.isSuccess()) {
            m_cartData = restoreResp.payload;
        }
        emit cartChanged();
    }
}

} // namespace bookclub::client

