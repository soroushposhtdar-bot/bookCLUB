// src/server/RequestHandlerBase.cpp
#include "src/server/RequestHandlerBase.h"
#include "src/server/ClientConnection.h"
#include "src/server/DatabaseManager.h"
#include "common/Utils/Logger.h"

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

    // TODO: Load user from database by userId
    // For now, return nullptr (will be implemented in AuthHandler)
    return nullptr;
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
