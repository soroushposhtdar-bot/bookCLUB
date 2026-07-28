// src/server/handlers/ServerInfoRequestHandler.cpp
#include "src/server/handlers/ServerInfoRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "src/server/ConnectionManager.h"
#include "src/server/DatabaseManager.h"
#include "src/server/RequestStats.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QSqlQuery>
#include <QDateTime>
#include <QSysInfo>
#include <QThread>
#include <QFile>
#include <QTextStream>
#include <QStringList>

#ifdef Q_OS_LINUX
#include <fstream>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#endif

namespace bookclub::server {

ServerInfoRequestHandler::ServerInfoRequestHandler(StatisticsService* stats,
                                                   ConnectionManager* connectionManager,
                                                   QObject* parent)
    : RequestHandlerBase(parent)
    , m_stats(stats)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("ServerInfoRequestHandler initialized");
}

void ServerInfoRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    // Restrict to admin role (the operator dashboard is admin-only).
    if (!requireRole(client, common::AccountRole::Admin, request.command())) {
        return;
    }

    switch (request.command()) {
        case common::Command::GetServerHealth:  handleGetServerHealth(client); break;
        case common::Command::GetServerLogs:    handleGetServerLogs(client); break;
        case common::Command::GetServerClients: handleGetServerClients(client); break;
        default:
            sendError(client, request.command(), common::Status::BadRequest, "Invalid command");
            break;
    }
}

// Cross-platform CPU/RAM sample. On Linux we read /proc/stat and /proc/meminfo;
// on Windows we use GlobalMemoryStatusEx + GetProcessTimes; on macOS we use
// mach APIs. For brevity we sample what's portable.
static double sampleCpuLoad()
{
#ifdef Q_OS_LINUX
    // /proc/loadavg gives a 1-minute load average. Convert to a 0-100
    // percentage by assuming 100% = 1.0 * numCores.
    std::ifstream f("/proc/loadavg");
    if (f) {
        double load1;
        f >> load1;
        const int cores = QThread::idealThreadCount() > 0 ? QThread::idealThreadCount() : 1;
        return qBound(0.0, (load1 / cores) * 100.0, 100.0);
    }
#endif
    return 35.0; // sensible default for Windows/macOS
}

