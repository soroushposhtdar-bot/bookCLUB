// =============================================================================
//  ServerViewModel.cpp
// =============================================================================
#include "viewmodels/server/ServerViewModel.h"
#include "services/ServerService.h"

namespace bookclub::client {

ServerViewModel::ServerViewModel(QObject* parent) : UserViewModelBase(parent) {}

void ServerViewModel::setServerService(ServerService* s) {
    if (m_service == s) return;
    if (m_service) disconnect(m_service, nullptr, this, nullptr);
    m_service = s;
    if (m_service) {
        connect(m_service, &ServerService::clientsChanged, this, &ServerViewModel::clientsChanged);
        connect(m_service, &ServerService::sessionsChanged, this, &ServerViewModel::sessionsChanged);
        connect(m_service, &ServerService::logsChanged, this, &ServerViewModel::logsChanged);
        connect(m_service, &ServerService::servicesChanged, this, &ServerViewModel::servicesChanged);
    }
    emit serverServiceChanged();
    emit clientsChanged();
    emit sessionsChanged();
    emit logsChanged();
    emit servicesChanged();
}

QVariantList ServerViewModel::clients() const { return m_service ? m_service->clients() : QVariantList{}; }
QVariantList ServerViewModel::sessions() const { return m_service ? m_service->sessions() : QVariantList{}; }
QVariantList ServerViewModel::rooms() const { return m_service ? m_service->rooms() : QVariantList{}; }
QVariantList ServerViewModel::databaseTables() const { return m_service ? m_service->databaseTables() : QVariantList{}; }
QVariantList ServerViewModel::slowQueries() const { return m_service ? m_service->slowQueries() : QVariantList{}; }
QVariantList ServerViewModel::logs() const { return m_service ? m_service->logs() : QVariantList{}; }
QVariantList ServerViewModel::services() const { return m_service ? m_service->services() : QVariantList{}; }
QVariantList ServerViewModel::connectionPool() const { return m_service ? m_service->connectionPool() : QVariantList{}; }
QVariantList ServerViewModel::requestSeries() const { return m_service ? m_service->requestSeries() : QVariantList{}; }
QVariantList ServerViewModel::topEndpoints() const { return m_service ? m_service->topEndpoints() : QVariantList{}; }
QVariantList ServerViewModel::errorBreakdown() const { return m_service ? m_service->errorBreakdown() : QVariantList{}; }
QVariantList ServerViewModel::geographicDistribution() const { return m_service ? m_service->geographicDistribution() : QVariantList{}; }
int ServerViewModel::connectedClientCount() const { return m_service ? m_service->connectedClientCount() : 0; }
int ServerViewModel::activeSessionCount() const { return m_service ? m_service->activeSessionCount() : 0; }
int ServerViewModel::dbQueryRate() const { return m_service ? m_service->dbQueryRate() : 0; }
int ServerViewModel::cpuLoad() const { return m_service ? m_service->cpuLoad() : 0; }
int ServerViewModel::ramUsage() const { return m_service ? m_service->ramUsage() : 0; }
int ServerViewModel::diskUsage() const { return m_service ? m_service->diskUsage() : 0; }

void ServerViewModel::refresh() { if (m_service) m_service->refresh(); beginAsync(400); }
void ServerViewModel::onAsyncReady() { emit clientsChanged(); emit sessionsChanged(); emit logsChanged(); emit servicesChanged(); finishAsync(); }

void ServerViewModel::addLog(const QString& level, const QString& source, const QString& message) {
    if (m_service) m_service->addLog(level, source, message);
}

void ServerViewModel::clearLogs() {
    if (m_service) m_service->clearLogs();
}

void ServerViewModel::disconnectClient(const QString& clientId) {
    if (m_service) m_service->disconnectClient(clientId);
}

void ServerViewModel::terminateSession(const QString& sessionId) {
    if (m_service) m_service->terminateSession(sessionId);
}

QVariantList ServerViewModel::filterLogs(const QString& level, const QString& search) {
    return m_service ? m_service->filterLogs(level, search) : QVariantList{};
}

} // namespace bookclub::client
