// common/Services/NetworkService.h (client/include/services/NetworkService.h)
//
// Thin wrapper around ClientNetworkManager that exposes both a synchronous
// sendRequest() (kept for legacy code paths) and an async sendAsync() that
// delivers the response via a signal/slot callback.
//
// The synchronous sendRequest() now uses requestId-based routing so
// concurrent same-Command requests no longer race. It still uses a
// QEventLoop to block the calling thread, so prefer sendAsync() for new
// code paths that run on the QML/GUI thread.
#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QEventLoop>
#include <QTimer>
#include <QPointer>
#include <functional>

#include "src/client/network/ClientNetworkManager.h"
#include "common/Network/Message.h"
#include "common/Network/Protocol.h"

namespace bookclub::client {

class NetworkService : public QObject {
    Q_OBJECT
public:
    struct Response {
        bool ok = false;
        common::Status status = common::Status::Success;
        QJsonObject payload;
        QString errorMessage;

        bool isSuccess() const { return ok && status == common::Status::Success; }
    };

    using AsyncCallback = std::function<void(const Response&)>;

    static NetworkService& instance();

    bool connectToServer(const QString& host = QStringLiteral("127.0.0.1"),
                         quint16 port = 8080);
    void disconnectFromServer();
    bool isConnected() const;

    // Synchronous request. Blocks the calling thread for up to timeoutMs.
    // Uses requestId-based routing so multiple concurrent same-Command
    // requests are safe. Prefer sendAsync() on the GUI thread.
    Response sendRequest(common::Command command,
                         const QJsonObject& payload = {},
                         int timeoutMs = 5000);

    // Fire-and-forget. No callback, no waiting. Useful for telemetry / logs.
    void sendAsync(common::Command command, const QJsonObject& payload = {});

    // Asynchronous request. Returns immediately; the callback is invoked
    // from the network thread when the response arrives. The callback
    // MUST marshal back to the GUI thread if it touches QML.
    // The callback pointer is tracked via QPointer, so it's safe to call
    // from a destroyed-object context (it'll just be a no-op).
    void sendRequestAsync(common::Command command,
                          const QJsonObject& payload,
                          QObject* context,
                          AsyncCallback callback,
                          int timeoutMs = 5000);

    // Subscribe to a server-pushed event. The handler is invoked every
    // time an Evt* message of the given command arrives. Unsubscribe by
    // destroying `context` or by calling unsubscribeEvent().
    void subscribeEvent(common::Command eventCommand,
                        QObject* context,
                        std::function<void(const common::Message&)> handler);
    void unsubscribeEvent(common::Command eventCommand);

private:
    NetworkService(QObject* parent = nullptr);
    ~NetworkService() override = default;
    NetworkService(const NetworkService&) = delete;
    NetworkService& operator=(const NetworkService&) = delete;

    ClientNetworkManager& m_network;
};

} // namespace bookclub::client
