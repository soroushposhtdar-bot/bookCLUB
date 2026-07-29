// src/server/handlers/AuthRequestHandler.cpp
#include "src/server/handlers/AuthRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/Logger.h"
#include "common/Utils/PasswordHasher.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/ValidationUtils.h"

#include <QJsonArray>
#include "common/Models/UserAccount.h"

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
        case common::Command::Login:              handleLogin(payload, client); break;
        case common::Command::Register:           handleRegister(payload, client); break;
        case common::Command::ResetPassword:      handleResetPassword(payload, client); break;
        case common::Command::ChangePassword:     handleChangePassword(payload, client); break;
        case common::Command::Logout:             handleLogout(payload, client); break;
        case common::Command::GetCurrentUser:     handleGetCurrentUser(payload, client); break;
        case common::Command::UpdateProfile:      handleUpdateProfile(payload, client); break;
        case common::Command::SaveFavoriteGenres: handleSaveFavoriteGenres(payload, client); break;
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

    const QString username = payload["username"].toString();
    const QString password = payload["password"].toString();
    const QString email    = payload["email"].toString();
    const QString phone    = payload["phone"].toString();

    // --- Input validation (C5) ---
    // Reject malformed input before hitting the DB. Return 422
    // ValidationError with a user-friendly message.
    if (!common::ValidationUtils::isValidUsername(username)) {
        sendError(client, common::Command::Register, common::Status::ValidationError,
                  "Username must be 3-20 alphanumeric characters (underscores allowed).");
        return;
    }
    if (!common::ValidationUtils::isValidPassword(password)) {
        sendError(client, common::Command::Register, common::Status::ValidationError,
                  "Password must be 6-64 characters.");
        return;
    }
    if (!common::ValidationUtils::isValidEmail(email)) {
        sendError(client, common::Command::Register, common::Status::ValidationError,
                  "Invalid email format.");
        return;
    }
    if (!common::ValidationUtils::isValidPhoneNumber(phone)) {
        sendError(client, common::Command::Register, common::Status::ValidationError,
                  "Invalid phone number format.");
        return;
    }

    if (!m_authService->isUsernameUnique(username)) {
        sendError(client, common::Command::Register, common::Status::Conflict,
                  "Username already exists");
        return;
    }

    // v22 (Issue 5): the role is now parsed inside createUserFromPayload,
    // which accepts BOTH the integer form (0/1/2) and the string form
    // ("user"/"publisher"/"admin"). The client (AuthService::registerUser)
    // sends the int form (`payload["role"] = 1`), so this works seamlessly
    // with the existing protocol.
    common::UserAccount* user = createUserFromPayload(payload);
    if (!user) {
        sendError(client, common::Command::Register, common::Status::InternalError,
                  "Failed to create user account");
        return;
    }

    user->setId(common::IdGenerator::generateUuid());
    // createUserFromPayload already instantiated the right derived type
    // (RegularUser / Publisher / Admin) based on payload["role"], so the
    // role is implicitly set. No explicit setRole() call is needed — the
    // derived type carries the role via its virtual roleName() override.

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
    // BUG FIX: include requiresGenreSetup + status in the registration
    // response so the client can route the user to the GenreSelection page
    // after registering. Newly-registered users have status=Pending, so
    // requiresFirstGenreSetup() returns true.
    responsePayload["requiresGenreSetup"] = savedUser->requiresFirstGenreSetup();
    responsePayload["status"] = static_cast<int>(savedUser->status());

    // BUG FIX: auto-authenticate the user after registration so they can
    // immediately call authenticated endpoints (SaveFavoriteGenres,
    // GetLibrary, etc.) without having to log in separately. This makes
    // the post-registration genre selection flow work: the client goes
    // Register → GenreSelection → SaveFavoriteGenres → Login, and the
    // SaveFavoriteGenres call succeeds because the session is authenticated.
    client->setUserId(savedUser->id());
    client->setAuthenticated(true);

    sendSuccess(client, common::Command::Register, responsePayload);
    delete savedUser;

    LOG_INFO("User registered and auto-authenticated: " + username);
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
    // v22 (Issue 5): role can be sent either as an int (0=User, 1=Publisher,
    // 2=Admin) or as a string ("user" / "publisher" / "admin"). The int
    // form is preferred by the protocol; the string form is the documented
    // Issue 5 contract ("read payload["role"].toString()"). We accept both.
    common::AccountRole role = common::AccountRole::User;
    if (payload.contains("role")) {
        const QJsonValue v = payload.value("role");
        if (v.isString()) {
            const QString s = v.toString().trimmed().toLower();
            if (s == QStringLiteral("publisher")) {
                role = common::AccountRole::Publisher;
            } else if (s == QStringLiteral("admin")) {
                role = common::AccountRole::Admin;
            } else {
                role = common::AccountRole::User;
            }
        } else {
            // Numeric form. Coerce via int.
            const int r = v.toInt(0);
            switch (r) {
                case 1:  role = common::AccountRole::Publisher; break;
                case 2:  role = common::AccountRole::Admin;     break;
                case 0:
                default: role = common::AccountRole::User;     break;
            }
        }
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

// --- Get current user (profile + favorites) ---
void AuthRequestHandler::handleGetCurrentUser(const QJsonObject& /*payload*/, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::GetCurrentUser, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    common::UserAccount* user = m_userRepo->findById(client->userId());
    if (!user) {
        sendError(client, common::Command::GetCurrentUser, common::Status::NotFound,
                  "User not found");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["id"]            = user->id();
    responsePayload["username"]      = user->username();
    responsePayload["displayName"]   = user->displayName();
    responsePayload["email"]         = user->email();
    responsePayload["phone"]         = user->phone();
    responsePayload["role"]          = static_cast<int>(user->role());
    responsePayload["roleName"]      = user->roleName();
    responsePayload["status"]        = static_cast<int>(user->status());

    if (auto* regular = dynamic_cast<common::RegularUser*>(user)) {
        responsePayload["favoriteGenres"] = QJsonArray::fromStringList(regular->favoriteGenreIds());
    }
    if (auto* publisher = dynamic_cast<common::Publisher*>(user)) {
        responsePayload["publisherName"] = publisher->publisherName();
        responsePayload["biography"]     = publisher->biography();
        responsePayload["website"]       = publisher->website();
        responsePayload["taxId"]         = publisher->taxId();
    }

    delete user;
    sendSuccess(client, common::Command::GetCurrentUser, responsePayload);
}

// --- Update profile (currently only displayName) ---
void AuthRequestHandler::handleUpdateProfile(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::UpdateProfile, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    common::UserAccount* user = m_userRepo->findById(client->userId());
    if (!user) {
        sendError(client, common::Command::UpdateProfile, common::Status::NotFound,
                  "User not found");
        return;
    }

    if (payload.contains("displayName")) {
        user->setDisplayName(payload["displayName"].toString());
    }
    if (payload.contains("email")) {
        user->setEmail(payload["email"].toString());
    }
    if (payload.contains("phone")) {
        user->setPhone(payload["phone"].toString());
    }

    if (!m_userRepo->update(user)) {
        delete user;
        sendError(client, common::Command::UpdateProfile, common::Status::InternalError,
                  "Failed to update profile");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["displayName"] = user->displayName();
    delete user;
    sendSuccess(client, common::Command::UpdateProfile, responsePayload);
    LOG_INFO("Profile updated for user: " + client->userId());
}

// --- Save favorite genres (regular users only) ---
void AuthRequestHandler::handleSaveFavoriteGenres(const QJsonObject& payload, ClientConnection* client)
{
    if (!isAuthenticated(client)) {
        sendError(client, common::Command::SaveFavoriteGenres, common::Status::Unauthorized,
                  "Authentication required");
        return;
    }

    common::UserAccount* user = m_userRepo->findById(client->userId());
    if (!user) {
        sendError(client, common::Command::SaveFavoriteGenres, common::Status::NotFound,
                  "User not found");
        return;
    }

    auto* regular = dynamic_cast<common::RegularUser*>(user);
    if (!regular) {
        delete user;
        sendError(client, common::Command::SaveFavoriteGenres, common::Status::BadRequest,
                  "Only regular users can save favorite genres");
        return;
    }

    QStringList genres;
    for (const auto& v : payload["genres"].toArray()) {
        genres.append(v.toString());
    }
    // Cap at 3 (per spec).
    while (genres.size() > 3) genres.removeLast();
    regular->setFavoriteGenreIds(genres);

    // BUG FIX: promote the user from Pending → Active now that they've
    // completed the genre-selection onboarding step. Previously the user
    // stayed Pending forever, so `requiresFirstGenreSetup()` kept returning
    // true on every subsequent login → the GenreSelection page kept
    // appearing even after the user had already picked their genres.
    if (user->status() == common::AccountStatus::Pending) {
        user->setStatus(common::AccountStatus::Active);
    }

    if (!m_userRepo->update(regular)) {
        delete user;
        sendError(client, common::Command::SaveFavoriteGenres, common::Status::InternalError,
                  "Failed to save favorite genres");
        return;
    }

    QJsonObject responsePayload;
    responsePayload["favoriteGenres"] = QJsonArray::fromStringList(genres);
    responsePayload["status"] = static_cast<int>(user->status());
    delete user;
    sendSuccess(client, common::Command::SaveFavoriteGenres, responsePayload);
    LOG_INFO("Favorite genres saved for user: " + client->userId());
}

} // namespace bookclub::server
