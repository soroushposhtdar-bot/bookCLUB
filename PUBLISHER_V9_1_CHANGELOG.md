# Publisher Panel Fixes — v9.1

This patch addresses four classes of issues reported in the publisher panels.
All changes are backward-compatible — no protocol bumps, no schema migrations,
no breaking API changes.

## 1. Data sync mismatches across publisher panels

**Root cause:** `PublisherViewModel::refresh()` only kicked off a 400 ms async
delay and then re-emitted its per-property signals — it never actually told
`PublisherService` to invalidate its caches. The 30-second periodic refresh
timer in `PublisherShell.qml` therefore kept re-reading stale cached data
(the analytics cache, the books cache, the notifications cache) until the
publisher manually mutated something (addBook / removeBook / etc).

**Fix:**

- `client/src/viewmodels/publisher/PublisherViewModel.cpp` — `refresh()`
  now calls `m_service->refresh()` first, which clears every cache and
  re-fetches the publisher profile from `GetCurrentUser`, before starting
  the async delay and re-emitting signals.
- `client/qml/publisher/PublisherDashboardPage.qml` — added a
  `Connections { onBooksChanged: ... }` block that updates the
  "Last refreshed at" footer whenever the VM finishes a refresh, so the
  user can see the dashboard is actually live.
- `client/qml/publisher/PublisherProfilePage.qml` — fixed the catalog
  composition bars. `BookDto::status` returns `"active"`/`"inactive"`,
  but the bars expected `"published"`/`"draft"`/`"pending"`/`"removed"`,
  so they always rendered 0 across the board. Added a
  `_normalizeBookStatus()` helper that maps active→published and
  inactive→removed (the server has no draft/pending state, so those stay
  at 0 — which is correct).

## 2. Publisher TopBar

**Root cause:** A `PublisherTopBar.qml` already existed, but it used a plain
`Row` with an `Item { width: 1; height: 1 }` "spacer". `Row` does not expand
children, so the spacer stayed at 1 px and the user name + avatar sat
immediately next to the page title instead of being pushed to the right
edge. The avatar was also a bare `MouseArea` with no visual hover state,
making it easy to miss as a "profile" button.

**Fix:** Rewrote `client/qml/components/navigation/PublisherTopBar.qml`:

- Root container is now `RowLayout` so `Layout.fillWidth: true` on the
  spacer actually consumes the remaining horizontal space.
- Dark-mode toggle icon now flips between `dark_mode` and `light_mode`
  based on `Theme.isDark`, with a tooltip.
- Notifications bell keeps its unread-count badge and now has a tooltip
  showing the exact unread count.
- A "PUBLISHER" role chip is rendered before the user name so the role
  is always visible (toggleable via `showRoleChip`, default true).
- The avatar is now a real "profile button": a pill-shaped rectangle
  with a hover tint, a 1 px border on hover, a chevron icon, and a
  "View profile" tooltip.
