// =============================================================================
//  GenreSelectionViewModel.h
// =============================================================================
//  MVVM view-model for GenreSelectionPage (post-registration first-login).
//
//  Properties:
//      availableGenres  : QStringList (read-only source for grid)
//      selectedGenres   : QStringList (bound to UI selection state)
//      selectedCount    : int (convenience for QML bindings)
//      canGoBack        : bool
//      isSubmitting / canSubmit / formError (inherited)
//
//  Q_INVOKABLE:
//      toggleGenre(QString genre)
//      isSelected(QString genre) → bool
//      submit()
//      skip()
//      reset()
// =============================================================================
#ifndef GENRESELECTIONVIEWMODEL_H
#define GENRESELECTIONVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QQmlEngine>

#include "viewmodels/auth/AuthViewModelBase.h"
#include "services/AuthService.h"

namespace bookclub::client {

class GenreSelectionViewModel : public AuthViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList availableGenres READ availableGenres NOTIFY availableGenresChanged)
    Q_PROPERTY(QStringList selectedGenres  READ selectedGenres  NOTIFY selectedGenresChanged)
    Q_PROPERTY(int  selectedCount          READ selectedCount   NOTIFY selectedGenresChanged)
    Q_PROPERTY(bool canGoBack              READ canGoBack       CONSTANT)
    Q_PROPERTY(int  minSelection           READ minSelection    CONSTANT)
    // BUG FIX: `username` was a plain C++ member with a non-Q_INVOKABLE setter.
    // QML's `_genreSelectionVM.username = AuthService.currentUsername` wrote to a
    // JS dynamic property instead of the C++ `m_username`, so `_doSubmit()`'s
    // `m_username.isEmpty()` guard always fired "Cannot save preferences — no
    // active session." → genre selection silently failed for every new user.
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)

public:
    explicit GenreSelectionViewModel(QObject* parent = nullptr);

    const QString& username() const { return m_username; }
    void setUsername(const QString& username);   // which user is selecting

    QStringList availableGenres() const { return m_availableGenres; }
    QStringList selectedGenres() const { return m_selectedGenres; }
    int selectedCount() const { return m_selectedGenres.size(); }
    bool canGoBack() const { return false; }   // first-login — no back
    int minSelection() const { return 1; }  // spec: select 1-3 genres

public slots:
    Q_INVOKABLE void toggleGenre(const QString& genre);
    Q_INVOKABLE bool isSelected(const QString& genre) const;
    Q_INVOKABLE void skip();

signals:
    void availableGenresChanged(const QStringList& genres);
    void selectedGenresChanged(const QStringList& genres);
    void usernameChanged(const QString& username);
    void completed();

protected:
    void _doSubmit() override;
    void _doReset() override;
    bool _computeCanSubmit() const override;
    void _onAuthServiceChanged() override;

private:
    QStringList m_availableGenres;
    QStringList m_selectedGenres;
    QString m_username;
};

} // namespace bookclub::client

#endif // GENRESELECTIONVIEWMODEL_H
