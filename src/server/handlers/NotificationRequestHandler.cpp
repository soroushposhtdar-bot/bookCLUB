#include "src/server/handlers/NotificationRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"

#include <QJsonArray>
#include "common/Models/Notification.h"

namespace bookclub::server {

NotificationRequestHandler::NotificationRequestHandler(common::INotificationRepository* notificationRepo,
                                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_notificationRepo(notificationRepo)
{
    LOG_INFO("NotificationRequestHandler initialized");
}

void NotificationRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::GetNotifications:
            handleGetNotifications(payload, client);
            break;
        case common::Command::MarkNotificationRead:
            handleMarkNotificationRead(payload, client);
            break;
        case common::Command::MarkAllNotificationsRead:
            handleMarkAllNotificationsRead(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void NotificationRequestHandler::handleGetNotifications(const QJsonObject& payload, ClientConnection* client)
{
    QString userId = client->userId();

    QVector<common::Notification*> notifications = m_notificationRepo->findByUser(userId);
    QJsonArray notificationsArray;

    for (common::Notification* notif : notifications) {
        QJsonObject obj;
        obj["id"] = notif->id();
        obj["title"] = notif->title();
        obj["message"] = notif->message();
        obj["type"] = static_cast<int>(notif->type());
        obj["state"] = static_cast<int>(notif->state());
        obj["createdAt"] = notif->createdAt().toString(Qt::ISODate);
        obj["isRead"] = notif->isRead();
        notificationsArray.append(obj);
        delete notif;
    }

    QJsonObject responsePayload;
    responsePayload["notifications"] = notificationsArray;
    responsePayload["count"] = notificationsArray.size();

    sendSuccess(client, common::Command::GetNotifications, responsePayload);
}

void NotificationRequestHandler::handleMarkNotificationRead(const QJsonObject& payload, ClientConnection* client)
{
    QString notificationId = payload["notificationId"].toString();
    if (notificationId.isEmpty()) {
        sendError(client, common::Command::MarkNotificationRead, common::Status::BadRequest,
                  "notificationId is required");
        return;
    }

    if (!m_notificationRepo->markAsRead(notificationId)) {
        sendError(client, common::Command::MarkNotificationRead, common::Status::NotFound,
                  "Notification not found");
        return;
    }

    sendSuccess(client, common::Command::MarkNotificationRead, {});
    LOG_INFO("Notification marked as read: " + notificationId);
}

void NotificationRequestHandler::handleMarkAllNotificationsRead(const QJsonObject& payload, ClientConnection* client)
{
    if (!m_notificationRepo->markAllAsRead(client->userId())) {
        sendError(client, common::Command::MarkAllNotificationsRead, common::Status::InternalError,
                  "Failed to mark all as read");
        return;
    }

    sendSuccess(client, common::Command::MarkAllNotificationsRead, {});
    LOG_INFO("All notifications marked as read for user: " + client->userId());
}

} // namespace bookclub::server
