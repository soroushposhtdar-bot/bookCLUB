#pragma once

#include <QString>
#include <QVector>

namespace bookclub::common {

class Notification;

class INotificationRepository {
public:
    virtual ~INotificationRepository() = default;

    virtual bool save(Notification* notification) = 0;
    virtual bool update(Notification* notification) = 0;
    virtual Notification* findById(const QString& id) const = 0;
    virtual QVector<Notification*> findByUser(const QString& userId) const = 0;
    virtual QVector<Notification*> findUnreadByUser(const QString& userId) const = 0;
    virtual bool markAsRead(const QString& notificationId) = 0;
    virtual bool markAllAsRead(const QString& userId) = 0;
};

INotificationRepository* createNotificationRepository();

} // namespace bookclub::common
