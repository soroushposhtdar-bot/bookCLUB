#include "services/LibraryService.h"
#include "services/NetworkService.h"
#include "services/BookDto.h"
#include "services/LibraryDtos.h"
#include "common/Network/Protocol.h"

#include <QJsonObject>
#include <QJsonArray>

namespace bookclub::client {

LibraryService::LibraryService(QObject* parent) : QObject(parent) {}

void LibraryService::refreshFromServer() const {
    auto resp = NetworkService::instance().sendRequest(common::Command::GetLibrary);
    if (resp.isSuccess()) m_libraryData = resp.payload;
}

int LibraryService::purchasedCount() const {
    if (m_libraryData.isEmpty()) refreshFromServer();
    const QJsonArray arr = m_libraryData.value("purchasedBookIds").toArray();
    return arr.size();
}

int LibraryService::savedCount() const {
    if (m_libraryData.isEmpty()) refreshFromServer();
    // Count books in the Wishlist system shelf + any savedBookIds in the payload.
    int count = m_libraryData.value("savedBookIds").toArray().size();
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            count += s.value("bookIds").toArray().size();
            break;
        }
    }
    return count;
}

QList<QObject*> LibraryService::purchasedBooks() const {
    if (m_libraryData.isEmpty()) refreshFromServer();
    QList<QObject*> result;
    const QJsonArray arr = m_libraryData.value("purchasedBookIds").toArray();
    if (arr.isEmpty()) return result;

    // Batch fetch in ONE round-trip via GetBooksByIds (avoids N sequential
    // GetBookDetails calls).
    QJsonObject p;
    p["bookIds"] = arr;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (resp.isSuccess()) {
        const QJsonArray books = resp.payload.value("books").toArray();
        for (const auto& v : books) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            dto->setPurchased(true);
            result.append(dto);
        }
    }
    return result;
}

QList<QObject*> LibraryService::downloadedBooks() const {
    // Return purchased books that the user has marked as downloaded (local set).
    // The m_downloaded set is populated via toggleDownloaded().
    if (m_downloaded.isEmpty()) return {};
    QList<QObject*> result;
    // Batch-fetch the downloaded book IDs.
    QJsonObject p;
    QJsonArray idsArr;
    for (const QString& id : m_downloaded) idsArr.append(id);
    p["bookIds"] = idsArr;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (resp.isSuccess()) {
        for (const auto& v : resp.payload.value("books").toArray()) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            dto->setPurchased(true);
            result.append(dto);
        }
    }
    return result;
}
bool LibraryService::isDownloaded(const QString& bookId) const { return m_downloaded.contains(bookId); }
void LibraryService::toggleDownloaded(const QString& bookId) {
    if (m_downloaded.contains(bookId)) m_downloaded.removeAll(bookId);
    else m_downloaded.append(bookId);
    emit libraryChanged();
}

QList<QObject*> LibraryService::savedBooks() const {
    // "Saved" = books in the user's "Wishlist" system shelf. We fetch the
    // library payload once, find the Wishlist shelf, and batch-fetch its
    // books via GetBooksByIds.
    if (m_libraryData.isEmpty()) refreshFromServer();
    QList<QObject*> result;

    QJsonArray savedIds;
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            savedIds = s.value("bookIds").toArray();
            break;
        }
    }
    // Also include savedBookIds (from the server payload) if present.
    const QJsonArray savedFromPayload = m_libraryData.value("savedBookIds").toArray();
    for (const auto& v : savedFromPayload) {
        if (!savedIds.contains(v)) savedIds.append(v);
    }
    if (savedIds.isEmpty()) return result;

    QJsonObject p;
    p["bookIds"] = savedIds;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (resp.isSuccess()) {
        const QJsonArray books = resp.payload.value("books").toArray();
        for (const auto& v : books) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            result.append(dto);
        }
    }
    return result;
}

bool LibraryService::isSaved(const QString& bookId) const {
    if (m_libraryData.isEmpty()) refreshFromServer();
    const QJsonArray savedFromPayload = m_libraryData.value("savedBookIds").toArray();
    if (savedFromPayload.contains(bookId)) return true;
    // Check the Wishlist shelf.
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            return s.value("bookIds").toArray().contains(bookId);
        }
    }
    return false;
}

