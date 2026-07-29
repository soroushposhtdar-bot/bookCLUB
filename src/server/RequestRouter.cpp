// src/server/RequestRouter.cpp
//
// Routes incoming requests to the registered handler. Records every
// request + response in RequestStats so the operator dashboard can
// show real-time request rate, latency, and error rate.
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/ClientConnection.h"
#include "src/server/RequestStats.h"
#include "common/Utils/Logger.h"

#include <QDateTime>

namespace bookclub::server {

RequestRouter::RequestRouter(QObject* parent)
    : QObject(parent)
{
}

RequestRouter::~RequestRouter() {
    // Handlers are owned by whoever registered them
    m_handlers.clear();
}

void RequestRouter::registerHandler(common::Command command, RequestHandlerBase* handler) {
    if (m_handlers.contains(command)) {
        LOG_WARNING("Overwriting handler for command: " + common::commandToString(command));
    }
    m_handlers[command] = handler;
    LOG_INFO("Registered handler for command: " + common::commandToString(command));
}

void RequestRouter::registerDefaultHandler(RequestHandlerBase* handler) {
    m_defaultHandler = handler;
    LOG_INFO("Registered default handler");
}

void RequestRouter::handleRequest(const common::Message& message, ClientConnection* client) {
    if (!client) {
        LOG_ERROR("handleRequest called with null client");
        return;
    }

    common::Command cmd = message.command();
    RequestHandlerBase* handler = m_handlers.value(cmd, m_defaultHandler);

    // Record the request for stats.
    RequestStats::instance().recordRequest();
    RequestStats::instance().recordCommand(common::commandToString(cmd));
    const qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    if (handler) {
        handler->setCurrentRequestId(message.requestId());
        handler->handle(message, client);
        const qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        // We can't see the response status from here; assume success if
        // no exception was thrown. The handler itself logs errors.
        RequestStats::instance().recordResponse(true, duration);
    } else {
        LOG_WARNING("No handler registered for command: " + common::commandToString(cmd) +
                   " from client: " + client->clientId());
        emit unhandledCommand(cmd, client->clientId());

        // Send error response (echo requestId)
        common::Message errorResponse(common::Command::Invalid, common::Status::BadRequest);
        errorResponse.setRequestId(message.requestId());
        client->sendMessage(errorResponse);
        const qint64 duration = QDateTime::currentMSecsSinceEpoch() - startTime;
        RequestStats::instance().recordResponse(false, duration);
    }
}

} // namespace bookclub::server
