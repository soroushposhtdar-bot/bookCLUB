// =============================================================================
//  AuthService.h
// =============================================================================
//  Mocked authentication service — stands in for the real backend while the
//  network layer is being built out. Implements the IAuthService contract but
//  resolves everything locally against an in-memory user table.
//
//  The mock is intentionally realistic:
//      • Latency simulated via QTimer (default 600-1000ms)
//      • Pre-seeded users: alice/admin, bob/user (passwords as in README)
//      • Passwords hashed via PasswordHasher (SHA-256 + per-user salt)
//      • Security-question answers stored hashed
//      • Reset tokens are random 32-byte hex strings
//
//  All API is Q_INVOKABLE so it can be invoked from QML directly when needed.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QQmlEngine>
#include <memory>

namespace bookclub::client {

struct MockUser {
    QString id;
    QString username;
    QString displayName;
    QString passwordHash;
    QString securityQuestion;
    QString securityAnswerHash;
    QStringList selectedGenres;
    bool requiresGenreSetup = false;
    QString role = QStringLiteral("user");  // user | publisher | admin | server
    QString status = QStringLiteral("Active");  // Active | Blocked
};

class AuthService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // The role of the currently logged-in user. Empty until login succeeds.
    // One of: "user", "publisher", "admin", "server".
    Q_PROPERTY(QString currentRole     READ currentRole     NOTIFY currentRoleChanged)
    Q_PROPERTY(QString currentUsername  READ currentUsername  NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentDisplayName READ currentDisplayName NOTIFY currentDisplayNameChanged)
    Q_PROPERTY(bool    isLoggedIn       READ isLoggedIn       NOTIFY currentRoleChanged)

public:
    explicit AuthService(QObject* parent = nullptr);

    // ----- Current-session state -----
    QString currentRole()        const { return _currentRole; }
    QString currentUsername()    const { return _currentUsername; }
    QString currentDisplayName() const { return _currentDisplayName; }
    bool    isLoggedIn()         const { return !_currentUsername.isEmpty(); }

    Q_INVOKABLE void logout();

    // ----- Public API -----
    Q_INVOKABLE bool userExists(const QString& username) const;
    Q_INVOKABLE QString securityQuestionFor(const QString& username) const;
    Q_INVOKABLE bool verifySecurityAnswer(const QString& username, const QString& answer) const;

    // Login — returns true on success. On failure, errorMessage is set.
    Q_INVOKABLE bool login(const QString& username, const QString& password, QString& errorMessage);
    Q_INVOKABLE bool registerUser(const QString& username,
                                  const QString& displayName,
                                  const QString& password,
                                  const QString& securityQuestion,
                                  const QString& securityAnswer,
                                  QString& errorMessage);

    // Password recovery
    Q_INVOKABLE QString issueResetToken(const QString& username);
    Q_INVOKABLE bool resetPassword(const QString& username,
                                   const QString& resetToken,
                                   const QString& newPassword,
                                   QString& errorMessage);

    // Genre selection
    Q_INVOKABLE bool saveGenreSelection(const QString& username, const QStringList& genres);
    Q_INVOKABLE bool requiresGenreSetup(const QString& username) const;

    // Predefined security questions (UI dropdown source)
    Q_INVOKABLE QStringList availableSecurityQuestions() const;

    // Predefined genres (UI grid source)
    Q_INVOKABLE QStringList availableGenres() const;

    // Username availability check (for the registration page).
    // Returns true if no existing user has this username.
    Q_INVOKABLE bool isUsernameAvailable(const QString& username) const;

    // Seed one demo account — exposed so tests / first-run wizard can add more.
    void seedDemoUser(const QString& username,
                      const QString& displayName,
                      const QString& password,
                      const QString& securityQuestion,
                      const QString& securityAnswer,
                      const QString& role = QStringLiteral("user"));

signals:
    void currentRoleChanged();
    void currentUsernameChanged();
    void currentDisplayNameChanged();

private:
    QHash<QString, MockUser> _users;
    QHash<QString, QString> _resetTokens;  // username → token

    QString _currentRole;
    QString _currentUsername;
    QString _currentDisplayName;

    void _seedDefaults();
};

} // namespace bookclub::client
