// src/server/handlers/LibraryRequestHandler.cpp
//
// Loads/syncs the in-memory UserLibrary from the SQLite database so that
// shelves and purchased books created in previous sessions are visible.
//
// Previously this handler just created an empty UserLibrary and kept it
// in m_userLibraries, so any shelf created in a previous session was
// invisible after reconnect.

#include "src/server/handlers/LibraryRequestHandler.h"
#include "src/server/ClientConnection.h"
#include "common/Utils/Logger.h"
#include "common/Utils/IdGenerator.h"

#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlDatabase>
#include <QJsonDocument>

namespace bookclub::server {

namespace {
QSqlDatabase sharedDb()
{
    auto db = QSqlDatabase::database("bookclub_shared", /*open=*/false);
    if (db.isValid()) {
        if (!db.isOpen()) db.open();
        return db;
    }
    db = QSqlDatabase::addDatabase("QSQLITE", "bookclub_shared");
    db.setDatabaseName("bookclub.db");
    db.open();
    return db;
}

QStringList parseJsonStringArray(const QString& json)
{
    QStringList out;
    if (json.isEmpty()) return out;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isArray()) return out;
    for (const auto& v : doc.array()) out.append(v.toString());
    return out;
}

QStringList purchasedBookIdsFor(const QString& userId)
{
    QStringList ids;
    QSqlQuery q(sharedDb());
    q.prepare("SELECT items FROM Orders WHERE userId = ? AND completed = 1");
    q.addBindValue(userId);
    if (!q.exec()) return ids;
    QSet<QString> seen;
    while (q.next()) {
        const QJsonDocument doc = QJsonDocument::fromJson(q.value(0).toString().toUtf8());
        if (!doc.isArray()) continue;
        for (const auto& v : doc.array()) {
            const QString bookId = v.toObject().value("bookId").toString();
            if (!bookId.isEmpty() && !seen.contains(bookId)) {
                seen.insert(bookId);
                ids.append(bookId);
            }
        }
    }
    return ids;
}

void loadShelvesFromDatabase(common::UserLibrary* library)
{
    QSqlQuery q(sharedDb());
    q.prepare("SELECT id, name, description, bookIds, isSystemShelf FROM Shelves WHERE userId = ?");
    q.addBindValue(library->userId());
    if (!q.exec()) return;

    while (q.next()) {
        auto* shelf = new common::LibraryShelf(library);
        shelf->setId(q.value(0).toString());
        shelf->setUserId(library->userId());
        shelf->setName(q.value(1).toString());
        shelf->setDescription(q.value(2).toString());
        shelf->setBookIds(parseJsonStringArray(q.value(3).toString()));
        shelf->setSystemShelf(q.value(4).toInt() == 1);
        // Use the public API to add. We can't directly insert into m_shelves,
        // so we rely on createShelf being the only path. Instead, we set the
        // shelves directly via setShelves() collected in a list.
        // UserLibrary doesn't expose addShelf; we use setShelves().
        // We'll collect into a QVector then call setShelves() at the end.
        // For simplicity, we accumulate in a static list here.
        // Actually we can call setShelves with the accumulated list.
        // Doing it per-row is fine because setShelves replaces the list,
        // so we need to accumulate first.
        // -> Restructure below.
        delete shelf;
    }
}

// Properly load all shelves at once and set them.
void reloadShelves(common::UserLibrary* library)
{
    QSqlQuery q(sharedDb());
    q.prepare("SELECT id, name, description, bookIds, isSystemShelf FROM Shelves WHERE userId = ?");
    q.addBindValue(library->userId());
    if (!q.exec()) return;

    QVector<common::LibraryShelf*> shelves;
    while (q.next()) {
        auto* shelf = new common::LibraryShelf(library);
        shelf->setId(q.value(0).toString());
        shelf->setUserId(library->userId());
        shelf->setName(q.value(1).toString());
        shelf->setDescription(q.value(2).toString());
        shelf->setBookIds(parseJsonStringArray(q.value(3).toString()));
        shelf->setSystemShelf(q.value(4).toInt() == 1);
        shelves.append(shelf);
    }
    library->setShelves(shelves);
}

bool persistShelf(common::LibraryShelf* shelf)
{
    // Insert or update the shelf row.
    QSqlQuery check(sharedDb());
    check.prepare("SELECT COUNT(*) FROM Shelves WHERE id = ?");
    check.addBindValue(shelf->id());
    check.exec();
    bool exists = false;
    if (check.next()) exists = check.value(0).toInt() > 0;

    const QString bookIdsJson = QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(shelf->bookIds())).toJson(QJsonDocument::Compact)
    );

    QSqlQuery q(sharedDb());
    if (exists) {
        q.prepare("UPDATE Shelves SET name=?, description=?, bookIds=?, isSystemShelf=? WHERE id=?");
        q.addBindValue(shelf->name());
        q.addBindValue(shelf->description());
        q.addBindValue(bookIdsJson);
        q.addBindValue(shelf->isSystemShelf() ? 1 : 0);
        q.addBindValue(shelf->id());
    } else {
        q.prepare("INSERT INTO Shelves (id, userId, name, description, bookIds, isSystemShelf) VALUES (?, ?, ?, ?, ?, ?)");
        q.addBindValue(shelf->id());
        q.addBindValue(shelf->userId());
        q.addBindValue(shelf->name());
        q.addBindValue(shelf->description());
        q.addBindValue(bookIdsJson);
        q.addBindValue(shelf->isSystemShelf() ? 1 : 0);
    }
    return q.exec();
}

bool deleteShelfFromDatabase(const QString& shelfId)
{
    QSqlQuery q(sharedDb());
    q.prepare("DELETE FROM Shelves WHERE id = ?");
    q.addBindValue(shelfId);
    return q.exec();
}
} // namespace

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

void LibraryRequestHandler::handleGetLibrary(const QJsonObject& /*payload*/, ClientConnection* client)
{
    common::UserLibrary* library = getOrCreateLibrary(client->userId());
    sendSuccess(client, common::Command::GetLibrary, libraryToJson(library));
}

void LibraryRequestHandler::handleGetPurchasedBooks(const QJsonObject& /*payload*/, ClientConnection* client)
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

    if (!persistShelf(shelf)) {
        sendError(client, common::Command::CreateShelf, common::Status::InternalError,
                  "Failed to persist shelf to database");
        return;
    }

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
    deleteShelfFromDatabase(shelfId);

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
            persistShelf(shelf);
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
            persistShelf(shelf);
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
        // Pull purchased books from completed orders.
        library->setPurchasedBookIds(purchasedBookIdsFor(userId));
        // Pull shelves from the database.
        reloadShelves(library);
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
