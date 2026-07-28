#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QJsonObject>

namespace bookclub::client {

class NotificationService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY notificationsChanged)
    Q_PROPERTY(int totalCount  READ totalCount  NOTIFY notificationsChanged)

public:
    explicit NotificationService(QObject* parent = nullptr);
    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op

    // BUG FIX (MSVC const-correctness): all accessor methods are now
    // NON-const. Previously they were const and used const_cast to call
    // refresh() (which emits notificationsChanged — a non-const signal).
    // MSVC on Windows rejects this pattern with error C2662. Making the
    // methods non-const eliminates the need for const_cast entirely.
    // QML doesn't care about const-ness, so this is safe.
    int unreadCount();
    int totalCount();

    Q_INVOKABLE QList<QObject*> all();
    Q_INVOKABLE QList<QObject*> byCategory(const QString& category);
    Q_INVOKABLE QList<QObject*> search(const QString& query);

    Q_INVOKABLE void markRead(const QString& id);
    Q_INVOKABLE void markUnread(const QString& id);
    Q_INVOKABLE void markAllRead();
    Q_INVOKABLE void clearReadNotifications();  // v12: remove all read notifications
    Q_INVOKABLE bool deleteNotification(const QString& id);
    Q_INVOKABLE bool archiveNotification(const QString& id);
    Q_INVOKABLE bool unarchiveNotification(const QString& id);
    Q_INVOKABLE int countByCategory(const QString& category);

    Q_INVOKABLE void refresh();

signals:
    void notificationsChanged();
    void notificationReceived(QObject* dto);

private:
    QList<QObject*> m_cache;
};

} // namespace bookclub::client
