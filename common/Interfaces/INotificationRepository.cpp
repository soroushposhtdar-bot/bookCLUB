// common/Interfaces/INotificationRepository.cpp
//
// SQLite-backed INotificationRepository.
#include "common/Interfaces/INotificationRepository.h"
#include "common/Models/Notification.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace bookclub::common {

namespace {
Notification* notifFromCurrentRecord(QSqlQuery& q)
{
    auto* n = new Notification;
    n->setId(q.value("id").toString());
    n->setReceiverUserId(q.value("receiverUserId").toString());
    n->setTitle(q.value("title").toString());
    n->setMessage(q.value("message").toString());
    n->setRelatedEntityId(q.value("relatedEntityId").toString());
    n->setType(static_cast<NotificationType>(q.value("type").toInt()));
    n->setState(static_cast<NotificationState>(q.value("state").toInt()));
    n->setCreatedAt(q.value("createdAt").toDateTime());
    n->setReadAt(q.value("readAt").toDateTime());
    return n;
}
} // namespace

// ============== Implementation ==============
class NotificationRepositoryImpl : public INotificationRepository {
public:
    bool save(Notification* n) override
    {
        if (!n) return false;
        if (n->id().isEmpty()) n->setId(IdGenerator::generateUuid());
        if (!n->createdAt().isValid()) n->setCreatedAt(QDateTime::currentDateTime());

        // UPSERT by id.
        const QString sql = QStringLiteral(
            "INSERT INTO Notifications (id, receiverUserId, title, message,"
            "  relatedEntityId, type, state, createdAt, readAt) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET "
            "  title = excluded.title, message = excluded.message,"
            "  relatedEntityId = excluded.relatedEntityId,"
            "  type = excluded.type, state = excluded.state,"
            "  readAt = excluded.readAt"
        );
        const QVariant readAt = n->readAt().isValid() ? QVariant(n->readAt()) : QVariant();
        return DbConnection::execOk(sql, {
            n->id(), n->receiverUserId(), n->title(), n->message(),
            n->relatedEntityId(),
            static_cast<int>(n->type()),
            static_cast<int>(n->state()),
            n->createdAt(),
            readAt
        });
    }

    bool update(Notification* n) override { return save(n); }

    Notification* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Notifications WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return notifFromCurrentRecord(q);
    }

    QVector<Notification*> findByUser(const QString& userId) const override
    {
        QVector<Notification*> out;
        auto q = DbConnection::run(
            "SELECT * FROM Notifications WHERE receiverUserId = ? "
            "ORDER BY createdAt DESC",
            {userId}
        );
        while (q.next()) out.append(notifFromCurrentRecord(q));
        return out;
    }

    QVector<Notification*> findUnreadByUser(const QString& userId) const override
    {
        QVector<Notification*> out;
        auto q = DbConnection::run(
            "SELECT * FROM Notifications WHERE receiverUserId = ? AND state = 0 "
            "ORDER BY createdAt DESC",
            {userId}
        );
        while (q.next()) out.append(notifFromCurrentRecord(q));
        return out;
    }

    bool markAsRead(const QString& notificationId) override
    {
        return DbConnection::execOk(
            "UPDATE Notifications SET state = 1, readAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), notificationId}
        );
    }

    bool markAllAsRead(const QString& userId) override
    {
        return DbConnection::execOk(
            "UPDATE Notifications SET state = 1, readAt = ? "
            "WHERE receiverUserId = ? AND state = 0",
            {QDateTime::currentDateTime(), userId}
        );
    }
};

// ============== Factory ==============
INotificationRepository* createNotificationRepository() {
    static NotificationRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
