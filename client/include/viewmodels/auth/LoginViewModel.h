// =============================================================================
//  LoginViewModel.h
// =============================================================================
//  MVVM view-model for LoginPage. Owns username/password state, runs
//  frontend validation, and orchestrates the mocked AuthService.login() call.
//
//  Properties exposed to QML (all NOTIFY-enabled):
//      username         : QString
//      password         : QString
//      rememberMe       : bool
//      usernameError    : QString (validation error for username field)
//      passwordError    : QString (validation error for password field)
//      formError        : QString (form-level banner message)
//      isSubmitting     : bool
//      canSubmit        : bool
//
//  Q_INVOKABLE slots (callable from QML):
//      validateUsername()
//      validatePassword()
//      submit()
//      reset()
//
//  Signals (so QML can react):
//      loginSucceeded()  — fire after a successful login
//      loginFailed(QString error)
// =============================================================================
#ifndef LOGINVIEWMODEL_H
#define LOGINVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <memory>

#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

class LoginViewModel : public AuthViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString username      READ username      WRITE setUsername      NOTIFY usernameChanged)
    Q_PROPERTY(QString password      READ password      WRITE setPassword      NOTIFY passwordChanged)
    Q_PROPERTY(bool     rememberMe   READ rememberMe    WRITE setRememberMe    NOTIFY rememberMeChanged)
    Q_PROPERTY(QString usernameError READ usernameError NOTIFY usernameErrorChanged)
    Q_PROPERTY(QString passwordError READ passwordError NOTIFY passwordErrorChanged)

public:
    explicit LoginViewModel(QObject* parent = nullptr);

    const QString& username() const { return m_username; }
    const QString& password() const { return m_password; }
    bool rememberMe() const { return m_rememberMe; }
    const QString& usernameError() const { return m_usernameError; }
    const QString& passwordError() const { return m_passwordError; }

public slots:
    void setUsername(const QString& v);
    void setPassword(const QString& v);
    void setRememberMe(bool v);

    // Validation hooks — callable from QML on each field edit.
    Q_INVOKABLE void validateUsername();
    Q_INVOKABLE void validatePassword();

signals:
    void usernameChanged(const QString& v);
    void passwordChanged(const QString& v);
    void rememberMeChanged(bool v);
    void usernameErrorChanged(const QString& v);
    void passwordErrorChanged(const QString& v);

    // High-level outcome signals
    void loginSucceeded();
    void loginFailed(const QString& error);

protected:
    void _doSubmit() override;
    void _doReset() override;
    bool _computeCanSubmit() const override;

private:
    QString m_username;
    QString m_password;
    bool m_rememberMe = false;
    QString m_usernameError;
    QString m_passwordError;
};

} // namespace bookclub::client

#endif // LOGINVIEWMODEL_H
