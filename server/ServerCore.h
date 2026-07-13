#pragma once

#include <QObject>
#include <QHostAddress>
#include <QPointer>
#include <QString>

namespace bookclub::server {

class ConnectionManager;
class RequestRouter;
class DatabaseManager;
class NotificationDispatcher;
class StudySessionManager;
class StatisticsService;
class ThreadPool;

class ServerCore : public QObject {
    Q_OBJECT
public:
    explicit ServerCore(QObject* parent = nullptr);
    ~ServerCore() override = default;

    bool start(const QHostAddress& address, quint16 port);
    void stop();
    bool isRunning() const;
    quint16 port() const;

signals:
    void started();
    void stopped();
    void overloaded(bool overloaded);
    void healthChanged(const QString& state);

private:
    ConnectionManager* m_connectionManager = nullptr;
    RequestRouter* m_requestRouter = nullptr;
    DatabaseManager* m_databaseManager = nullptr;
    NotificationDispatcher* m_notificationDispatcher = nullptr;
    StudySessionManager* m_studySessionManager = nullptr;
    StatisticsService* m_statisticsService = nullptr;
    ThreadPool* m_threadPool = nullptr;
};

} // namespace bookclub::server
