// common/Utils/Logger.cpp
#include <QIODevice>
#include "common/Utils/Logger.h"
#include "common/Utils/FileSystemUtils.h"

#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QMutexLocker>

namespace bookclub::common {

Logger::Logger() = default;

Logger::~Logger()
{
    QMutexLocker locker(&m_mutex);
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

    m_filePath = path;
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: failed to open log file:" << path;
    }
}

void Logger::setMaxFileSizeBytes(qint64 bytes)
{
    QMutexLocker locker(&m_mutex);
    m_maxFileSize = qMax<qint64>(1024, bytes);  // minimum 1 KB
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
            rotateIfNeeded();
        }
    }

    emit newLogEntry(level, message);
}

void Logger::rotateIfNeeded()
{
    // Called with m_mutex already held.
    if (m_filePath.isEmpty() || !m_file.isOpen()) return;

    const qint64 size = m_file.size();
    if (size < m_maxFileSize) return;

    // Close, rename to .1.log (overwriting any previous backup), reopen.
    m_file.close();

    const QString backup = m_filePath + ".1";
    QFile::remove(backup);
    QFile::rename(m_filePath, backup);

    m_file.setFileName(m_filePath);
    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: failed to reopen log file after rotation:" << m_filePath;
    }
}

} // namespace bookclub::common
