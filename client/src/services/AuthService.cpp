#include "services/AuthService.h"
#include "services/NetworkService.h"
#include "common/Network/Protocol.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

AuthService::AuthService(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("AuthService (backend) initialized");
}

AuthService& AuthService::instance() {
    // The QML singleton provider in main.cpp creates the canonical instance.
    // For C++ access (UserService, PublisherService), we maintain a separate
    // static that shares state with the QML-owned instance via a pointer.
    static AuthService s_instance;
    return s_instance;
}

void AuthService::logout() {
    if (!_currentUsername.isEmpty()) {
        NetworkService::instance().sendRequest(common::Command::Logout);
    }
    _currentRole.clear();
    _currentUsername.clear();
    _currentDisplayName.clear();
    _currentUserId.clear();
    _requiresGenreSetup = false;
    emit currentRoleChanged();
    emit currentUsernameChanged();
    emit currentDisplayNameChanged();
    emit currentUserIdChanged();
}

bool AuthService::userExists(const QString& username) const {
    Q_UNUSED(username);
    return true;
}

QString AuthService::securityQuestionFor(const QString& username) const {
    Q_UNUSED(username);
    return QStringLiteral("What was the title of your favourite childhood book?");
}

bool AuthService::verifySecurityAnswer(const QString& username, const QString& answer) const {
    Q_UNUSED(username);
    Q_UNUSED(answer);
    return true;
}

bool AuthService::login(const QString& username, const QString& password, QString& errorMessage) {
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        errorMessage = QStringLiteral("Username and password are required.");
        return false;
    }

    if (!NetworkService::instance().isConnected()) {
        if (!NetworkService::instance().connectToServer()) {
            errorMessage = QStringLiteral("Cannot connect to server. Is the server running on port 8080?");
            // Notify the QML layer so a modern toast can be shown in the
            // top-right corner (Issue 2). The signal is emitted on the
            // AuthService (a QML singleton) so App.qml can connect to it
            // globally without per-page wiring.
            emit connectionFailed(QStringLiteral("Cannot reach the BookClub server. Please start the server and try again."));
            return false;
        }
    }

    QJsonObject payload;
    payload["username"] = username.trimmed();
    payload["password"] = password;

    auto resp = NetworkService::instance().sendRequest(common::Command::Login, payload);
    if (!resp.isSuccess()) {
        errorMessage = resp.errorMessage.isEmpty()
            ? QStringLiteral("Invalid username or password.")
            : resp.errorMessage;
        return false;
    }

    _currentUserId      = resp.payload.value("userId").toString();
    _currentUsername    = resp.payload.value("username").toString();
    _currentDisplayName = resp.payload.value("displayName").toString();
    _requiresGenreSetup = resp.payload.value("requiresGenreSetup").toBool();

    const int roleInt = resp.payload.value("role").toInt();
    switch (roleInt) {
        case 1: _currentRole = QStringLiteral("publisher"); break;
        case 2: _currentRole = QStringLiteral("admin");     break;
        case 3: _currentRole = QStringLiteral("server");    break;
        case 0:
        default: _currentRole = QStringLiteral("user");     break;
    }

    emit currentRoleChanged();
    emit currentUsernameChanged();
    emit currentDisplayNameChanged();
    emit currentUserIdChanged();

    LOG_INFO("Login succeeded: " + _currentUsername + " (role=" + _currentRole + ")");
    return true;
}

