#include "services/NotificationService.h"
#include "services/NetworkService.h"
#include "services/LibraryDtos.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QQmlEngine>

namespace bookclub::client {

NotificationService::NotificationService(QObject* parent) : QObject(parent)
{
    // Listen for server-pushed real-time notifications.
    NetworkService::instance().subscribeEvent(common::Command::EvtNotification,
        this,
        [this](const common::Message& msg) {
            auto* dto = new NotificationDto();
            dto->fromJson(msg.payload());
            m_cache.prepend(dto);
            emit notificationsChanged();
            emit notificationReceived(dto);
        });
}

void NotificationService::refresh() {
    // Transfer old cache to QML GC instead of qDeleteAll — QML may still
    // hold references via bindings that haven't been re-evaluated yet.
    for (auto* o : m_cache) QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
    m_cache.clear();
    auto resp = NetworkService::instance().sendRequest(common::Command::GetNotifications);
    if (!resp.isSuccess()) return;
    const QJsonArray arr = resp.payload.value("notifications").toArray();
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        auto* dto = new NotificationDto();
        dto->fromJson(v.toObject());
        m_cache.append(dto);
    }
    emit notificationsChanged();
}

int NotificationService::unreadCount() {
    if (m_cache.isEmpty()) refresh();
    int count = 0;
    for (QObject* n : m_cache) {
        if (!n->property("isRead").toBool()) count++;
    }
    return count;
}

int NotificationService::totalCount() {
    if (m_cache.isEmpty()) refresh();
    return m_cache.size();
}

QList<QObject*> NotificationService::all() {
    if (m_cache.isEmpty()) refresh();
    return m_cache;
}

QList<QObject*> NotificationService::byCategory(const QString& category) {
    if (m_cache.isEmpty()) refresh();
    if (category == "all" || category.isEmpty()) return m_cache;
    QList<QObject*> result;
    for (QObject* n : m_cache) {
        if (n->property("category").toString() == category) result.append(n);
    }
    return result;
}

QList<QObject*> NotificationService::search(const QString& query) {
    if (m_cache.isEmpty()) refresh();
    QList<QObject*> result;
    for (QObject* n : m_cache) {
        if (n->property("title").toString().contains(query, Qt::CaseInsensitive) ||
            n->property("message").toString().contains(query, Qt::CaseInsensitive)) {
            result.append(n);
        }
    }
    return result;
}

void NotificationService::markRead(const QString& id) {
    QJsonObject p;
    p["notificationId"] = id;
    auto resp = NetworkService::instance().sendRequest(common::Command::MarkNotificationRead, p);
    if (resp.isSuccess()) {
        for (QObject* n : m_cache) {
            if (n->property("id").toString() == id) {
                n->setProperty("isRead", true);
                break;
            }
        }
        emit notificationsChanged();
    }
}

void NotificationService::markUnread(const QString& id) {
    for (QObject* n : m_cache) {
        if (n->property("id").toString() == id) {
            n->setProperty("isRead", false);
            break;
        }
    }
    emit notificationsChanged();
}

void NotificationService::markAllRead() {
    auto resp = NetworkService::instance().sendRequest(common::Command::MarkAllNotificationsRead);
    if (resp.isSuccess()) {
        for (QObject* n : m_cache) {
            n->setProperty("isRead", true);
        }
        emit notificationsChanged();
    }
}

// v12: clear all read notifications from the local cache.
void NotificationService::clearReadNotifications() {
    QList<QObject*> kept;
    for (QObject* n : m_cache) {
        if (n->property("isRead").toBool()) {
            delete n;  // remove read notifications
        } else {
            kept.append(n);
        }
    }
    m_cache = kept;
    emit notificationsChanged();
}

bool NotificationService::deleteNotification(const QString& id) {
    for (int i = 0; i < m_cache.size(); ++i) {
        if (m_cache[i]->property("id").toString() == id) {
            delete m_cache.takeAt(i);
            emit notificationsChanged();
            return true;
        }
    }
    return false;
}

bool NotificationService::archiveNotification(const QString& id) {
    return deleteNotification(id);
}

bool NotificationService::unarchiveNotification(const QString&) {
    refresh();
    emit notificationsChanged();
    return true;
}

int NotificationService::countByCategory(const QString& category) {
    // v15e: return the UNREAD count per category (not total). The
    // notification page's category badges should decrease when a
    // notification is marked as read. Previously this returned the total
    // count (read + unread), so the badges never decreased.
    if (m_cache.isEmpty()) refresh();
    int count = 0;
    if (category == "all" || category.isEmpty()) {
        for (QObject* n : m_cache) {
            if (!n->property("isRead").toBool()) count++;
        }
    } else {
        for (QObject* n : m_cache) {
            if (n->property("category").toString() == category
                && !n->property("isRead").toBool()) {
                count++;
            }
        }
    }
    return count;
}

} // namespace bookclub::client
