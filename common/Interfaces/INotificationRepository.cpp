// common/interfaces/INotificationRepository.cpp
#include "common/interfaces/INotificationRepository.h"
#include "common/models/Notification.h"
#include "common/utils/IdGenerator.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::common {

class NotificationRepositoryImpl : public INotificationRepository {
public:
    NotificationRepositoryImpl() = default;
    ~NotificationRepositoryImpl() override = default;

    bool save(Notification* notification) override {
        if (!notification) return false;
        if (notification->id().isEmpty()) {
            notification->setId(IdGenerator::generateUuid());
        }

        QString sql = R"(
            INSERT INTO Notifications (
                id, receiverUserId, title, message, relatedEntityId,
                type, state, createdAt, readAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getQuery(sql, {
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

        return query.lastError().type() == QSqlError::NoError;
    }

    bool update(Notification* notification) override {
        if (!notification || notification->id().isEmpty()) return false;

        QString sql = R"(
            UPDATE Notifications SET
                title = ?,
                message = ?,
                state = ?,
                readAt = ?
            WHERE id = ?
        )";

        QSqlQuery query = getQuery(sql, {
            notification->title(),
            notification->message(),
            static_cast<int>(notification->state()),
            notification->readAt(),
            notification->id()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    Notification* findById(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT * FROM Notifications WHERE id = ?", {id});
        if (query.next()) {
            return createNotificationFromCurrentRecord(query);
        }
        return nullptr;
    }

    QVector<Notification*> findByUser(const QString& userId) const override {
        QVector<Notification*> notifications;
        QSqlQuery query = getQuery(
            "SELECT * FROM Notifications WHERE receiverUserId = ? "
            "ORDER BY createdAt DESC",
            {userId}
        );
        while (query.next()) {
            notifications.append(createNotificationFromCurrentRecord(query));
        }
        return notifications;
    }

    QVector<Notification*> findUnreadByUser(const QString& userId) const override {
        QVector<Notification*> notifications;
        QSqlQuery query = getQuery(
            "SELECT * FROM Notifications WHERE receiverUserId = ? "
            "AND state = ? ORDER BY createdAt DESC",
            {userId, static_cast<int>(NotificationState::Unread)}
        );
        while (query.next()) {
            notifications.append(createNotificationFromCurrentRecord(query));
        }
        return notifications;
    }

    bool markAsRead(const QString& notificationId) override {
        QSqlQuery query = getQuery(
            "UPDATE Notifications SET state = ?, readAt = ? WHERE id = ?",
            {
                static_cast<int>(NotificationState::Read),
                QDateTime::currentDateTime(),
                notificationId
            }
        );
        return query.lastError().type() == QSqlError::NoError;
    }

    bool markAllAsRead(const QString& userId) override {
        QSqlQuery query = getQuery(
            "UPDATE Notifications SET state = ?, readAt = ? "
            "WHERE receiverUserId = ? AND state = ?",
            {
                static_cast<int>(NotificationState::Read),
                QDateTime::currentDateTime(),
                userId,
                static_cast<int>(NotificationState::Unread)
            }
        );
        return query.lastError().type() == QSqlError::NoError;
    }

private:
    // ====== Helper methods ======
    QSqlQuery getQuery(const QString& sql, const QVariantList& params = {}) const {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);
        query.prepare(sql);
        for (const auto& param : params) {
            query.addBindValue(param);
        }
        query.exec();
        return query;
    }

    QSqlDatabase getDatabase() const {
        static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if (!db.isOpen()) {
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        return db;
    }

    Notification* createNotificationFromCurrentRecord(const QSqlQuery& query) const {
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

// ========== Factory function ==========
INotificationRepository* createNotificationRepository() {
    static NotificationRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