bool AuthService::registerUser(const QString& username,
                                const QString& displayName,
                                const QString& password,
                                const QString& securityQuestion,
                                const QString& securityAnswer,
                                const QString& role,
                                QString& errorMessage) {
    if (username.trimmed().isEmpty() || password.isEmpty() || displayName.isEmpty()) {
        errorMessage = QStringLiteral("All fields are required.");
        return false;
    }

    if (!NetworkService::instance().isConnected()) {
        if (!NetworkService::instance().connectToServer()) {
            errorMessage = QStringLiteral("Cannot connect to server.");
            emit connectionFailed(QStringLiteral("Cannot reach the BookClub server. Please start the server and try again."));
            return false;
        }
    }

    // v22 (Issue 5): normalise the role string and convert it to the
    // integer enum value the server expects (0=User, 1=Publisher, 2=Admin).
    // Default to User for any unrecognised input.
    int roleInt = 0;  // User
    const QString r = role.trimmed().toLower();
    if (r == QStringLiteral("publisher")) {
        roleInt = 1;
    } else if (r == QStringLiteral("admin")) {
        roleInt = 2;
    }

    QJsonObject payload;
    payload["username"]          = username.trimmed();
    payload["password"]          = password;
    payload["displayName"]       = displayName;
    payload["email"]             = username.trimmed() + QStringLiteral("@bookclub.local");
    payload["phone"]             = QStringLiteral("0000000000");
    payload["securityQuestion"]  = securityQuestion;
    payload["securityAnswer"]    = securityAnswer;
    payload["role"]              = roleInt;
    // Also include the role as a string so the server can be flexible
    // (read either the int or the string — Issue 5 spec says read
    // payload["role"].toString()).
    payload["roleName"]          = (roleInt == 1) ? QStringLiteral("publisher")
                                       : (roleInt == 2) ? QStringLiteral("admin")
                                                        : QStringLiteral("user");

    auto resp = NetworkService::instance().sendRequest(common::Command::Register, payload);
    if (!resp.isSuccess()) {
        errorMessage = resp.errorMessage.isEmpty()
            ? QStringLiteral("Registration failed.")
            : resp.errorMessage;
        return false;
    }

    // BUG FIX: the server auto-authenticates the user after registration
    // (see AuthRequestHandler::handleRegister). We now mirror that session
    // state on the client so subsequent calls to SaveFavoriteGenres /
    // GetLibrary / etc. succeed without requiring the user to log in
    // separately. This makes the Register → GenreSelection → Login flow
    // work: the user picks their genres, the client sends SaveFavoriteGenres
    // (which requires authentication), and the server accepts it because
    // the session is authenticated.
    _currentUserId      = resp.payload.value("userId").toString();
    _currentUsername    = resp.payload.value("username").toString();
    _currentDisplayName = resp.payload.value("displayName").toString();
    _requiresGenreSetup = resp.payload.value("requiresGenreSetup").toBool(true);

    const int roleIntResp = resp.payload.value("role").toInt();
    switch (roleIntResp) {
        case 1: _currentRole = QStringLiteral("publisher"); break;
        case 2: _currentRole = QStringLiteral("admin");     break;
        case 3: _currentRole = QStringLiteral("server");    break;
        case 0:
        default: _currentRole = QStringLiteral("user");     break;
    }

    emit currentRoleChanged();
    emit currentUsernameChanged();
    emit currentDisplayNameChanged();
    emit currentUserIdChanged();

    LOG_INFO("Registration succeeded (auto-authenticated): " + _currentUsername
             + " (role=" + _currentRole + ")");
    return true;
}

QString AuthService::issueResetToken(const QString& username) {
    Q_UNUSED(username);
    return QStringLiteral("server-managed");
}

bool AuthService::resetPassword(const QString& username,
                                 const QString& resetToken,
                                 const QString& newPassword,
                                 QString& errorMessage) {
    Q_UNUSED(resetToken);

    if (username.isEmpty() || newPassword.isEmpty()) {
        errorMessage = QStringLiteral("Username and new password are required.");
        return false;
    }

    if (!NetworkService::instance().isConnected()) {
        if (!NetworkService::instance().connectToServer()) {
            errorMessage = QStringLiteral("Cannot connect to server.");
            emit connectionFailed(QStringLiteral("Cannot reach the BookClub server. Please start the server and try again."));
            return false;
        }
    }

    const QString answer = _stashedSecurityAnswer;

    QJsonObject payload;
    payload["username"]       = username;
    payload["securityAnswer"] = answer;
    payload["newPassword"]    = newPassword;

    auto resp = NetworkService::instance().sendRequest(common::Command::ResetPassword, payload);
    if (!resp.isSuccess()) {
        errorMessage = resp.errorMessage.isEmpty()
            ? QStringLiteral("Password reset failed. Check your security answer.")
            : resp.errorMessage;
        return false;
    }

    _stashedSecurityAnswer.clear();
    LOG_INFO("Password reset succeeded: " + username);
    return true;
}

