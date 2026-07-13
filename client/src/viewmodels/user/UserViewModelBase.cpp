// =============================================================================
//  UserViewModelBase.cpp
// =============================================================================
#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

UserViewModelBase::UserViewModelBase(QObject* parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this]() {
        onAsyncReady();
    });
}

void UserViewModelBase::setError(const QString& e) {
    if (m_error == e) return;
    m_error = e;
    emit errorChanged(m_error);
}

void UserViewModelBase::clearError() { setError({}); }

void UserViewModelBase::beginAsync(int latencyMs) {
    if (m_isBusy) return;
    clearError();
    m_isBusy = true;
    emit isBusyChanged(true);
    m_timer.start(latencyMs);
}

void UserViewModelBase::finishAsync() {
    if (!m_isBusy) return;
    m_isBusy = false;
    emit isBusyChanged(false);
}

} // namespace bookclub::client
