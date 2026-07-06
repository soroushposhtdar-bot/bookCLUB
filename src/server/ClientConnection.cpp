// src/server/ClientConnection.cpp
#include "src/server/ClientConnection.h"
#include "src/server/ConnectionManager.h"
#include "src/server/RequestRouter.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QDataStream>
#include <QDebug>

namespace bookclub::server {

ClientConnection::ClientConnection(qintptr socketDescriptor,
                                   ConnectionManager* manager,
                                   RequestRouter* router,
                                   QObject* parent)
    : QObject(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_clientId(common::IdGenerator::generateUuid())
    , m_manager(manager)
    , m_router(router)
{
    m_socket = new QTcpSocket(this);
    m_socket->setSocketDescriptor(socketDescriptor);

    // Connect signals
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientConnection::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &ClientConnection::onErrorOccurred);

    LOG_INFO("Client connected: " + QString::number(socketDescriptor) + " (ID: " + m_clientId + ")");
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
    m_userId = userId;
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
