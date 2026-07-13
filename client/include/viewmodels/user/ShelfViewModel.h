// =============================================================================
//  ShelfViewModel.h
// =============================================================================
//  Dedicated shelves page VM — distinct from LibraryViewModel (which only
//  covers the "Shelves" tab in the Library page). This powers a full-page
//  shelves management experience with drag-drop, reorder, colors, etc.
// =============================================================================
#ifndef SHELFVIEWMODEL_H
#define SHELFVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class LibraryService;
class ShelfDto;

class ShelfViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(LibraryService* libraryService READ libraryService WRITE setLibraryService NOTIFY libraryServiceChanged)

    Q_PROPERTY(QList<QObject*> shelves READ shelves NOTIFY shelvesChanged)
    Q_PROPERTY(int count READ count NOTIFY shelvesChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY shelvesChanged)

    // New shelf form
    Q_PROPERTY(QString newName        READ newName        WRITE setNewName        NOTIFY newShelfChanged)
    Q_PROPERTY(QString newDescription READ newDescription WRITE setNewDescription NOTIFY newShelfChanged)
    Q_PROPERTY(QString newColor       READ newColor       WRITE setNewColor       NOTIFY newShelfChanged)
    Q_PROPERTY(bool newIsPrivate      READ newIsPrivate   WRITE setNewIsPrivate   NOTIFY newShelfChanged)
    Q_PROPERTY(bool canCreate         READ canCreate      NOTIFY newShelfChanged)

    // View state
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)

    // Selected shelf (for the detail panel)
    Q_PROPERTY(QString selectedShelfId READ selectedShelfId NOTIFY selectionChanged)
    Q_PROPERTY(QList<QObject*> selectedShelfBooks READ selectedShelfBooks NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedShelf READ selectedShelf NOTIFY selectionChanged)

public:
    explicit ShelfViewModel(QObject* parent = nullptr);

    LibraryService* libraryService() const { return m_libraryService; }
    void setLibraryService(LibraryService* s);

    QList<QObject*> shelves() const;
    int count() const;
    bool isEmpty() const { return count() == 0; }

    QString newName() const { return m_newName; }
    QString newDescription() const { return m_newDescription; }
    QString newColor() const { return m_newColor; }
    bool newIsPrivate() const { return m_newIsPrivate; }
    bool canCreate() const { return !m_newName.trimmed().isEmpty(); }
    void setNewName(const QString& v) { if (m_newName != v) { m_newName = v; emit newShelfChanged(); } }
    void setNewDescription(const QString& v) { if (m_newDescription != v) { m_newDescription = v; emit newShelfChanged(); } }
    void setNewColor(const QString& v) { if (m_newColor != v) { m_newColor = v; emit newShelfChanged(); } }
    void setNewIsPrivate(bool v) { if (m_newIsPrivate != v) { m_newIsPrivate = v; emit newShelfChanged(); } }

    QString viewMode() const { return m_viewMode; }
    QString sortMode() const { return m_sortMode; }
    QString searchQuery() const { return m_searchQuery; }
    void setViewMode(const QString& v) { if (m_viewMode != v) { m_viewMode = v; emit viewModeChanged(); } }
    void setSortMode(const QString& v) { if (m_sortMode != v) { m_sortMode = v; emit sortModeChanged(); emit shelvesChanged(); } }
    void setSearchQuery(const QString& v) { if (m_searchQuery != v) { m_searchQuery = v; emit searchQueryChanged(); emit shelvesChanged(); } }

    QString selectedShelfId() const { return m_selectedShelfId; }
    QList<QObject*> selectedShelfBooks() const;
    QVariantMap selectedShelf() const;

    Q_INVOKABLE void createShelf();
    Q_INVOKABLE void renameShelf(const QString& id, const QString& name);
    Q_INVOKABLE void deleteShelf(const QString& id);
    Q_INVOKABLE void duplicateShelf(const QString& id);
    Q_INVOKABLE void setShelfColor(const QString& id, const QString& color);
    Q_INVOKABLE void toggleFavorite(const QString& id);
    Q_INVOKABLE void togglePrivate(const QString& id);
    Q_INVOKABLE void moveUp(const QString& id);
    Q_INVOKABLE void moveDown(const QString& id);
    Q_INVOKABLE void reorder(const QStringList& orderedIds);
    Q_INVOKABLE void addBookToShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE void removeBookFromShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE void moveBook(const QString& fromShelfId, const QString& toShelfId, const QString& bookId);
    Q_INVOKABLE void copyBook(const QString& fromShelfId, const QString& toShelfId, const QString& bookId);
    Q_INVOKABLE void selectShelf(const QString& id);

signals:
    void libraryServiceChanged();
    void shelvesChanged();
    void newShelfChanged();
    void viewModeChanged();
    void sortModeChanged();
    void searchQueryChanged();
    void selectionChanged();
    void shelfCreated(const QString& id);
    void shelfDeleted(const QString& id);

protected:
    void onAsyncReady() override;

private:
    LibraryService* m_libraryService = nullptr;
    QString m_newName;
    QString m_newDescription;
    QString m_newColor = "#1A73E8";
    bool m_newIsPrivate = false;
    QString m_viewMode = "grid";
    QString m_sortMode = "order";      // order | name | recent | bookCount
    QString m_searchQuery;
    QString m_selectedShelfId;

    enum class PendingOp { None, Create };
    PendingOp m_pending = PendingOp::None;
};

} // namespace bookclub::client

#endif // SHELFVIEWMODEL_H
