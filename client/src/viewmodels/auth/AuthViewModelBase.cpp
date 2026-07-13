// =============================================================================
//  AuthViewModelBase.cpp
// =============================================================================
#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

AuthViewModelBase::AuthViewModelBase(QObject* parent)
    : QObject(parent)
{
    m_mockTimer.setSingleShot(true);
    connect(&m_mockTimer, &QTimer::timeout, this, [this]() {
        _doSubmit();   // subclass performs the actual (mocked) work
    });
    _recomputeCanSubmit();
}

void AuthViewModelBase::setAuthService(AuthService* service) {
    if (m_authService == service) return;
    m_authService = service;
    emit authServiceChanged(m_authService);
    _onAuthServiceChanged();
    _recomputeCanSubmit();
}

void AuthViewModelBase::setFormError(const QString& error) {
    if (m_formError == error) return;
    m_formError = error;
    emit formErrorChanged(m_formError);
}

void AuthViewModelBase::setCanSubmit(bool can) {
    if (m_canSubmit == can) return;
    m_canSubmit = can;
    emit canSubmitChanged(m_canSubmit);
}

void AuthViewModelBase::submit() {
    if (m_isSubmitting) return;
    setFormError({});
    _beginMockedOperation(900);   // mocked network latency
}

void AuthViewModelBase::reset() {
    setFormError({});
    _doReset();
    _recomputeCanSubmit();
}

void AuthViewModelBase::_beginMockedOperation(int latencyMs) {
    m_isSubmitting = true;
    emit isSubmittingChanged(true);
    _recomputeCanSubmit();
    m_mockTimer.start(latencyMs);
}

void AuthViewModelBase::_finishMockedOperation() {
    if (!m_isSubmitting) return;
    m_isSubmitting = false;
    emit isSubmittingChanged(false);
    _recomputeCanSubmit();
}

void AuthViewModelBase::_recomputeCanSubmit() {
    setCanSubmit(_computeCanSubmit());
}

} // namespace bookclub::client
