// common/Utils/Logger.cpp
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QMutexLocker>

namespace bookclub::common {

Logger::Logger() = default;

Logger::~Logger()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::setLogFile(const QString& path)
{
    QMutexLocker locker(&m_mutex);

    if (m_file.isOpen()) {
        m_file.close();
    }

    QString dir = QFileInfo(path).absolutePath();
    FileSystemUtils::ensureDirectoryExists(dir);

    m_file.setFileName(path);
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: failed to open log file:" << path;
    }
}

void Logger::info(const QString& message)    { log("INFO", message); }
void Logger::warning(const QString& message) { log("WARN", message); }
void Logger::error(const QString& message)   { log("ERROR", message); }
void Logger::debug(const QString& message)   { log("DEBUG", message); }

void Logger::log(const QString& level, const QString& message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    const QString line = QString("[%1] [%2] %3").arg(timestamp, level, message);

    // Always echo to the console.
    if (level == "ERROR") {
        qCritical().noquote() << line;
    } else if (level == "WARN") {
        qWarning().noquote() << line;
    } else {
        qDebug().noquote() << line;
    }

    // Append to file if one was configured.
    {
        QMutexLocker locker(&m_mutex);
        if (m_file.isOpen()) {
            QTextStream stream(&m_file);
            stream << line << '\n';
            stream.flush();
        }
    }

    emit newLogEntry(level, message);
}

} // namespace bookclub::common
