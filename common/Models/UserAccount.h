#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUrl>

#include "common/AppEnums.h"

namespace bookclub::common {

class UserAccount : public QObject {
    Q_OBJECT
public:
    explicit UserAccount(QObject* parent = nullptr);
    UserAccount(const QString& id, const QString& username, QObject* parent = nullptr);
    ~UserAccount() override = default;

    const QString& id() const;
    const QString& username() const;
    const QString& passwordHash() const;
    const QString& displayName() const;
    const QString& email() const;
    const QString& phone() const;
    const QString& securityQuestion() const;
    const QString& securityAnswerHash() const;
    const QStringList& tags() const;
    const QDateTime& createdAt() const;
    const QDateTime& updatedAt() const;
    AccountStatus status() const;

    virtual AccountRole role() const = 0;
    virtual QString roleName() const = 0;

    void setId(const QString& id);
    void setUsername(const QString& username);
    void setPasswordHash(const QString& hash);
    void setDisplayName(const QString& displayName);
    void setEmail(const QString& email);
    void setPhone(const QString& phone);
    void setSecurityQuestion(const QString& question);
    void setSecurityAnswerHash(const QString& hash);
    void setTags(const QStringList& tags);
    void setCreatedAt(const QDateTime& createdAt);
    void setUpdatedAt(const QDateTime& updatedAt);
    void setStatus(AccountStatus status);

    bool isBlocked() const;
    bool canLogin() const;
    bool requiresFirstGenreSetup() const;

signals:
    void profileChanged();
    void statusChanged(bookclub::common::AccountStatus status);

protected:
    QString m_id;
    QString m_username;
    QString m_passwordHash;
    QString m_displayName;
    QString m_email;
    QString m_phone;
    QString m_securityQuestion;
    QString m_securityAnswerHash;
    QStringList m_tags;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    AccountStatus m_status = AccountStatus::Pending;
};

} // namespace bookclub::common
