// client/include/services/ServerService.h
//
// Server-operator service. Backed by GetServerHealth, GetServerClients,
// GetServerLogs. Live KPIs (CPU/RAM/client count) come from the server
// process via GetServerHealth — no more local random-walk.
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>

namespace bookclub::client {

class ServerService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ServerService(QObject* parent = nullptr);
    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op

    // ----- Collections (server-backed) -----
    Q_INVOKABLE QVariantList clients() const;
    Q_INVOKABLE QVariantList sessions() const;
    Q_INVOKABLE QVariantList rooms() const;
    Q_INVOKABLE QVariantList databaseTables() const;
    Q_INVOKABLE QVariantList slowQueries() const;
    Q_INVOKABLE QVariantList logs() const;
    Q_INVOKABLE QVariantList services() const;
    Q_INVOKABLE QVariantList connectionPool() const;
    Q_INVOKABLE QVariantList requestSeries() const;
    Q_INVOKABLE QVariantList topEndpoints() const;
    Q_INVOKABLE QVariantList errorBreakdown() const;
    Q_INVOKABLE QVariantList geographicDistribution() const;

    // ----- KPIs (server-backed via GetServerHealth) -----
    Q_INVOKABLE int connectedClientCount() const;
    Q_INVOKABLE int authenticatedClientCount() const;
    Q_INVOKABLE int activeSessionCount() const;
    Q_INVOKABLE int dbQueryRate() const;
    Q_INVOKABLE qint64 totalRequests() const;
    Q_INVOKABLE qint64 totalErrors() const;
    Q_INVOKABLE int requestsPerMinute() const;
    Q_INVOKABLE double averageLatencyMs() const;
    Q_INVOKABLE int activeRequests() const;
    Q_INVOKABLE int errorRatePercent() const;

    // ----- System metrics (server-backed) -----
    Q_INVOKABLE double cpuLoad() const;
    Q_INVOKABLE double ramUsage() const;
    Q_INVOKABLE double diskUsage() const;

    // ----- Formatted text -----
    Q_INVOKABLE QString cpuLoadText() const;
    Q_INVOKABLE QString ramUsageText() const;
    Q_INVOKABLE QString diskUsageText() const;
    Q_INVOKABLE QString healthStatus() const;
    Q_INVOKABLE QString uptime() const;

    // ----- Actions -----
    Q_INVOKABLE void kickClient(const QString& clientId);
    Q_INVOKABLE void disconnectClient(const QString& clientId) { kickClient(clientId); }
    Q_INVOKABLE void closeSession(const QString& sessionId);
    Q_INVOKABLE void terminateSession(const QString& sessionId) { closeSession(sessionId); }
    Q_INVOKABLE void closeRoom(const QString& roomId);
    Q_INVOKABLE void restartService(const QString& serviceName);
    Q_INVOKABLE void clearLogs();

    // ----- Log management -----
    Q_INVOKABLE void addLog(const QString& level, const QString& source, const QString& message);
    Q_INVOKABLE QVariantList filterLogs(const QString& level, const QString& search) const;

    // ----- Refresh -----
    Q_INVOKABLE void refresh();

signals:
    void dataChanged();
    void logsChanged();
    void clientsChanged();
    void servicesChanged();

private:
    // Fetches GetServerHealth once per refresh cycle.
    QJsonObject fetchHealth() const;

    mutable QJsonObject m_healthCache;
    mutable QVariantList m_clientsCache;
    mutable QVariantList m_logsCache;
    // BUG FIX (server logs hidden after re-entry): added a validity flag so
    // that an EMPTY logs cache (0 logs from the server) is NOT re-fetched
    // on every read. Previously, `if (!m_logsCache.isEmpty()) return m_logsCache;`
    // meant that an empty cache always triggered a network round-trip — which
    // failed silently when the server was unreachable, leaving the logs page
    // permanently blank after the first refresh() cleared the cache.
    mutable bool m_logsCacheValid = false;
    mutable bool m_healthCacheValid = false;
    mutable bool m_clientsCacheValid = false;
};

} // namespace bookclub::client
