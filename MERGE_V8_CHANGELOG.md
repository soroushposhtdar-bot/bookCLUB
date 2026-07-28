# BookClub — v8 Merge Changelog

This revision merges the **v4 server** (from `v4-pfp-fixed.zip`) into the
**v7 client** (from `bookclub_build_ready.zip`) and removes every
mock-data fallback from the publisher module so all pages now run on
the real SQLite database.

## Source Projects

| Project | Role | Key features |
|---------|------|--------------|
| `bookclub_build_ready.zip` (v7) | **Client base** | Polished QML UI, DejaVu Sans font, English locale, qmldir files, fixed dark mode, password change dialog, ComboBox genre picker, live cover preview |
| `v4-pfp-fixed.zip` (v4) | **Server base** | Real `DiscountCodes` table, real admin review moderation (`AdminDeleteReview` / `AdminApproveReview`), real cart-level discount lookup |

## What Changed

### 1. Server-side files replaced with v4 versions

| File | Change |
|------|--------|
| `common/Constants.h` | Was a `// TODO` placeholder (empty file). Now contains the full constants block from v4 (server ports, DB path, auth, books, cart, discounts, pagination, file upload, logging, app version). |
| `common/Network/Protocol.h` | Added v4's `AdminDeleteReview` (86) and `AdminApproveReview` (87) commands. |
| `common/Network/Protocol.cpp` | Added matching `commandToString` cases. |
| `database/schema.sql` | Added v4's `DiscountCodes` table. Added a new `description` column for promo-code text. |
| `src/server/DatabaseManager.cpp` | Schema version bumped 1 → 3 (v2 = added DiscountCodes, v3 = added description column). Old DBs are auto-deleted and recreated from scratch. |
| `src/server/ServerCore.cpp` | Wires the 4 new discount-code commands to the publisher handler. Wires `AdminDeleteReview`/`AdminApproveReview` to the admin handler. |
| `src/server/handlers/AdminRequestHandler.h/.cpp` | Replaced with v4's version (real review moderation — `handleAdminDeleteReview`, `handleAdminApproveReview`, `m_reviewRepo` member). |
| `src/server/handlers/CartRequestHandler.cpp` | Replaced with v4's version (real `DiscountCodes` lookup in `handleApplyDiscount` — no more TODO returning cart unchanged). |

### 2. New server endpoints for publisher-managed promo codes

Added **4 new commands** to the publisher module so the Promotions page
runs on the real `DiscountCodes` table:

| Command | Wire name | Handler | Effect |
|---------|-----------|---------|--------|
| `GetDiscountCodes` | `GetDiscountCodes` | `PublisherRequestHandler::handleGetDiscountCodes` | Lists the caller's promo codes (admins see all). |
| `CreateDiscountCode` | `CreateDiscountCode` | `PublisherRequestHandler::handleCreateDiscountCode` | Inserts a new row into `DiscountCodes`. Rejects duplicates with `409 Conflict`. |
| `UpdateDiscountCode` | `UpdateDiscountCode` | `PublisherRequestHandler::handleUpdateDiscountCode` | Updates a code's value/type/dates/description. Owner-locked (admin override). |
| `DeleteDiscountCode` | `DeleteDiscountCode` | `PublisherRequestHandler::handleDeleteDiscountCode` | Soft-deletes (`isActive = 0`) so historical orders still resolve. Owner-locked. |

### 3. Client `AdminService.cpp` replaced with v4 version

The v7 client had stubbed-out `deleteReview()` and `approveReview()`
methods (TODOs that returned `false`). Replaced with v4's real
implementations that fire `AdminDeleteReview` and `AdminApproveReview`
on the server and emit `dataChanged` / `reviewsChanged` on success.

### 4. `PublisherService.cpp/.h` rewritten to remove mock data

Every method now hits the real server. No local fallback. Specifically
removed:

| Removed mock | Now |
|--------------|-----|
| `_hashSeed` / `_rand01` deterministic PRNG | Deleted entirely. |
| `m_promotionsCache` local cache | Deleted — `promotions()` queries `GetDiscountCodes`. |
| `addBook()` "local-<timestamp>" ID fallback on server failure | Returns empty string — QML shows an error toast. |
| `addPromotion()` "add to cache either way" | Calls real `CreateDiscountCode` — fails if server rejects (e.g. duplicate code). |
| `topBooks()` synthetic rating fallback (3.5–5.0 hash) | Uses real `averageRating` from analytics (0 if 0). |
| `leastSellingBooks()` synthetic rating fallback (3.0–4.8 hash) | Uses real `averageRating`. |
| `revenueTrend()` synthetic concentration-derived trend | Returns `"+0.0%"` (no real trend data). |
| `unitsSoldTrend()` synthetic concentration-derived trend | Returns `"+0.0%"`. |
| `repeatBuyerRate()` synthetic 18+N×3% | Returns `0`. |
| `salesSeries(days)` synthetic deterministic daily spread | Returns real per-book sales as `{label: bookTitle, value: salesCount}`. |
| `revenueSeries(days)` synthetic deterministic daily spread | Returns real per-book revenue as `{label: bookTitle, value: revenue}`. |
| `monthlyRevenue(months)` synthetic 12-point growth curve | Returns single aggregate point `{label: "Total", value: totalRevenue}`. |
| `recentOrders()` fabricated Persian customer names + status + time | Returns real per-book sales rows. `customer` set to `"—"` so the QML delegate renders cleanly. |
| `topBuyers()` fabricated Persian names + initials + books + lastOrder | Returns empty list — QML shows the section header with no rows. |
| `geographicBreakdown()` fabricated 8-region fixed-weight breakdown | Returns empty list — QML shows the EmptyState. |
| `publisherProfile()` fabricated `verified`, `plan="Publisher Pro"`, `joinedAt="2024-01-15"`, `country="Iran"` | Returns real values from `AuthService` + local cache. Empty fields render as `"—"` via the QML's existing fallback. |

