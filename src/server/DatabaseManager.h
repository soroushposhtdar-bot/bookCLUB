// src/server/DatabaseManager.h
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariantList>
#include <QMutex>

namespace bookclub::server {

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance();

    // --- Initialization ---
    bool initialize(const QString& dbPath);
    bool isOpen() const;
    void close();
    QSqlDatabase database() const;

    // --- Query Execution ---
    bool executeQuery(const QString& query, const QVariantList& params = {});
    QSqlQuery executeQueryWithResult(const QString& query, const QVariantList& params = {});

    // --- Transaction Management ---
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // --- Error Handling ---
    QString lastError() const;

signals:
    void errorOccurred(const QString& message);

private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_db;
    QString m_lastError;
    QMutex m_mutex;
    bool m_initialized = false;
};

} // namespace bookclub::server
