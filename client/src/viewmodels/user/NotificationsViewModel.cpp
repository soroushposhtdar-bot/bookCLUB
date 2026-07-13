// =============================================================================
//  NotificationsViewModel.cpp
// =============================================================================
#include "viewmodels/user/NotificationsViewModel.h"
#include "services/NotificationService.h"
#include "services/LibraryDtos.h"

namespace bookclub::client {

NotificationsViewModel::NotificationsViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void NotificationsViewModel::setService(NotificationService* s) {
    if (m_service == s) return;
    if (m_service) disconnect(m_service, nullptr, this, nullptr);
    m_service = s;
    if (m_service) {
        connect(m_service, &NotificationService::notificationsChanged,
                this, &NotificationsViewModel::notificationsChanged);
        connect(m_service, &NotificationService::notificationReceived,
                this, &NotificationsViewModel::realtimeNotificationReceived);
    }
    emit serviceChanged();
    emit notificationsChanged();
}

QList<QObject*> NotificationsViewModel::notifications() const {
    if (!m_service) return {};
    QList<QObject*> list;
    if (m_activeCategory == "all") {
        list = m_service->all();
    } else {
        list = m_service->byCategory(m_activeCategory);
    }
    // Apply search
    if (!m_searchQuery.trimmed().isEmpty()) {
        const QString q = m_searchQuery.trimmed().toLower();
        QList<QObject*> filtered;
        for (auto* o : list) {
            auto n = qobject_cast<NotificationDto*>(o);
            if (!n) { delete o; continue; }
            if (n->title().toLower().contains(q) || n->body().toLower().contains(q)) {
                filtered.append(n);
            } else {
                delete n;
            }
        }
        list = filtered;
    }
    return list;
}

int NotificationsViewModel::unreadCount() const {
    return m_service ? m_service->unreadCount() : 0;
}

int NotificationsViewModel::totalCount() const {
    return m_service ? m_service->countByCategory(m_activeCategory) : 0;
}

void NotificationsViewModel::markRead(const QString& id)   { if (m_service) m_service->markRead(id); }
void NotificationsViewModel::markUnread(const QString& id) { if (m_service) m_service->markUnread(id); }
void NotificationsViewModel::markAllRead()                 { if (m_service) m_service->markAllRead(); }
void NotificationsViewModel::deleteNotification(const QString& id) { if (m_service) m_service->deleteNotification(id); }
void NotificationsViewModel::archiveNotification(const QString& id) { if (m_service) m_service->archiveNotification(id); }

} // namespace bookclub::client
