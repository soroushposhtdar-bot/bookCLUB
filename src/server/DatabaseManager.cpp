// src/server/DatabaseManager.cpp
#include "src/server/DatabaseManager.h"
#include <QDebug>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>

namespace bookclub::server {

namespace {
// Tries a list of candidate locations for `database/<file>` relative to the
// working directory, the executable, and the source tree. Returns the first
// absolute path that exists, or an empty string if none matched.
QString locateDatabaseFile(const QString& fileName)
{
    const QStringList candidates = {
        QDir::currentPath() + "/database/" + fileName,
        QDir::currentPath() + "/../database/" + fileName,
        QCoreApplication::applicationDirPath() + "/database/" + fileName,
        QCoreApplication::applicationDirPath() + "/../database/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../database/" + fileName,
        QCoreApplication::applicationDirPath() + "/../../../database/" + fileName,
    };
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return {};
}

// Executes a multi-statement SQL script (comments and semicolon-separated
// statements). Returns true on success. Stops at the first failing statement.
bool executeSqlScript(QSqlDatabase& db, const QString& script)
{
    for (const QString& raw : script.split(';', Qt::SkipEmptyParts)) {
        // Strip SQL line comments starting with --
        QStringList lines;
        for (const QString& line : raw.split('\n')) {
            if (line.trimmed().startsWith("--")) continue;
            lines << line;
        }
        QString stmt = lines.join('\n').trimmed();
        if (stmt.isEmpty()) continue;

        QSqlQuery q(db);
        if (!q.exec(stmt)) {
            qCritical() << "SQL statement failed:" << stmt;
            qCritical() << "Error:" << q.lastError().text();
            return false;
        }
    }
    return true;
}
} // namespace

// ---- Singleton ----
DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

// ---- Constructor & Destructor ----
DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    // Use a named connection so repositories sharing "bookclub_shared" can
    // also access the same underlying SQLite file via QSqlDatabase::database().
    m_db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
}

DatabaseManager::~DatabaseManager() {
    close();
}

// ---- Initialization ----
bool DatabaseManager::initialize(const QString& dbPath) {
    QMutexLocker locker(&m_mutex);

    if (m_initialized && m_db.isOpen()) {
        return true;
    }

    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qCritical() << "Failed to open database:" << m_lastError;
        emit errorOccurred(m_lastError);
        return false;
    }

    // Enable foreign keys for cascading deletes (per schema.sql).
    {
        QSqlQuery pragma(m_db);
        pragma.exec("PRAGMA foreign_keys = ON;");
    }

    if (!runSchemaScript()) {
        qCritical() << "Failed to run schema script";
        return false;
    }

    if (!runSeedScript()) {
        qWarning() << "Seed script failed, but continuing...";
    }

    m_initialized = true;
    qDebug() << "Database initialized successfully:" << dbPath;
    return true;
}

bool DatabaseManager::isOpen() const {
    return m_initialized && m_db.isOpen();
}

void DatabaseManager::close() {
    QMutexLocker locker(&m_mutex);
    if (m_db.isOpen()) {
        m_db.close();
        m_initialized = false;
        qDebug() << "Database closed.";
    }
}

QSqlDatabase DatabaseManager::database() const {
    return m_db;
}

// ---- Query Execution ----
bool DatabaseManager::executeQuery(const QString& query, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    if (!isOpen()) {
        m_lastError = "Database is not open.";
        return false;
    }

    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);

    for (const auto& param : params) {
        sqlQuery.addBindValue(param);
    }

    if (!sqlQuery.exec()) {
        m_lastError = sqlQuery.lastError().text();
        qCritical() << "Query execution failed:" << m_lastError;
        qCritical() << "Query:" << query;
        emit errorOccurred(m_lastError);
        return false;
    }

    return true;
}

QSqlQuery DatabaseManager::executeQueryWithResult(const QString& query, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);
    QSqlQuery sqlQuery(m_db);

    if (!isOpen()) {
        m_lastError = "Database is not open.";
        return sqlQuery;
    }

    sqlQuery.prepare(query);
    for (const auto& param : params) {
        sqlQuery.addBindValue(param);
    }

    if (!sqlQuery.exec()) {
        m_lastError = sqlQuery.lastError().text();
        qCritical() << "Query execution failed:" << m_lastError;
        qCritical() << "Query:" << query;
        emit errorOccurred(m_lastError);
    }

    return sqlQuery;
}

// ---- Transaction Management ----
bool DatabaseManager::beginTransaction() {
    QMutexLocker locker(&m_mutex);
    if (!isOpen()) return false;
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction() {
    QMutexLocker locker(&m_mutex);
    if (!isOpen()) return false;
    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction() {
    QMutexLocker locker(&m_mutex);
    if (!isOpen()) return false;
    return m_db.rollback();
}

// ---- Error Handling ----
QString DatabaseManager::lastError() const {
    return m_lastError;
}

// ---- Private: Schema Script ----
bool DatabaseManager::runSchemaScript() {
    // Skip if tables already exist (e.g. database file already initialised).
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='Users'");
    if (checkQuery.next()) {
        qDebug() << "Tables already exist. Skipping schema creation.";
        return true;
    }

    const QString schemaPath = locateDatabaseFile("schema.sql");
    if (schemaPath.isEmpty()) {
        qCritical() << "Could not locate database/schema.sql in any search path";
        return false;
    }

    QFile schemaFile(schemaPath);
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not open schema.sql at:" << schemaPath;
        return false;
    }

    const QString schemaScript = QString::fromUtf8(schemaFile.readAll());
    schemaFile.close();

    if (!executeSqlScript(m_db, schemaScript)) {
        qCritical() << "Schema script execution failed";
        return false;
    }

    qDebug() << "Schema created successfully from" << schemaPath;
    return true;
}

// ---- Private: Seed Script ----
bool DatabaseManager::runSeedScript() {
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT COUNT(*) FROM Users");
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qDebug() << "Data already exists. Skipping seed.";
        return true;
    }

    const QString seedPath = locateDatabaseFile("seeds/sample_data.sql");
    if (seedPath.isEmpty()) {
        qWarning() << "Could not locate database/seeds/sample_data.sql (seeding skipped)";
        return true;
    }

    QFile seedFile(seedPath);
    if (!seedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open sample_data.sql at:" << seedPath;
        return true;
    }

    const QString seedScript = QString::fromUtf8(seedFile.readAll());
    seedFile.close();

    // Seed script may have multiple INSERTs; we tolerate individual failures
    // (e.g. duplicate rows from a previous partial run) but log them.
    bool anyError = false;
    for (const QString& raw : seedScript.split(';', Qt::SkipEmptyParts)) {
        QStringList lines;
        for (const QString& line : raw.split('\n')) {
            if (line.trimmed().startsWith("--")) continue;
            lines << line;
        }
        QString stmt = lines.join('\n').trimmed();
        if (stmt.isEmpty()) continue;

        QSqlQuery q(m_db);
        if (!q.exec(stmt)) {
            qWarning() << "Seed statement failed:" << q.lastError().text();
            anyError = true;
        }
    }

    if (anyError) {
        qWarning() << "Seed script completed with warnings";
    } else {
        qDebug() << "Seed data inserted successfully";
    }
    return true;
}

} // namespace bookclub::server
