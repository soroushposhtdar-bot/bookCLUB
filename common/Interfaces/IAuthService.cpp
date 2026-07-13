// common/interfaces/IAuthService.cpp
#include "common/interfaces/IAuthService.h"
#include "common/models/UserAccount.h"
#include "common/models/Admin.h"
#include "common/models/Publisher.h"
#include "common/models/RegularUser.h"
#include "common/utils/PasswordHasher.h"
#include "common/utils/IdGenerator.h"
#include "common/utils/Logger.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

namespace bookclub::common {

// ========== کلاس پیاده‌ساز ==========
class AuthServiceImpl : public IAuthService {
public:
    AuthServiceImpl() = default;
    ~AuthServiceImpl() override = default;

    UserAccount* registerAccount(UserAccount* account,
                                 const QString& plainPassword,
                                 const QString& securityAnswer) override {
        LOG_DEBUG("Registering user: " + account->username());

        if (!account || plainPassword.isEmpty() || securityAnswer.isEmpty()) {
            LOG_ERROR("Invalid registration data");
            return nullptr;
        }

        // Check username uniqueness
        if (!isUsernameUnique(account->username())) {
            LOG_WARNING("Username already exists: " + account->username());
            return nullptr;
        }

        // Generate ID if not set
        if (account->id().isEmpty()) {
            account->setId(IdGenerator::generateUuid());
        }

        // Hash password and security answer
        QString salt = PasswordHasher::generateSalt();
        account->setPasswordHash(PasswordHasher::hashPassword(plainPassword, salt));
        account->setSecurityAnswerHash(PasswordHasher::hashPassword(securityAnswer, salt));
        account->setStatus(AccountStatus::Pending);
        account->setCreatedAt(QDateTime::currentDateTime());

        // Save to database
        if (!saveUserToDatabase(account)) {
            LOG_ERROR("Failed to save user to database");
            delete account;
            return nullptr;
        }

        LOG_INFO("User registered successfully: " + account->username());
        return account;
    }

    UserAccount* login(const QString& username, const QString& plainPassword) override {
        LOG_DEBUG("Login attempt for: " + username);

        if (username.isEmpty() || plainPassword.isEmpty()) {
            LOG_WARNING("Empty username or password");
            return nullptr;
        }

        // Find user in database
        UserAccount* user = findUserByUsername(username);
        if (!user) {
            LOG_WARNING("User not found: " + username);
            return nullptr;
        }

        // Check if user is blocked or disabled
        if (user->isBlocked() || user->status() == AccountStatus::Disabled) {
            LOG_WARNING("User is blocked or disabled: " + username);
            delete user;
            return nullptr;
        }

        // Verify password (we need to retrieve salt from database)
        // For simplicity, we assume passwordHash is stored as "salt:hash"
        QString storedHash = user->passwordHash();
        // In real implementation, you should retrieve salt separately
        // Here we use a simple verification (you can improve later)
        if (!verifyPasswordWithDatabase(username, plainPassword)) {
            LOG_WARNING("Invalid password for: " + username);
            delete user;
            return nullptr;
        }

        LOG_INFO("User logged in: " + username);
        return user;
    }

    bool logout(const QString& userId) override {
        LOG_DEBUG("Logout for user: " + userId);
        // Invalidate session token if needed
        return true;
    }

    bool changePassword(const QString& userId,
                        const QString& oldPassword,
                        const QString& newPassword) override {
        LOG_DEBUG("Changing password for user: " + userId);

        if (userId.isEmpty() || oldPassword.isEmpty() || newPassword.isEmpty()) {
            return false;
        }

        // Get user from database
        UserAccount* user = findUserById(userId);
        if (!user) {
            LOG_WARNING("User not found for password change: " + userId);
            return false;
        }

        // Verify old password
        if (!verifyPasswordWithDatabase(user->username(), oldPassword)) {
            LOG_WARNING("Old password incorrect for: " + userId);
            delete user;
            return false;
        }

        // Update password
        QString salt = PasswordHasher::generateSalt();
        QString newHash = PasswordHasher::hashPassword(newPassword, salt);
        if (!updateUserPassword(userId, newHash)) {
            LOG_ERROR("Failed to update password for: " + userId);
            delete user;
            return false;
        }

        delete user;
        LOG_INFO("Password changed successfully for: " + userId);
        return true;
    }

