// src/server/DatabaseManager.cpp
#include "src/server/DatabaseManager.h"
#include <QDebug>
#include <QSqlRecord>

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
