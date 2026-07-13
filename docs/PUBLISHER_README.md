# BookCLUB — Publisher Module

Complete documentation for the BookCLUB **publisher-role** subsystem. This
module covers everything a logged-in `publisher` role can do — managing the
catalog (CRUD), viewing sales analytics, creating promotions, handling
publisher-specific notifications, and editing the publisher profile.

> For the login/registration/password-recovery flow, see
> [`AUTH_README.md`](./AUTH_README.md). For the regular-user experience, see
> [`USER_README.md`](./USER_README.md). For admin/server role docs, see the
> corresponding role docs.

---

## 1. Overview

```
                     ┌─────────────────────────────────────────┐
                     │            PublisherShell               │
                     │  (sidebar + topbar + page loader)       │
                     └───────────────────┬─────────────────────┘
                                         │
   ┌──────────┬──────────┬───────────────┼──────────────┬──────────────┐
   ▼          ▼          ▼               ▼              ▼              ▼
Dashboard   Catalog    Sales         Promotions    Notifications    Profile
   │          │          │               │              │              │
   ▼          ▼          ▼               ▼              ▼              ▼
PublisherViewModel (single VM, delegates every call to PublisherService)
   │
   ▼
PublisherService (QML singleton, backed by MockDataStore)
   │
   ▼
MockDataStore (shared with the user role — single source of truth)
```

The publisher role is routed to `PublisherShell.qml` after login. The shell
owns the `PublisherViewModel`, injects `PublisherService` into it, and routes
between 6 publisher pages via a `Loader`. A 5-second refresh timer keeps the
KPIs + recent-orders feed feeling live.

---

## 2. Architecture (MVVM)

Every publisher page follows the same MVVM split as the user role:

```
┌─────────────────────────┐        ┌──────────────────────────┐
│  QML Page (View)        │  data  │  C++ ViewModel           │
│  - PublisherDashboardPage│ ◀────▶│  - PublisherViewModel    │
│  - PublisherCatalogPage │ binds  │                          │
│  - PublisherSalesPage   │        │                          │
│  - PublisherPromotions  │        │                          │
│  - PublisherNotifications│       │                          │
│  - PublisherProfilePage │        │                          │
│  - PublisherBookDetailDrawer│    │                          │
└─────────────────────────┘        └────────────┬─────────────┘
                                                │ calls
                                                ▼
                                ┌──────────────────────────────┐
                                │  PublisherService (singleton)│
                                │  - Catalog CRUD              │
                                │  - Sales analytics           │
                                │  - Promotions                │
                                │  - Notifications             │
                                │  - Profile                   │
                                └────────────┬─────────────────┘
                                             │ delegates to
                                             ▼
                                ┌──────────────────────────────┐
                                │  MockDataStore (shared)      │
                                │  - 25-book catalog           │
                                │  - 16 reviews                │
                                │  - wishlist / purchased      │
                                └──────────────────────────────┘
```

| Layer         | Files                                                              |
|---------------|-------------------------------------------------------------------|
| **View (QML)**| `client/qml/publisher/*.qml`                                       |
| **ViewModel** | `client/include/viewmodels/publisher/PublisherViewModel.h` + `.cpp`|
| **Service**   | `client/include/services/PublisherService.h` + `.cpp`              |
| **Mock layer**| `MockDataStore` + `MockTypes` (shared with user role)              |
| **Common models** | `common/Models/Publisher.{h,cpp}`, `common/Models/PublisherStats.{h,cpp}` |
| **Server handler** | `src/server/handlers/PublisherRequestHandler.{h,cpp}`          |
| **Client controller (legacy)** | `src/client/controllers/PublisherController.{h,cpp}`   |
| **Tests**     | `tests/client/test_publisher_services.cpp`                         |

---

## 3. Pages in detail

### 3.1 Dashboard (`PublisherDashboardPage.qml`)

Overview screen with:

- **4 KPI stat cards**: Revenue (30d) with trend %, Units sold with trend %,
  Active titles / total, Avg. rating
- **Revenue sparkline** (last 14 days) — Canvas-drawn area chart with grid
  lines, peak/avg labels, CSV-copy button
- **Recent activity feed** — synthesized from catalog events (sales, reviews,
  promos, publishes, flags)
- **Top performing titles** — sorted by units sold, with cover, rating, sales
- **Top 5 most-viewed titles** — sorted by ratingCount (proxy for views),
  with synthesized viewCount
- **Recent orders + Top buyers** — two-column row with live order feed and
  loyal customer list
- **Top 5 least-selling titles** — underperforming books highlighted in red
- **Per-book rating distribution** — 1-5 star histogram for the top book

### 3.2 Catalog (`PublisherCatalogPage.qml`)

Catalog management with:

- **Toolbar**: search field, status filter chips (All/Published/Drafts/
  Pending/Removed), "Add new title" button
