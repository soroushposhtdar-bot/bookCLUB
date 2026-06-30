#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "common/AppEnums.h"

namespace bookclub::common {
class UserAccount;
}

namespace bookclub::client {

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);
    ~SessionManager() override = default;

    bool hasActiveSession() const;
    QString currentUserId() const;
    QString currentUsername() const;
    bookclub::common::AccountRole currentRole() const;
    QString sessionToken() const;
    QSharedPointer<bookclub::common::UserAccount> currentUser() const;

    void setCurrentUser(const QSharedPointer<bookclub::common::UserAccount>& user);
    void setSessionToken(const QString& token);
    void clear();

signals:
    void sessionChanged();
    void loggedIn();
    void loggedOut();

private:
    QSharedPointer<bookclub::common::UserAccount> m_currentUser;
    QString m_sessionToken;
};

} // namespace bookclub::client
