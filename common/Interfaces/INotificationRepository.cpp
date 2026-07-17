// common/Interfaces/INotificationRepository.cpp
//
// Concrete `INotificationRepository` backed by the shared SQLite database.
// Fixes from the previous version:
//   - Use shared "bookclub_shared" connection.
//   - Include `<QVariantList>` and `<QSqlDatabase>` so `getQuery` parameter
//     inference works and `QVariant` is complete.

#include "common/Interfaces/INotificationRepository.h"
#include "common/Models/Notification.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
QSqlDatabase sharedDatabase()
{
    auto db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }
    db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
    db.setDatabaseName("bookclub.db");
    db.open();
    return db;
}

QSqlQuery runQuery(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery query(sharedDatabase());
    query.prepare(sql);
    for (const auto& p : params) {
        query.addBindValue(p);
    }
    query.exec();
    return query;
}

bool execOk(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery q = runQuery(sql, params);
    return q.lastError().type() == QSqlError::NoError;
}
} // namespace

// ========== Implementation ==========
class NotificationRepositoryImpl : public INotificationRepository {
public:
    NotificationRepositoryImpl() = default;
    ~NotificationRepositoryImpl() override = default;

    bool save(Notification* notification) override
    {
        if (!notification) return false;
        if (notification->id().isEmpty()) {
            notification->setId(IdGenerator::generateUuid());
        }

        const QString sql = R"(
            INSERT INTO Notifications (
                id, receiverUserId, title, message, relatedEntityId,
                type, state, createdAt, readAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        return execOk(sql, {
            notification->id(),
            notification->receiverUserId(),
            notification->title(),
            notification->message(),
            notification->relatedEntityId(),
            static_cast<int>(notification->type()),
            static_cast<int>(notification->state()),
            QDateTime::currentDateTime(),
            notification->readAt()
        });
    }

    bool update(Notification* notification) override
    {
        if (!notification || notification->id().isEmpty()) return false;

        const QString sql = R"(
            UPDATE Notifications SET
                title = ?,
                message = ?,
                state = ?,
                readAt = ?
            WHERE id = ?
        )";

        return execOk(sql, {
            notification->title(),
            notification->message(),
            static_cast<int>(notification->state()),
            notification->readAt(),
            notification->id()
        });
    }

    Notification* findById(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Notifications WHERE id = ?", {id});
        if (query.next()) {
            return createNotificationFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Notification*> findByUser(const QString& userId) const override
    {
        QVector<Notification*> notifications;
        QSqlQuery query = runQuery(
            "SELECT * FROM Notifications WHERE receiverUserId = ? "
            "ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            notifications.append(createNotificationFromCurrentRecord(query));
        }
        return notifications;
    }

    QVector<Notification*> findUnreadByUser(const QString& userId) const override
    {
        QVector<Notification*> notifications;
        QSqlQuery query = runQuery(
            "SELECT * FROM Notifications WHERE receiverUserId = ? "
            "AND state = ? ORDER BY createdAt DESC",
            {userId, static_cast<int>(NotificationState::Unread)}
        );
        while (query.next()) {
            notifications.append(createNotificationFromCurrentRecord(query));
        }
        return notifications;
    }

    bool markAsRead(const QString& notificationId) override
    {
        return execOk(
            "UPDATE Notifications SET state = ?, readAt = ? WHERE id = ?",
            {
                static_cast<int>(NotificationState::Read),
                QDateTime::currentDateTime(),
                notificationId
            }
        );
    }

    bool markAllAsRead(const QString& userId) override
    {
        return execOk(
            "UPDATE Notifications SET state = ?, readAt = ? "
            "WHERE receiverUserId = ? AND state = ?",
            {
                static_cast<int>(NotificationState::Read),
                QDateTime::currentDateTime(),
                userId,
                static_cast<int>(NotificationState::Unread)
            }
        );
    }

private:
    Notification* createNotificationFromCurrentRecord(QSqlQuery& query) const
    {
        QSqlRecord rec = query.record();
        auto* notification = new Notification;
        notification->setId(rec.value("id").toString());
        notification->setReceiverUserId(rec.value("receiverUserId").toString());
        notification->setTitle(rec.value("title").toString());
        notification->setMessage(rec.value("message").toString());
        notification->setRelatedEntityId(rec.value("relatedEntityId").toString());
        notification->setType(static_cast<NotificationType>(rec.value("type").toInt()));
        notification->setState(static_cast<NotificationState>(rec.value("state").toInt()));
        notification->setCreatedAt(rec.value("createdAt").toDateTime());
        notification->setReadAt(rec.value("readAt").toDateTime());
        return notification;
    }
};

// ========== Factory ==========
INotificationRepository* createNotificationRepository() {
    static NotificationRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
