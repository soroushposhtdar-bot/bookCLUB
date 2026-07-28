# BookCLUB Backend — Phase 2/3/4/5 Changes

This document summarises every change made to the backend in this pass.
The goal was to convert the project from a prototype into a real,
data-driven, production-quality Client-Server system.

## Phase 1 — Analysis

A complete analysis was performed (see previous conversation). The V3
codebase already had:

- A QML client with MVVM and a `NetworkService → ClientNetworkManager →
  server` data path
- A headless C++ TCP server with `ConnectionManager → RequestRouter →
  8 handlers`
- A SQLite-backed `DatabaseManager` that loaded `schema.sql` + seed data
- Repository implementations in `common/Interfaces/*.cpp`

Critical bugs identified:

1. `App.qml` still referenced `_dataStore` (the removed MockDataStore)
2. `AuthService::instance()` returned a different object from the QML singleton
3. `NetworkService::sendRequest` used a synchronous QEventLoop that froze the UI
4. `HomeViewModel` triggered 8 round-trips per refresh; `LibraryService` did N
5. Multiple files re-called `QSqlDatabase::addDatabase("bookclub_shared")`
6. Real-time notifications were dropped (no client handler)
7. PDF reader was fake; publisher addBook discarded cover/PDF
8. CartService::checkout emitted `[orderId]` instead of purchased book IDs
9. No role checks on admin/publisher handlers
10. No `EVT_*` server-pushed commands in `Protocol.h`
11. `InMemoryRepositories.h` was dead code

## Phase 2 — Database & Data Layer

### `database/schema.sql` — full rewrite

The new schema has 21 tables (was 10) with proper relationships,
constraints, and indexes:

| Table | Purpose |
|-------|---------|
| `Roles` | Lookup table for the 3 account roles |
| `Users` | Base account table for all roles |
| `Publishers` | Extended profile for role=1 users |
| `Authors` | Normalised author table |
| `Genres` | Normalised genre table |
| `Books` | Book metadata + denormalised `authorName` for fast search |
| `BookGenres` | Many-to-many Books↔Genres |
| `Carts` | One-per-user cart header |
| `CartItems` | Cart line items |
| `Orders` | Checkout records |
| `OrderItems` | Normalised order line items (was JSON blob before) |
| `Libraries` | One-per-user library metadata |
| `Shelves` | User-created collections |
| `ShelfBooks` | Many-to-many Shelves↔Books |
| `Reviews` | Text reviews (UNIQUE on bookId+userId) |
| `Ratings` | Lightweight star-only ratings (separate from reviews) |
| `Discounts` | Time-boxed promotions with `CHECK (endsAt > startsAt)` |
| `Notifications` | User-scoped notifications |
| `StudySessions` | Group-reading sessions |
| `StudySessionParticipants` | Many-to-many |
| `Sessions` | Auth tokens |

**Constraints added:**
- `CHECK` on `status`, `role`, `visibility`, `availability`, `stars` (1-5),
  `quantity > 0`, `basePrice >= 0`, `discountValue >= 0`, `endsAt > startsAt`
- `UNIQUE` on `Users.username` (case-insensitive), `Users.email`,
  `Reviews(bookId,userId)`, `Ratings(bookId,userId)`, `CartItems(userId,bookId)`
- `FOREIGN KEY` on every reference field with `ON DELETE CASCADE` / `RESTRICT`
- 65 indexes for query performance

### `database/seeds/sample_data.sql` — full rewrite

- Fresh password hashes generated via `scripts/gen_hash.py` (correct
  `salt$sha256(salt+pw)` format)
- 4 users (admin, publisher1, amir, sara) with all 3 roles covered
- 1 publisher profile
- 4 authors
- 8 genres
- 5 books (one is free)
- BookGenres mappings
- 2 carts
- 2 libraries
- 4 shelves (including system shelves "Read" and "Wishlist")
- ShelfBooks mappings
- 3 reviews (UNIQUE on bookId+userId enforced)
- 3 ratings (separate from reviews)
- 1 time-boxed discount
- 2 notifications
- 1 active study session with 2 participants

