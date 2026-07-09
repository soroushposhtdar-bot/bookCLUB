// =============================================================================
//  ResetPasswordViewModel.h
// =============================================================================
//  MVVM view-model for ResetPasswordPage.
//
//  Properties:
//      username                : QString (set by parent router from ForgotPasswordVM)
//      resetToken              : QString (set by parent router)
//      password                : QString
//      confirmPassword         : QString
//      passwordStrength        : int (0..4)
//      strengthLabel           : QString
//      requirementsStatus      : QVariantMap (key→bool for minLength/caseMix/digit/special)
//      passwordError           : QString
//      confirmPasswordError    : QString
//      isSubmitting / canSubmit / formError (inherited)
// =============================================================================
#ifndef RESETPASSWORDVIEWMODEL_H
#define RESETPASSWORDVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QQmlEngine>

#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

class ResetPasswordViewModel : public AuthViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString username        READ username        WRITE setUsername        NOTIFY usernameChanged)
    Q_PROPERTY(QString resetToken      READ resetToken      WRITE setResetToken      NOTIFY resetTokenChanged)
    Q_PROPERTY(QString password        READ password        WRITE setPassword        NOTIFY passwordChanged)
    Q_PROPERTY(QString confirmPassword READ confirmPassword WRITE setConfirmPassword NOTIFY confirmPasswordChanged)
    Q_PROPERTY(int passwordStrength    READ passwordStrength NOTIFY passwordStrengthChanged)
    Q_PROPERTY(QString strengthLabel   READ strengthLabel   NOTIFY strengthLabelChanged)
    Q_PROPERTY(QVariantMap requirementsStatus READ requirementsStatus NOTIFY requirementsStatusChanged)
    Q_PROPERTY(QString passwordError        READ passwordError        NOTIFY passwordErrorChanged)
    Q_PROPERTY(QString confirmPasswordError READ confirmPasswordError NOTIFY confirmPasswordErrorChanged)

public:
    explicit ResetPasswordViewModel(QObject* parent = nullptr);

    const QString& username() const { return m_username; }
    const QString& resetToken() const { return m_resetToken; }
    const QString& password() const { return m_password; }
    const QString& confirmPassword() const { return m_confirmPassword; }
    int passwordStrength() const { return m_passwordStrength; }
    const QString& strengthLabel() const { return m_strengthLabel; }
    const QVariantMap& requirementsStatus() const { return m_requirementsStatus; }
    const QString& passwordError() const { return m_passwordError; }
    const QString& confirmPasswordError() const { return m_confirmPasswordError; }

public slots:
    void setUsername(const QString& v);
    void setResetToken(const QString& v);
    void setPassword(const QString& v);
    void setConfirmPassword(const QString& v);

    Q_INVOKABLE void validatePassword();
    Q_INVOKABLE void validateConfirmPassword();

signals:
    void usernameChanged(const QString& v);
    void resetTokenChanged(const QString& v);
    void passwordChanged(const QString& v);
    void confirmPasswordChanged(const QString& v);
    void passwordStrengthChanged(int s);
    void strengthLabelChanged(const QString& label);
    void requirementsStatusChanged(const QVariantMap& status);
    void passwordErrorChanged(const QString& e);
    void confirmPasswordErrorChanged(const QString& e);

    void resetSucceeded();
    void resetFailed(const QString& error);

protected:
    void _doSubmit() override;
    void _doReset() override;
    bool _computeCanSubmit() const override;

private:
    void _recomputeStrengthAndRequirements();

    QString m_username;
    QString m_resetToken;
    QString m_password;
    QString m_confirmPassword;
    int m_passwordStrength = 0;
    QString m_strengthLabel;
    QVariantMap m_requirementsStatus;
    QString m_passwordError;
    QString m_confirmPasswordError;
};

} // namespace bookclub::client

#endif // RESETPASSWORDVIEWMODEL_H
