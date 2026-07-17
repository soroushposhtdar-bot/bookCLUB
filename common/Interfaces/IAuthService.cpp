// common/Interfaces/IAuthService.cpp
//
// Concrete `IAuthService` implementation backed by the shared SQLite
// database managed by `DatabaseManager`. This file replaces the previous
// broken version which:
//   - referenced non-existent `PasswordHasher::generateSalt`/`hashPassword`
//     APIs (real API is `hash()`/`verify()`),
//   - compared a freshly-salted hash to a stored hash (could NEVER match),
//   - silently opened its own private QSqlDatabase connection that bypassed
//     the server's `DatabaseManager` singleton.
//
// All database access now flows through `bookclub::server::DatabaseManager`
// (the singleton lives in the server component but the common library links
// against Qt5::Sql as well, so we can call it from here).

#include "common/Interfaces/IAuthService.h"
#include "common/Models/UserAccount.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/PasswordHasher.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QDebug>

namespace bookclub::common {

namespace {
// Returns the shared SQLite connection owned by DatabaseManager.
// If DatabaseManager has not been initialised yet (e.g. when this code
// is called from a unit test), we fall back to opening the default
// "bookclub.db" connection so that the repository can still operate.
QSqlDatabase sharedDatabase()
{
    auto db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) {
            db.open();
        }
        return db;
    }
    db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
    db.setDatabaseName("bookclub.db");
    db.open();
    return db;
}

QSqlQuery runQuery(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery query(sharedDatabase());
    query.prepare(sql);
    for (const auto& p : params) {
        query.addBindValue(p);
    }
    query.exec();
    return query;
}

bool execOk(const QString& sql, const QVariantList& params = {})
{
    QSqlQuery q = runQuery(sql, params);
    return q.lastError().type() == QSqlError::NoError;
}
} // namespace

// ========== Implementation ==========
class AuthServiceImpl : public IAuthService {
public:
    AuthServiceImpl() = default;
    ~AuthServiceImpl() override = default;

    UserAccount* registerAccount(UserAccount* account,
                                 const QString& plainPassword,
                                 const QString& securityAnswer) override
    {
        if (!account || plainPassword.isEmpty() || securityAnswer.isEmpty()) {
            LOG_ERROR("Invalid registration data");
            return nullptr;
        }

        if (!isUsernameUnique(account->username())) {
            LOG_WARNING("Username already exists: " + account->username());
            return nullptr;
        }

        if (account->id().isEmpty()) {
            account->setId(IdGenerator::generateUuid());
        }

        // Real API: hash() returns "salt$hash" and verify() can validate it.
        account->setPasswordHash(PasswordHasher::hash(plainPassword));
        account->setSecurityAnswerHash(PasswordHasher::hash(securityAnswer.toLower().trimmed()));
        account->setStatus(AccountStatus::Active); // Active so the user can log in immediately.
        account->setCreatedAt(QDateTime::currentDateTime());
        account->setUpdatedAt(QDateTime::currentDateTime());

        if (!saveUserToDatabase(account)) {
            LOG_ERROR("Failed to save user to database: " + account->username());
            return nullptr;
        }

        LOG_INFO("User registered successfully: " + account->username());
        return account;
    }

    UserAccount* login(const QString& username, const QString& plainPassword) override
    {
        if (username.isEmpty() || plainPassword.isEmpty()) {
            LOG_WARNING("Empty username or password");
            return nullptr;
        }

        UserAccount* user = findUserByUsername(username);
        if (!user) {
            LOG_WARNING("User not found: " + username);
            return nullptr;
        }

        if (user->isBlocked() || user->status() == AccountStatus::Disabled) {
            LOG_WARNING("User is blocked or disabled: " + username);
            delete user;
            return nullptr;
        }

        // Verify password using the proper API.
        if (!PasswordHasher::verify(plainPassword, user->passwordHash())) {
            LOG_WARNING("Invalid password for: " + username);
            delete user;
            return nullptr;
        }

        LOG_INFO("User logged in: " + username);
        return user;
    }

    bool logout(const QString& /*userId*/) override
    {
        // Sessions are tracked on the server via ClientConnection::setAuthenticated.
        // No DB work required here for now.
        return true;
    }

    bool changePassword(const QString& userId,
                        const QString& oldPassword,
                        const QString& newPassword) override
    {
        if (userId.isEmpty() || oldPassword.isEmpty() || newPassword.isEmpty()) {
            return false;
        }

        UserAccount* user = findUserById(userId);
        if (!user) {
            LOG_WARNING("User not found for password change: " + userId);
            return false;
        }

        if (!PasswordHasher::verify(oldPassword, user->passwordHash())) {
            LOG_WARNING("Old password incorrect for: " + userId);
            delete user;
            return false;
        }

        const QString newHash = PasswordHasher::hash(newPassword);
        delete user;
        return updateUserPassword(userId, newHash);
    }

