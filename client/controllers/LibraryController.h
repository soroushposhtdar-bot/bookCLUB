#pragma once

#include <QObject>
#include <QString>

namespace bookclub::client {

class LibraryController : public QObject {
    Q_OBJECT
public:
    explicit LibraryController(QObject* parent = nullptr);
    ~LibraryController() override = default;

    void loadLibrary();
    void loadPurchasedBooks();
    void loadSavedBooks();
    void createShelf(const QString& name, const QString& description = {});
    void renameShelf(const QString& shelfId, const QString& newName);
    void deleteShelf(const QString& shelfId);
    void addBookToShelf(const QString& shelfId, const QString& bookId);
    void removeBookFromShelf(const QString& shelfId, const QString& bookId);
    void moveBookBetweenShelves(const QString& fromShelfId,
                                const QString& toShelfId,
                                const QString& bookId);

signals:
    void libraryChanged();
    void shelfChanged(const QString& shelfId);
    void errorOccurred(const QString& message);

};

} // namespace bookclub::client
