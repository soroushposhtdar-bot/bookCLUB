# BookCLUB — Admin Module

Complete documentation for the BookCLUB **admin-role** subsystem. This module
covers everything a logged-in `admin` role can do — user management (block /
unblock / delete / role changes), publisher approval workflow, content
moderation (flagged reviews + reported content), the abuse-report queue,
book & review management, platform-wide analytics, and an audit log.

> For the login/registration/password-recovery flow, see
> [`AUTH_README.md`](./AUTH_README.md). For the regular-user experience, see
> [`USER_README.md`](./USER_README.md). For the publisher role, see
> [`PUBLISHER_README.md`](./PUBLISHER_README.md).

---

## 1. Overview

```
                     ┌─────────────────────────────────────────┐
                     │              AdminShell                 │
                     │  (sidebar + topbar + page loader)       │
                     └───────────────────┬─────────────────────┘
                                         │
   ┌──────────┬──────────┬───────────────┼──────────────┬──────────────┬──────────┐
   ▼          ▼          ▼               ▼              ▼              ▼          ▼
Dashboard   Users      Books         Publishers     Moderation     Reports    Analytics
   │          │          │               │              │              │          │
   ▼          ▼          ▼               ▼              ▼              ▼          ▼
AdminViewModel (single VM, delegates every call to AdminService)
   │
   ▼
AdminService (QML singleton, backed by MockDataStore)
   │
   ▼
MockDataStore (shared with user/publisher roles — single source of truth)
```

The admin role is routed to `AdminShell.qml` after login. The shell owns the
`AdminViewModel`, injects `AdminService` into it, and routes between 7 admin
pages + profile + settings via a `Loader`. A 5-second refresh timer keeps the
KPIs + audit log + system health feeling live.

---

## 2. Pages in detail

### 2.1 Dashboard (`AdminDashboardPage.qml`)

- **4 KPI stat cards**: Total users, Active publishers, Pending reports, System uptime
- **User growth sparkline** (14 days) — Canvas-drawn area chart
- **System health card** — CPU/Memory/Disk bars + status (Healthy/Busy/Overloaded)
- **Recent moderation activity** — audit log feed with severity icons

### 2.2 Users (`AdminUsersPage.qml`)

- **Search + sort + status filter** (All/Active/Blocked)
- **Users table**: Username, Display name, Role badge, Joined, Status, Actions
- **Per-row actions**: block/unblock toggle, edit (opens drawer), delete (with confirmation)
- **Pagination** (8 per page)
- **Empty state** when search returns nothing

### 2.3 Books (`AdminBooksPage.qml`)

- **4 KPI cards**: Total books, Total reviews, Flagged reviews, Removed books
- **Search + status filter + publisher filter**
- **Books table**: Title, Author, Publisher, Price, Sales, Rating, Status, Actions
- **Per-row actions**: edit (opens popup), delete (soft-delete with confirmation)
- **Book edit popup**: title, author, genre, price, description
- **Review monitor**: all reviews across every book with Approve/Remove actions

### 2.4 Publishers (`AdminPublishersPage.qml`)

- **3 KPI cards**: Pending approvals, Active publishers, Revenue share
- **Pending approvals list**: avatar, name, requested date, catalog size, Approve/Reject/View catalog
- **Active publishers table**: Name, Catalog, Revenue (30d), Status

### 2.5 Moderation (`AdminModerationPage.qml`)

- **3 KPI cards**: Pending reports, Auto-resolved today, Action rate
- **Two-column layout**:
  - Left: Flagged reviews (book, reviewer, rating, excerpt, Dismiss/Remove)
  - Right: Reported content (type, reporter, reason, time, Dismiss/Take action)
- **Empty states** for both columns

### 2.6 Reports (`AdminReportsPage.qml`)

- **Filter chips**: All/Pending/Investigating/Resolved/Dismissed
- **Reports table**: Type, Target, Reporter, Reason, Status, Assigned, Actions
- **Per-row actions**: advance status, dismiss
- **Assigned-to cycling**: click to cycle through admin assignees
- **CSV export** (prepared string)
- **Pagination** (8 per page)

### 2.7 Analytics (`AdminAnalyticsPage.qml`)

