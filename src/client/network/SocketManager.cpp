// src/client/network/SocketManager.cpp
#include "src/client/network/SocketManager.h"
#include "common/Utils/Logger.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>

namespace bookclub::client {

SocketManager& SocketManager::instance() {
    static SocketManager instance;
    return instance;
}

SocketManager::SocketManager(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(3000);

    connect(m_socket, &QTcpSocket::connected, this, &SocketManager::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketManager::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketManager::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &SocketManager::onError);
    connect(m_reconnectTimer, &QTimer::timeout, this, &SocketManager::onReconnectTimeout);

    LOG_INFO("SocketManager initialized");
}

SocketManager::~SocketManager() {
    disconnectFromServer();
}

bool SocketManager::connectToServer(const QString& host, quint16 port) {
    if (m_connected) {
        LOG_WARNING("Already connected to server");
        return true;
    }

    m_host = host;
    m_port = port;

    LOG_INFO("Connecting to server: " + host + ":" + QString::number(port));
    m_socket->connectToHost(host, port);

    if (!m_socket->waitForConnected(5000)) {
        LOG_ERROR("Connection timeout: " + m_socket->errorString());
        emit errorOccurred("Connection timeout: " + m_socket->errorString());
        return false;
    }

    return true;
}

void SocketManager::disconnectFromServer() {
    if (!m_connected && !m_socket) return;

    LOG_INFO("Disconnecting from server");
    if (m_socket) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(3000);
        }
    }
    m_connected = false;
    m_reconnectTimer->stop();
}

bool SocketManager::isConnected() const {
    return m_connected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void SocketManager::sendMessage(const common::Message& message) {
    if (!isConnected()) {
        LOG_ERROR("Cannot send message: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QByteArray packet = common::PacketParser::pack(message);
    qint64 written = m_socket->write(packet);
    if (written != packet.size()) {
        LOG_ERROR("Failed to send complete message. Written: " + QString::number(written) +
                  ", Expected: " + QString::number(packet.size()));
        emit errorOccurred("Failed to send message");
        return;
    }
    m_socket->flush();

    LOG_DEBUG("Message sent: " + common::commandToString(message.command()));
}

void SocketManager::sendRequest(common::Command command, const QJsonObject& payload) {
    common::Message msg(command, payload);
    sendMessage(msg);
}

void SocketManager::registerRequestHandler(common::Command command,
                                           std::function<void(const common::Message&)> handler) {
    QMutexLocker locker(&m_mutex);
    m_requestHandlers[command] = handler;
    LOG_DEBUG("Registered handler for command: " + common::commandToString(command));
}

void SocketManager::unregisterRequestHandler(common::Command command) {
    QMutexLocker locker(&m_mutex);
    m_requestHandlers.remove(command);
    LOG_DEBUG("Unregistered handler for command: " + common::commandToString(command));
}

// --- Private Slots ---

void SocketManager::onConnected() {
    m_connected = true;
    m_reconnectTimer->stop();
    LOG_INFO("Connected to server: " + m_host + ":" + QString::number(m_port));
    emit connected();
}

void SocketManager::onDisconnected() {
    m_connected = false;
    LOG_INFO("Disconnected from server");
    emit disconnected();

    if (!m_host.isEmpty() && m_port > 0) {
        LOG_INFO("Scheduling reconnect in 3 seconds...");
        m_reconnectTimer->start();
    }
}

void SocketManager::onReadyRead() {
    if (!m_socket) return;

    QByteArray data = m_socket->readAll();
    m_packetParser.feed(data);
    processPendingPackets();
}

void SocketManager::processPendingPackets() {
    while (m_packetParser.hasNextPacket()) {
        common::Message message = m_packetParser.nextPacket();
        if (!message.isValid()) {
            LOG_WARNING("Received invalid packet");
            continue;
        }

        LOG_DEBUG("Received message: " + common::commandToString(message.command()));
        emit messageReceived(message);
        handleResponse(message);
    }
}

void SocketManager::handleResponse(const common::Message& response) {
    common::Command cmd = response.command();

    QMutexLocker locker(&m_mutex);
    auto it = m_requestHandlers.find(cmd);
    if (it != m_requestHandlers.end()) {
        it.value()(response);
        emit requestCompleted(cmd, response);
    } else {
        LOG_DEBUG("No handler registered for response: " + common::commandToString(cmd));
    }
}

void SocketManager::onError(QAbstractSocket::SocketError error) {
    QString errorMsg = m_socket->errorString();
    LOG_ERROR("Socket error: " + errorMsg + " (Code: " + QString::number(error) + ")");
    emit errorOccurred(errorMsg);

    if (m_connected) {
        m_connected = false;
        emit disconnected();
        if (!m_host.isEmpty() && m_port > 0) {
            LOG_INFO("Scheduling reconnect in 3 seconds...");
            m_reconnectTimer->start();
        }
    }
}

void SocketManager::onReconnectTimeout() {
    LOG_INFO("Attempting to reconnect to " + m_host + ":" + QString::number(m_port));
    m_socket->connectToHost(m_host, m_port);
    if (m_socket->waitForConnected(3000)) {
        LOG_INFO("Reconnected successfully");
        emit connected();
    } else {
        LOG_WARNING("Reconnect failed. Will retry...");
        m_reconnectTimer->start();
    }
}

} // namespace bookclub::client
