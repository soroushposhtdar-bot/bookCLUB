// =============================================================================
//  ServerService.cpp
// =============================================================================
#include "services/ServerService.h"

#include <QDateTime>
#include <QColor>
#include <algorithm>

namespace bookclub::client {

ServerService::ServerService(QObject* parent)
    : QObject(parent)
{
    _seedClients();
    _seedSessions();
    _seedRooms();
    _seedDatabaseTables();
    _seedSlowQueries();
    _seedLogs();
    _seedServices();
    _seedConnectionPool();
    _seedRequestSeries();
    _seedTopEndpoints();
    _seedErrorBreakdown();
    _seedGeographicDistribution();
}

void ServerService::setDataStore(MockDataStore* store) {
    m_store = store;
}

// ---- Live monitoring collections ----

QVariantList ServerService::clients() const { return m_clients; }
QVariantList ServerService::sessions() const { return m_sessions; }
QVariantList ServerService::rooms() const { return m_rooms; }
QVariantList ServerService::databaseTables() const { return m_databaseTables; }
QVariantList ServerService::slowQueries() const { return m_slowQueries; }
QVariantList ServerService::logs() const { return m_logs; }
QVariantList ServerService::services() const { return m_services; }
QVariantList ServerService::connectionPool() const { return m_connectionPool; }
QVariantList ServerService::requestSeries() const { return m_requestSeries; }
QVariantList ServerService::topEndpoints() const { return m_topEndpoints; }
QVariantList ServerService::errorBreakdown() const { return m_errorBreakdown; }
QVariantList ServerService::geographicDistribution() const { return m_geographicDistribution; }

// ---- KPI metrics ----

int ServerService::connectedClientCount() const { return m_connectedClientCount; }
int ServerService::activeSessionCount()   const { return m_activeSessionCount; }
int ServerService::dbQueryRate()          const { return m_dbQueryRate; }
int ServerService::cpuLoad()              const { return m_cpuLoad; }
int ServerService::ramUsage()             const { return m_ramUsage; }
int ServerService::diskUsage()            const { return m_diskUsage; }

// ---- Mutations / actions ----

void ServerService::addLog(const QString& level, const QString& source, const QString& message) {
    QVariantMap entry;
    entry["timestamp"] = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    entry["level"]     = level.isEmpty() ? QStringLiteral("INFO") : level.toUpper();
    entry["source"]    = source;
    entry["message"]   = message;
    m_logs.prepend(entry);
    while (m_logs.size() > 200) m_logs.removeLast();
    emit logsChanged();
}

void ServerService::clearLogs() {
    m_logs.clear();
    addLog(QStringLiteral("INFO"), QStringLiteral("server"),
           QStringLiteral("Log buffer cleared by operator."));
    emit logsChanged();
}

void ServerService::disconnectClient(const QString& clientId) {
    // Remove the client from the list + decrement the count.
    for (int i = 0; i < m_clients.size(); ++i) {
        if (m_clients[i].toMap().value(QStringLiteral("clientId")).toString() == clientId) {
            const QString user = m_clients[i].toMap().value(QStringLiteral("user")).toString();
            m_clients.removeAt(i);
            if (m_connectedClientCount > 0) --m_connectedClientCount;
            addLog(QStringLiteral("WARN"), QStringLiteral("server"),
                   QStringLiteral("Operator disconnected client %1 (%2).").arg(clientId, user));
            emit clientsChanged();
            return;
        }
    }
}

void ServerService::terminateSession(const QString& sessionId) {
    for (int i = 0; i < m_sessions.size(); ++i) {
        if (m_sessions[i].toMap().value(QStringLiteral("sessionId")).toString() == sessionId) {
            const QString user = m_sessions[i].toMap().value(QStringLiteral("user")).toString();
            m_sessions.removeAt(i);
            if (m_activeSessionCount > 0) --m_activeSessionCount;
            addLog(QStringLiteral("WARN"), QStringLiteral("server"),
                   QStringLiteral("Operator terminated session %1 (%2).").arg(sessionId, user));
            emit sessionsChanged();
            return;
        }
    }
}

QVariantList ServerService::filterLogs(const QString& level, const QString& search) const {
    QVariantList out;
    const QString lvl = level.trimmed().toLower();
    const QString needle = search.trimmed().toLower();
    const bool matchAllLevels = (lvl.isEmpty() || lvl == QStringLiteral("all"));
    for (const auto& v : m_logs) {
        const QVariantMap m = v.toMap();
        if (!matchAllLevels) {
            if (m.value(QStringLiteral("level")).toString().toLower() != lvl) continue;
        }
        if (!needle.isEmpty()) {
            const QString hay = (m.value(QStringLiteral("source")).toString()
                                 + QStringLiteral(" ")
                                 + m.value(QStringLiteral("message")).toString()).toLower();
            if (!hay.contains(needle)) continue;
        }
        out.append(m);
    }
    return out;
}

void ServerService::refresh() {
    // Simulate a live monitoring feed by slightly perturbing the KPI scalars.
    auto jitter = [](int base, int delta) -> int {
        int v = base + (QDateTime::currentMSecsSinceEpoch() % (2 * delta + 1)) - delta;
        return std::clamp(v, 0, 100);
    };
    m_cpuLoad              = jitter(m_cpuLoad, 5);
    m_ramUsage             = jitter(m_ramUsage, 3);
    m_connectedClientCount = std::clamp(m_connectedClientCount
                                        + (QDateTime::currentMSecsSinceEpoch() % 5) - 2, 0, 9999);
    m_dbQueryRate          = std::clamp(m_dbQueryRate
                                        + (QDateTime::currentMSecsSinceEpoch() % 121) - 60, 0, 99999);

    emit clientsChanged();
    emit sessionsChanged();
    emit logsChanged();
    emit servicesChanged();
}

// ---- Seeding ----

void ServerService::_seedClients() {
    // 20 connected clients — mix of user / publisher / admin / server.
    struct Row { QString id, user, role, ip, since; int latency; };
    const QList<Row> rows = {
        {QStringLiteral("C-1042"), QStringLiteral("alice"),   QStringLiteral("user"),      QStringLiteral("192.168.1.24"), QStringLiteral("08:14:02"), 12},
        {QStringLiteral("C-1043"), QStringLiteral("penguin_pub"), QStringLiteral("publisher"), QStringLiteral("172.16.4.91"), QStringLiteral("08:02:55"),  8},
        {QStringLiteral("C-1044"), QStringLiteral("admin"),   QStringLiteral("admin"),     QStringLiteral("10.0.2.55"),   QStringLiteral("07:58:11"),  4},
        {QStringLiteral("C-1045"), QStringLiteral("mark"),    QStringLiteral("user"),      QStringLiteral("192.168.2.10"), QStringLiteral("08:21:33"), 18},
        {QStringLiteral("C-1046"), QStringLiteral("node-1"),  QStringLiteral("server"),    QStringLiteral("10.0.0.7"),    QStringLiteral("00:00:00"),  2},
        {QStringLiteral("C-1047"), QStringLiteral("elena"),   QStringLiteral("user"),      QStringLiteral("172.16.8.42"), QStringLiteral("08:30:09"), 22},
        {QStringLiteral("C-1048"), QStringLiteral("james"),   QStringLiteral("user"),      QStringLiteral("192.168.1.88"), QStringLiteral("08:18:47"), 14},
        {QStringLiteral("C-1049"), QStringLiteral("harborpress"), QStringLiteral("publisher"), QStringLiteral("10.0.3.19"),  QStringLiteral("08:05:22"),  9},
        {QStringLiteral("C-1050"), QStringLiteral("priya"),   QStringLiteral("user"),      QStringLiteral("192.168.5.30"), QStringLiteral("08:41:12"), 27},
        {QStringLiteral("C-1051"), QStringLiteral("root"),    QStringLiteral("admin"),     QStringLiteral("172.16.2.110"),QStringLiteral("07:51:38"),  6},
        {QStringLiteral("C-1052"), QStringLiteral("noah"),    QStringLiteral("user"),      QStringLiteral("10.0.6.71"),   QStringLiteral("08:47:55"), 33},
        {QStringLiteral("C-1053"), QStringLiteral("fatima"),  QStringLiteral("user"),      QStringLiteral("192.168.7.4"),  QStringLiteral("08:49:21"), 19},
        {QStringLiteral("C-1054"), QStringLiteral("node-2"),  QStringLiteral("server"),    QStringLiteral("10.0.0.12"),   QStringLiteral("00:00:00"),  2},
        {QStringLiteral("C-1055"), QStringLiteral("lucas"),   QStringLiteral("user"),      QStringLiteral("172.16.9.55"), QStringLiteral("08:55:03"), 16},
        {QStringLiteral("C-1056"), QStringLiteral("aurora_books"), QStringLiteral("publisher"), QStringLiteral("192.168.3.66"), QStringLiteral("08:11:48"), 11},
        {QStringLiteral("C-1057"), QStringLiteral("mia"),     QStringLiteral("user"),      QStringLiteral("10.0.4.88"),   QStringLiteral("09:01:30"), 41},
        {QStringLiteral("C-1058"), QStringLiteral("david"),   QStringLiteral("user"),      QStringLiteral("192.168.4.27"), QStringLiteral("09:03:18"), 24},
        {QStringLiteral("C-1059"), QStringLiteral("ops"),     QStringLiteral("admin"),     QStringLiteral("172.16.7.130"),QStringLiteral("08:09:59"),  7},
        {QStringLiteral("C-1060"), QStringLiteral("sofia"),   QStringLiteral("user"),      QStringLiteral("192.168.6.49"), QStringLiteral("09:06:42"), 21},
        {QStringLiteral("C-1061"), QStringLiteral("ethan"),   QStringLiteral("user"),      QStringLiteral("10.0.5.33"),   QStringLiteral("09:08:11"), 15},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["clientId"] = r.id;
        m["user"]     = r.user;
        m["role"]     = r.role;
        m["ip"]       = r.ip;
        m["since"]    = r.since;
        m["latency"]  = r.latency;
        m_clients.append(m);
    }
}

void ServerService::_seedSessions() {
    // 18 active sessions — Reading / Browsing / Cart / Checkout / Auth types,
    // Active / Idle / Expiring statuses.
    struct Row { QString id, user, started, last, type, status; };
    const QList<Row> rows = {
        {QStringLiteral("S-9001"), QStringLiteral("alice"),   QStringLiteral("08:14:02"), QStringLiteral("09:08:11"), QStringLiteral("Reading"),  QStringLiteral("Active")},
        {QStringLiteral("S-9002"), QStringLiteral("penguin_pub"), QStringLiteral("08:02:55"), QStringLiteral("09:08:09"), QStringLiteral("Browsing"), QStringLiteral("Active")},
        {QStringLiteral("S-9003"), QStringLiteral("mark"),    QStringLiteral("08:21:33"), QStringLiteral("08:58:41"), QStringLiteral("Cart"),     QStringLiteral("Idle")},
        {QStringLiteral("S-9004"), QStringLiteral("elena"),   QStringLiteral("08:30:09"), QStringLiteral("09:07:55"), QStringLiteral("Reading"),  QStringLiteral("Active")},
        {QStringLiteral("S-9005"), QStringLiteral("james"),   QStringLiteral("08:18:47"), QStringLiteral("09:05:02"), QStringLiteral("Checkout"), QStringLiteral("Expiring")},
        {QStringLiteral("S-9006"), QStringLiteral("priya"),   QStringLiteral("08:41:12"), QStringLiteral("09:08:14"), QStringLiteral("Browsing"), QStringLiteral("Active")},
        {QStringLiteral("S-9007"), QStringLiteral("noah"),    QStringLiteral("08:47:55"), QStringLiteral("09:02:30"), QStringLiteral("Cart"),     QStringLiteral("Idle")},
        {QStringLiteral("S-9008"), QStringLiteral("fatima"),  QStringLiteral("08:49:21"), QStringLiteral("09:08:18"), QStringLiteral("Reading"),  QStringLiteral("Active")},
        {QStringLiteral("S-9009"), QStringLiteral("lucas"),   QStringLiteral("08:55:03"), QStringLiteral("08:59:12"), QStringLiteral("Auth"),     QStringLiteral("Expiring")},
        {QStringLiteral("S-9010"), QStringLiteral("mia"),     QStringLiteral("09:01:30"), QStringLiteral("09:08:21"), QStringLiteral("Browsing"), QStringLiteral("Active")},
        {QStringLiteral("S-9011"), QStringLiteral("david"),   QStringLiteral("09:03:18"), QStringLiteral("09:07:44"), QStringLiteral("Checkout"), QStringLiteral("Active")},
        {QStringLiteral("S-9012"), QStringLiteral("sofia"),   QStringLiteral("09:06:42"), QStringLiteral("09:08:25"), QStringLiteral("Reading"),  QStringLiteral("Active")},
        {QStringLiteral("S-9013"), QStringLiteral("ethan"),   QStringLiteral("09:08:11"), QStringLiteral("09:08:27"), QStringLiteral("Cart"),     QStringLiteral("Active")},
        {QStringLiteral("S-9014"), QStringLiteral("harborpress"), QStringLiteral("08:05:22"), QStringLiteral("08:42:18"), QStringLiteral("Browsing"), QStringLiteral("Idle")},
        {QStringLiteral("S-9015"), QStringLiteral("aurora_books"), QStringLiteral("08:11:48"), QStringLiteral("09:01:09"), QStringLiteral("Browsing"), QStringLiteral("Idle")},
        {QStringLiteral("S-9016"), QStringLiteral("ops"),     QStringLiteral("08:09:59"), QStringLiteral("08:30:55"), QStringLiteral("Auth"),     QStringLiteral("Idle")},
        {QStringLiteral("S-9017"), QStringLiteral("root"),    QStringLiteral("07:51:38"), QStringLiteral("09:08:30"), QStringLiteral("Browsing"), QStringLiteral("Active")},
        {QStringLiteral("S-9018"), QStringLiteral("guest_22"),QStringLiteral("09:07:02"), QStringLiteral("09:08:31"), QStringLiteral("Auth"),     QStringLiteral("Expiring")},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["sessionId"]     = r.id;
        m["user"]          = r.user;
        m["started"]       = r.started;
        m["lastActivity"]  = r.last;
        m["type"]          = r.type;
        m["status"]        = r.status;
        m_sessions.append(m);
    }
}

void ServerService::_seedRooms() {
    // 4 group reading rooms.
    struct Row { QString room, book, owner, started, status; int participants; };
    const QList<Row> rows = {
        {QStringLiteral("Room A — Quiet Rivers"),     QStringLiteral("Quiet Rivers"),      QStringLiteral("alice"),         QStringLiteral("08:30:00"), QStringLiteral("Active"),    6},
        {QStringLiteral("Room B — Midnight Library"), QStringLiteral("The Midnight Library"), QStringLiteral("mark"),       QStringLiteral("08:45:00"), QStringLiteral("Active"),    4},
        {QStringLiteral("Room C — Atomic Habits"),    QStringLiteral("Atomic Habits"),     QStringLiteral("elena"),         QStringLiteral("09:00:00"), QStringLiteral("Idle"),      3},
        {QStringLiteral("Room D — Sapiens"),          QStringLiteral("Sapiens"),           QStringLiteral("noah"),          QStringLiteral("09:05:00"), QStringLiteral("Active"),    8},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["room"]        = r.room;
        m["book"]        = r.book;
        m["participants"]= r.participants;
        m["owner"]       = r.owner;
        m["started"]     = r.started;
        m["status"]      = r.status;
        m_rooms.append(m);
    }
}

void ServerService::_seedDatabaseTables() {
    // 6 database tables: users / books / orders / reviews / notifications / sessions.
    struct Row { QString name; qint64 rows; double sizeMb; };
    const QList<Row> rows = {
        {QStringLiteral("users"),         48213,   124.5},
        {QStringLiteral("books"),         18942,    88.2},
        {QStringLiteral("orders"),        73591,   312.8},
        {QStringLiteral("reviews"),       51204,   156.1},
        {QStringLiteral("notifications"), 102847,  204.6},
        {QStringLiteral("sessions"),      3128,     18.4},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["name"]   = r.name;
        m["rows"]   = static_cast<qlonglong>(r.rows);
        m["sizeMb"] = r.sizeMb;
        m_databaseTables.append(m);
    }
}

void ServerService::_seedSlowQueries() {
    // 5 slow queries.
    struct Row { QString query, duration, table, time; };
    const QList<Row> rows = {
        {QStringLiteral("SELECT * FROM orders JOIN users ON … WHERE created_at > ?"), QStringLiteral("3.42 s"), QStringLiteral("orders"),  QStringLiteral("09:01:18")},
        {QStringLiteral("SELECT COUNT(*) FROM reviews WHERE rating >= 4 GROUP BY book_id"), QStringLiteral("2.87 s"), QStringLiteral("reviews"), QStringLiteral("09:03:44")},
        {QStringLiteral("UPDATE notifications SET read = 1 WHERE user_id IN (…)"), QStringLiteral("2.15 s"), QStringLiteral("notifications"), QStringLiteral("09:04:09")},
        {QStringLiteral("SELECT * FROM books WHERE MATCH(title) AGAINST (?)"), QStringLiteral("1.98 s"), QStringLiteral("books"), QStringLiteral("09:05:51")},
        {QStringLiteral("DELETE FROM sessions WHERE last_activity < ?"), QStringLiteral("1.74 s"), QStringLiteral("sessions"), QStringLiteral("09:06:33")},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["query"]    = r.query;
        m["duration"] = r.duration;
        m["table"]    = r.table;
        m["time"]     = r.time;
        m_slowQueries.append(m);
    }
}

void ServerService::_seedLogs() {
    // 15 log entries — INFO / WARN / ERROR levels.
    struct Row { QString ts, level, source, msg; };
    const QList<Row> rows = {
        {QStringLiteral("09:08:31"), QStringLiteral("INFO"),  QStringLiteral("auth"),        QStringLiteral("Session refreshed for user 'alice'")},
        {QStringLiteral("09:08:18"), QStringLiteral("WARN"),  QStringLiteral("db"),          QStringLiteral("Query on 'reviews' exceeded 2s threshold")},
        {QStringLiteral("09:07:55"), QStringLiteral("INFO"),  QStringLiteral("cart"),        QStringLiteral("Cart checkout completed — order #7841")},
        {QStringLiteral("09:07:12"), QStringLiteral("ERROR"), QStringLiteral("notification"),QStringLiteral("Failed to dispatch push notification to APNS")},
        {QStringLiteral("09:06:48"), QStringLiteral("INFO"),  QStringLiteral("book"),        QStringLiteral("New title published: 'Quiet Rivers'")},
        {QStringLiteral("09:06:33"), QStringLiteral("WARN"),  QStringLiteral("db"),          QStringLiteral("Slow DELETE on 'sessions' (1.74s)")},
        {QStringLiteral("09:05:51"), QStringLiteral("WARN"),  QStringLiteral("db"),          QStringLiteral("Full-text search on 'books' took 1.98s")},
        {QStringLiteral("09:05:09"), QStringLiteral("INFO"),  QStringLiteral("reader"),      QStringLiteral("Reading session started for 'Sapiens'")},
        {QStringLiteral("09:04:32"), QStringLiteral("ERROR"), QStringLiteral("auth"),        QStringLiteral("Login throttled for IP 10.0.6.71")},
        {QStringLiteral("09:03:44"), QStringLiteral("WARN"),  QStringLiteral("db"),          QStringLiteral("Aggregate COUNT on 'reviews' took 2.87s")},
        {QStringLiteral("09:02:55"), QStringLiteral("INFO"),  QStringLiteral("cart"),        QStringLiteral("Item added to cart — book #B-2284")},
        {QStringLiteral("09:01:18"), QStringLiteral("ERROR"), QStringLiteral("db"),          QStringLiteral("JOIN on 'orders' exceeded 3s threshold")},
        {QStringLiteral("09:00:42"), QStringLiteral("INFO"),  QStringLiteral("book"),        QStringLiteral("Catalog cache invalidated")},
        {QStringLiteral("08:59:11"), QStringLiteral("WARN"),  QStringLiteral("notification"),QStringLiteral("Websocket backpressure on broadcast channel")},
        {QStringLiteral("08:58:03"), QStringLiteral("INFO"),  QStringLiteral("server"),      QStringLiteral("Health check OK — 20 clients online")},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["timestamp"] = r.ts;
        m["level"]     = r.level;
        m["source"]    = r.source;
        m["message"]   = r.msg;
        m_logs.append(m);
    }
}

void ServerService::_seedServices() {
    // 5 microservices — Auth / Book / Cart / Notification / Reader.
    struct Row { QString name, status; int latency; };
    const QList<Row> rows = {
        {QStringLiteral("Auth"),         QStringLiteral("Operational"),  42},
        {QStringLiteral("Book"),         QStringLiteral("Operational"),  78},
        {QStringLiteral("Cart"),         QStringLiteral("Degraded"),     163},
        {QStringLiteral("Notification"), QStringLiteral("Operational"),  91},
        {QStringLiteral("Reader"),       QStringLiteral("Operational"),  55},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["name"]    = r.name;
        m["status"]  = r.status;
        m["latency"] = r.latency;
        m_services.append(m);
    }
}

void ServerService::_seedConnectionPool() {
    // 32 connection pool entries — state: "active" / "idle" / "slow".
    const QStringList states = {
        QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"),
        QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"),
        QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"),
        QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"), QStringLiteral("active"),
        QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),
        QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),
        QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),   QStringLiteral("idle"),
        QStringLiteral("slow"),   QStringLiteral("slow"),   QStringLiteral("slow"),   QStringLiteral("slow"),
    };
    for (int i = 0; i < states.size(); ++i) {
        QVariantMap m;
        m["connId"] = QStringLiteral("P-%1").arg(101 + i, 3, 10, QLatin1Char('0'));
        m["state"]  = states[i];
        m["heldForMs"] = (states[i] == QStringLiteral("active")) ? 120 + (i * 17) % 900
                       : (states[i] == QStringLiteral("slow"))   ? 2400 + (i * 53) % 1800
                       : 0;
        m_connectionPool.append(m);
    }
}

