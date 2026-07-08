// common/Utils/Logger.h
#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace bookclub::common {

// Simple thread-safe singleton logger.
// Writes to stderr (qDebug/qWarning/qCritical) always, and additionally
// appends to a log file once setLogFile() has been called.
class Logger : public QObject {
    Q_OBJECT
public:
    static Logger& instance();

    // Sets (or changes) the file that log lines are appended to.
    // Creates the file/parent directories if needed.
    void setLogFile(const QString& path);

    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void debug(const QString& message);

signals:
    void newLogEntry(const QString& level, const QString& message);

private:
    Logger();
    ~Logger() override;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const QString& level, const QString& message);

    QFile m_file;
    QMutex m_mutex;
};

} // namespace bookclub::common

#define LOG_INFO(msg)    bookclub::common::Logger::instance().info(msg)
#define LOG_WARNING(msg) bookclub::common::Logger::instance().warning(msg)
#define LOG_ERROR(msg)   bookclub::common::Logger::instance().error(msg)
#define LOG_DEBUG(msg)   bookclub::common::Logger::instance().debug(msg)