- **Catalog table**: 7 columns (Title, Status, Price, Units, Rating, Updated,
  Actions) with hover-highlighted rows
- **Row click** opens the book-detail drawer
- **Per-row actions**: edit (opens editor in edit mode), remove/restore
  (soft-delete or re-publish)
- **Book editor popup**: title, author, genre, description, price, discount %,
  cover color, cover accent, cover image picker, PDF file picker
- **Status filter** syncs a local ListModel from the VM's books list, then
  filters by status + search query

### 3.3 Sales (`PublisherSalesPage.qml`)

Sales analytics with:

- **4 KPI cards**: Revenue (30d), Units sold, Avg. order value, Repeat buyer %
- **Monthly revenue bar chart** (12 months) — Canvas-drawn with gradient bars,
  grid lines, month labels, CSV-copy
- **Revenue trend chart** (14 days) — area chart with points, peak/avg, CSV-copy
- **Units by genre** — horizontal bars with counts and percentages
- **Top performing titles** — same as dashboard but full-width
- **Geographic distribution** — revenue by region with bars

### 3.4 Promotions (`PublisherPromotionsPage.qml`)

Promo code management with:

- **3 KPI cards**: total promotions, avg. discount %, total redemptions
- **Create promotion form**: code, description, discount %, usage cap, start
  date, end date
- **Promotions table**: 8 columns (Code, Description, Scope, Discount,
  Uses/Cap, Period, Status, Actions)
- **Date-aware status**: scheduled (start in future), active, expired (end in
  past)
- **Per-row delete** action

### 3.5 Notifications (`PublisherNotificationsPage.qml`)

Publisher-specific notifications (sales milestones, review alerts, platform
announcements, promo performance) with:

- **Header**: unread count, total count, "Mark all as read" + "Clear read"
  buttons
- **Notification list**: icon, title, time, body (rich text), per-item
  mark-as-read toggle
- **8 seeded notifications** across success/info/warning tones

### 3.6 Profile (`PublisherProfilePage.qml`)

Publisher account & profile management with:

- **Header card**: avatar, publisher name, verified badge, publisher ID, joined
  date, plan badge, "Edit profile" button
- **4 KPI cards**: Total books, Total revenue, Units sold, Avg. rating
- **Two-column body**:
  - Left: Account info (editable publisher name, biography, website, email,
    tax ID) + "Save changes" button
  - Right: Catalog composition (published/draft/pending/removed counts with
    bars) + Contact card (email, website, country)
- **Edit-profile dialog**: popup version of the inline editor

### 3.7 Book detail drawer (`PublisherBookDetailDrawer.qml`)

Slide-in drawer showing full book details:

- **Header**: "Book details" title + close button
- **Cover + title + author + publisher + status badge**
- **4 mini stat cards**: Price, Sales, Rating, Reviews
- **Genres** chip row
- **Description** card
- **Reviews** list (rating stars, author, comment, helpful count, verified
  badge) with empty state
- **Footer action bar**: "Edit metadata" (opens editor) + "Remove from
  storefront" / "Re-publish" (status-aware)
- **Slide animations**: slide-in from right, slide-out + hide timer

---

## 4. Service in detail

### 4.1 `PublisherService` — the publisher's window into the catalog

The service is a QML singleton that wraps `MockDataStore` and exposes
publisher-scoped operations. It maintains its own state for promotions and
notifications (not in the shared store).

**Catalog management** (delegates to MockDataStore):
- `publisherBooks()` → `QList<QObject*>` of `BookDto*`
- `addBook(title, author, genre, description, price, discount%, coverColor,
  coverAccent, coverImage?, pdfFilePath?)` → bookId
- `updateBook(bookId, ...)` — partial edit (empty strings skipped)
- `removeBook(bookId)` — soft-delete (sets status="removed")
- `setBookStatus(bookId, status)` — status ∈ {published, draft, pending,
  removed}
- `bookDetail(bookId)` → `QVariantMap` with all fields + `reviews` list

**Sales analytics**:
- `totalRevenue()`, `totalUnitsSold()`, `activeTitleCount()`,
  `averageRating()`, `totalBooks()`
- `topSellingBooks(count)`, `topViewedBooks(count)`,
  `topBooks()`, `topViewedBooksVariant(count)`, `leastSellingBooks(count)`
- `revenueSeries(days=14)`, `monthlyRevenue(months=12)`,
  `revenueTrend()` (vs last week %)
- `genreBreakdown()`, `geographicBreakdown()`, `activityFeed(count=8)`
- `recentOrders(count=10)`, `topBuyers(count=5)`,
  `repeatBuyerRate()`, `unitsSoldTrend()`
- `ratingDistribution(bookId)` — 5-element 1-5 star histogram