void LibraryService::toggleSaved(const QString& bookId) {
    // Toggle on the server via the Wishlist shelf. Find the shelf id first,
    // then AddBookToShelf / RemoveBookFromShelf.
    if (m_libraryData.isEmpty()) refreshFromServer();
    QString wishlistShelfId;
    bool currentlyIn = false;
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            wishlistShelfId = s.value("id").toString();
            currentlyIn = s.value("bookIds").toArray().contains(bookId);
            break;
        }
    }
    if (wishlistShelfId.isEmpty()) return;

    QJsonObject p;
    p["shelfId"] = wishlistShelfId;
    p["bookId"]  = bookId;
    common::Command cmd = currentlyIn
        ? common::Command::RemoveBookFromShelf
        : common::Command::AddBookToShelf;
    auto resp = NetworkService::instance().sendRequest(cmd, p);
    if (resp.isSuccess()) {
        // Refresh cached library so the next read reflects the change.
        m_libraryData = QJsonObject();
        emit wishlistChanged();
        emit libraryChanged();
    }
}

// BUG FIX (Issue 33): `moveToCart` in WishlistViewModel was calling
// `toggleSaved(bookId)`, which is a TRUE toggle — if the book was
// already removed (by a concurrent action or a double-click), it
// re-ADDS the book to the wishlist instead of being a no-op. This
// dedicated `removeFromWishlist` method always sends RemoveBookFromShelf
// (idempotent on the server) so it's safe to call regardless of current
// state. Used by WishlistViewModel::moveToCart / moveSelectedToCart.
void LibraryService::removeFromWishlist(const QString& bookId) {
    if (m_libraryData.isEmpty()) refreshFromServer();
    QString wishlistShelfId;
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("name").toString() == "Wishlist") {
            wishlistShelfId = s.value("id").toString();
            break;
        }
    }
    if (wishlistShelfId.isEmpty()) return;

    QJsonObject p;
    p["shelfId"] = wishlistShelfId;
    p["bookId"]  = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::RemoveBookFromShelf, p);
    if (resp.isSuccess()) {
        m_libraryData = QJsonObject();
        emit wishlistChanged();
        emit libraryChanged();
    }
}

QList<QObject*> LibraryService::shelves() const {
    if (m_libraryData.isEmpty()) refreshFromServer();
    QList<QObject*> result;
    const QJsonArray arr = m_libraryData.value("shelves").toArray();
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        auto* s = new ShelfDto();
        s->fromJson(v.toObject());
        result.append(s);
    }
    return result;
}

// BUG FIX (Issue 6 + Issue 7):
//   - Issue 6: previously `createShelf` did `m_libraryData = resp.payload;`,
//     which OVERWROTE the cached `purchasedBookIds`, `savedBookIds`, and
//     `lastOpenedBookId` (the server's CreateShelf response only contains
//     `{ "shelves": [...] }`). Subsequent calls to `purchasedCount()`
//     returned 0, and the Profile/Library "My Books" tabs went empty
//     until a manual `refresh()`. We now merge only the `shelves` field
//     into the cache, mirroring the pattern used in `renameShelf` /
//     `deleteShelf` / `addToShelf` / `removeFromShelf`.
//   - Issue 7: the `color` and `isPrivate` parameters were unnamed and
//     silently dropped. The ShelvesPage create form collects a color
//     (8-swatch picker) and a private toggle — both were discarded.
//     We now include them in the request payload.
QString LibraryService::createShelf(const QString& name, const QString& description, const QString& color, bool isPrivate) {
    QJsonObject p;
    p["name"] = name;
    p["description"] = description;
    p["color"] = color;
    p["isPrivate"] = isPrivate;
    auto resp = NetworkService::instance().sendRequest(common::Command::CreateShelf, p);
    if (resp.isSuccess()) {
        // Merge only the shelves field — don't blow away the rest of the cache.
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        }
        emit shelvesChanged();
        // Return the new shelf's id
        const QJsonArray shelves = resp.payload.value("shelves").toArray();
        if (!shelves.isEmpty()) {
            return shelves.last().toObject().value("id").toString();
        }
    }
    return {};
}

