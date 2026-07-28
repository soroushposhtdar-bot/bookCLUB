// src/server/ConnectionManager.h
#pragma once

#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QList>
#include <QPointer>
#include <QMutex>

#include "common/Network/Message.h"

namespace bookclub::server {

class ClientConnection;
class RequestRouter;

class ConnectionManager : public QObject {
    Q_OBJECT
public:
    explicit ConnectionManager(RequestRouter* router, QObject* parent = nullptr);
    ~ConnectionManager() override;

    // --- Server Control ---
    bool startServer(quint16 port);
    void stopServer();
    bool isRunning() const;

    // --- Connection Management ---
    ClientConnection* getConnection(const QString& clientId) const;
    ClientConnection* getConnectionByUserId(const QString& userId) const;
    QList<ClientConnection*> getAllConnections() const;
    QList<ClientConnection*> getAuthenticatedConnections() const;
    int connectionCount() const;
    int authenticatedCount() const;

    // --- Message Broadcasting ---
    void broadcastMessage(const common::Message& message);
    void sendToUser(const QString& userId, const common::Message& message);
    void sendToAllAuthenticated(const common::Message& message);

    // --- Remove Connection ---
    void removeConnection(const QString& clientId);

    // --- User Mapping ---
    void registerUser(const QString& userId, const QString& clientId);
    void unregisterUser(const QString& userId);

signals:
    void clientConnected(const QString& clientId);
    void clientDisconnected(const QString& clientId);
    void newMessageReceived(const common::Message& message);

private slots:
    void onNewConnection();
    void onClientDisconnected(const QString& clientId);

private:
    QTcpServer* m_server;
    QMap<QString, ClientConnection*> m_connections;
    QMap<QString, QString> m_userToClientMap; // userId -> clientId
    QPointer<RequestRouter> m_router;
    mutable QMutex m_mutex;
    bool m_running = false;
};

} // namespace bookclub::server
