#include "src/server/handlers/StudySessionRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/ConnectionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DateTimeUtils.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Models/UserAccount.h"

#include <QJsonArray>
#include <QJsonObject>
#include <memory>

namespace bookclub::server {

namespace {

// Pick a stable color from the user's id so participants are visually
// distinguishable in the participant list without needing per-user state.
QString colorForUserId(const QString& userId) {
    static const QStringList palette = {
        "#1A73E8", "#9C27B0", "#00897B", "#E65100",
        "#C2185B", "#3F51B5", "#7B1FA2", "#388E3C",
        "#F57C00", "#5D4037"
    };
    if (userId.isEmpty()) return palette.first();
    quint32 h = 0;
    for (const QChar c : userId) h = (h * 131) + static_cast<quint32>(c.unicode());
    return palette.at(h % palette.size());
}

QString initialsFromName(const QString& name) {
    if (name.isEmpty()) return QStringLiteral("?");
    const QStringList parts = name.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) return QStringLiteral("?");
    QString out;
    for (const QString& p : parts) {
        if (!p.isEmpty()) out.append(p.left(1).toUpper());
        if (out.length() >= 2) break;
    }
    return out;
}

// Look up the user's display name. Returns userId as a fallback if the
// repo can't find the user (so the client still has *something* to render).
QString displayNameForUser(const QString& userId) {
    if (userId.isEmpty()) return QStringLiteral("Anonymous");
    auto* repo = common::createUserRepository();
    if (!repo) return userId;
    std::unique_ptr<common::UserAccount> user(repo->findById(userId));
    if (!user) return userId;
    const QString dn = user->displayName();
    return dn.isEmpty() ? user->username() : dn;
}

} // namespace

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
        case common::Command::GetStudySessions:
            handleGetStudySessions(client);
            break;
        case common::Command::SendStudyMessage:
            handleSendStudyMessage(payload, client);
            break;
        case common::Command::InviteToStudySession:
            handleInviteToStudySession(payload, client);
            break;
        case common::Command::AddStudyNote:
            handleAddStudyNote(payload, client);
            break;
        case common::Command::GetStudyNotes:
            handleGetStudyNotes(payload, client);
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

    // Bug 7: enrich participants with display info so the client can render
    // the participant list without an extra GetCurrentUser round-trip per
    // user. Each entry has { userId, displayName, initials, color, page,
    // pageCount, online, isHost }.
    QJsonArray participants;
    const QString hostUserId = session->hostUserId();
    for (const QString& uid : session->participantUserIds()) {
        const QString dn = displayNameForUser(uid);
        QJsonObject p;
        p["userId"]      = uid;
        p["displayName"] = dn;
        p["initials"]    = initialsFromName(dn);
        p["color"]       = colorForUserId(uid);
        p["page"]        = session->currentPage();
        p["pageCount"]   = 0;
        p["online"]      = true;
        p["isHost"]      = (uid == hostUserId);
        participants.append(p);
    }

    QJsonObject responsePayload;
    responsePayload["sessionId"]    = session->id();
    responsePayload["bookId"]       = session->bookId();
    responsePayload["currentPage"]  = session->currentPage();
    responsePayload["zoomLevel"]    = session->zoomLevel();
    responsePayload["participants"] = participants;

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

// Bug 2: server-side handler for the GetStudySessions request.
void StudySessionRequestHandler::handleGetStudySessions(ClientConnection* client)
{
    QJsonObject responsePayload;
    responsePayload["sessions"] = m_sessionManager->activeSessionsSummary();
    sendSuccess(client, common::Command::GetStudySessions, responsePayload);
}

