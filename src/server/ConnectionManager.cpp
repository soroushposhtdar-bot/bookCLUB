// src/server/ConnectionManager.cpp
#include "src/server/ConnectionManager.h"
#include "src/server/ClientConnection.h"
#include "src/server/RequestRouter.h"
#include "common/Utils/Logger.h"

#include <QTcpSocket>
#include <QDebug>

namespace bookclub::server {

ConnectionManager::ConnectionManager(RequestRouter* router, QObject* parent)
    : QObject(parent)
    , m_router(router)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &ConnectionManager::onNewConnection);
}

ConnectionManager::~ConnectionManager() {
    stopServer();
    qDeleteAll(m_connections);
    m_connections.clear();
}

bool ConnectionManager::startServer(quint16 port) {
    if (m_running) {
        LOG_WARNING("Server is already running");
        return true;
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        LOG_ERROR("Failed to start server on port " + QString::number(port) + ": " + m_server->errorString());
        return false;
    }

    m_running = true;
    LOG_INFO("Server started on port " + QString::number(port));
    return true;
}

void ConnectionManager::stopServer() {
    if (!m_running) return;

    m_server->close();
    m_running = false;

    // Disconnect all clients
    for (ClientConnection* conn : m_connections.values()) {
        conn->disconnect();
    }

    LOG_INFO("Server stopped");
}

bool ConnectionManager::isRunning() const {
    return m_running;
}

void ConnectionManager::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) continue;

        qintptr descriptor = socket->socketDescriptor();

        // Create new client connection
        auto* connection = new ClientConnection(descriptor, this, m_router, this);
        m_connections[connection->clientId()] = connection;

        connect(connection, &ClientConnection::disconnected,
                this, &ConnectionManager::onClientDisconnected);

        LOG_INFO("New client connected: " + connection->clientId());
        emit clientConnected(connection->clientId());
    }
}

void ConnectionManager::onClientDisconnected(const QString& clientId) {
    removeConnection(clientId);
}

void ConnectionManager::removeConnection(const QString& clientId) {
    QMutexLocker locker(&m_mutex);

    ClientConnection* conn = m_connections.value(clientId, nullptr);
    if (!conn) return;

    // Remove from user mapping
    if (!conn->userId().isEmpty()) {
        m_userToClientMap.remove(conn->userId());
    }

    m_connections.remove(clientId);
    conn->deleteLater();

    LOG_INFO("Connection removed: " + clientId);
    emit clientDisconnected(clientId);
}

ClientConnection* ConnectionManager::getConnection(const QString& clientId) const {
    QMutexLocker locker(&m_mutex);
    return m_connections.value(clientId, nullptr);
}

ClientConnection* ConnectionManager::getConnectionByUserId(const QString& userId) const {
    QMutexLocker locker(&m_mutex);
    QString clientId = m_userToClientMap.value(userId, QString());
    if (clientId.isEmpty()) return nullptr;
    return m_connections.value(clientId, nullptr);
}

QList<ClientConnection*> ConnectionManager::getAllConnections() const {
    QMutexLocker locker(&m_mutex);
    return m_connections.values();
}

QList<ClientConnection*> ConnectionManager::getAuthenticatedConnections() const {
    QMutexLocker locker(&m_mutex);
    QList<ClientConnection*> result;
    for (ClientConnection* conn : m_connections) {
        if (conn->isAuthenticated()) {
            result.append(conn);
        }
    }
    return result;
}

int ConnectionManager::connectionCount() const {
    QMutexLocker locker(&m_mutex);
    return m_connections.size();
}

int ConnectionManager::authenticatedCount() const {
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (ClientConnection* conn : m_connections) {
        if (conn->isAuthenticated()) count++;
    }
    return count;
}

void ConnectionManager::broadcastMessage(const common::Message& message) {
    QByteArray packet = common::PacketParser::pack(message);
    for (ClientConnection* conn : m_connections) {
        conn->sendPacket(packet);
    }
}

void ConnectionManager::sendToUser(const QString& userId, const common::Message& message) {
    ClientConnection* conn = getConnectionByUserId(userId);
    if (conn) {
        conn->sendMessage(message);
    }
}

void ConnectionManager::sendToAllAuthenticated(const common::Message& message) {
    QByteArray packet = common::PacketParser::pack(message);
    for (ClientConnection* conn : m_connections) {
        if (conn->isAuthenticated()) {
            conn->sendPacket(packet);
        }
    }
}

} // namespace bookclub::server
