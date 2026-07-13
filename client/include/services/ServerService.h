// =============================================================================
//  ServerService.h
// =============================================================================
//  Service layer for the Server / Operations dashboard. Provides live
//  monitoring data: connected clients, active sessions, group reading rooms,
//  database table stats, slow queries, rolling logs, microservice health,
//  connection-pool status, request-rate series, top endpoints, error
//  breakdown and geographic distribution — plus the headline KPIs
//  (connected clients, active sessions, DB query rate, CPU / RAM / disk).
//
//  All data is mocked in-memory; refresh() slightly perturbs the KPIs to
//  simulate a live monitoring feed.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>

#include "services/MockDataStore.h"

namespace bookclub::client {

class ServerService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit ServerService(QObject* parent = nullptr);

    Q_INVOKABLE void setDataStore(MockDataStore* store);

    // ---- Live monitoring collections ----
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

    // ---- Headline KPI metrics ----
    Q_INVOKABLE int connectedClientCount() const;
    Q_INVOKABLE int activeSessionCount() const;
    Q_INVOKABLE int dbQueryRate() const;
    Q_INVOKABLE int cpuLoad() const;
    Q_INVOKABLE int ramUsage() const;
    Q_INVOKABLE int diskUsage() const;

    // ---- Mutations / actions ----
    Q_INVOKABLE void addLog(const QString& level, const QString& source, const QString& message);
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void disconnectClient(const QString& clientId);
    Q_INVOKABLE void terminateSession(const QString& sessionId);
    Q_INVOKABLE QVariantList filterLogs(const QString& level, const QString& search) const;
    Q_INVOKABLE void refresh();

signals:
    void clientsChanged();
    void sessionsChanged();
    void logsChanged();
    void servicesChanged();

private:
    MockDataStore* m_store = nullptr;

    // Mock data collections
    QVariantList m_clients;
    QVariantList m_sessions;
    QVariantList m_rooms;
    QVariantList m_databaseTables;
    QVariantList m_slowQueries;
    QVariantList m_logs;
    QVariantList m_services;
    QVariantList m_connectionPool;
    QVariantList m_requestSeries;
    QVariantList m_topEndpoints;
    QVariantList m_errorBreakdown;
    QVariantList m_geographicDistribution;

    // KPI scalars
    int m_connectedClientCount = 20;
    int m_activeSessionCount   = 18;
    int m_dbQueryRate          = 4820;
    int m_cpuLoad              = 38;
    int m_ramUsage             = 64;
    int m_diskUsage            = 71;

    void _seedClients();
    void _seedSessions();
    void _seedRooms();
    void _seedDatabaseTables();
    void _seedSlowQueries();
    void _seedLogs();
    void _seedServices();
    void _seedConnectionPool();
    void _seedRequestSeries();
    void _seedTopEndpoints();
    void _seedErrorBreakdown();
    void _seedGeographicDistribution();
};

} // namespace bookclub::client
