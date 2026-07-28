// =============================================================================
//  ProfileViewModel.cpp
// =============================================================================
#include "viewmodels/user/ProfileViewModel.h"
#include "services/UserService.h"
#include "services/LibraryDtos.h"

#include <QQmlEngine>

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
// BUG FIX (Issue 3): previously this returned UserService's displayName
// (which itself returned AuthService's stale currentDisplayName). When
// the user typed into the Profile InputField, `setDisplayName(v)`
// stored the edit in `m_displayName`, but the very next READ returned
// the stale UserService value, so the binding reverted the keystroke.
// We now return the local cache `m_displayName` once the user has
// started editing (m_displayNameEdited=true). Before the first edit,
// we fall back to UserService for the initial load.
QString ProfileViewModel::displayName() const {
    if (m_displayNameEdited) return m_displayName;
    return m_userService ? m_userService->displayName() : QStringLiteral("Guest");
}
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
    // BUG FIX (profile-empty): return the cached list instead of calling
    // m_userService->purchaseHistory() on every Q_PROPERTY read. The cache
    // is populated by refresh() (called from Component.onCompleted) and
    // invalidated on userChanged.
    return m_purchaseHistory;
}

int ProfileViewModel::purchaseCount() const {
    return m_purchaseHistory.size();
}

void ProfileViewModel::setDisplayName(const QString& v) {
    if (m_displayNameEdited && m_displayName == v) return;
    m_displayName = v;
    m_displayNameEdited = true;
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
    // BUG FIX (Issue 32): guard against re-entrant calls. Without this
    // check, if the user clicks "Save profile" twice in quick succession,
    // the second call overwrites m_pending while the first op is still
    // in flight — when the first op's timer fires, onAsyncReady runs
    // the WRONG branch (e.g. saves genres instead of profile).
    if (m_isBusy) return;
    if (!m_userService || m_displayName.trimmed().isEmpty()) return;
    m_pending = PendingOp::SaveProfile;
    beginAsync(10);
}

void ProfileViewModel::saveGenres() {
    // BUG FIX (Issue 32): same re-entrancy guard.
    if (m_isBusy) return;
    if (!canSaveGenres() || !m_userService) return;
    m_pending = PendingOp::SaveGenres;
    beginAsync(10);
}

void ProfileViewModel::changePassword() {
    // BUG FIX (Issue 32): same re-entrancy guard.
    if (m_isBusy) return;
    if (!canChangePassword() || !m_userService) return;
    m_pending = PendingOp::ChangePassword;
    beginAsync(10);
}

void ProfileViewModel::clearPasswordFields() {
    m_currentPassword.clear();
    m_newPassword.clear();
    m_confirmPassword.clear();
    m_passwordError.clear();
    emit passwordFieldsChanged();
}

void ProfileViewModel::refresh() {
    // BUG FIX (profile-empty): fetch the user's purchase history + favorite
    // genres from the server ONCE and cache them. Without this, every Q_PROPERTY
    // read of `purchaseHistory` or `purchaseCount` triggered a blocking
    // `GetPurchasedBooks` round-trip, freezing the UI and making the page
    // feel "empty" (the reads often returned empty before the server responded).
    if (m_userService) {
        // Transfer old DTOs to QML GC before clearing.
        for (auto* o : m_purchaseHistory) {
            QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership);
        }
        m_purchaseHistory = m_userService->purchaseHistory();
        // Give the new DTOs C++ ownership so QML doesn't GC them while
        // the Repeater is iterating.
        for (auto* o : m_purchaseHistory) {
            QQmlEngine::setObjectOwnership(o, QQmlEngine::CppOwnership);
        }
    }
    loadGenresFromUser();
    emit userChanged();
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