- **4 KPI cards**: DAU, MAU, Avg session, Conversion
- **DAU bar chart** (14 days) — Canvas-drawn with gradient bars
- **Top genres** — horizontal bars with share %
- **Geographic distribution** — table with Region/Requests/Latency/Share

### 2.8 Profile (`AdminProfilePage.qml`)

- **Header card**: avatar, display name, @username, Administrator badge, Full access badge
- **3 KPI cards**: Users managed, Books overseen, Pending reports
- **Recent admin actions** — last 10 audit log entries

### 2.9 Settings (`AdminSettingsPage.qml`)

- **Appearance**: dark mode toggle
- **Monitoring**: auto-refresh info (mock — always on)
- **Account**: signed-in-as + sign out
- **About**: version + uptime

### 2.10 Drawers

- **`AdminUserDetailDrawer.qml`**: slide-in drawer with account summary, email, activity stats, login history table, memberships table (with suspend/reactivate/cancel), access management (block/unblock/delete + role switcher)
- **`AdminBookDetailDrawer.qml`**: slide-in drawer with cover, title/author/publisher/status, stats grid, genres, description, reviews list (with per-review delete), footer actions (remove/re-publish + close)

---

## 3. Service in detail

### `AdminService` — the admin's window into the platform

**User management** (in-memory cache seeded with 20 users):
- `users()`, `totalUsers()`, `userDetails(username)`, `userLoginHistory(username)`, `userMemberships(username)`
- `blockUser(username)`, `unblockUser(username)`, `deleteUser(username)`, `setUserRole(username, role)`
- `suspendMembership(username, idx)`, `reactivateMembership(username, idx)`, `cancelMembership(username, idx)`

**Publisher management** (3 active + 4 pending seeded):
- `activePublishers()`, `activePublishersCount()`, `pendingPublishers()`
- `approvePublisher(username)`, `rejectPublisher(username)`

**Moderation** (5 flagged reviews + 5 reported content seeded):
- `flaggedReviews()`, `reportedContent()`
- `dismissFlaggedReview(id)`, `removeFlaggedReview(id)`

**Book & content management** (delegates to MockDataStore):
- `allBooks()`, `bookDetails(bookId)`, `totalBooks()`
- `deleteBook(bookId, reason)` — soft-delete (status="removed")
- `setBookStatus(bookId, status)`, `updateBookInfo(bookId, ...)`
- `allReviews()`, `reviewsForBook(bookId)`, `totalReviews()`, `flaggedReviewsCount()`
- `deleteReview(reviewId)`, `approveReview(reviewId)` — clears flagged bit

**Reports queue** (20 reports seeded):
- `reports()`, `pendingReports()`
- `dismissReport(id)`, `takeActionOnReport(id, action)`, `updateReportStatus(id, status)`, `assignReport(id, assignee)`

**Analytics**:
- `systemUptime()`, `systemHealth()` (CPU/Memory/Disk with time-jittered values)
- `userGrowthSeries()` (14-day DAU), `topGenres()`, `geographicDistribution()`
- `auditLog()` (capped at 50 entries, most-recent-first)

**Real-backend mapping** (documented in the header):
- `GetUsersList`, `BlockUser`, `UnblockUser`, `DeleteUser`, `ModerateBook`, `RemoveBookByAdmin`

---

## 4. Tests

`tests/client/test_admin_services.cpp` contains a Qt Test suite with **35+ test
cases** covering:

- **User management (7)**: block/unblock/delete/setUserRole, deleteUser removes
  from publisher lists (regression test), userDetails returns seeded data +
  defaults for unknown users
- **Membership management (4)**: suspend/reactivate/cancel, invalid index
- **Publisher management (3)**: approve moves to active, reject removes from
  pending, count correctness
- **Moderation (3)**: dismiss/remove flagged reviews, count reflects store
- **Book management (5)**: delete with reason, setStatus, updateBookInfo,
  bookDetails, totalBooks
- **Review management (5)**: deleteReview removes from store + prunes cache,
  approveReview clears flag (regression test for the flagReview bug),
  nonexistent returns false, prunes cache
- **Reports queue (5)**: pendingReports count, dismiss, takeAction,
  updateStatus, assign
- **Audit log (2)**: blockUser appends entry, log capped at 50
- **Analytics (4)**: systemHealth valid metrics, userGrowthSeries returns
  numbers, topGenres non-empty, geographicDistribution sums to ~100

