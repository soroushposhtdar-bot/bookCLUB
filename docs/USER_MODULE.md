# BookClub — User Module (Regular User role)

This document describes the **User-role dashboard** layer built on top of the
existing Authentication module. It implements every screen, ViewModel and
service required by the project spec for the **Regular User** role, in the
same modern minimal black-and-white design language as the auth pages.

The implementation is **socket-ready**: every method on every service is
annotated with the matching `REQ_*` / `RES_*` message type from
`common/Network/Protocol.h`. The current build ships a fully-functional
**mock** backed by `MockDataStore`; swapping in the real socket layer only
requires rewriting the service `.cpp` files — the ViewModels and QML stay
unchanged.

---

## 1. What's new

### C++ (compiled)

| Layer | Files |
|-------|-------|
| **Mock data layer** | `services/MockTypes.h`, `services/MockDataStore.h/.cpp` |
| **Services** (singletons, socket-ready) | `BookService`, `CartService`, `LibraryService`, `NotificationService`, `ReaderService`, `UserService` (each `.h/.cpp`) |
| **DTOs** (QObject wrappers exposed to QML) | `BookDto`, `ReviewDto` (in `BookDto.h`); `CartItemDto`; `ShelfDto`, `PurchaseDto`, `NotificationDto` (in `LibraryDtos.h`) |
| **User ViewModels** | `UserViewModelBase` + 8 concrete VMs: `HomeViewModel`, `SearchViewModel`, `BookDetailViewModel`, `CartViewModel`, `LibraryViewModel`, `ReaderViewModel`, `NotificationsViewModel`, `ProfileViewModel` |

### QML

| Layer | Files |
|-------|-------|
| **Theme** | `theme/Theme.qml` — extended with dark mode + dashboard tokens |
| **Components** | `components/book/{BookCover,BookCard,BookCarousel,RatingStars,StarInput,GenreChip}.qml`<br>`components/navigation/{NavItem,Sidebar,TopBar,TabBar,Avatar}.qml`<br>`components/data/{EmptyState,SectionHeader,NotificationItem}.qml` |
| **Layouts** | `layouts/DashboardLayout.qml` (sidebar + topbar + content slot) |
| **Pages** | `user/{UserShell,HomePage,SearchPage,BookDetailPage,CartPage,LibraryPage,PdfReaderPage,NotificationsPage,ProfilePage}.qml` |
| **App wiring** | `App.qml` — two-phase router (auth → dashboard); `main.cpp` — registers all new types |

### Build config
- `src/client/CMakeLists.txt` — adds all new C++ sources
- `client/resources/qml.qrc` — registers all new QML files

---

## 2. Architecture

```
                         ┌─────────────────────────────────────────────┐
                         │                  QML Views                   │
                         │  (HomePage, BookDetailPage, CartPage, ...)   │
                         └──────────────────┬──────────────────────────┘
                                            │ Q_PROPERTY / Q_INVOKABLE
                                            ▼
                         ┌─────────────────────────────────────────────┐
                         │                ViewModels                    │
                         │  (HomeVM, BookDetailVM, CartVM, ...)         │
                         │  inherits UserViewModelBase (async state)    │
                         └──────────────────┬──────────────────────────┘
                                            │ calls Q_INVOKABLE methods
                                            ▼
                         ┌─────────────────────────────────────────────┐
                         │                 Services                     │
                         │  (BookService, CartService, ...) — singletons│
                         │  Each method annotated with the matching     │
                         │  REQ_*/RES_* socket message type.            │
                         └──────────────────┬──────────────────────────┘
                                            │ today: in-process mock
                                            │ future: ClientNetworkManager
                                            ▼
                         ┌─────────────────────────────────────────────┐
                         │              MockDataStore                   │
                         │  (shared in-memory catalog + user state)     │
                         │  — OR —                                      │
                         │              ClientNetworkManager            │
                         │  (TCP socket → BookClubServer)               │
                         └─────────────────────────────────────────────┘
```

### Why two layers (Service + ViewModel)?

- **Services** mirror the wire protocol 1-to-1. Each method = one socket
  request. Easy to audit, easy to swap mock → real.
- **ViewModels** own UI state (loading flags, form fields, draft state) and
  orchestrate one or more service calls. They shield the QML from the wire
  protocol so swapping the mock for a real backend never touches the UI.

### MVVM conventions used

