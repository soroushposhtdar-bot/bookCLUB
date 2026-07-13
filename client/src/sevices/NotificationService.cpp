// =============================================================================
//  NotificationService.cpp
// =============================================================================
#include "services/NotificationService.h"
#include "services/LibraryDtos.h"

namespace bookclub::client {

NotificationService::NotificationService(QObject* parent)
    : QObject(parent)
{}

void NotificationService::setDataStore(MockDataStore* store) {
    if (m_store == store) return;
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    m_store = store;
    if (m_store) {
        connect(m_store, &MockDataStore::notificationsChanged, this, [this]() {
            emit notificationsChanged();
            // Emit a real-time "received" signal for the newest one (if unread).
            if (!m_store->notifications().isEmpty()) {
                const auto& newest = m_store->notifications().first();
                if (!newest.read) {
                    auto dto = new NotificationDto(newest);
                    dto->setParent(nullptr);
                    emit notificationReceived(dto);
                }
            }
        });
    }
    emit notificationsChanged();
}

int NotificationService::unreadCount() const {
    if (!m_store) return 0;
    int n = 0;
    for (const auto& item : m_store->notifications()) if (!item.read) ++n;
    return n;
}

int NotificationService::totalCount() const {
    return m_store ? m_store->notifications().size() : 0;
}

QList<QObject*> NotificationService::all() const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& n : m_store->notifications()) {
        if (!n.archived) out.append(new NotificationDto(n));
    }
    return out;
}

QList<QObject*> NotificationService::byCategory(const QString& category) const {
    QList<QObject*> out;
    if (!m_store) return out;
    for (const auto& n : m_store->notifications()) {
        if (n.archived) continue;
        if (category == QStringLiteral("all") || n.category == category) {
            out.append(new NotificationDto(n));
        }
    }
    return out;
}

QList<QObject*> NotificationService::search(const QString& query) const {
    QList<QObject*> out;
    if (!m_store) return out;
    const QString q = query.trimmed().toLower();
    for (const auto& n : m_store->notifications()) {
        if (n.archived) continue;
        if (q.isEmpty() || n.title.toLower().contains(q) || n.body.toLower().contains(q)) {
            out.append(new NotificationDto(n));
        }
    }
    return out;
}

void NotificationService::markRead(const QString& id) {
    if (m_store) m_store->markNotificationRead(id, true);
}

void NotificationService::markUnread(const QString& id) {
    if (m_store) m_store->markNotificationRead(id, false);
}

void NotificationService::markAllRead() {
    if (m_store) m_store->markAllNotificationsRead();
}

bool NotificationService::deleteNotification(const QString& id) {
    if (!m_store) return false;
    return m_store->deleteNotification(id);
}

bool NotificationService::archiveNotification(const QString& id) {
    if (!m_store) return false;
    return m_store->archiveNotification(id, true);
}

bool NotificationService::unarchiveNotification(const QString& id) {
    if (!m_store) return false;
    return m_store->archiveNotification(id, false);
}

int NotificationService::countByCategory(const QString& category) const {
    if (!m_store) return 0;
    int n = 0;
    for (const auto& item : m_store->notifications()) {
        if (item.archived) continue;
        if (category == QStringLiteral("all") || item.category == category) ++n;
    }
    return n;
}

} // namespace bookclub::client
