// src/client/network/ClientNetworkManager.h
#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QMap>
#include <QHash>
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

    using MessageHandler = std::function<void(const common::Message&)>;

    // --- Connection Management ---
    bool connectToServer(const QString& host, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    // --- Send Message ---
    void sendMessage(const common::Message& message);
    void sendRequest(common::Command command, const QJsonObject& payload = {});

    // --- Request Tracking ---
    // Per-Command handler — kept for backwards compatibility (used by the
    // legacy AuthController and the real-time Evt* handlers). For concurrent
    // same-Command requests, prefer registerResponseHandler(requestId, ...).
    void registerRequestHandler(common::Command command, MessageHandler handler);
    void unregisterRequestHandler(common::Command command);

    // Per-requestId handler — used by NetworkService::sendRequest to support
    // concurrent same-Command requests without races. The handler is removed
    // automatically after the first matching response.
    void registerResponseHandler(const QString& requestId, MessageHandler handler);

    // BUG FIX: unregister a per-requestId handler. Used by NetworkService
    // when a request times out so that a late-arriving server response
    // doesn't fire a handler whose captured references are now dangling
    // (use-after-free). Previously the handler stayed in the map and
    // would crash the client if the server replied after the timeout.
    void unregisterResponseHandler(const QString& requestId);

    // Server-pushed event subscription — used by services that want to
    // receive Evt* messages in real time. The handler stays registered
    // across multiple events.
    void registerEventHandler(common::Command eventCommand, MessageHandler handler);
    void unregisterEventHandler(common::Command eventCommand);

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
    void routeResponse(const common::Message& response);

    QTcpSocket* m_socket = nullptr;
    common::PacketParser m_packetParser;
    QTimer* m_reconnectTimer = nullptr;

    // Per-Command handlers (legacy + Evt* subscription).
    QHash<common::Command, MessageHandler> m_requestHandlers;
    // Per-Command event handlers (long-lived). Used by services that want
    // to subscribe to server-pushed Evt* messages.
    QHash<common::Command, MessageHandler> m_eventHandlers;
    // Per-requestId handlers (one-shot). Used by NetworkService::sendRequest.
    QHash<QString, MessageHandler> m_responseHandlers;

    QMutex m_mutex;
    QString m_host;
    quint16 m_port = 0;
    bool m_connected = false;
};

} // namespace bookclub::client
