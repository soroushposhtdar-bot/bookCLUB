#include "src/server/handlers/StudySessionRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Models/StudySession.h"

namespace bookclub::server {

StudySessionRequestHandler::StudySessionRequestHandler(StudySessionManager* sessionManager,
                                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_sessionManager(sessionManager)
{
    LOG_INFO("StudySessionRequestHandler initialized");
}

void StudySessionRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::CreateStudySession:
            handleCreateStudySession(payload, client);
            break;
        case common::Command::JoinStudySession:
            handleJoinStudySession(payload, client);
            break;
        case common::Command::LeaveStudySession:
            handleLeaveStudySession(payload, client);
            break;
        case common::Command::SyncStudyPage:
            handleSyncStudyPage(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void StudySessionRequestHandler::handleCreateStudySession(const QJsonObject& payload, ClientConnection* client)
{
    QString bookId = payload["bookId"].toString();
    if (bookId.isEmpty()) {
        sendError(client, common::Command::CreateStudySession, common::Status::BadRequest,
                  "bookId is required");
        return;
    }

    common::StudySession* session = m_sessionManager->createSession(bookId, client->userId());
    if (!session) {
        sendError(client, common::Command::CreateStudySession, common::Status::InternalError,
                  "Failed to create session");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["sessionId"] = session->id();
    responsePayload["bookId"] = session->bookId();
    responsePayload["hostUserId"] = session->hostUserId();
    responsePayload["state"] = static_cast<int>(session->state());

    sendSuccess(client, common::Command::CreateStudySession, responsePayload);
    LOG_INFO("Study session created: " + session->id() + " by user: " + client->userId());
}

void StudySessionRequestHandler::handleJoinStudySession(const QJsonObject& payload, ClientConnection* client)
{
    QString sessionId = payload["sessionId"].toString();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::JoinStudySession, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }

    if (!m_sessionManager->joinSession(sessionId, client->userId())) {
        sendError(client, common::Command::JoinStudySession, common::Status::NotFound,
                  "Session not found or already closed");
        return;
    }

    common::StudySession* session = m_sessionManager->getSession(sessionId);
    if (!session) {
        sendError(client, common::Command::JoinStudySession, common::Status::NotFound,
                  "Session not found");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["sessionId"] = session->id();
    responsePayload["bookId"] = session->bookId();
    responsePayload["currentPage"] = session->currentPage();
    responsePayload["zoomLevel"] = session->zoomLevel();
    responsePayload["participants"] = QJsonArray::fromStringList(session->participantUserIds());

    sendSuccess(client, common::Command::JoinStudySession, responsePayload);
    LOG_INFO("User " + client->userId() + " joined session: " + sessionId);
}

void StudySessionRequestHandler::handleLeaveStudySession(const QJsonObject& payload, ClientConnection* client)
{
    QString sessionId = payload["sessionId"].toString();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::LeaveStudySession, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }

    if (!m_sessionManager->leaveSession(sessionId, client->userId())) {
        sendError(client, common::Command::LeaveStudySession, common::Status::NotFound,
                  "Session not found");
        return;
    }

    sendSuccess(client, common::Command::LeaveStudySession, {});
    LOG_INFO("User " + client->userId() + " left session: " + sessionId);
}

void StudySessionRequestHandler::handleSyncStudyPage(const QJsonObject& payload, ClientConnection* client)
{
    QString sessionId = payload["sessionId"].toString();
    int page = payload["page"].toInt();
    double zoom = payload["zoom"].toDouble(1.0);

    if (sessionId.isEmpty()) {
        sendError(client, common::Command::SyncStudyPage, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }

    m_sessionManager->syncPage(sessionId, client->userId(), page, zoom);

    sendSuccess(client, common::Command::SyncStudyPage, {});
}

} // namespace bookclub::server