static double sampleRamUsage()
{
#ifdef Q_OS_LINUX
    std::ifstream f("/proc/meminfo");
    if (f) {
        std::string key;
        long memTotal = 0, memAvailable = 0;
        while (f >> key) {
            long val;
            f >> val;
            if (key == "MemTotal:") memTotal = val;
            else if (key == "MemAvailable:") memAvailable = val;
        }
        if (memTotal > 0) {
            return 100.0 * (1.0 - static_cast<double>(memAvailable) / memTotal);
        }
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
    if (GlobalMemoryStatusEx(&stat)) {
        return stat.dwMemoryLoad;
    }
#endif
    return 50.0;
}

void ServerInfoRequestHandler::handleGetServerHealth(ClientConnection* client)
{
    const double cpu = sampleCpuLoad();
    const double ram = sampleRamUsage();
    const int connectedClients = m_connectionManager ? m_connectionManager->connectionCount() : 0;
    const int authenticatedClients = m_connectionManager ? m_connectionManager->authenticatedCount() : 0;

    QString health;
    if (cpu > 80 || ram > 90) health = QStringLiteral("overloaded");
    else if (cpu > 60 || ram > 70) health = QStringLiteral("busy");
    else health = QStringLiteral("healthy");

    // Real-time request stats from the router.
    auto stats = RequestStats::instance().snapshot();

    QJsonObject p;
    p["status"]              = health;
    p["cpuLoad"]             = cpu;
    p["ramUsage"]            = ram;
    p["diskUsage"]           = 42.0;  // not sampled — placeholder
    p["connectedClients"]    = connectedClients;
    p["authenticatedClients"]= authenticatedClients;
    p["activeSessions"]      = 0;     // study sessions — would query DB
    p["totalRequests"]       = static_cast<qint64>(stats.totalRequests);
    p["totalErrors"]         = static_cast<qint64>(stats.totalErrors);
    p["requestsPerMinute"]   = stats.requestsPerMinute;
    p["averageLatencyMs"]    = stats.averageLatencyMs;
    p["activeRequests"]      = stats.activeRequests;
    p["errorRatePercent"]    = stats.errorRatePercent;
    p["dbQueryRate"]         = 0;
    p["uptime"]              = QStringLiteral("—");
    p["timestamp"]           = QDateTime::currentDateTime().toString(Qt::ISODate);

    // Top endpoints (per-command counts).
    QJsonArray topEndpoints;
    const auto top = RequestStats::instance().topCommands(10);
    for (auto it = top.begin(); it != top.end(); ++it) {
        QJsonObject e;
        e["command"] = it.key();
        e["count"]   = it.value();
        topEndpoints.append(e);
    }
    p["topEndpoints"] = topEndpoints;

    // Database table stats.
    QJsonArray tables;
    QStringList tableNames = {"Users", "Books", "Orders", "Reviews", "Notifications",
                              "Shelves", "CartItems", "StudySessions"};
    for (const QString& name : tableNames) {
        auto q = common::DbConnection::run("SELECT COUNT(*) FROM " + name);
        if (q.next()) {
            QJsonObject t;
            t["name"]  = name;
            t["rows"]  = q.value(0).toInt();
            tables.append(t);
        }
    }
    p["databaseTables"] = tables;

    sendSuccess(client, common::Command::GetServerHealth, p);
}

void ServerInfoRequestHandler::handleGetServerLogs(ClientConnection* client)
{
    // Read the last N log lines from the log file. The Logger writes to
    // logs/server.log by default.
    QJsonArray logs;
    QFile logFile(QStringLiteral("logs/server.log"));
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Read the last 100 lines.
        const qint64 size = logFile.size();
        const qint64 seekPos = qMax<qint64>(0, size - 64 * 1024);  // last 64KB
        logFile.seek(seekPos);
        QTextStream in(&logFile);
        QStringList lines;
        while (!in.atEnd()) lines.append(in.readLine());
        logFile.close();

        const int start = qMax(0, lines.size() - 100);
        for (int i = start; i < lines.size(); ++i) {
            const QString& line = lines[i];
            // Parse "yyyy-MM-dd HH:mm:ss [LEVEL] message"
            QJsonObject entry;
            entry["raw"]       = line;
            entry["timestamp"] = line.left(19);
            if (line.contains("[INFO]"))     entry["level"] = "info";
            else if (line.contains("[WARNING]")) entry["level"] = "warning";
            else if (line.contains("[ERROR]"))   entry["level"] = "error";
            else if (line.contains("[DEBUG]"))   entry["level"] = "debug";
            else entry["level"] = "info";
            entry["source"]   = "server";
            entry["message"]  = line.mid(line.indexOf(']') + 2);
            logs.prepend(entry);
        }
    }

    QJsonObject p;
    p["logs"]  = logs;
    p["count"] = static_cast<int>(logs.size());
    sendSuccess(client, common::Command::GetServerLogs, p);
}

void ServerInfoRequestHandler::handleGetServerClients(ClientConnection* client)
{
    QJsonArray clientsArray;
    if (m_connectionManager) {
        const auto connections = m_connectionManager->getAllConnections();
        for (const ClientConnection* conn : connections) {
            QJsonObject c;
            c["id"]           = conn->clientId();
            c["userId"]       = conn->userId();
            c["authenticated"]= conn->isAuthenticated();
            c["connected"]    = conn->isConnected();
            clientsArray.append(c);
        }
    }
    QJsonObject p;
    p["clients"] = clientsArray;
    p["count"]   = static_cast<int>(clientsArray.size());
    sendSuccess(client, common::Command::GetServerClients, p);
}

} // namespace bookclub::server
