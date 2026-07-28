// =============================================================================
//  LoginViewModel.cpp
// =============================================================================
#include "viewmodels/auth/LoginViewModel.h"

#include "common/Utils/ValidationUtils.h"
#include <QSettings>

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
    // BUG FIX (Issue 19): persist the `rememberMe` flag to QSettings
    // immediately so the checkbox state survives across sessions, even
    // if the user toggles it without submitting. If the user unchecks
    // it, also clear any previously-saved username/password (so they
    // don't get re-loaded on next launch). If the user re-checks it,
    // the username/password will be saved on the next successful login.
    QSettings settings("BookClub", "Login");
    settings.setValue("rememberMe", v);
    if (!v && !m_loadingSaved) {
        clearSavedCredentials();
        // Re-write the rememberMe flag because clearSavedCredentials
        // removes it from settings.
        settings.setValue("rememberMe", false);
    }
}

void LoginViewModel::loadSavedCredentials() {
    QSettings settings("BookClub", "Login");
    m_loadingSaved = true;
    bool remembered = settings.value("rememberMe", false).toBool();
    // BUG FIX (Issue 19): always load the `rememberMe` flag, not just
    // when it's true. This keeps the checkbox state in sync with what
    // was persisted on the previous toggle (see setRememberMe).
    m_rememberMe = remembered;
    emit rememberMeChanged(remembered);
    if (remembered) {
        QString savedUser = settings.value("username", "").toString();
        QString savedPass = settings.value("password", "").toString();
        if (!savedUser.isEmpty()) {
            m_username = savedUser;
            emit usernameChanged(m_username);
        }
        if (!savedPass.isEmpty()) {
            m_password = savedPass;
            emit passwordChanged(m_password);
        }
    }
    m_loadingSaved = false;
    _recomputeCanSubmit();
}

void LoginViewModel::saveCredentials() {
    QSettings settings("BookClub", "Login");
    settings.setValue("rememberMe", true);
    settings.setValue("username", m_username);
    settings.setValue("password", m_password);
}

void LoginViewModel::clearSavedCredentials() {
    QSettings settings("BookClub", "Login");
    settings.remove("rememberMe");
    settings.remove("username");
    settings.remove("password");
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
        // Save credentials if Remember Me is checked.
        if (m_rememberMe) {
            saveCredentials();
        }
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