**Promotions** (in-memory, seeded with 6 promos):
- `promotions()` → `QVariantList` with date-aware status
- `addPromotion(code, description, discount%, cap, startDate, endDate)`
- `removePromotion(code)` — case-insensitive

**Notifications** (in-memory, seeded with 8 notifications):
- `publisherNotifications()`
- `markAllNotificationsRead()`, `markNotificationRead(id, read)`,
  `clearReadNotifications()`

**Profile** (in-memory, seeded):
- `publisherProfile()` → `QVariantMap` with editable fields + live catalog
  stats
- `updatePublisherProfile(publisherName, biography, website, email, taxId)`

**Real-backend mapping** (documented in the header):
- `publisherBooks()` → `GetPublisherBooks`
- `addBook(...)` → `PublishBook`
- `updateBook(...)` → `UpdateBook`
- `removeBook(bookId)` → `DeactivateBook`
- `setBookStatus(bookId, "published")` → `ActivateBook`
- `applyTimedDiscount` → `ApplyTimedDiscount`
- analytics → `GetPublisherAnalytics`

---

## 5. Tests

`tests/client/test_publisher_services.cpp` contains a Qt Test suite covering
**30+ test cases** across the PublisherService:

- **Catalog management (6)**: addBook increases count, addBookWithCoverImage
  persists image path (regression test for the 9-arg vs 11-arg bug),
  updateBook changes title, updateBookWithCoverImage doesn't corrupt colors
  (regression test), removeBook soft-deletes, setBookStatus toggles
- **Analytics (8)**: totalRevenue, totalUnitsSold, activeTitleCount excludes
  removed, averageRating range, topBooks sorted desc, leastSellingBooks sorted
  asc, topViewedBooks sorted by ratingCount desc
- **Promotions (4)**: addPromotion uppercases code, removePromotion
  case-insensitive (regression test), removePromotion nonexistent returns
  false, promotions computes date-aware status
- **Notifications (3)**: markAllRead, markSingleRead, clearRead keeps unread
- **Profile (3)**: publisherProfile returns seeded data,
  updatePublisherProfile persists, updatePublisherProfile emits profileChanged
- **Rating distribution (2)**: returns 5 entries, unknown book returns zeros
- **Extended analytics (6)**: monthlyRevenue count, recentOrders count,
  topBuyers count, revenueTrend format, genreBreakdown sums, geographicBreakdown
  sums to 1.0

**Build & run:**

```bash
cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_BUILD_TESTS=ON
cmake --build --preset qt-6.11-mingw-64-debug --target test_publisher_services
./build/qt-6.11-mingw-64-debug/bin/test_publisher_services
```

Or via CTest:

```bash
ctest --test-dir build/qt-6.11-mingw-64-debug -R test_publisher_services --output-on-failure
```

---

## 6. File manifest (publisher-only)

### 6.1 C++ / headers

```
client/include/viewmodels/publisher/PublisherViewModel.h
client/src/viewmodels/publisher/PublisherViewModel.cpp

client/include/services/PublisherService.h
client/src/services/PublisherService.cpp

common/Models/Publisher.h
common/Models/Publisher.cpp
common/Models/PublisherStats.h
common/Models/PublisherStats.cpp

src/server/handlers/PublisherRequestHandler.h
src/server/handlers/PublisherRequestHandler.cpp

src/client/controllers/PublisherController.h
src/client/controllers/PublisherController.cpp
```

### 6.2 QML pages

```
client/qml/publisher/PublisherShell.qml
client/qml/publisher/PublisherDashboardPage.qml
client/qml/publisher/PublisherCatalogPage.qml
client/qml/publisher/PublisherSalesPage.qml
client/qml/publisher/PublisherPromotionsPage.qml
client/qml/publisher/PublisherNotificationsPage.qml
client/qml/publisher/PublisherProfilePage.qml
client/qml/publisher/PublisherBookDetailDrawer.qml
```

### 6.3 Tests

```
tests/client/test_publisher_services.cpp
tests/CMakeLists.txt   ← updated to include test_publisher_services target
```

### 6.4 Docs

```
docs/UML/publisher-flow.puml
PUBLISHER_README.md   ← this file
```

---

## 7. How to commit to GitHub

### 7.1 Committing only the publisher part

Stage every publisher-related path explicitly:

```bash
git add \
  client/include/viewmodels/publisher/ \
  client/src/viewmodels/publisher/ \
  client/include/services/PublisherService.h \
  client/src/services/PublisherService.cpp \
  client/qml/publisher/ \
  common/Models/Publisher.h \
  common/Models/Publisher.cpp \
  common/Models/PublisherStats.h \
  common/Models/PublisherStats.cpp \
  src/server/handlers/PublisherRequestHandler.h \
  src/server/handlers/PublisherRequestHandler.cpp \
  src/client/controllers/PublisherController.h \
  src/client/controllers/PublisherController.cpp \
  tests/client/test_publisher_services.cpp \
  tests/CMakeLists.txt \
  docs/UML/publisher-flow.puml \
  PUBLISHER_README.md

# Verify what's staged
git status

# Commit
git commit -m "feat(publisher): complete publisher-role module (MVVM + service + tests)"
```

