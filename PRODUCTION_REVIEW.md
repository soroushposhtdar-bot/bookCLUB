# BookCLUB — Step 1 Production Code Review

## Executive Summary

The application is functionally complete and the frontend is fully
connected to the backend. However, it is **not yet production-quality**.
This review identifies 23 issues across 8 categories, with 7 critical
bugs that must be fixed before delivery.

## Critical Issues (must fix)

### C1. Server is single-threaded — ThreadPool exists but is never used
**Location**: `src/server/ConnectionManager.cpp`, `ClientConnection.cpp`,
`RequestRouter.cpp`

`ThreadPool.h/.cpp` exists and is compiled, but no handler call goes
through it. Every request is processed on the Qt event loop thread. A
single slow query (e.g., a 500ms SQLite write) blocks **every** other
connected client for that duration. With 10 concurrent users, the app
feels frozen.

**Fix**: Move `RequestRouter::handleRequest` to the thread pool. Each
`ClientConnection::processPendingPackets` call submits a task that runs
the handler on a worker thread. The handler's `client->sendMessage`
response is marshalled back to the I/O thread via `QMetaObject::invokeMethod`.

### C2. `NetworkService::sendRequest` blocks the UI thread
**Location**: `client/src/services/NetworkService.cpp:55-93`

The synchronous `sendRequest` uses a `QEventLoop` that blocks the QML
thread for up to 5 seconds per call. Every service method
(`BookService::bestsellers`, `CartService::items`,
`LibraryService::purchasedBooks`, etc.) calls this synchronously from
QML property getters. The UI freezes during every network round-trip.

**Fix**: Convert the hot-path services (Book, Cart, Library, User) to
use `sendRequestAsync` + signals. The ViewModels already have NOTIFY
signals — wire them to async responses instead of synchronous returns.

### C3. `ClientNetworkManager::connectToServer` blocks for 5 seconds
**Location**: `src/client/network/ClientNetworkManager.cpp:50`

`m_socket->waitForConnected(5000)` blocks the calling thread (which is
the QML thread) for up to 5 seconds during auto-connect. The app appears
hung on startup if the server is down.

**Fix**: Don't block. Connect asynchronously and emit `connected()` /
`errorOccurred()` signals.

### C4. `searchBooks` only searches by title
**Location**: `common/Interfaces/IBookService.cpp:100-102`

`searchBooks(keyword)` calls `m_bookRepo->searchByTitle(keyword)` only.
The spec (§2-2) requires searching by **title, author, AND publisher**
simultaneously.

**Fix**: Make `searchBooks` query all three fields via `OR LIKE` and
deduplicate the results.

### C5. No input validation on the server
**Location**: all `src/server/handlers/*.cpp`

While `validateRequiredFields` checks for missing fields, there's no
validation of:
- String length limits (username, password, book title, etc.)
- Numeric ranges (price ≥ 0, stars 1-5, quantity > 0)
- Email format
- File path safety (publisher upload paths)

The DB has `CHECK` constraints that catch some of these, but the error
messages are cryptic SQL errors instead of user-friendly validation
messages.

**Fix**: Add a `ValidationUtils::validate*` set of functions on the
server and call them before DB writes. Return `Status::ValidationError`
(422) with a clear message.

### C6. Real-time events are broadcast but not all are wired on the client
**Location**: `client/src/services/`

The server pushes `EvtNotification`, `EvtBookAdded`, `EvtDiscountApplied`,
`EvtReviewUpdated`, `EvtStudySync`. The client subscribes to all 5 in
the respective services. **However**, the server only **fires** these
events from `NotificationDispatcher`. The actual business logic that
should trigger them doesn't:

- `PublisherRequestHandler::handlePublishBook` doesn't call
  `notifyNewBook()` → no `EvtBookAdded` is ever fired
- `PublisherRequestHandler::handleApplyTimedDiscount` doesn't call
  `notifyDiscountOnBook()` → no `EvtDiscountApplied` is ever fired
- `BookRequestHandler::handleSubmitReview` doesn't call
  `notifyNewReview()` → no `EvtReviewUpdated` is ever fired
- `CartRequestHandler::handleCheckout` doesn't call
  `notifyNewSale()` → publishers never learn about sales in real-time

**Fix**: Wire the dispatcher calls into the handlers. The dispatcher is
already created in `ServerCore` — pass it to the handlers that need it.

### C7. No request timeout / queue on the server
**Location**: `src/server/ClientConnection.cpp`

If a client sends a malformed packet that causes the handler to hang
(e.g., an infinite loop in a search query), the server thread is stuck
forever. There's no request timeout, no max-connections limit, no rate
limiting.

**Fix**: Add a per-request timeout (e.g., 10s), a max-connections cap
(e.g., 100), and basic rate limiting (e.g., 50 req/min per client).

## High-Severity Issues

### H1. Memory management in handlers is inconsistent
**Location**: `src/server/handlers/*.cpp`

