#pragma once

#include "common/Models/UserAccount.h"

namespace bookclub::common {

class Admin final : public UserAccount {
    Q_OBJECT
public:
    explicit Admin(QObject* parent = nullptr);
    Admin(const QString& id, const QString& username, QObject* parent = nullptr);
    ~Admin() override = default;

    AccountRole role() const override;
    QString roleName() const override;

    bool canModerateContent() const;
    bool canManageAccounts() const;
    bool canViewSystemMetrics() const;
};

} // namespace bookclub::common
