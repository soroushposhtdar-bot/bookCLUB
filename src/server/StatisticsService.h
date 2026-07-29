// src/server/StatisticsService.h
#pragma once

#include <QObject>
#include <QDateTime>
#include <QMap>
#include <QMutex>

#include "common/Models/ServerHealth.h"
#include "common/Models/PublisherStats.h"

namespace bookclub::server {

class DatabaseManager;
class ConnectionManager;

class StatisticsService : public QObject {
    Q_OBJECT
public:
    explicit StatisticsService(DatabaseManager* db, ConnectionManager* connectionManager, QObject* parent = nullptr);
    ~StatisticsService() override = default;

    // --- Server Health ---
    common::ServerHealth getServerHealth() const;
    double getCpuUsage() const;
    double getRamUsage() const;
    int getOnlineUsers() const;

    // --- Sales Statistics ---
    double getTotalRevenue() const;
    int getTotalOrders() const;
    int getTotalBooksSold() const;
    QMap<QString, int> getSalesByPeriod(const QDateTime& from, const QDateTime& to);

    // --- Publisher Statistics ---
    common::PublisherStats* getPublisherStats(const QString& publisherId);

    // --- Request Logging ---
    void logRequest(const QString& clientId, const QString& command, int statusCode, qint64 durationMs);

signals:
    void statsUpdated();

private:
    void updateHealthMetrics() const;

    DatabaseManager* m_db;
    ConnectionManager* m_connectionManager;

    // Health metrics
    mutable common::ServerHealth m_health;
    mutable QDateTime m_lastHealthUpdate;
    mutable QMutex m_mutex;

    // Request counters
    mutable QMap<QString, int> m_requestCounters;
    mutable qint64 m_totalRequests = 0;
    mutable qint64 m_errorRequests = 0;
};

} // namespace bookclub::server
