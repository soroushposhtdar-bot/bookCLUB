// =============================================================================
//  ProfileViewModel.h
// =============================================================================
//  MVVM view-model for the Profile / Settings page.
//
//  Sections:
//      • Identity   — read-only username, editable display name
//      • Genres     — view & update favorite genres (1–3)
//      • Password   — change password (current + new + confirm)
//      • History    — purchase history list
//      • Settings   — theme mode toggle
// =============================================================================
#ifndef PROFILEVIEWMODEL_H
#define PROFILEVIEWMODEL_H

#include <QObject>
#include <QStringList>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class UserService;
class BookService;

class ProfileViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(UserService* userService READ userService WRITE setUserService NOTIFY userServiceChanged)

    Q_PROPERTY(QString username     READ username     NOTIFY userChanged)
    Q_PROPERTY(QString displayName  READ displayName  WRITE setDisplayName NOTIFY userChanged)
    Q_PROPERTY(QString initials     READ initials     NOTIFY userChanged)
    Q_PROPERTY(QString favoriteGenresSummary READ favoriteGenresSummary NOTIFY userChanged)

    // Genre editing
    Q_PROPERTY(QStringList availableGenres READ availableGenres CONSTANT)
    Q_PROPERTY(QStringList selectedGenres  READ selectedGenres  NOTIFY selectedGenresChanged)
    Q_PROPERTY(int selectedGenreCount READ selectedGenreCount NOTIFY selectedGenresChanged)
    Q_PROPERTY(bool canSaveGenres READ canSaveGenres NOTIFY selectedGenresChanged)

    // Password change
    Q_PROPERTY(QString currentPassword READ currentPassword WRITE setCurrentPassword NOTIFY passwordFieldsChanged)
    Q_PROPERTY(QString newPassword     READ newPassword     WRITE setNewPassword     NOTIFY passwordFieldsChanged)
    Q_PROPERTY(QString confirmPassword READ confirmPassword WRITE setConfirmPassword NOTIFY passwordFieldsChanged)
    Q_PROPERTY(QString passwordError   READ passwordError   NOTIFY passwordFieldsChanged)
    Q_PROPERTY(bool canChangePassword  READ canChangePassword  NOTIFY passwordFieldsChanged)

    // Purchase history
    Q_PROPERTY(QList<QObject*> purchaseHistory READ purchaseHistory NOTIFY userChanged)
    Q_PROPERTY(int purchaseCount READ purchaseCount NOTIFY userChanged)

public:
    explicit ProfileViewModel(QObject* parent = nullptr);

    UserService* userService() const { return m_userService; }
    void setUserService(UserService* s);

    QString username() const;
    QString displayName() const;
    QString initials() const;
    QString favoriteGenresSummary() const;

    QStringList availableGenres() const;
    QStringList selectedGenres() const { return m_selectedGenres; }
    int selectedGenreCount() const { return m_selectedGenres.size(); }
    bool canSaveGenres() const { return m_selectedGenres.size() >= 1 && m_selectedGenres.size() <= 3; }

    QString currentPassword() const { return m_currentPassword; }
    QString newPassword() const     { return m_newPassword; }
    QString confirmPassword() const { return m_confirmPassword; }
    QString passwordError() const   { return m_passwordError; }
    bool canChangePassword() const {
        return !m_currentPassword.isEmpty() && m_newPassword.length() >= 6
            && m_newPassword == m_confirmPassword && m_newPassword != m_currentPassword;
    }

    QList<QObject*> purchaseHistory() const;
    int purchaseCount() const;

public slots:
    void setDisplayName(const QString& v);
    void setCurrentPassword(const QString& v) { m_currentPassword = v; emit passwordFieldsChanged(); }
    void setNewPassword(const QString& v)     { m_newPassword = v;     emit passwordFieldsChanged(); }
    void setConfirmPassword(const QString& v) { m_confirmPassword = v; emit passwordFieldsChanged(); }

    Q_INVOKABLE bool isGenreSelected(const QString& g) const { return m_selectedGenres.contains(g); }
    Q_INVOKABLE void toggleGenre(const QString& g);
    Q_INVOKABLE void loadGenresFromUser();
    Q_INVOKABLE void saveProfile();
    Q_INVOKABLE void saveGenres();
    Q_INVOKABLE void changePassword();
    Q_INVOKABLE void clearPasswordFields();

signals:
    void userServiceChanged();
    void userChanged();
    void selectedGenresChanged();
    void passwordFieldsChanged();
    void profileSaved();
    void genresSaved();
    void passwordChanged();
    void passwordChangeFailed(const QString& error);

protected:
    void onAsyncReady() override;

private:
    UserService* m_userService = nullptr;

    QString m_displayName;
    QStringList m_selectedGenres;

    QString m_currentPassword;
    QString m_newPassword;
    QString m_confirmPassword;
    QString m_passwordError;

    enum class PendingOp { None, SaveProfile, SaveGenres, ChangePassword };
    PendingOp m_pending = PendingOp::None;
};

} // namespace bookclub::client

#endif // PROFILEVIEWMODEL_H