bool LibraryService::renameShelf(const QString& id, const QString& name) {
    QJsonObject p;
    p["shelfId"] = id;
    p["name"] = name;
    auto resp = NetworkService::instance().sendRequest(common::Command::RenameShelf, p);
    if (resp.isSuccess()) {
        // Server now returns the updated shelf list — merge it into the cache.
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

bool LibraryService::deleteShelf(const QString& id) {
    QJsonObject p;
    p["shelfId"] = id;
    auto resp = NetworkService::instance().sendRequest(common::Command::DeleteShelf, p);
    if (resp.isSuccess()) {
        // Server now returns the updated shelf list — merge it into the cache.
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

QString LibraryService::duplicateShelf(const QString& id) {
    // v15e: duplicate a shelf by creating a new one with "(copy)" suffix
    // and copying all its books. Fetches the source shelf from the cache.
    if (id.isEmpty()) return {};
    // Find the source shelf in the cache.
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();
    QJsonObject sourceShelf;
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("id").toString() == id) { sourceShelf = s; break; }
    }
    if (sourceShelf.isEmpty()) return {};

    // Create the copy.
    const QString newName = sourceShelf.value("name").toString() + " (copy)";
    const QString newId = createShelf(newName,
                                       sourceShelf.value("description").toString(),
                                       sourceShelf.value("color").toString("#1A73E8"),
                                       false);
    if (newId.isEmpty()) return {};

    // Copy all books from the source shelf to the new shelf.
    const QJsonArray bookIds = sourceShelf.value("bookIds").toArray();
    for (const auto& bid : bookIds) {
        addToShelf(newId, bid.toString());
    }
    return newId;
}

// v15e: setShelfColor — sends an UpdateShelf command to the server with
// the new color. The server persists it and returns the updated shelf list.
bool LibraryService::setShelfColor(const QString& id, const QString& color) {
    QJsonObject p;
    p["shelfId"] = id;
    p["color"] = color;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    if (resp.isSuccess()) {
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

// v15e: setShelfFavorite — sends an UpdateShelf command with favorite flag.
bool LibraryService::setShelfFavorite(const QString& id, bool favorite) {
    QJsonObject p;
    p["shelfId"] = id;
    p["favorite"] = favorite;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    if (resp.isSuccess()) {
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

// v15e: setShelfPrivate — sends an UpdateShelf command with isPrivate flag.
bool LibraryService::setShelfPrivate(const QString& id, bool isPrivate) {
    QJsonObject p;
    p["shelfId"] = id;
    p["isPrivate"] = isPrivate;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    if (resp.isSuccess()) {
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

// v15g: moveShelfUp / moveShelfDown — implemented using UpdateShelf with
// sortOrder. The shelves are sorted by (sortOrder, name) in the DB. To
// move a shelf up, we give it a sortOrder LOWER than the shelf above it.
// To move down, we give it a sortOrder HIGHER than the shelf below.
bool LibraryService::moveShelfUp(const QString& id) {
    if (id.isEmpty()) return false;
    if (m_libraryData.isEmpty()) refreshFromServer();
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();

    // Find the index of the shelf with the given ID.
    int targetIdx = -1;
    for (int i = 0; i < shelves.size(); ++i) {
        if (shelves[i].toObject().value("id").toString() == id) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx <= 0) return false;  // already first or not found

    // Get the sortOrder of the shelf ABOVE (targetIdx - 1).
    const QJsonObject above = shelves[targetIdx - 1].toObject();
    int aboveSort = above.value("sortOrder").toInt(0);

    // Set this shelf's sortOrder to (aboveSort - 1) so it sorts before
    // the shelf that was above it.
    QJsonObject p;
    p["shelfId"] = id;
    p["sortOrder"] = aboveSort - 1;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    if (resp.isSuccess()) {
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

bool LibraryService::moveShelfDown(const QString& id) {
    if (id.isEmpty()) return false;
    if (m_libraryData.isEmpty()) refreshFromServer();
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();

    // Find the index of the shelf with the given ID.
    int targetIdx = -1;
    for (int i = 0; i < shelves.size(); ++i) {
        if (shelves[i].toObject().value("id").toString() == id) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx < 0 || targetIdx >= shelves.size() - 1) return false;  // last or not found

    // Get the sortOrder of the shelf BELOW (targetIdx + 1).
    const QJsonObject below = shelves[targetIdx + 1].toObject();
    int belowSort = below.value("sortOrder").toInt(0);

    // Set this shelf's sortOrder to (belowSort + 1) so it sorts after
    // the shelf that was below it.
    QJsonObject p;
    p["shelfId"] = id;
    p["sortOrder"] = belowSort + 1;
    auto resp = NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    if (resp.isSuccess()) {
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

bool LibraryService::reorderShelves(const QStringList& orderedIds) {
    // v15g: assign sequential sortOrders to all shelves.
    for (int i = 0; i < orderedIds.size(); ++i) {
        QJsonObject p;
        p["shelfId"] = orderedIds[i];
        p["sortOrder"] = i;
        NetworkService::instance().sendRequest(common::Command::UpdateShelf, p);
    }
    // Refresh the cache.
    m_libraryData = QJsonObject();
    refreshFromServer();
    emit shelvesChanged();
    return true;
}

bool LibraryService::addToShelf(const QString& shelfId, const QString& bookId) {
    QJsonObject p;
    p["shelfId"] = shelfId;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::AddBookToShelf, p);
    if (resp.isSuccess()) {
        // Server now returns the updated shelf list — merge it into the cache.
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        // If the shelf being modified is the Wishlist, emit wishlistChanged
        // so the WishlistPage + BookCard hearts update.
        for (const auto& v : m_libraryData.value("shelves").toArray()) {
            const QJsonObject s = v.toObject();
            if (s.value("id").toString() == shelfId && s.value("name").toString() == "Wishlist") {
                emit wishlistChanged();
                break;
            }
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

bool LibraryService::removeFromShelf(const QString& shelfId, const QString& bookId) {
    QJsonObject p;
    p["shelfId"] = shelfId;
    p["bookId"] = bookId;
    auto resp = NetworkService::instance().sendRequest(common::Command::RemoveBookFromShelf, p);
    if (resp.isSuccess()) {
        // Server now returns the updated shelf list — merge it into the cache.
        if (resp.payload.contains("shelves")) {
            m_libraryData["shelves"] = resp.payload.value("shelves");
        } else {
            m_libraryData = QJsonObject();
        }
        // If the shelf being modified is the Wishlist, emit wishlistChanged.
        for (const auto& v : m_libraryData.value("shelves").toArray()) {
            const QJsonObject s = v.toObject();
            if (s.value("id").toString() == shelfId && s.value("name").toString() == "Wishlist") {
                emit wishlistChanged();
                break;
            }
        }
        emit shelvesChanged();
        return true;
    }
    return false;
}

bool LibraryService::moveBookBetweenShelves(const QString& from, const QString& to, const QString& bookId) {
    removeFromShelf(from, bookId);
    return addToShelf(to, bookId);
}

bool LibraryService::copyBookBetweenShelves(const QString&, const QString& to, const QString& bookId) {
    return addToShelf(to, bookId);
}

QList<QObject*> LibraryService::booksInShelf(const QString& shelfId) const {
    QList<QObject*> result;
    if (m_libraryData.isEmpty()) refreshFromServer();
    const QJsonArray shelves = m_libraryData.value("shelves").toArray();

    // Find the shelf and collect its bookIds.
    QJsonArray bookIds;
    for (const auto& v : shelves) {
        const QJsonObject s = v.toObject();
        if (s.value("id").toString() == shelfId) {
            bookIds = s.value("bookIds").toArray();
            break;
        }
    }
    if (bookIds.isEmpty()) return result;

    // Batch-fetch the books in a single round-trip.
    QJsonObject p;
    p["bookIds"] = bookIds;
    auto resp = NetworkService::instance().sendRequest(common::Command::GetBooksByIds, p);
    if (resp.isSuccess()) {
        const QJsonArray books = resp.payload.value("books").toArray();
        for (const auto& v : books) {
            if (!v.isObject()) continue;
            auto* dto = new BookDto();
            dto->fromJson(v.toObject());
            result.append(dto);
        }
    }
    return result;
}

QList<QObject*> LibraryService::searchShelves(const QString&) const { return shelves(); }
QStringList LibraryService::shelfNames() const {
    QStringList names;
    for (QObject* s : shelves()) names.append(s->property("name").toString());
    return names;
}
QObject* LibraryService::shelfById(const QString& id) const {
    for (QObject* s : shelves()) {
        if (s->property("id").toString() == id) return s;
    }
    return nullptr;
}
bool LibraryService::isFavoriteShelf(const QString&) const { return false; }

void LibraryService::refresh() {
    m_libraryData = QJsonObject();
    refreshFromServer();
    emit libraryChanged();
    emit shelvesChanged();
}

} // namespace bookclub::client

