// =============================================================================
//  LoginViewModel.cpp
// =============================================================================
#include "viewmodels/auth/LoginViewModel.h"

#include "common/Utils/ValidationUtils.h"

namespace bookclub::client {

LoginViewModel::LoginViewModel(QObject* parent)
    : AuthViewModelBase(parent)
{}

// ----- Setters -----

void LoginViewModel::setUsername(const QString& v) {
    if (m_username == v) return;
    m_username = v;
    emit usernameChanged(m_username);
    if (m_usernameError.length() > 0) {
        m_usernameError.clear();
        emit usernameErrorChanged(m_usernameError);
    }
    _recomputeCanSubmit();
}

void LoginViewModel::setPassword(const QString& v) {
    if (m_password == v) return;
    m_password = v;
    emit passwordChanged(m_password);
    if (m_passwordError.length() > 0) {
        m_passwordError.clear();
        emit passwordErrorChanged(m_passwordError);
    }
    _recomputeCanSubmit();
}

void LoginViewModel::setRememberMe(bool v) {
    if (m_rememberMe == v) return;
    m_rememberMe = v;
    emit rememberMeChanged(v);
}

// ----- Validation -----

void LoginViewModel::validateUsername() {
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

void LoginViewModel::validatePassword() {
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
    _recomputeCanSubmit();
}

// ----- Submit -----

void LoginViewModel::_doSubmit() {
    // Re-run full validation before calling the service.
    validateUsername();
    validatePassword();
    if (!m_usernameError.isEmpty() || !m_passwordError.isEmpty()) {
        _finishMockedOperation();
        return;
    }

    if (!authService()) {
        setFormError(QStringLiteral("Authentication service unavailable."));
        _finishMockedOperation();
        emit loginFailed(formError());
        return;
    }

    QString error;
    if (authService()->login(m_username, m_password, error)) {
        _finishMockedOperation();
        emit loginSucceeded();
    } else {
        setFormError(error);
        _finishMockedOperation();
        emit loginFailed(error);
    }
}

void LoginViewModel::_doReset() {
    m_username.clear();
    m_password.clear();
    m_rememberMe = false;
    m_usernameError.clear();
    m_passwordError.clear();
    emit usernameChanged(m_username);
    emit passwordChanged(m_password);
    emit rememberMeChanged(m_rememberMe);
    emit usernameErrorChanged(m_usernameError);
    emit passwordErrorChanged(m_passwordError);
}

bool LoginViewModel::_computeCanSubmit() const {
    if (isSubmitting()) return false;
    return !m_username.trimmed().isEmpty() && !m_password.isEmpty();
}

} // namespace bookclub::client
