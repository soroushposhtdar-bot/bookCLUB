// src/client/network/SocketManager.h
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <functional>

#include "common/Network/PacketParser.h"
#include "common/Network/Message.h"

namespace bookclub::client {

class SocketManager : public QObject {
    Q_OBJECT
public:
    static SocketManager& instance();

    // --- Connection Management ---
    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // --- Send Message ---
    void sendMessage(const common::Message& message);
    void sendRequest(common::Command command, const QJsonObject& payload = {});

    // --- Request Tracking ---
    void registerRequestHandler(common::Command command, std::function<void(const common::Message&)> handler);
    void unregisterRequestHandler(common::Command command);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void messageReceived(const common::Message& message);
    void requestCompleted(common::Command command, const common::Message& response);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onReconnectTimeout();

private:
    SocketManager(QObject* parent = nullptr);
    ~SocketManager() override;
    SocketManager(const SocketManager&) = delete;
    SocketManager& operator=(const SocketManager&) = delete;

    void processPendingPackets();
    void handleResponse(const common::Message& response);

    QTcpSocket* m_socket = nullptr;
    common::PacketParser m_packetParser;
    QTimer* m_reconnectTimer = nullptr;

    QMap<common::Command, std::function<void(const common::Message&)>> m_requestHandlers;
    QMutex m_mutex;
    QString m_host;
    quint16 m_port = 0;
    bool m_connected = false;
};

} // namespace bookclub::client
