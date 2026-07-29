// src/server/RequestStats.cpp
#include "src/server/RequestStats.h"

#include <QDateTime>
#include <algorithm>

namespace bookclub::server {

RequestStats& RequestStats::instance() {
    static RequestStats instance;
    return instance;
}

RequestStats::RequestStats(QObject* parent) : QObject(parent) {}

void RequestStats::recordRequest()
{
    QMutexLocker locker(&m_mutex);
    ++m_totalRequests;
    ++m_activeRequests;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_recentRequestTimes.enqueue(now);
    // Prune entries older than 60 seconds.
    const qint64 cutoff = now - 60000;
    while (!m_recentRequestTimes.isEmpty() && m_recentRequestTimes.head() < cutoff) {
        m_recentRequestTimes.dequeue();
    }
}

void RequestStats::recordResponse(bool success, qint64 durationMs)
{
    QMutexLocker locker(&m_mutex);
    if (m_activeRequests > 0) --m_activeRequests;
    if (!success) ++m_totalErrors;
    m_totalLatencyMs += durationMs;
    ++m_latencyCount;
}

RequestStats::Snapshot RequestStats::snapshot() const
{
    QMutexLocker locker(&m_mutex);
    Snapshot s;
    s.totalRequests     = m_totalRequests;
    s.totalErrors       = m_totalErrors;
    s.requestsPerMinute = m_recentRequestTimes.size();
    s.activeRequests    = m_activeRequests;
    s.averageLatencyMs  = m_latencyCount > 0
                          ? static_cast<double>(m_totalLatencyMs) / m_latencyCount
                          : 0.0;
    s.errorRatePercent  = m_totalRequests > 0
                          ? static_cast<int>(m_totalErrors * 100 / m_totalRequests)
                          : 0;
    return s;
}

QHash<QString, int> RequestStats::topCommands(int count) const
{
    QMutexLocker locker(&m_mutex);
    QHash<QString, int> result;
    // Sort by count descending, take top N.
    QList<QPair<QString, int>> sorted;
    for (auto it = m_commandCounts.begin(); it != m_commandCounts.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });
    const int n = qMin(count, sorted.size());
    for (int i = 0; i < n; ++i) {
        result.insert(sorted[i].first, sorted[i].second);
    }
    return result;
}

void RequestStats::recordCommand(const QString& commandName)
{
    QMutexLocker locker(&m_mutex);
    m_commandCounts[commandName]++;
}

} // namespace bookclub::server