### New `common/Utils/DbConnection` — centralised DB access

A new `DbConnection` singleton provides `database()`, `run()`, `execOk()`,
and `lastErrorText()`. All repositories now go through this helper instead
of each opening their own private `QSqlDatabase::addDatabase` connection
(fixes bug #5). The shared connection name `bookclub_shared` is created
exactly once by `DatabaseManager::initialize()` and reused everywhere.

### Repository pattern — 14 interfaces, all SQL-backed

Every entity now has a proper CRUD repository. Existing ones were rewritten
to use `DbConnection`; new ones were added.

| Interface | Status | Key operations |
|-----------|--------|----------------|
| `IUserRepository` | rewritten | save/update/findById/findByUsername/findAll/search/remove/blockUser/unblockUser/setAccountStatus/registeredAt |
| `IBookRepository` | rewritten | save/update/findById/findAll/findByPublisher/searchByTitle/Author/PublisherName/GenreIds/remove/activate/deactivate/reviewsOf/attachReview/attachDiscount |
| `IOrderRepository` | rewritten | save/update/findById/findByUser/findByPublisher/findAll/totalSalesCount |
| `IReviewRepository` | rewritten | findById/findByBook/findByUser/save/update/remove/averageRating/ratingCount + auto-recalc of book's averageRating |
| `INotificationRepository` | rewritten | save/update/findById/findByUser/findUnreadByUser/markAsRead/markAllAsRead |
| `IAuthService` | rewritten | registerAccount/login/logout/changePassword/resetPassword/isUsernameUnique |
| `IAuthorRepository` | **NEW** | findById/findByName/findAll/save/remove |
| `IGenreRepository` | **NEW** | findById/findByName/findAll/allNames/save/remove/attachToBook/detachFromBook/genresOfBook |
| `ICartRepository` | **NEW** | getOrCreateForUser/save/addItem/updateItemQuantity/removeItem/clear |
| `IShelfRepository` | **NEW** | findById/findByUser/save/remove/addBook/removeBook/bookIdsOf |
| `IRatingRepository` | **NEW** | ratingOfUser/setRating/removeRating/averageRating/count + auto-recalc |
| `IDiscountRepository` | **NEW** | findById/findByBook/findActiveByBook/save/remove/deactivateExpired |
| `IStudySessionRepository` | **NEW** | findById/findByBook/findActiveByBook/findByHost/findByParticipant/save/update/remove/addParticipant/removeParticipant/participants |

### New `common/Models/Author` model

Added to support the normalised `Authors` table.

## Phase 3 — Authentication System

### `common/Interfaces/IAuthService.cpp` — full rewrite

- `registerAccount`: validates uniqueness, hashes password + security
  answer (via `PasswordHasher::hash`), persists to `Users` (+ `Publishers`
  if role=publisher), returns the saved user.
- `login`: case-insensitive lookup, verifies password with
  `PasswordHasher::verify`, **enforces status checks** — blocked / disabled
  / deleted users cannot log in.
- `logout`: server-side no-op (sessions tracked via `ClientConnection`).
- `changePassword`: verifies old password, hashes new, updates.
- `resetPassword`: verifies security answer (case-insensitive, trimmed),
  hashes new password, updates.
- `isUsernameUnique`: case-insensitive check.

### `RequestHandlerBase` — new permission helpers

- `requireRole(client, role, cmd)`: returns true iff the client is
  authenticated AND has the given role. Sends 403 Forbidden otherwise.
- `requireAnyRole(client, {roles}, cmd)`: same but accepts multiple roles.
- `getAuthenticatedUser(client)`: now actually loads the user from the
  database (was returning `nullptr` before).

### `AdminRequestHandler` — admin role enforced

Every admin command now calls `requireRole(client, AccountRole::Admin, cmd)`
before processing. Non-admins get 403.

### `PublisherRequestHandler` — publisher role enforced

Every publisher command now calls
`requireAnyRole(client, {Publisher, Admin}, cmd)`. Non-publishers get 403.

## Phase 4 — Server Core

### `common/Network/Protocol.h` — extended

The `Command` enum was reorganised:
- Commands 1-999 are client→server REQUESTS (server replies with same command + status)
- Commands 1000+ are server→client PUSHED EVENTS (no reply expected)

**New request commands:**
- `GetBooksByIds` (batch fetch — kills the N-round-trip anti-pattern)
- `ClearCart`
- `RenameShelf`
- `SubmitReview`, `UpdateReview`, `DeleteReview`
- `SetRating`
- `ToggleWishlist`, `GetWishlist`
- `GetCurrentUser`, `UpdateProfile`, `SaveFavoriteGenres`
- `GetGenres`, `GetAuthors`
- `GetServerLogs`, `GetServerClients`

**New server-pushed events:**
- `EvtNotification` — real-time notification push
- `EvtReviewUpdated` — a review was added/edited on a book
- `EvtStudySync` — group-reading page sync
- `EvtBookAdded` — new book in user's favourite genre
- `EvtDiscountApplied` — discount applied to a saved book
- `EvtUserBlocked` — admin pushed a block to a live user
- `EvtServerShutdown` — graceful shutdown notice

Helper `isEventCommand(cmd)` distinguishes events from requests.

### `NotificationDispatcher` — full rewrite

- All pushes now use `Command::EvtNotification` (was reusing `GetNotifications`)
- Every pushed notification is **persisted to the database** so offline
  users see it on next login
- `notifyNewBook` actually looks up users whose favourite genres match
  the new book's genres
- `broadcastToRole` actually filters by role from the database
- `sendNotifications` builds fresh `Notification` objects per user
  (QObjects can't be copied)

### `CartRequestHandler` — rewritten to use `ICartRepository`

- Cart state is now **persistent** across server restarts (was in-memory `QMap`)
- `handleAddToCart` verifies the book exists and is active before adding
- `handleCheckout` includes `purchasedBookIds` array in the response
  (fixes bug #9 — client's `LibraryService` can refresh without extra round-trip)
- Bumps `Books.totalSales` for each purchased item
- `handleClearCart` added (new command)

### `LibraryRequestHandler` — rewritten to use `IShelfRepository`

- Removed the dead `loadShelvesFromDatabase()` function
- Removed the in-memory `m_userLibraries` cache — every request hits the DB
- `handleRenameShelf` added (new command)
- Ownership check on rename — users can only rename their own shelves
- Purchased book IDs queried from `OrderItems` directly (was parsing JSON
  from `Orders.items`)

### `BookRequestHandler` — extended

- `handleGetBooksByIds` added — batch fetch by list of IDs
- `bookToJson` now joins `publisherName` from `Users` so the client doesn't
  need a second round-trip
- `bookToJson` includes `createdAt` for "new releases" sorting

### `ServerCore::registerDefaultHandlers` — rewired

- Instantiates all 12 repository singletons via their factories
- Registers every command in `Protocol.h` to its handler (was missing
  `ClearCart`, `RenameShelf`, `GetBooksByIds`, etc.)
- Calls `DbConnection::database()` after `DatabaseManager::initialize()`
  to warm the shared connection

### `DatabaseManager::runSchemaScript` — idempotent

- Now re-runs the schema with `IF NOT EXISTS` on every startup, applying
  any new tables/indexes added in later schema versions (poor man's migration)

## Phase 5 — Code Quality + Bug Fixes

### `App.qml` — fixed `_dataStore` reference (bug #1)

Both `onLoginSuccess` and the genre-page `onCompleted` were calling
`_dataStore.setCurrentUser(...)` / `_dataStore.setFavoriteGenres(...)`,
but `MockDataStore` was no longer instantiated. Removed the dead calls;
services now talk to the server directly.

### `client/main.cpp` — fixed `AuthService::instance()` split (bug #2)

The QML singleton provider was creating its own `AuthService` instance,
separate from the `static AuthService s_instance` returned by
`AuthService::instance()`. As a result, `UserService::username()` and
`PublisherService::publisherName()` always read empty strings.

The provider now returns `&AuthService::instance()` so QML and C++ share
the same object.

### `client/src/services/CartService.cpp` — fixed checkout (bug #9)

Now reads `purchasedBookIds` array from the server response and emits
the proper list via `checkoutSucceeded`.

### `client/src/services/LibraryService.cpp` — fixed N-round-trip (bug #4)

`purchasedBooks()` and `booksInShelf()` now use `GetBooksByIds` to
batch-fetch all books in a single round-trip (was N round-trips before).

### `client/src/services/BookService.cpp` — added cache (bug #4)

`fetchHomeSections()` caches the `GetHomeSections` response so the 8
section getters (bestsellers / freeBooks / newArrivals / etc.) don't
each fire a separate request.

### `client/src/services/NotificationService.cpp` — real-time handler (bug #6)

The constructor now registers a handler for `Command::EvtNotification`
that prepends the notification to the cache and emits
`notificationsChanged()` + `notificationReceived(dto)`. The QML bell
icon and toast host can bind to these signals.

### `src/server/InMemoryRepositories.h` — removed (dead code)

Was 418 lines of in-memory repository implementations that were no
longer used. Removed from disk and from `src/server/CMakeLists.txt`.

### `common/CMakeLists.txt` — updated

Added all new sources: `Models/Author.cpp`, `Utils/DbConnection.cpp`,
and 7 new `Interfaces/*.cpp` files. Updated headers list accordingly.

### `scripts/e2e_test.py` — extended

Now covers 18 test cases (was 13):
- Original: login, home sections, search, book details, cart, checkout,
  library, shelf, notifications, publisher analytics
- New: `GetBooksByIds` batch fetch, `ClearCart`, admin permission check
  (publisher gets 403 on `BlockUser`), user permission check (user gets
  403 on `PublishBook`), logout

## Validation

- Schema validated with sqlite3: 21 tables, 65 indexes, all CHECK/UNIQUE/FK
  constraints work
- Seed data validated: 4 users, 5 books, 3 reviews, 3 ratings, 4 shelves,
  1 discount, 2 notifications, 1 study session — all password hashes
  verify correctly with `PasswordHasher::verify`
- Business logic simulated end-to-end in Python: login → add to cart →
  checkout → library refresh → review submission → shelf creation →
  study session join — all passed

## What's NOT in this pass (front-end UI work)

Per the task instructions, the QML UI was NOT modified (except for the
two `_dataStore` reference removals in `App.qml`, which were blocking
bugs). The next phase should:

1. Add an `OfflineBanner` driven by `ClientNetworkManager::disconnected`
2. Wire the QML bell icon to `NotificationService::notificationReceived`
3. Replace `PdfReaderPage.qml`'s placeholder with `QtPdf` + `PdfPageView`
4. Add cover-image + PDF file pickers to the publisher "Add Book" form
5. Make `NetworkService::sendRequest` non-blocking (use signal/slot
   instead of `QEventLoop`)

## Build instructions

```bash
cd bookCLUB
mkdir build && cd build
cmake .. -DBOOKCLUB_BUILD_CLIENT=OFF -DBOOKCLUB_BUILD_SERVER=ON
cmake --build . -j8
./bin/BookClubServer -p 8080
```

Then in another terminal:
```bash
python3 scripts/e2e_test.py
```

The server will create `bookclub.db` in the working directory, apply the
schema, seed the data, and start listening on port 8080.
