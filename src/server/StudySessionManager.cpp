// src/server/StudySessionManager.cpp
#include "src/server/StudySessionManager.h"
#include "src/server/ConnectionManager.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/DateTimeUtils.h"
#include "common/Models/StudySession.h"
#include "common/Models/Notification.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::server {

StudySessionManager::StudySessionManager(ConnectionManager* connectionManager, QObject* parent)
    : QObject(parent)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("StudySessionManager initialized");
}

StudySessionManager::~StudySessionManager()
{
    qDeleteAll(m_sessions);
    m_sessions.clear();
}

common::StudySession* StudySessionManager::createSession(const QString& bookId, const QString& hostUserId)
{
    QMutexLocker locker(&m_mutex);

    auto* session = new common::StudySession(this);
    session->setId(common::IdGenerator::generateUuid());
    session->setBookId(bookId);
    session->setHostUserId(hostUserId);
    session->addParticipant(hostUserId);
    session->setState(common::StudySessionState::Created);
    session->setCreatedAt(common::DateTimeUtils::nowUtc());

    m_sessions[session->id()] = session;
    m_userToSessionMap[hostUserId] = session->id();

    LOG_INFO("Study session created: " + session->id() + " for book: " + bookId + " by: " + hostUserId);
    emit sessionCreated(session->id());

    // Bug 2: broadcast EvtSessionListUpdated so every connected client can
    // refresh its room list. We build the summary outside the lock by
    // re-entering activeSessionsSummary() — but that would deadlock on the
    // same mutex. Instead, build the payload inline here.
    if (m_connectionManager) {
        QJsonArray arr;
        for (common::StudySession* s : m_sessions) {
            if (s->state() == common::StudySessionState::Closed) continue;
            QJsonObject o;
            o["sessionId"]        = s->id();
            o["bookId"]           = s->bookId();
            o["hostUserId"]       = s->hostUserId();
            o["participantCount"] = s->participantUserIds().size();
            o["currentPage"]      = s->currentPage();
            o["state"]            = static_cast<int>(s->state());
            arr.append(o);
        }
        QJsonObject payload;
        payload["sessions"] = arr;
        common::Message event(common::Command::EvtSessionListUpdated,
                              common::Status::Success, payload);
        m_connectionManager->broadcastMessage(event);
    }

    return session;
}

bool StudySessionManager::joinSession(const QString& sessionId, const QString& userId)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) {
        LOG_WARNING("Session not found: " + sessionId);
        return false;
    }

    if (session->state() == common::StudySessionState::Closed) {
        LOG_WARNING("Session already closed: " + sessionId);
        return false;
    }

    const bool wasAlreadyIn = session->participantUserIds().contains(userId);

    if (!wasAlreadyIn) {
        session->addParticipant(userId);
        m_userToSessionMap[userId] = sessionId;
        session->setState(common::StudySessionState::Active);
    }

    LOG_INFO("User " + userId + " joined session: " + sessionId);
    emit sessionJoined(sessionId, userId);

    // Bug 8: broadcast EvtStudyParticipantChanged with the enriched
    // participant list so every client can refresh its participant panel
    // without re-fetching via JoinStudySession.
    {
        QJsonObject payload;
        payload["sessionId"] = sessionId;
        payload["event"]     = QStringLiteral("userJoined");
        payload["userId"]    = userId;
        payload["participants"] = enrichedParticipantsLocked(session);
        common::Message event(common::Command::EvtStudyParticipantChanged,
                              common::Status::Success, payload);
        broadcastToSession(sessionId, event);
    }

    return true;
}

bool StudySessionManager::leaveSession(const QString& sessionId, const QString& userId)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) return false;

    session->removeParticipant(userId);
    m_userToSessionMap.remove(userId);

    if (session->participantUserIds().isEmpty()) {
        // Close session if no participants
        session->setState(common::StudySessionState::Closed);
        emit sessionClosed(sessionId);
    }

    LOG_INFO("User " + userId + " left session: " + sessionId);
    emit sessionLeft(sessionId, userId);

    // Bug 8: broadcast participant change to remaining members (if any).
    if (session->state() != common::StudySessionState::Closed) {
        QJsonObject payload;
        payload["sessionId"] = sessionId;
        payload["event"]     = QStringLiteral("userLeft");
        payload["userId"]    = userId;
        payload["participants"] = enrichedParticipantsLocked(session);
        common::Message event(common::Command::EvtStudyParticipantChanged,
                              common::Status::Success, payload);
        broadcastToSession(sessionId, event);
    }

    return true;
}

