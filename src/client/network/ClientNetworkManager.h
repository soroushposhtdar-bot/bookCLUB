// src/client/network/ClientNetworkManager.h
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include <functional>

#include "common/Network/PacketParser.h"
#include "common/Network/Message.h"

namespace bookclub::client {

class ClientNetworkManager : public QObject {
    Q_OBJECT
public:
    static ClientNetworkManager& instance();

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
    ClientNetworkManager(QObject* parent = nullptr);
    ~ClientNetworkManager() override;
    ClientNetworkManager(const ClientNetworkManager&) = delete;
    ClientNetworkManager& operator=(const ClientNetworkManager&) = delete;

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