Some handlers use `std::unique_ptr` (good), others use raw `new` with
manual `delete` (error-prone). Examples:
- `LibraryRequestHandler::handleCreateShelf` — raw `new common::LibraryShelf`,
  manual `delete` (OK but fragile)
- `BookRequestHandler::handleSubmitReview` — raw `new common::Review`,
  manual `delete` (OK)
- `NotificationDispatcher::sendNotifications` — raw `new common::Notification`
  per user, manual `delete` (OK)
- `AuthRequestHandler::createUserFromPayload` — returns raw pointer,
  caller must delete (documented but fragile)

**Fix**: Standardize on `std::unique_ptr` for all heap-allocated model
objects in handlers. Use `QScopedPointer` for Qt-parented objects.

### H2. `AdminService::allBooks` uses `GetHomeSections` as a proxy
**Location**: `client/src/services/AdminService.cpp:75-87`

`allBooks()` calls `GetHomeSections` and concatenates `featured` +
`newBooks` + `bestSellers` + `freeBooks`. This misses inactive books
and duplicates books that appear in multiple sections. The admin can't
see all books, and sees some twice.

**Fix**: Add a dedicated `GetAllBooks` admin command on the server
that returns every book regardless of `isActive`.

### H3. `AdminService::flaggedReviews` iterates all books (N+1 queries)
**Location**: `client/src/services/AdminService.cpp:107-116`

For each book in `allBooks()`, it calls `reviewsForBook(bookId)` which
fires a `GetBookDetails` request. With 100 books, that's 100 round-trips.

**Fix**: Add a server-side `GetFlaggedReviews` command that runs a
single SQL query: `SELECT * FROM Reviews WHERE isFlagged = 1`.

### H4. `PublisherService::reviewsList` iterates all publisher books (N+1)
**Location**: `client/src/services/PublisherService.cpp:383-399`

Same N+1 pattern as H3 — one `GetBookDetails` per book.

**Fix**: Add a server-side `GetReviewsByPublisher` command.

### H5. `StatisticsService` is never instantiated
**Location**: `src/server/ServerCore.cpp`

`StatisticsService` is a singleton but `ServerCore` never creates it.
`ServerInfoRequestHandler` receives `nullptr` for the stats parameter.
The handler currently does its own CPU/RAM sampling, so it works, but
the `StatisticsService` code is dead.

**Fix**: Either wire `StatisticsService` into `ServerCore` and pass it
to `ServerInfoRequestHandler`, or delete `StatisticsService` entirely.

### H6. No pagination on list endpoints
**Location**: all list endpoints (`GetUsersList`, `GetPublisherBooks`,
`GetHomeSections`, `SearchBooks`, etc.)

Every list endpoint returns the full result set. With 10,000 books or
1,000 users, the JSON payload could be megabytes, and the client
allocates a `QList<QObject*>` of that size.

**Fix**: Add `offset` + `limit` parameters to list commands. Return a
total count alongside the page so the client can render pagination
controls.

### H7. No connection-state UI on the client
**Location**: `client/qml/`

The `OfflineBanner.qml` component exists but isn't wired to
`ClientNetworkManager::disconnected`. If the server goes down, the user
sees no indication — buttons just stop working silently.

**Fix**: Add a `ConnectionState` QML singleton (or Q_PROPERTY on
`NetworkService`) that exposes `isConnected`. Bind `OfflineBanner.visible`
to it. Disable checkout/review/wishlist buttons when offline.

## Medium-Severity Issues

### M1. `BookService::fetchHomeSections` cache never expires
**Location**: `client/src/services/BookService.cpp:52-57`

The cache is cleared on `EvtBookAdded` / `EvtDiscountApplied`, but not
on a timer. If the user leaves the app open for hours, the home feed
shows stale data.

**Fix**: Add a 60-second TTL to the cache.

### M2. `LibraryService` cache is never invalidated on shelf changes
**Location**: `client/src/services/LibraryService.cpp`

`m_libraryData` is cached, but `createShelf` / `deleteShelf` /
`addToShelf` / `removeFromShelf` only clear it on success. If the server
returns an error, the cache stays stale.

**Fix**: Always clear `m_libraryData` after any shelf mutation,
regardless of success/failure.

### M3. `NotificationDispatcher::broadcastSystemMessage` loads ALL users
**Location**: `src/server/NotificationDispatcher.cpp:96-104`

`userRepo->findAll()` loads every user into memory, creates a
`Notification` per user, and saves each one individually. With 10,000
users, that's 10,000 DB writes in a single call.

**Fix**: Use a single SQL `INSERT INTO Notifications ... SELECT id FROM
Users WHERE role = ?` to bulk-insert.

### M4. `ServerInfoRequestHandler::handleGetServerLogs` reads the log file
**Location**: `src/server/handlers/ServerInfoRequestHandler.cpp:139-167`

