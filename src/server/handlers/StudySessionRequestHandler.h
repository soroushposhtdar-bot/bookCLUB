#pragma once

#include "src/server/RequestHandlerBase.h"
#include "src/server/StudySessionManager.h"

namespace bookclub::server {

class StudySessionRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit StudySessionRequestHandler(StudySessionManager* sessionManager,
                                        QObject* parent = nullptr);
    ~StudySessionRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleCreateStudySession(const QJsonObject& payload, ClientConnection* client);
    void handleJoinStudySession(const QJsonObject& payload, ClientConnection* client);
    void handleLeaveStudySession(const QJsonObject& payload, ClientConnection* client);
    void handleSyncStudyPage(const QJsonObject& payload, ClientConnection* client);

    StudySessionManager* m_sessionManager;
};

} // namespace bookclub::server
