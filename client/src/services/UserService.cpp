#include "services/UserService.h"
#include "services/NetworkService.h"
#include "services/AuthService.h"
#include "services/LibraryDtos.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

UserService::UserService(QObject* parent) : QObject(parent) {}

QString UserService::username() const { return AuthService::instance().currentUsername(); }
// BUG FIX (Issue 3): previously this returned AuthService's stale
// `currentDisplayName()` even after `updateProfile()` had cached the
// new name in `m_displayName`. The Profile page's InputField therefore
// reverted the user's keystrokes to the old name on every edit. We now
// return the locally-cached value if it's been set, falling back to
// AuthService for the initial load.
QString UserService::displayName() const {
    if (!m_displayName.isEmpty()) return m_displayName;
    return AuthService::instance().currentDisplayName();
}

QString UserService::initials() const {
    const QString name = displayName();
    if (name.isEmpty()) return QStringLiteral("?");
    QStringList parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return QStringLiteral("?");
    QString initials;
    for (const QString& p : parts) {
        if (!p.isEmpty()) initials.append(p.left(1).toUpper());
        if (initials.length() >= 2) break;
    }
    return initials;
}

QString UserService::favoriteGenresSummary() const {
    return m_favoriteGenres.join(QStringLiteral(", "));
}

bool UserService::updateProfile(const QString& displayName) {
    QJsonObject p;
    p["displayName"] = displayName;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateProfile, p);
    if (resp.isSuccess()) {
        // AuthService's currentDisplayName is read-only on the client; the
        // next login will refresh it. For now, cache locally so the UI
        // reflects the change immediately.
        m_displayName = displayName;
        emit userChanged();
        emit profileUpdated();
        return true;
    }
    return false;
}

bool UserService::changePassword(const QString& currentPassword,
                                  const QString& newPassword,
                                  QString& errorMessage) {
    return AuthService::instance().changePassword(currentPassword, newPassword, errorMessage);
}

QStringList UserService::favoriteGenres() const {
    // Lazy-load from the server on first access.
    if (m_favoriteGenres.isEmpty()) {
        auto resp = NetworkService::instance().sendRequest(common::Command::GetCurrentUser);
        if (resp.isSuccess()) {
            const QJsonArray arr = resp.payload.value("favoriteGenres").toArray();
            for (const auto& v : arr) m_favoriteGenres.append(v.toString());
        }
    }
    return m_favoriteGenres;
}

bool UserService::saveFavoriteGenres(const QStringList& genres) {
    QJsonObject p;
    p["genres"] = QJsonArray::fromStringList(genres);
    auto resp = NetworkService::instance().sendRequest(common::Command::SaveFavoriteGenres, p);
    if (resp.isSuccess()) {
        m_favoriteGenres = genres;
        emit userChanged();
        return true;
    }
    return false;
}

int UserService::purchaseCount() const {
    auto resp = NetworkService::instance().sendRequest(common::Command::GetPurchasedBooks);
    if (!resp.isSuccess()) return 0;
    return resp.payload.value("purchasedBooks").toArray().size();
}

QList<QObject*> UserService::purchaseHistory() const {
    // v15j: fetch the user's purchase history from the server's
    // GetPurchasedBooks endpoint, then batch-fetch book details to
    // populate titles. The server now returns order details (orderId,
    // date, total, bookIds, titlesSummary) directly.
    auto resp = NetworkService::instance().sendRequest(common::Command::GetPurchasedBooks);
    if (!resp.isSuccess()) return {};
    QList<QObject*> result;
    const QJsonArray arr = resp.payload.value("orders").toArray();
    if (arr.isEmpty()) {
        // v15j: fallback — if the server doesn't return "orders", try
        // the old "purchasedBooks" format (just book IDs).
        const QJsonArray bookIds = resp.payload.value("purchasedBooks").toArray();
        if (bookIds.isEmpty()) return result;
        // Batch-fetch book details.
        QJsonObject p;
        p["bookIds"] = bookIds;
        auto booksResp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
        if (booksResp.isSuccess()) {
            for (const auto& v : booksResp.payload.value("books").toArray()) {
                if (!v.isObject()) continue;
                QJsonObject dto;
                dto["orderId"] = QStringLiteral("—");
                dto["date"] = v.toObject().value("createdAt").toString();
                dto["relativeDate"] = v.toObject().value("createdAt").toString();
                dto["total"] = v.toObject().value("price").toDouble();
                dto["titlesSummary"] = v.toObject().value("title").toString();
                dto["discountText"] = QStringLiteral("No discount");
                QJsonArray ids;
                ids.append(v.toObject().value("id").toString());
                dto["bookIds"] = ids;
                auto* purchase = new PurchaseDto();
                purchase->fromJson(dto);
                result.append(purchase);
            }
        }
        return result;
    }
    // New format — server returns full order details.
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        auto* purchase = new PurchaseDto();
        purchase->fromJson(v.toObject());
        result.append(purchase);
    }
    return result;
}

} // namespace bookclub::client
