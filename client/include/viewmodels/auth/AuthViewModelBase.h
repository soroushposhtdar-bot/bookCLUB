// =============================================================================
//  AuthViewModelBase.h
// =============================================================================
//  Common base class for every Authentication ViewModel.
//
//  Provides shared infrastructure:
//      • isSubmitting / canSubmit state (common to every auth flow)
//      • formError property (string shown as a banner above the form)
//      • async submit() helper that drives isSubmitting → emits finished/error
//      • QTimer-based mock latency simulation (no real backend required)
//      • reset() hook for subclasses to clear their own state
//      • authService property — bound from QML so all VMs share the same
//        singleton instance of AuthService.
//
//  Subclasses are expected to:
//      • Expose field properties (username, password, etc.) with Q_PROPERTY
//      • Emit their own success/failure signals
//      • Override _doSubmit() to perform the actual (mocked) work
//      • Override _doReset() to clear subclass-specific state
//      • Override _onAuthServiceChanged() to react when authService is attached
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QTimer>
#include <memory>

namespace bookclub::client {

class AuthService;   // fwd

class AuthViewModelBase : public QObject {
    Q_OBJECT
    QML_UNCREATABLE("Abstract base — instantiate a concrete subclass.")

    // Injected from QML — every auth VM shares the same AuthService singleton.
    Q_PROPERTY(AuthService* authService READ authService WRITE setAuthService NOTIFY authServiceChanged)

    Q_PROPERTY(bool isSubmitting READ isSubmitting NOTIFY isSubmittingChanged)
    Q_PROPERTY(bool canSubmit READ canSubmit NOTIFY canSubmitChanged)
    Q_PROPERTY(QString formError READ formError WRITE setFormError NOTIFY formErrorChanged)

public:
    explicit AuthViewModelBase(QObject* parent = nullptr);
    ~AuthViewModelBase() override = default;

    AuthService* authService() const { return m_authService; }
    void setAuthService(AuthService* service);

    bool isSubmitting() const { return m_isSubmitting; }
    bool canSubmit() const { return m_canSubmit; }
    const QString& formError() const { return m_formError; }

    void setFormError(const QString& error);
    void setCanSubmit(bool can);

    Q_INVOKABLE void submit();
    Q_INVOKABLE void reset();

signals:
    void authServiceChanged(AuthService* service);
    void isSubmittingChanged(bool submitting);
    void canSubmitChanged(bool can);
    void formErrorChanged(const QString& error);

    void succeeded();
    void failed(const QString& error);

protected:
    // Subclass hooks
    virtual void _doSubmit() = 0;
    virtual void _doReset() {}
    virtual void _onAuthServiceChanged() {}

    // Helper for subclasses to start a mocked async operation.
    // Calls _doSubmit() after `latencyMs` ms; sets isSubmitting=true in between.
    void _beginMockedOperation(int latencyMs);

    // Helper for subclasses to finish an async op (call from a timer slot or directly).
    void _finishMockedOperation();

    // Recompute canSubmit — called by subclasses via _setCanSubmitReady().
    virtual bool _computeCanSubmit() const { return !m_isSubmitting; }

    void _recomputeCanSubmit();

private:
    AuthService* m_authService = nullptr;
    bool m_isSubmitting = false;
    bool m_canSubmit = false;
    QString m_formError;
    QTimer m_mockTimer;
};

} // namespace bookclub::client

// Forward-declare QML type so Q_PROPERTY compiles without the full header.
Q_DECLARE_METATYPE(bookclub::client::AuthService*)
