# BookCLUB — User Module

Complete documentation for the BookCLUB **regular-user** subsystem. This module
covers everything a logged-in `user` role can do — browsing the catalog,
searching/filtering, viewing book details, managing the cart and library,
reading PDFs, reviewing books, tracking notifications, managing the profile
and wishlist, organizing shelves, and joining group reading sessions.

> For the login/registration/password-recovery flow, see
> [`AUTH_README.md`](./AUTH_README.md). For role-specific dashboards
> (publisher / admin / server), see the corresponding role docs.

---

## 1. Overview

```
                     ┌─────────────────────────────────────────┐
                     │              UserShell                  │
                     │  (sidebar + topbar + page loader)       │
                     └───────────────────┬─────────────────────┘
                                         │
   ┌──────────┬──────────┬──────────────┼──────────────┬──────────────┬──────────┐
   ▼          ▼          ▼              ▼              ▼              ▼          ▼
 Home       Search    BookDetail      Cart          Library      Notifications  Profile
   │          │          │              │              │              │          │
   │          │          │              │              │              │          │
   ▼          ▼          ▼              ▼              ▼              ▼          ▼
HomeVM    SearchVM   BookDetailVM    CartVM        LibraryVM    NotifsVM     ProfileVM
   │          │          │              │              │              │          │
   └──────────┴────┬─────┴──────┬───────┴───────┬──────┴──────┬───────┴────┬─────┘
                   ▼            ▼               ▼             ▼            ▼
              BookService   CartService    LibraryService  NotifSvc   UserService
                   │            │               │             │            │
                   └────────────┴───────┬───────┴─────────────┴────────────┘
                                        ▼
                                 MockDataStore
                              (single source of truth)
```

The user role is routed to `UserShell.qml` after login. The shell owns all 13
User ViewModels, injects the shared services into them, and routes between
the 11 user-facing pages via a `Loader` whose `sourceComponent` is selected
from `_componentMap[route]`.

---

## 2. Architecture (MVVM)

Every user page follows the same MVVM split:

```
┌─────────────────────────┐        ┌──────────────────────────┐
│  QML Page (View)        │  data  │  C++ ViewModel           │
│  - HomePage.qml         │ ◀────▶ │  - HomeViewModel         │
│  - SearchPage.qml       │ binds  │  - SearchViewModel       │
│  - BookDetailPage.qml   │        │  - BookDetailViewModel   │
│  - CartPage.qml         │        │  - CartViewModel         │
│  - LibraryPage.qml      │        │  - LibraryViewModel      │
│  - PdfReaderPage.qml    │        │  - ReaderViewModel       │
│  - NotificationsPage    │        │  - NotificationsViewModel│
│  - ProfilePage.qml      │        │  - ProfileViewModel      │
│  - WishlistPage.qml     │        │  - WishlistViewModel     │
│  - SettingsPage.qml     │        │  - SettingsViewModel     │
│  - ShelvesPage.qml      │        │  - ShelfViewModel        │
│  - GroupReadingPage     │        │  - StudySessionViewModel │
└─────────────────────────┘        └────────────┬─────────────┘
                                                │ calls
                                                ▼
                                ┌──────────────────────────────┐
                                │  Services (QML singletons)   │
                                │  - BookService               │
                                │  - CartService               │
                                │  - LibraryService            │
                                │  - NotificationService       │
                                │  - ReaderService             │
                                │  - UserService               │
                                └────────────┬─────────────────┘
                                             │ delegates to
                                             ▼
                                ┌──────────────────────────────┐
                                │  MockDataStore (shared)      │
                                │  - catalog (25 books)        │
                                │  - reviews                   │
                                │  - wishlist / purchased      │
                                │  - shelves                   │
                                │  - notifications             │
                                │  - purchase history          │
                                │  - reading progress          │
                                │  - downloaded (offline)      │
                                │  - recent searches           │
                                └──────────────────────────────┘
```

| Layer         | Files                                                              |
|---------------|-------------------------------------------------------------------|
| **View (QML)**| `client/qml/user/*.qml` + `client/qml/layouts/DashboardLayout.qml` |
| **ViewModel** | `client/include/viewmodels/user/*.h` + `client/src/viewmodels/user/*.cpp` |
| **Service**   | `client/include/services/*.h` + `client/src/services/*.cpp`       |
| **DTOs**      | `BookDto`, `ReviewDto`, `ShelfDto`, `PurchaseDto`, `NotificationDto`, `CartItemDto`, `RatingDistDto`, `FilterChipDto` |
| **Mock layer**| `MockDataStore` + `MockTypes` (plain C++ structs)                  |
| **Tests**     | `tests/client/test_user_services.cpp`                              |

