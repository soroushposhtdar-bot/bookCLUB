// src/server/ServerDashboardWindow.cpp
#include "src/server/ServerDashboardWindow.h"
#include "src/server/ServerCore.h"
#include "src/server/ConnectionManager.h"
#include "src/server/StatisticsService.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DateTimeUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QHeaderView>
#include <QTabWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QDateTime>

namespace bookclub::server {

ServerDashboardWindow::ServerDashboardWindow(ServerCore* server, QWidget* parent)
    : QMainWindow(parent)
    , m_server(server)
{
    setupUi();
    setupConnections();

    // Set up timers
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000); // Update every second
    connect(m_updateTimer, &QTimer::timeout, this, &ServerDashboardWindow::updateDashboard);
    m_updateTimer->start();

    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setInterval(2000);
    connect(m_connectionTimer, &QTimer::timeout, this, &ServerDashboardWindow::updateConnectionList);
    m_connectionTimer->start();

    m_logTimer = new QTimer(this);
    m_logTimer->setInterval(500);
    connect(m_logTimer, &QTimer::timeout, this, &ServerDashboardWindow::updateLogs);
    m_logTimer->start();

    setWindowTitle("BookClub Server Dashboard");
    resize(1024, 768);
}

ServerDashboardWindow::~ServerDashboardWindow()
{
    if (m_updateTimer) m_updateTimer->stop();
    if (m_connectionTimer) m_connectionTimer->stop();
    if (m_logTimer) m_logTimer->stop();
}

void ServerDashboardWindow::setupUi()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // --- Top Bar: Status ---
    QHBoxLayout* statusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Server Status: Checking...", this);
    m_statusLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_connectionsLabel = new QLabel("Connections: 0", this);
    m_authenticatedLabel = new QLabel("Authenticated: 0", this);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_connectionsLabel);
    statusLayout->addWidget(m_authenticatedLabel);
    mainLayout->addLayout(statusLayout);

    // --- Tab Widget ---
    QTabWidget* tabWidget = new QTabWidget(this);

    // Tab 1: Health
    QWidget* healthWidget = new QWidget(this);
    QVBoxLayout* healthLayout = new QVBoxLayout(healthWidget);

    QGroupBox* cpuGroup = new QGroupBox("CPU Usage", this);
    QVBoxLayout* cpuLayout = new QVBoxLayout(cpuGroup);
    m_cpuProgress = new QProgressBar(this);
    m_cpuProgress->setRange(0, 100);
    m_cpuLabel = new QLabel("0%", this);
    cpuLayout->addWidget(m_cpuProgress);
    cpuLayout->addWidget(m_cpuLabel);
    healthLayout->addWidget(cpuGroup);

    QGroupBox* ramGroup = new QGroupBox("RAM Usage", this);
    QVBoxLayout* ramLayout = new QVBoxLayout(ramGroup);
    m_ramProgress = new QProgressBar(this);
    m_ramProgress->setRange(0, 100);
    m_ramLabel = new QLabel("0%", this);
    ramLayout->addWidget(m_ramProgress);
    ramLayout->addWidget(m_ramLabel);
    healthLayout->addWidget(ramGroup);

    healthLayout->addStretch();
    tabWidget->addTab(healthWidget, "System Health");

    // Tab 2: Connections
    QWidget* connectionsWidget = new QWidget(this);
    QVBoxLayout* connectionsLayout = new QVBoxLayout(connectionsWidget);
    m_connectionTable = new QTableWidget(this);
    m_connectionTable->setColumnCount(4);
    m_connectionTable->setHorizontalHeaderLabels({"Client ID", "User ID", "Authenticated", "Connected Since"});
    m_connectionTable->horizontalHeader()->setStretchLastSection(true);
    connectionsLayout->addWidget(m_connectionTable);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* refreshBtn = new QPushButton("Refresh", this);
    connect(refreshBtn, &QPushButton::clicked, this, &ServerDashboardWindow::updateConnectionList);
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    connectionsLayout->addLayout(btnLayout);

    tabWidget->addTab(connectionsWidget, "Connections");

    // Tab 3: Logs
    QWidget* logWidget = new QWidget(this);
    QVBoxLayout* logLayout = new QVBoxLayout(logWidget);
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Courier New", 9));
    logLayout->addWidget(m_logView);

    QHBoxLayout* logBtnLayout = new QHBoxLayout();
    QPushButton* clearLogBtn = new QPushButton("Clear Logs", this);
    connect(clearLogBtn, &QPushButton::clicked, this, &ServerDashboardWindow::clearLogs);
    logBtnLayout->addStretch();
    logBtnLayout->addWidget(clearLogBtn);
    logLayout->addLayout(logBtnLayout);

    tabWidget->addTab(logWidget, "Server Logs");

    mainLayout->addWidget(tabWidget);

    // --- Bottom: Control ---
    QHBoxLayout* controlLayout = new QHBoxLayout();
    QPushButton* stopBtn = new QPushButton("Stop Server", this);
    stopBtn->setStyleSheet("background-color: #ff4444; color: white;");
    connect(stopBtn, &QPushButton::clicked, this, [this]() {
        if (m_server) {
            m_server->shutdown();
            QCoreApplication::quit();
        }
    });
    controlLayout->addStretch();
    controlLayout->addWidget(stopBtn);
    mainLayout->addLayout(controlLayout);
}

