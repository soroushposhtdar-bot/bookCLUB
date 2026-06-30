#include "common/Models/RegularUser.h"

namespace bookclub::common {

RegularUser::RegularUser(QObject* parent) : UserAccount(parent) {}

RegularUser::RegularUser(const QString& id, const QString& username, QObject* parent)
    : UserAccount(id, username, parent) {}

AccountRole RegularUser::role() const {
    return AccountRole::User;
}

QString RegularUser::roleName() const {
    return QStringLiteral("کاربر عادی");
}

const QStringList& RegularUser::favoriteGenreIds() const { return m_favoriteGenreIds; }
const QStringList& RegularUser::savedBookIds() const { return m_savedBookIds; }
const QStringList& RegularUser::purchasedBookIds() const { return m_purchasedBookIds; }

void RegularUser::setFavoriteGenreIds(const QStringList& ids) {
    if (m_favoriteGenreIds != ids) {
        m_favoriteGenreIds = ids;
        emit favoriteGenresChanged();
        emit profileChanged();
    }
}

void RegularUser::setSavedBookIds(const QStringList& ids) {
    if (m_savedBookIds != ids) {
        m_savedBookIds = ids;
        emit libraryChanged();
    }
}

void RegularUser::setPurchasedBookIds(const QStringList& ids) {
    if (m_purchasedBookIds != ids) {
        m_purchasedBookIds = ids;
        emit libraryChanged();
    }
}

bool RegularUser::hasFavoriteGenres() const {
    return !m_favoriteGenreIds.isEmpty();
}

} // namespace bookclub::common
