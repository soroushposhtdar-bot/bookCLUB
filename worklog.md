# BookCLUB Backend Audit & Fix Worklog

## Initial Survey
- Read all of `bookCLUB.pdf` (project requirements in Persian).
- Unzipped `BookCLUB V1.zip` and surveyed structure.
- Project is C++/Qt5 headless client-server scaffold.
- Build system: CMake with subdirs `common`, `src/client`, `src/server`.

## Toolchain bootstrap
- Installed cmake 3.31 + Qt 5.15.15 under `/home/z/my-project/tools/` (apt without sudo via `ar x` + `tar xf`).
- `env.sh` exports PATH/LD_LIBRARY_PATH/CMAKE_ROOT/Qt5_DIR.

## Build Attempt #1 — include-path case bug
- All `common/Interfaces/*.cpp` files used lowercase `#include "common/interfaces/..."`, `common/models/...`, `common/utils/...`.
- Filesystem on Linux is case-sensitive, so include guards failed.
- Fix: case-rewrite all `#include "common/(interfaces|models|utils|network|config)/..."` → Capital dirs in `common/Interfaces/*.cpp` files.

## Build Attempt #2 — actual code bugs surface
1. `Book::setActive` missing — `IBookRepository.cpp` uses `book->setActive(bool)` but `Book` only has `activate()`/`deactivate()`.
2. `PasswordHasher::generateSalt` and `hashPassword` do not exist — real API is `hash(password)` (returns "salt$hash") and `verify(password, storedHash)`. `IAuthService.cpp` is wired to a stale API; password verification is also logically broken.
3. `const QSqlQuery& query` then `query.next()` — QSqlQuery::next() is non-const, so the const helper methods don't compile.
4. `QVariantList` initializer-list conversions fail because `INotificationRepository.cpp` doesn't include `<QVariantList>` (and `getQuery` parameter inference breaks).
5. `QJsonObject` used without include in `IOrderRepository.cpp`.
6. `IAuthService::verifyPasswordWithDatabase` compares stored hash against `PasswordHasher::hashPassword(plain, "")` — this can NEVER match a real stored hash because the real hash uses a random salt.


## Build Attempt #3 — link errors
- `createUserRepository`, `createBookRepository`, … factory functions were
  defined in the `.cpp` files but not declared in the corresponding headers.
- Fix: added `createXxx()` declarations to all interface headers
  (`IAuthService.h`, `IUserRepository.h`, `IBookRepository.h`,
  `IOrderRepository.h`, `IReviewRepository.h`, `INotificationRepository.h`,
  `IBookService.h`).

## Build Attempt #4 — clean build
- Both `bookclub_common` and the server/client executables compile cleanly.

## Runtime Attempt #1 — driver not loaded
- `QSqlDatabase: QSQLITE driver not loaded` — the Qt5 sqlite plugin was
  present but `QT_PLUGIN_PATH` was not exported. Fix: env.sh now sets it.

## Runtime Attempt #2 — schema.sql not found
- `DatabaseManager::runSchemaScript` used a hardcoded `../database/schema.sql`
  relative path. Fix: new `locateDatabaseFile()` helper searches cwd,
  applicationDirPath, and a few parent dirs.
- Also `executeSqlScript()` now correctly splits multi-statement SQL
  on `;` and strips `--` line comments, which the previous version did
  not do.

## Runtime Attempt #3 — duplicate schema execution
- `ServerCore::setupDatabase` re-executed schema.sql as a single
  `QSqlQuery::exec()` call (which silently fails because SQLite only runs
  one statement per exec). Removed the redundant block; DatabaseManager
  alone is now responsible for schema + seed.

## Runtime Attempt #4 — QSocketNotifier: Invalid socket
- `ConnectionManager::onNewConnection` stole the socket descriptor from
  the QTcpSocket returned by `nextPendingConnection()` and created a NEW
  QTcpSocket for the same descriptor. Two QTcpSockets on one fd caused
  `QSocketNotifier: Invalid socket 7 and type 'Read', disabling...` and
  no incoming bytes were ever delivered to the handler.
