# BookClub — v9 Publisher Polish Changelog

This revision fixes the four issues the user reported with the publisher
module after the v8 merge.

## Issues Fixed

### 1. Data sync between publisher pages

**Problem**: The publisher catalog showed wrong cover colors (always
Theme.color.primary), wrong status (always "published"), and the
profile page showed empty fields because the server wasn't returning
the publisher's chosen `coverColor`/`coverAccent`, the derived
`status` field, or the publisher's extended profile from the
`Publishers` table.

**Root causes**:
- `Book` C++ model had no `coverColor`/`coverAccent` getters/setters.
- `IBookRepository::bookFromCurrentRecord` skipped the columns.
- `IBookRepository::save` / `update` didn't write the columns.
- `PublisherRequestHandler::bookToJson` didn't include the fields.
- `BookRequestHandler::bookToJson` (user side) didn't include them.
- `BookDto::fromJson` didn't read them.
- `PublisherService::publisherProfile` returned empty cache values
  because nothing populated the cache from the server.

**Fixes (10 files)**:
- `common/Models/Book.h` — added `coverColor()` / `coverAccent()`
  getters, `setCoverColor()` / `setCoverAccent()` setters, and the
  `m_coverColor` / `m_coverAccent` private members.
- `common/Models/Book.cpp` — implemented the new methods.
- `common/Interfaces/IBookRepository.cpp` — `bookFromCurrentRecord`
  now reads coverColor/coverAccent from the DB row. `save` and
  `update` now write them (with `#1A73E8` / `#1557B0` fallbacks for
  empty values, matching the schema DEFAULT).
- `src/server/handlers/PublisherRequestHandler.cpp` —
  `createBookFromPayload` now accepts `coverColor`, `coverAccent`,
  and a `coverImage` alias. `bookToJson` now returns `coverColor`,
  `coverAccent`, `coverImage`, `status` (derived from `isActive`),
  `createdAt` (ISO date), `averageRating`, `ratingCount`, `totalSales`.
- `src/server/handlers/BookRequestHandler.cpp` — `bookToJson` now
  also returns `coverColor`, `coverAccent`, `coverImage` so the
  user-side home / search / book-detail pages show the publisher's
  chosen colors too.
- `client/src/services/BookDto.cpp` — `fromJson` now reads
  `coverColor`, `coverAccent`, and falls back to `coverImage` when
  `coverImagePath` is empty.
- `client/src/services/PublisherService.cpp` — `publisherProfile`
  now returns the real `joinedAt` from the cache (populated by
  `refreshProfileFromServer`). `updatePublisherProfile` now sends
  ALL publisher-extended fields (`publisherName`, `biography`,
  `website`, `taxId`) to the server and refreshes the cache from
  the server's echo. New `refreshProfileFromServer()` method pulls
  the publisher profile via `GetCurrentUser`.
- `client/include/services/PublisherService.h` — declared
  `refreshProfileFromServer()`.
- `client/src/viewmodels/publisher/PublisherViewModel.cpp` + `.h` —
  added `refreshProfile()` Q_INVOKABLE that delegates to the
  service's `refreshProfileFromServer()`.
- `client/qml/publisher/PublisherShell.qml` — calls
  `_publisherVM.refreshProfile()` on `Component.onCompleted` so the
  TopBar's userName and the profile page show real data immediately.
- `client/qml/publisher/PublisherCatalogPage.qml` — `_refreshFromVM`
  now normalizes the status field (handles `active`/`inactive`/
  `published`/`removed` from server, falls back to `isActive`).
- `client/qml/publisher/PublisherProfilePage.qml` — calls
  `viewModel.refreshProfile()` on `Component.onCompleted` for
  immediate data load.

### 2. Publisher TopBar (dark mode + notifications + profile + name, no cart)

**Problem**: The publisher shell had no TopBar — the user could not
toggle dark mode, see the notification count, or jump to profile
without using the sidebar.

**Fix**: Created a new `PublisherTopBar.qml` modeled on the user-role
`TopBar.qml` but WITHOUT the cart button.