bool StudySessionManager::closeSession(const QString& sessionId)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) return false;

    session->setState(common::StudySessionState::Closed);
    emit sessionClosed(sessionId);

    // Remove all users from mapping
    for (const QString& userId : session->participantUserIds()) {
        m_userToSessionMap.remove(userId);
    }

    // Broadcast session closed
    QJsonObject payload;
    payload["event"] = "sessionClosed";
    payload["sessionId"] = sessionId;
    broadcastToSession(sessionId, payload);

    return true;
}

void StudySessionManager::syncPage(const QString& sessionId, const QString& userId, int page, double zoom)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) return;

    session->setCurrentPage(page);
    session->setZoomLevel(zoom);
    session->setSynced(true);

    // Broadcast EvtStudySync to all participants so the reader page and
    // the participant list both update.
    QJsonObject payload;
    payload["sessionId"] = sessionId;
    payload["userId"]    = userId;
    payload["page"]      = page;
    payload["zoom"]      = zoom;
    common::Message event(common::Command::EvtStudySync,
                          common::Status::Success, payload);
    broadcastToSession(sessionId, event);

    emit pageUpdated(sessionId, userId, page);
}

void StudySessionManager::syncState(const QString& sessionId, const QString& userId)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) return;

    QJsonObject payload;
    payload["event"] = "stateSync";
    payload["page"] = session->currentPage();
    payload["zoom"] = session->zoomLevel();
    payload["participants"] = QJsonArray::fromStringList(session->participantUserIds());

    ClientConnection* conn = m_connectionManager ? m_connectionManager->getConnectionByUserId(userId) : nullptr;
    if (conn) {
        common::Message msg(common::Command::SyncStudyPage, common::Status::Success, payload);
        conn->sendMessage(msg);
    }
}

// Bug 3: in-room chat. Broadcasts an EvtStudyMessage to every participant.
void StudySessionManager::broadcastMessage(const QString& sessionId,
                                           const QString& userId,
                                           const QString& displayName,
                                           const QString& text)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) {
        LOG_WARNING("broadcastMessage: session not found: " + sessionId);
        return;
    }

    QJsonObject payload;
    payload["sessionId"]   = sessionId;
    payload["userId"]      = userId;
    payload["displayName"] = displayName;
    payload["text"]        = text;
    payload["timestamp"]   = QDateTime::currentDateTime().toString("HH:mm");
    common::Message event(common::Command::EvtStudyMessage,
                          common::Status::Success, payload);
    broadcastToSession(sessionId, event);
}

// Bug 4: invites. Sends an EvtNotification to each target user. The
// notification's payload includes the sessionId + hostUserId + a friendly
// message; the client's NotificationsViewModel renders it like any other
// notification. We do NOT add the target users to the session — they have
// to call JoinStudySession themselves once they accept the invite.
void StudySessionManager::inviteUsers(const QString& sessionId,
                                      const QString& hostUserId,
                                      const QStringList& targetUserIds)
{
    if (!m_connectionManager) return;

    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) {
        LOG_WARNING("inviteUsers: session not found: " + sessionId);
        return;
    }

    const QString hostName = [hostUserId]() -> QString {
        // The host's display name is best fetched from the user repo, but
        // StudySessionManager deliberately does not depend on the user
        // repository — the handler already looks it up and passes it in.
        // For the in-process broadcast we just use the userId as a fallback.
        return hostUserId;
    }();

    for (const QString& targetId : targetUserIds) {
        if (targetId.isEmpty() || targetId == hostUserId) continue;

        QJsonObject notifPayload;
        notifPayload["id"]              = common::IdGenerator::generateUuid();
        notifPayload["receiverUserId"]  = targetId;
        notifPayload["title"]           = QStringLiteral("Room invitation");
        notifPayload["message"]         = QStringLiteral("You've been invited to a group reading room.");
        notifPayload["relatedEntityId"] = sessionId;
        notifPayload["type"]            = static_cast<int>(common::NotificationType::Generic);
        notifPayload["state"]           = static_cast<int>(common::NotificationState::Unread);
        notifPayload["createdAt"]       = common::DateTimeUtils::nowUtc().toString(Qt::ISODateWithMs);
        notifPayload["sessionId"]       = sessionId;
        notifPayload["hostUserId"]      = hostUserId;
        notifPayload["hostDisplayName"] = hostName;
        notifPayload["bookId"]          = session->bookId();

        common::Message event(common::Command::EvtNotification,
                              common::Status::Success, notifPayload);
        m_connectionManager->sendToUser(targetId, event);
    }
}

