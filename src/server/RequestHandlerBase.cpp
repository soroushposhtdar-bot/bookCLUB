// src/server/RequestHandlerBase.cpp
#include "src/server/RequestHandlerBase.h"
#include "src/server/ClientConnection.h"
#include "src/server/DatabaseManager.h"
#include "common/Interfaces/IUserRepository.h"
#include "common/Utils/Logger.h"
#include "common/Models/UserAccount.h"

namespace bookclub::server {

RequestHandlerBase::RequestHandlerBase(QObject* parent)
    : QObject(parent)
    , m_db(DatabaseManager::instance())
{
}

void RequestHandlerBase::sendSuccess(ClientConnection* client,
                                     common::Command command,
                                     const QJsonObject& payload)
{
    common::Message response(command, common::Status::Success, payload);
    // Echo the original requestId so the client can route the response.
    if (!m_currentRequestId.isEmpty()) {
        response.setRequestId(m_currentRequestId);
    }
    sendResponse(client, response);
}

void RequestHandlerBase::sendError(ClientConnection* client,
                                   common::Command command,
                                   common::Status status,
                                   const QString& errorMessage)
{
    QJsonObject payload;
    if (!errorMessage.isEmpty()) {
        payload["error"] = errorMessage;
    }
    common::Message response(command, status, payload);
    // Echo the original requestId so the client can route the response.
    if (!m_currentRequestId.isEmpty()) {
        response.setRequestId(m_currentRequestId);
    }
    sendResponse(client, response);
}

void RequestHandlerBase::sendResponse(ClientConnection* client,
                                      const common::Message& response)
{
    if (!client) {
        LOG_ERROR("Cannot send response to null client");
        return;
    }
    client->sendMessage(response);
}

bool RequestHandlerBase::isAuthenticated(ClientConnection* client) const
{
    return client && client->isAuthenticated();
}

common::UserAccount* RequestHandlerBase::getAuthenticatedUser(ClientConnection* client) const
{
    if (!isAuthenticated(client)) return nullptr;
    // Look up the user from the database via the shared repository.
    // The caller owns the returned pointer and must delete it.
    auto* userRepo = common::createUserRepository();
    return userRepo->findById(client->userId());
}

bool RequestHandlerBase::requireRole(ClientConnection* client,
                                     common::AccountRole role,
                                     common::Command cmd)
{
    if (!isAuthenticated(client)) {
        sendError(client, cmd, common::Status::Unauthorized,
                  "Authentication required");
        return false;
    }
    auto* user = getAuthenticatedUser(client);
    if (!user) {
        sendError(client, cmd, common::Status::Unauthorized,
                  "User account not found");
        return false;
    }
    const bool ok = (user->role() == role);
    delete user;
    if (!ok) {
        sendError(client, cmd, common::Status::Forbidden,
                  "Insufficient permissions for this action");
        return false;
    }
    return true;
}

bool RequestHandlerBase::requireAnyRole(ClientConnection* client,
                                        const QVector<common::AccountRole>& roles,
                                        common::Command cmd)
{
    if (!isAuthenticated(client)) {
        sendError(client, cmd, common::Status::Unauthorized,
                  "Authentication required");
        return false;
    }
    auto* user = getAuthenticatedUser(client);
    if (!user) {
        sendError(client, cmd, common::Status::Unauthorized,
                  "User account not found");
        return false;
    }
    const auto actual = user->role();
    delete user;
    for (auto r : roles) {
        if (actual == r) return true;
    }
    sendError(client, cmd, common::Status::Forbidden,
              "Insufficient permissions for this action");
    return false;
}

bool RequestHandlerBase::validateRequiredFields(const QJsonObject& payload,
                                                const QStringList& requiredFields,
                                                QString& missingField) const
{
    for (const QString& field : requiredFields) {
        if (!payload.contains(field) || payload[field].isNull() || payload[field].isUndefined()) {
            missingField = field;
            return false;
        }
    }
    return true;
}

DatabaseManager& RequestHandlerBase::db() const
{
    return m_db;
}

} // namespace bookclub::server
