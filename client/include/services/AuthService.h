#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>

namespace bookclub::client {

class AuthService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString currentRole        READ currentRole        NOTIFY currentRoleChanged)
    Q_PROPERTY(QString currentUsername    READ currentUsername    NOTIFY currentUsernameChanged)
    Q_PROPERTY(QString currentDisplayName READ currentDisplayName NOTIFY currentDisplayNameChanged)
    Q_PROPERTY(QString currentUserId      READ currentUserId      NOTIFY currentUserIdChanged)
    // BUG FIX: isLoggedIn() returns !_currentUsername.isEmpty(), so the
    // correct NOTIFY is currentUsernameChanged (not currentRoleChanged).
    // Previously worked only because every code path that changes the
    // username also happens to change the role and emit both signals.
    Q_PROPERTY(bool    isLoggedIn         READ isLoggedIn         NOTIFY currentUsernameChanged)

public:
    explicit AuthService(QObject* parent = nullptr);

    // C++ accessor (mirrors NetworkService::instance() pattern).
    // UserService and PublisherService use this to read the current user's
    // identity without going through QML.
    static AuthService& instance();

    QString currentRole()        const { return _currentRole; }
    QString currentUsername()    const { return _currentUsername; }
    QString currentDisplayName() const { return _currentDisplayName; }
    QString currentUserId()      const { return _currentUserId; }
    bool    isLoggedIn()         const { return !_currentUsername.isEmpty(); }

    // v8: allows PublisherService to update the display name after a
    // successful UpdateProfile call so the change propagates app-wide.
    void setCurrentDisplayName(const QString& name) {
        if (_currentDisplayName != name) {
            _currentDisplayName = name;
            emit currentDisplayNameChanged();
        }
    }

    Q_INVOKABLE void logout();

    Q_INVOKABLE bool userExists(const QString& username) const;
    Q_INVOKABLE QString securityQuestionFor(const QString& username) const;
    Q_INVOKABLE bool verifySecurityAnswer(const QString& username, const QString& answer) const;

    // NOT Q_INVOKABLE — Qt6 MOC cannot register non-const reference params.
    bool login(const QString& username, const QString& password, QString& errorMessage);
    // v22 (Issue 5): `role` is "user" (default) or "publisher". Passes
    // through to the server's Register endpoint so the user is created
    // with the appropriate derived type (RegularUser vs Publisher).
    bool registerUser(const QString& username,
                      const QString& displayName,
                      const QString& password,
                      const QString& securityQuestion,
                      const QString& securityAnswer,
                      const QString& role,
                      QString& errorMessage);
    // Back-compat overload — calls the 7-arg version with role="user".
    // Kept so existing call sites that don't yet pass a role still compile.
    inline bool registerUser(const QString& username,
                             const QString& displayName,
                             const QString& password,
                             const QString& securityQuestion,
                             const QString& securityAnswer,
                             QString& errorMessage) {
        return registerUser(username, displayName, password,
                             securityQuestion, securityAnswer,
                             QStringLiteral("user"), errorMessage);
    }

    Q_INVOKABLE QString issueResetToken(const QString& username);
    bool resetPassword(const QString& username,
                       const QString& resetToken,
                       const QString& newPassword,
                       QString& errorMessage);

    Q_INVOKABLE void stashSecurityAnswer(const QString& answer);
    Q_INVOKABLE QString takeStashedSecurityAnswer();

    bool changePassword(const QString& oldPassword,
                        const QString& newPassword,
                        QString& errorMessage);

    // v9: Q_INVOKABLE wrapper so QML can call AuthService.changePassword(old, new).
    // The 3-arg version with reference param can't be Q_INVOKABLE (Qt6 MOC
    // limitation). This wrapper swallows the error message into a console
    // warning and returns a bool.
    Q_INVOKABLE bool changePassword(const QString& oldPassword,
                                    const QString& newPassword);

    Q_INVOKABLE bool saveGenreSelection(const QString& username, const QStringList& genres);
    Q_INVOKABLE bool requiresGenreSetup(const QString& username) const;

    Q_INVOKABLE QStringList availableSecurityQuestions() const;
    Q_INVOKABLE QStringList availableGenres() const;
    Q_INVOKABLE bool isUsernameAvailable(const QString& username) const;

signals:
    void currentRoleChanged();
    void currentUsernameChanged();
    void currentDisplayNameChanged();
    void currentUserIdChanged();

    // Emitted whenever any auth operation fails because the server is
    // unreachable. App.qml wires this to the global ToastManager so the
    // user gets a modern top-right notification instead of a silent
    // form-error banner (Issue 2).
    void connectionFailed(const QString& reason);

private:
    QString _currentRole;
    QString _currentUsername;
    QString _currentDisplayName;
    QString _currentUserId;
    bool    _requiresGenreSetup = false;
    QString _stashedSecurityAnswer;

    static QStringList _defaultSecurityQuestions();
    static QStringList _defaultGenres();
};

} // namespace bookclub::client