// Bug 5: shared notes. Appends a NoteEntry to the session and broadcasts
// EvtStudyNote to every participant.
void StudySessionManager::addNote(const QString& sessionId,
                                  const QString& userId,
                                  const QString& displayName,
                                  const QString& text,
                                  int page)
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) {
        LOG_WARNING("addNote: session not found: " + sessionId);
        return;
    }

    common::NoteEntry entry;
    entry.id          = common::IdGenerator::generateUuid();
    entry.userId      = userId;
    entry.displayName = displayName;
    entry.text        = text;
    entry.page        = page;
    entry.timestamp   = common::DateTimeUtils::nowUtc();
    session->addNote(entry);

    QJsonObject payload;
    payload["sessionId"]   = sessionId;
    payload["id"]          = entry.id;
    payload["userId"]      = entry.userId;
    payload["displayName"] = entry.displayName;
    payload["text"]        = entry.text;
    payload["page"]        = entry.page;
    payload["timestamp"]   = entry.timestamp.toString("HH:mm");
    common::Message event(common::Command::EvtStudyNote,
                          common::Status::Success, payload);
    broadcastToSession(sessionId, event);
}

QJsonArray StudySessionManager::notesForSession(const QString& sessionId) const
{
    QMutexLocker locker(&m_mutex);

    common::StudySession* session = findSession(sessionId);
    if (!session) return {};

    QJsonArray arr;
    for (const common::NoteEntry& n : session->notes()) {
        QJsonObject o;
        o["id"]          = n.id;
        o["userId"]      = n.userId;
        o["displayName"] = n.displayName;
        o["text"]        = n.text;
        o["page"]        = n.page;
        o["timestamp"]   = n.timestamp.toString("HH:mm");
        arr.append(o);
    }
    return arr;
}

common::StudySession* StudySessionManager::getSession(const QString& sessionId) const
{
    QMutexLocker locker(&m_mutex);
    return findSession(sessionId);
}

QList<common::StudySession*> StudySessionManager::getActiveSessionsForBook(const QString& bookId) const
{
    QMutexLocker locker(&m_mutex);
    QList<common::StudySession*> result;
    for (common::StudySession* session : m_sessions) {
        if (session->bookId() == bookId && session->state() != common::StudySessionState::Closed) {
            result.append(session);
        }
    }
    return result;
}

QList<common::StudySession*> StudySessionManager::getSessionsForUser(const QString& userId) const
{
    QMutexLocker locker(&m_mutex);
    QList<common::StudySession*> result;
    for (common::StudySession* session : m_sessions) {
        if (session->participantUserIds().contains(userId)) {
            result.append(session);
        }
    }
    return result;
}

// Bug 2: snapshot of every active session. Each entry contains enough
// metadata for the client to render a room card without an extra round-trip.
QJsonArray StudySessionManager::activeSessionsSummary() const
{
    QMutexLocker locker(&m_mutex);
    QJsonArray arr;
    for (common::StudySession* s : m_sessions) {
        if (s->state() == common::StudySessionState::Closed) continue;
        QJsonObject o;
        o["sessionId"]        = s->id();
        o["bookId"]           = s->bookId();
        o["hostUserId"]       = s->hostUserId();
        o["participantCount"] = s->participantUserIds().size();
        o["currentPage"]      = s->currentPage();
        o["state"]            = static_cast<int>(s->state());
        arr.append(o);
    }
    return arr;
}

void StudySessionManager::broadcastToSession(const QString& sessionId, const common::Message& message)
{
    common::StudySession* session = findSession(sessionId);
    if (!session || !m_connectionManager) return;

    for (const QString& userId : session->participantUserIds()) {
        ClientConnection* conn = m_connectionManager->getConnectionByUserId(userId);
        if (conn) {
            conn->sendMessage(message);
        }
    }
}

void StudySessionManager::broadcastToSession(const QString& sessionId, const QJsonObject& payload)
{
    common::Message msg(common::Command::SyncStudyPage, common::Status::Success, payload);
    broadcastToSession(sessionId, msg);
}

common::StudySession* StudySessionManager::findSession(const QString& sessionId) const
{
    return m_sessions.value(sessionId, nullptr);
}

// Helper used by joinSession/leaveSession to build the enriched participant
// list that the client needs (displayName, initials, color, page, isHost…).
// Caller MUST hold m_mutex.
QJsonArray StudySessionManager::enrichedParticipantsLocked(common::StudySession* session) const
{
    if (!session) return {};
    QJsonArray arr;
    const QString hostUserId = session->hostUserId();
    for (const QString& uid : session->participantUserIds()) {
        QJsonObject p;
        p["userId"]     = uid;
        // We don't have the display name here (no user repo dependency).
        // The handler enriches the list at JoinStudySession time; for the
        // live EvtStudyParticipantChanged push we let the client merge by
        // userId (it already knows the display names of joined participants).
        p["displayName"]= uid;
        p["initials"]   = uid.left(2).toUpper();
        p["color"]      = QStringLiteral("#1A73E8");
        p["page"]       = session->currentPage();
        p["pageCount"]  = 0;
        p["online"]     = true;
        p["isHost"]     = (uid == hostUserId);
        arr.append(p);
    }
    return arr;
}

} // namespace bookclub::server