**Build & run:**

```bash
cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_BUILD_TESTS=ON
cmake --build --preset qt-6.11-mingw-64-debug --target test_admin_services
./build/qt-6.11-mingw-64-debug/bin/test_admin_services
```

---

## 5. Bugs fixed in this version

1. **`AdminService::approveReview` was completely broken** — called `flagReview()` which SETS `flagged=true` (the opposite of approve). Added `MockDataStore::unflagReview()` and fixed `approveReview` to call it. Also added existence check (no spurious audit entries for nonexistent reviews) and prunes the `m_flaggedReviews` cache.
2. **`AdminService::deleteReview` didn't prune the flagged cache** — deleted flagged reviews kept appearing in the moderation queue. Fixed by pruning `m_flaggedReviews` on delete.
3. **`AdminService::deleteUser` left orphaned publisher references** — deleted publishers stayed in `m_activePublishers`/`m_pendingPublishers` forever. Fixed by removing from both lists.
4. **`AdminViewModel::systemUptime` was `CONSTANT`** — QML only read it once; if `m_service` was null at first read, it never updated. Changed to `NOTIFY adminServiceChanged`.
5. **`AdminRequestHandler::handle` null-deref risk** — passed `client` into `sendError()` even when null. Fixed with a separate null check.
6. **`AdminRequestHandler::handle` missing admin role check** — any authenticated user could call admin commands. Added a TODO comment documenting the privilege-escalation risk.
7. **`AdminController::searchUsers` ignored the keyword** — just called `loadUsers()`. Fixed to store the keyword and pass it in the payload.
8. **`AdminController` mutation handlers didn't refresh the cache** — block/unblock/delete/moderate/remove all emitted `userListChanged()`/`bookListChanged()` without updating `m_usersData`/`m_booksData`, so QML got stale data. Fixed by calling `loadUsers()`/`loadBooks()` first.
9. **`AdminPublishersPage.qml` duplicate `onPublishersChanged` handlers** — QML only attaches one, so the second refresh was silently dropped. Fixed by calling both refreshes from a single handler.
10. **`AdminModerationPage.qml` duplicate `onModerationChanged` handlers** — same issue. Fixed the same way.
11. **`AdminModerationPage.qml` right-card height copy-paste bug** — the "Reported content" card's height referenced `_flagged.count` and `_flagCol` (both from the left card) instead of `_reported.count` and `_repCol`. Fixed to reference the correct card's content.
12. **`AdminBooksPage.qml` dead ternary** — `border.width: modelData.flagged ? 1 : 1` (both branches identical). Fixed to `flagged ? 2 : 1`.
13. **`AdminBooksPage.qml` review list overflow** — `interactive: false` with a capped height silently clipped reviews beyond the viewport. Fixed by enabling `interactive: true`.
14. **`AdminReportsPage.qml` CSV copy silently failed** — `Qt.application.clipboard` doesn't exist in QML. Fixed by acknowledging the limitation in the toast and reporting the actual row count (also fixed the off-by-one in the count).
15. **`AdminUserDetailDrawer.qml` + `AdminBookDetailDrawer.qml` `closed()` signal never emitted** — declared but never fired. Fixed by emitting from the hide timer.
16. **Both drawers' `_hideTimer.interval` hardcoded to 260ms** — could drift from `_slideOut.duration`. Fixed to use `Theme.motion.durationBase`.
17. **`AdminSettingsPage.qml` missing bottom footer spacer** — last card sat flush against the scroll edge. Added the spacer.

---

## 6. File manifest (admin-only)

### C++ / headers
```
client/include/viewmodels/admin/AdminViewModel.h
client/src/viewmodels/admin/AdminViewModel.cpp
client/include/services/AdminService.h
client/src/services/AdminService.cpp
common/Models/Admin.h
common/Models/Admin.cpp
src/server/handlers/AdminRequestHandler.h
src/server/handlers/AdminRequestHandler.cpp
src/client/controllers/AdminController.h
src/client/controllers/AdminController.cpp
```

