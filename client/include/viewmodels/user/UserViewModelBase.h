// =============================================================================
//  UserViewModelBase.h
// =============================================================================
//  Common base for every User-role ViewModel.
//
//  Mirrors AuthViewModelBase's state machine but is role-agnostic:
//      • isBusy / error / hasError — shared async-state flags
//      • mock-latency helper (the real backend calls will be async via the
//        socket layer; the VMs use the same begin/finish pattern either way)
//      • service injection: every User VM receives the singleton services it
//        needs via Q_PROPERTY setters, called from App.qml at startup.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QQmlEngine>
#include <QTimer>

namespace bookclub::client {

class UserViewModelBase : public QObject {
    Q_OBJECT
    QML_UNCREATABLE("Abstract base — instantiate a concrete subclass.")

    Q_PROPERTY(bool   isBusy   READ isBusy   NOTIFY isBusyChanged)
    Q_PROPERTY(bool   hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString error   READ error    NOTIFY errorChanged)

public:
    explicit UserViewModelBase(QObject* parent = nullptr);

    bool isBusy() const { return m_isBusy; }
    bool hasError() const { return !m_error.isEmpty(); }
    const QString& error() const { return m_error; }

    void setError(const QString& e);
    void clearError();

    // Begin a mocked async op: sets isBusy=true, runs the lambda after
    // `latencyMs` (default 400ms — short, because the mock is in-process).
    // The lambda should call finish() when done.
    void beginAsync(int latencyMs = 400);

    // Subclass hook — called inside beginAsync after the latency delay.
    // Default impl just finishes immediately.
    virtual void onAsyncReady() { finishAsync(); }

    void finishAsync();

signals:
    void isBusyChanged(bool busy);
    void errorChanged(const QString& error);

protected:
    bool m_isBusy = false;
    QString m_error;
    QTimer m_timer;
};

} // namespace bookclub::client
