// src/server/DatabaseManager.cpp
#include "src/server/DatabaseManager.h"
#include <QDebug>
#include <QSqlRecord>
#include <QFile>
#include <QTextStream>
namespace bookclub::server {

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

DatabaseManager::~DatabaseManager() {
    close();
}

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

    // ---- ۱. اجرای اسکریپت ایجاد جداول (اجباری) ----
    if (!runSchemaScript()) {
        qCritical() << "Failed to run schema script";
        return false;
    }

    // ---- ۲. اجرای اسکریپت داده‌های نمونه (اختیاری) ----
    if (!runSeedScript()) {
        qWarning() << "Seed script failed, but continuing...";
        // خطای seed را نادیده می‌گیریم تا سرور متوقف نشود
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

QString DatabaseManager::lastError() const {
    return m_lastError;
}

} // namespace bookclub::server
bool DatabaseManager::runSchemaScript() {
    // بررسی اینکه آیا جداول از قبل وجود دارند
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='Users'");
    if (checkQuery.next()) {
        // جداول وجود دارند، نیازی به اجرای مجدد نیست
        qDebug() << "Tables already exist. Skipping schema creation.";
        return true;
    }

    // خواندن فایل schema.sql
    QFile schemaFile("database/schema.sql");
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Could not open schema.sql file";
        return false;
    }

    QString schemaScript = QString::fromUtf8(schemaFile.readAll());
    schemaFile.close();

    // اجرای اسکریپت (تقسیم به کوئری‌های جداگانه با جداکننده ';')
    QStringList queries = schemaScript.split(';', Qt::SkipEmptyParts);
    for (const QString& query : queries) {
        QString trimmedQuery = query.trimmed();
        if (trimmedQuery.isEmpty()) continue;

        QSqlQuery sqlQuery(m_db);
        if (!sqlQuery.exec(trimmedQuery)) {
            qCritical() << "Failed to execute query:" << trimmedQuery;
            qCritical() << "Error:" << sqlQuery.lastError().text();
            return false;
        }
    }

    qDebug() << "Schema created successfully";
    return true;
}

bool DatabaseManager::runSeedScript() {
    // بررسی اینکه آیا داده‌های نمونه قبلاً وارد شده‌اند
    QSqlQuery checkQuery(m_db);
    checkQuery.exec("SELECT COUNT(*) FROM Users");
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qDebug() << "Data already exists. Skipping seed.";
        return true;
    }

    QFile seedFile("database/seeds/sample_data.sql");
    if (!seedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open sample_data.sql file (seeding skipped)";
        return true; // عدم وجود فایل نمونه خطا نیست
    }

    QString seedScript = QString::fromUtf8(seedFile.readAll());
    seedFile.close();

    QStringList queries = seedScript.split(';', Qt::SkipEmptyParts);
    for (const QString& query : queries) {
        QString trimmedQuery = query.trimmed();
        if (trimmedQuery.isEmpty()) continue;

        QSqlQuery sqlQuery(m_db);
        if (!sqlQuery.exec(trimmedQuery)) {
            qWarning() << "Failed to execute seed query:" << trimmedQuery;
            qWarning() << "Error:" << sqlQuery.lastError().text();
            // خطای seed را نادیده می‌گیریم تا برنامه متوقف نشود
        }
    }

    qDebug() << "Seed data inserted successfully";
    return true;
}
