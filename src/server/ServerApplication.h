// src/server/ServerApplication.h
#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>

#include "src/server/ServerCore.h"

namespace bookclub::server {

class ServerApplication : public QObject {
    Q_OBJECT
public:
    explicit ServerApplication(QObject* parent = nullptr);
    ~ServerApplication() override;

    // --- Main Entry Point ---
    int run(int argc, char* argv[]);

    // --- Control ---
    void stop();

signals:
    void applicationReady();
    void shutdownRequested();

private slots:
    void onServerStarted(quint16 port);
    void onServerError(const QString& message);
    void onShutdownTimer();

private:
    bool parseArguments(const QStringList& arguments);
    void setupSignalHandlers();

    QCoreApplication* m_app = nullptr;
    ServerCore* m_server = nullptr;
    QTimer* m_shutdownTimer = nullptr;

    // Configuration
    QString m_dbPath = "bookclub.db";
    quint16 m_port = 8080;
    bool m_showHelp = false;
};

} // namespace bookclub::server
