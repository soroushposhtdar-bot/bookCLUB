// src/server/handlers/LibraryRequestHandler.cpp
//
// Library handler now uses IShelfRepository for persistent shelves and
// queries Orders/OrderItems directly for purchased book IDs.
// Removed: the dead loadShelvesFromDatabase() helper and the in-memory
// m_userLibraries cache (every request now goes to the DB).
#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"
#include "common/Utils/DbConnection.h"
#include "common/Interfaces/IShelfRepository.h"
#include "common/Models/LibraryShelf.h"
#include "common/Models/UserLibrary.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

namespace bookclub::server {

namespace {
// Returns the list of book IDs the user has purchased (from completed orders).
QStringList purchasedBookIdsFor(const QString& userId)
{
    QStringList ids;
    auto q = common::DbConnection::run(
        "SELECT DISTINCT oi.bookId FROM OrderItems oi "
        "JOIN Orders o ON o.id = oi.orderId "
        "WHERE o.userId = ? AND o.completed = 1",
        {userId}
    );
    while (q.next()) ids.append(q.value(0).toString());
    return ids;
}

// Returns the user's saved-book IDs (the "Wishlist" system shelf, if present).
QStringList savedBookIdsFor(const QString& userId)
{
    QStringList ids;
    auto q = common::DbConnection::run(
        "SELECT sb.bookId FROM ShelfBooks sb "
        "JOIN Shelves s ON s.id = sb.shelfId "
        "WHERE s.userId = ? AND s.name = 'Wishlist'",
        {userId}
    );
    while (q.next()) ids.append(q.value(0).toString());
    return ids;
}

QJsonObject shelfToJson(common::LibraryShelf* shelf)
{
    QJsonObject obj;
    obj["id"]           = shelf->id();
    obj["userId"]       = shelf->userId();
    obj["name"]         = shelf->name();
    obj["description"]  = shelf->description();
    obj["bookIds"]      = QJsonArray::fromStringList(shelf->bookIds());
    obj["isSystemShelf"]= shelf->isSystemShelf();
    // v15e: include color, favorite, isPrivate so the client can render
    // shelf cards with the correct color + favorite/private icons.
    obj["color"]        = shelf->color();
    obj["favorite"]     = shelf->favorite();
    obj["isPrivate"]    = shelf->isPrivate();
    obj["sortOrder"]    = shelf->sortOrder();
    return obj;
}
} // namespace

LibraryRequestHandler::LibraryRequestHandler(common::IUserRepository* /*userRepo*/,
                                             QObject* parent)
    : RequestHandlerBase(parent)
{
    LOG_INFO("LibraryRequestHandler initialized");
}

LibraryRequestHandler::~LibraryRequestHandler()
{
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
        case common::Command::GetLibrary:            handleGetLibrary(payload, client); break;
        case common::Command::GetPurchasedBooks:     handleGetPurchasedBooks(payload, client); break;
        case common::Command::CreateShelf:           handleCreateShelf(payload, client); break;
        case common::Command::DeleteShelf:           handleDeleteShelf(payload, client); break;
        case common::Command::AddBookToShelf:        handleAddBookToShelf(payload, client); break;
        case common::Command::RemoveBookFromShelf:   handleRemoveBookFromShelf(payload, client); break;
        case common::Command::RenameShelf:           handleRenameShelf(payload, client); break;
        case common::Command::UpdateShelf:           handleUpdateShelf(payload, client); break;
        default:
            sendError(client, cmd, common::Status::BadRequest, "Invalid command");
            break;
    }
}

void LibraryRequestHandler::handleGetLibrary(const QJsonObject& /*payload*/, ClientConnection* client)
{
    auto* shelfRepo = common::createShelfRepository();
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());

    // BUG FIX: auto-create the default "Wishlist" system shelf for users who
    // don't have one yet (e.g. newly-registered users). Without this shelf,
    // the client's BookService::toggleWishlist falls back to a local-only
    // toggle that doesn't persist — the user clicks the heart, it turns red,
    // then on next page load the book is no longer in the wishlist. By
    // ensuring the Wishlist shelf exists on every GetLibrary call, the
    // client's toggleWishlist always finds a real shelfId to AddBookToShelf.
    bool hasWishlist = false;
    for (auto* s : shelves) {
        if (s->name() == QStringLiteral("Wishlist")) { hasWishlist = true; break; }
    }
    if (!hasWishlist) {
        auto* wishlist = new common::LibraryShelf;
        wishlist->setUserId(client->userId());
        wishlist->setName(QStringLiteral("Wishlist"));
        wishlist->setDescription(QStringLiteral("Books I want to read"));
        wishlist->setSystemShelf(true);
        if (shelfRepo->save(wishlist)) {
            LOG_INFO("Auto-created Wishlist shelf for user: " + client->userId());
        }
        delete wishlist;
        // Re-fetch so the new shelf is included in the response.
        qDeleteAll(shelves);
        shelves = shelfRepo->findByUser(client->userId());
    }

    QJsonObject responsePayload;
    responsePayload["userId"]           = client->userId();
    responsePayload["purchasedBookIds"] = QJsonArray::fromStringList(purchasedBookIdsFor(client->userId()));
    responsePayload["savedBookIds"]     = QJsonArray::fromStringList(savedBookIdsFor(client->userId()));

    QJsonArray shelvesArray;
    for (common::LibraryShelf* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    responsePayload["shelves"] = shelvesArray;

    sendSuccess(client, common::Command::GetLibrary, responsePayload);
}