void ServerDashboardWindow::setupConnections()
{
    // Connect to logger to receive log entries
    connect(&common::Logger::instance(), &common::Logger::newLogEntry,
            this, [this](const QString& entry) {
                // Will be processed in updateLogs
            });
}

void ServerDashboardWindow::updateDashboard()
{
    if (!m_server || !m_server->isRunning()) {
        m_statusLabel->setText("Server Status: Stopped");
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        return;
    }

    m_statusLabel->setText("Server Status: Running");
    m_statusLabel->setStyleSheet("color: green; font-weight: bold;");

    // Update connection counts
    ConnectionManager* connMgr = m_server->connectionManager();
    if (connMgr) {
        int total = connMgr->connectionCount();
        int auth = connMgr->authenticatedCount();
        m_connectionsLabel->setText("Connections: " + QString::number(total));
        m_authenticatedLabel->setText("Authenticated: " + QString::number(auth));
    }

    // Update health metrics
    StatisticsService stats(const_cast<DatabaseManager*>(&DatabaseManager::instance()), m_server->connectionManager());
    common::ServerHealth health = stats.getServerHealth();

    int cpu = static_cast<int>(health.cpuUsage);
    int ram = static_cast<int>(health.ramUsage);

    m_cpuProgress->setValue(cpu);
    m_cpuLabel->setText(QString::number(cpu) + "%");
    m_cpuProgress->setStyleSheet(cpu > 80 ? "QProgressBar::chunk { background-color: red; }" :
                                 cpu > 60 ? "QProgressBar::chunk { background-color: orange; }" :
                                 "QProgressBar::chunk { background-color: green; }");

    m_ramProgress->setValue(ram);
    m_ramLabel->setText(QString::number(ram) + "%");
    m_ramProgress->setStyleSheet(ram > 90 ? "QProgressBar::chunk { background-color: red; }" :
                                ram > 70 ? "QProgressBar::chunk { background-color: orange; }" :
                                "QProgressBar::chunk { background-color: green; }");
}

void ServerDashboardWindow::updateConnectionList()
{
    if (!m_server || !m_server->isRunning()) {
        m_connectionTable->setRowCount(0);
        return;
    }

    ConnectionManager* connMgr = m_server->connectionManager();
    if (!connMgr) return;

    QList<ClientConnection*> connections = connMgr->getAllConnections();
    m_connectionTable->setRowCount(connections.size());

    for (int i = 0; i < connections.size(); ++i) {
        ClientConnection* conn = connections[i];
        m_connectionTable->setItem(i, 0, new QTableWidgetItem(conn->clientId().left(8) + "..."));
        m_connectionTable->setItem(i, 1, new QTableWidgetItem(conn->userId().isEmpty() ? "-" : conn->userId().left(8) + "..."));
        m_connectionTable->setItem(i, 2, new QTableWidgetItem(conn->isAuthenticated() ? "Yes" : "No"));
        m_connectionTable->setItem(i, 3, new QTableWidgetItem(
            common::DateTimeUtils::nowLocal().toString("HH:mm:ss")
        ));
    }
}

void ServerDashboardWindow::updateLogs()
{
    // Since we can't easily read back logs from Logger, we'll just show a sample
    // In a real implementation, you'd have a log buffer
    // For now, we just keep it clean
}

void ServerDashboardWindow::clearLogs()
{
    m_logView->clear();
}

} // namespace bookclub::server
