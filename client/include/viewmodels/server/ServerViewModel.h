// =============================================================================
//  ServerViewModel.h
// =============================================================================
#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {
class ServerService;

class ServerViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(ServerService* serverService READ serverService WRITE setServerService NOTIFY serverServiceChanged)
    Q_PROPERTY(QVariantList clients READ clients NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(QVariantList rooms READ rooms NOTIFY sessionsChanged)
    Q_PROPERTY(QVariantList databaseTables READ databaseTables NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList slowQueries READ slowQueries NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QVariantList services READ services NOTIFY servicesChanged)
    Q_PROPERTY(QVariantList connectionPool READ connectionPool NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList requestSeries READ requestSeries NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList topEndpoints READ topEndpoints NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList errorBreakdown READ errorBreakdown NOTIFY clientsChanged)
    Q_PROPERTY(QVariantList geographicDistribution READ geographicDistribution NOTIFY clientsChanged)
    Q_PROPERTY(int connectedClientCount READ connectedClientCount NOTIFY clientsChanged)
    Q_PROPERTY(int activeSessionCount READ activeSessionCount NOTIFY sessionsChanged)
    Q_PROPERTY(int dbQueryRate READ dbQueryRate NOTIFY clientsChanged)
    Q_PROPERTY(int cpuLoad READ cpuLoad NOTIFY clientsChanged)
    Q_PROPERTY(int ramUsage READ ramUsage NOTIFY clientsChanged)
    Q_PROPERTY(int diskUsage READ diskUsage NOTIFY clientsChanged)
    Q_PROPERTY(bool loading READ isBusy NOTIFY isBusyChanged)

public:
    explicit ServerViewModel(QObject* parent = nullptr);
    ServerService* serverService() const { return m_service; }
    void setServerService(ServerService* s);

    QVariantList clients() const;
    QVariantList sessions() const;
    QVariantList rooms() const;
    QVariantList databaseTables() const;
    QVariantList slowQueries() const;
    QVariantList logs() const;
    QVariantList services() const;
    QVariantList connectionPool() const;
    QVariantList requestSeries() const;
    QVariantList topEndpoints() const;
    QVariantList errorBreakdown() const;
    QVariantList geographicDistribution() const;
    int connectedClientCount() const;
    int activeSessionCount() const;
    int dbQueryRate() const;
    int cpuLoad() const;
    int ramUsage() const;
    int diskUsage() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void addLog(const QString& level, const QString& source, const QString& message);
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void disconnectClient(const QString& clientId);
    Q_INVOKABLE void terminateSession(const QString& sessionId);
    Q_INVOKABLE QVariantList filterLogs(const QString& level, const QString& search);

signals:
    void serverServiceChanged();
    void clientsChanged();
    void sessionsChanged();
    void logsChanged();
    void servicesChanged();

protected:
    void onAsyncReady() override;

private:
    ServerService* m_service = nullptr;
};

} // namespace bookclub::client
