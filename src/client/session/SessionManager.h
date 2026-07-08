// src/client/session/SessionManager.h
#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>

#include "common/AppEnums.h"

namespace bookclub::client {

class SessionManager : public QObject {
    Q_OBJECT
public:
    static SessionManager& instance();

    // --- Authentication State ---
    bool isAuthenticated() const;
    bool isSessionValid() const;

    // --- Getter Methods ---
    QString userId() const;
    QString username() const;
    QString displayName() const;
    QString email() const;
    common::AccountRole role() const;
    QString roleName() const;
    QString sessionToken() const;
    QDateTime loginTime() const;
    QDateTime sessionExpiry() const;

    // --- Setter Methods ---
    void setUserInfo(const QString& userId,
                     const QString& username,
                     const QString& displayName,
                     const QString& email,
                     common::AccountRole role);
    void setSessionToken(const QString& token);
    void setLoginTime(const QDateTime& time);
    void setSessionExpiry(const QDateTime& expiry);

    // --- Session Management ---
    void startSession(const QString& userId,
                      const QString& username,
                      const QString& displayName,
                      const QString& email,
                      common::AccountRole role,
                      const QString& token = {});
    void endSession();
    void refreshSession();

    // --- Role Checks ---
    bool isAdmin() const;
    bool isPublisher() const;
    bool isRegularUser() const;

    // --- Utility ---
    QString getDisplayNameOrUsername() const;
    bool hasValidRole() const;

signals:
    void sessionStarted(const QString& userId);
    void sessionEnded();
    void sessionRefreshed();
    void userInfoChanged();

private:
    SessionManager(QObject* parent = nullptr);
    ~SessionManager() override;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    void clearSession();

    // Session Data
    QString m_userId;
    QString m_username;
    QString m_displayName;
    QString m_email;
    common::AccountRole m_role = common::AccountRole::User;
    QString m_sessionToken;
    QDateTime m_loginTime;
    QDateTime m_sessionExpiry;

    mutable QMutex m_mutex;
};

} // namespace bookclub::client
