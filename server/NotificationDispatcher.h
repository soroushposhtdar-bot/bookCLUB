#pragma once

#include <QObject>
#include <QStringList>

#include "common/AppEnums.h"

namespace bookclub::common {
class Notification;
}

namespace bookclub::server {

class ConnectionManager;
class NotificationDispatcher : public QObject {
    Q_OBJECT
public:
    explicit NotificationDispatcher(QObject* parent = nullptr);
    ~NotificationDispatcher() override = default;

    void setConnectionManager(ConnectionManager* manager);
    void dispatchToUser(const QString& userId, bookclub::common::Notification* notification);
    void dispatchToRole(bookclub::common::AccountRole role, bookclub::common::Notification* notification);
    void dispatchBroadcast(bookclub::common::Notification* notification);

signals:
    void notificationQueued(const QString& notificationId);
    void notificationDispatched(const QString& notificationId);
    void notificationFailed(const QString& reason);

private:
    ConnectionManager* m_connectionManager = nullptr;
};

} // namespace bookclub::server
