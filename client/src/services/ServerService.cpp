// client/src/services/ServerService.cpp
//
// Full socket-backed implementation of ServerService. All KPIs come from
// the server via GetServerHealth — no more local random-walk.
#include "services/ServerService.h"
#include "services/NetworkService.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

namespace bookclub::client {

ServerService::ServerService(QObject* parent) : QObject(parent) {}

QJsonObject ServerService::fetchHealth() const {
    // BUG FIX (server logs hidden after re-entry): use a validity flag
    // instead of checking isEmpty() so an empty-but-valid cache is not
    // re-fetched on every read.
    if (m_healthCacheValid) return m_healthCache;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetServerHealth);
    if (resp.isSuccess()) {
        m_healthCache = resp.payload;
        m_healthCacheValid = true;
    }
    return m_healthCache;
}

// ============================================================================
//  Collections
// ============================================================================

QVariantList ServerService::clients() const {
    // BUG FIX: use a validity flag instead of checking isEmpty().
    if (m_clientsCacheValid) return m_clientsCache;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetServerClients);
    if (!resp.isSuccess()) return {};
    m_clientsCache = resp.payload.value("clients").toArray().toVariantList();
    m_clientsCacheValid = true;
    return m_clientsCache;
}

QVariantList ServerService::sessions() const {
    // Active group-reading sessions — server exposes them via the
    // StudySession handler's list endpoint, but we don't have a list
    // command. Return empty for now.
    return {};
}

QVariantList ServerService::rooms() const {
    return sessions();
}

QVariantList ServerService::databaseTables() const {
    // The server can expose its DB schema via GetServerHealth.
    return fetchHealth().value("databaseTables").toArray().toVariantList();
}

QVariantList ServerService::slowQueries() const {
    return fetchHealth().value("slowQueries").toArray().toVariantList();
}

QVariantList ServerService::logs() const {
    // BUG FIX (server logs hidden after re-entry): use a validity flag
    // instead of checking isEmpty() so an empty-but-valid cache (0 logs
    // from the server) is NOT re-fetched on every read. Previously,
    // `if (!m_logsCache.isEmpty()) return m_logsCache;` meant that an
    // empty cache always triggered a network round-trip — which failed
    // silently when the server was unreachable, leaving the logs page
    // permanently blank after the first refresh() cleared the cache.
    if (m_logsCacheValid) return m_logsCache;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetServerLogs);
    if (!resp.isSuccess()) return m_logsCache;  // return existing cache (may be empty)
    m_logsCache = resp.payload.value("logs").toArray().toVariantList();
    m_logsCacheValid = true;
    return m_logsCache;
}

QVariantList ServerService::services() const {
    return fetchHealth().value("services").toArray().toVariantList();
}

QVariantList ServerService::connectionPool() const {
    return fetchHealth().value("connectionPool").toArray().toVariantList();
}

QVariantList ServerService::requestSeries() const {
    return fetchHealth().value("requestSeries").toArray().toVariantList();
}

QVariantList ServerService::topEndpoints() const {
    return fetchHealth().value("topEndpoints").toArray().toVariantList();
}

QVariantList ServerService::errorBreakdown() const {
    return fetchHealth().value("errorBreakdown").toArray().toVariantList();
}

QVariantList ServerService::geographicDistribution() const {
    return fetchHealth().value("geographicDistribution").toArray().toVariantList();
}

// ============================================================================
//  KPIs (server-backed)
// ============================================================================

int ServerService::connectedClientCount() const {
    return fetchHealth().value("connectedClients").toInt();
}

int ServerService::authenticatedClientCount() const {
    return fetchHealth().value("authenticatedClients").toInt();
}

int ServerService::activeSessionCount() const {
    return fetchHealth().value("activeSessions").toInt();
}

int ServerService::dbQueryRate() const {
    return fetchHealth().value("dbQueryRate").toInt();
}

qint64 ServerService::totalRequests() const {
    return static_cast<qint64>(fetchHealth().value("totalRequests").toDouble());
}

qint64 ServerService::totalErrors() const {
    return static_cast<qint64>(fetchHealth().value("totalErrors").toDouble());
}

int ServerService::requestsPerMinute() const {
    return fetchHealth().value("requestsPerMinute").toInt();
}

double ServerService::averageLatencyMs() const {
    return fetchHealth().value("averageLatencyMs").toDouble();
}

int ServerService::activeRequests() const {
    return fetchHealth().value("activeRequests").toInt();
}

