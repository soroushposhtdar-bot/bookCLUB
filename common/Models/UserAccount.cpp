#include "common/Models/UserAccount.h"

namespace bookclub::common {

UserAccount::UserAccount(QObject* parent) : QObject(parent) {}

UserAccount::UserAccount(const QString& id, const QString& username, QObject* parent)
    : QObject(parent), m_id(id), m_username(username) {}

// ---- Getter ----
const QString& UserAccount::id() const { return m_id; }
const QString& UserAccount::username() const { return m_username; }
const QString& UserAccount::passwordHash() const { return m_passwordHash; }
const QString& UserAccount::displayName() const { return m_displayName; }
const QString& UserAccount::email() const { return m_email; }
const QString& UserAccount::phone() const { return m_phone; }
const QString& UserAccount::securityQuestion() const { return m_securityQuestion; }
const QString& UserAccount::securityAnswerHash() const { return m_securityAnswerHash; }
const QStringList& UserAccount::tags() const { return m_tags; }
const QDateTime& UserAccount::createdAt() const { return m_createdAt; }
const QDateTime& UserAccount::updatedAt() const { return m_updatedAt; }
AccountStatus UserAccount::status() const { return m_status; }

// ---- Setter ----
void UserAccount::setId(const QString& id) { m_id = id; }
void UserAccount::setUsername(const QString& username) { m_username = username; }
void UserAccount::setPasswordHash(const QString& hash) { m_passwordHash = hash; }
void UserAccount::setDisplayName(const QString& displayName) {
    if (m_displayName != displayName) {
        m_displayName = displayName;
        emit profileChanged();
    }
}
void UserAccount::setEmail(const QString& email) {
    if (m_email != email) {
        m_email = email;
        emit profileChanged();
    }
}
void UserAccount::setPhone(const QString& phone) {
    if (m_phone != phone) {
        m_phone = phone;
        emit profileChanged();
    }
}
void UserAccount::setSecurityQuestion(const QString& question) { m_securityQuestion = question; }
void UserAccount::setSecurityAnswerHash(const QString& hash) { m_securityAnswerHash = hash; }
void UserAccount::setTags(const QStringList& tags) { m_tags = tags; }
void UserAccount::setCreatedAt(const QDateTime& createdAt) { m_createdAt = createdAt; }
void UserAccount::setUpdatedAt(const QDateTime& updatedAt) { m_updatedAt = updatedAt; }

void UserAccount::setStatus(AccountStatus status) {
    if (m_status != status) {
        m_status = status;
        emit statusChanged(status);
        emit profileChanged();
    }
}

// ---- Utility ----
bool UserAccount::isBlocked() const {
    return m_status == AccountStatus::Blocked;
}

bool UserAccount::canLogin() const {
    return m_status == AccountStatus::Active || m_status == AccountStatus::Pending;
}

bool UserAccount::requiresFirstGenreSetup() const {
    return m_status == AccountStatus::Pending;
}

} // namespace bookclub::common
