// common/Interfaces/IUserRepository.cpp
//
// SQLite-backed implementation of IUserRepository.
// Uses the shared `bookclub_shared` connection via common::DbConnection.
//
// Full CRUD: save (INSERT), update, findById, findByUsername, findAll,
// search, remove, blockUser, unblockUser, setAccountStatus, registeredAt.
#include "common/Interfaces/IUserRepository.h"
#include "common/Models/UserAccount.h"
#include "common/Models/Admin.h"
#include "common/Models/Publisher.h"
#include "common/Models/RegularUser.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DbConnection.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace bookclub::common {

namespace {
QStringList parseJsonStringArray(const QString& json)
{
    QStringList out;
    if (json.isEmpty()) return out;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return out;
    for (const auto& v : doc.array()) out.append(v.toString());
    return out;
}

QString toJsonStringArray(const QStringList& list)
{
    return QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(list)).toJson(QJsonDocument::Compact)
    );
}

// Build a UserAccount (or subclass) from the current record of `query`.
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

    if (auto* regular = dynamic_cast<RegularUser*>(user)) {
        regular->setFavoriteGenreIds(parseJsonStringArray(rec.value("favoriteGenreIds").toString()));
    }
    if (auto* publisher = dynamic_cast<Publisher*>(user)) {
        // Load publisher profile from Publishers table (best-effort).
        QSqlQuery pubQuery = DbConnection::run(
            "SELECT publisherName, biography, website, taxId FROM Publishers WHERE userId = ?",
            {user->id()}
        );
        if (pubQuery.next()) {
            publisher->setPublisherName(pubQuery.value(0).toString());
            publisher->setBiography(pubQuery.value(1).toString());
            publisher->setWebsite(pubQuery.value(2).toString());
            publisher->setTaxId(pubQuery.value(3).toString());
        }
    }
    return user;
}
} // namespace

// ============== Implementation ==============
class UserRepositoryImpl : public IUserRepository {
public:
    bool existsByUsername(const QString& username) const override
    {
        auto q = DbConnection::run(
            "SELECT COUNT(*) FROM Users WHERE username = ? COLLATE NOCASE",
            {username}
        );
        return q.next() && q.value(0).toInt() > 0;
    }

    UserAccount* findById(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT * FROM Users WHERE id = ?", {id});
        if (!q.next()) return nullptr;
        return userFromCurrentRecord(q);
    }

    UserAccount* findByUsername(const QString& username) const override
    {
        auto q = DbConnection::run(
            "SELECT * FROM Users WHERE username = ? COLLATE NOCASE",
            {username}
        );
        if (!q.next()) return nullptr;
        return userFromCurrentRecord(q);
    }

    QVector<UserAccount*> findAll() const override
    {
        QVector<UserAccount*> users;
        auto q = DbConnection::run("SELECT * FROM Users ORDER BY createdAt DESC");
        while (q.next()) {
            if (auto* u = userFromCurrentRecord(q)) users.append(u);
        }
        return users;
    }

    QVector<UserAccount*> search(const QString& keyword) const override
    {
        QVector<UserAccount*> users;
        const QString pattern = "%" + keyword + "%";
        const QString sql = QStringLiteral(
            "SELECT * FROM Users "
            "WHERE username LIKE ? OR displayName LIKE ? OR email LIKE ? OR phone LIKE ? "
            "ORDER BY username"
        );
        auto q = DbConnection::run(sql, {pattern, pattern, pattern, pattern});
        while (q.next()) {
            if (auto* u = userFromCurrentRecord(q)) users.append(u);
        }
        return users;
    }

    bool save(UserAccount* user) override
    {
        if (!user) return false;
        if (user->id().isEmpty()) user->setId(IdGenerator::generateUuid());
        if (!user->createdAt().isValid()) user->setCreatedAt(QDateTime::currentDateTime());
        user->setUpdatedAt(QDateTime::currentDateTime());

        const QString sql = QStringLiteral(
            "INSERT INTO Users ("
            "  id, username, passwordHash, displayName, email, phone,"
            "  securityQuestion, securityAnswerHash, status, role, favoriteGenreIds,"
            "  createdAt, updatedAt"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

        QString favoriteGenresJson;
        if (auto* regular = dynamic_cast<RegularUser*>(user)) {
            favoriteGenresJson = toJsonStringArray(regular->favoriteGenreIds());
        }

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
            LOG_ERROR("Failed to save user: " + user->username()
                      + " | " + DbConnection::lastErrorText());
            return false;
        }

        // Persist publisher profile if applicable.
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

    bool update(UserAccount* user) override
    {
        if (!user || user->id().isEmpty()) return false;
        user->setUpdatedAt(QDateTime::currentDateTime());

        QString favoriteGenresJson;
        if (auto* regular = dynamic_cast<RegularUser*>(user)) {
            favoriteGenresJson = toJsonStringArray(regular->favoriteGenreIds());
        }

        const QString sql = QStringLiteral(
            "UPDATE Users SET "
            "  username=?, passwordHash=?, displayName=?, email=?, phone=?,"
            "  securityQuestion=?, securityAnswerHash=?, status=?, role=?,"
            "  favoriteGenreIds=?, updatedAt=? "
            "WHERE id=?"
        );

        const bool ok = DbConnection::execOk(sql, {
            user->username(), user->passwordHash(),
            user->displayName(), user->email(), user->phone(),
            user->securityQuestion(), user->securityAnswerHash(),
            static_cast<int>(user->status()),
            static_cast<int>(user->role()),
            favoriteGenresJson,
            user->updatedAt(),
            user->id()
        });

        if (!ok) {
            LOG_ERROR("Failed to update user: " + user->username());
            return false;
        }

        if (auto* publisher = dynamic_cast<Publisher*>(user)) {
            DbConnection::execOk(
                "UPDATE Publishers SET publisherName=?, biography=?, website=?, taxId=? "
                "WHERE userId=?",
                {publisher->publisherName(), publisher->biography(),
                 publisher->website(), publisher->taxId(), user->id()}
            );
        }
        return true;
    }

    bool remove(const QString& id) override
    {
        // Soft delete: mark as Deleted. Hard delete would break FK references.
        return DbConnection::execOk(
            "UPDATE Users SET status = 4, updatedAt = ? WHERE id = ?",
            {QDateTime::currentDateTime(), id}
        );
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
        return DbConnection::execOk(
            "UPDATE Users SET status = ?, updatedAt = ? WHERE id = ?",
            {static_cast<int>(status), QDateTime::currentDateTime(), id}
        );
    }

    QDateTime registeredAt(const QString& id) const override
    {
        auto q = DbConnection::run("SELECT createdAt FROM Users WHERE id = ?", {id});
        return q.next() ? q.value(0).toDateTime() : QDateTime();
    }
};

// ============== Factory ==============
IUserRepository* createUserRepository() {
    static UserRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
