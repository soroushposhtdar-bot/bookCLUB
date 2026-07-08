#include "common/Models/Admin.h"

namespace bookclub::common {

Admin::Admin(QObject* parent) : UserAccount(parent) {}

Admin::Admin(const QString& id, const QString& username, QObject* parent)
    : UserAccount(id, username, parent) {}

AccountRole Admin::role() const {
    return AccountRole::Admin;
}

QString Admin::roleName() const {
    return QStringLiteral("مدیر سیستم");
}

bool Admin::canModerateContent() const { return true; }
bool Admin::canManageAccounts() const { return true; }
bool Admin::canViewSystemMetrics() const { return true; }

} // namespace bookclub::common