### 5. QML error handling for real server responses

Because the client no longer silently swallows server failures, the
QML now checks return values and shows error toasts:

| File | Change |
|------|--------|
| `client/qml/publisher/PublisherCatalogPage.qml` | `addBook` and `updateBook` now check the return value. On failure, shows `"Publish failed"` / `"Save failed"` error toast instead of always saying `"Title published"`. |
| `client/qml/publisher/PublisherPromotionsPage.qml` | `addPromotion` and `updatePromotion` now check the bool return. On failure (e.g. duplicate code), shows `"Create failed"` / `"Update failed"` error toast with hint about duplicate codes or invalid dates. |

### 6. Seed data for DiscountCodes

`database/seeds/sample_data.sql` now seeds 4 promo codes for the demo
publisher (`publisher1`):

| Code | Type | Value | Status |
|------|------|-------|--------|
| `WELCOME10` | Percentage | 10% | Active |
| `SUMMER20` | Percentage | 20% (min $50) | Active |
| `VIP50` | Fixed | $25 (min $100) | Active |
| `FLASH15` | Percentage | 15% | Expired (inactive) |

So the Promotions page renders with real rows the first time the demo
publisher logs in.

## Build & Run

### Prerequisites
- **Qt 6.11+** (with `llvm-mingw_64` on Windows, or `gcc_64` on Linux)
- **CMake 3.16+**
- **Qt Creator** (recommended) or command-line build

### Build Steps (Qt Creator)
1. Open `CMakeLists.txt` in Qt Creator.
2. Configure the project with your Qt 6 kit.
3. **Build → Build All** (or Ctrl+B).
4. Run `BookClubServer` first (the server must be running for the
   client to function — there is no longer any offline mock fallback).
5. Run `BookClubClient`.

### Build Steps (Command Line)
```bash
cd bookclub
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt/6.11.1/llvm-mingw_64
cmake --build . -j8
```

### Run
```bash
# Start the server first (creates bookclub.db with schema v3 + seeds)
./build/bin/BookClubServer -p 8080

# Then start the client
./build/bin/BookClubClient
```

### Demo Accounts
| Username    | Password     | Role     |
|-------------|--------------|----------|
| `admin`     | `admin`      | Admin    |
| `publisher1`| `publisher1` | Publisher|
| `amir`      | `amir1234`   | User     |

## What to expect at runtime

Because all mock fallback is gone:

- **Catalog page**: shows the books the publisher actually has in the
  database (4 books seeded for `publisher1`). Adding a book that
  violates validation (empty title, >200 chars, etc.) shows an error
  toast.
- **Dashboard**: KPI cards show real numbers (revenue, units sold,
  active titles, average rating). Recent-orders feed shows per-book
  sales rows. Top-buyers card shows the section header with no rows
  (server has no top-buyers endpoint).
- **Sales page**: Sales/revenue charts render as one bar per book (the
  real per-book data). Geo breakdown card shows the EmptyState. Units
  by Genre card shows real genre aggregation (uses books' `genreIds` +
  analytics `salesCount`).
- **Promotions page**: lists the 4 seeded promo codes. Creating a new
  code with a duplicate name returns `409 Conflict` and shows an error
  toast. The Reset button now properly clears all fields.
- **Notifications page**: lists real notifications from `GetNotifications`.
- **Profile page**: shows the publisher's real display name + email
  from `AuthService`. The "verified" badge is `false` and `joinedAt`
  is empty (no longer fabricated) — the QML's existing fallback shows
  `"—"` for empty values.
- **Admin → Reviews**: admin can now actually delete and approve any
  review via the real `AdminDeleteReview` / `AdminApproveReview`
  endpoints.

## Schema Migration

The schema version is bumped to **3**. If an existing `bookclub.db`
file from a previous version is present, the server will:

1. Detect the version mismatch (1, 2, or 0).
2. Close, delete, and recreate the database file from `schema.sql` +
   `sample_data.sql`.

So all data is reset on first run after the upgrade. This is by design
for a course project — no in-place ALTER TABLE migrations.

## Files Modified (summary)

**Server side (8 files):**
- `common/Constants.h`
- `common/Network/Protocol.h`
- `common/Network/Protocol.cpp`
- `database/schema.sql`
- `database/seeds/sample_data.sql`
- `src/server/DatabaseManager.cpp`
- `src/server/ServerCore.cpp`
- `src/server/handlers/AdminRequestHandler.h`
- `src/server/handlers/AdminRequestHandler.cpp`
- `src/server/handlers/CartRequestHandler.cpp`
- `src/server/handlers/PublisherRequestHandler.h`
- `src/server/handlers/PublisherRequestHandler.cpp`

**Client side (5 files):**
- `client/src/services/AdminService.cpp`
- `client/src/services/PublisherService.cpp`
- `client/include/services/PublisherService.h`
- `client/qml/publisher/PublisherCatalogPage.qml`
- `client/qml/publisher/PublisherPromotionsPage.qml`

**Documentation (this file):**
- `MERGE_V8_CHANGELOG.md`

All other files are unchanged from `bookclub_build_ready.zip` (v7).
