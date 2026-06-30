#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include "common/AppEnums.h"

namespace bookclub::common {

class UserAccount;

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual bool existsByUsername(const QString& username) const = 0;
    virtual UserAccount* findById(const QString& id) const = 0;
    virtual UserAccount* findByUsername(const QString& username) const = 0;
    virtual QVector<UserAccount*> findAll() const = 0;
    virtual QVector<UserAccount*> search(const QString& keyword) const = 0;
    virtual bool save(UserAccount* user) = 0;
    virtual bool update(UserAccount* user) = 0;
    virtual bool remove(const QString& id) = 0;
    virtual bool blockUser(const QString& id) = 0;
    virtual bool unblockUser(const QString& id) = 0;
    virtual bool setAccountStatus(const QString& id, AccountStatus status) = 0;
    virtual QDateTime registeredAt(const QString& id) const = 0;
};

} // namespace bookclub::common