void LibraryRequestHandler::handleGetPurchasedBooks(const QJsonObject& /*payload*/, ClientConnection* client)
{
    // v15j: return full order details (not just book IDs) so the
    // Profile page can show order date, total, and book titles.
    QJsonObject responsePayload;

    // Still include the flat purchasedBookIds array for backwards compat.
    QStringList purchasedIds = purchasedBookIdsFor(client->userId());
    responsePayload["purchasedBooks"] = QJsonArray::fromStringList(purchasedIds);

    // Build the orders array with full details.
    QJsonArray ordersArray;
    auto q = common::DbConnection::run(
        "SELECT o.id, o.finalTotal, o.completedAt, o.paid, o.completed "
        "FROM Orders o WHERE o.userId = ? AND o.completed = 1 "
        "ORDER BY o.completedAt DESC",
        {client->userId()}
    );
    while (q.next()) {
        QJsonObject order;
        QString orderId = q.value(0).toString();
        order["orderId"] = orderId;
        order["total"] = q.value(1).toDouble();
        order["date"] = q.value(2).toDateTime().toString(Qt::ISODate);
        order["relativeDate"] = q.value(2).toDateTime().toString("MMM d, yyyy");

        // Fetch the book IDs + titles for this order.
        QStringList bookIds;
        QStringList bookTitles;
        auto itemQ = common::DbConnection::run(
            "SELECT oi.bookId, b.title FROM OrderItems oi "
            "LEFT JOIN Books b ON b.id = oi.bookId "
            "WHERE oi.orderId = ?",
            {orderId}
        );
        while (itemQ.next()) {
            bookIds.append(itemQ.value(0).toString());
            bookTitles.append(itemQ.value(1).toString());
        }
        order["bookIds"] = QJsonArray::fromStringList(bookIds);
        // Join titles with ", " for the summary.
        order["titlesSummary"] = bookTitles.join(", ");
        order["discountText"] = QStringLiteral("No discount");

        ordersArray.append(order);
    }
    responsePayload["orders"] = ordersArray;

    sendSuccess(client, common::Command::GetPurchasedBooks, responsePayload);
}

void LibraryRequestHandler::handleCreateShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString name        = payload["name"].toString();
    const QString description = payload["description"].toString();
    if (name.isEmpty()) {
        sendError(client, common::Command::CreateShelf, common::Status::BadRequest,
                  "Shelf name is required");
        return;
    }

    auto* shelfRepo = common::createShelfRepository();
    auto* shelf = new common::LibraryShelf;
    shelf->setUserId(client->userId());
    shelf->setName(name);
    shelf->setDescription(description);
    shelf->setSystemShelf(false);

    if (!shelfRepo->save(shelf)) {
        delete shelf;
        sendError(client, common::Command::CreateShelf, common::Status::InternalError,
                  "Failed to persist shelf");
        return;
    }
    const QString newId = shelf->id();
    delete shelf;

    // Return the updated shelf list.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"]  = shelvesArray;
    responsePayload["newShelfId"] = newId;
    sendSuccess(client, common::Command::CreateShelf, responsePayload);
    LOG_INFO("Shelf created: " + name + " for user: " + client->userId());
}

void LibraryRequestHandler::handleDeleteShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString shelfId = payload["shelfId"].toString();
    if (shelfId.isEmpty()) {
        sendError(client, common::Command::DeleteShelf, common::Status::BadRequest,
                  "shelfId is required");
        return;
    }
    auto* shelfRepo = common::createShelfRepository();
    if (!shelfRepo->remove(shelfId)) {
        sendError(client, common::Command::DeleteShelf, common::Status::NotFound,
                  "Shelf not found");
        return;
    }
    // Issue: return the updated shelf list so the client can refresh its
    // cache without an extra GetLibrary round-trip.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"] = shelvesArray;
    sendSuccess(client, common::Command::DeleteShelf, responsePayload);
}