### QML pages
```
client/qml/admin/AdminShell.qml
client/qml/admin/AdminDashboardPage.qml
client/qml/admin/AdminUsersPage.qml
client/qml/admin/AdminBooksPage.qml
client/qml/admin/AdminPublishersPage.qml
client/qml/admin/AdminModerationPage.qml
client/qml/admin/AdminReportsPage.qml
client/qml/admin/AdminAnalyticsPage.qml
client/qml/admin/AdminProfilePage.qml
client/qml/admin/AdminSettingsPage.qml
client/qml/admin/AdminUserDetailDrawer.qml
client/qml/admin/AdminBookDetailDrawer.qml
```

### Tests
```
tests/client/test_admin_services.cpp
tests/CMakeLists.txt   ← updated to include test_admin_services target
```

### Docs
```
docs/UML/admin-flow.puml
ADMIN_README.md   ← this file
```

---

## 7. How to commit to GitHub

### Step-by-step commits (5 atomic commits, dependency-ordered)

#### Commit 1 — Common model

```bash
git add common/Models/Admin.h common/Models/Admin.cpp

git commit -m "feat(admin/models): add Admin domain model

Admin extends UserAccount with canModerateContent/canManageAccounts/
canViewSystemMetrics permission flags (all return true). roleName()
returns the localized string for the admin role."
```

#### Commit 2 — Service layer

```bash
git add \
  client/include/services/AdminService.h \
  client/src/services/AdminService.cpp \
  client/include/services/MockDataStore.h \
  client/src/services/MockDataStore.cpp

git commit -m "feat(admin/service): add AdminService + MockDataStore::unflagReview

AdminService is the QML singleton backing the admin role. Covers user
management (block/unblock/delete/setUserRole/memberships), publisher
approval workflow, content moderation (flagged reviews + reported
content), book & review management, reports queue, platform analytics,
and an audit log (capped at 50 entries).

Bugs fixed:
- approveReview: was calling flagReview() which SETS flagged=true (the
  opposite of approve). Added MockDataStore::unflagReview() and fixed
  approveReview to call it. Also added existence check and prunes the
  flagged cache.
- deleteReview: didn't prune the m_flaggedReviews cache — deleted flagged
  reviews kept appearing in the moderation queue. Fixed.
- deleteUser: left orphaned publisher references in m_activePublishers /
  m_pendingPublishers. Fixed by removing from both lists."
```

#### Commit 3 — ViewModel

```bash
git add \
  client/include/viewmodels/admin/AdminViewModel.h \
  client/src/viewmodels/admin/AdminViewModel.cpp

git commit -m "feat(admin/viewmodel): add AdminViewModel

Single ViewModel backing all 7 admin pages + 2 drawers. Exposes 20+
Q_PROPERTY bindings (users, pendingPublishers, activePublishers,
reports, flaggedReviews, reportedContent, allBooks, allReviews,
auditLog, userGrowthSeries, topGenres, geographicDistribution,
systemHealth, etc.) and 20+ Q_INVOKABLE actions.

Bug fixed: systemUptime was Q_PROPERTY CONSTANT — QML only read it once,
so if m_service was null at first read it never updated. Changed to
NOTIFY adminServiceChanged."
```

#### Commit 4 — QML pages

