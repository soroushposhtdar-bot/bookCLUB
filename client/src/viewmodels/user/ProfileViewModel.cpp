// =============================================================================
//  ProfileViewModel.cpp
// =============================================================================
#include "viewmodels/user/ProfileViewModel.h"
#include "services/UserService.h"
#include "services/MockDataStore.h"
#include "services/LibraryDtos.h"

namespace bookclub::client {

ProfileViewModel::ProfileViewModel(QObject* parent)
    : UserViewModelBase(parent)
{}

void ProfileViewModel::setUserService(UserService* s) {
    if (m_userService == s) return;
    if (m_userService) disconnect(m_userService, nullptr, this, nullptr);
    m_userService = s;
    if (m_userService) {
        connect(m_userService, &UserService::userChanged, this, [this](){
            if (m_userService) m_displayName = m_userService->displayName();
            emit userChanged();
        });
    }
    if (m_userService) m_displayName = m_userService->displayName();
    emit userServiceChanged();
    emit userChanged();
}

QString ProfileViewModel::username() const     { return m_userService ? m_userService->username()     : QStringLiteral("guest"); }
QString ProfileViewModel::displayName() const  { return m_userService ? m_userService->displayName()  : QStringLiteral("Guest"); }
QString ProfileViewModel::initials() const     { return m_userService ? m_userService->initials()     : QStringLiteral("?"); }
QString ProfileViewModel::favoriteGenresSummary() const {
    return m_userService ? m_userService->favoriteGenresSummary() : QStringLiteral("Not set");
}
QStringList ProfileViewModel::availableGenres() const {
    // The full genre catalog lives on BookService (a QML singleton). The
    // Profile page binds the genre grid to `BookService.availableGenres()`
    // directly — see ProfilePage.qml. This property is kept only for
    // completeness and returns an empty list intentionally; it is NOT a
    // source of genre data for the UI.
    return {};
}

QList<QObject*> ProfileViewModel::purchaseHistory() const {
    return m_userService ? m_userService->purchaseHistory() : QList<QObject*>{};
}

int ProfileViewModel::purchaseCount() const {
    return m_userService ? m_userService->purchaseCount() : 0;
}

void ProfileViewModel::setDisplayName(const QString& v) {
    if (m_displayName == v) return;
    m_displayName = v;
    emit userChanged();
}

void ProfileViewModel::toggleGenre(const QString& g) {
    int idx = m_selectedGenres.indexOf(g);
    if (idx >= 0) {
        m_selectedGenres.removeAt(idx);
    } else {
        if (m_selectedGenres.size() >= 3) return;   // cap at 3
        m_selectedGenres.append(g);
    }
    emit selectedGenresChanged();
}

void ProfileViewModel::loadGenresFromUser() {
    if (!m_userService) return;
    m_selectedGenres = m_userService->favoriteGenres();
    emit selectedGenresChanged();
}

void ProfileViewModel::saveProfile() {
    if (!m_userService || m_displayName.trimmed().isEmpty()) return;
    m_pending = PendingOp::SaveProfile;
    beginAsync(400);
}

void ProfileViewModel::saveGenres() {
    if (!canSaveGenres() || !m_userService) return;
    m_pending = PendingOp::SaveGenres;
    beginAsync(400);
}

void ProfileViewModel::changePassword() {
    if (!canChangePassword() || !m_userService) return;
    m_pending = PendingOp::ChangePassword;
    beginAsync(500);
}

void ProfileViewModel::clearPasswordFields() {
    m_currentPassword.clear();
    m_newPassword.clear();
    m_confirmPassword.clear();
    m_passwordError.clear();
    emit passwordFieldsChanged();
}

void ProfileViewModel::onAsyncReady() {
    if (m_pending == PendingOp::SaveProfile) {
        if (m_userService) m_userService->updateProfile(m_displayName);
        emit profileSaved();
    } else if (m_pending == PendingOp::SaveGenres) {
        if (m_userService) m_userService->saveFavoriteGenres(m_selectedGenres);
        emit genresSaved();
    } else if (m_pending == PendingOp::ChangePassword) {
        QString err;
        if (m_userService && m_userService->changePassword(m_currentPassword, m_newPassword, err)) {
            clearPasswordFields();
            emit passwordChanged();
        } else {
            m_passwordError = err;
            emit passwordFieldsChanged();
            emit passwordChangeFailed(err);
        }
    }
    m_pending = PendingOp::None;
    finishAsync();
}

} // namespace bookclub::client
