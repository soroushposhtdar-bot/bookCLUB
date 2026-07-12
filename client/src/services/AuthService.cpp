// =============================================================================
//  AuthService.cpp — mocked authentication service
// =============================================================================
#include "services/AuthService.h"

#include "common/Utils/PasswordHasher.h"
#include "common/Utils/CryptoUtils.h"
#include "common/Utils/IdGenerator.h"

namespace bookclub::client {

AuthService::AuthService(QObject* parent)
    : QObject(parent)
{
    _seedDefaults();
}

// -----------------------------------------------------------------------------
//  Lookups
// -----------------------------------------------------------------------------

bool AuthService::userExists(const QString& username) const {
    return _users.contains(username.trimmed().toLower());
}

QString AuthService::securityQuestionFor(const QString& username) const {
    auto it = _users.constFind(username.trimmed().toLower());
    return it != _users.constEnd() ? it->securityQuestion : QString();
}

bool AuthService::verifySecurityAnswer(const QString& username, const QString& answer) const {
    auto it = _users.constFind(username.trimmed().toLower());
    if (it == _users.constEnd()) return false;

    // Compare the normalized answer against the stored hash.
    // Security answers are stored hashed (case-insensitively) at registration
    // time, so we normalize the input the same way before verifying.
    const QString normalizedAnswer = answer.trimmed().toLower();
    return bookclub::common::PasswordHasher::verify(normalizedAnswer, it->securityAnswerHash);
}

// -----------------------------------------------------------------------------
//  Authentication
// -----------------------------------------------------------------------------

bool AuthService::login(const QString& username, const QString& password, QString& errorMessage) {
    const QString key = username.trimmed().toLower();
    auto it = _users.constFind(key);
    if (it == _users.constEnd()) {
        errorMessage = QStringLiteral("No account found with that username.");
        return false;
    }
    if (it->status == QStringLiteral("Blocked")) {
        errorMessage = QStringLiteral("This account has been blocked. Please contact support.");
        return false;
    }
    if (!bookclub::common::PasswordHasher::verify(password, it->passwordHash)) {
        errorMessage = QStringLiteral("Incorrect password. Please try again.");
        return false;
    }

    // Track current session state for QML.
    _currentUsername    = it->username;
    _currentDisplayName = it->displayName;
    _currentRole        = it->role;
    emit currentUsernameChanged();
    emit currentDisplayNameChanged();
    emit currentRoleChanged();
    return true;
}

void AuthService::logout() {
    _currentUsername.clear();
    _currentDisplayName.clear();
    _currentRole.clear();
    emit currentUsernameChanged();
    emit currentDisplayNameChanged();
    emit currentRoleChanged();
}

bool AuthService::registerUser(const QString& username,
                                const QString& displayName,
                                const QString& password,
                                const QString& securityQuestion,
                                const QString& securityAnswer,
                                QString& errorMessage) {
    const QString key = username.trimmed().toLower();
    if (_users.contains(key)) {
        errorMessage = QStringLiteral("That username is already taken.");
        return false;
    }

    MockUser u;
    u.id = bookclub::common::IdGenerator::generateUuid();
    u.username = username.trimmed();
    u.displayName = displayName.trimmed();
    u.passwordHash = bookclub::common::PasswordHasher::hash(password);
    u.securityQuestion = securityQuestion;
    u.securityAnswerHash = bookclub::common::PasswordHasher::hash(securityAnswer.trimmed().toLower());
    u.requiresGenreSetup = true;
    _users.insert(key, u);
    return true;
}

// -----------------------------------------------------------------------------
//  Password recovery
// -----------------------------------------------------------------------------

QString AuthService::issueResetToken(const QString& username) {
    const QString key = username.trimmed().toLower();
    if (!_users.contains(key)) return {};

    // 32-byte random hex string
    const QByteArray bytes = bookclub::common::CryptoUtils::generateRandomBytes(32);
    const QString token = QString::fromLatin1(bytes.toHex());
    _resetTokens.insert(key, token);
    return token;
}

bool AuthService::resetPassword(const QString& username,
                                const QString& resetToken,
                                const QString& newPassword,
                                QString& errorMessage) {
    const QString key = username.trimmed().toLower();
    auto it = _users.find(key);
    if (it == _users.end()) {
        errorMessage = QStringLiteral("No account found with that username.");
        return false;
    }
    auto tokIt = _resetTokens.constFind(key);
    if (tokIt == _resetTokens.constEnd() || *tokIt != resetToken) {
        errorMessage = QStringLiteral("Invalid or expired reset token.");
        return false;
    }
    it->passwordHash = bookclub::common::PasswordHasher::hash(newPassword);
    _resetTokens.remove(key);
    return true;
}

// -----------------------------------------------------------------------------
//  Genre selection
// -----------------------------------------------------------------------------

bool AuthService::saveGenreSelection(const QString& username, const QStringList& genres) {
    const QString key = username.trimmed().toLower();
    auto it = _users.find(key);
    if (it == _users.end()) return false;
    it->selectedGenres = genres;
    it->requiresGenreSetup = false;
    return true;
}

bool AuthService::requiresGenreSetup(const QString& username) const {
    auto it = _users.constFind(username.trimmed().toLower());
    return it != _users.constEnd() ? it->requiresGenreSetup : false;
}

// -----------------------------------------------------------------------------
//  Predefined values
// -----------------------------------------------------------------------------

QStringList AuthService::availableSecurityQuestions() const {
    return {
        QStringLiteral("What was the name of your first pet?"),
        QStringLiteral("In what city were you born?"),
        QStringLiteral("What is your mother's maiden name?"),
        QStringLiteral("What was the make of your first car?"),
        QStringLiteral("What was the title of your favourite childhood book?")
    };
}

QStringList AuthService::availableGenres() const {
    return {
        QStringLiteral("Fiction"),
        QStringLiteral("Non-Fiction"),
        QStringLiteral("Mystery"),
        QStringLiteral("Thriller"),
        QStringLiteral("Romance"),
        QStringLiteral("Science Fiction"),
        QStringLiteral("Fantasy"),
        QStringLiteral("Historical Fiction"),
        QStringLiteral("Biography"),
        QStringLiteral("Self-Help"),
        QStringLiteral("Business"),
        QStringLiteral("Technology"),
        QStringLiteral("Poetry"),
        QStringLiteral("Young Adult"),
        QStringLiteral("Children's")
    };
}

bool AuthService::isUsernameAvailable(const QString& username) const {
    if (username.trimmed().isEmpty()) return false;
    return !_users.contains(username.trimmed().toLower());
}

// -----------------------------------------------------------------------------
//  Seeding
// -----------------------------------------------------------------------------

void AuthService::seedDemoUser(const QString& username,
                                const QString& displayName,
                                const QString& password,
                                const QString& securityQuestion,
                                const QString& securityAnswer,
                                const QString& role) {
    MockUser u;
    u.id = bookclub::common::IdGenerator::generateUuid();
    u.username = username.trimmed();
    u.displayName = displayName.trimmed();
    u.passwordHash = bookclub::common::PasswordHasher::hash(password);
    u.securityQuestion = securityQuestion;
    u.securityAnswerHash = bookclub::common::PasswordHasher::hash(securityAnswer.trimmed().toLower());
    u.requiresGenreSetup = false;
    u.selectedGenres = { QStringLiteral("Fiction"), QStringLiteral("Mystery"), QStringLiteral("Fantasy") };
    u.role = role;
    _users.insert(username.trimmed().toLower(), u);
}

void AuthService::_seedDefaults() {
    // Reader (regular user) accounts
    seedDemoUser(QStringLiteral("alice"),
                 QStringLiteral("Alice Reader"),
                 QStringLiteral("password123"),
                 QStringLiteral("What was the name of your first pet?"),
                 QStringLiteral("whiskers"),
                 QStringLiteral("user"));
    seedDemoUser(QStringLiteral("bob"),
                 QStringLiteral("Bob Bibliophile"),
                 QStringLiteral("password123"),
                 QStringLiteral("In what city were you born?"),
                 QStringLiteral("london"),
                 QStringLiteral("user"));

    // Publisher demo account — opens the Publisher dashboard
    seedDemoUser(QStringLiteral("publisher"),
                 QStringLiteral("Penguin Press"),
                 QStringLiteral("password123"),
                 QStringLiteral("What was the title of your favourite childhood book?"),
                 QStringLiteral("alice"),
                 QStringLiteral("publisher"));

    // Admin demo account — opens the Admin moderation panel
    seedDemoUser(QStringLiteral("admin"),
                 QStringLiteral("System Admin"),
                 QStringLiteral("password123"),
                 QStringLiteral("What was the make of your first car?"),
                 QStringLiteral("volvo"),
                 QStringLiteral("admin"));

    // Server operator demo account — opens the Server dashboard
    seedDemoUser(QStringLiteral("server"),
                 QStringLiteral("Server Operator"),
                 QStringLiteral("password123"),
                 QStringLiteral("What is your mother's maiden name?"),
                 QStringLiteral("smith"),
                 QStringLiteral("server"));

    // Blocked demo account — for testing the "blocked users can't access" spec
    seedDemoUser(QStringLiteral("blocked"),
                 QStringLiteral("Blocked User"),
                 QStringLiteral("password123"),
                 QStringLiteral("What was the name of your first pet?"),
                 QStringLiteral("blocked"),
                 QStringLiteral("user"));
    // Manually set the blocked user's status
    auto it = _users.find(QStringLiteral("blocked"));
    if (it != _users.end()) it->status = QStringLiteral("Blocked");
}

} // namespace bookclub::client