void LibraryRequestHandler::handleAddBookToShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString shelfId = payload["shelfId"].toString();
    const QString bookId  = payload["bookId"].toString();
    if (shelfId.isEmpty() || bookId.isEmpty()) {
        sendError(client, common::Command::AddBookToShelf, common::Status::BadRequest,
                  "shelfId and bookId are required");
        return;
    }
    auto* shelfRepo = common::createShelfRepository();
    if (!shelfRepo->addBook(shelfId, bookId)) {
        sendError(client, common::Command::AddBookToShelf, common::Status::NotFound,
                  "Shelf not found");
        return;
    }
    // Issue: return the updated shelf list so the client refreshes instantly.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"] = shelvesArray;
    sendSuccess(client, common::Command::AddBookToShelf, responsePayload);
}

void LibraryRequestHandler::handleRemoveBookFromShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString shelfId = payload["shelfId"].toString();
    const QString bookId  = payload["bookId"].toString();
    if (shelfId.isEmpty() || bookId.isEmpty()) {
        sendError(client, common::Command::RemoveBookFromShelf, common::Status::BadRequest,
                  "shelfId and bookId are required");
        return;
    }
    auto* shelfRepo = common::createShelfRepository();
    shelfRepo->removeBook(shelfId, bookId);
    // Issue: return the updated shelf list so the client refreshes instantly.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"] = shelvesArray;
    sendSuccess(client, common::Command::RemoveBookFromShelf, responsePayload);
}

void LibraryRequestHandler::handleRenameShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString shelfId = payload["shelfId"].toString();
    const QString newName = payload["name"].toString();
    if (shelfId.isEmpty() || newName.isEmpty()) {
        sendError(client, common::Command::RenameShelf, common::Status::BadRequest,
                  "shelfId and name are required");
        return;
    }
    auto* shelfRepo = common::createShelfRepository();
    auto* shelf = shelfRepo->findById(shelfId);
    if (!shelf) {
        sendError(client, common::Command::RenameShelf, common::Status::NotFound,
                  "Shelf not found");
        return;
    }
    if (shelf->userId() != client->userId()) {
        delete shelf;
        sendError(client, common::Command::RenameShelf, common::Status::Forbidden,
                  "You do not own this shelf");
        return;
    }
    shelf->setName(newName);
    const bool ok = shelfRepo->save(shelf);
    delete shelf;
    if (!ok) {
        sendError(client, common::Command::RenameShelf, common::Status::InternalError,
                  "Failed to rename shelf");
        return;
    }
    // Issue: return the updated shelf list so the client refreshes instantly.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"] = shelvesArray;
    sendSuccess(client, common::Command::RenameShelf, responsePayload);
}

// v15e: handleUpdateShelf — updates shelf metadata (color / favorite /
// isPrivate / sortOrder / name / description). The payload may include
// any subset of these fields; only the included fields are updated.
void LibraryRequestHandler::handleUpdateShelf(const QJsonObject& payload, ClientConnection* client)
{
    const QString shelfId = payload["shelfId"].toString();
    if (shelfId.isEmpty()) {
        sendError(client, common::Command::UpdateShelf, common::Status::BadRequest,
                  "shelfId is required");
        return;
    }
    auto* shelfRepo = common::createShelfRepository();
    auto* shelf = shelfRepo->findById(shelfId);
    if (!shelf) {
        sendError(client, common::Command::UpdateShelf, common::Status::NotFound,
                  "Shelf not found");
        return;
    }
    if (shelf->userId() != client->userId()) {
        delete shelf;
        sendError(client, common::Command::UpdateShelf, common::Status::Forbidden,
                  "You do not own this shelf");
        return;
    }

    // Apply only the fields that are present in the payload.
    if (payload.contains("color")) {
        shelf->setColor(payload["color"].toString());
    }
    if (payload.contains("favorite")) {
        shelf->setFavorite(payload["favorite"].toBool());
    }
    if (payload.contains("isPrivate")) {
        shelf->setIsPrivate(payload["isPrivate"].toBool());
    }
    if (payload.contains("sortOrder")) {
        shelf->setSortOrder(payload["sortOrder"].toInt());
    }
    if (payload.contains("name")) {
        shelf->setName(payload["name"].toString());
    }
    if (payload.contains("description")) {
        shelf->setDescription(payload["description"].toString());
    }

    const bool ok = shelfRepo->save(shelf);
    delete shelf;
    if (!ok) {
        sendError(client, common::Command::UpdateShelf, common::Status::InternalError,
                  "Failed to update shelf");
        return;
    }

    // Return the updated shelf list so the client refreshes instantly.
    QVector<common::LibraryShelf*> shelves = shelfRepo->findByUser(client->userId());
    QJsonArray shelvesArray;
    for (auto* s : shelves) {
        shelvesArray.append(shelfToJson(s));
        delete s;
    }
    QJsonObject responsePayload;
    responsePayload["shelves"] = shelvesArray;
    sendSuccess(client, common::Command::UpdateShelf, responsePayload);
    LOG_INFO("Shelf updated: " + shelfId);
}

} // namespace bookclub::server
