// =============================================================================
//  LibraryViewModel.h
// =============================================================================
//  MVVM view-model for the Personal Library page (3 tabs: My Books, Saved,
//  My Shelves).
// =============================================================================
#ifndef LIBRARYVIEWMODEL_H
#define LIBRARYVIEWMODEL_H

#include <QObject>
#include <QQmlEngine>

#include "viewmodels/user/UserViewModelBase.h"

namespace bookclub::client {

class LibraryService;

class LibraryViewModel : public UserViewModelBase {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(LibraryService* libraryService READ libraryService WRITE setLibraryService NOTIFY libraryServiceChanged)

    Q_PROPERTY(int activeTab        READ activeTab        WRITE setActiveTab        NOTIFY activeTabChanged)

    Q_PROPERTY(QList<QObject*> myBooks READ myBooks NOTIFY myBooksChanged)
    Q_PROPERTY(int myBooksCount READ myBooksCount NOTIFY myBooksChanged)
    Q_PROPERTY(QList<QObject*> downloadedBooks READ downloadedBooks NOTIFY downloadedBooksChanged)
    Q_PROPERTY(int downloadedCount READ downloadedCount NOTIFY downloadedBooksChanged)
    Q_PROPERTY(QList<QObject*> savedBooks READ savedBooks NOTIFY savedBooksChanged)
    Q_PROPERTY(int savedCount READ savedCount NOTIFY savedBooksChanged)
    Q_PROPERTY(QList<QObject*> shelves READ shelves NOTIFY shelvesChanged)

    // Shelf management
    Q_PROPERTY(QString newShelfName        READ newShelfName        WRITE setNewShelfName        NOTIFY newShelfChanged)
    Q_PROPERTY(QString newShelfDescription READ newShelfDescription WRITE setNewShelfDescription NOTIFY newShelfChanged)

public:
    explicit LibraryViewModel(QObject* parent = nullptr);

    LibraryService* libraryService() const { return m_libraryService; }
    void setLibraryService(LibraryService* s);

    int activeTab() const { return m_activeTab; }
    void setActiveTab(int v) { if (m_activeTab != v) { m_activeTab = v; emit activeTabChanged(); } }

    QList<QObject*> myBooks() const;
    int myBooksCount() const;
    QList<QObject*> downloadedBooks() const;
    int downloadedCount() const;
    QList<QObject*> savedBooks() const;
    int savedCount() const;
    QList<QObject*> shelves() const;

    QString newShelfName() const { return m_newShelfName; }
    QString newShelfDescription() const { return m_newShelfDescription; }
    void setNewShelfName(const QString& v) { if (m_newShelfName != v) { m_newShelfName = v; emit newShelfChanged(); } }
    void setNewShelfDescription(const QString& v) { if (m_newShelfDescription != v) { m_newShelfDescription = v; emit newShelfChanged(); } }

    Q_INVOKABLE bool canCreateShelf() const { return !m_newShelfName.trimmed().isEmpty(); }
    Q_INVOKABLE void createShelf();
    Q_INVOKABLE void renameShelf(const QString& id, const QString& name);
    Q_INVOKABLE void deleteShelf(const QString& id);
    Q_INVOKABLE void addToShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE void removeFromShelf(const QString& shelfId, const QString& bookId);
    Q_INVOKABLE QList<QObject*> booksInShelf(const QString& shelfId) const;
    Q_INVOKABLE void toggleSaved(const QString& bookId);
    Q_INVOKABLE void toggleDownloaded(const QString& bookId);
    Q_INVOKABLE bool isDownloaded(const QString& bookId) const;

signals:
    void libraryServiceChanged();
    void activeTabChanged();
    void myBooksChanged();
    void downloadedBooksChanged();
    void savedBooksChanged();
    void shelvesChanged();
    void newShelfChanged();
    void shelfCreated(const QString& shelfId);

protected:
    void onAsyncReady() override;

private:
    LibraryService* m_libraryService = nullptr;
    int m_activeTab = 0;   // 0 = My Books, 1 = Saved, 2 = Shelves

    QString m_newShelfName;
    QString m_newShelfDescription;

    enum class PendingOp { None, CreateShelf };
    PendingOp m_pending = PendingOp::None;
};

} // namespace bookclub::client

#endif // LIBRARYVIEWMODEL_H
