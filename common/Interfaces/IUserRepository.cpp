// common/interfaces/IUserRepository.cpp
#include "common/interfaces/IUserRepository.h"
#include "common/models/UserAccount.h"
#include "common/models/Admin.h"
#include "common/models/Publisher.h"
#include "common/models/RegularUser.h"
#include "common/utils/IdGenerator.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace bookclub::common {

// ========== کلاس پیاده‌ساز ==========
class UserRepositoryImpl : public IUserRepository {
public:
    UserRepositoryImpl() = default;
    ~UserRepositoryImpl() override = default;

    bool existsByUsername(const QString& username) const override {
        QSqlQuery query = getQuery("SELECT COUNT(*) FROM Users WHERE username = ?", {username});
        if (query.next()) {
            return query.value(0).toInt() > 0;
        }
        return false;
    }

    UserAccount* findById(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT * FROM Users WHERE id = ?", {id});
        return createUserFromQuery(query);
    }

    UserAccount* findByUsername(const QString& username) const override {
        QSqlQuery query = getQuery("SELECT * FROM Users WHERE username = ?", {username});
        return createUserFromQuery(query);
    }

    QVector<UserAccount*> findAll() const override {
        QVector<UserAccount*> users;
        QSqlQuery query = getQuery("SELECT * FROM Users ORDER BY username");
        while (query.next()) {
            UserAccount* user = createUserFromCurrentRecord(query);
            if (user) users.append(user);
        }
        return users;
    }

    QVector<UserAccount*> search(const QString& keyword) const override {
        QVector<UserAccount*> users;
        QString sql = R"(
            SELECT * FROM Users
            WHERE username LIKE ?
               OR displayName LIKE ?
               OR email LIKE ?
               OR phone LIKE ?
            ORDER BY username
        )";
        QString pattern = "%" + keyword + "%";
        QSqlQuery query = getQuery(sql, {pattern, pattern, pattern, pattern});
        while (query.next()) {
            UserAccount* user = createUserFromCurrentRecord(query);
            if (user) users.append(user);
        }
        return users;
    }

    bool save(UserAccount* user) override {
        if (!user) return false;

        if (user->id().isEmpty()) {
            user->setId(IdGenerator::generateUuid());
        }

        QString sql = R"(
            INSERT INTO Users (
                id, username, passwordHash, displayName, email, phone,
                securityQuestion, securityAnswerHash, status, role,
                createdAt, updatedAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getQuery(sql, {
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
            QDateTime::currentDateTime(),
            QDateTime::currentDateTime()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    bool update(UserAccount* user) override {
        if (!user || user->id().isEmpty()) return false;

        QString sql = R"(
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

        QSqlQuery query = getQuery(sql, {
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

        return query.lastError().type() == QSqlError::NoError;
    }

    bool remove(const QString& id) override {
        QSqlQuery query = getQuery("DELETE FROM Users WHERE id = ?", {id});
        return query.lastError().type() == QSqlError::NoError;
    }

    bool blockUser(const QString& id) override {
        return setAccountStatus(id, AccountStatus::Blocked);
    }

    bool unblockUser(const QString& id) override {
        return setAccountStatus(id, AccountStatus::Active);
    }

    bool setAccountStatus(const QString& id, AccountStatus status) override {
        QSqlQuery query = getQuery(
            "UPDATE Users SET status = ?, updatedAt = ? WHERE id = ?",
            {static_cast<int>(status), QDateTime::currentDateTime(), id}
        );
        return query.lastError().type() == QSqlError::NoError;
    }

    QDateTime registeredAt(const QString& id) const override {
        QSqlQuery query = getQuery("SELECT createdAt FROM Users WHERE id = ?", {id});
        if (query.next()) {
            return query.value(0).toDateTime();
        }
        return QDateTime();
    }

private:
    // ====== Helper methods ======
    QSqlQuery getQuery(const QString& sql, const QVariantList& params = {}) const {
        QSqlDatabase db = getDatabase();
        QSqlQuery query(db);
        query.prepare(sql);
        for (const auto& param : params) {
            query.addBindValue(param);
        }
        query.exec();
        return query;
    }

    QSqlDatabase getDatabase() const {
        static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if (!db.isOpen()) {
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        return db;
    }

    UserAccount* createUserFromQuery(const QSqlQuery& query) const {
        if (!query.next()) return nullptr;
        return createUserFromCurrentRecord(query);
    }

    UserAccount* createUserFromCurrentRecord(const QSqlQuery& query) const {
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

        // Load additional fields for subclasses
        if (auto* regular = dynamic_cast<RegularUser*>(user)) {
            // Load favoriteGenres, savedBooks, purchasedBooks from other tables if needed
            // For simplicity, we'll skip these now
        }
        if (auto* publisher = dynamic_cast<Publisher*>(user)) {
            // Load publisher-specific fields
        }

        return user;
    }
};

// ========== Factory function ==========
IUserRepository* createUserRepository() {
    static UserRepositoryImpl repo;
    return &repo;
}

} // namespace bookclub::common
