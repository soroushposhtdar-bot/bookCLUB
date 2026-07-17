// src/client/controllers/NotificationController.h
#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class NotificationController : public QObject {
    Q_OBJECT
public:
    explicit NotificationController(QObject* parent = nullptr);
    ~NotificationController() override;

    void loadNotifications();
    void markAsRead(const QString& notificationId);
    void markAllAsRead();
    void clearArchived();

    // ---- Accessors ----
    QJsonArray getNotifications() const;
    int getNotificationCount() const;
    QJsonObject getNotification(const QString& notificationId) const;
    QJsonArray getUnreadNotifications() const;
    int countUnread() const;

signals:
    void notificationsChanged();
    void unreadCountChanged(int unreadCount);
    void errorOccurred(const QString& message);

private:
    void handleGetNotificationsResponse(const common::Message& response);
    void handleMarkAsReadResponse(const common::Message& response);
    void handleMarkAllAsReadResponse(const common::Message& response);

    QJsonObject m_notificationsData;
};

} // namespace bookclub::client
