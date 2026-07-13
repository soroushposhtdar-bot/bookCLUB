// src/client/controllers/AuthController.cpp
#include "src/client/controllers/AuthController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "src/client/session/SessionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/PasswordHasher.h"

#include <QJsonObject>
#include <QJsonDocument>

namespace bookclub::client {

AuthController::AuthController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    network.registerRequestHandler(common::Command::Login, [this](const common::Message& response) {
        handleLoginResponse(response);
    });

    network.registerRequestHandler(common::Command::Register, [this](const common::Message& response) {
        handleRegisterResponse(response);
    });

    network.registerRequestHandler(common::Command::ResetPassword, [this](const common::Message& response) {
        handleResetPasswordResponse(response);
    });

    network.registerRequestHandler(common::Command::Logout, [this](const common::Message& response) {
        handleLogoutResponse(response);
    });

    LOG_INFO("AuthController initialized");
}

AuthController::~AuthController() {
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::Login);
    network.unregisterRequestHandler(common::Command::Register);
    network.unregisterRequestHandler(common::Command::ResetPassword);
    network.unregisterRequestHandler(common::Command::Logout);
}

// ---- Setter Methods ----

void AuthController::setLoginCredentials(const QString& username, const QString& password) {
    m_username = username;
    m_password = password;
}

void AuthController::setRegistrationData(const QString& username,
                                         const QString& password,
                                         const QString& displayName,
                                         const QString& email,
                                         const QString& phone,
                                         const QString& securityQuestion,
                                         const QString& securityAnswer) {
    m_username = username;
    m_password = password;
    m_displayName = displayName;
    m_email = email;
    m_phone = phone;
    m_securityQuestion = securityQuestion;
    m_securityAnswer = securityAnswer;
}

void AuthController::setResetPasswordData(const QString& username,
                                          const QString& securityAnswer,
                                          const QString& newPassword) {
    m_username = username;
    m_securityAnswer = securityAnswer;
    m_newPassword = newPassword;
}

// ---- Public Methods ----

void AuthController::login() {
    LOG_DEBUG("AuthController::login() called for user: " + m_username);

    if (m_username.isEmpty() || m_password.isEmpty()) {
        LOG_WARNING("Login failed: username or password is empty");
        emit loginFailed("Username and password are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Login failed: not connected to server");
        emit loginFailed("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["username"] = m_username;
    payload["password"] = m_password;

    emit busyChanged(true);
    ClientNetworkManager::instance().sendRequest(common::Command::Login, payload);
}

void AuthController::registerUser() {
    LOG_DEBUG("AuthController::registerUser() called for: " + m_username);

    if (m_username.isEmpty() || m_password.isEmpty() || m_displayName.isEmpty() ||
        m_email.isEmpty() || m_phone.isEmpty() || m_securityQuestion.isEmpty() ||
        m_securityAnswer.isEmpty()) {
        LOG_WARNING("Registration failed: missing required fields");
        emit registrationFailed("All fields are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Registration failed: not connected to server");
        emit registrationFailed("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["username"] = m_username;
    payload["password"] = m_password;
    payload["displayName"] = m_displayName;
    payload["email"] = m_email;
    payload["phone"] = m_phone;
    payload["securityQuestion"] = m_securityQuestion;
    payload["securityAnswer"] = m_securityAnswer;
    payload["role"] = static_cast<int>(common::AccountRole::User);

    emit busyChanged(true);
    ClientNetworkManager::instance().sendRequest(common::Command::Register, payload);
}

void AuthController::resetPassword() {
    LOG_DEBUG("AuthController::resetPassword() called for: " + m_username);

    if (m_username.isEmpty() || m_securityAnswer.isEmpty() || m_newPassword.isEmpty()) {
        LOG_WARNING("Reset password failed: missing required fields");
        emit passwordResetFailed("Username, security answer and new password are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Reset password failed: not connected to server");
        emit passwordResetFailed("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["username"] = m_username;
    payload["securityAnswer"] = m_securityAnswer;
    payload["newPassword"] = m_newPassword;

    emit busyChanged(true);
    ClientNetworkManager::instance().sendRequest(common::Command::ResetPassword, payload);
}

void AuthController::logout() {
    LOG_DEBUG("AuthController::logout() called for user: " + m_username);

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Logout failed: not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::Logout, {});
    emit busyChanged(true);
}

// ---- Response Handlers ----

void AuthController::handleLoginResponse(const common::Message& response) {
    emit busyChanged(false);

    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Login failed");
        LOG_WARNING("Login failed: " + error);
        emit loginFailed(error);
        return;
    }

    QJsonObject data = response.payload();
    QString userId = data["userId"].toString();
    QString username = data["username"].toString();
    QString displayName = data["displayName"].toString();
    int roleInt = data["role"].toInt();
    common::AccountRole role = static_cast<common::AccountRole>(roleInt);
    QString email = data["email"].toString();
    bool requiresGenreSetup = data["requiresGenreSetup"].toBool();

    // Store user info in SessionManager
    auto& session = SessionManager::instance();
    session.startSession(userId, username, displayName, email, role);

    LOG_INFO("User logged in successfully: " + username + " (ID: " + userId +
             ", Role: " + session.roleName() + ")");

    emit loginSucceeded();
}

void AuthController::handleRegisterResponse(const common::Message& response) {
    emit busyChanged(false);

    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Registration failed");
        LOG_WARNING("Registration failed: " + error);
        emit registrationFailed(error);
        return;
    }

    QJsonObject data = response.payload();
    QString userId = data["userId"].toString();
    QString username = data["username"].toString();

    LOG_INFO("User registered successfully: " + username + " (ID: " + userId + ")");
    emit registrationSucceeded();
}

void AuthController::handleResetPasswordResponse(const common::Message& response) {
    emit busyChanged(false);

    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Password reset failed");
        LOG_WARNING("Password reset failed: " + error);
        emit passwordResetFailed(error);
        return;
    }

    LOG_INFO("Password reset successfully for user: " + m_username);
    emit passwordResetSucceeded();
}

void AuthController::handleLogoutResponse(const common::Message& response) {
    emit busyChanged(false);

    if (!response.isSuccess()) {
        LOG_WARNING("Logout failed: " + response.payload().value("error").toString(""));
        return;
    }

    // Clear session
    auto& session = SessionManager::instance();
    session.endSession();

    LOG_INFO("User logged out successfully");
    m_username.clear();
    m_password.clear();
}

} // namespace bookclub::client
