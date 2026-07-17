// src/server/ClientConnection.cpp
#include "src/server/ClientConnection.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QDataStream>
#include <QDebug>

namespace bookclub::server {

ClientConnection::ClientConnection(QTcpSocket* socket,
                                   ConnectionManager* manager,
                                   RequestRouter* router,
                                   QObject* parent)
    : QObject(parent)
    , m_socketDescriptor(socket ? socket->socketDescriptor() : -1)
    , m_clientId(common::IdGenerator::generateUuid())
    , m_manager(manager)
    , m_router(router)
{
    // Take ownership of the socket the server already created. This avoids
    // having two QTcpSocket objects pointing at the same descriptor (which
    // causes 'QSocketNotifier: Invalid socket' warnings and breaks reads).
    m_socket = socket;
    if (m_socket) {
        m_socket->setParent(this);
    }

    // Connect signals
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientConnection::onDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred,
            this, &ClientConnection::onErrorOccurred);

    LOG_INFO("Client connected: fd=" + QString::number(m_socketDescriptor) + " (ID: " + m_clientId + ")");
}

ClientConnection::~ClientConnection() {
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }
    LOG_INFO("Client disconnected: " + m_clientId);
}

qintptr ClientConnection::socketDescriptor() const {
    return m_socketDescriptor;
}

QString ClientConnection::clientId() const {
    return m_clientId;
}

QString ClientConnection::userId() const {
    return m_userId;
}

bool ClientConnection::isAuthenticated() const {
    return m_authenticated;
}

void ClientConnection::setUserId(const QString& userId) {
    if (m_userId == userId) return;
    // Keep ConnectionManager's userId->clientId mapping in sync so that
    // NotificationDispatcher and StudySessionManager can find us by userId.
    if (m_manager) {
        if (!m_userId.isEmpty()) {
            m_manager->unregisterUser(m_userId);
        }
        m_userId = userId;
        if (!m_userId.isEmpty()) {
            m_manager->registerUser(m_userId, m_clientId);
        }
    } else {
        m_userId = userId;
    }
}

void ClientConnection::setAuthenticated(bool authenticated) {
    m_authenticated = authenticated;
}

bool ClientConnection::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ClientConnection::sendMessage(const common::Message& message) {
    if (!isConnected()) {
        LOG_WARNING("Cannot send message to disconnected client: " + m_clientId);
        return;
    }

    QByteArray packet = common::PacketParser::pack(message);
    sendPacket(packet);
}

void ClientConnection::sendPacket(const QByteArray& packet) {
    if (!m_socket || !m_socket->isOpen()) return;

    qint64 bytesWritten = m_socket->write(packet);
    if (bytesWritten != packet.size()) {
        LOG_WARNING("Failed to send complete packet to: " + m_clientId);
    }
    m_socket->flush();
}

void ClientConnection::onReadyRead() {
    if (!m_socket) return;

    QByteArray data = m_socket->readAll();
    m_packetParser.feed(data);
    processPendingPackets();
}

void ClientConnection::processPendingPackets() {
    while (m_packetParser.hasNextPacket()) {
        common::Message message = m_packetParser.nextPacket();
        if (message.isValid()) {
            emit messageReceived(message);

            // Forward to router for processing
            if (m_router) {
                m_router->handleRequest(message, this);
            }
        } else {
            LOG_WARNING("Invalid packet received from: " + m_clientId);
        }
    }
}

void ClientConnection::onDisconnected() {
    LOG_INFO("Client disconnected: " + m_clientId);
    emit disconnected(m_clientId);
    if (m_manager) {
        m_manager->removeConnection(m_clientId);
    }
}

void ClientConnection::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    LOG_ERROR("Socket error for client " + m_clientId + ": " + m_socket->errorString());
    if (m_socket) {
        m_socket->close();
    }
    emit disconnected(m_clientId);
    if (m_manager) {
        m_manager->removeConnection(m_clientId);
    }
}

} // namespace bookclub::server