- Every ViewModel inherits `UserViewModelBase` (or `AuthViewModelBase` for
  the auth flow), which provides the `isBusy` / `error` / `beginAsync` /
  `finishAsync` state machine.
- List data is exposed as `QList<QObject*>` of DTO instances. QML iterates
  these directly via `Repeater { model: vm.someList }` and accesses fields
  through Q_PROPERTY.
- Every state mutation emits a `*Changed` signal — no manual refresh needed.
- Form-field validation lives in the ViewModel (`canSubmit` getters), not
  in QML.

---

## 3. Service ↔ socket mapping

Every service method is annotated with the matching `Protocol.h` message
type. The table below summarizes the mapping; full per-method comments live
in each service header.

| Service | Method | Socket message |
|---------|--------|----------------|
| `BookService` | `recommended()` | `REQ_BOOK_RECOMMENDED` → `RES_BOOK_LIST` |
|                 | `newReleases()` | `REQ_BOOK_NEW` → `RES_BOOK_LIST` |
|                 | `bestsellers()` | `REQ_BOOK_BESTSELLER` → `RES_BOOK_LIST` |
|                 | `freeBooks()` | `REQ_BOOK_FREE` → `RES_BOOK_LIST` |
|                 | `popularBooks()` | `REQ_BOOK_POPULAR` → `RES_BOOK_LIST` |
|                 | `search(...)` | `REQ_BOOK_SEARCH` → `RES_BOOK_LIST` |
|                 | `bookById(id)` | `REQ_BOOK_DETAIL` → `RES_BOOK_DETAIL` |
|                 | `toggleWishlist(id)` | `REQ_WISHLIST_TOGGLE` → `RES_WISHLIST` |
|                 | `submitReview(...)` | `REQ_REVIEW_SUBMIT` → `RES_REVIEW` |
|                 | `updateReview(...)` | `REQ_REVIEW_UPDATE` → `RES_REVIEW` |
|                 | `deleteReview(...)` | `REQ_REVIEW_DELETE` → `RES_OK` |
| `CartService` | `items()` | `REQ_CART_GET` → `RES_CART` |
|                | `add(id)` | `REQ_CART_ADD` → `RES_CART` |
|                | `remove(id)` | `REQ_CART_REMOVE` → `RES_CART` |
|                | `clear()` | `REQ_CART_CLEAR` → `RES_CART` |
|                | `checkout()` | `REQ_CART_CHECKOUT` → `RES_ORDER` |
| `LibraryService` | `purchasedBooks()` | `REQ_LIB_PURCHASED` → `RES_BOOK_LIST` |
|                   | `savedBooks()` | `REQ_LIB_SAVED` → `RES_BOOK_LIST` |
|                   | `shelves()` | `REQ_SHELF_LIST` → `RES_SHELF_LIST` |
|                   | `createShelf(...)` | `REQ_SHELF_CREATE` → `RES_SHELF` |
|                   | `renameShelf(...)` | `REQ_SHELF_RENAME` → `RES_SHELF` |
|                   | `deleteShelf(id)` | `REQ_SHELF_DELETE` → `RES_OK` |
|                   | `addToShelf(...)` / `removeFromShelf(...)` | `REQ_SHELF_ADD` / `REQ_SHELF_REMOVE` |
| `NotificationService` | `all()` | `REQ_NOTIF_LIST` → `RES_NOTIF_LIST` |
|                       | `markRead(id)` / `markAllRead()` | `REQ_NOTIF_MARK_READ` / `REQ_NOTIF_MARK_ALL` |
|                       | (real-time push) | `EVT_NOTIFICATION` (server-pushed) |
| `ReaderService` | `openBook(id)` | `REQ_READER_OPEN` → `RES_READER_OPEN` |
|                  | `setPage(p)` / `nextPage()` / ... | `REQ_READER_SAVE_PAGE` → `RES_OK` |
|                  | (group sync) | `SyncEventType::TurnPage / JumpToPage / ZoomChanged / LastPageSaved` (broadcast) |
| `UserService` | `updateProfile(...)` | `REQ_USER_UPDATE` → `RES_USER_UPDATE` |
|                | `changePassword(...)` | `REQ_USER_CHANGE_PASSWORD` → `RES_USER_CHANGE_PASSWORD` |
|                | `saveFavoriteGenres(...)` | `REQ_USER_GENRES` → `RES_USER_GENRES` |
|                | `purchaseHistory()` | `REQ_USER_PURCHASES` → `RES_USER_PURCHASES` |

