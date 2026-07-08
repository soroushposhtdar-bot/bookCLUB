// src/server/NotificationDispatcher.cpp
#include "src/server/NotificationDispatcher.h"
#include "src/server/ConnectionManager.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/DateTimeUtils.h"

namespace bookclub::server {

NotificationDispatcher::NotificationDispatcher(ConnectionManager* connectionManager, QObject* parent)
    : QObject(parent)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("NotificationDispatcher initialized");
}

void NotificationDispatcher::sendNotification(common::Notification* notification)
{
    if (!notification || !m_connectionManager) return;

    // Send to specific user
    sendNotification(notification->receiverUserId(), notification);
}

void NotificationDispatcher::sendNotification(const QString& userId, common::Notification* notification)
{
    if (!notification || userId.isEmpty() || !m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    ClientConnection* conn = m_connectionManager->getConnectionByUserId(userId);
    if (conn) {
        sendToConnection(conn, notification);
        emit notificationSent(userId, notification->id());
    } else {
        LOG_WARNING("User not connected: " + userId + ". Notification stored for later.");
        // TODO: Store in database for offline users
    }
}

void NotificationDispatcher::sendNotifications(const QStringList& userIds, common::Notification* notification)
{
    if (!notification || userIds.isEmpty() || !m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    int sentCount = 0;
    for (const QString& userId : userIds) {
        ClientConnection* conn = m_connectionManager->getConnectionByUserId(userId);
        if (conn) {
            sendToConnection(conn, notification);
            emit notificationSent(userId, notification->id());
            sentCount++;
        }
    }

    LOG_INFO("Sent notification to " + QString::number(sentCount) + " users");
}

void NotificationDispatcher::broadcastSystemMessage(const QString& title, const QString& message)
{
    if (!m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    auto* notification = new common::Notification(this);
    notification->setId(common::IdGenerator::generateUuid());
    notification->setTitle(title);
    notification->setMessage(message);
    notification->setType(common::NotificationType::SystemAlert);
    notification->setCreatedAt(common::DateTimeUtils::nowUtc());

    QJsonObject payload = notificationToJson(notification);
    common::Message msg(common::Command::GetNotifications, common::Status::Success, payload);

    QList<ClientConnection*> clients = m_connectionManager->getAllConnections();
    for (ClientConnection* conn : clients) {
        conn->sendMessage(msg);
    }

    LOG_INFO("Broadcast system message to " + QString::number(clients.size()) + " clients");
    emit broadcastCompleted(clients.size());
    delete notification;
}

void NotificationDispatcher::broadcastToRole(common::AccountRole role, const QString& title, const QString& message)
{
    // TODO: Filter users by role from database
    // For now, broadcast to all authenticated users
    if (!m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    auto* notification = new common::Notification(this);
    notification->setId(common::IdGenerator::generateUuid());
    notification->setTitle(title);
    notification->setMessage(message);
    notification->setType(common::NotificationType::SystemAlert);
    notification->setCreatedAt(common::DateTimeUtils::nowUtc());

    QJsonObject payload = notificationToJson(notification);
    common::Message msg(common::Command::GetNotifications, common::Status::Success, payload);

    QList<ClientConnection*> clients = m_connectionManager->getAuthenticatedConnections();
    for (ClientConnection* conn : clients) {
        conn->sendMessage(msg);
    }

    delete notification;
}

void NotificationDispatcher::notifyNewBook(const QString& bookId, const QStringList& targetGenres)
{
    LOG_INFO("New book notification for genres: " + targetGenres.join(", "));
    broadcastSystemMessage("کتاب جدید", "یک کتاب جدید در ژانرهای مورد علاقه شما منتشر شد!");
}

void NotificationDispatcher::notifyDiscountOnBook(const QString& bookId, const QStringList& targetUserIds)
{
    LOG_INFO("Discount notification for " + QString::number(targetUserIds.size()) + " users");

    auto* notification = new common::Notification(this);
    notification->setId(common::IdGenerator::generateUuid());
    notification->setTitle("تخفیف ویژه");
    notification->setMessage("یکی از کتاب‌های ذخیره‌شده شما تخفیف خورد!");
    notification->setType(common::NotificationType::DiscountOnSavedBook);
    notification->setCreatedAt(common::DateTimeUtils::nowUtc());

    sendNotifications(targetUserIds, notification);
    delete notification;
}

void NotificationDispatcher::notifyNewSale(const QString& bookId, const QString& publisherId)
{
    LOG_INFO("New sale notification for publisher: " + publisherId);

    auto* notification = new common::Notification(this);
    notification->setId(common::IdGenerator::generateUuid());
    notification->setTitle("فروش جدید");
    notification->setMessage("کتاب شما فروخته شد!");
    notification->setType(common::NotificationType::SaleRegistered);
    notification->setCreatedAt(common::DateTimeUtils::nowUtc());

    sendNotification(publisherId, notification);
    delete notification;
}

void NotificationDispatcher::notifyNewReview(const QString& bookId, const QString& publisherId)
{
    LOG_INFO("New review notification for publisher: " + publisherId);

    auto* notification = new common::Notification(this);
    notification->setId(common::IdGenerator::generateUuid());
    notification->setTitle("نظر جدید");
    notification->setMessage("کاربری برای کتاب شما نظر جدید ثبت کرد!");
    notification->setType(common::NotificationType::NewReview);
    notification->setCreatedAt(common::DateTimeUtils::nowUtc());

    sendNotification(publisherId, notification);
    delete notification;
}

void NotificationDispatcher::sendToConnection(ClientConnection* conn, common::Notification* notification)
{
    if (!conn || !notification) return;

    QJsonObject payload = notificationToJson(notification);
    common::Message msg(common::Command::GetNotifications, common::Status::Success, payload);
    conn->sendMessage(msg);
}

QJsonObject NotificationDispatcher::notificationToJson(common::Notification* notification) const
{
    QJsonObject obj;
    obj["id"] = notification->id();
    obj["title"] = notification->title();
    obj["message"] = notification->message();
    obj["type"] = static_cast<int>(notification->type());
    obj["createdAt"] = notification->createdAt().toString(Qt::ISODateWithMs);
    obj["isRead"] = notification->isRead();
    return obj;
}

} // namespace bookclub::server
