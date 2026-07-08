// src/client/controllers/StudySessionController.cpp
#include "src/client/controllers/StudySessionController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "src/client/controllers/ReaderController.h"
#include "src/client/session/SessionManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

StudySessionController::StudySessionController(QObject* parent)
    : QObject(parent)
    , m_readerController(nullptr)
    , m_isInSession(false)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::CreateStudySession, [this](const common::Message& response) {
        handleCreateSessionResponse(response);
    });

    network.registerRequestHandler(common::Command::JoinStudySession, [this](const common::Message& response) {
        handleJoinSessionResponse(response);
    });

    network.registerRequestHandler(common::Command::LeaveStudySession, [this](const common::Message& response) {
        handleLeaveSessionResponse(response);
    });

    network.registerRequestHandler(common::Command::SyncStudyPage, [this](const common::Message& response) {
        handleSyncStateResponse(response);
    });

    LOG_INFO("StudySessionController initialized");
}

StudySessionController::~StudySessionController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::CreateStudySession);
    network.unregisterRequestHandler(common::Command::JoinStudySession);
    network.unregisterRequestHandler(common::Command::LeaveStudySession);
    network.unregisterRequestHandler(common::Command::SyncStudyPage);

    // Leave session if currently in one
    if (m_isInSession && !m_sessionId.isEmpty()) {
        leaveSession();
    }
}

// ---- Public Methods ----

void StudySessionController::createSession(const QString& bookId)
{
    LOG_DEBUG("StudySessionController::createSession() called for book: " + bookId);

    if (bookId.isEmpty()) {
        LOG_WARNING("Create session failed: book ID is empty");
        emit errorOccurred("Book ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Create session failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    if (m_isInSession) {
        LOG_WARNING("Create session failed: already in a session");
        emit errorOccurred("You are already in a session. Please leave it first");
        return;
    }

    QJsonObject payload;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::CreateStudySession, payload);

    LOG_INFO("Create session request sent for book: " + bookId);
}