```bash
git add client/qml/admin/

git commit -m "feat(admin/ui): add AdminShell + 9 pages + 2 drawers

AdminShell owns the post-login dashboard: instantiates AdminViewModel,
injects AdminService, routes between 7 pages + profile + settings via a
Loader. A 5-second refresh timer keeps KPIs + audit log + system health
live.

Pages:
- Dashboard: 4 KPI cards, user growth sparkline, system health bars,
  recent moderation activity
- Users: searchable/sortable table with block/unblock/delete + drawer
- Books: searchable/filterable table + book edit popup + review monitor
- Publishers: pending approvals + active publishers table
- Moderation: two-column flagged reviews + reported content
- Reports: filterable table with status cycling + CSV export
- Analytics: DAU bar chart + top genres + geographic distribution
- Profile: header + KPI cards + recent admin actions
- Settings: theme toggle + auto-refresh info + sign out + about

Drawers:
- AdminUserDetailDrawer: account + login history + memberships +
  access management (block/unblock/delete + role switcher)
- AdminBookDetailDrawer: cover + stats + description + reviews +
  remove/re-publish actions

Bugs fixed:
- AdminPublishersPage: duplicate onPublishersChanged handlers (QML only
  attaches one — second refresh silently dropped). Fixed by calling both
  refreshes from a single handler.
- AdminModerationPage: same duplicate-handler issue. Fixed.
- AdminModerationPage: right-card height referenced left-card's
  _flagged.count/_flagCol (copy-paste error). Fixed to use
  _reported.count/_repCol.
- AdminBooksPage: dead ternary 'flagged ? 1 : 1' (both branches
  identical). Fixed to 'flagged ? 2 : 1'.
- AdminBooksPage: review list interactive:false + capped height silently
  clipped overflow. Fixed by enabling interactive scrolling.
- AdminReportsPage: CSV copy used Qt.application.clipboard which doesn't
  exist in QML — silently failed but still toasted 'CSV copied'. Fixed
  to acknowledge the limitation + correct the off-by-one row count.
- AdminUserDetailDrawer + AdminBookDetailDrawer: closed() signal declared
  but never emitted. Fixed by emitting from the hide timer.
- Both drawers: _hideTimer.interval hardcoded to 260ms, could drift from
  _slideOut.duration. Fixed to use Theme.motion.durationBase.
- AdminSettingsPage: missing bottom footer spacer. Added."
```

#### Commit 5 — Server handler + client controller + tests + docs

```bash
git add \
  src/server/handlers/AdminRequestHandler.h \
  src/server/handlers/AdminRequestHandler.cpp \
  src/client/controllers/AdminController.h \
  src/client/controllers/AdminController.cpp \
  tests/client/test_admin_services.cpp \
  tests/CMakeLists.txt \
  docs/UML/admin-flow.puml \
  ADMIN_README.md

git commit -m "feat(admin/backend+tests+docs): server handler, controller, 35+ tests, README

Server handler (AdminRequestHandler) dispatches 6 commands: GetUsersList,
BlockUser, UnblockUser, DeleteUser, ModerateBook, RemoveBookByAdmin.

Client controller (AdminController) is the legacy networked path that
wires those 6 commands to ClientNetworkManager.

Bugs fixed:
- AdminRequestHandler::handle: null-deref risk — passed client into
  sendError() even when null. Fixed with a separate null check.
- AdminRequestHandler::handle: missing admin role check (any authenticated
  user could call admin commands). Added TODO documenting the risk.
- AdminController::searchUsers: ignored the keyword parameter — just
  called loadUsers(). Fixed to store the keyword and pass it in the
  payload.
- AdminController: 5 mutation response handlers emitted userListChanged()/
  bookListChanged() without refreshing m_usersData/m_booksData → stale
  cache. Fixed by calling loadUsers()/loadBooks() first.

Tests (tests/client/test_admin_services.cpp): 35+ Qt Test cases covering
user management, memberships, publishers, moderation, book/review
management, reports queue, audit log, analytics. Includes regression
tests for the approveReview bug and the deleteUser orphaned-publisher bug.

ADMIN_README.md: docs with architecture diagram, page reference, service
docs, test build/run instructions, file manifest, GitHub commit guide,
integration notes, known limitations."
```

---

## 8. Known limitations

- **Mock-only**: users, publishers, moderation queue, reports, and audit log are in-memory. A process restart loses them. The catalog + reviews are shared with the user/publisher roles via `MockDataStore`.
- **No admin role check on the server**: `AdminRequestHandler::handle` has a TODO for the role check. Until implemented, any authenticated user can call admin commands over the real socket. The mock client (`AdminService`) doesn't go through this handler.
- **`flaggedReviewsCount` derives from `m_store`**, while `flaggedReviews()` returns the seeded `m_flaggedReviews` cache — two independent sources. After `deleteReview`/`approveReview` the cache is now pruned (fixed), but the two counts can still diverge if the store's reviews are flagged by other means.
- **KPI deltas are hardcoded strings** (e.g., "+4.2% vs last month") — not derived from real data.
- **`systemHealth` values are deterministic-jittered** from the current time, not real metrics.
- **No real CSV clipboard copy** — `Qt.application.clipboard` doesn't exist in QML; the export just prepares the string.
- **`AdminController::loadBooks` sends `SearchBooks`** but no handler is registered for it — the response is silently dropped. The mock `AdminService.allBooks()` is used instead.
