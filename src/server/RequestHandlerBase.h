// src/server/RequestHandlerBase.h
#pragma once

#include <QObject>
#include <QVector>
#include <functional>

#include "common/Network/Message.h"
#include "common/Network/Protocol.h"
#include "common/Models/UserAccount.h"

namespace bookclub::server {

class ClientConnection;
class DatabaseManager;

class RequestHandlerBase : public QObject {
    Q_OBJECT
public:
    explicit RequestHandlerBase(QObject* parent = nullptr);
    ~RequestHandlerBase() override = default;

    // --- Main Handler Method ---
    virtual void handle(const common::Message& request, ClientConnection* client) = 0;

    // --- Request correlation ---
    // Called by RequestRouter before handle() so that sendSuccess / sendError
    // can echo the original requestId back to the client.
    void setCurrentRequestId(const QString& id) { m_currentRequestId = id; }

protected:
    // --- Helper Methods for Sending Responses ---
    void sendSuccess(ClientConnection* client,
                     common::Command command,
                     const QJsonObject& payload = {});

    void sendError(ClientConnection* client,
                   common::Command command,
                   common::Status status,
                   const QString& errorMessage = {});

    void sendResponse(ClientConnection* client,
                      const common::Message& response);

    // --- Authentication Helpers ---
    bool isAuthenticated(ClientConnection* client) const;
    common::UserAccount* getAuthenticatedUser(ClientConnection* client) const;

    // --- Permission helpers ---
    // Returns true iff the client is authenticated AND has the given role.
    // Sends a 403 Forbidden error and returns false otherwise.
    bool requireRole(ClientConnection* client, common::AccountRole role,
                     common::Command cmd);
    // Returns true iff the client is authenticated AND has any of the given roles.
    bool requireAnyRole(ClientConnection* client,
                        const QVector<common::AccountRole>& roles,
                        common::Command cmd);

    // --- Validation Helpers ---
    bool validateRequiredFields(const QJsonObject& payload,
                                const QStringList& requiredFields,
                                QString& missingField) const;

    // --- Access to Database ---
    DatabaseManager& db() const;

signals:
    void requestProcessed(const QString& clientId, common::Command command, bool success);

private:
    DatabaseManager& m_db;
    QString m_currentRequestId;
};

} // namespace bookclub::server