### How to migrate to the real backend

For each service `.cpp`:

1. Replace the body of each method with a
   `ClientNetworkManager::sendRequest(msgType, payload)` call.
2. Connect the network manager's `replyReceived` signal to a slot that
   parses the response, populates the DTOs, and emits the service's existing
   `*Changed` signal.
3. The ViewModels and QML **do not need to change**.

---

## 4. Page inventory

| Route | Page | ViewModel | Purpose |
|-------|------|-----------|---------|
| `home` | `HomePage.qml` | `HomeViewModel` | Greeting hero + Recommended / New / Bestseller / Free carousels + genre grid |
| `search` | `SearchPage.qml` | `SearchViewModel` | Search field + collapsible filter panel (field, genres, price, rating) + sort + results grid |
| `bookDetail` | `BookDetailPage.qml` | `BookDetailViewModel` | Cover + metadata + CTAs (buy / cart / wishlist / read) + reviews list + add-your-review form |
| `cart` | `CartPage.qml` | `CartViewModel` | Cart items list + sticky order summary (subtotal / discount / total) + checkout |
| `library` | `LibraryPage.qml` | `LibraryViewModel` | 3 tabs: My Books / Saved / My Shelves (CRUD + book chips) |
| `reader` | `PdfReaderPage.qml` | `ReaderViewModel` | Full-screen PDF reader overlay (prev/next, zoom, fit-width, clean mode, keyboard shortcuts) |
| `notifications` | `NotificationsPage.qml` | `NotificationsViewModel` | Notification list + mark read/unread + mark-all-read + real-time toast |
| `profile` | `ProfilePage.qml` | `ProfileViewModel` | Identity card + edit display name + favorite genres + change password + purchase history + settings (theme + sign out) |

The post-login **onboarding** flow reuses the existing `GenreSelectionPage`
from the auth module — it's already wired in `App.qml` to push to the
dashboard once the user picks 1–3 genres.

---

## 5. Design system

`Theme.qml` was extended with:

- **Dark mode** — `Theme.mode = "dark"` flips every component. The `c(key)`
  helper resolves colors for the active mode; legacy `Theme.color.xxx`
  shortcuts remain backward-compatible.
- **Dashboard tokens** — `sidebarWidth` (248), `sidebarCollapsedWidth` (72),
  `topbarHeight` (64), `contentMaxWidth` (1280), `bookCardWidth` (188),
  `bookCoverRatio` (1.5), `avatarSize` (40), `navItemHeight` (44), plus
  sidebar/topbar background tokens.
- **Icon expansion** — `AppIcon.qml` lookup table extended with ~80 new
  Material Symbols glyphs (home, library, cart, notifications, bookmarks,
  shelves, reader controls, etc.).

### Responsive behaviour

- **≥ 1100px** — full sidebar (248px), topbar with inline search field.
- **760–1100px** — full sidebar, search still inline.
- **< 760px** — sidebar collapses to icon-only rail (72px); the `DashboardLayout`
  animates the width transition.

---

## 6. Build & run

### Prerequisites (unchanged from the auth module)

- Qt 5.15+ with `Core`, `Gui`, `Network`, `Sql`, `Qml`, `Quick`,
  `QuickControls2`, `QtGraphicalEffects`
- CMake 3.16+
- C++17 compiler

### Build

```bash
cd bookCLUB
mkdir build && cd build
cmake ..
cmake --build . -j8
```

### Run

```bash
./bin/BookClubClient
```

### Demo accounts

| Username | Password | Notes |
|----------|----------|-------|
| `alice` | `password123` | Pre-seeded with 3 favorite genres + 2 purchases |
| `bob`   | `password123` | Fresh account — genre selection onboarding will show |

Log in as `alice` to land directly on the dashboard. Log in as a freshly
registered account (or `bob`) to see the genre-selection onboarding first.

---

## 7. What's intentionally out of scope

These features are documented in the project spec but live outside the
**Regular User** role and were therefore not implemented in this pass:

- **Publisher panel** — add/edit/deactivate books, discounts, stats dashboard.
- **Admin panel** — user management, content moderation, account blocking.
- **Server dashboard GUI** — online users, request log, server health.
- **Real socket backend** — the mock covers the full UX; wiring up
  `ClientNetworkManager` is a separate task (see §3).

These are tracked as future work and can be layered on top of the existing
service/ViewModel split without touching the User-role UI.
