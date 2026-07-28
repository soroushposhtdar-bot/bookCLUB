// client/src/services/NetworkService.cpp
//
// Concrete NetworkService implementation. The synchronous sendRequest()
// uses requestId-based routing so concurrent same-Command requests no
// longer race (was: one per-Command handler that got overwritten).
#include "services/NetworkService.h"
#include "common/Utils/Logger.h"
#include "common/Network/Protocol.h"

#include <QMutexLocker>
#include <QPointer>
#include <QTimer>
#include <memory>

namespace bookclub::client {

NetworkService& NetworkService::instance() {
    static NetworkService instance;
    return instance;
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent)
    , m_network(ClientNetworkManager::instance())
{
    LOG_INFO("NetworkService initialized");
}

bool NetworkService::connectToServer(const QString& host, quint16 port) {
    if (m_network.isConnected()) return true;
    return m_network.connectToServer(host, port);
}

void NetworkService::disconnectFromServer() {
    m_network.disconnectFromServer();
}

bool NetworkService::isConnected() const {
    return m_network.isConnected();
}

NetworkService::Response NetworkService::sendRequest(common::Command command,
                                                      const QJsonObject& payload,
                                                      int timeoutMs) {
    Response resp;
    if (!m_network.isConnected()) {
        if (!m_network.connectToServer(QStringLiteral("127.0.0.1"), 8080)) {
            resp.errorMessage = QStringLiteral("Not connected to server");
            return resp;
        }
    }

    // Build the request Message up-front so we know its requestId before
    // we register the handler.
    common::Message msg(command, payload);
    const QString requestId = msg.requestId();

    QEventLoop loop;
    bool gotResponse = false;

    m_network.registerResponseHandler(requestId,
        [&resp, &gotResponse, &loop](const common::Message& reply) {
            resp.ok = true;
            resp.status = reply.status();
            resp.payload = reply.payload();
            // Server puts the error text inside payload["error"].
            resp.errorMessage = reply.payload().value("error").toString();
            gotResponse = true;
            loop.quit();
        });

    // Send the request (use sendMessage so the requestId matches what
    // we registered).
    m_network.sendMessage(msg);

    // Timeout safety.
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();

    // BUG FIX (Issue 9 — use-after-free): if we timed out, the response
    // handler is still registered in the ClientNetworkManager map. A
    // late-arriving server response would fire the lambda above, which
    // captures `&resp`, `&gotResponse`, `&loop` by reference — all of
    // which are now dangling stack variables → use-after-free / crash.
    // We now explicitly unregister the handler if the response hasn't
    // arrived yet. (If it has arrived, routeResponse already erased it.)
    if (!gotResponse) {
        m_network.unregisterResponseHandler(requestId);
        resp.errorMessage = QStringLiteral("Request timed out");
        LOG_WARNING("NetworkService: request timed out for command " +
                    common::commandToString(command));
    }
    return resp;
}

void NetworkService::sendAsync(common::Command command, const QJsonObject& payload) {
    if (!m_network.isConnected()) {
        LOG_WARNING("NetworkService: cannot send async request, not connected");
        return;
    }
    m_network.sendRequest(command, payload);
}

void NetworkService::sendRequestAsync(common::Command command,
                                       const QJsonObject& payload,
                                       QObject* context,
                                       AsyncCallback callback,
                                       int timeoutMs) {
    if (!m_network.isConnected()) {
        if (!m_network.connectToServer(QStringLiteral("127.0.0.1"), 8080)) {
            if (callback) {
                Response r;
                r.errorMessage = QStringLiteral("Not connected to server");
                QMetaObject::invokeMethod(context, [callback, r]() { callback(r); },
                                          Qt::QueuedConnection);
            }
            return;
        }
    }

    common::Message msg(command, payload);
    const QString requestId = msg.requestId();
    QPointer<QObject> ctx(context);

    // Use a shared_ptr<bool> flag so the timeout lambda and the response
    // lambda can coordinate — whoever fires first wins, the other is a
    // no-op. This prevents double-invocation of the callback when the
    // response arrives right after the timeout fires.
    auto fired = std::make_shared<bool>(false);

    m_network.registerResponseHandler(requestId,
        [callback, ctx, fired](const common::Message& reply) {
            if (*fired) return;
            *fired = true;
            if (!callback) return;
            Response r;
            r.ok = true;
            r.status = reply.status();
            r.payload = reply.payload();
            r.errorMessage = reply.payload().value("error").toString();
            // Marshal back to the context's thread.
            if (ctx) {
                QMetaObject::invokeMethod(ctx.data(),
                    [callback, r]() { callback(r); },
                    Qt::QueuedConnection);
            }
        });

    m_network.sendMessage(msg);

    // Timeout safety — if no response, fire the callback with a timeout
    // error and unregister the handler so a late-arriving response is
    // a no-op (prevents use-after-free on the captured `fired` flag and
    // double-invocation of the callback).
    QTimer::singleShot(timeoutMs, context, [this, callback, ctx, fired, requestId]() {
        if (*fired) return;
        *fired = true;
        // Unregister the response handler so a late server reply can't
        // fire the (now-stale) lambda.
        m_network.unregisterResponseHandler(requestId);
        if (callback) {
            Response r;
            r.errorMessage = QStringLiteral("Request timed out");
            if (ctx) {
                QMetaObject::invokeMethod(ctx.data(),
                    [callback, r]() { callback(r); },
                    Qt::QueuedConnection);
            }
        }
    });
}

void NetworkService::subscribeEvent(common::Command eventCommand,
                                     QObject* context,
                                     std::function<void(const common::Message&)> handler) {
    QPointer<QObject> ctx(context);
    m_network.registerEventHandler(eventCommand,
        [handler, ctx](const common::Message& msg) {
            if (!ctx) return;
            // Marshal to the context's thread so QML access is safe.
            QMetaObject::invokeMethod(ctx.data(),
                [handler, msg]() { handler(msg); },
                Qt::QueuedConnection);
        });
}

void NetworkService::unsubscribeEvent(common::Command eventCommand) {
    m_network.unregisterEventHandler(eventCommand);
}

} // namespace bookclub::client