void ServerService::_seedRequestSeries() {
    // 14 data points varying between 2000 and 5000 requests/min.
    const QList<int> values = {
        2380, 2742, 3101, 2890, 3475, 4012, 3688,
        4290, 4633, 4180, 3874, 4521, 4890, 4233
    };
    for (int v : values) m_requestSeries.append(QVariant(v));
}

void ServerService::_seedTopEndpoints() {
    // 5 top endpoints — method, path, requests, avgTime, errorRate.
    struct Row { QString method, path; int requests; double avgMs, errorRate; };
    const QList<Row> rows = {
        {QStringLiteral("GET"),    QStringLiteral("/api/books"),              18234, 18.4, 0.3},
        {QStringLiteral("GET"),    QStringLiteral("/api/books/{id}"),         15021, 22.7, 0.5},
        {QStringLiteral("POST"),   QStringLiteral("/api/cart/checkout"),       8412, 96.1, 1.8},
        {QStringLiteral("GET"),    QStringLiteral("/api/reviews"),            12088, 31.9, 0.7},
        {QStringLiteral("POST"),   QStringLiteral("/api/auth/login"),         9907, 42.3, 2.4},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["method"]    = r.method;
        m["path"]      = r.path;
        m["requests"]  = r.requests;
        m["avgTime"]   = r.avgMs;
        m["errorRate"] = r.errorRate;
        m_topEndpoints.append(m);
    }
}

