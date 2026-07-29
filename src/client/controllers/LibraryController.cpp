// src/client/controllers/LibraryController.cpp
#include "src/client/controllers/LibraryController.h"
#include "src/client/network/ClientNetworkManager.h"
#include "common/Utils/Logger.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace bookclub::client {

LibraryController::LibraryController(QObject* parent)
    : QObject(parent)
{
    auto& network = ClientNetworkManager::instance();

    // Register response handlers
    network.registerRequestHandler(common::Command::GetLibrary, [this](const common::Message& response) {
        handleGetLibraryResponse(response);
    });

    network.registerRequestHandler(common::Command::GetPurchasedBooks, [this](const common::Message& response) {
        handleGetPurchasedBooksResponse(response);
    });

    network.registerRequestHandler(common::Command::CreateShelf, [this](const common::Message& response) {
        handleCreateShelfResponse(response);
    });

    network.registerRequestHandler(common::Command::DeleteShelf, [this](const common::Message& response) {
        handleDeleteShelfResponse(response);
    });

    network.registerRequestHandler(common::Command::AddBookToShelf, [this](const common::Message& response) {
        handleAddBookToShelfResponse(response);
    });

    network.registerRequestHandler(common::Command::RemoveBookFromShelf, [this](const common::Message& response) {
        handleRemoveBookFromShelfResponse(response);
    });

    LOG_INFO("LibraryController initialized");
}

LibraryController::~LibraryController()
{
    auto& network = ClientNetworkManager::instance();
    network.unregisterRequestHandler(common::Command::GetLibrary);
    network.unregisterRequestHandler(common::Command::GetPurchasedBooks);
    network.unregisterRequestHandler(common::Command::CreateShelf);
    network.unregisterRequestHandler(common::Command::DeleteShelf);
    network.unregisterRequestHandler(common::Command::AddBookToShelf);
    network.unregisterRequestHandler(common::Command::RemoveBookFromShelf);
}

// ---- Public Methods ----

void LibraryController::loadLibrary()
{
    LOG_DEBUG("LibraryController::loadLibrary() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load library failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetLibrary, {});
}

void LibraryController::loadPurchasedBooks()
{
    LOG_DEBUG("LibraryController::loadPurchasedBooks() called");

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Load purchased books failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::GetPurchasedBooks, {});
}

