// =============================================================================
//  UserService.cpp
// =============================================================================
#include "services/UserService.h"
#include "services/AuthService.h"
#include "services/LibraryDtos.h"

namespace bookclub::client {

UserService::UserService(QObject* parent)
    : QObject(parent)
{}

void UserService::setDataStore(MockDataStore* store) {
    if (m_store == store) return;
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    m_store = store;
    if (m_store) {
        connect(m_store, &MockDataStore::userChanged, this, &UserService::userChanged);
    }
    emit userChanged();
}

QString UserService::username() const {
    return m_store ? m_store->currentUsername() : QStringLiteral("guest");
}

QString UserService::displayName() const {
    return m_store ? m_store->currentDisplayName() : QStringLiteral("Guest");
}

QString UserService::initials() const {
    const QString name = displayName().trimmed();
    if (name.isEmpty()) return QStringLiteral("?");
    const auto parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.size() == 1) return parts[0].left(1).toUpper();
    return (parts.first().left(1) + parts.last().left(1)).toUpper();
}

QString UserService::favoriteGenresSummary() const {
    if (!m_store) return QStringLiteral("Not set");
    const auto g = m_store->favoriteGenres();
    if (g.isEmpty()) return QStringLiteral("Not set");
    if (g.size() <= 3) return g.join(QStringLiteral(" · "));
    return g.mid(0, 3).join(QStringLiteral(" · ")) + QStringLiteral(" +%1").arg(g.size() - 3);
}

bool UserService::updateProfile(const QString& displayName) {
    if (!m_store || displayName.trimmed().isEmpty()) return false;
    m_store->setCurrentUser(m_store->currentUsername(), displayName.trimmed());
    emit profileUpdated();
    return true;
}

bool UserService::changePassword(const QString& currentPassword,
                                  const QString& newPassword,
                                  QString& errorMessage) {
    // In production this round-trips to the server (REQ_USER_CHANGE_PASSWORD).
    // For the mock we just sanity-check the new password length.
    if (newPassword.length() < 6) {
        errorMessage = QStringLiteral("New password must be at least 6 characters.");
        emit passwordChangeFailed(errorMessage);
        return false;
    }
    if (currentPassword == newPassword) {
        errorMessage = QStringLiteral("New password must differ from the current one.");
        emit passwordChangeFailed(errorMessage);
        return false;
    }
    emit passwordChanged();
    return true;
}

QStringList UserService::favoriteGenres() const {
    return m_store ? m_store->favoriteGenres() : QStringList{};
}

bool UserService::saveFavoriteGenres(const QStringList& genres) {
    if (!m_store || genres.isEmpty()) return false;
    m_store->setFavoriteGenres(genres);
    emit userChanged();
    return true;
}

int UserService::purchaseCount() const {
    return m_store ? m_store->purchaseHistory().size() : 0;
}

QList<QObject*> UserService::purchaseHistory() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& p : m_store->purchaseHistory()) out.append(new PurchaseDto(p));
    return out;
}

} // namespace bookclub::client
