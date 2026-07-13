// =============================================================================
//  UserService.h
// =============================================================================
//  Mocked user-profile service for the Regular User role.
//
//  Responsibilities:
//      • Holds the currently logged-in user's identity (set after auth).
//      • Reads & updates profile fields (display name, favorite genres).
//      • Changes password (delegated to AuthService for hashing/verify).
//      • Exposes purchase history for the Profile page.
//
//  Real-backend mapping (see common/Network/Protocol.h):
//      getUserProfile()          → REQ_USER_PROFILE          → RES_USER_PROFILE
//      updateProfile(...)        → REQ_USER_UPDATE           → RES_USER_UPDATE
//      changePassword(...)       → REQ_USER_CHANGE_PASSWORD  → RES_USER_CHANGE_PASSWORD
//      saveFavoriteGenres(...)   → REQ_USER_GENRES           → RES_USER_GENRES
//      purchaseHistory()         → REQ_USER_PURCHASES        → RES_USER_PURCHASES
//
//  All network responses arrive asynchronously via the socket layer; here we
//  simulate the same flow with a short QTimer delay so the ViewModels can
//  exercise the same isBusy / success / failure state machine they will use
//  against the real backend.
// =============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>
#include <QDateTime>

#include "services/MockDataStore.h"

namespace bookclub::client {

class UserService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Identity
    Q_PROPERTY(QString username     READ username     NOTIFY userChanged)
    Q_PROPERTY(QString displayName  READ displayName  NOTIFY userChanged)
    Q_PROPERTY(QString initials     READ initials     NOTIFY userChanged)
    Q_PROPERTY(QString favoriteGenresSummary READ favoriteGenresSummary NOTIFY userChanged)

public:
    explicit UserService(QObject* parent = nullptr);

    // Bind to the shared mock store (called once from App.qml / main.cpp).
    Q_INVOKABLE void setDataStore(MockDataStore* store);

    QString username() const;
    QString displayName() const;
    QString initials() const;
    QString favoriteGenresSummary() const;

    // ----- Q_INVOKABLE actions (called from ProfileViewModel) -----
    // Each returns true/false synchronously and emits the matching signal.
    // The ViewModel wraps these with the mock-latency state machine.
    Q_INVOKABLE bool updateProfile(const QString& displayName);
    Q_INVOKABLE bool changePassword(const QString& currentPassword,
                                     const QString& newPassword,
                                     QString& errorMessage);
    Q_INVOKABLE QStringList favoriteGenres() const;
    Q_INVOKABLE bool saveFavoriteGenres(const QStringList& genres);
    Q_INVOKABLE int purchaseCount() const;
    Q_INVOKABLE QList<QObject*> purchaseHistory() const;

signals:
    void userChanged();
    void profileUpdated();
    void passwordChanged();
    void passwordChangeFailed(const QString& error);

private:
    MockDataStore* m_store = nullptr;
};

} // namespace bookclub::client