---

## 3. Pages in detail

### 3.1 Home (`HomePage.qml` → `HomeViewModel`)

Premium dashboard with **12 horizontally-scrollable sections** rendered in two
loading waves:

- **Wave 1 (priority)**: greeting, continue reading, recommended, new releases
- **Wave 2 (deferred 500ms later)**: bestsellers, free, trending, editor's
  picks, discounted, recently viewed, new arrivals, because-you-read

Plus two chip grids: **Browse by genre** and **Featured publishers**. The hero
banner's primary CTA opens the most-recent continue-reading book (or falls
back to the first bestseller). A 10-second refresh timer keeps the sections
live.

### 3.2 Search (`SearchPage.qml` → `SearchViewModel`)

Advanced search with **15+ filter dimensions**:

- Text query (debounced 300ms) + field selector (all/title/author/publisher)
- Genre multi-select (15 genres)
- Price range (min/max sliders)
- Min rating (1–5 stars)
- Toggles: onlyFree, onlyPaid, onlyDiscounted, onlyDownloaded, onlyFavorite
- Availability (all/in-stock/out-of-stock)
- Language + publication year (UI accepts, mock doesn't filter — documented)
- **10 sort modes**: relevance, price↑/↓, rating, newest, oldest, popular, alphabetical
- Active-filter chips with individual + clear-all
- Suggestions dropdown (auto-complete from catalog)
- Recent + popular searches (chip rows, shown when query is empty)
- Collapsible filter panel with animated height
- Skeleton loading grid, empty state, error state with retry

### 3.3 Book detail (`BookDetailPage.qml` → `BookDetailViewModel`)

Two-column layout: scrollable left + sticky right action panel.

- **Hero**: large cover, title, author, publisher, rating + sales badges,
  expandable description, CTAs (buy / add to cart / wishlist)
- **Tabs**: Overview, Reviews, Details, Preview
- **Overview**: reading-progress card (for purchased books), about, details
  grid, related books, same author, same publisher
- **Reviews**: rating summary + distribution, write/edit/delete review form
  with star input, list of reviews with helpful/reply/report actions,
  inline reply input, verified-purchase + author/publisher badges
- **Sticky panel**: price (with discount badge), availability, primary CTAs,
  wishlist + share, reading progress, stats (ratings/sold)

### 3.4 Cart (`CartPage.qml` → `CartViewModel`)

Two-column: items list (left) + sticky order summary (right).

- Empty state with "Discover books" CTA
- Per-item: mini cover, title, author, discount badge, unit/base price, remove
- Summary: subtotal, discount (if any), total, savings note, checkout button
- Loading overlay during checkout
- On success → emits `checkoutSuccessRequested` → router shows toast + navigates to library

### 3.5 Library (`LibraryPage.qml` → `LibraryViewModel`)

3 tabs: **My Books / Downloaded / My Shelves**.

- My Books: grid of purchased books + per-book download toggle
- Downloaded: grid of offline-downloaded books
- My Shelves: create-new-shelf form + list of shelves with rename/delete

### 3.6 PDF reader (`PdfReaderPage.qml` → `ReaderViewModel`)

Full-screen reader overlay with:

- **Collapsible left sidebar** with 3 tabs: TOC, Pages (thumbnails grid),
  Marks (bookmarks list with remove)
- **Top toolbar**: close, title, find-in-book, prev/next page, page indicator,
  zoom out/in, fit-width toggle, bookmark toggle, clean-mode toggle
- **Page surface**: Flickable with synthesized page text (3 paragraphs per
  page, deterministic per book+page)
- **Bottom progress bar**: current/total
- **States**: loading, error, empty
- **Keyboard shortcuts**: ←/→ (prev/next page), Esc (close), Ctrl+F (fit),
  Ctrl+L (clean mode), Ctrl+T (toggle sidebar)
- **Clean mode**: hides all chrome for distraction-free reading; floating
  exit button in bottom-right

### 3.7 Notifications (`NotificationsPage.qml` → `NotificationsViewModel`)

- 9 category tabs (All/Purchase/Review/Discount/Recommendation/Publisher/
  System/Security/Reminder) with per-category count badges
- Search bar
- Per-item right-click context menu: mark read/unread, archive, delete
- Real-time push via `realtimeNotificationReceived` signal
- Empty state ("You're all caught up") + loading skeletons

### 3.8 Profile (`ProfilePage.qml` → `ProfileViewModel`)

- **Identity card**: avatar, display name, @username, favorite-genres badge,
  purchase count, edit display name form
- **Favorite genres**: 1–3 selection from 15-genre catalog, save/reset
- **Change password**: current/new/confirm fields with live strength meter
  (0–4 score) and validation
- **Purchase history**: list of past orders with date, item count, discount,
  total
- **Settings**: theme toggle, sign out

### 3.9 Wishlist (`WishlistPage.qml` → `WishlistViewModel`)

- Grid/list toggle, sort (recent/title/price↑/price↓/rating), search
- **Summary card**: total value, item count, on-sale count, biggest saving
- Bulk-select mode: select all, move to cart, remove selected
- Per-book: add to cart, remove from wishlist
- Empty state with "Discover books" CTA

### 3.10 Settings (`SettingsPage.qml` → `SettingsViewModel`)

8-section sidebar layout:

0. **General** — language, reduce animations
1. **Appearance** — theme (light/dark/auto), accent color, font family, font size
2. **Notifications** — per-event toggles (new books, discounts, sales, reviews, email digest)
3. **Privacy** — share reading, public wishlist, personalized ads
4. **Reading** — reader theme (light/sepia/dark), font size, sync, download
   location (with real folder picker via `Qt.Dialogs.FileDialog`), auto-download
5. **Account** — avatar (with real file picker), change password (routes to
   Profile), sign out
6. **Storage** — cache size, clear cache button, storage usage bar
7. **About** — version, help center, feedback, licenses, check for updates

Save indicator shows "✓ Saved" for 2.5s after a successful save.

### 3.11 Shelves (`ShelvesPage.qml` → `ShelfViewModel`)

Full shelves management (separate from the Library "My Shelves" tab).

- Header: count badge, sort (manual/name/recent/book count), view toggle
- **Create form**: name, description, color picker (8 swatches), private toggle
- **Shelves grid/list**: each card shows colored folder icon, name, description
  or book count, favorite star, private lock, book count chip
- **Context menu**: rename, duplicate, set color (popup), toggle favorite,
  toggle private, move up/down, delete (with confirmation dialog)
- **Detail panel**: book list with per-book remove, add-book button → opens
  book-picker popup (searchable catalog)
- Empty state with "Create your first shelf" CTA

### 3.12 Group reading (`GroupReadingPage.qml` → `StudySessionViewModel`)

- Active rooms grid (2-column) with live/idle badge, reading progress bar,
  page indicator, host, privacy badge
- **Selected room detail**: synchronized reader progress (per-participant
  mini bars), room chat (left/right alignment for others/self), compose row
- **Create room dialog**: name, book, privacy, capacity
- **Invite dialog**: add usernames, send invitations
- **Shared notes popup**: per-page notes with author, add note form

---

## 4. Services in detail

### 4.1 `MockDataStore` — the single source of truth

Central in-memory store shared by every user-role service. Holds:

- **Catalog**: 25 seeded books (`b001`–`b025`) with realistic metadata
  (title, author, publisher, genres, description, price, discount, rating,
  sales, cover colors, createdAt)
- **Reviews**: 16 seeded reviews across 8 books, with replies, helpful
  counts, verified-purchase flags, pinned/flagged state
- **Notifications**: 8 seeded notifications across 8 categories
- **Shelves**: 2 seeded shelves with books
- **Purchase history**: 2 seeded orders
- **Reading progress**: 2 books in progress
- **Recently viewed**: 4 book IDs
- **Current user**: username, display name, favorite genres, wishlist,
  purchased books
- **Downloaded**: offline-downloaded book IDs
- **Recent searches**: deduplicated, capped at 10

Emits 10 distinct `*Changed()` signals so every service can react to
mutations. Thread-safety: mock runs on the QML thread only; the real
socket-backed implementation would marshal responses back via queued
connections.

### 4.2 `BookService` — catalog + reviews + wishlist

- 14 catalog queries (`recommended`, `newReleases`, `bestsellers`, `freeBooks`,
  `popularBooks`, `trending`, `editorsPicks`, `discounted`, `newArrivals`,
  `bySameAuthor`, `bySamePublisher`, `relatedTo`, `becauseYouRead`,
  `continueReading`, `recentlyViewed`)
- `search(query, field, genres, minPrice, maxPrice, minRating)`
- `bookById(id)` → `BookDto*` with `inWishlist` + `purchased` flags set
- Wishlist: `isInWishlist`, `toggleWishlist`
- Reviews: `reviewsForBook`, `submitReview`, `updateReview`, `deleteReview`,
  `markHelpful`, `pinReview`, `flagReview`, `addReply`, `deleteReply`
- Real-backend mapping documented in the header (REQ_BOOK_* / RES_BOOK_*)

### 4.3 `CartService` — shopping cart

- `add(bookId)`, `remove(bookId)`, `clear()`, `checkout()`
- Properties: `itemCount`, `subtotal`, `discountTotal`, `total`,
  `subtotalText`, `discountText`, `totalText`, `savingsText`
- `checkout()` calls `MockDataStore::addPurchase` (which marks books as
  purchased + records the order) + emits `checkoutSucceeded(ids)` and a
  `SaleRegistered` notification

### 4.4 `LibraryService` — purchased + saved + shelves + downloaded

- `purchasedBooks()`, `savedBooks()`, `shelves()`, `downloadedBooks()`
- `toggleSaved`, `toggleDownloaded`
- Full shelf CRUD: `createShelf`, `renameShelf`, `deleteShelf`,
  `duplicateShelf`, `setShelfColor`, `setShelfFavorite`, `setShelfPrivate`,
  `moveShelfUp/Down`, `reorderShelves`, `addToShelf`, `removeFromShelf`,
  `moveBookBetweenShelves`, `copyBookBetweenShelves`, `booksInShelf`,
  `searchShelves`

### 4.5 `NotificationService` — notification center

- `all()`, `byCategory(cat)`, `search(query)`, `countByCategory(cat)`
- `markRead`, `markUnread`, `markAllRead`, `deleteNotification`,
  `archiveNotification`, `unarchiveNotification`
- Real-time push via `notificationReceived(dto)` signal (forwarded from
  `MockDataStore::notificationsChanged` when the newest is unread)

### 4.6 `ReaderService` — PDF reader state

- `openBook(bookId)`, `closeBook()`, `setPage`, `nextPage`, `prevPage`,
  `firstPage`, `lastPage`, `lastReadPage(bookId)`
- Bookmarks: `isBookmarked`, `toggleBookmark`, `clearBookmarks`
- Synthesizes a 4–8 chapter TOC + plausible page count from the book's
  description length
- Persists last-read page per book (in-memory)

### 4.7 `UserService` — profile + password + genres + history

- `username()`, `displayName()`, `initials()`, `favoriteGenresSummary()`
- `updateProfile(displayName)`, `changePassword(current, new, err)`,
  `saveFavoriteGenres(genres)`
- `favoriteGenres()`, `purchaseCount()`, `purchaseHistory()`

---

## 5. DTOs

QObject wrappers exposed to QML via Q_PROPERTY:

| DTO                | Source struct         | Used by |
|--------------------|-----------------------|---------|
| `BookDto`          | `MockBook`            | BookService, HomeVM, SearchVM, BookDetailVM, LibraryVM, WishlistVM |
| `ReviewDto`        | `MockReview`          | BookService, BookDetailVM |
| `ShelfDto`         | `MockShelf`           | LibraryService, LibraryVM, ShelfVM |
| `PurchaseDto`      | `MockPurchase`        | UserService, ProfileVM |
| `NotificationDto`  | `MockNotification`    | NotificationService, NotificationsVM |
| `CartItemDto`      | `MockCartItem`        | CartService, CartVM |
| `RatingDistDto`    | (computed)            | BookDetailVM |
| `FilterChipDto`    | (computed)            | SearchVM |

---

## 6. Tests

`tests/client/test_user_services.cpp` contains a Qt Test suite covering:

- **MockDataStore** (19 tests): catalog seeding, `bookById`, `recommendedFor`,
  `newReleases`/`bestsellers`/`freeBooks` ordering, `search` (query/genre/price),
  wishlist toggle, `markPurchased`, shelf CRUD + idempotent add, notification
  prepend + mark-all-read, downloaded tracking, search-history dedup
- **BookService** (6 tests): `recommended`, `bookById`, `toggleWishlist`,
  `submitReview`, `deleteReview`, `markHelpful` toggle
- **CartService** (4 tests): add/remove/clear, checkout clears cart + marks
  purchased + emits `checkoutSucceeded`
- **LibraryService** (6 tests): purchased/saved reflect store, `toggleSaved`,
  `createShelf`, `addToShelf`, downloaded tracking
- **UserService** (6 tests): `updateProfile`, `changePassword` (rejects too
  short / same-as-current / accepts valid), `saveFavoriteGenres`, purchase
  history reflection

**Build & run:**

```bash
cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_BUILD_TESTS=ON
cmake --build --preset qt-6.11-mingw-64-debug --target test_user_services
./build/qt-6.11-mingw-64-debug/bin/test_user_services
```

Or via CTest:

```bash
ctest --test-dir build/qt-6.11-mingw-64-debug -R test_user_services --output-on-failure
```

---

## 7. File manifest (user-only)

### 7.1 C++ / headers

```
client/include/viewmodels/user/UserViewModelBase.h
client/include/viewmodels/user/HomeViewModel.h
client/include/viewmodels/user/SearchViewModel.h
client/include/viewmodels/user/BookDetailViewModel.h
client/include/viewmodels/user/CartViewModel.h
client/include/viewmodels/user/LibraryViewModel.h
client/include/viewmodels/user/ReaderViewModel.h
client/include/viewmodels/user/NotificationsViewModel.h
client/include/viewmodels/user/ProfileViewModel.h
client/include/viewmodels/user/WishlistViewModel.h
client/include/viewmodels/user/SettingsViewModel.h
client/include/viewmodels/user/ShelfViewModel.h
client/include/viewmodels/user/StudySessionViewModel.h

client/include/services/BookService.h
client/include/services/CartService.h
client/include/services/LibraryService.h
client/include/services/NotificationService.h
client/include/services/ReaderService.h
client/include/services/UserService.h
client/include/services/BookDto.h
client/include/services/CartItemDto.h
client/include/services/LibraryDtos.h
client/include/services/MockDataStore.h
client/include/services/MockTypes.h

client/src/viewmodels/user/UserViewModelBase.cpp
client/src/viewmodels/user/HomeViewModel.cpp
client/src/viewmodels/user/SearchViewModel.cpp
client/src/viewmodels/user/BookDetailViewModel.cpp
client/src/viewmodels/user/CartViewModel.cpp
client/src/viewmodels/user/LibraryViewModel.cpp
client/src/viewmodels/user/ReaderViewModel.cpp
client/src/viewmodels/user/NotificationsViewModel.cpp
client/src/viewmodels/user/ProfileViewModel.cpp
client/src/viewmodels/user/WishlistViewModel.cpp
client/src/viewmodels/user/SettingsViewModel.cpp
client/src/viewmodels/user/ShelfViewModel.cpp
client/src/viewmodels/user/StudySessionViewModel.cpp

client/src/services/BookService.cpp
client/src/services/CartService.cpp
client/src/services/LibraryService.cpp
client/src/services/NotificationService.cpp
client/src/services/ReaderService.cpp
client/src/services/UserService.cpp
client/src/services/BookDto.cpp
client/src/services/LibraryDtos.cpp
client/src/services/MockDataStore.cpp
```

### 7.2 QML pages

```
client/qml/user/UserShell.qml
client/qml/user/HomePage.qml
client/qml/user/SearchPage.qml
client/qml/user/BookDetailPage.qml
client/qml/user/CartPage.qml
client/qml/user/LibraryPage.qml
client/qml/user/PdfReaderPage.qml
client/qml/user/NotificationsPage.qml
client/qml/user/ProfilePage.qml
client/qml/user/WishlistPage.qml
client/qml/user/SettingsPage.qml
client/qml/user/ShelvesPage.qml
client/qml/user/GroupReadingPage.qml
client/qml/user/GroupReadingInviteDialog.qml
```

### 7.3 Tests

```
tests/client/test_user_services.cpp
tests/CMakeLists.txt   ← updated to include test_user_services target
```

### 7.4 Docs

```
docs/USER_MODULE.md
USER_README.md   ← this file
```

---

## 8. How to commit to GitHub

### 8.1 Committing only the user part

Stage every user-related path explicitly:

```bash
git add \
  client/include/viewmodels/user/ \
  client/src/viewmodels/user/ \
  client/include/services/BookService.h \
  client/include/services/CartService.h \
  client/include/services/LibraryService.h \
  client/include/services/NotificationService.h \
  client/include/services/ReaderService.h \
  client/include/services/UserService.h \
  client/include/services/BookDto.h \
  client/include/services/CartItemDto.h \
  client/include/services/LibraryDtos.h \
  client/include/services/MockDataStore.h \
  client/include/services/MockTypes.h \
  client/src/services/BookService.cpp \
  client/src/services/CartService.cpp \
  client/src/services/LibraryService.cpp \
  client/src/services/NotificationService.cpp \
  client/src/services/ReaderService.cpp \
  client/src/services/UserService.cpp \
  client/src/services/BookDto.cpp \
  client/src/services/LibraryDtos.cpp \
  client/src/services/MockDataStore.cpp \
  client/qml/user/ \
  tests/client/test_user_services.cpp \
  tests/CMakeLists.txt \
  docs/USER_MODULE.md \
  USER_README.md

# Verify what's staged
git status

# Commit
git commit -m "feat(user): complete user-role module (MVVM + services + tests)"
```

### 8.2 Suggested commit message

Follow Conventional Commits:

```
feat(user): complete user-role module with 11 pages + 6 services + tests

- MVVM architecture: UserViewModelBase + 13 concrete VMs
- 6 services (Book/Cart/Library/Notification/Reader/User) backed by MockDataStore
- 25-book catalog, 16 reviews, 8 notifications, 2 shelves, 2 purchase orders seeded
- Pages: Home, Search, BookDetail, Cart, Library, PdfReader, Notifications,
  Profile, Wishlist, Settings, Shelves, GroupReading
- Premium UI: split-screen layouts, skeleton loaders, empty/error states,
  animations, dark mode, responsive grids
- PdfReader: TOC/thumbnails/bookmarks sidebar, find-in-book, clean mode,
  keyboard shortcuts
- Shelves: full CRUD + color picker + book picker + drag-reorder
- GroupReading: synchronized reader progress, room chat, shared notes
- Qt Test suite: MockDataStore + BookService + CartService + LibraryService +
  UserService (41+ test cases)

Bugs fixed in this version:
- MockDataStore: 'Non-Felp' genre typo → 'Non-Fiction' (book b017)
- BookDetailViewModel: 'newest'/'oldest' review sort now uses the service's
  newest-first ordering instead of string-comparing relativeTime() labels
- MockDataStore::trending: dead recencyBoost code removed; recency boost
  now actually applies (×1.5 for <30d, ×1.2 for <90d)
- WishlistPage: count badge was reading viewModel.bookCount (non-existent
  property) → fixed to viewModel.count
- ProfileViewModel::availableGenres: was returning the user's currently-
  selected genres (misleading) → now returns empty list with clear comment
  (the page binds to BookService.availableGenres() directly)
```

---

## 9. Integration notes (for the real backend)

When the real server is ready:

1. **Implement the socket protocol** in `ClientNetworkManager` (already
   exists in `src/client/network/`).
2. **Replace each service method's mock body** with a `sendRequest()` call.
   The headers already document the exact REQ_*/RES_* message-type mapping.
3. **Wire the response slots** to update `MockDataStore` (or replace it
   entirely with a server-backed store).
4. **The ViewModels do not need to change** — they only call service methods,
   which can be re-routed through the network transparently.
5. **Thread safety**: marshal every socket reply back to the QML thread via
   `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` before touching
   shared state.

---

## 10. Known limitations

- **Mock-only**: all data is in-memory. A process restart loses cart state,
  reading progress, bookmarks, and shelf edits. The real backend fixes this.
- **`language` and `publicationYear` filters** in `SearchViewModel` are
  accepted by the UI but not applied by the mock (documented in the code).
- **`OtpInput` component** exists but is not wired into any user flow (it's
  ready for future email/OTP verification).
- **"Remember me"** on the login page is cosmetic (no `QSettings` persistence).
- **Rate limiting** is not implemented in any user-facing action.
- **PDF rendering** is synthesized text (no real PDF library bundled). The
  `ReaderService::openBook` synthesizes a page count from the description
  length; `ReaderViewModel::pageText` generates 3 deterministic paragraphs
  per page. Swap for `Poppler` / `QtPDF` when ready.
