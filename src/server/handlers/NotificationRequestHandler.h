#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/INotificationRepository.h"

namespace bookclub::server {

class NotificationRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit NotificationRequestHandler(common::INotificationRepository* notificationRepo,
                                        QObject* parent = nullptr);
    ~NotificationRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetNotifications(const QJsonObject& payload, ClientConnection* client);
    void handleMarkNotificationRead(const QJsonObject& payload, ClientConnection* client);
    void handleMarkAllNotificationsRead(const QJsonObject& payload, ClientConnection* client);

    common::INotificationRepository* m_notificationRepo;
};

} // namespace bookclub::server
