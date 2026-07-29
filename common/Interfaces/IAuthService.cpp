// common/Interfaces/IAuthService.cpp
//
// SQLite-backed IAuthService implementation.
// Uses common::DbConnection for all DB access (no private connections).
//
// Responsibilities:
//   - registerAccount:  validate uniqueness, hash password + answer, persist
//   - login:             look up user, verify password, enforce status checks
//   - logout:            server-side session teardown (no DB state today)
//   - changePassword:    verify old, hash new, update
//   - resetPassword:     verify security answer, hash new, update
//   - isUsernameUnique:  case-insensitive check
#include "common/Interfaces/IAuthService.h"
#include "common/Models/UserAccount.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/PasswordHasher.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace bookclub::common {

namespace {
// Re-hydrate a UserAccount (or subclass) from the current record of `query`.
// Mirrors the helper in IUserRepository.cpp but kept local to avoid coupling.
UserAccount* userFromCurrentRecord(QSqlQuery& query)
{
    QSqlRecord rec = query.record();
    const AccountRole role = static_cast<AccountRole>(rec.value("role").toInt());

    UserAccount* user = nullptr;
    switch (role) {
        case AccountRole::Admin:     user = new Admin;     break;
        case AccountRole::Publisher: user = new Publisher; break;
        case AccountRole::User:
        default:                     user = new RegularUser; break;
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
} // namespace

// ============== Implementation ==============
class AuthServiceImpl : public IAuthService {
public:
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

        account->setPasswordHash(PasswordHasher::hash(plainPassword));
        account->setSecurityAnswerHash(PasswordHasher::hash(securityAnswer.toLower().trimmed()));
        account->setStatus(AccountStatus::Active);
        account->setCreatedAt(QDateTime::currentDateTime());
        account->setUpdatedAt(QDateTime::currentDateTime());

        if (!saveUserToDatabase(account)) {
            LOG_ERROR("Failed to save user to database: " + account->username()
                      + " | " + DbConnection::lastErrorText());
            return nullptr;
        }

        LOG_INFO("User registered successfully: " + account->username()
                 + " (role=" + account->roleName() + ")");
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
            LOG_WARNING("Login failed — user not found: " + username);
            return nullptr;
        }

        if (user->isBlocked()) {
            LOG_WARNING("Login denied — user is blocked: " + username);
            delete user;
            return nullptr;
        }
        if (user->status() == AccountStatus::Disabled
            || user->status() == AccountStatus::Deleted) {
            LOG_WARNING("Login denied — account disabled/deleted: " + username);
            delete user;
            return nullptr;
        }

        if (!PasswordHasher::verify(plainPassword, user->passwordHash())) {
            LOG_WARNING("Login failed — invalid password for: " + username);
            delete user;
            return nullptr;
        }

        LOG_INFO("User logged in: " + username);
        return user;
    }

    bool logout(const QString& /*userId*/) override
    {
        // Sessions are tracked on the server via ClientConnection::setAuthenticated.
        // No DB work required here for now — a future Sessions table could be
        // updated here.
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

        if (!PasswordHasher::verify(securityAnswer.toLower().trimmed(),
                                     user->securityAnswerHash())) {
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
        auto q = DbConnection::run(
            "SELECT COUNT(*) FROM Users WHERE username = ? COLLATE NOCASE",
            {username}
        );
        return q.next() ? q.value(0).toInt() == 0 : true;
    }

private:
    bool saveUserToDatabase(UserAccount* user)
    {
        QString favoriteGenresJson;
        if (auto* regular = dynamic_cast<RegularUser*>(user)) {
            favoriteGenresJson = QString::fromUtf8(
                QJsonDocument(QJsonArray::fromStringList(regular->favoriteGenreIds()))
                    .toJson(QJsonDocument::Compact)
            );
        }

        const QString sql = QStringLiteral(
            "INSERT INTO Users ("
            "  id, username, passwordHash, displayName, email, phone,"
            "  securityQuestion, securityAnswerHash, status, role, favoriteGenreIds,"
            "  createdAt, updatedAt"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

        const bool ok = DbConnection::execOk(sql, {
            user->id(), user->username(), user->passwordHash(),
            user->displayName(), user->email(), user->phone(),
            user->securityQuestion(), user->securityAnswerHash(),
            static_cast<int>(user->status()),
            static_cast<int>(user->role()),
            favoriteGenresJson,
            user->createdAt(), user->updatedAt()
        });

        if (!ok) {
            LOG_ERROR("SQL insert failed for user: " + user->username()
                      + " | " + DbConnection::lastErrorText());
            return false;
        }

        // Persist publisher profile.
        if (auto* publisher = dynamic_cast<Publisher*>(user)) {
            DbConnection::execOk(
                "INSERT OR REPLACE INTO Publishers "
                "(userId, publisherName, biography, website, taxId, approved) "
                "VALUES (?, ?, ?, ?, ?, 1)",
                {user->id(), publisher->publisherName(), publisher->biography(),
                 publisher->website(), publisher->taxId()}
            );
        }
        return true;
    }

    bool updateUserPassword(const QString& userId, const QString& newHash)
    {
        const bool ok = DbConnection::execOk(
            "UPDATE Users SET passwordHash = ?, updatedAt = ? WHERE id = ?",
            {newHash, QDateTime::currentDateTime(), userId}
        );
        if (ok) LOG_INFO("Password updated for user: " + userId);
        else    LOG_ERROR("Failed to update password for user: " + userId);
        return ok;
    }

    UserAccount* findUserById(const QString& id) const
    {
        auto q = DbConnection::run("SELECT * FROM Users WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return userFromCurrentRecord(q);
    }

    UserAccount* findUserByUsername(const QString& username) const
    {
        auto q = DbConnection::run(
            "SELECT * FROM Users WHERE username = ? COLLATE NOCASE",
            {username}
        );
        if (!q.next()) return nullptr;
        return userFromCurrentRecord(q);
    }
};

// ============== Factory ==============
IAuthService* createAuthService() {
    static AuthServiceImpl service;
    return &service;
}

} // namespace bookclub::common
