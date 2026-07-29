// src/server/NotificationDispatcher.cpp
//
// Real-time notification push. All events use the dedicated
// Command::EvtNotification channel (kept separate from GetNotifications
// which is the pull/request-response channel).
//
// Every pushed notification is also persisted via INotificationRepository
// so users who are offline at push time will see it on next login.
#include "src/server/NotificationDispatcher.h"
#include "src/server/ConnectionManager.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/DateTimeUtils.h"
#include "common/Interfaces/INotificationRepository.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Interfaces/IBookRepository.h"
#include "common/Models/Notification.h"
#include "common/Models/UserAccount.h"
#include "common/Models/RegularUser.h"
#include "common/Models/Publisher.h"
#include "common/Models/Book.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <memory>

namespace bookclub::server {

NotificationDispatcher::NotificationDispatcher(ConnectionManager* connectionManager, QObject* parent)
    : QObject(parent)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("NotificationDispatcher initialized");
}

void NotificationDispatcher::sendNotification(common::Notification* notification)
{
    if (!notification || !m_connectionManager) return;
    sendNotification(notification->receiverUserId(), notification);
}

void NotificationDispatcher::sendNotification(const QString& userId, common::Notification* notification)
{
    if (!notification || userId.isEmpty() || !m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    // Always persist so the user sees it on next login even if currently offline.
    notification->setReceiverUserId(userId);
    auto* notifRepo = common::createNotificationRepository();
    notifRepo->save(notification);

    ClientConnection* conn = m_connectionManager->getConnectionByUserId(userId);
    if (conn) {
        sendToConnection(conn, notification);
        emit notificationSent(userId, notification->id());
    } else {
        LOG_DEBUG("User not connected: " + userId + ". Notification stored for later.");
    }
}

void NotificationDispatcher::sendNotifications(const QStringList& userIds, common::Notification* notification)
{
    if (!notification || userIds.isEmpty() || !m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    int sentCount = 0;
    for (const QString& userId : userIds) {
        // Build a fresh Notification for each user (QObjects can't be copied).
        common::Notification* copy = new common::Notification;
        copy->setId(common::IdGenerator::generateUuid());
        copy->setReceiverUserId(userId);
        copy->setTitle(notification->title());
        copy->setMessage(notification->message());
        copy->setRelatedEntityId(notification->relatedEntityId());
        copy->setType(notification->type());
        copy->setState(notification->state());
        copy->setCreatedAt(notification->createdAt().isValid()
                           ? notification->createdAt()
                           : common::DateTimeUtils::nowUtc());

        auto* notifRepo = common::createNotificationRepository();
        notifRepo->save(copy);

        ClientConnection* conn = m_connectionManager->getConnectionByUserId(userId);
        if (conn) {
            sendToConnection(conn, copy);
            emit notificationSent(userId, copy->id());
            sentCount++;
        }
        delete copy;
    }

    LOG_INFO("Sent notification to " + QString::number(sentCount) + " users");
}

void NotificationDispatcher::broadcastSystemMessage(const QString& title, const QString& message)
{
    if (!m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    // Persist one copy per user.
    auto* notifRepo = common::createNotificationRepository();
    auto* userRepo  = common::createUserRepository();
    QVector<common::UserAccount*> users = userRepo->findAll();

    const QDateTime now = common::DateTimeUtils::nowUtc();
    for (common::UserAccount* u : users) {
        common::Notification* n = new common::Notification;
        n->setId(common::IdGenerator::generateUuid());
        n->setReceiverUserId(u->id());
        n->setTitle(title);
        n->setMessage(message);
        n->setType(common::NotificationType::SystemAlert);
        n->setCreatedAt(now);
        notifRepo->save(n);
        delete n;

        // Push to the live socket if any.
        ClientConnection* conn = m_connectionManager->getConnectionByUserId(u->id());
        if (conn) {
            common::Notification live;
            live.setId(common::IdGenerator::generateUuid());
            live.setReceiverUserId(u->id());
            live.setTitle(title);
            live.setMessage(message);
            live.setType(common::NotificationType::SystemAlert);
            live.setCreatedAt(now);
            sendToConnection(conn, &live);
        }
        delete u;
    }

    emit broadcastCompleted(users.size());
    LOG_INFO("Broadcast system message to " + QString::number(users.size()) + " users");
}

void NotificationDispatcher::broadcastToRole(common::AccountRole role, const QString& title, const QString& message)
{
    if (!m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    auto* notifRepo = common::createNotificationRepository();
    auto* userRepo  = common::createUserRepository();
    QVector<common::UserAccount*> users = userRepo->findAll();

    const QDateTime now = common::DateTimeUtils::nowUtc();
    int sent = 0;
    for (common::UserAccount* u : users) {
        if (u->role() != role) { delete u; continue; }

        common::Notification* n = new common::Notification;
        n->setId(common::IdGenerator::generateUuid());
        n->setReceiverUserId(u->id());
        n->setTitle(title);
        n->setMessage(message);
        n->setType(common::NotificationType::SystemAlert);
        n->setCreatedAt(now);
        notifRepo->save(n);
        delete n;

        ClientConnection* conn = m_connectionManager->getConnectionByUserId(u->id());
        if (conn) {
            common::Notification live;
            live.setId(common::IdGenerator::generateUuid());
            live.setReceiverUserId(u->id());
            live.setTitle(title);
            live.setMessage(message);
            live.setType(common::NotificationType::SystemAlert);
            live.setCreatedAt(now);
            sendToConnection(conn, &live);
            sent++;
        }
        delete u;
    }
    LOG_INFO("Broadcast to role " + QString::number(static_cast<int>(role))
             + ": " + QString::number(sent) + " recipients");
}

void NotificationDispatcher::notifyNewBook(const QString& bookId, const QStringList& targetGenres)
{
    LOG_INFO("New book notification for genres: " + targetGenres.join(", "));

    // Find every user whose favourite genres intersect `targetGenres`.
    auto* userRepo = common::createUserRepository();
    QVector<common::UserAccount*> users = userRepo->findAll();
    QStringList targets;
    for (common::UserAccount* u : users) {
        auto* regular = dynamic_cast<common::RegularUser*>(u);
        if (!regular) { delete u; continue; }
        for (const QString& g : regular->favoriteGenreIds()) {
            if (targetGenres.contains(g, Qt::CaseInsensitive)) {
                targets.append(u->id());
                break;
            }
        }
        delete u;
    }

    if (targets.isEmpty()) return;

    common::Notification n;
    n.setTitle("New book in your favourite genre");
    n.setMessage("A new book matching your favourite genres was just published!");
    n.setRelatedEntityId(bookId);
    n.setType(common::NotificationType::NewBookInFavoriteGenre);
    n.setCreatedAt(common::DateTimeUtils::nowUtc());
    sendNotifications(targets, &n);
}

void NotificationDispatcher::notifyDiscountOnBook(const QString& bookId, const QStringList& targetUserIds)
{
    LOG_INFO("Discount notification for " + QString::number(targetUserIds.size()) + " users");
    common::Notification n;
    n.setTitle("Special discount");
    n.setMessage("One of your saved books just went on sale!");
    n.setRelatedEntityId(bookId);
    n.setType(common::NotificationType::DiscountOnSavedBook);
    n.setCreatedAt(common::DateTimeUtils::nowUtc());
    sendNotifications(targetUserIds, &n);
}

// v15i: broadcast an EvtDiscountApplied event to ALL connected clients.
// This is separate from notifyDiscountOnBook (which only sends a
// Notification to specific users). The event tells every client to
// invalidate their book cache so the discounted price shows up in
// real-time on the Home page, Search page, and Book Detail page.
void NotificationDispatcher::broadcastDiscountApplied(const QString& bookId, double discountValue)
{
    QJsonObject payload;
    payload["bookId"] = bookId;
    payload["discountValue"] = discountValue;
    common::Message event(common::Command::EvtDiscountApplied,
                          common::Status::Success, payload);
    // Broadcast to all connected clients.
    if (m_connectionManager) {
        m_connectionManager->broadcastMessage(event);
    }
    LOG_INFO("Broadcast EvtDiscountApplied for book: " + bookId);
}

void NotificationDispatcher::notifyNewSale(const QString& bookId, const QString& publisherId)
{
    LOG_INFO("New sale notification for publisher: " + publisherId);

    // v22 (Issue 3): look up the book so the notification message includes
    // the actual title + sale price. Previously the notification just said
    // "Your book was just purchased!" with no context — the publisher had
    // no idea WHICH book sold.
    QString title = QStringLiteral("your book");
    double price = 0.0;
    auto* bookRepo = common::createBookRepository();
    if (bookRepo) {
        std::unique_ptr<common::Book> book(bookRepo->findById(bookId));
        if (book) {
            title = book->title().isEmpty() ? title : book->title();
            price = book->price();
        }
    }

    common::Notification n;
    n.setId(common::IdGenerator::generateUuid());
    n.setReceiverUserId(publisherId);
    n.setTitle(QStringLiteral("New sale: ") + title);
    n.setMessage(QStringLiteral("Sold for $%1")
                  .arg(QString::number(price, 'f', 2)));
    n.setRelatedEntityId(bookId);
    n.setType(common::NotificationType::SaleRegistered);
    n.setCreatedAt(common::DateTimeUtils::nowUtc());
    // sendNotification(userId, notif) already persists to the DB via
    // INotificationRepository and pushes via the live socket if the
    // publisher is online. So no extra save() call is needed here.
    sendNotification(publisherId, &n);
}

void NotificationDispatcher::notifyNewReview(const QString& bookId, const QString& publisherId)
{
    LOG_INFO("New review notification for publisher: " + publisherId);
    common::Notification n;
    n.setReceiverUserId(publisherId);
    n.setTitle("New review");
    n.setMessage("A user just reviewed your book!");
    n.setRelatedEntityId(bookId);
    n.setType(common::NotificationType::NewReview);
    n.setCreatedAt(common::DateTimeUtils::nowUtc());
    sendNotification(publisherId, &n);
}

void NotificationDispatcher::sendToConnection(ClientConnection* conn, common::Notification* notification)
{
    if (!conn || !notification) return;

    QJsonObject payload = notificationToJson(notification);
    common::Message msg(common::Command::EvtNotification,
                        common::Status::Success, payload);
    conn->sendMessage(msg);
}

QJsonObject NotificationDispatcher::notificationToJson(common::Notification* notification) const
{
    QJsonObject obj;
    obj["id"]               = notification->id();
    obj["receiverUserId"]   = notification->receiverUserId();
    obj["title"]            = notification->title();
    obj["message"]          = notification->message();
    obj["relatedEntityId"]  = notification->relatedEntityId();
    obj["type"]             = static_cast<int>(notification->type());
    obj["state"]            = static_cast<int>(notification->state());
    obj["createdAt"]        = notification->createdAt().toString(Qt::ISODateWithMs);
    obj["isRead"]           = notification->isRead();
    return obj;
}

} // namespace bookclub::server
