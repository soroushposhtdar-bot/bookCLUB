// src/server/NotificationDispatcher.h
#pragma once

#include <QObject>
#include <QMap>
#include <QList>
#include <QPointer>
#include <QMutex>

#include "common/Models/Notification.h"
#include "common/Network/Message.h"

namespace bookclub::server {

class ConnectionManager;
class ClientConnection;

class NotificationDispatcher : public QObject {
    Q_OBJECT
public:
    explicit NotificationDispatcher(ConnectionManager* connectionManager, QObject* parent = nullptr);
    ~NotificationDispatcher() override = default;

    // --- Send Notification ---
    void sendNotification(common::Notification* notification);
    void sendNotification(const QString& userId, common::Notification* notification);
    void sendNotifications(const QStringList& userIds, common::Notification* notification);

    // --- Broadcast ---
    void broadcastSystemMessage(const QString& title, const QString& message);
    void broadcastToRole(common::AccountRole role, const QString& title, const QString& message);

    // --- Real-time Events ---
    void notifyNewBook(const QString& bookId, const QStringList& targetGenres);
    void notifyDiscountOnBook(const QString& bookId, const QStringList& targetUserIds);
    void notifyNewSale(const QString& bookId, const QString& publisherId);
    void notifyNewReview(const QString& bookId, const QString& publisherId);

signals:
    void notificationSent(const QString& userId, const QString& notificationId);
    void broadcastCompleted(int count);

private:
    void sendToConnection(ClientConnection* conn, common::Notification* notification);
    QJsonObject notificationToJson(common::Notification* notification) const;

    QPointer<ConnectionManager> m_connectionManager;
    QMutex m_mutex;
};

} // namespace bookclub::server