void LibraryController::createShelf(const QString& name, const QString& description)
{
    LOG_DEBUG("LibraryController::createShelf() called with name: " + name);

    if (name.isEmpty()) {
        LOG_WARNING("Create shelf failed: name is empty");
        emit errorOccurred("Shelf name is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Create shelf failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["name"] = name;
    if (!description.isEmpty()) {
        payload["description"] = description;
    }

    ClientNetworkManager::instance().sendRequest(common::Command::CreateShelf, payload);
}

void LibraryController::renameShelf(const QString& shelfId, const QString& newName)
{
    LOG_DEBUG("LibraryController::renameShelf() called for shelf: " + shelfId +
              " with new name: " + newName);

    if (shelfId.isEmpty() || newName.isEmpty()) {
        LOG_WARNING("Rename shelf failed: shelfId or new name is empty");
        emit errorOccurred("Shelf ID and new name are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Rename shelf failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // Note: Since there's no direct rename command, we'll create a new shelf
    // and copy books over, then delete the old one.
    // This is a workaround; in production you'd have a dedicated rename command.

    // For now, we'll just emit an error
    LOG_WARNING("Rename shelf not implemented yet");
    emit errorOccurred("Rename shelf feature is not yet implemented");
}

void LibraryController::deleteShelf(const QString& shelfId)
{
    LOG_DEBUG("LibraryController::deleteShelf() called for shelf: " + shelfId);

    if (shelfId.isEmpty()) {
        LOG_WARNING("Delete shelf failed: shelfId is empty");
        emit errorOccurred("Shelf ID is required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Delete shelf failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["shelfId"] = shelfId;

    ClientNetworkManager::instance().sendRequest(common::Command::DeleteShelf, payload);
}

void LibraryController::addBookToShelf(const QString& shelfId, const QString& bookId)
{
    LOG_DEBUG("LibraryController::addBookToShelf() called for shelf: " + shelfId +
              ", book: " + bookId);

    if (shelfId.isEmpty() || bookId.isEmpty()) {
        LOG_WARNING("Add book to shelf failed: shelfId or bookId is empty");
        emit errorOccurred("Shelf ID and Book ID are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Add book to shelf failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["shelfId"] = shelfId;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::AddBookToShelf, payload);
}

void LibraryController::removeBookFromShelf(const QString& shelfId, const QString& bookId)
{
    LOG_DEBUG("LibraryController::removeBookFromShelf() called for shelf: " + shelfId +
              ", book: " + bookId);

    if (shelfId.isEmpty() || bookId.isEmpty()) {
        LOG_WARNING("Remove book from shelf failed: shelfId or bookId is empty");
        emit errorOccurred("Shelf ID and Book ID are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Remove book from shelf failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    QJsonObject payload;
    payload["shelfId"] = shelfId;
    payload["bookId"] = bookId;

    ClientNetworkManager::instance().sendRequest(common::Command::RemoveBookFromShelf, payload);
}

void LibraryController::moveBookBetweenShelves(const QString& fromShelfId,
                                               const QString& toShelfId,
                                               const QString& bookId)
{
    LOG_DEBUG("LibraryController::moveBookBetweenShelves() called for book: " + bookId +
              " from shelf: " + fromShelfId + " to shelf: " + toShelfId);

    if (fromShelfId.isEmpty() || toShelfId.isEmpty() || bookId.isEmpty()) {
        LOG_WARNING("Move book between shelves failed: missing parameters");
        emit errorOccurred("All parameters are required");
        return;
    }

    if (!ClientNetworkManager::instance().isConnected()) {
        LOG_WARNING("Move book between shelves failed: not connected to server");
        emit errorOccurred("Not connected to server");
        return;
    }

    // First remove from source shelf
    // Then add to destination shelf
    // This is a workaround; ideally you'd have a single move command

    removeBookFromShelf(fromShelfId, bookId);
    addBookToShelf(toShelfId, bookId);

    LOG_INFO("Book move initiated (as separate operations)");
}

// ---- Response Handlers ----

void LibraryController::handleGetLibraryResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load library");
        LOG_WARNING("Load library failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_libraryData = data;

    emit libraryChanged();

    LOG_INFO("Library loaded successfully");
}

void LibraryController::handleGetPurchasedBooksResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to load purchased books");
        LOG_WARNING("Load purchased books failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    QJsonArray purchasedBooks = data["purchasedBooks"].toArray();

    // Update library data with purchased books
    if (!m_libraryData.isEmpty()) {
        m_libraryData["purchasedBooks"] = purchasedBooks;
    }

    emit libraryChanged();

    LOG_INFO("Purchased books loaded successfully. Count: " + QString::number(purchasedBooks.size()));
}

void LibraryController::handleCreateShelfResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to create shelf");
        LOG_WARNING("Create shelf failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_libraryData = data;

    emit libraryChanged();

    LOG_INFO("Shelf created successfully");
}

void LibraryController::handleDeleteShelfResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to delete shelf");
        LOG_WARNING("Delete shelf failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_libraryData = data;

    emit libraryChanged();

    LOG_INFO("Shelf deleted successfully");
}

void LibraryController::handleAddBookToShelfResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to add book to shelf");
        LOG_WARNING("Add book to shelf failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_libraryData = data;

    emit libraryChanged();

    LOG_INFO("Book added to shelf successfully");
}

void LibraryController::handleRemoveBookFromShelfResponse(const common::Message& response)
{
    if (!response.isSuccess()) {
        QString error = response.payload().value("error").toString("Failed to remove book from shelf");
        LOG_WARNING("Remove book from shelf failed: " + error);
        emit errorOccurred(error);
        return;
    }

    QJsonObject data = response.payload();
    m_libraryData = data;

    emit libraryChanged();

    LOG_INFO("Book removed from shelf successfully");
}

// ---- Helper Methods ----

QJsonArray LibraryController::getPurchasedBooks() const
{
    return m_libraryData["purchasedBooks"].toArray();
}

QJsonArray LibraryController::getShelves() const
{
    return m_libraryData["shelves"].toArray();
}

QJsonObject LibraryController::getShelf(const QString& shelfId) const
{
    QJsonArray shelves = m_libraryData["shelves"].toArray();
    for (const auto& shelf : shelves) {
        QJsonObject shelfObj = shelf.toObject();
        if (shelfObj["id"].toString() == shelfId) {
            return shelfObj;
        }
    }
    return {};
}

QStringList LibraryController::getBookIdsOnShelf(const QString& shelfId) const
{
    QStringList bookIds;
    QJsonObject shelf = getShelf(shelfId);
    QJsonArray books = shelf["bookIds"].toArray();
    for (const auto& bookId : books) {
        bookIds.append(bookId.toString());
    }
    return bookIds;
}

} // namespace bookclub::client
