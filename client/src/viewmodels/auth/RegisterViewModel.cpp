// =============================================================================
//  RegisterViewModel.cpp
// =============================================================================
#include "viewmodels/auth/RegisterViewModel.h"

#include "common/Utils/ValidationUtils.h"

#include <algorithm>

namespace bookclub::client {

RegisterViewModel::RegisterViewModel(QObject* parent)
    : AuthViewModelBase(parent)
{}

// ----- AuthService attachment hook -----

void RegisterViewModel::_onAuthServiceChanged() {
    if (authService()) {
        emit availableSecurityQuestionsChanged(authService()->availableSecurityQuestions());
    }
}

// ----- Setters -----

void RegisterViewModel::setUsername(const QString& v) {
    if (m_username == v) return;
    m_username = v;
    emit usernameChanged(m_username);
    _setUsernameAvailable(false);
    if (m_usernameError.length() > 0) {
        m_usernameError.clear();
        emit usernameErrorChanged(m_usernameError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::setDisplayName(const QString& v) {
    if (m_displayName == v) return;
    m_displayName = v;
    emit displayNameChanged(m_displayName);
    if (m_displayNameError.length() > 0) {
        m_displayNameError.clear();
        emit displayNameErrorChanged(m_displayNameError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::setPassword(const QString& v) {
    if (m_password == v) return;
    m_password = v;
    emit passwordChanged(m_password);
    _recomputePasswordStrength();
    if (m_passwordError.length() > 0) {
        m_passwordError.clear();
        emit passwordErrorChanged(m_passwordError);
    }
    // Confirm password may need re-validation
    if (!m_confirmPassword.isEmpty()) validateConfirmPassword();
    _recomputeCanSubmit();
}

void RegisterViewModel::setConfirmPassword(const QString& v) {
    if (m_confirmPassword == v) return;
    m_confirmPassword = v;
    emit confirmPasswordChanged(m_confirmPassword);
    if (m_confirmPasswordError.length() > 0) {
        m_confirmPasswordError.clear();
        emit confirmPasswordErrorChanged(m_confirmPasswordError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::setSecurityQuestion(const QString& v) {
    if (m_securityQuestion == v) return;
    m_securityQuestion = v;
    emit securityQuestionChanged(m_securityQuestion);
    if (m_securityQuestionError.length() > 0) {
        m_securityQuestionError.clear();
        emit securityQuestionErrorChanged(m_securityQuestionError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::setSecurityAnswer(const QString& v) {
    if (m_securityAnswer == v) return;
    m_securityAnswer = v;
    emit securityAnswerChanged(m_securityAnswer);
    if (m_securityAnswerError.length() > 0) {
        m_securityAnswerError.clear();
        emit securityAnswerErrorChanged(m_securityAnswerError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::setAcceptTerms(bool v) {
    if (m_acceptTerms == v) return;
    m_acceptTerms = v;
    emit acceptTermsChanged(v);
    if (m_acceptTermsError.length() > 0) {
        m_acceptTermsError.clear();
        emit acceptTermsErrorChanged(m_acceptTermsError);
    }
    _recomputeCanSubmit();
}

// ----- Validation -----

void RegisterViewModel::validateUsername() {
    QString err;
    if (m_username.trimmed().isEmpty()) {
        err = QStringLiteral("Username is required.");
    } else if (!bookclub::common::ValidationUtils::isValidUsernameLength(m_username)) {
        err = QStringLiteral("Username must be 3–20 characters.");
    } else if (!bookclub::common::ValidationUtils::isValidUsername(m_username)) {
        err = QStringLiteral("Use letters, digits and underscores only.");
    } else if (authService() && !authService()->isUsernameAvailable(m_username)) {
        err = QStringLiteral("That username is already taken.");
    }
    if (err != m_usernameError) {
        m_usernameError = err;
        emit usernameErrorChanged(m_usernameError);
    }
    _setUsernameAvailable(err.isEmpty() && authService());
    _recomputeCanSubmit();
}

void RegisterViewModel::validateDisplayName() {
    QString err;
    const QString trimmed = m_displayName.trimmed();
    if (trimmed.isEmpty()) {
        err = QStringLiteral("Display name is required.");
    } else if (trimmed.length() > 50) {
        err = QStringLiteral("Display name must be ≤ 50 characters.");
    }
    if (err != m_displayNameError) {
        m_displayNameError = err;
        emit displayNameErrorChanged(m_displayNameError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::validatePassword() {
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

void RegisterViewModel::validateConfirmPassword() {
    QString err;
    if (m_confirmPassword.isEmpty()) {
        err = QStringLiteral("Please confirm your password.");
    } else if (m_confirmPassword != m_password) {
        err = QStringLiteral("Passwords do not match.");
    }
    if (err != m_confirmPasswordError) {
        m_confirmPasswordError = err;
        emit confirmPasswordErrorChanged(m_confirmPasswordError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::validateSecurityQuestion() {
    QString err;
    if (m_securityQuestion.isEmpty()) {
        err = QStringLiteral("Please choose a security question.");
    }
    if (err != m_securityQuestionError) {
        m_securityQuestionError = err;
        emit securityQuestionErrorChanged(m_securityQuestionError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::validateSecurityAnswer() {
    QString err;
    if (m_securityAnswer.trimmed().isEmpty()) {
        err = QStringLiteral("Security answer is required.");
    } else if (m_securityAnswer.trimmed().length() < 2) {
        err = QStringLiteral("Answer must be at least 2 characters.");
    }
    if (err != m_securityAnswerError) {
        m_securityAnswerError = err;
        emit securityAnswerErrorChanged(m_securityAnswerError);
    }
    _recomputeCanSubmit();
}

void RegisterViewModel::validateAcceptTerms() {
    QString err;
    if (!m_acceptTerms) {
        err = QStringLiteral("You must accept the terms to continue.");
    }
    if (err != m_acceptTermsError) {
        m_acceptTermsError = err;
        emit acceptTermsErrorChanged(m_acceptTermsError);
    }
    _recomputeCanSubmit();
}

// ----- Strength meter -----

void RegisterViewModel::_recomputePasswordStrength() {
    int score = 0;
    if (m_password.length() >= 6)  score++;
    if (m_password.length() >= 10) score++;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    const QString specials = QStringLiteral("!@#$%^&*()_+-=[]{}|;:,.<>?/");
    for (const QChar& ch : m_password) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
        else if (specials.contains(ch)) hasSpecial = true;
    }
    if (hasUpper && hasLower) score++;
    if (hasDigit && hasSpecial) score++;
    score = std::min(score, 4);

    QString label;
    switch (score) {
        case 0:  label = QStringLiteral("Too short");        break;
        case 1:  label = QStringLiteral("Weak");             break;
        case 2:  label = QStringLiteral("Fair");             break;
        case 3:  label = QStringLiteral("Good");             break;
        case 4:  label = QStringLiteral("Strong");           break;
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

void RegisterViewModel::_doSubmit() {
    validateUsername();
    validateDisplayName();
    validatePassword();
    validateConfirmPassword();
    validateSecurityQuestion();
    validateSecurityAnswer();
    validateAcceptTerms();

    if (!m_usernameError.isEmpty() ||
        !m_displayNameError.isEmpty() ||
        !m_passwordError.isEmpty() ||
        !m_confirmPasswordError.isEmpty() ||
        !m_securityQuestionError.isEmpty() ||
        !m_securityAnswerError.isEmpty() ||
        !m_acceptTermsError.isEmpty()) {
        _finishMockedOperation();
        return;
    }

    if (!authService()) {
        setFormError(QStringLiteral("Authentication service unavailable."));
        _finishMockedOperation();
        emit registerFailed(formError());
        return;
    }

    QString error;
    if (authService()->registerUser(m_username, m_displayName, m_password,
                                     m_securityQuestion, m_securityAnswer, error)) {
        _finishMockedOperation();
        emit registerSucceeded();
    } else {
        setFormError(error);
        _finishMockedOperation();
        emit registerFailed(error);
    }
}

void RegisterViewModel::_doReset() {
    m_username.clear();
    m_displayName.clear();
    m_password.clear();
    m_confirmPassword.clear();
    m_securityQuestion.clear();
    m_securityAnswer.clear();
    m_acceptTerms = false;
    m_usernameAvailable = false;
    m_passwordStrength = 0;
    m_strengthLabel.clear();
    m_usernameError.clear();
    m_displayNameError.clear();
    m_passwordError.clear();
    m_confirmPasswordError.clear();
    m_securityQuestionError.clear();
    m_securityAnswerError.clear();
    m_acceptTermsError.clear();

    emit usernameChanged(m_username);
    emit displayNameChanged(m_displayName);
    emit passwordChanged(m_password);
    emit confirmPasswordChanged(m_confirmPassword);
    emit securityQuestionChanged(m_securityQuestion);
    emit securityAnswerChanged(m_securityAnswer);
    emit acceptTermsChanged(m_acceptTerms);
    emit usernameAvailableChanged(m_usernameAvailable);
    emit passwordStrengthChanged(m_passwordStrength);
    emit strengthLabelChanged(m_strengthLabel);
    emit usernameErrorChanged(m_usernameError);
    emit displayNameErrorChanged(m_displayNameError);
    emit passwordErrorChanged(m_passwordError);
    emit confirmPasswordErrorChanged(m_confirmPasswordError);
    emit securityQuestionErrorChanged(m_securityQuestionError);
    emit securityAnswerErrorChanged(m_securityAnswerError);
    emit acceptTermsErrorChanged(m_acceptTermsError);
}

bool RegisterViewModel::_computeCanSubmit() const {
    if (isSubmitting()) return false;
    return !m_username.trimmed().isEmpty()
        && !m_displayName.trimmed().isEmpty()
        && !m_password.isEmpty()
        && !m_confirmPassword.isEmpty()
        && !m_securityQuestion.isEmpty()
        && !m_securityAnswer.trimmed().isEmpty()
        && m_acceptTerms;
}

QStringList RegisterViewModel::availableSecurityQuestions() const {
    return authService() ? authService()->availableSecurityQuestions() : QStringList{};
}

void RegisterViewModel::_setUsernameAvailable(bool v) {
    if (m_usernameAvailable == v) return;
    m_usernameAvailable = v;
    emit usernameAvailableChanged(v);
}

} // namespace bookclub::client
