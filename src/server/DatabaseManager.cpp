// src/server/DatabaseManager.cpp
#include <QIODevice>
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
    QStringList candidates;

    // 1. Compile-time source directory (most reliable — works regardless
    //    of the working directory or build output location).
#ifdef BOOKCLUB_SOURCE_DIR
    candidates << QString(BOOKCLUB_SOURCE_DIR) + "/database/" + fileName;
#endif

    // 2. Working directory + relative paths.
    candidates << QDir::currentPath() + "/database/" + fileName;
    candidates << QDir::currentPath() + "/../database/" + fileName;

    // 3. Executable directory + relative paths.
    candidates << QCoreApplication::applicationDirPath() + "/database/" + fileName;
    candidates << QCoreApplication::applicationDirPath() + "/../database/" + fileName;
    candidates << QCoreApplication::applicationDirPath() + "/../../database/" + fileName;
    candidates << QCoreApplication::applicationDirPath() + "/../../../database/" + fileName;

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

    // Set WAL journal mode BEFORE anything else — must not be inside a transaction.
    {
        QSqlQuery wal(m_db);
        if (!wal.exec("PRAGMA journal_mode = WAL;")) {
            qWarning() << "Failed to set WAL mode:" << wal.lastError().text();
        }
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
    // Check if the DB already has the correct schema version.
    // We use SQLite's built-in PRAGMA user_version (a 32-bit integer
    // stored in the DB header) to track schema versions.
    //
    // Version 0 = empty DB or old schema (pre-versioning)
    // Version 1 = schema with 21 tables, 65 indexes
    // Version 2 = added DiscountCodes table + index
    //
    // If the version doesn't match, we delete the old DB file and
    // recreate it from scratch. This is the simplest migration
    // strategy for a course project — no ALTER TABLE needed.
    const int EXPECTED_SCHEMA_VERSION = 2;

    QSqlQuery versionQuery(m_db);
    versionQuery.exec("PRAGMA user_version");
    int currentVersion = 0;
    if (versionQuery.next()) {
        currentVersion = versionQuery.value(0).toInt();
    }

    if (currentVersion == EXPECTED_SCHEMA_VERSION) {
        qDebug() << "Schema version" << currentVersion << "is up to date. Skipping schema creation.";
        return true;
    }

    if (currentVersion > 0 && currentVersion < EXPECTED_SCHEMA_VERSION) {
        qWarning() << "Old schema version" << currentVersion << "detected. Rebuilding database from scratch.";
        // Close, delete, and reopen.
        m_db.close();
        QFile::remove(m_db.databaseName());
        if (!m_db.open()) {
            qCritical() << "Failed to reopen database after deletion:" << m_db.lastError().text();
            return false;
        }
        // Re-set WAL mode and foreign keys after reopening
        {
            QSqlQuery wal(m_db);
            wal.exec("PRAGMA journal_mode = WAL;");
        }
        {
            QSqlQuery pragma(m_db);
            pragma.exec("PRAGMA foreign_keys = ON;");
        }
    }

    const QString schemaPath = locateDatabaseFile("schema.sql");
    if (schemaPath.isEmpty()) {
        qCritical() << "Could not locate database/schema.sql in any search path."
                    << " Searched:"
#ifdef BOOKCLUB_SOURCE_DIR
                    << "\n  BOOKCLUB_SOURCE_DIR:" << QString(BOOKCLUB_SOURCE_DIR) + "/database/schema.sql"
#endif
                    << "\n  CWD:" << QDir::currentPath() + "/database/schema.sql"
                    << "\n  ExeDir:" << QCoreApplication::applicationDirPath() + "/database/schema.sql";
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

    // Stamp the DB with the current schema version.
    QSqlQuery setVersion(m_db);
    setVersion.exec(QString("PRAGMA user_version = %1").arg(EXPECTED_SCHEMA_VERSION));

    qDebug() << "Schema created successfully (version" << EXPECTED_SCHEMA_VERSION << ") from" << schemaPath;
    return true;
}

// ---- Private: Seed Script ----
bool DatabaseManager::runSeedScript() {
    // Skip seeding if the Users table already has data (the DB was
    // already seeded in a previous run with the same schema version).
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT COUNT(*) FROM Users");
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qDebug() << "Data already exists (" << checkQuery.value(0).toInt()
                 << "users). Skipping seed.";
        return true;
    }

    const QString seedPath = locateDatabaseFile("seeds/sample_data.sql");
    if (seedPath.isEmpty()) {
        qWarning() << "Could not locate database/seeds/sample_data.sql (seeding skipped)"
                   << " Searched:"
#ifdef BOOKCLUB_SOURCE_DIR
                   << "\n  BOOKCLUB_SOURCE_DIR:" << QString(BOOKCLUB_SOURCE_DIR) + "/database/seeds/sample_data.sql"
#endif
                   << "\n  CWD:" << QDir::currentPath() + "/database/seeds/sample_data.sql"
                   << "\n  ExeDir:" << QCoreApplication::applicationDirPath() + "/database/seeds/sample_data.sql";
        return true;
    }

    QFile seedFile(seedPath);
    if (!seedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open sample_data.sql at:" << seedPath;
        return true;
    }

    const QString seedScript = QString::fromUtf8(seedFile.readAll());
    seedFile.close();

    qDebug() << "Running seed script from:" << seedPath;

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
            qWarning() << "Seed statement failed:" << q.lastError().text()
                       << "| SQL:" << stmt.left(80);
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
