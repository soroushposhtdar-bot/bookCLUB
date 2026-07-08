// src/client/controllers/NotificationController.cpp
#include "src/client/controllers/NotificationController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

NotificationController::NotificationController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::GetNotifications, [this](const common::Message& response) {
        handleGetNotificationsResponse(response);
    });

    network.registerRequestHandler(common::Command::MarkNotificationRead, [this](const common::Message& response) {
        handleMarkAsReadResponse(response);
    });

    network.registerRequestHandler(common::Command::MarkAllNotificationsRead, [this](const common::Message& response) {
        handleMarkAllAsReadResponse(response);
    });

    LOG_INFO("NotificationController initialized");
}

NotificationController::~NotificationController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::GetNotifications);
    network.unregisterRequestHandler(common::Command::MarkNotificationRead);
    network.unregisterRequestHandler(common::Command::MarkAllNotificationsRead);
}

// ---- Public Methods ----

void NotificationController::loadNotifications()
{
    LOG_DEBUG("NotificationController::loadNotifications() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load notifications failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetNotifications, {});
}

void NotificationController::markAsRead(const QString& notificationId)
{
    LOG_DEBUG("NotificationController::markAsRead() called for notification: " + notificationId);

    if (notificationId.isEmpty()) {
        LOG_WARNING("Mark as read failed: notification ID is empty");
        emit errorOccurred("Notification ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Mark as read failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["notificationId"] = notificationId;

    ClientNetworkManager::instance().sendRequest(common::Command::MarkNotificationRead, payload);
}

void NotificationController::markAllAsRead()
{
    LOG_DEBUG("NotificationController::markAllAsRead() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Mark all as read failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::MarkAllNotificationsRead, {});
}

void NotificationController::clearArchived()
{
    LOG_DEBUG("NotificationController::clearArchived() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Clear archived failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Since there's no direct clear archived command, we'll load all notifications
    // and then mark archived ones for deletion (or just reload)
    loadNotifications();
    LOG_INFO("Clear archived initiated - reloading notifications");
}

// ---- Response Handlers ----

void NotificationController::handleGetNotificationsResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load notifications");
        LOG_WARNING("Load notifications failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QJsonArray notifications = data["notifications"].toArray();
    int count = data["count"].toInt();

    m_notificationsData = data;

    // Count unread notifications
    int unreadCount = 0;
    for (const auto& notif : notifications) {
        QJsonObject notifObj = notif.toObject();
        bool isRead = notifObj["isRead"].toBool();
        if (!isRead) {
            unreadCount++;
        }
    }

    emit notificationsChanged();
    emit unreadCountChanged(unreadCount);

    LOG_INFO("Notifications loaded successfully. Count: " + QString::number(count) +
             ", Unread: " + QString::number(unreadCount));
}

void NotificationController::handleMarkAsReadResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to mark notification as read");
        LOG_WARNING("Mark as read failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Reload notifications to update state
    loadNotifications();

    LOG_INFO("Notification marked as read successfully");
}

void NotificationController::handleMarkAllAsReadResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to mark all notifications as read");
        LOG_WARNING("Mark all as read failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Reload notifications to update state
    loadNotifications();

    LOG_INFO("All notifications marked as read successfully");
}

// ---- Helper Methods ----

QJsonArray NotificationController::getNotifications() const
{
    return m_notificationsData["notifications"].toArray();
}

int NotificationController::getNotificationCount() const
{
    return m_notificationsData["count"].toInt();
}

QJsonObject NotificationController::getNotification(const QString& notificationId) const
{
    QJsonArray notifications = m_notificationsData["notifications"].toArray();
    for (const auto& notif : notifications) {
        QJsonObject notifObj = notif.toObject();
        if (notifObj["id"].toString() == notificationId) {
            return notifObj;
        }
    }
    return {};
}

QJsonArray NotificationController::getUnreadNotifications() const
{
    QJsonArray unreadArray;
    QJsonArray notifications = m_notificationsData["notifications"].toArray();
    for (const auto& notif : notifications) {
        QJsonObject notifObj = notif.toObject();
        bool isRead = notifObj["isRead"].toBool();
        if (!isRead) {
            unreadArray.append(notifObj);
        }
    }
    return unreadArray;
}

int NotificationController::countUnread() const
{
    int count = 0;
    QJsonArray notifications = m_notificationsData["notifications"].toArray();
    for (const auto& notif : notifications) {
        QJsonObject notifObj = notif.toObject();
        bool isRead = notifObj["isRead"].toBool();
        if (!isRead) {
            count++;
        }
    }
    return count;
}

} // namespace bookclub::client
