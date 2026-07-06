#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"

#include <QJsonArray>

namespace bookclub::server {

LibraryRequestHandler::LibraryRequestHandler(common::IUserRepository* userRepo, QObject* parent)
    : RequestHandlerBase(parent)
    , m_userRepo(userRepo)
{
    LOG_INFO("LibraryRequestHandler initialized");
}

LibraryRequestHandler::~LibraryRequestHandler()
{
    qDeleteAll(m_userLibraries);
}

void LibraryRequestHandler::handle(const common::Message& request, ClientConnection* client)
{
    if (!client || !client->isAuthenticated()) {
        sendError(client, request.command(), common::Status::Unauthorized, "Authentication required");
        return;
    }

    common::Command cmd = request.command();
    QJsonObject payload = request.payload();

    switch (cmd) {
        case common::Command::GetLibrary:
            handleGetLibrary(payload, client);
            break;
        case common::Command::GetPurchasedBooks:
            handleGetPurchasedBooks(payload, client);
            break;
        case common::Command::CreateShelf:
            handleCreateShelf(payload, client);
            break;
        case common::Command::DeleteShelf:
            handleDeleteShelf(payload, client);
            break;
        case common::Command::AddBookToShelf:
            handleAddBookToShelf(payload, client);
            break;
        case common::Command::RemoveBookFromShelf:
            handleRemoveBookFromShelf(payload, client);
            break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void LibraryRequestHandler::handleGetLibrary(const QJsonObject& payload, ClientConnection* client)
{
    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    sendSuccess(client, common::Command::GetLibrary, libraryToJson(library));
}

void LibraryRequestHandler::handleGetPurchasedBooks(const QJsonObject& payload, ClientConnection* client)
{
    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    QJsonObject responsePayload;
    responsePayload["purchasedBooks"] = QJsonArray::fromStringList(library->purchasedBookIds());
    sendSuccess(client, common::Command::GetPurchasedBooks, responsePayload);
}

void LibraryRequestHandler::handleCreateShelf(const QJsonObject& payload, ClientConnection* client)
{
    QString name = payload["name"].toString();
    QString description = payload["description"].toString();

    if (name.isEmpty()) {
        sendError(client, common::Command::CreateShelf, common::Status::BadRequest, "Shelf name is required");
        return;
    }

    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    common::LibraryShelf* shelf = library->createShelf(name, description);

    sendSuccess(client, common::Command::CreateShelf, libraryToJson(library));
    LOG_INFO("Shelf created: " + name + " for user: " + client->userId());
}

void LibraryRequestHandler::handleDeleteShelf(const QJsonObject& payload, ClientConnection* client)
{
    QString shelfId = payload["shelfId"].toString();
    if (shelfId.isEmpty()) {
        sendError(client, common::Command::DeleteShelf, common::Status::BadRequest, "shelfId is required");
        return;
    }

    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    if (!library->removeShelf(shelfId)) {
        sendError(client, common::Command::DeleteShelf, common::Status::NotFound, "Shelf not found");
        return;
    }

    sendSuccess(client, common::Command::DeleteShelf, libraryToJson(library));
}

void LibraryRequestHandler::handleAddBookToShelf(const QJsonObject& payload, ClientConnection* client)
{
    QString shelfId = payload["shelfId"].toString();
    QString bookId = payload["bookId"].toString();

    if (shelfId.isEmpty() || bookId.isEmpty()) {
        sendError(client, common::Command::AddBookToShelf, common::Status::BadRequest,
                  "shelfId and bookId are required");
        return;
    }

    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    for (common::LibraryShelf* shelf : library->shelves()) {
        if (shelf->id() == shelfId) {
            shelf->addBook(bookId);
            sendSuccess(client, common::Command::AddBookToShelf, libraryToJson(library));
            return;
        }
    }

    sendError(client, common::Command::AddBookToShelf, common::Status::NotFound, "Shelf not found");
}

void LibraryRequestHandler::handleRemoveBookFromShelf(const QJsonObject& payload, ClientConnection* client)
{
    QString shelfId = payload["shelfId"].toString();
    QString bookId = payload["bookId"].toString();

    if (shelfId.isEmpty() || bookId.isEmpty()) {
        sendError(client, common::Command::RemoveBookFromShelf, common::Status::BadRequest,
                  "shelfId and bookId are required");
        return;
    }

    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    for (common::LibraryShelf* shelf : library->shelves()) {
        if (shelf->id() == shelfId) {
            shelf->removeBook(bookId);
            sendSuccess(client, common::Command::RemoveBookFromShelf, libraryToJson(library));
            return;
        }
    }

    sendError(client, common::Command::RemoveBookFromShelf, common::Status::NotFound, "Shelf not found");
}

common::UserLibrary* LibraryRequestHandler::getOrCreateLibrary(const QString& userId)
{
    if (!m_userLibraries.contains(userId)) {
        auto* library = new common::UserLibrary;
        library->setUserId(userId);
        // Load from database if needed
        m_userLibraries[userId] = library;
    }
    return m_userLibraries[userId];
}

QJsonObject LibraryRequestHandler::libraryToJson(common::UserLibrary* library) const
{
    if (!library) return {};

    QJsonObject obj;
    obj["userId"] = library->userId();
    obj["purchasedBookIds"] = QJsonArray::fromStringList(library->purchasedBookIds());
    obj["savedBookIds"] = QJsonArray::fromStringList(library->savedBookIds());

    QJsonArray shelvesArray;
    for (common::LibraryShelf* shelf : library->shelves()) {
        QJsonObject shelfObj;
        shelfObj["id"] = shelf->id();
        shelfObj["name"] = shelf->name();
        shelfObj["description"] = shelf->description();
        shelfObj["bookIds"] = QJsonArray::fromStringList(shelf->bookIds());
        shelfObj["isSystemShelf"] = shelf->isSystemShelf();
        shelvesArray.append(shelfObj);
    }
    obj["shelves"] = shelvesArray;

    return obj;
}

} // namespace bookclub::server
