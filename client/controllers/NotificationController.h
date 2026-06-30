#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class NotificationController : public QObject {
    Q_OBJECT
public:
    explicit NotificationController(QObject* parent = nullptr);
    ~NotificationController() override = default;

    void loadNotifications();
    void markAsRead(const QString& notificationId);
    void markAllAsRead();
    void clearArchived();

signals:
    void notificationsChanged();
    void unreadCountChanged(int count);
    void notificationReceived(const QString& notificationId);
    void errorOccurred(const QString& message);

};

} // namespace bookclub::client
