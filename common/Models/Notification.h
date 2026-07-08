#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

#include "common/AppEnums.h"

namespace bookclub::common {

class Notification : public QObject {
    Q_OBJECT
public:
    explicit Notification(QObject* parent = nullptr);
    Notification(const QString& id, QObject* parent = nullptr);
    ~Notification() override = default;

    const QString& id() const;
    const QString& receiverUserId() const;
    const QString& title() const;
    const QString& message() const;
    const QString& relatedEntityId() const;
    const QDateTime& createdAt() const;
    const QDateTime& readAt() const;
    NotificationType type() const;
    NotificationState state() const;

    void setId(const QString& id);
    void setReceiverUserId(const QString& userId);
    void setTitle(const QString& title);
    void setMessage(const QString& message);
    void setRelatedEntityId(const QString& id);
    void setCreatedAt(const QDateTime& createdAt);
    void setReadAt(const QDateTime& readAt);
    void setType(NotificationType type);
    void setState(NotificationState state);

    bool isRead() const;
    void markAsRead();

signals:
    void notificationChanged();

private:
    QString m_id;
    QString m_receiverUserId;
    QString m_title;
    QString m_message;
    QString m_relatedEntityId;
    QDateTime m_createdAt;
    QDateTime m_readAt;
    NotificationType m_type = NotificationType::Generic;
    NotificationState m_state = NotificationState::Unread;
};

} // namespace bookclub::common
