// common/Utils/Logger.h
//
// Thread-safe singleton logger with size-based rotation.
//
// Writes to stderr (qDebug/qWarning/qCritical) always, and additionally
// appends to a log file once setLogFile() has been called. When the log
// file exceeds 10 MB, it is renamed to <name>.1.log and a fresh file is
// started. Only one rotation backup is kept (so max disk usage ≈ 20 MB).
#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

namespace bookclub::common {

class Logger : public QObject {
    Q_OBJECT
public:
    static Logger& instance();

    // Sets (or changes) the file that log lines are appended to.
    // Creates the file/parent directories if needed.
    void setLogFile(const QString& path);

    // Sets the max log file size before rotation (default 10 MB).
    void setMaxFileSizeBytes(qint64 bytes);

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
    void rotateIfNeeded();

    QFile m_file;
    QString m_filePath;
    qint64 m_maxFileSize = 10 * 1024 * 1024;  // 10 MB
    QMutex m_mutex;
};

} // namespace bookclub::common

#define LOG_INFO(msg)    bookclub::common::Logger::instance().info(msg)
#define LOG_WARNING(msg) bookclub::common::Logger::instance().warning(msg)
#define LOG_ERROR(msg)   bookclub::common::Logger::instance().error(msg)
#define LOG_DEBUG(msg)   bookclub::common::Logger::instance().debug(msg)
