// src/server/ServerDashboardWindow.h
#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <QProgressBar>

#include "common/Models/ServerHealth.h"

namespace bookclub::server {

class ServerCore;

class ServerDashboardWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ServerDashboardWindow(ServerCore* server, QWidget* parent = nullptr);
    ~ServerDashboardWindow() override;

private slots:
    void updateDashboard();
    void updateConnectionList();
    void updateLogs();
    void clearLogs();

private:
    void setupUi();
    void setupConnections();

    ServerCore* m_server;

    // UI Components
    QLabel* m_statusLabel;
    QLabel* m_connectionsLabel;
    QLabel* m_authenticatedLabel;
    QLabel* m_cpuLabel;
    QLabel* m_ramLabel;
    QProgressBar* m_cpuProgress;
    QProgressBar* m_ramProgress;
    QTableWidget* m_connectionTable;
    QTextEdit* m_logView;

    QTimer* m_updateTimer;
    QTimer* m_connectionTimer;
    QTimer* m_logTimer;
};

} // namespace bookclub::server