- No cart button (publishers don't have a cart) — matches the spec.

## 3. Broken logics

**Root cause:** When the publisher edited their profile (publisher name,
biography, etc.) on the profile page, `PublisherService::updatePublisherProfile`
updated its local `m_profileCache` but never pushed the new name into
`AuthService::currentDisplayName`. The TopBar's `userName` binding read
`AuthService.currentDisplayName`, so the name stayed stale until the user
logged out and back in.

**Fix:**

- `client/include/services/AuthService.h` + `.cpp` — added a
  `Q_INVOKABLE setCurrentDisplayName(const QString&)` method that updates
  the cached name and emits `currentDisplayNameChanged()` so every QML
  binding re-evaluates.
- `client/src/services/PublisherService.cpp` — `updatePublisherProfile`
  now calls `AuthService::instance().setCurrentDisplayName(...)` after
  a successful `UpdateProfile` round-trip. `refreshProfileFromServer`
  also syncs the name (so the very first render after login shows the
  publisher's real `publisherName` from the Publishers table, not the
  Users-table `displayName` that AuthService was seeded with on login).
- `client/qml/publisher/PublisherShell.qml` — the TopBar's `userName`
  is now bound to a `_resolvedUserName()` helper that prefers the live
  `publisherProfile.publisherName` (which is the first thing to update
  after a profile save) and falls back to `AuthService.currentDisplayName`
  for the very first render. A defensive `Connections { onProfileChanged }`
  block is included in case future refactors break QML's automatic
  binding re-evaluation.

## 4. Publisher can actually publish / edit / change own info in real time

**Root cause:** The server's `PublisherRequestHandler::handlePublishBook`
already broadcast a `notifyNewBook` event (which sends an `EvtNotification`
to users whose favorite genres match), but it never pushed an `EvtBookAdded`
event. The client's `BookService` subscribes to `EvtBookAdded` and uses it
as the "catalog changed" signal — it clears its home-sections cache and
emits `booksChanged`. So users never saw newly-published books until their
60-second home-sections cache TTL expired, and updates / deactivations /
re-activations never propagated to connected users at all.

**Fix:**

- `src/server/NotificationDispatcher.h` + `.cpp` — added a
  `broadcastCatalogChanged(bookId, action)` method that pushes an
  `EvtBookAdded` event (with `{bookId, action}` payload) to every
  authenticated connection. Reuses the existing protocol command to
  avoid a protocol bump; the `action` field (`"published"` /
  `"updated"` / `"activated"` / `"deactivated"`) lets future clients
  differentiate if needed.
- `src/server/handlers/PublisherRequestHandler.cpp` — calls
  `m_dispatcher->broadcastCatalogChanged(bookId, ...)` from
  `handlePublishBook`, `handleUpdateBook`, `handleActivateBook`, and
  `handleDeactivateBook`. So every connected user's `BookService`
  invalidates its cache the instant the publisher mutates the catalog,
  and the next render of Home / Search / Category shows the new state.

The publisher profile update flow already worked server-side (`UpdateProfile`
persists `publisherName`/`biography`/`website`/`taxId` to the Publishers
table via `IUserRepository::update()`). The client-side fix (section 3
above) makes the new name show up in the TopBar instantly.

## Files changed

| File | Change |
| --- | --- |
| `client/include/services/AuthService.h` | Added `setCurrentDisplayName()` declaration |
| `client/src/services/AuthService.cpp` | Implemented `setCurrentDisplayName()` |
| `client/src/services/PublisherService.cpp` | `updatePublisherProfile` + `refreshProfileFromServer` now push the new name into `AuthService` |
| `client/src/viewmodels/publisher/PublisherViewModel.cpp` | `refresh()` now calls `m_service->refresh()` before re-emitting signals |
| `client/qml/components/navigation/PublisherTopBar.qml` | Full rewrite — `RowLayout`, hover affordance on profile button, role chip |
| `client/qml/publisher/PublisherShell.qml` | `userName` now bound to `_resolvedUserName()`; added `onProfileChanged` Connections |
| `client/qml/publisher/PublisherDashboardPage.qml` | Added `onBooksChanged` Connections to update "Last refreshed" timestamp |
| `client/qml/publisher/PublisherProfilePage.qml` | Added `_normalizeBookStatus()` so catalog composition bars reflect reality |
| `src/server/NotificationDispatcher.h` | Added `broadcastCatalogChanged()` declaration |
| `src/server/NotificationDispatcher.cpp` | Implemented `broadcastCatalogChanged()` |
| `src/server/handlers/PublisherRequestHandler.cpp` | Call `broadcastCatalogChanged()` on publish / update / activate / deactivate |

## Build & run

No CMakeLists changes are needed — every modified file was already
registered in the existing build graph. No new QML files were added, so
`qml.qrc` is unchanged. No new protocol commands were added, so
`Protocol.h` is unchanged. No schema changes, so `database/schema.sql`
and `database/migrations/*` are unchanged.

After unzipping this patch over the existing `bookclub_v9_merged` tree:

```bash
cd bookclub
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt6
cmake --build . -j
```

Then run the server (`./bookclub_server`) and the client
(`./bookclub_client`) as before. Demo accounts unchanged:
`admin` / `admin`, `publisher1` / `publisher1`, `amir` / `amir1234`.
