// common/Utils/DbConnection.h
//
// Centralised access to the shared SQLite connection.
//
// All repositories and services in `bookclub::common` MUST go through
// `DbConnection::database()` instead of calling `QSqlDatabase::addDatabase`
// themselves. The connection is created exactly once by
// `bookclub::server::DatabaseManager::initialize()` and registered under
// the name `bookclub_shared`.
//
// If `DatabaseManager` has not been initialised yet (e.g. when this code is
// called from a unit test that does not boot the full server), `database()`
// falls back to opening `bookclub.db` from the current working directory
// so that repository operations still work.
#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariantList>

namespace bookclub::common {

class DbConnection {
public:
    // The canonical name of the shared SQLite connection.
    static const char* connectionName() { return "bookclub_shared"; }

    // Returns the shared database connection. Opens it if needed.
    // Never re-adds the connection — DatabaseManager owns registration.
    static QSqlDatabase database();

    // Prepare + bind + exec in one call. Returns the executed query.
    static QSqlQuery run(const QString& sql, const QVariantList& params = {});

    // Convenience: returns true iff the executed query had no error.
    static bool execOk(const QString& sql, const QVariantList& params = {});

    // Returns the last QSqlError text from the most recent run() call on
    // this thread. Useful for logging inside a repository method.
    static QString lastErrorText();
};

} // namespace bookclub::common
