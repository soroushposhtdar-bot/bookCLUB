#include "auth/AuthManager.h"
#include <QCryptographicHash>

AuthManager &AuthManager::instance()
{
    static AuthManager inst;
    return inst;
}

AuthManager::AuthManager()
    : m_settings("BookClub", "Auth")
{}

QString AuthManager::lastError() const
{
    return m_lastError;
}

QString AuthManager::hashPassword(const QString &password)
{
    return QCryptographicHash::hash(
               password.toUtf8(),
               QCryptographicHash::Sha256
           ).toHex();
}

bool AuthManager::registerUser(const QString &username,
                               const QString &email,
                               const QString &password,
                               const QString &securityQuestion,
                               const QString &securityAnswer)
{
    m_settings.beginGroup("users");
    const QStringList existing = m_settings.childGroups();
    m_settings.endGroup();

    for (const QString &u : existing) {
        if (u.compare(username, Qt::CaseInsensitive) == 0) {
            m_lastError = "Username is already taken.";
            return false;
        }
    }

    if (emailExists(email)) {
        m_lastError = "An account with that email already exists.";
        return false;
    }

    m_settings.beginGroup("users/" + username);
    m_settings.setValue("email",            email.toLower().trimmed());
    m_settings.setValue("password",         hashPassword(password));
    m_settings.setValue("securityQuestion", securityQuestion);
    m_settings.setValue("securityAnswer",   hashPassword(securityAnswer.toLower().trimmed()));
    m_settings.endGroup();

    return true;
}

bool AuthManager::loginUser(const QString &username, const QString &password)
{
    m_settings.beginGroup("users/" + username);
    const bool    exists = m_settings.contains("password");
    const QString stored = m_settings.value("password").toString();
    m_settings.endGroup();

    if (!exists) {
        m_lastError = "No account found with that username.";
        return false;
    }
    if (stored != hashPassword(password)) {
        m_lastError = "Incorrect password. Please try again.";
        return false;
    }
    return true;
}

bool AuthManager::emailExists(const QString &email) const
{
    m_settings.beginGroup("users");
    const QStringList users = m_settings.childGroups();
    m_settings.endGroup();

    const QString normalised = email.toLower().trimmed();
    for (const QString &u : users) {
        m_settings.beginGroup("users/" + u);
        const QString stored = m_settings.value("email").toString();
        m_settings.endGroup();
        if (stored == normalised)
            return true;
    }
    return false;
}

QString AuthManager::getUsernameByEmail(const QString &email) const
{
    m_settings.beginGroup("users");
    const QStringList users = m_settings.childGroups();
    m_settings.endGroup();

    const QString normalised = email.toLower().trimmed();
    for (const QString &u : users) {
        m_settings.beginGroup("users/" + u);
        const QString stored = m_settings.value("email").toString();
        m_settings.endGroup();
        if (stored == normalised)
            return u;
    }
    return {};
}

QString AuthManager::getSecurityQuestion(const QString &email) const
{
    const QString username = getUsernameByEmail(email);
    if (username.isEmpty())
        return {};

    m_settings.beginGroup("users/" + username);
    const QString q = m_settings.value("securityQuestion").toString();
    m_settings.endGroup();
    return q;
}

bool AuthManager::verifySecurityAnswer(const QString &email, const QString &answer)
{
    const QString username = getUsernameByEmail(email);
    if (username.isEmpty()) {
        m_lastError = "No account is registered with that email.";
        return false;
    }

    m_settings.beginGroup("users/" + username);
    const QString stored = m_settings.value("securityAnswer").toString();
    m_settings.endGroup();

    if (stored != hashPassword(answer.toLower().trimmed())) {
        m_lastError = "Incorrect answer. Please try again.";
        return false;
    }
    return true;
}

bool AuthManager::resetPassword(const QString &email, const QString &newPassword)
{
    const QString username = getUsernameByEmail(email);
    if (username.isEmpty()) {
        m_lastError = "No account is registered with that email.";
        return false;
    }
    m_settings.beginGroup("users/" + username);
    m_settings.setValue("password", hashPassword(newPassword));
    m_settings.endGroup();
    return true;
}