void ServerService::_seedErrorBreakdown() {
    // 5 error codes — code, label, count, percent, color.
    struct Row { QString code, label, color; int count; double percent; };
    const QList<Row> rows = {
        {QStringLiteral("500"), QStringLiteral("Internal Server Error"), QColor(QStringLiteral("#D93025")).name(), 142, 48.1},
        {QStringLiteral("404"), QStringLiteral("Not Found"),             QColor(QStringLiteral("#F29900")).name(),  78, 26.4},
        {QStringLiteral("401"), QStringLiteral("Unauthorized"),          QColor(QStringLiteral("#1A73E8")).name(),  44, 14.9},
        {QStringLiteral("403"), QStringLiteral("Forbidden"),             QColor(QStringLiteral("#9C27B0")).name(),  21,  7.1},
        {QStringLiteral("503"), QStringLiteral("Service Unavailable"),   QColor(QStringLiteral("#00897B")).name(),  10,  3.5},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["code"]    = r.code;
        m["label"]   = r.label;
        m["count"]   = r.count;
        m["percent"] = r.percent;
        m["color"]   = r.color;
        m_errorBreakdown.append(m);
    }
}

void ServerService::_seedGeographicDistribution() {
    // 6 regions — region, requests, share, latency.
    struct Row { QString region; int requests; double share, latency; };
    const QList<Row> rows = {
        {QStringLiteral("North America"), 38420, 41.2, 28},
        {QStringLiteral("Europe"),        21388, 22.9, 41},
        {QStringLiteral("Asia"),          18904, 20.3, 88},
        {QStringLiteral("South America"),  8412,  9.0, 67},
        {QStringLiteral("Oceania"),        4233,  4.5, 52},
        {QStringLiteral("Africa"),         2104,  2.1, 99},
    };
    for (const auto& r : rows) {
        QVariantMap m;
        m["region"]   = r.region;
        m["requests"] = r.requests;
        m["share"]    = r.share;
        m["latency"]  = r.latency;
        m_geographicDistribution.append(m);
    }
}

} // namespace bookclub::client
