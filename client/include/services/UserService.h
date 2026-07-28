#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>

namespace bookclub::client {

class UserService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString username     READ username     NOTIFY userChanged)
    Q_PROPERTY(QString displayName  READ displayName  NOTIFY userChanged)
    Q_PROPERTY(QString initials     READ initials     NOTIFY userChanged)
    Q_PROPERTY(QString favoriteGenresSummary READ favoriteGenresSummary NOTIFY userChanged)

public:
    explicit UserService(QObject* parent = nullptr);
    Q_INVOKABLE void setDataStore(QObject*) {}  // no-op

    QString username() const;
    QString displayName() const;
    QString initials() const;
    QString favoriteGenresSummary() const;

    Q_INVOKABLE bool updateProfile(const QString& displayName);
    bool changePassword(const QString& currentPassword,
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
    QString m_username, m_displayName;
    mutable QStringList m_favoriteGenres;
};

} // namespace bookclub::client
