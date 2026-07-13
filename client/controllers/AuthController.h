#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class AuthController : public QObject {
    Q_OBJECT
public:
    explicit AuthController(QObject* parent = nullptr);
    ~AuthController() override = default;

    void setLoginCredentials(const QString& username, const QString& password);
    void setRegistrationData(const QString& username,
                             const QString& password,
                             const QString& displayName,
                             const QString& email,
                             const QString& phone,
                             const QString& securityQuestion,
                             const QString& securityAnswer);
    void setResetPasswordData(const QString& username,
                              const QString& securityAnswer,
                              const QString& newPassword);

    void login();
    void registerUser();
    void resetPassword();
    void logout();

signals:
    void busyChanged(bool busy);
    void loginSucceeded();
    void loginFailed(const QString& message);
    void registrationSucceeded();
    void registrationFailed(const QString& message);
    void passwordResetSucceeded();
    void passwordResetFailed(const QString& message);

private:
    QString m_username;
    QString m_password;
    QString m_displayName;
    QString m_email;
    QString m_phone;
    QString m_securityQuestion;
    QString m_securityAnswer;
    QString m_newPassword;
};

} // namespace bookclub::client
