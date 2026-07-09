// =============================================================================
//  ResetPasswordViewModel.cpp
// =============================================================================
#include "viewmodels/auth/ResetPasswordViewModel.h"

#include "common/Utils/ValidationUtils.h"

#include <algorithm>

namespace bookclub::client {

ResetPasswordViewModel::ResetPasswordViewModel(QObject* parent)
    : AuthViewModelBase(parent)
{
    // Initialise requirements status map
    m_requirementsStatus = {
        { QStringLiteral("minLength"), false },
        { QStringLiteral("caseMix"),   false },
        { QStringLiteral("digit"),     false },
        { QStringLiteral("special"),   false }
    };
}

// ----- Setters -----

void ResetPasswordViewModel::setUsername(const QString& v) {
    if (m_username == v) return;
    m_username = v;
    emit usernameChanged(m_username);
}

void ResetPasswordViewModel::setResetToken(const QString& v) {
    if (m_resetToken == v) return;
    m_resetToken = v;
    emit resetTokenChanged(m_resetToken);
    _recomputeCanSubmit();
}

void ResetPasswordViewModel::setPassword(const QString& v) {
    if (m_password == v) return;
    m_password = v;
    emit passwordChanged(m_password);
    _recomputeStrengthAndRequirements();
    if (m_passwordError.length() > 0) {
        m_passwordError.clear();
        emit passwordErrorChanged(m_passwordError);
    }
    if (!m_confirmPassword.isEmpty()) validateConfirmPassword();
    _recomputeCanSubmit();
}

void ResetPasswordViewModel::setConfirmPassword(const QString& v) {
    if (m_confirmPassword == v) return;
    m_confirmPassword = v;
    emit confirmPasswordChanged(m_confirmPassword);
    if (m_confirmPasswordError.length() > 0) {
        m_confirmPasswordError.clear();
        emit confirmPasswordErrorChanged(m_confirmPasswordError);
    }
    _recomputeCanSubmit();
}

// ----- Validation -----

void ResetPasswordViewModel::validatePassword() {
    QString err;
    if (m_password.isEmpty()) {
        err = QStringLiteral("Password is required.");
    } else if (!bookclub::common::ValidationUtils::isValidPassword(m_password)) {
        err = QStringLiteral("Password must be 6–64 characters.");
    }
    if (err != m_passwordError) {
        m_passwordError = err;
        emit passwordErrorChanged(m_passwordError);
    }
    if (!m_confirmPassword.isEmpty()) validateConfirmPassword();
    _recomputeCanSubmit();
}

void ResetPasswordViewModel::validateConfirmPassword() {
    QString err;
    if (m_confirmPassword.isEmpty()) {
        err = QStringLiteral("Please confirm your new password.");
    } else if (m_confirmPassword != m_password) {
        err = QStringLiteral("Passwords do not match.");
    }
    if (err != m_confirmPasswordError) {
        m_confirmPasswordError = err;
        emit confirmPasswordErrorChanged(m_confirmPasswordError);
    }
    _recomputeCanSubmit();
}

// ----- Strength + requirements -----

void ResetPasswordViewModel::_recomputeStrengthAndRequirements() {
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    const QString specials = QStringLiteral("!@#$%^&*()_+-=[]{}|;:,.<>?/");
    for (const QChar& ch : m_password) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
        else if (specials.contains(ch)) hasSpecial = true;
    }

    QVariantMap newStatus;
    newStatus.insert(QStringLiteral("minLength"), m_password.length() >= 6);
    newStatus.insert(QStringLiteral("caseMix"),   hasUpper && hasLower);
    newStatus.insert(QStringLiteral("digit"),     hasDigit);
    newStatus.insert(QStringLiteral("special"),   hasSpecial);

    if (newStatus != m_requirementsStatus) {
        m_requirementsStatus = newStatus;
        emit requirementsStatusChanged(m_requirementsStatus);
    }

    int score = 0;
    if (m_password.length() >= 6)  score++;
    if (m_password.length() >= 10) score++;
    if (hasUpper && hasLower) score++;
    if (hasDigit && hasSpecial) score++;
    score = std::min(score, 4);

    QString label;
    switch (score) {
        case 0:  label = QStringLiteral("Too short"); break;
        case 1:  label = QStringLiteral("Weak");      break;
        case 2:  label = QStringLiteral("Fair");      break;
        case 3:  label = QStringLiteral("Good");      break;
        case 4:  label = QStringLiteral("Strong");    break;
    }

    if (m_passwordStrength != score) {
        m_passwordStrength = score;
        emit passwordStrengthChanged(score);
    }
    if (m_strengthLabel != label) {
        m_strengthLabel = label;
        emit strengthLabelChanged(label);
    }
}

// ----- Submit / reset / canSubmit -----

void ResetPasswordViewModel::_doSubmit() {
    validatePassword();
    validateConfirmPassword();

    if (!m_passwordError.isEmpty() || !m_confirmPasswordError.isEmpty()) {
        _finishMockedOperation();
        return;
    }

    if (!authService()) {
        setFormError(QStringLiteral("Authentication service unavailable."));
        _finishMockedOperation();
        emit resetFailed(formError());
        return;
    }

    if (m_username.isEmpty() || m_resetToken.isEmpty()) {
        setFormError(QStringLiteral("Missing recovery context. Please start over."));
        _finishMockedOperation();
        emit resetFailed(formError());
        return;
    }

    QString error;
    if (authService()->resetPassword(m_username, m_resetToken, m_password, error)) {
        setFormError({});
        _finishMockedOperation();
        emit resetSucceeded();
    } else {
        setFormError(error);
        _finishMockedOperation();
        emit resetFailed(error);
    }
}

void ResetPasswordViewModel::_doReset() {
    m_password.clear();
    m_confirmPassword.clear();
    m_passwordStrength = 0;
    m_strengthLabel.clear();
    m_passwordError.clear();
    m_confirmPasswordError.clear();
    emit passwordChanged(m_password);
    emit confirmPasswordChanged(m_confirmPassword);
    emit passwordStrengthChanged(m_passwordStrength);
    emit strengthLabelChanged(m_strengthLabel);
    emit passwordErrorChanged(m_passwordError);
    emit confirmPasswordErrorChanged(m_confirmPasswordError);
    _recomputeStrengthAndRequirements();
}

bool ResetPasswordViewModel::_computeCanSubmit() const {
    if (isSubmitting()) return false;
    return !m_username.isEmpty()
        && !m_resetToken.isEmpty()
        && !m_password.isEmpty()
        && !m_confirmPassword.isEmpty();
}

} // namespace bookclub::client
