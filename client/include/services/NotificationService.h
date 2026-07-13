// =============================================================================
//  NotificationService.h
// =============================================================================
//  Mocked notification center for the Regular User role.
//
//  Real-backend mapping (see common/Network/Protocol.h):
//      notifications()         → REQ_NOTIF_LIST      → RES_NOTIF_LIST
//      markRead(id)            → REQ_NOTIF_MARK_READ → RES_OK
//      markAllRead()           → REQ_NOTIF_MARK_ALL  → RES_OK
//      (real-time push)        → EVT_NOTIFICATION     (server-pushed, async)
//
//  Real-time behavior:
//      In production the server pushes EVT_NOTIFICATION messages over the
//      socket whenever an event the user cares about occurs (new book in a
//      favorite genre, discount on a saved book, new sale of the user's
//      book, etc.). The socket layer would call
//      `emit notificationReceived(dto)` from the queued reply slot.
//      Here, the MockDataStore emits notificationsChanged whenever a new
//      notification is added (e.g. when the user submits a review or checks
//      out), and we forward it as a notificationReceived signal so the UI
//      gets the same real-time effect.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "services/MockDataStore.h"

namespace bookclub::client {

class NotificationService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY notificationsChanged)
    Q_PROPERTY(int totalCount  READ totalCount  NOTIFY notificationsChanged)

public:
    explicit NotificationService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    int unreadCount() const;
    int totalCount() const;

    Q_INVOKABLE QList<QObject*> all() const;
    Q_INVOKABLE QList<QObject*> byCategory(const QString& category) const;     // "all" | "purchase" | "review" | "discount" | "recommendation" | "publisher" | "system" | "security" | "reminder"
    Q_INVOKABLE QList<QObject*> search(const QString& query) const;

    Q_INVOKABLE void markRead(const QString& id);
    Q_INVOKABLE void markUnread(const QString& id);
    Q_INVOKABLE void markAllRead();
    Q_INVOKABLE bool deleteNotification(const QString& id);
    Q_INVOKABLE bool archiveNotification(const QString& id);
    Q_INVOKABLE bool unarchiveNotification(const QString& id);
    Q_INVOKABLE int countByCategory(const QString& category) const;

signals:
    void notificationsChanged();
    void notificationReceived(QObject* dto);  // real-time push

private:
    MockDataStore* m_store = nullptr;
};

} // namespace bookclub::client
