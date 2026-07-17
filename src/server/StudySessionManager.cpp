// src/server/StudySessionManager.cpp
#include "src/server/StudySessionManager.h"
#include "src/server/ConnectionManager.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/DateTimeUtils.h"

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

    if (session->participantUserIds().contains(userId)) {
        LOG_WARNING("User already in session: " + userId);
        return true;
    }

    session->addParticipant(userId);
    m_userToSessionMap[userId] = sessionId;
    session->setState(common::StudySessionState::Active);

    LOG_INFO("User " + userId + " joined session: " + sessionId);
    emit sessionJoined(sessionId, userId);

    // Broadcast to all participants
    QJsonObject payload;
    payload["event"] = "userJoined";
    payload["userId"] = userId;
    payload["sessionId"] = sessionId;
    broadcastToSession(sessionId, payload);

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

    // Broadcast to remaining participants
    QJsonObject payload;
    payload["event"] = "userLeft";
    payload["userId"] = userId;
    broadcastToSession(sessionId, payload);

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

    // Broadcast to all other participants
    QJsonObject payload;
    payload["event"] = "pageSync";
    payload["userId"] = userId;
    payload["page"] = page;
    payload["zoom"] = zoom;
    broadcastToSession(sessionId, payload);

    emit pageUpdated(sessionId, userId, page);
}

void StudySessionManager::syncState(const QString& sessionId, const QString& userId)
{
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

} // namespace bookclub::server