// Bug 3: server-side handler for the SendStudyMessage request. Forwards
// the message to the session manager, which broadcasts EvtStudyMessage.
void StudySessionRequestHandler::handleSendStudyMessage(const QJsonObject& payload, ClientConnection* client)
{
    const QString sessionId = payload["sessionId"].toString();
    const QString text      = payload["text"].toString();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::SendStudyMessage, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }
    if (text.trimmed().isEmpty()) {
        sendError(client, common::Command::SendStudyMessage, common::Status::BadRequest,
                  "text is required");
        return;
    }

    const QString userId = client->userId();
    const QString displayName = [userId]() {
        auto* repo = common::createUserRepository();
        if (!repo) return userId;
        std::unique_ptr<common::UserAccount> user(repo->findById(userId));
        if (!user) return userId;
        const QString dn = user->displayName();
        return dn.isEmpty() ? user->username() : dn;
    }();

    m_sessionManager->broadcastMessage(sessionId, userId, displayName, text);
    sendSuccess(client, common::Command::SendStudyMessage, {});
}

// Bug 4: server-side handler for InviteToStudySession. Validates that the
// caller is the host and forwards the target user ids to the manager.
void StudySessionRequestHandler::handleInviteToStudySession(const QJsonObject& payload, ClientConnection* client)
{
    const QString sessionId = payload["sessionId"].toString();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::InviteToStudySession, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }

    common::StudySession* session = m_sessionManager->getSession(sessionId);
    if (!session) {
        sendError(client, common::Command::InviteToStudySession, common::Status::NotFound,
                  "Session not found");
        return;
    }
    if (session->hostUserId() != client->userId()) {
        sendError(client, common::Command::InviteToStudySession, common::Status::Forbidden,
                  "Only the host can invite users");
        return;
    }

    // Accept either targetUserIds (preferred) or usernames (legacy).
    QStringList targetUserIds;
    if (payload.contains("targetUserIds")) {
        const QJsonArray arr = payload["targetUserIds"].toArray();
        for (const auto& v : arr) targetUserIds.append(v.toString());
    } else if (payload.contains("usernames")) {
        const QJsonArray arr = payload["usernames"].toArray();
        // Resolve usernames to userIds via the user repo.
        auto* repo = common::createUserRepository();
        if (repo) {
            for (const auto& v : arr) {
                const QString uname = v.toString();
                std::unique_ptr<common::UserAccount> u(repo->findByUsername(uname));
                if (u) targetUserIds.append(u->id());
            }
        }
    }
    if (targetUserIds.isEmpty()) {
        sendError(client, common::Command::InviteToStudySession, common::Status::BadRequest,
                  "targetUserIds is required");
        return;
    }

    m_sessionManager->inviteUsers(sessionId, client->userId(), targetUserIds);
    sendSuccess(client, common::Command::InviteToStudySession, {});
}

// Bug 5: server-side handler for AddStudyNote. Forwards to the manager
// which appends the note to the session and broadcasts EvtStudyNote.
void StudySessionRequestHandler::handleAddStudyNote(const QJsonObject& payload, ClientConnection* client)
{
    const QString sessionId = payload["sessionId"].toString();
    const QString text      = payload["text"].toString();
    const int page          = payload["page"].toInt();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::AddStudyNote, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }
    if (text.trimmed().isEmpty()) {
        sendError(client, common::Command::AddStudyNote, common::Status::BadRequest,
                  "text is required");
        return;
    }

    const QString userId = client->userId();
    const QString displayName = [userId]() {
        auto* repo = common::createUserRepository();
        if (!repo) return userId;
        std::unique_ptr<common::UserAccount> user(repo->findById(userId));
        if (!user) return userId;
        const QString dn = user->displayName();
        return dn.isEmpty() ? user->username() : dn;
    }();

    m_sessionManager->addNote(sessionId, userId, displayName, text, page);
    sendSuccess(client, common::Command::AddStudyNote, {});
}

// Bug 5: server-side handler for GetStudyNotes. Returns the session's
// full note history so a freshly-joined client can render the panel.
void StudySessionRequestHandler::handleGetStudyNotes(const QJsonObject& payload, ClientConnection* client)
{
    const QString sessionId = payload["sessionId"].toString();
    if (sessionId.isEmpty()) {
        sendError(client, common::Command::GetStudyNotes, common::Status::BadRequest,
                  "sessionId is required");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["sessionId"] = sessionId;
    responsePayload["notes"]     = m_sessionManager->notesForSession(sessionId);
    sendSuccess(client, common::Command::GetStudyNotes, responsePayload);
}

} // namespace bookclub::server
