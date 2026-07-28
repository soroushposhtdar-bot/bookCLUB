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

// BUG FIX (Issue 32): return bool so callers can detect the "already
// busy" case and bail out before overwriting m_pending.
//
// PERF FIX (Issue 1): The original code passed `latencyMs` straight to
// the timer, adding 400-700ms of *artificial* delay on top of the real
// network round-trip. The actual backend call is already async (the
// socket layer dispatches its own callbacks), so the timer is only
// here to coalesce the in-process mock path. Cap the local delay at
// 10ms — the user-perceived response time then equals the network
// round-trip, not (network + latencyMs).
bool UserViewModelBase::beginAsync(int latencyMs) {
    if (m_isBusy) return false;
    clearError();
    m_isBusy = true;
    emit isBusyChanged(true);
    const int effectiveDelay = qMax(10, latencyMs / 10);
    m_timer.start(effectiveDelay);
    return true;
}

void UserViewModelBase::finishAsync() {
    if (!m_isBusy) return;
    m_isBusy = false;
    emit isBusyChanged(false);
}

} // namespace bookclub::client
