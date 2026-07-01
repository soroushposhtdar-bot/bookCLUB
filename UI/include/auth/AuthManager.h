#pragma once

#include <QString>
#include <QSettings>

// Singleton that manages user accounts persisted via QSettings.
// Passwords and security answers are stored as SHA-256 hashes.
class AuthManager
{
public:
    static AuthManager &instance();

    // Returns true on success; on failure, lastError() explains why.
    bool registerUser(const QString &username,
                      const QString &email,
                      const QString &password,
                      const QString &securityQuestion,
                      const QString &securityAnswer);

    bool loginUser(const QString &username, const QString &password);

    bool emailExists(const QString &email) const;
    QString getUsernameByEmail(const QString &email) const;

    // Returns the security question for the account owning `email`, or empty string.
    QString getSecurityQuestion(const QString &email) const;

    // Returns true if the answer matches the stored (hashed) answer.
    bool verifySecurityAnswer(const QString &email, const QString &answer);

    // Overwrites the stored password for the account owning `email`.
    bool resetPassword(const QString &email, const QString &newPassword);

    QString lastError() const;

private:
    AuthManager();
    AuthManager(const AuthManager &)            = delete;
    AuthManager &operator=(const AuthManager &) = delete;

    static QString hashPassword(const QString &password);

    mutable QSettings m_settings;
    QString           m_lastError;
};
