// =============================================================================
//  ForgotPasswordViewModel.h
// =============================================================================
//  MVVM view-model for ForgotPasswordPage.
//
//  Two-step flow:
//      Step 1 "username": user enters their username. We look it up in the
//      AuthService and, if found, load the user's security question and
//      transition to step 2.
//      Step 2 "answer": user enters their security answer. If correct, the
//      AuthService issues a reset token and we emit recoverySucceeded().
//
//  Properties:
//      step             : QString ("username" | "answer")
//      username         : QString
//      securityQuestion : QString (loaded in step 2)
//      securityAnswer   : QString
//      resetToken       : QString (set on success, passed to ResetPasswordVM)
//      <field>Error     : QString
//      isSubmitting / canSubmit / formError (inherited)
// =============================================================================
#ifndef FORGOTPASSWORDVIEWMODEL_H
#define FORGOTPASSWORDVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

class ForgotPasswordViewModel : public AuthViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString step              READ step              NOTIFY stepChanged)
    Q_PROPERTY(QString username          READ username          WRITE setUsername          NOTIFY usernameChanged)
    Q_PROPERTY(QString securityQuestion  READ securityQuestion  NOTIFY securityQuestionChanged)
    Q_PROPERTY(QString securityAnswer    READ securityAnswer    WRITE setSecurityAnswer    NOTIFY securityAnswerChanged)
    Q_PROPERTY(QString resetToken        READ resetToken        NOTIFY resetTokenChanged)
    Q_PROPERTY(QString usernameError     READ usernameError     NOTIFY usernameErrorChanged)
    Q_PROPERTY(QString securityAnswerError READ securityAnswerError NOTIFY securityAnswerErrorChanged)

public:
    explicit ForgotPasswordViewModel(QObject* parent = nullptr);

    const QString& step() const { return m_step; }
    const QString& username() const { return m_username; }
    const QString& securityQuestion() const { return m_securityQuestion; }
    const QString& securityAnswer() const { return m_securityAnswer; }
    const QString& resetToken() const { return m_resetToken; }
    const QString& usernameError() const { return m_usernameError; }
    const QString& securityAnswerError() const { return m_securityAnswerError; }

public slots:
    void setUsername(const QString& v);
    void setSecurityAnswer(const QString& v);

    Q_INVOKABLE void validateUsername();
    Q_INVOKABLE void validateSecurityAnswer();

    Q_INVOKABLE void goToStep(const QString& step);
    Q_INVOKABLE void backToUsername();

signals:
    void stepChanged(const QString& step);
    void usernameChanged(const QString& v);
    void securityQuestionChanged(const QString& q);
    void securityAnswerChanged(const QString& v);
    void resetTokenChanged(const QString& token);
    void usernameErrorChanged(const QString& e);
    void securityAnswerErrorChanged(const QString& e);

    void recoverySucceeded();
    void recoveryFailed(const QString& error);

protected:
    void _doSubmit() override;
    void _doReset() override;
    bool _computeCanSubmit() const override;

private:
    void _setStep(const QString& step);

    QString m_step = QStringLiteral("username");
    QString m_username;
    QString m_securityQuestion;
    QString m_securityAnswer;
    QString m_resetToken;
    QString m_usernameError;
    QString m_securityAnswerError;
};

} // namespace bookclub::client

#endif // FORGOTPASSWORDVIEWMODEL_H
