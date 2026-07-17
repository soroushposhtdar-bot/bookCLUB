// src/client/session/SessionManager.cpp
#include "src/client/session/SessionManager.h"
#include "common/Utils/Logger.h"
#include "common/Utils/DateTimeUtils.h"
#include "common/Utils/IdGenerator.h"

#include <QDateTime>

namespace bookclub::client {

SessionManager& SessionManager::instance() {
    static SessionManager instance;
    return instance;
}

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("SessionManager initialized");
    clearSession();
}

SessionManager::~SessionManager() {
    endSession();
}

// ---- Authentication State ----

bool SessionManager::isAuthenticated() const {
    QMutexLocker locker(&m_mutex);
    return !m_userId.isEmpty() && !m_username.isEmpty();
}

bool SessionManager::isSessionValid() const {
    QMutexLocker locker(&m_mutex);
    if (m_userId.isEmpty()) {
        return false;
    }
    if (m_sessionExpiry.isValid() && m_sessionExpiry < common::DateTimeUtils::nowUtc()) {
        LOG_WARNING("Session expired for user: " + m_username);
        return false;
    }
    return true;
}

// ---- Getter Methods ----

QString SessionManager::userId() const {
    QMutexLocker locker(&m_mutex);
    return m_userId;
}

QString SessionManager::username() const {
    QMutexLocker locker(&m_mutex);
    return m_username;
}

QString SessionManager::displayName() const {
    QMutexLocker locker(&m_mutex);
    return m_displayName.isEmpty() ? m_username : m_displayName;
}

QString SessionManager::email() const {
    QMutexLocker locker(&m_mutex);
    return m_email;
}

common::AccountRole SessionManager::role() const {
    QMutexLocker locker(&m_mutex);
    return m_role;
}

QString SessionManager::roleName() const {
    QMutexLocker locker(&m_mutex);
    switch (m_role) {
        case common::AccountRole::Admin:
            return "مدیر سیستم";
        case common::AccountRole::Publisher:
            return "ناشر";
        case common::AccountRole::User:
        default:
            return "کاربر عادی";
    }
}

QString SessionManager::sessionToken() const {
    QMutexLocker locker(&m_mutex);
    return m_sessionToken;
}

QDateTime SessionManager::loginTime() const {
    QMutexLocker locker(&m_mutex);
    return m_loginTime;
}

QDateTime SessionManager::sessionExpiry() const {
    QMutexLocker locker(&m_mutex);
    return m_sessionExpiry;
}

// ---- Setter Methods ----

void SessionManager::setUserInfo(const QString& userId,
                                 const QString& username,
                                 const QString& displayName,
                                 const QString& email,
                                 common::AccountRole role) {
    QMutexLocker locker(&m_mutex);
    m_userId = userId;
    m_username = username;
    m_displayName = displayName;
    m_email = email;
    m_role = role;
    emit userInfoChanged();
    LOG_DEBUG("User info updated for: " + username);
}

void SessionManager::setSessionToken(const QString& token) {
    QMutexLocker locker(&m_mutex);
    m_sessionToken = token;
    LOG_DEBUG("Session token updated");
}

void SessionManager::setLoginTime(const QDateTime& time) {
    QMutexLocker locker(&m_mutex);
    m_loginTime = time;
}

void SessionManager::setSessionExpiry(const QDateTime& expiry) {
    QMutexLocker locker(&m_mutex);
    m_sessionExpiry = expiry;
}

// ---- Session Management ----

void SessionManager::startSession(const QString& userId,
                                  const QString& username,
                                  const QString& displayName,
                                  const QString& email,
                                  common::AccountRole role,
                                  const QString& token) {
    QMutexLocker locker(&m_mutex);

    clearSession();

    m_userId = userId;
    m_username = username;
    m_displayName = displayName;
    m_email = email;
    m_role = role;

    if (token.isEmpty()) {
        m_sessionToken = common::IdGenerator::generateUuid();
    } else {
        m_sessionToken = token;
    }

    m_loginTime = common::DateTimeUtils::nowUtc();
    m_sessionExpiry = m_loginTime.addDays(7); // Session valid for 7 days

    LOG_INFO("Session started for user: " + username + " (ID: " + userId + ", Role: " + roleName() + ")");
    emit sessionStarted(userId);
    emit userInfoChanged();
}

void SessionManager::endSession() {
    QMutexLocker locker(&m_mutex);

    if (m_userId.isEmpty()) {
        return;
    }

    LOG_INFO("Session ended for user: " + m_username);
    clearSession();
    emit sessionEnded();
    emit userInfoChanged();
}

void SessionManager::refreshSession() {
    QMutexLocker locker(&m_mutex);

    if (m_userId.isEmpty()) {
        LOG_WARNING("Cannot refresh empty session");
        return;
    }

    m_sessionExpiry = common::DateTimeUtils::nowUtc().addDays(7);
    LOG_INFO("Session refreshed for user: " + m_username);
    emit sessionRefreshed();
}

// ---- Role Checks ----

bool SessionManager::isAdmin() const {
    QMutexLocker locker(&m_mutex);
    return m_role == common::AccountRole::Admin;
}

bool SessionManager::isPublisher() const {
    QMutexLocker locker(&m_mutex);
    return m_role == common::AccountRole::Publisher;
}

bool SessionManager::isRegularUser() const {
    QMutexLocker locker(&m_mutex);
    return m_role == common::AccountRole::User;
}

// ---- Utility ----

QString SessionManager::getDisplayNameOrUsername() const {
    QMutexLocker locker(&m_mutex);
    return m_displayName.isEmpty() ? m_username : m_displayName;
}

bool SessionManager::hasValidRole() const {
    QMutexLocker locker(&m_mutex);
    return m_role == common::AccountRole::User ||
           m_role == common::AccountRole::Publisher ||
           m_role == common::AccountRole::Admin;
}

// ---- Private Methods ----

void SessionManager::clearSession() {
    m_userId.clear();
    m_username.clear();
    m_displayName.clear();
    m_email.clear();
    m_role = common::AccountRole::User;
    m_sessionToken.clear();
    m_loginTime = QDateTime();
    m_sessionExpiry = QDateTime();
}

} // namespace bookclub::client