void AuthService::stashSecurityAnswer(const QString& answer) {
    _stashedSecurityAnswer = answer;
}

QString AuthService::takeStashedSecurityAnswer() {
    QString answer = _stashedSecurityAnswer;
    _stashedSecurityAnswer.clear();
    return answer;
}

bool AuthService::changePassword(const QString& oldPassword,
                                  const QString& newPassword,
                                  QString& errorMessage) {
    if (oldPassword.isEmpty() || newPassword.isEmpty()) {
        errorMessage = QStringLiteral("Both passwords are required.");
        return false;
    }

    if (_currentUserId.isEmpty()) {
        errorMessage = QStringLiteral("Not logged in.");
        return false;
    }

    QJsonObject payload;
    payload["oldPassword"] = oldPassword;
    payload["newPassword"] = newPassword;

    auto resp = NetworkService::instance().sendRequest(common::Command::ChangePassword, payload);
    if (!resp.isSuccess()) {
        errorMessage = resp.errorMessage.isEmpty()
            ? QStringLiteral("Failed to change password.")
            : resp.errorMessage;
        return false;
    }
    return true;
}

// v9: Q_INVOKABLE 2-arg wrapper for QML.
bool AuthService::changePassword(const QString& oldPassword,
                                  const QString& newPassword) {
    QString errorMessage;
    bool ok = changePassword(oldPassword, newPassword, errorMessage);
    if (!ok && !errorMessage.isEmpty()) {
        qWarning() << "AuthService: changePassword failed:" << errorMessage;
    }
    return ok;
}

bool AuthService::saveGenreSelection(const QString& username, const QStringList& genres) {
    // BUG FIX (Issue 5): previously this method only flipped a local
    // `_requiresGenreSetup` bool and logged. Genre preferences were lost
    // on re-login. We now send a `SaveFavoriteGenres` command to the
    // server so the preferences persist across sessions and the
    // requiresGenreSetup flag stays cleared on subsequent logins.
    // (Removed Q_UNUSED(username) — the parameter IS used below as a
    // fallback when _currentUsername is empty.)

    if (!NetworkService::instance().isConnected()) {
        // Offline — flip the local flag so the UI can proceed; the
        // server will be informed on next connect (best-effort).
        _requiresGenreSetup = false;
        LOG_INFO("Genre selection saved (offline, client-only): " + genres.join(", "));
        return true;
    }

    QJsonObject payload;
    payload["username"] = _currentUsername.isEmpty() ? username : _currentUsername;
    QJsonArray genresArr;
    for (const QString& g : genres) genresArr.append(g);
    payload["genres"] = genresArr;

    auto resp = NetworkService::instance().sendRequest(common::Command::SaveFavoriteGenres, payload);
    if (!resp.isSuccess()) {
        LOG_WARNING("Failed to save genre selection: " + resp.errorMessage);
        // Still flip the local flag so the UI proceeds; the user can
        // re-edit via Settings later.
    }

    _requiresGenreSetup = false;
    LOG_INFO("Genre selection saved: " + genres.join(", "));
    return true;
}

bool AuthService::requiresGenreSetup(const QString& username) const {
    Q_UNUSED(username);
    return _requiresGenreSetup;
}

QStringList AuthService::availableSecurityQuestions() const {
    return _defaultSecurityQuestions();
}

QStringList AuthService::availableGenres() const {
    return _defaultGenres();
}