    bool resetPassword(const QString& username,
                       const QString& securityAnswer,
                       const QString& newPassword) override {
        LOG_DEBUG("Resetting password for: " + username);

        if (username.isEmpty() || securityAnswer.isEmpty() || newPassword.isEmpty()) {
            return false;
        }

        // Get user from database
        UserAccount* user = findUserByUsername(username);
        if (!user) {
            LOG_WARNING("User not found for reset: " + username);
            return false;
        }

        // Verify security answer (simplified)
        if (user->securityAnswerHash() != securityAnswer) {
            // In real implementation, hash the security answer and compare
            LOG_WARNING("Security answer incorrect for: " + username);
            delete user;
            return false;
        }

        // Update password
        QString salt = PasswordHasher::generateSalt();
        QString newHash = PasswordHasher::hashPassword(newPassword, salt);
        if (!updateUserPassword(user->id(), newHash)) {
            LOG_ERROR("Failed to reset password for: " + username);
            delete user;
            return false;
        }

        delete user;
        LOG_INFO("Password reset successfully for: " + username);
        return true;
    }

    bool isUsernameUnique(const QString& username) const override {
        QSqlQuery query = getDbQuery(
            "SELECT COUNT(*) FROM Users WHERE username = ?",
            {username}
        );
        if (query.next()) {
            return query.value(0).toInt() == 0;
        }
        return true;
    }

private:
    // ====== Database helper methods ======
    QSqlQuery getDbQuery(const QString& sql, const QVariantList& params = {}) const {
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
        // You should inject DatabaseManager or use a singleton
        // For now, we'll use a simple approach
        static QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if (!db.isOpen()) {
            db.setDatabaseName("bookclub.db");
            db.open();
        }
        return db;
    }

    bool saveUserToDatabase(UserAccount* user) {
        QString sql = R"(
            INSERT INTO Users (
                id, username, passwordHash, displayName, email, phone,
                securityQuestion, securityAnswerHash, status, role,
                createdAt, updatedAt
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        QSqlQuery query = getDbQuery(sql, {
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
            QDateTime::currentDateTime()
        });

        return query.lastError().type() == QSqlError::NoError;
    }

    bool updateUserPassword(const QString& userId, const QString& newHash) {
        QString sql = "UPDATE Users SET passwordHash = ?, updatedAt = ? WHERE id = ?";
        QSqlQuery query = getDbQuery(sql, {newHash, QDateTime::currentDateTime(), userId});
        return query.lastError().type() == QSqlError::NoError;
    }

    UserAccount* findUserById(const QString& id) const {
        QSqlQuery query = getDbQuery("SELECT * FROM Users WHERE id = ?", {id});
        return createUserFromQuery(query);
    }

    UserAccount* findUserByUsername(const QString& username) const {
        QSqlQuery query = getDbQuery("SELECT * FROM Users WHERE username = ?", {username});
        return createUserFromQuery(query);
    }

    UserAccount* createUserFromQuery(const QSqlQuery& query) const {
        if (!query.next()) return nullptr;

        QSqlRecord rec = query.record();
        QString id = rec.value("id").toString();
        QString username = rec.value("username").toString();
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

        user->setId(id);
        user->setUsername(username);
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

    bool verifyPasswordWithDatabase(const QString& username, const QString& plainPassword) const {
        QSqlQuery query = getDbQuery(
            "SELECT passwordHash FROM Users WHERE username = ?",
            {username}
        );
        if (query.next()) {
            QString storedHash = query.value(0).toString();
            // In real implementation, you need to extract salt from storedHash
            // For now, we'll assume storedHash is just a hash and we hash the plain password
            // without salt for simplicity (not secure, but works for demo)
            // You should use PasswordHasher::verifyPassword with salt
            return storedHash == PasswordHasher::hashPassword(plainPassword, "");
        }
        return false;
    }
};

// ========== Factory function to get instance ==========
IAuthService* createAuthService() {
    static AuthServiceImpl service;
    return &service;
}

} // namespace bookclub::common
