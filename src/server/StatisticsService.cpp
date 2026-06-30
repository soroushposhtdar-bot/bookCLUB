// src/server/StatisticsService.cpp
#include "src/server/StatisticsService.h"
#include "src/server/DatabaseManager.h"
#include "src/server/ConnectionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DateTimeUtils.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QProcess>
#include <QFile>
#include <QDebug>

namespace bookclub::server {

StatisticsService::StatisticsService(DatabaseManager* db, ConnectionManager* connectionManager, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_connectionManager(connectionManager)
{
    LOG_INFO("StatisticsService initialized");
    updateHealthMetrics();
}

common::ServerHealth StatisticsService::getServerHealth() const
{
    QMutexLocker locker(&m_mutex);

    // Update if more than 5 seconds old
    if (m_lastHealthUpdate.isValid() && m_lastHealthUpdate.secsTo(common::DateTimeUtils::nowUtc()) > 5) {
        updateHealthMetrics();
    }

    return m_health;
}

double StatisticsService::getCpuUsage() const
{
    QMutexLocker locker(&m_mutex);
#ifdef Q_OS_WIN
    // Windows: Use performance counters or WMI
    return 0.0;
#else
    // Linux: Read /proc/stat
    QFile statFile("/proc/stat");
    if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.0;
    }
    QString line = QString::fromUtf8(statFile.readLine());
    statFile.close();

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5) return 0.0;

    // Simple calculation (user + nice + system + idle)
    qint64 user = parts[1].toLongLong();
    qint64 nice = parts[2].toLongLong();
    qint64 system = parts[3].toLongLong();
    qint64 idle = parts[4].toLongLong();

    qint64 total = user + nice + system + idle;
    if (total == 0) return 0.0;

    return 100.0 * (1.0 - (double)idle / (double)total);
#endif
}

double StatisticsService::getRamUsage() const
{
    QMutexLocker locker(&m_mutex);
#ifdef Q_OS_WIN
    return 0.0;
#else
    // Linux: Read /proc/meminfo
    QFile memInfo("/proc/meminfo");
    if (!memInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0.0;
    }

    qint64 totalMem = 0, availableMem = 0;
    QStringList lines = QString::fromUtf8(memInfo.readAll()).split('\n');
    memInfo.close();

    for (const QString& line : lines) {
        if (line.startsWith("MemTotal:")) {
            totalMem = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
        } else if (line.startsWith("MemAvailable:")) {
            availableMem = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
        }
    }

    if (totalMem == 0) return 0.0;
    return 100.0 * (1.0 - (double)availableMem / (double)totalMem);
#endif
}

int StatisticsService::getOnlineUsers() const
{
    if (!m_connectionManager) return 0;
    return m_connectionManager->authenticatedCount();
}

double StatisticsService::getTotalRevenue() const
{
    if (!m_db || !m_db->isOpen()) return 0.0;

    QSqlQuery query = m_db->executeQueryWithResult(
        "SELECT SUM(finalTotal) FROM Orders WHERE paid = 1 AND completed = 1"
    );

    if (query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

int StatisticsService::getTotalOrders() const
{
    if (!m_db || !m_db->isOpen()) return 0;

    QSqlQuery query = m_db->executeQueryWithResult(
        "SELECT COUNT(*) FROM Orders WHERE paid = 1"
    );

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int StatisticsService::getTotalBooksSold() const
{
    if (!m_db || !m_db->isOpen()) return 0;

    QSqlQuery query = m_db->executeQueryWithResult(
        "SELECT SUM(totalSales) FROM Books"
    );

    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QMap<QString, int> StatisticsService::getSalesByPeriod(const QDateTime& from, const QDateTime& to)
{
    QMap<QString, int> result;
    if (!m_db || !m_db->isOpen()) return result;

    QSqlQuery query = m_db->executeQueryWithResult(
        "SELECT DATE(createdAt) as date, COUNT(*) as count "
        "FROM Orders WHERE paid = 1 AND createdAt BETWEEN ? AND ? "
        "GROUP BY DATE(createdAt) ORDER BY date",
        {from, to}
    );

    while (query.next()) {
        result[query.value("date").toString()] = query.value("count").toInt();
    }

    return result;
}

common::PublisherStats* StatisticsService::getPublisherStats(const QString& publisherId)
{
    if (!m_db || !m_db->isOpen()) return nullptr;

    auto* stats = new common::PublisherStats;
    stats->setPublisherId(publisherId);

    // Get all books by this publisher
    QSqlQuery bookQuery = m_db->executeQueryWithResult(
        "SELECT id, title, totalSales, averageRating, price FROM Books WHERE publisherId = ?",
        {publisherId}
    );

    int totalBooks = 0;
    int totalSales = 0;
    double totalRevenue = 0.0;
    QVector<common::BookStatItem*> bookStats;

    while (bookQuery.next()) {
        auto* item = new common::BookStatItem;
        item->setBookId(bookQuery.value("id").toString());
        item->setTitle(bookQuery.value("title").toString());
        item->setSalesCount(bookQuery.value("totalSales").toInt());
        item->setRevenue(bookQuery.value("totalSales").toInt() * bookQuery.value("price").toDouble());
        item->setAverageRating(bookQuery.value("averageRating").toDouble());
        bookStats.append(item);

        totalBooks++;
        totalSales += item->salesCount();
        totalRevenue += item->revenue();
    }

    stats->setBookStats(bookStats);
    stats->setTotalBooks(totalBooks);
    stats->setTotalSales(totalSales);
    stats->setTotalRevenue(totalRevenue);
    stats->setUpdatedAt(common::DateTimeUtils::nowUtc());

    return stats;
}

void StatisticsService::logRequest(const QString& clientId, const QString& command, int statusCode, qint64 durationMs)
{
    QMutexLocker locker(&m_mutex);
    m_totalRequests++;

    if (statusCode >= 400) {
        m_errorRequests++;
    }

    m_requestCounters[command] = m_requestCounters.value(command, 0) + 1;
}

void StatisticsService::updateHealthMetrics() const
{
    QMutexLocker locker(&m_mutex);

    m_health.cpuUsage = getCpuUsage();
    m_health.ramUsage = getRamUsage();
    m_health.onlineClients = getOnlineUsers();
    m_health.overloaded = (m_health.cpuUsage > 80.0 || m_health.ramUsage > 90.0);
    m_health.state = m_health.overloaded ? "Overloaded" :
                    (m_health.cpuUsage > 60.0 || m_health.ramUsage > 70.0) ? "Busy" : "Healthy";

    m_lastHealthUpdate = common::DateTimeUtils::nowUtc();
}

} // namespace bookclub::server
