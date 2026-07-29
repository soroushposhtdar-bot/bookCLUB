// src/client/controllers/LibraryController.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::common { class Message; }

namespace bookclub::client {

class LibraryController : public QObject {
    Q_OBJECT
public:
    explicit LibraryController(QObject* parent = nullptr);
    ~LibraryController() override;

    // ---- Public Methods ----
    void loadLibrary();
    void loadPurchasedBooks();
    void createShelf(const QString& name, const QString& description = {});
    void renameShelf(const QString& shelfId, const QString& newName);
    void deleteShelf(const QString& shelfId);
    void addBookToShelf(const QString& shelfId, const QString& bookId);
    void removeBookFromShelf(const QString& shelfId, const QString& bookId);
    void moveBookBetweenShelves(const QString& fromShelfId,
                                const QString& toShelfId,
                                const QString& bookId);

    // ---- Accessors ----
    QJsonArray getPurchasedBooks() const;
    QJsonArray getShelves() const;
    QJsonObject getShelf(const QString& shelfId) const;
    QStringList getBookIdsOnShelf(const QString& shelfId) const;

signals:
    void libraryChanged();
    void errorOccurred(const QString& message);

private:
    // ---- Response Handlers ----
    void handleGetLibraryResponse(const common::Message& response);
    void handleGetPurchasedBooksResponse(const common::Message& response);
    void handleCreateShelfResponse(const common::Message& response);
    void handleDeleteShelfResponse(const common::Message& response);
    void handleAddBookToShelfResponse(const common::Message& response);
    void handleRemoveBookFromShelfResponse(const common::Message& response);

    QJsonObject m_libraryData;
};

} // namespace bookclub::client