int ServerService::errorRatePercent() const {
    return fetchHealth().value("errorRatePercent").toInt();
}

// ============================================================================
//  System metrics (server-backed)
// ============================================================================

double ServerService::cpuLoad() const {
    return fetchHealth().value("cpuLoad").toDouble();
}

double ServerService::ramUsage() const {
    return fetchHealth().value("ramUsage").toDouble();
}

double ServerService::diskUsage() const {
    return fetchHealth().value("diskUsage").toDouble();
}

QString ServerService::cpuLoadText() const {
    return QString::number(cpuLoad(), 'f', 1) + "%";
}

QString ServerService::ramUsageText() const {
    return QString::number(ramUsage(), 'f', 1) + "%";
}

QString ServerService::diskUsageText() const {
    return QString::number(diskUsage(), 'f', 1) + "%";
}

QString ServerService::healthStatus() const {
    const double cpu = cpuLoad();
    const double ram = ramUsage();
    if (cpu > 80 || ram > 90) return QStringLiteral("overloaded");
    if (cpu > 60 || ram > 70) return QStringLiteral("busy");
    return QStringLiteral("healthy");
}

QString ServerService::uptime() const {
    return fetchHealth().value("uptime").toString();
}

// ============================================================================
//  Actions (most are no-ops — server has no admin-action endpoints yet)
// ============================================================================

void ServerService::kickClient(const QString& clientId) {
    // Use BlockUser as the closest available disconnect mechanism.
    // The server will block the user associated with this clientId.
    if (clientId.isEmpty()) return;
    QJsonObject p;
    p["userId"] = clientId;
    auto resp = NetworkService::instance().sendRequest(common::Command::BlockUser, p);
    if (resp.isSuccess()) {
        m_clientsCacheValid = false;
        emit dataChanged();
        emit clientsChanged();
    }
}
void ServerService::closeSession(const QString& /*sessionId*/) {}
void ServerService::closeRoom(const QString& /*roomId*/) {}
void ServerService::restartService(const QString& /*serviceName*/) {}
void ServerService::clearLogs() {
    m_logsCache.clear();
    // BUG FIX: mark the cache as valid (empty) so logs() doesn't re-fetch
    // from the server immediately after clearing. The next refresh() will
    // invalidate the flag and re-fetch.
    m_logsCacheValid = true;
    emit dataChanged();
}

// ============================================================================
//  Log management
// ============================================================================

void ServerService::addLog(const QString& level, const QString& source, const QString& message) {
    QVariantMap entry;
    entry["level"]    = level;
    entry["source"]   = source;
    entry["message"]  = message;
    entry["timestamp"]= QDateTime::currentDateTime().toString(Qt::ISODate);
    m_logsCache.prepend(entry);
    // BUG FIX: mark the cache as valid so logs() doesn't overwrite the
    // newly-added entry with a fresh fetch from the server.
    m_logsCacheValid = true;
    emit dataChanged();
}

QVariantList ServerService::filterLogs(const QString& level, const QString& search) const {
    QVariantList out;
    for (const QVariant& v : logs()) {
        const auto m = v.toMap();
        const bool levelMatch = level.isEmpty() || level == "all" ||
                                m.value("level").toString().toLower() == level.toLower();
        const bool searchMatch = search.isEmpty() ||
                                 m.value("message").toString().contains(search, Qt::CaseInsensitive) ||
                                 m.value("source").toString().contains(search, Qt::CaseInsensitive);
        if (levelMatch && searchMatch) out.append(v);
    }
    return out;
}

// ============================================================================
//  Refresh
// ============================================================================

void ServerService::refresh() {
    // Invalidate all caches so the next property read fetches fresh data.
    m_healthCacheValid = false;
    m_clientsCacheValid = false;
    m_logsCacheValid = false;
    // Proactively fetch logs so ServerLogsPage doesn't need to call
    // filterLogs() on a stale cache after a tab switch.
    auto logResp = NetworkService::instance().sendRequest(common::Command::GetServerLogs);
    if (logResp.isSuccess()) {
        m_logsCache = logResp.payload.value("logs").toArray().toVariantList();
        m_logsCacheValid = true;
    }
    // Proactively fetch clients
    auto clientResp = NetworkService::instance().sendRequest(common::Command::GetServerClients);
    if (clientResp.isSuccess()) {
        m_clientsCache = clientResp.payload.value("clients").toArray().toVariantList();
        m_clientsCacheValid = true;
    }
    emit dataChanged();
    emit logsChanged();
    emit clientsChanged();
}

} // namespace bookclub::client
