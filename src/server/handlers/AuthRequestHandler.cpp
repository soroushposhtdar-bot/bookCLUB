// src/server/handlers/AuthRequestHandler.cpp
#include "src/server/handlers/AuthRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/Logger.h"
#include "common/Utils/PasswordHasher.h"
#include "common/Utils/IdGenerator.h"

namespace bookclub::server {

AuthRequestHandler::AuthRequestHandler(common::IAuthService* authService,
                                       common::IUserRepository* userRepo,
                                       QObject* parent)
    : RequestHandlerBase(parent)
    , m_authService(authService)
    , m_userRepo(userRepo)
{
    LOG_INFO("AuthRequestHandler initialized");
}

void AuthRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client) {
        LOG_ERROR("AuthRequestHandler: null client");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::Login:
            handleLogin(payload, client);
            break;
        case common::Command::Register:
            handleRegister(payload, client);
            break;
        case common::Command::ResetPassword:
            handleResetPassword(payload, client);
            break;
        case common::Command::ChangePassword:
            handleChangePassword(payload, client);
            break;
        case common::Command::Logout:
            handleLogout(payload, client);
            break;
        default:
            LOG_WARNING("AuthRequestHandler: Unhandled command " + common::commandToString(cmd));
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

// --- Login ---
void AuthRequestHandler::handleLogin(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("Login request from client: " + client->clientId());

    QString missingField;
    if (!validateRequiredFields(payload, {"username", "password"}, missingField)) {
        sendError(client, common::Command::Login, common::Status::BadRequest,
                  "Missing field: " + missingField);
        return;
    }

    QString username = payload["username"].toString();
    QString password = payload["password"].toString();

    common::UserAccount* user = m_authService->login(username, password);
    if (!user) {
        sendError(client, common::Command::Login, common::Status::Unauthorized,
                  "Invalid username or password");
        return;
    }

    if (user->isBlocked() || user->status() == common::AccountStatus::Disabled) {
        delete user;
        sendError(client, common::Command::Login, common::Status::Forbidden,
                  "Your account has been blocked or disabled");
        return;
    }

    // Set authenticated state
    client->setUserId(user->id());
    client->setAuthenticated(true);

    QJsonObject responsePayload;
    responsePayload["userId"] = user->id();
    responsePayload["username"] = user->username();
    responsePayload["displayName"] = user->displayName();
    responsePayload["role"] = static_cast<int>(user->role());
    responsePayload["roleName"] = user->roleName();
    responsePayload["requiresGenreSetup"] = user->requiresFirstGenreSetup();
    responsePayload["status"] = static_cast<int>(user->status());

    if (auto* regular = dynamic_cast<common::RegularUser*>(user)) {
        responsePayload["favoriteGenres"] = QJsonArray::fromStringList(regular->favoriteGenreIds());
    }
    if (auto* publisher = dynamic_cast<common::Publisher*>(user)) {
        responsePayload["publisherName"] = publisher->publisherName();
    }

    sendSuccess(client, common::Command::Login, responsePayload);
    delete user;

    LOG_INFO("User logged in: " + username + " (ID: " + client->userId() + ")");
}

// --- Register ---
void AuthRequestHandler::handleRegister(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("Register request from client: " + client->clientId());

    QString missingField;
    if (!validateRequiredFields(payload, {"username", "password", "displayName", "email", "phone",
                                          "securityQuestion", "securityAnswer"}, missingField)) {
        sendError(client, common::Command::Register, common::Status::BadRequest,
                  "Missing field: " + missingField);
        return;
    }

    QString username = payload["username"].toString();
    if (!m_authService->isUsernameUnique(username)) {
        sendError(client, common::Command::Register, common::Status::Conflict,
                  "Username already exists");
        return;
    }

    // Create user based on role (default: User)
    common::AccountRole role = common::AccountRole::User;
    if (payload.contains("role")) {
        role = static_cast<common::AccountRole>(payload["role"].toInt());
    }

    common::UserAccount* user = createUserFromPayload(payload);
    if (!user) {
        sendError(client, common::Command::Register, common::Status::InternalError,
                  "Failed to create user account");
        return;
    }

    user->setId(common::IdGenerator::generateUuid());
    // Set role properly (the factory method createUserFromPayload should have done this,
    // but we ensure it matches the requested role)
    // Actually createUserFromPayload creates the right derived type, so we don't need to set role directly.

    QString securityAnswer = payload["securityAnswer"].toString();
    common::UserAccount* savedUser = m_authService->registerAccount(user, payload["password"].toString(), securityAnswer);

    if (!savedUser) {
        delete user;
        sendError(client, common::Command::Register, common::Status::InternalError,
                  "Registration failed. Please try again.");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["userId"] = savedUser->id();
    responsePayload["username"] = savedUser->username();
    responsePayload["displayName"] = savedUser->displayName();
    responsePayload["role"] = static_cast<int>(savedUser->role());
    responsePayload["roleName"] = savedUser->roleName();

    sendSuccess(client, common::Command::Register, responsePayload);
    delete savedUser;

    LOG_INFO("User registered: " + username);
}

// --- Reset Password ---
void AuthRequestHandler::handleResetPassword(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("Reset password request from client: " + client->clientId());

    QString missingField;
    if (!validateRequiredFields(payload, {"username", "securityAnswer", "newPassword"}, missingField)) {
        sendError(client, common::Command::ResetPassword, common::Status::BadRequest,
                  "Missing field: " + missingField);
        return;
    }

    QString username = payload["username"].toString();
    QString securityAnswer = payload["securityAnswer"].toString();
    QString newPassword = payload["newPassword"].toString();

    bool success = m_authService->resetPassword(username, securityAnswer, newPassword);
    if (!success) {
        sendError(client, common::Command::ResetPassword, common::Status::BadRequest,
                  "Invalid username or security answer");
        return;
    }

    sendSuccess(client, common::Command::ResetPassword, {});
    LOG_INFO("Password reset for user: " + username);
}

// --- Change Password ---
void AuthRequestHandler::handleChangePassword(const QJsonObject& payload, ClientConnection* client)
{
    LOG_DEBUG("Change password request from client: " + client->clientId());

    if (!client->isAuthenticated()) {
        sendError(client, common::Command::ChangePassword, common::Status::Unauthorized,
                  "Authentication required to change password");
        return;
    }

    QString missingField;
    if (!validateRequiredFields(payload, {"oldPassword", "newPassword"}, missingField)) {
        sendError(client, common::Command::ChangePassword, common::Status::BadRequest,
                  "Missing field: " + missingField);
        return;
    }

    QString oldPassword = payload["oldPassword"].toString();
    QString newPassword = payload["newPassword"].toString();

    bool success = m_authService->changePassword(client->userId(), oldPassword, newPassword);
    if (!success) {
        sendError(client, common::Command::ChangePassword, common::Status::BadRequest,
                  "Invalid old password or error updating password");
        return;
    }

    sendSuccess(client, common::Command::ChangePassword, {});
    LOG_INFO("Password changed for user: " + client->userId());
}

// --- Logout ---
void AuthRequestHandler::handleLogout(const QJsonObject& payload, ClientConnection* client)
{
    if (!client) return;

    QString userId = client->userId();
    m_authService->logout(userId);

    client->setAuthenticated(false);
    client->setUserId({});

    sendSuccess(client, common::Command::Logout, {});
    LOG_INFO("User logged out: " + userId);
}

// --- Factory for creating user from payload ---
common::UserAccount* AuthRequestHandler::createUserFromPayload(const QJsonObject& payload)
{
    common::AccountRole role = common::AccountRole::User;
    if (payload.contains("role")) {
        role = static_cast<common::AccountRole>(payload["role"].toInt());
    }

    common::UserAccount* user = nullptr;
    switch (role) {
        case common::AccountRole::Admin:
            user = new common::Admin;
            break;
        case common::AccountRole::Publisher:
            user = new common::Publisher;
            break;
        case common::AccountRole::User:
        default:
            user = new common::RegularUser;
            break;
    }

    // Fill common fields
    user->setUsername(payload["username"].toString());
    user->setDisplayName(payload["displayName"].toString());
    user->setEmail(payload["email"].toString());
    user->setPhone(payload["phone"].toString());
    user->setSecurityQuestion(payload["securityQuestion"].toString());
    user->setStatus(common::AccountStatus::Pending);

    // Publisher specific fields
    if (auto* publisher = dynamic_cast<common::Publisher*>(user)) {
        if (payload.contains("publisherName"))
            publisher->setPublisherName(payload["publisherName"].toString());
        if (payload.contains("biography"))
            publisher->setBiography(payload["biography"].toString());
        if (payload.contains("website"))
            publisher->setWebsite(payload["website"].toString());
        if (payload.contains("taxId"))
            publisher->setTaxId(payload["taxId"].toString());
    }

    // Regular user specific fields (favorite genres, etc.)
    if (auto* regular = dynamic_cast<common::RegularUser*>(user)) {
        if (payload.contains("favoriteGenres")) {
            QStringList genres;
            for (const auto& val : payload["favoriteGenres"].toArray()) {
                genres.append(val.toString());
            }
            regular->setFavoriteGenreIds(genres);
        }
    }

    return user;
}

} // namespace bookclub::server
