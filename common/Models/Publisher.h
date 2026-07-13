#pragma once

#include "common/Models/UserAccount.h"

namespace bookclub::common {

class Publisher final : public UserAccount {
    Q_OBJECT
public:
    explicit Publisher(QObject* parent = nullptr);
    Publisher(const QString& id, const QString& username, QObject* parent = nullptr);
    ~Publisher() override = default;

    AccountRole role() const override;
    QString roleName() const override;

    const QString& publisherName() const;
    const QString& biography() const;
    const QString& website() const;
    const QString& taxId() const;

    void setPublisherName(const QString& name);
    void setBiography(const QString& biography);
    void setWebsite(const QString& website);
    void setTaxId(const QString& taxId);

signals:
    void publisherInfoChanged();

private:
    QString m_publisherName;
    QString m_biography;
    QString m_website;
    QString m_taxId;
};

} // namespace bookclub::common
