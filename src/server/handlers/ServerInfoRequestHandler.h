// src/server/handlers/ServerInfoRequestHandler.h
//
// Handles operator-only info commands: GetServerHealth, GetServerLogs,
// GetServerClients. The ServerShell on the client side polls these every
// 5 seconds to keep the operator dashboard live.
//
// GetServerHealth returns:
//   - CPU + RAM (sampled cross-platform)
//   - Connected + authenticated client counts (live from ConnectionManager)
//   - Request rate (from RequestStats: requests in the last 60s)
//   - Average latency + error rate
//   - Active (in-flight) requests
//   - Top endpoints (per-command counts)
//   - Database table row counts
#pragma once

#include "src/server/RequestHandlerBase.h"
#include "src/server/StatisticsService.h"

namespace bookclub::server {

class ConnectionManager;
class DatabaseManager;

class ServerInfoRequestHandler : public RequestHandlerBase {
    Q_OBJECT
public:
    explicit ServerInfoRequestHandler(StatisticsService* stats,
                                       ConnectionManager* connectionManager,
                                       QObject* parent = nullptr);
    ~ServerInfoRequestHandler() override = default;

    void handle(const common::Message& request, ClientConnection* client) override;

private:
    void handleGetServerHealth(ClientConnection* client);
    void handleGetServerLogs(ClientConnection* client);
    void handleGetServerClients(ClientConnection* client);

    StatisticsService* m_stats;
    ConnectionManager* m_connectionManager;
};

} // namespace bookclub::server
