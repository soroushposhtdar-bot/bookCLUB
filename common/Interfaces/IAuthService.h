#pragma once

#include <QString>

namespace bookclub::common {

class UserAccount;

class IAuthService {
public:
    virtual ~IAuthService() = default;

    virtual UserAccount* registerAccount(UserAccount* account,
                                         const QString& plainPassword,
                                         const QString& securityAnswer) = 0;
    virtual UserAccount* login(const QString& username, const QString& plainPassword) = 0;
    virtual bool logout(const QString& userId) = 0;
    virtual bool changePassword(const QString& userId,
                                const QString& oldPassword,
                                const QString& newPassword) = 0;
    virtual bool resetPassword(const QString& username,
                               const QString& securityAnswer,
                               const QString& newPassword) = 0;
    virtual bool isUsernameUnique(const QString& username) const = 0;
};

} // namespace bookclub::common