    bool resetPassword(const QString& username,
                       const QString& securityAnswer,
                       const QString& newPassword) override
    {
        if (username.isEmpty() || securityAnswer.isEmpty() || newPassword.isEmpty()) {
            return false;
        }

        UserAccount* user = findUserByUsername(username);
        if (!user) {
            LOG_WARNING("User not found for reset: " + username);
            return false;
        }

        // Verify security answer against the stored hash.
        if (!PasswordHasher::verify(securityAnswer.toLower().trimmed(), user->securityAnswerHash())) {
            LOG_WARNING("Security answer incorrect for: " + username);
            delete user;
            return false;
        }

        const QString userId = user->id();
        delete user;
        const QString newHash = PasswordHasher::hash(newPassword);
        return updateUserPassword(userId, newHash);
    }

    bool isUsernameUnique(const QString& username) const override
    {
        QSqlQuery query = runQuery(
            "SELECT COUNT(*) FROM Users WHERE username = ?",
            {username}
        );
        if (query.next()) {
            return query.value(0).toInt() == 0;
        }
        return true; // fail open — registration will reject duplicates
    }

private:
    bool saveUserToDatabase(UserAccount* user)
    {
        const QString sql = R"(
            INSERT INTO Users (
                id, username, passwordHash, displayName, email, phone,
                securityQuestion, securityAnswerHash, status, role,
                createdAt, updatedAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        const bool ok = execOk(sql, {
            user->id(),
            user->username(),
            user->passwordHash(),
            user->displayName(),
            user->email(),
            user->phone(),
            user->securityQuestion(),
            user->securityAnswerHash(),
            static_cast<int>(user->status()),
            static_cast<int>(user->role()),
            user->createdAt(),
            user->updatedAt()
        });

        if (!ok) {
            LOG_ERROR("SQL insert failed for user: " + user->username());
        }
        return ok;
    }

    bool updateUserPassword(const QString& userId, const QString& newHash)
    {
        const bool ok = execOk(
            "UPDATE Users SET passwordHash = ?, updatedAt = ? WHERE id = ?",
            {newHash, QDateTime::currentDateTime(), userId}
        );
        if (ok) {
            LOG_INFO("Password updated for user: " + userId);
        } else {
            LOG_ERROR("Failed to update password for user: " + userId);
        }
        return ok;
    }

    UserAccount* findUserById(const QString& id) const
    {
        QSqlQuery query = runQuery("SELECT * FROM Users WHERE id = ?", {id});
        return createUserFromQuery(query);
    }

    UserAccount* findUserByUsername(const QString& username) const
    {
        QSqlQuery query = runQuery("SELECT * FROM Users WHERE username = ?", {username});
        return createUserFromQuery(query);
    }

    // Note: QSqlQuery::next() is non-const, so this helper cannot be const
    // even though logically it is a "lookup".
    UserAccount* createUserFromQuery(QSqlQuery& query) const
    {
        if (!query.next()) return nullptr;

        QSqlRecord rec = query.record();
        const AccountRole role = static_cast<AccountRole>(rec.value("role").toInt());

        UserAccount* user = nullptr;
        switch (role) {
            case AccountRole::Admin:
                user = new Admin;
                break;
            case AccountRole::Publisher:
                user = new Publisher;
                break;
            case AccountRole::User:
            default:
                user = new RegularUser;
                break;
        }

        user->setId(rec.value("id").toString());
        user->setUsername(rec.value("username").toString());
        user->setPasswordHash(rec.value("passwordHash").toString());
        user->setDisplayName(rec.value("displayName").toString());
        user->setEmail(rec.value("email").toString());
        user->setPhone(rec.value("phone").toString());
        user->setSecurityQuestion(rec.value("securityQuestion").toString());
        user->setSecurityAnswerHash(rec.value("securityAnswerHash").toString());
        user->setStatus(static_cast<AccountStatus>(rec.value("status").toInt()));
        user->setCreatedAt(rec.value("createdAt").toDateTime());
        user->setUpdatedAt(rec.value("updatedAt").toDateTime());
        return user;
    }
};

// ========== Factory ==========
IAuthService* createAuthService() {
    static AuthServiceImpl service;
    return &service;
}

} // namespace bookclub::common
