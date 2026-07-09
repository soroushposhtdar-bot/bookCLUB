// =============================================================================
//  RegisterViewModel.h
// =============================================================================
//  MVVM view-model for RegisterPage.
//
//  Properties:
//      username              : QString
//      displayName           : QString
//      password              : QString
//      confirmPassword       : QString
//      securityQuestion      : QString
//      securityAnswer        : QString
//      acceptTerms           : bool
//      usernameAvailable     : bool   (async-checked against AuthService)
//      passwordStrength      : int    (0..4)
//      strengthLabel         : QString
//      availableSecurityQuestions : QStringList (read-only source for dropdown)
//      <field>Error          : QString (per-field validation messages)
//      isSubmitting / canSubmit / formError (inherited)
//
//  Q_INVOKABLE:
//      validateUsername(), validateDisplayName(), validatePassword(),
//      validateConfirmPassword(), validateSecurityQuestion(),
//      validateSecurityAnswer(), validateAcceptTerms(),
//      submit(), reset()
//
//  Signals:
//      registerSucceeded()
//      registerFailed(QString error)
// =============================================================================
#ifndef REGISTERVIEWMODEL_H
#define REGISTERVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>
#include <memory>

#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

class RegisterViewModel : public AuthViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString username         READ username         WRITE setUsername         NOTIFY usernameChanged)
    Q_PROPERTY(QString displayName      READ displayName      WRITE setDisplayName      NOTIFY displayNameChanged)
    Q_PROPERTY(QString password         READ password         WRITE setPassword         NOTIFY passwordChanged)
    Q_PROPERTY(QString confirmPassword  READ confirmPassword  WRITE setConfirmPassword  NOTIFY confirmPasswordChanged)
    Q_PROPERTY(QString securityQuestion READ securityQuestion WRITE setSecurityQuestion NOTIFY securityQuestionChanged)
    Q_PROPERTY(QString securityAnswer   READ securityAnswer   WRITE setSecurityAnswer   NOTIFY securityAnswerChanged)
    Q_PROPERTY(bool    acceptTerms      READ acceptTerms      WRITE setAcceptTerms      NOTIFY acceptTermsChanged)

    Q_PROPERTY(bool     usernameAvailable  READ usernameAvailable  NOTIFY usernameAvailableChanged)
    Q_PROPERTY(int      passwordStrength   READ passwordStrength   NOTIFY passwordStrengthChanged)
    Q_PROPERTY(QString  strengthLabel      READ strengthLabel      NOTIFY strengthLabelChanged)
    Q_PROPERTY(QStringList availableSecurityQuestions READ availableSecurityQuestions NOTIFY availableSecurityQuestionsChanged)

    Q_PROPERTY(QString usernameError       READ usernameError       NOTIFY usernameErrorChanged)
    Q_PROPERTY(QString displayNameError    READ displayNameError    NOTIFY displayNameErrorChanged)
    Q_PROPERTY(QString passwordError       READ passwordError       NOTIFY passwordErrorChanged)
    Q_PROPERTY(QString confirmPasswordError READ confirmPasswordError NOTIFY confirmPasswordErrorChanged)
    Q_PROPERTY(QString securityQuestionError READ securityQuestionError NOTIFY securityQuestionErrorChanged)
    Q_PROPERTY(QString securityAnswerError READ securityAnswerError NOTIFY securityAnswerErrorChanged)
    Q_PROPERTY(QString acceptTermsError    READ acceptTermsError    NOTIFY acceptTermsErrorChanged)

public:
    explicit RegisterViewModel(QObject* parent = nullptr);

    const QString&  username() const { return m_username; }
    const QString&  displayName() const { return m_displayName; }
    const QString&  password() const { return m_password; }
    const QString&  confirmPassword() const { return m_confirmPassword; }
    const QString&  securityQuestion() const { return m_securityQuestion; }
    const QString&  securityAnswer() const { return m_securityAnswer; }
    bool            acceptTerms() const { return m_acceptTerms; }
    bool            usernameAvailable() const { return m_usernameAvailable; }
    int             passwordStrength() const { return m_passwordStrength; }
    const QString&  strengthLabel() const { return m_strengthLabel; }
    QStringList     availableSecurityQuestions() const;

    const QString&  usernameError() const { return m_usernameError; }
    const QString&  displayNameError() const { return m_displayNameError; }
    const QString&  passwordError() const { return m_passwordError; }
    const QString&  confirmPasswordError() const { return m_confirmPasswordError; }
    const QString&  securityQuestionError() const { return m_securityQuestionError; }
    const QString&  securityAnswerError() const { return m_securityAnswerError; }
    const QString&  acceptTermsError() const { return m_acceptTermsError; }

public slots:
    void setUsername(const QString& v);
    void setDisplayName(const QString& v);
    void setPassword(const QString& v);
    void setConfirmPassword(const QString& v);
    void setSecurityQuestion(const QString& v);
    void setSecurityAnswer(const QString& v);
    void setAcceptTerms(bool v);

    Q_INVOKABLE void validateUsername();
    Q_INVOKABLE void validateDisplayName();
    Q_INVOKABLE void validatePassword();
    Q_INVOKABLE void validateConfirmPassword();
    Q_INVOKABLE void validateSecurityQuestion();
    Q_INVOKABLE void validateSecurityAnswer();
    Q_INVOKABLE void validateAcceptTerms();

signals:
    void usernameChanged(const QString& v);
    void displayNameChanged(const QString& v);
    void passwordChanged(const QString& v);
    void confirmPasswordChanged(const QString& v);
    void securityQuestionChanged(const QString& v);
    void securityAnswerChanged(const QString& v);
    void acceptTermsChanged(bool v);
    void usernameAvailableChanged(bool v);
    void passwordStrengthChanged(int s);
    void strengthLabelChanged(const QString& label);
    void availableSecurityQuestionsChanged(const QStringList& q);
    void usernameErrorChanged(const QString& e);
    void displayNameErrorChanged(const QString& e);
    void passwordErrorChanged(const QString& e);
    void confirmPasswordErrorChanged(const QString& e);
    void securityQuestionErrorChanged(const QString& e);
    void securityAnswerErrorChanged(const QString& e);
    void acceptTermsErrorChanged(const QString& e);

    void registerSucceeded();
    void registerFailed(const QString& error);

protected:
    void _doSubmit() override;
    void _doReset() override;
    bool _computeCanSubmit() const override;
    void _onAuthServiceChanged() override;

private:
    void _recomputePasswordStrength();
    void _setUsernameAvailable(bool v);

    QString m_username;
    QString m_displayName;
    QString m_password;
    QString m_confirmPassword;
    QString m_securityQuestion;
    QString m_securityAnswer;
    bool    m_acceptTerms = false;

    bool    m_usernameAvailable = false;
    int     m_passwordStrength = 0;
    QString m_strengthLabel;

    QString m_usernameError;
    QString m_displayNameError;
    QString m_passwordError;
    QString m_confirmPasswordError;
    QString m_securityQuestionError;
    QString m_securityAnswerError;
    QString m_acceptTermsError;
};

} // namespace bookclub::client

#endif // REGISTERVIEWMODEL_H
