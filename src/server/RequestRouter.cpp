// src/server/RequestRouter.cpp
#include "src/server/RequestRouter.h"
#include "src/server/RequestHandlerBase.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"

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

    if (handler) {
        handler->handle(message, client);
    } else {
        LOG_WARNING("No handler registered for command: " + common::commandToString(cmd) +
                   " from client: " + client->clientId());
        emit unhandledCommand(cmd, client->clientId());

        // Send error response
        common::Message errorResponse(common::Command::Invalid, common::Status::BadRequest);
        client->sendMessage(errorResponse);
    }
}

} // namespace bookclub::server