- Fix: `ClientConnection` constructor now takes a `QTcpSocket*` and
  reparents it instead of cloning the descriptor.

## Runtime Attempt #5 — login always fails
- Seed data used placeholder hashes (`'e7c3b8f9...'`) that don't match
  any real password.
- Fix: regenerated seed hashes via `scripts/gen_hash.py` using the
  real `PasswordHasher::hash` algorithm (`salt$sha256(salt+pw)`).
- Login now succeeds for admin/publisher1/amir.

## Runtime Attempt #6 — checkout fails (500)
Two issues:
1. `purchaseCart` in `IBookService.cpp` called `m_orderRepo->save(order)`
   AND `handleCheckout` called `m_orderRepo->save(order)` again, causing
   `UNIQUE constraint failed: Orders.id`.
   Fix: `purchaseCart` no longer persists the order; `handleCheckout`
   is the single place that saves (after marking paid+completed).
2. `handleAddToCart` created a CartItem with empty title and 0 price,
   so cart total was always 0 and checkout had nothing meaningful to
   persist.
   Fix: `handleAddToCart` now loads book info (title, basePrice,
   discountValue, isActive) from the database before adding the item.

## Runtime Attempt #7 — library always empty
- `LibraryRequestHandler::getOrCreateLibrary` returned an empty
  `UserLibrary`; shelves and purchased books were never loaded.
- Fix: on first access, `purchasedBookIdsFor()` walks `Orders.items`
  JSON; `reloadShelves()` pulls rows from the `Shelves` table. Shelf
  create/delete/add-book now also persist to the database via
  `persistShelf` / `deleteShelfFromDatabase`.

## NotificationDispatcher wiring
- `NotificationDispatcher` existed but was never instantiated by
  `ServerCore`. Patched `ServerCore.h/.cpp` to construct it and expose
  it via `ServerCore::notificationDispatcher()`.

## ConnectionManager user mapping
- `m_userToClientMap` was declared but never populated, so
  `getConnectionByUserId` always returned nullptr (notifications and
  study-session broadcasts never reached their recipient).
- Fix: added `registerUser` / `unregisterUser` to `ConnectionManager`,
  and `ClientConnection::setUserId` now keeps the mapping in sync.

## LibraryShelf id generation
- `UserLibrary::createShelf` constructed `new LibraryShelf(this)` but
  never assigned an id, so `shelf->id()` was empty and downstream
  `AddBookToShelf` / `DeleteShelf` operations could never find the shelf.
- Fix: `LibraryShelf` default constructor now auto-generates a UUID,
  and `UserLibrary::createShelf` explicitly assigns a fresh UUID as
  well (defensive).

## Use-after-free in PublisherRequestHandler
- `handlePublishBook` did `delete book; LOG_INFO("Book published: " +
  book->title())` — using `book` after delete.
- Fix: capture `const QString bookTitle = book->title();` before delete.

## Smoke test results
`scripts/e2e_test.py` passes all 14 checks:
  1. admin login
  2. publisher1 login
  3. amir login
  4. GetHomeSections returns 3 featured / 3 new / 3 best / 0 free
  5. SearchBooks 'Qt' returns book-001
  6. GetBookDetails returns book-001 details
  7. AddToCart(book-001) -> 200
  8. GetCart -> total=35.0
  9. Checkout -> 200, orderId returned
 10. GetLibrary -> purchasedBookIds includes book-001
 11. CreateShelf + AddBookToShelf -> 200
 12. GetNotifications -> 1 unread
 13. GetPublisherAnalytics -> totalBooks=3
 14. GetUsersList -> 3 users

Verified after restart that Orders, Shelves and Books.totalSales persist
correctly in the SQLite database.
