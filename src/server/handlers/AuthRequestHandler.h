// src/server/handlers/AuthRequestHandler.h
#pragma once

#include "src/server/RequestHandlerBase.h"
#include "common/Interfaces/IAuthService.h"
#include "common/Interfaces/IUserRepository.h"

namespace bookclub::server {

class AuthRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit AuthRequestHandler(common::IAuthService* authService,
                                common::IUserRepository* userRepo,
                                QObject* parent = nullptr);
    ~AuthRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleLogin(const QJsonObject& payload, ClientConnection* client);
    void handleRegister(const QJsonObject& payload, ClientConnection* client);
    void handleResetPassword(const QJsonObject& payload, ClientConnection* client);
    void handleChangePassword(const QJsonObject& payload, ClientConnection* client);
    void handleLogout(const QJsonObject& payload, ClientConnection* client);

    common::UserAccount* createUserFromPayload(const QJsonObject& payload);

    common::IAuthService* m_authService;
    common::IUserRepository* m_userRepo;
};

} // namespace bookclub::server
