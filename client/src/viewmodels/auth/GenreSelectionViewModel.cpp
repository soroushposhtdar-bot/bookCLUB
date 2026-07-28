// =============================================================================
//  GenreSelectionViewModel.cpp
// =============================================================================
#include "viewmodels/auth/GenreSelectionViewModel.h"

namespace bookclub::client {

GenreSelectionViewModel::GenreSelectionViewModel(QObject* parent)
    : AuthViewModelBase(parent)
{}

void GenreSelectionViewModel::_onAuthServiceChanged() {
    if (authService()) {
        m_availableGenres = authService()->availableGenres();
        emit availableGenresChanged(m_availableGenres);
    }
}

void GenreSelectionViewModel::setUsername(const QString& username) {
    // BUG FIX: emit usernameChanged so QML bindings on `viewModel.username`
    // re-evaluate. Previously the setter was a plain method (no Q_PROPERTY,
    // no NOTIFY) so QML writes never reached C++ and genre selection always
    // failed with "no active session".
    if (m_username == username) return;
    m_username = username;
    emit usernameChanged(m_username);
}

// ----- Selection logic -----

void GenreSelectionViewModel::toggleGenre(const QString& genre) {
    int idx = m_selectedGenres.indexOf(genre);
    if (idx >= 0) {
        m_selectedGenres.removeAt(idx);
    } else {
        m_selectedGenres.append(genre);
    }
    emit selectedGenresChanged(m_selectedGenres);
    _recomputeCanSubmit();
}

bool GenreSelectionViewModel::isSelected(const QString& genre) const {
    return m_selectedGenres.contains(genre);
}

void GenreSelectionViewModel::skip() {
    emit completed();
}

// ----- Submit -----

void GenreSelectionViewModel::_doSubmit() {
    if (m_selectedGenres.size() < minSelection()) {
        _finishMockedOperation();
        return;
    }

    if (!authService() || m_username.isEmpty()) {
        setFormError(QStringLiteral("Cannot save preferences — no active session."));
        _finishMockedOperation();
        return;
    }

    authService()->saveGenreSelection(m_username, m_selectedGenres);
    setFormError({});
    _finishMockedOperation();
    emit completed();
}

void GenreSelectionViewModel::_doReset() {
    m_selectedGenres.clear();
    emit selectedGenresChanged(m_selectedGenres);
}

bool GenreSelectionViewModel::_computeCanSubmit() const {
    if (isSubmitting()) return false;
    return m_selectedGenres.size() >= minSelection();
}

} // namespace bookclub::client