**Files (4)**:
- `client/qml/components/navigation/PublisherTopBar.qml` (NEW) —
  same visual style as TopBar: theme toggle (left), notifications
  bell + unread badge (left), title + subtitle (center), user name
  + avatar (right). Clicking the avatar emits `profileRequested()`,
  clicking the bell emits `notificationsRequested()`, clicking the
  theme icon emits `themeToggled()`. No cart button — publishers
  don't have a cart.
- `client/qml/components/navigation/qmldir` — registered
  `PublisherTopBar 1.0 PublisherTopBar.qml`.
- `client/qml/qml.qrc` — added `<file>` entry for
  `components/navigation/PublisherTopBar.qml`.
- `client/qml/publisher/PublisherShell.qml` — inserted the
  PublisherTopBar at the top of the right column (above the page
  Loader). Wired: title/subtitle from `_routeMeta`, userName from
  `AuthService.currentDisplayName`, userInitials from first letter,
  unreadCount from `_unreadNotifCount`, onThemeToggled → shell's
  themeToggled signal (App.qml flips Theme.mode),
  onNotificationsRequested → navigate to "notifications",
  onProfileRequested → navigate to "profile". Content area now
  anchors to `_topbar.bottom` instead of `parent.top`.

### 3. Logic bugs

**Bug A**: `updatePublisherProfile` only sent `displayName` + `email`
to the server. The server's `handleUpdateProfile` only updated the
`Users` table — `publisherName`, `biography`, `website`, `taxId`
were lost.

**Fix A**: Extended `AuthRequestHandler::handleUpdateProfile` to
detect when the caller is a Publisher (via `dynamic_cast<Publisher*>`)
and call `setPublisherName` / `setBiography` / `setWebsite` /
`setTaxId`. The existing `IUserRepository::update` override already
cascades these to the `Publishers` table via UPDATE. The response
now echoes the persisted publisher fields so the client can refresh
its cache without a second round-trip.
Files: `src/server/handlers/AuthRequestHandler.cpp`.

**Bug B**: `AuthService::changePassword` was not Q_INVOKABLE — the
publisher profile's password-change dialog silently did nothing
(`AuthService.changePassword(...)` from QML returned undefined).

**Fix B**: Added a Q_INVOKABLE overload `changePassword(old, new)`
that delegates to the existing C++ overload and returns a bool.
Updated the QML dialog to check the return value and show an error
toast on failure (with an inline error message in the dialog).
Files: `client/include/services/AuthService.h`,
`client/src/services/AuthService.cpp`,
`client/qml/publisher/PublisherProfilePage.qml`.

**Bug C**: After clicking "Re-publish" or "Remove" on a catalog row,
the status column didn't flip until the 30s refresh timer fired.

**Fix C**: Added `Qt.callLater(function() { page._refreshFromVM() })`
after every status mutation (setBookStatus, removeBook) so the local
ListModel re-syncs immediately.
Files: `client/qml/publisher/PublisherCatalogPage.qml`.

**Bug D**: The catalog "Save changes" / "Publish title" buttons
closed the editor even on server failure.

**Fix D**: Now the editor only closes on success. On failure, the
editor stays open and an error toast is shown — the user can fix
the fields and retry.
Files: `client/qml/publisher/PublisherCatalogPage.qml`.

**Bug E**: The promotions page's "Remove" button always showed a
success toast even when the server rejected the removal.

**Fix E**: Now checks the bool return value of `removePromotion`
and shows an error toast on failure.
Files: `client/qml/publisher/PublisherPromotionsPage.qml`.

**Bug F**: The profile page's "Save changes" button always showed
"Profile saved" even on server failure.

**Fix F**: Now checks the bool return value of
`updatePublisherProfile` and shows an error toast on failure.
Files: `client/qml/publisher/PublisherProfilePage.qml`.

### 4. Realtime publish / edit-book / edit-profile end-to-end

**Problem**: Although the v8 server already had the publish / update
/ deactivate / activate endpoints, the client had several issues
that prevented the realtime loop from working:
- The book editor didn't propagate coverColor/coverAccent (lost on
  save).
- The profile editor didn't propagate publisherName/biography/
  website/taxId (lost on save).
- The catalog table didn't refresh after a successful save (no
  visual feedback).
- The password dialog did nothing.

