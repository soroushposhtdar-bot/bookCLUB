// src/server/RequestStats.h
//
// Thread-safe request statistics tracker. Used by ServerInfoRequestHandler
// to report real-time request rate, error rate, and response queue depth
// to the operator dashboard.
//
// Every handler call increments totalRequests (and errorRequests on
// failure). A sliding 60-second window keeps per-second counts so we
// can compute requests-per-second.
#pragma once

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QHash>

namespace bookclub::server {

class RequestStats : public QObject {
    Q_OBJECT
public:
    static RequestStats& instance();

    // Called by the router before dispatching a request.
    void recordRequest();

    // Called by the router after the handler returns. `success` = false
    // means the handler returned a non-2xx status.
    void recordResponse(bool success, qint64 durationMs);

    // --- Snapshot for the operator dashboard ---
    struct Snapshot {
        qint64  totalRequests      = 0;
        qint64  totalErrors        = 0;
        int     requestsPerMinute  = 0;
        double  averageLatencyMs   = 0.0;
        int     activeRequests     = 0;   // in-flight (started, not finished)
        int     errorRatePercent   = 0;   // errors / total * 100
    };
    Snapshot snapshot() const;

    // Per-command counters (for the "top endpoints" view).
    QHash<QString, int> topCommands(int count = 10) const;

    void recordCommand(const QString& commandName);

private:
    RequestStats(QObject* parent = nullptr);
    ~RequestStats() override = default;
    RequestStats(const RequestStats&) = delete;
    RequestStats& operator=(const RequestStats&) = delete;

    mutable QMutex m_mutex;
    qint64 m_totalRequests = 0;
    qint64 m_totalErrors   = 0;
    int    m_activeRequests = 0;

    // Sliding window: timestamps of requests in the last 60 seconds.
    QQueue<qint64> m_recentRequestTimes;

    // Latency tracking (sum + count for average).
    qint64 m_totalLatencyMs = 0;
    qint64 m_latencyCount   = 0;

    // Per-command counters.
    QHash<QString, int> m_commandCounts;
};

} // namespace bookclub::server