void StudySessionController::joinSession(const QString& sessionId)
{
    LOG_DEBUG("StudySessionController::joinSession() called for session: " + sessionId);

    if (sessionId.isEmpty()) {
        LOG_WARNING("Join session failed: session ID is empty");
        emit errorOccurred("Session ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Join session failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    if (m_isInSession) {
        LOG_WARNING("Join session failed: already in a session");
        emit errorOccurred("You are already in a session. Please leave it first");
        return;
    }

    QJsonObject payload;
    payload["sessionId"] = sessionId;

    ClientNetworkManager::instance().sendRequest(common::Command::JoinStudySession, payload);

    LOG_INFO("Join session request sent for session: " + sessionId);
}

void StudySessionController::leaveSession()
{
    LOG_DEBUG("StudySessionController::leaveSession() called");

    if (!m_isInSession || m_sessionId.isEmpty()) {
        LOG_WARNING("Leave session failed: not in a session");
        emit errorOccurred("You are not in a session");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Leave session failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["sessionId"] = m_sessionId;

    ClientNetworkManager::instance().sendRequest(common::Command::LeaveStudySession, payload);

    // Clear local state
    m_sessionId.clear();
    m_bookId.clear();
    m_isInSession = false;
    m_participants.clear();

    emit sessionLeft();
    emit sessionUpdated();

    LOG_INFO("Leave session request sent");
}

void StudySessionController::setCurrentPage(int page)
{
    LOG_DEBUG("StudySessionController::setCurrentPage() called: " + QString::number(page));

    if (!m_isInSession || m_sessionId.isEmpty()) {
        LOG_WARNING("Set current page failed: not in a session");
        return;
    }

    m_currentPage = page;
    syncState();

    LOG_DEBUG("Current page set to: " + QString::number(page));
}

void StudySessionController::setZoom(double zoom)
{
    LOG_DEBUG("StudySessionController::setZoom() called: " + QString::number(zoom));

    if (!m_isInSession || m_sessionId.isEmpty()) {
        LOG_WARNING("Set zoom failed: not in a session");
        return;
    }

    m_zoom = zoom;
    syncState();

    LOG_DEBUG("Zoom set to: " + QString::number(zoom));
}

void StudySessionController::inviteParticipant(const QString& userId)
{
    LOG_DEBUG("StudySessionController::inviteParticipant() called for user: " + userId);

    if (!m_isInSession || m_sessionId.isEmpty()) {
        LOG_WARNING("Invite participant failed: not in a session");
        emit errorOccurred("You are not in a session");
        return;
    }

    if (userId.isEmpty()) {
        LOG_WARNING("Invite participant failed: user ID is empty");
        emit errorOccurred("User ID is required");
        return;
    }

    // This is a "bonus" feature - we'll just log it
    // In a real implementation, you might send a notification to the user
    LOG_INFO("Inviting user " + userId + " to session " + m_sessionId);

    // Emit session updated signal to refresh UI
    emit sessionUpdated();
}

void StudySessionController::syncState()
{
    LOG_DEBUG("StudySessionController::syncState() called");

    if (!m_isInSession || m_sessionId.isEmpty()) {
        LOG_WARNING("Sync state failed: not in a session");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Sync state failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["sessionId"] = m_sessionId;
    payload["page"] = m_currentPage;
    payload["zoom"] = m_zoom;

    ClientNetworkManager::instance().sendRequest(common::Command::SyncStudyPage, payload);

    LOG_DEBUG("State synced: page " + QString::number(m_currentPage) +
              ", zoom " + QString::number(m_zoom));
}

// ---- Response Handlers ----

void StudySessionController::handleCreateSessionResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to create session");
        LOG_WARNING("Create session failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_sessionId = data["sessionId"].toString();
    m_bookId = data["bookId"].toString();
    m_currentPage = 0;
    m_zoom = 1.0;
    m_isInSession = true;

    emit sessionCreated(m_sessionId);
    emit sessionUpdated();

    LOG_INFO("Study session created successfully: " + m_sessionId +
             " for book: " + m_bookId);
}

void StudySessionController::handleJoinSessionResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to join session");
        LOG_WARNING("Join session failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_sessionId = data["sessionId"].toString();
    m_bookId = data["bookId"].toString();
    m_currentPage = data["currentPage"].toInt();
    m_zoom = data["zoomLevel"].toDouble(1.0);

    // Parse participants
    m_participants.clear();
    QJsonArray participantsArray = data["participants"].toArray();
    for (const auto& participant : participantsArray) {
        m_participants.append(participant.toString());
    }

    m_isInSession = true;

    emit sessionJoined(m_sessionId);
    emit sessionUpdated();

    LOG_INFO("Joined session: " + m_sessionId +
             ", current page: " + QString::number(m_currentPage) +
             ", participants: " + QString::number(m_participants.size()));
}

void StudySessionController::handleLeaveSessionResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to leave session");
        LOG_WARNING("Leave session failed: " + error);
        emit errorOccurred(error);
        return;
    }

    // Local state was already cleared in leaveSession()
    LOG_INFO("Left session successfully");
}

void StudySessionController::handleSyncStateResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to sync session state");
        LOG_WARNING("Sync state failed: " + error);
        emit errorOccurred(error);
        return;
    }

    LOG_DEBUG("State synced successfully");

    // Check if the response contains updated state from other participants
    QJsonObject payload = response.payload();
    if (payload.contains("event")) {
        QString event = payload["event"].toString();

        if (event == "pageSync") {
            QString userId = payload["userId"].toString();
            int page = payload["page"].toInt();
            double zoom = payload["zoom"].toDouble();

            // Update local state if it's from someone else
            if (userId != SessionManager::instance().userId()) {
                m_currentPage = page;
                m_zoom = zoom;
                emit sessionUpdated();
                LOG_DEBUG("Page synced from " + userId + ": " + QString::number(page));
            }
        } else if (event == "userJoined") {
            QString userId = payload["userId"].toString();
            if (!m_participants.contains(userId)) {
                m_participants.append(userId);
                emit sessionUpdated();
                LOG_DEBUG("User joined: " + userId);
            }
        } else if (event == "userLeft") {
            QString userId = payload["userId"].toString();
            m_participants.removeAll(userId);
            emit sessionUpdated();
            LOG_DEBUG("User left: " + userId);
        } else if (event == "sessionClosed") {
            m_isInSession = false;
            m_sessionId.clear();
            m_bookId.clear();
            m_participants.clear();
            emit sessionLeft();
            emit sessionUpdated();
            LOG_INFO("Session closed by host");
        }
    }
}

// ---- Helper Methods ----

QStringList StudySessionController::getParticipants() const
{
    return m_participants;
}

int StudySessionController::getParticipantCount() const
{
    return m_participants.size();
}

bool StudySessionController::isParticipant(const QString& userId) const
{
    return m_participants.contains(userId);
}

QString StudySessionController::getSessionId() const
{
    return m_sessionId;
}

QString StudySessionController::getBookId() const
{
    return m_bookId;
}

int StudySessionController::getCurrentPage() const
{
    return m_currentPage;
}

double StudySessionController::getZoom() const
{
    return m_zoom;
}

bool StudySessionController::isInSession() const
{
    return m_isInSession;
}

void StudySessionController::setReaderController(ReaderController* reader)
{
    m_readerController = reader;
    LOG_DEBUG("ReaderController set for StudySessionController");

    if (m_readerController) {
        // Connect signals to sync with reader
        connect(m_readerController, &ReaderController::pageChanged,
                this, &StudySessionController::onReaderPageChanged);
        connect(m_readerController, &ReaderController::zoomChanged,
                this, &StudySessionController::onReaderZoomChanged);
    }
}

// ---- Private Slots for Reader Connection ----

void StudySessionController::onReaderPageChanged(int page)
{
    if (m_isInSession && m_sessionId.isEmpty()) {
        setCurrentPage(page);
    }
}

void StudySessionController::onReaderZoomChanged(double zoom)
{
    if (m_isInSession && m_sessionId.isEmpty()) {
        setZoom(zoom);
    }
}

} // namespace bookclub::client
