#pragma once

#include "common/Models/UserAccount.h"

namespace bookclub::common {

class RegularUser final : public UserAccount {
    Q_OBJECT
public:
    explicit RegularUser(QObject* parent = nullptr);
    RegularUser(const QString& id, const QString& username, QObject* parent = nullptr);
    ~RegularUser() override = default;

    AccountRole role() const override;
    QString roleName() const override;

    const QStringList& favoriteGenreIds() const;
    const QStringList& savedBookIds() const;
    const QStringList& purchasedBookIds() const;

    void setFavoriteGenreIds(const QStringList& ids);
    void setSavedBookIds(const QStringList& ids);
    void setPurchasedBookIds(const QStringList& ids);

    bool hasFavoriteGenres() const;

signals:
    void favoriteGenresChanged();
    void libraryChanged();

private:
    QStringList m_favoriteGenreIds;
    QStringList m_savedBookIds;
    QStringList m_purchasedBookIds;
};

} // namespace bookclub::common
