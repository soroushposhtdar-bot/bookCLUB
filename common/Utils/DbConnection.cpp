// common/Utils/DbConnection.cpp
#include "common/Utils/DbConnection.h"
#include "common/Utils/Logger.h"

#include <QMutexLocker>
#include <QMutex>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>

namespace bookclub::common {

namespace {
QMutex g_mutex;
QString g_lastErrorText;

// Tries a list of candidate locations for the SQLite file relative to the
// working directory and the executable. Returns the first existing path,
// or a default of "bookclub.db" in the CWD if none matched.
QString locateDbFile()
{
    const QStringList candidates = {
        QDir::currentPath() + "/bookclub.db",
        QCoreApplication::applicationDirPath() + "/bookclub.db",
        QCoreApplication::applicationDirPath() + "/../bookclub.db",
    };
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p)) return p;
    }
    return QStringLiteral("bookclub.db");
}
} // namespace

QSqlDatabase DbConnection::database()
{
    QMutexLocker locker(&g_mutex);

    // If DatabaseManager already registered the named connection, reuse it.
    QSqlDatabase db = QSqlDatabase::database(connectionName(), /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }

    // Fallback path: DatabaseManager hasn't been initialised yet.
    // Open the default bookclub.db file under the shared connection name
    // so all subsequent repository calls share the same connection.
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName());
    db.setDatabaseName(locateDbFile());
    if (!db.open()) {
        g_lastErrorText = db.lastError().text();
        LOG_ERROR("DbConnection: failed to open fallback bookclub.db: " + g_lastErrorText);
    }
    return db;
}

QSqlQuery DbConnection::run(const QString& sql, const QVariantList& params)
{
    QSqlQuery query(database());
    query.prepare(sql);
    for (const auto& p : params) {
        query.addBindValue(p);
    }
    if (!query.exec()) {
        g_lastErrorText = query.lastError().text();
        LOG_WARNING("SQL failed: " + query.lastError().text() + " | stmt: " + sql.left(120));
    }
    return query;
}

bool DbConnection::execOk(const QString& sql, const QVariantList& params)
{
    QSqlQuery q = run(sql, params);
    return q.lastError().type() == QSqlError::NoError;
}

QString DbConnection::lastErrorText()
{
    return g_lastErrorText;
}

} // namespace bookclub::common