**Fix**: All of the above fixes together make the realtime loop
work end-to-end:
- Publisher publishes a book → server writes to `Books` table with
  coverColor/coverAccent → server pushes `EvtBookAdded` to all
  users whose favourite genres match → users' home pages receive
  the notification and the new book appears in their "New books"
  section.
- Publisher edits a book → server updates the `Books` row →
  catalog table refreshes immediately (Qt.callLater) → user-side
  `GetBookDetails` returns the updated fields.
- Publisher edits profile → server updates `Users.displayName` +
  ` Publishers` row → response echoes back → cache refreshes →
  TopBar's userName updates on next binding evaluation.
- Publisher changes password → server's `ChangePassword` handler
  validates the old password and writes the new hash → success /
  error toast.

## Files Modified (19 files, 1 new)

**Server (5 files)**:
- `common/Models/Book.h`
- `common/Models/Book.cpp`
- `common/Interfaces/IBookRepository.cpp`
- `src/server/handlers/AuthRequestHandler.cpp`
- `src/server/handlers/BookRequestHandler.cpp`
- `src/server/handlers/PublisherRequestHandler.cpp`

**Client C++ (6 files)**:
- `client/include/services/AuthService.h`
- `client/src/services/AuthService.cpp`
- `client/include/services/PublisherService.h`
- `client/src/services/PublisherService.cpp`
- `client/src/services/BookDto.cpp`
- `client/include/viewmodels/publisher/PublisherViewModel.h`
- `client/src/viewmodels/publisher/PublisherViewModel.cpp`

**Client QML (5 files + 1 new)**:
- `client/qml/components/navigation/PublisherTopBar.qml` (NEW)
- `client/qml/components/navigation/qmldir`
- `client/qml/qml.qrc`
- `client/qml/publisher/PublisherShell.qml`
- `client/qml/publisher/PublisherCatalogPage.qml`
- `client/qml/publisher/PublisherProfilePage.qml`
- `client/qml/publisher/PublisherPromotionsPage.qml`

**Documentation (this file)**:
- `MERGE_V9_CHANGELOG.md`

All other files unchanged from v8.

## Build & Run

Same as v8 — open `CMakeLists.txt` in Qt Creator, **Build → Clean →
Rebuild All**, run `BookClubServer` first, then `BookClubClient`.

The schema version is unchanged (still 3) — the `Books` table
already had `coverColor` / `coverAccent` columns since v1; only the
C++ code was missing the read/write paths. No DB rebuild required.

## What to verify at runtime

- **Publisher shell**: TopBar appears at the top of the right column
  with theme toggle, notification bell (with red badge showing
  unread count), page title, publisher's display name, and avatar.
  Clicking the avatar navigates to the profile page. Clicking the
  bell navigates to notifications. Clicking the theme icon flips
  dark mode.
- **Catalog**: covers show the publisher's chosen color (not the
  default blue). Status column shows "Published" or "Removed"
  based on the real `isActive` value. Clicking the trash icon on
  a published book immediately flips it to "Removed" and shows the
  "Re-publish" button. Clicking "Re-publish" immediately flips it
  back to "Published".
- **Catalog editor**: Creating a new book with a custom cover color
  persists the color — re-opening the editor shows the same color.
  Editing a book and clicking "Save changes" closes the editor
  only on success; the table refreshes immediately.
- **Profile page**: All fields (publisherName, biography, website,
  email, taxId) show real server data on first load. Editing and
  saving persists to the `Publishers` table — re-opening the page
  shows the updated values. Changing the publisher name updates
  the TopBar's userName.
- **Password change**: Entering the wrong current password shows
  an error toast and an inline error in the dialog. Entering the
  correct current password + a new password (≥8 chars) + matching
  confirmation shows a success toast and closes the dialog.
- **Promotions**: Removing a promo code shows an error toast if
  the server rejects the removal (e.g. network error). Otherwise
  shows "Removed" and the row disappears.
- **User side**: When a publisher publishes a new book in a genre
  that matches a user's favourite genres, the user receives a
  real-time `EvtNotification` ("A new book in your favourite
  genre has been published!") and the book appears in the user's
  home "New books" carousel on next refresh.