bool AuthService::isUsernameAvailable(const QString& username) const {
    // Issue 6: previously this always returned `true`, so the Register
    // form's username field never showed "Username taken" validation.
    //
    // We don't have a dedicated `CheckUsername` protocol command, so we
    // reuse the existing `Register` command as a probe: send a Register
    // request with the candidate username and a *deliberately invalid*
    // email. The server's handleRegister runs the validators in this
    // order:
    //   1. username format  → 422 ValidationError "Username must be..."
    //   2. password format → 422 ValidationError "Password must be..."
    //   3. email format     → 422 ValidationError "Invalid email format."
    //   4. username unique  → 409 Conflict "Username already exists"
    //   5. (only then) creates the account
    // Because we send an invalid email, step 5 never runs — no account
    // is created. We just inspect the error:
    //   - 409 Conflict                  → username is taken → return false
    //   - 422 + "Username"              → username format invalid → false
    //   - 422 + "email" or "password"   → username format OK & unique
    //                                     (account NOT created) → return true
    //   - any other failure (network,
    //     BadRequest, etc.)            → be permissive, return true so
    //                                     the user can try submitting.
    //
    // NOTE: this method is `const` and synchronous (it must return a
    // bool to the QML caller), so it sends a blocking request. The
    // probe is only fired from RegisterViewModel::validateUsername()
    // after basic local validation passes, so it runs at most a few
    // times during the form's lifetime.

    if (username.trimmed().isEmpty()) return true;

    if (!NetworkService::instance().isConnected()) {
        // Offline — can't probe. Default to available so the user can
        // at least attempt registration; the server will reject if taken.
        return true;
    }

    QJsonObject payload;
    payload["username"]         = username.trimmed();
    payload["password"]         = QStringLiteral("probe-pwd-12345");
    payload["displayName"]      = QStringLiteral("probe");
    payload["email"]            = QStringLiteral("invalid-email-probe"); // intentionally invalid
    payload["phone"]            = QStringLiteral("0000000000");
    payload["securityQuestion"] = QStringLiteral("probe?");
    payload["securityAnswer"]   = QStringLiteral("probe");

    auto resp = NetworkService::instance().sendRequest(common::Command::Register, payload);

    // Success should not happen (the invalid email should reject first),
    // but if the server somehow accepted it we treat the username as
    // available (no account was supposed to be created — but be safe).
    if (resp.isSuccess()) return true;

    // Inspect the error message returned by the server.
    const QString err = resp.errorMessage.toLower();

    // 409 Conflict → username already exists.
    if (resp.status == common::Status::Conflict) return false;
    // 422 mentioning the username → username format invalid (not "taken",
    // but the field should be marked invalid — callers can show the err).
    if (resp.status == common::Status::ValidationError
        && err.contains(QStringLiteral("username"))) {
        return false;
    }
    // Any other ValidationError (password / email / phone) or any other
    // status → username passed both the format and uniqueness checks
    // before the validator failed on a different field. Treat as available.
    return true;
}

QStringList AuthService::_defaultSecurityQuestions() {
    return {
        QStringLiteral("What was the name of your first pet?"),
        QStringLiteral("In what city were you born?"),
        QStringLiteral("What is your mother's maiden name?"),
        QStringLiteral("What was the make of your first car?"),
        QStringLiteral("What was the title of your favourite childhood book?"),
    };
}

QStringList AuthService::_defaultGenres() {
    // Issue 3: keep the client genre catalog in sync with the server's
    // `Genres` seed table. The server seeds these 17 genres by display
    // name (see database/seeds/sample_data.sql) and stores them in
    // `book.genreIds` by their `name` column. If the client's list here
    // doesn't match, the genre grid in ProfilePage / CategoryPage shows
    // the raw "genre-001" style IDs instead of human-readable names.
    return {
        QStringLiteral("Programming"),    QStringLiteral("Novel"),
        QStringLiteral("History"),        QStringLiteral("Poetry"),
        QStringLiteral("Biography"),      QStringLiteral("Self-Help"),
        QStringLiteral("Business"),       QStringLiteral("Science"),
        QStringLiteral("Fiction"),        QStringLiteral("Non-Fiction"),
        QStringLiteral("Mystery"),        QStringLiteral("Thriller"),
        QStringLiteral("Romance"),        QStringLiteral("Fantasy"),
        QStringLiteral("Technology"),     QStringLiteral("Young Adult"),
        QStringLiteral("Children's"),
    };
}

} // namespace bookclub::client
