#include "common/Models/Notification.h"

namespace bookclub::common {

Notification::Notification(QObject* parent) : QObject(parent) {}

Notification::Notification(const QString& id, QObject* parent)
    : QObject(parent), m_id(id) {}

const QString& Notification::id() const { return m_id; }
const QString& Notification::receiverUserId() const { return m_receiverUserId; }
const QString& Notification::title() const { return m_title; }
const QString& Notification::message() const { return m_message; }
const QString& Notification::relatedEntityId() const { return m_relatedEntityId; }
const QDateTime& Notification::createdAt() const { return m_createdAt; }
const QDateTime& Notification::readAt() const { return m_readAt; }
NotificationType Notification::type() const { return m_type; }
NotificationState Notification::state() const { return m_state; }

void Notification::setId(const QString& id) { m_id = id; }
void Notification::setReceiverUserId(const QString& userId) { m_receiverUserId = userId; }
void Notification::setTitle(const QString& title) { m_title = title; }
void Notification::setMessage(const QString& message) { m_message = message; }
void Notification::setRelatedEntityId(const QString& id) { m_relatedEntityId = id; }
void Notification::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
void Notification::setReadAt(const QDateTime& readAt) { m_readAt = readAt; }
void Notification::setType(NotificationType type) { m_type = type; }

void Notification::setState(NotificationState state) {
    if (m_state != state) {
        m_state = state;
        if (state == NotificationState::Read && m_readAt.isNull()) {
            m_readAt = QDateTime::currentDateTime();
        }
        emit notificationChanged();
    }
}

bool Notification::isRead() const {
    return m_state == NotificationState::Read || m_state == NotificationState::Archived;
}

void Notification::markAsRead() {
    setState(NotificationState::Read);
}

} // namespace bookclub::common
