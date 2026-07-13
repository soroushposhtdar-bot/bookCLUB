// =============================================================================
//  ForgotPasswordViewModel.cpp
// =============================================================================
#include "viewmodels/auth/ForgotPasswordViewModel.h"

#include "common/Utils/ValidationUtils.h"

namespace bookclub::client {

ForgotPasswordViewModel::ForgotPasswordViewModel(QObject* parent)
    : AuthViewModelBase(parent)
{}

// ----- Setters -----

void ForgotPasswordViewModel::setUsername(const QString& v) {
    if (m_username == v) return;
    m_username = v;
    emit usernameChanged(m_username);
    if (m_usernameError.length() > 0) {
        m_usernameError.clear();
        emit usernameErrorChanged(m_usernameError);
    }
    _recomputeCanSubmit();
}

void ForgotPasswordViewModel::setSecurityAnswer(const QString& v) {
    if (m_securityAnswer == v) return;
    m_securityAnswer = v;
    emit securityAnswerChanged(m_securityAnswer);
    if (m_securityAnswerError.length() > 0) {
        m_securityAnswerError.clear();
        emit securityAnswerErrorChanged(m_securityAnswerError);
    }
    _recomputeCanSubmit();
}

// ----- Validation -----

void ForgotPasswordViewModel::validateUsername() {
    QString err;
    if (m_username.trimmed().isEmpty()) {
        err = QStringLiteral("Username is required.");
    } else if (!bookclub::common::ValidationUtils::isValidUsernameLength(m_username)) {
        err = QStringLiteral("Username must be 3–20 characters.");
    }
    if (err != m_usernameError) {
        m_usernameError = err;
        emit usernameErrorChanged(m_usernameError);
    }
    _recomputeCanSubmit();
}

void ForgotPasswordViewModel::validateSecurityAnswer() {
    QString err;
    if (m_securityAnswer.trimmed().isEmpty()) {
        err = QStringLiteral("Please enter your answer.");
    } else if (m_securityAnswer.trimmed().length() < 2) {
        err = QStringLiteral("Answer must be at least 2 characters.");
    }
    if (err != m_securityAnswerError) {
        m_securityAnswerError = err;
        emit securityAnswerErrorChanged(m_securityAnswerError);
    }
    _recomputeCanSubmit();
}

// ----- Step navigation -----

void ForgotPasswordViewModel::goToStep(const QString& step) {
    _setStep(step);
}

void ForgotPasswordViewModel::backToUsername() {
    m_securityAnswer.clear();
    m_securityAnswerError.clear();
    emit securityAnswerChanged(m_securityAnswer);
    emit securityAnswerErrorChanged(m_securityAnswerError);
    _setStep(QStringLiteral("username"));
}

void ForgotPasswordViewModel::_setStep(const QString& step) {
    if (m_step == step) return;
    m_step = step;
    emit stepChanged(m_step);
    _recomputeCanSubmit();
}

// ----- Submit -----

void ForgotPasswordViewModel::_doSubmit() {
    if (m_step == QStringLiteral("username")) {
        validateUsername();
        if (!m_usernameError.isEmpty()) {
            _finishMockedOperation();
            return;
        }

        if (!authService()) {
            setFormError(QStringLiteral("Authentication service unavailable."));
            _finishMockedOperation();
            emit recoveryFailed(formError());
            return;
        }

        if (!authService()->userExists(m_username)) {
            setFormError(QStringLiteral("No account found with that username."));
            _finishMockedOperation();
            emit recoveryFailed(formError());
            return;
        }

        const QString q = authService()->securityQuestionFor(m_username);
        if (q.isEmpty()) {
            setFormError(QStringLiteral("This account has no security question configured."));
            _finishMockedOperation();
            emit recoveryFailed(formError());
            return;
        }

        m_securityQuestion = q;
        emit securityQuestionChanged(m_securityQuestion);
        setFormError({});
        _finishMockedOperation();
        _setStep(QStringLiteral("answer"));
        return;
    }

    // Step: answer
    validateSecurityAnswer();
    if (!m_securityAnswerError.isEmpty()) {
        _finishMockedOperation();
        return;
    }

    if (!authService()) {
        setFormError(QStringLiteral("Authentication service unavailable."));
        _finishMockedOperation();
        emit recoveryFailed(formError());
        return;
    }

    if (!authService()->verifySecurityAnswer(m_username, m_securityAnswer)) {
        setFormError(QStringLiteral("That answer doesn't match our records."));
        _finishMockedOperation();
        emit recoveryFailed(formError());
        return;
    }

    const QString token = authService()->issueResetToken(m_username);
    if (token.isEmpty()) {
        setFormError(QStringLiteral("Could not issue a reset token. Please try again."));
        _finishMockedOperation();
        emit recoveryFailed(formError());
        return;
    }

    m_resetToken = token;
    emit resetTokenChanged(m_resetToken);
    setFormError({});
    _finishMockedOperation();
    emit recoverySucceeded();
}

void ForgotPasswordViewModel::_doReset() {
    m_step = QStringLiteral("username");
    m_username.clear();
    m_securityQuestion.clear();
    m_securityAnswer.clear();
    m_resetToken.clear();
    m_usernameError.clear();
    m_securityAnswerError.clear();
    emit stepChanged(m_step);
    emit usernameChanged(m_username);
    emit securityQuestionChanged(m_securityQuestion);
    emit securityAnswerChanged(m_securityAnswer);
    emit resetTokenChanged(m_resetToken);
    emit usernameErrorChanged(m_usernameError);
    emit securityAnswerErrorChanged(m_securityAnswerError);
}

bool ForgotPasswordViewModel::_computeCanSubmit() const {
    if (isSubmitting()) return false;
    if (m_step == QStringLiteral("username")) return !m_username.trimmed().isEmpty();
    return !m_securityAnswer.trimmed().isEmpty();
}

} // namespace bookclub::client