The handler opens `logs/server.log`, seeks to the last 64KB, and parses
100 lines. This happens on every 5-second poll from the operator
dashboard. With heavy logging, the file could be large and the seek
slow.

**Fix**: Cache the last 100 log lines in memory (ring buffer) and serve
from the cache. Write to the file in a background flush.

### M5. No transaction wrapping for multi-statement operations
**Location**: `CartRequestHandler::handleCheckout`,
`LibraryRequestHandler::handleCreateShelf`

Checkout writes the order + order items + bumps book sales in separate
SQL statements. If the server crashes mid-way, the data is inconsistent.

**Fix**: Wrap multi-statement operations in `DatabaseManager::beginTransaction`
/ `commitTransaction` / `rollbackTransaction`.

### M6. `PasswordHasher` uses SHA-256 (fast) instead of a slow KDF
**Location**: `common/Utils/PasswordHasher.cpp`

SHA-256 with a salt is OK for a course project, but a production system
should use bcrypt, scrypt, or Argon2. An attacker with the DB can brute
force SHA-256 at billions of attempts per second on a GPU.

**Fix**: For a course project, SHA-256 + salt is acceptable. Document
the limitation. For production, swap in `QPasswordDigestor::deriveKeyPbkdf2`
(available in Qt 6) with 100,000+ iterations.

### M7. `ClientNetworkManager::onReconnectTimeout` blocks
**Location**: `src/client/network/ClientNetworkManager.cpp:198-206`

The old version called `waitForConnected(3000)` inside the reconnect
slot. I already fixed this in the previous phase (removed the wait), but
the reconnect logic still has a subtle bug: if the socket is in a
connecting state when the timer fires, `connectToHost` is called again,
which can cause `QAbstractSocket::UnsupportedSocketOperationError`.

**Fix**: Check `m_socket->state()` before calling `connectToHost`. Only
reconnect if the state is `UnconnectedState`.

## Low-Severity Issues

### L1. Inconsistent error response format
Some handlers put the error message in `payload["error"]`, others in
`errorMessage`. The client reads `payload["error"]` only.

### L2. `Logger` doesn't rotate
`logs/server.log` grows forever. Add size-based rotation.

### L3. No graceful shutdown
`ServerApplication::stop()` calls `m_server->shutdown()` then
`m_app->quit()`, but doesn't wait for in-flight requests to complete.

### L4. `e2e_test.py` doesn't test real-time events
The test covers request/response but not `EvtNotification` push.

### L5. No `.gitignore` for build artifacts
The repo includes `build/` and `bookclub.db` in some snapshots.

### L6. `README.md` is one line
Should have install/run instructions, demo accounts, architecture
overview.

## Summary Table

| # | Severity | Category | Issue |
|---|----------|----------|-------|
| C1 | Critical | Performance | Server is single-threaded |
| C2 | Critical | UI | NetworkService blocks UI thread |
| C3 | Critical | UI | connectToServer blocks 5s |
| C4 | Critical | Correctness | searchBooks only searches title |
| C5 | Critical | Security | No server-side input validation |
| C6 | Critical | Realtime | Events never fired by handlers |
| C7 | Critical | Reliability | No request timeout / limits |
| H1 | High | Memory | Inconsistent new/delete in handlers |
| H2 | High | Correctness | Admin can't see all books |
| H3 | High | Performance | N+1 query for flagged reviews |
| H4 | High | Performance | N+1 query for publisher reviews |
| H5 | High | Dead code | StatisticsService never used |
| H6 | High | Performance | No pagination |
| H7 | High | UX | No connection-state UI |
| M1 | Medium | Performance | Home cache never expires |
| M2 | Medium | Correctness | Library cache stale on error |
| M3 | Medium | Performance | Broadcast loads all users |
| M4 | Medium | Performance | Log handler reads file every poll |
| M5 | Medium | Reliability | No transactions for multi-statement ops |
| M6 | Medium | Security | SHA-256 instead of slow KDF |
| M7 | Medium | Reliability | Reconnect state check missing |
| L1-L6 | Low | Various | Minor polish items |

## Plan of attack

I'll fix these in priority order, module by module:

1. **Step 2 (Realtime)**: Fix C6 — wire event firing into handlers
2. **Step 3 (Server monitoring)**: Fix H5 — wire StatisticsService, add real metrics
3. **Step 4 (Performance)**: Fix C1 (thread pool), C2 (async services), H6 (pagination), H3/H4 (N+1 queries), M1/M2 (cache TTL)
4. **Step 5 (Security)**: Fix C5 (input validation), C7 (limits), M5 (transactions), M6 (document password hashing)
5. **Step 6 (Advanced features)**: Fix C4 (multi-field search), add timed-discount auto-expiration, wire publisher analytics, finish group reading
6. **Step 7 (Error handling)**: Fix L1 (error format), L2 (log rotation), L3 (graceful shutdown), M7 (reconnect)
7. **Step 8 (Delivery)**: Fix L5 (.gitignore), L6 (README), verify build
