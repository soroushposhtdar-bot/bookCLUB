// common/Interfaces/IUserRepository.cpp
//
// Concrete `IUserRepository` implementation backed by the shared SQLite
// database managed by `DatabaseManager`. Fixes from the previous version:
//   - Shares one named QSqlDatabase connection ("bookclub_shared") instead
//     of opening a private one that bypassed DatabaseManager.
//   - `createUserFromQuery` no longer calls non-const methods on a
//     `const QSqlQuery&` (which broke compilation).
//   - Search now uses the same shared connection.

#include "common/Interfaces/IUserRepository.h"
#include "common/Models/UserAccount.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QVariantList>
#include <QDebug>

namespace bookclub::common {

namespace {
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
class UserRepositoryImpl : public IUserRepository {
public:
    UserRepositoryImpl() = default;
    ~UserRepositoryImpl() override = default;

    bool existsByUsername(const QString& username) const override
    {
        QSqlQuery query = runQuery(
            "SELECT COUNT(*) FROM Users WHERE username = ?",
            {username}
        );
        if (query.next()) {
            return query.value(0).toInt() > 0;
        }
        return false;
    }

    UserAccount* findById(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Users WHERE id = ?", {id});
        return createUserFromQuery(query);
    }

    UserAccount* findByUsername(const QString& username) const override
    {
        QSqlQuery query = runQuery("SELECT * FROM Users WHERE username = ?", {username});
        return createUserFromQuery(query);
    }

    QVector<UserAccount*> findAll() const override
    {
        QVector<UserAccount*> users;
        QSqlQuery query = runQuery("SELECT * FROM Users ORDER BY username");
        while (query.next()) {
            UserAccount* user = createUserFromCurrentRecord(query);
            if (user) users.append(user);
        }
        return users;
    }

    QVector<UserAccount*> search(const QString& keyword) const override
    {
        QVector<UserAccount*> users;
        const QString sql = R"(
            SELECT * FROM Users
            WHERE username LIKE ?
               OR displayName LIKE ?
               OR email LIKE ?
               OR phone LIKE ?
            ORDER BY username
        )";
        const QString pattern = "%" + keyword + "%";
        QSqlQuery query = runQuery(sql, {pattern, pattern, pattern, pattern});
        while (query.next()) {
            UserAccount* user = createUserFromCurrentRecord(query);
            if (user) users.append(user);
        }
        return users;
    }

    bool save(UserAccount* user) override
    {
        if (!user) return false;

        if (user->id().isEmpty()) {
            user->setId(IdGenerator::generateUuid());
        }
        if (!user->createdAt().isValid()) {
            user->setCreatedAt(QDateTime::currentDateTime());
        }
        user->setUpdatedAt(QDateTime::currentDateTime());

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
            LOG_ERROR("Failed to save user: " + user->username());
        }
        return ok;
    }

    bool update(UserAccount* user) override
    {
        if (!user || user->id().isEmpty()) return false;

        const QString sql = R"(
            UPDATE Users SET
                username = ?,
                passwordHash = ?,
                displayName = ?,
                email = ?,
                phone = ?,
                securityQuestion = ?,
                securityAnswerHash = ?,
                status = ?,
                role = ?,
                updatedAt = ?
            WHERE id = ?
        )";

        return execOk(sql, {
            user->username(),
            user->passwordHash(),
            user->displayName(),
            user->email(),
            user->phone(),
            user->securityQuestion(),
            user->securityAnswerHash(),
            static_cast<int>(user->status()),
            static_cast<int>(user->role()),
            QDateTime::currentDateTime(),
            user->id()
        });
    }

    bool remove(const QString& id) override
    {
        return execOk("DELETE FROM Users WHERE id = ?", {id});
    }

    bool blockUser(const QString& id) override
    {
        return setAccountStatus(id, AccountStatus::Blocked);
    }

    bool unblockUser(const QString& id) override
    {
        return setAccountStatus(id, AccountStatus::Active);
    }

    bool setAccountStatus(const QString& id, AccountStatus status) override
    {
        return execOk(
            "UPDATE Users SET status = ?, updatedAt = ? WHERE id = ?",
            {static_cast<int>(status), QDateTime::currentDateTime(), id}
        );
    }

    QDateTime registeredAt(const QString& id) const override
    {
        QSqlQuery query = runQuery("SELECT createdAt FROM Users WHERE id = ?", {id});
        if (query.next()) {
            return query.value(0).toDateTime();
        }
        return QDateTime();
    }

private:
    UserAccount* createUserFromQuery(QSqlQuery& query) const
    {
        if (!query.next()) return nullptr;
        return createUserFromCurrentRecord(query);
    }

    UserAccount* createUserFromCurrentRecord(QSqlQuery& query) const
    {
        QSqlRecord rec = query.record();
        AccountRole role = static_cast<AccountRole>(rec.value("role").toInt());

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
IUserRepository* createUserRepository() {
    static UserRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