### 7.2 Suggested commit message

Follow Conventional Commits:

```
feat(publisher): complete publisher-role module with 6 pages + service + tests

- MVVM architecture: PublisherViewModel (single VM for all 6 pages)
- PublisherService (QML singleton) backed by MockDataStore
- 6 pages: Dashboard, Catalog, Sales, Promotions, Notifications, Profile
- Book detail drawer with slide-in animation
- Premium UI: KPI stat cards, Canvas charts (sparkline, bar, area),
  skeleton loaders, empty states, dark mode
- Catalog CRUD with soft-delete (status="removed") + re-publish
- Promotions with date-aware status (scheduled/active/expired)
- Publisher profile with editable fields + live catalog stats
- Qt Test suite: 30+ cases covering catalog, analytics, promotions,
  notifications, profile, rating distribution

Bugs fixed in this version:
- PublisherRequestHandler::handlePublishBook: use-after-free (LOG_INFO
  accessed book->title() after delete book) — now captures title first
- PublisherService::addBook/updateBook: coverImage and pdfFilePath were
  passed in the wrong parameter slots (9-arg call vs 11-arg signature) —
  file paths were silently lost and corrupted the cover colors
- PublisherController::createBook: basePrice validation used toString()
  on a numeric JSON value, which always returned empty — now uses isDouble()
- PublisherController: mutation response handlers emitted bookListChanged()
  without refreshing m_booksData → stale cache; now calls loadMyBooks()
- Publisher model setters: setBiography/setWebsite/setTaxId didn't emit
  profileChanged() (only setPublisherName did) — now all four emit both
- PublisherRequestHandler::handleApplyTimedDiscount: startDate/endDate
  were extracted but never passed to Discount — now calls setStartsAt/setEndsAt
- PublisherStats::setBookStats: emitted statsChanged() twice (once inside
  recalculate(), once after) — now only emits once
- PublisherService::removePromotion: case-sensitive comparison missed
  lowercase input — now uppercases the input before comparing
- PublisherBookDetailDrawer: closed() signal was declared but never emitted
  — now emitted from the hide timer
- PublisherBookDetailDrawer: _hideTimer.interval was hardcoded to 260ms,
  could drift from _slideOut.duration — now uses Theme.motion.durationBase
- PublisherProfilePage: Item { ... } was declared inside onAboutToShow JS
  handler (invalid QML, silently dropped) — removed
- PublisherProfilePage: missing onBooksChanged handler (catalog-composition
  bars went stale after edits) — added
- PublisherProfilePage: missing bottom footer spacer — added
- PublisherShell: Settings nav item duplicated Profile (no settings route)
  — removed
```

---

## 8. Integration notes (for the real backend)

When the real server is ready:

1. **The server-side `PublisherRequestHandler` already exists** and dispatches
   7 commands: `GetPublisherBooks`, `PublishBook`, `UpdateBook`,
   `DeactivateBook`, `ActivateBook`, `ApplyTimedDiscount`,
   `GetPublisherAnalytics`.
2. **The client-side `PublisherController` already exists** (legacy networked
   path) and wires those 7 commands to `ClientNetworkManager`.
3. **The mocked `PublisherService` is the migration target** — replace each
   method's mock body with a `sendRequest()` call. The header documents the
   exact message-type mapping.
4. **The `PublisherViewModel` does not need to change** — it only calls
   `PublisherService` methods.
5. **Thread safety**: marshal every socket reply back to the QML thread via
   `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.

---

## 9. Known limitations

- **Mock-only**: promotions, notifications, and profile are in-memory. A
  process restart loses them. The catalog itself is shared with the user role
  via `MockDataStore`.
- **No real PDF rendering**: the publisher can upload a PDF path, but the
  reader uses synthesized text (see `USER_README.md` §10).
- **`revenueSeries` is synthesized**: it doesn't read from the store. The
  trend % is computed from the synthesized series, not real sales data.
- **`topViewedBooks` uses `ratingCount` as a proxy for views** — the mock
  doesn't track per-book view events.
- **`averageRating` is unweighted**: it's the mean of per-book averages, not
  a rating-count-weighted average. A book with 1 rating counts as much as one
  with 10,000.
- **`repeatBuyerRate` is synthesized**: 55-70% range derived from total sales.
  A real backend would compute this from the orders table.
- **No rate limiting** on catalog mutations.
- **Promotion codes are case-insensitive on removal** but displayed
  uppercased — the QML passes `_code.text.toUpperCase()` on create, and the
  service uppercases on both add and remove.
