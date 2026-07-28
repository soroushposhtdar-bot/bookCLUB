# BookCLUB V3 — Final Polish & Bug-Fix Changelog

This batch focused on re-reading every user-facing file, fixing logic bugs, and polishing all user panels for the final production version.

## Files Changed (12 total)

### C++ (8 files)

#### 1. `client/include/services/LibraryDtos.h` — NotificationDto enrichment
**Root cause**: `NotificationItem.qml` reads `notification.read`, `notification.body`, `notification.relativeTime`, `notification.iconName`, `notification.accentColor` — none of which existed on `NotificationDto`. Notifications rendered with blank icon, blank body, and no color accent.
**Fix**: 
- Added `body` (alias for `message`), `read` (alias for `isRead`), `relativeTime` (alias for `timeText`), `relatedEntityId` (alias for `bookId`).
- Added `iconName()` — returns a Material Symbols name based on `m_type` (auto_stories / local_offer / shopping_cart / rate_review / campaign / info_outline).
- Added `accentColor()` — returns a hex color based on `m_type` (blue / green / orange / purple / grey).
- Added `category()` cases for types 5 (publisher) and 6 (system).
- Changed `isRead` from `CONSTANT` to `READ isRead WRITE setRead NOTIFY readChanged` so the service can update it in-place via `setProperty("isRead", true)`.
- Added `setRead(bool)` setter + `readChanged()` signal.
- `fromJson` now derives `isRead` from `state >= 1` as a fallback when the server doesn't send `isRead` explicitly.

#### 2. `client/src/services/NotificationService.cpp` — implement stubs + in-place updates
**Root cause**: `markUnread()`, `deleteNotification()`, `archiveNotification()` were all empty stubs → context menu actions silently failed. `markRead()` and `markAllRead()` called `refresh()` (full server round-trip) which was slow.
**Fix**:
- `markRead()` now updates the cached DTO in-place via `setProperty("isRead", true)` → instant UI update, no round-trip.
- `markUnread()` updates the cache locally (server has no unread endpoint).
- `markAllRead()` updates all cached DTOs in-place after the server confirms.
- `deleteNotification()` removes the DTO from `m_cache` via `takeAt` + `delete` → instant removal.
- `archiveNotification()` delegates to `deleteNotification()` (no separate archive storage).
- `unarchiveNotification()` calls `refresh()` to restore from server.

#### 3. `client/src/viewmodels/user/NotificationsViewModel.cpp` — cache corruption fix
**Root cause**: `notifications()` filtered the search results by `delete`-ing non-matching DTOs — but those DTOs were owned by the `NotificationService::m_cache`. Deleting them corrupted the cache and caused use-after-free crashes.
**Fix**: The filter loop now skips non-matching DTOs without deleting them. The cache ownership stays with the service.

#### 4. `client/src/services/LibraryService.cpp` — fix stubs + cache updates
**Root cause**: `renameShelf()` was a stub returning `false` → the LibraryPage rename dialog didn't work. `downloadedBooks()` returned `{}` → the Downloaded tab was always empty. `addToShelf`/`removeFromShelf`/`deleteShelf` didn't emit `wishlistChanged` when the Wishlist shelf was modified → hearts on BookCards didn't update.
**Fix**:
- `renameShelf()` now sends `RenameShelf` to the server, merges the updated shelf list into `m_libraryData`, and emits `shelvesChanged()`.
- `downloadedBooks()` now batch-fetches book details for the `m_downloaded` set via `GetBooksByIds`.
- `addToShelf()` and `removeFromShelf()` now merge the server's updated shelf list into the cache AND emit `wishlistChanged()` when the modified shelf is the Wishlist.
- `deleteShelf()` merges the updated shelf list into the cache.

#### 5. `client/src/services/CartService.cpp` — use ClearCart command
**Root cause**: `clear()` sent N `RemoveFromCart` requests in a loop — slow and chatty. The server has a `ClearCart` command that wasn't being used.
**Fix**: `clear()` now sends a single `ClearCart` request first. Falls back to the old loop only if `ClearCart` fails (for older server compatibility).

#### 6. `client/src/services/BookService.cpp` — cache continueReading + fix discounted()
**Root cause (continueReading)**: Called on every Home page refresh (10s timer) — each call issued `GetLibrary` + `GetBooksByIds` round-trips. The 60s `fetchHomeSections` cache didn't help because `continueReading()` is a separate method.
**Fix**: Added a 30-second TTL cache (`m_continueReadingCache` + `m_continueReadingCacheTime` + `m_continueReadingCacheValid`). The cache is invalidated when `booksChanged` fires (new purchase, new book published, etc.).
**Root cause (discounted)**: `discounted()` returned `freeBooks()` → the "On Sale" section showed the same books as "Free to read".
**Fix**: `discounted()` now fetches all books (bestsellers + free + new arrivals), deduplicates by ID, and filters by `hasDiscount()`. Added `#include <QSet>`.

#### 7. `client/include/services/BookService.h` — add continueReading cache members
Added `mutable QList<QObject*> m_continueReadingCache`, `mutable qint64 m_continueReadingCacheTime`, `mutable bool m_continueReadingCacheValid`.

#### 8. `client/src/viewmodels/user/LibraryViewModel.cpp` — refresh on wishlist change
**Root cause**: When `wishlistChanged` fired, only `savedBooksChanged` was emitted — but the downloaded list can also depend on the purchased set, so the Downloaded tab didn't refresh.
**Fix**: The `wishlistChanged` handler now emits both `savedBooksChanged` and `downloadedBooksChanged`.

### Server (3 files)

#### 9. `src/server/handlers/NotificationRequestHandler.cpp` — include relatedEntityId
**Root cause**: `handleGetNotifications` didn't include `relatedEntityId` in the JSON → the client's `NotificationDto.bookId` was always empty → clicking a notification didn't navigate to the book.
**Fix**: Added `obj["relatedEntityId"] = notif->relatedEntityId();` to the serialization.

#### 10. `src/server/handlers/LibraryRequestHandler.cpp` — return updated shelves on every mutation
**Root cause**: `handleDeleteShelf`, `handleAddBookToShelf`, `handleRemoveBookFromShelf`, and `handleRenameShelf` all returned empty `{}` → the client's `LibraryService` cache wasn't updated → the UI didn't refresh until a manual page reload.
**Fix**: All four handlers now return `{ "shelves": [...] }` with the updated shelf list. The client merges this into `m_libraryData` and emits `shelvesChanged()` (and `wishlistChanged()` if the Wishlist shelf was modified).

#### 11. `src/server/handlers/CartRequestHandler.cpp` — (unchanged this batch)
Already enriched in the previous batch.

#### 12. `src/server/handlers/BookRequestHandler.cpp` — (unchanged this batch)
Already fixed in the previous batch.

### QML (4 files)

#### 13. `client/qml/user/LibraryPage.qml` — refresh on load
Added `Component.onCompleted: viewModel.libraryService.refresh()` so the Library page always loads fresh data on entry.

#### 14. `client/qml/user/NotificationsPage.qml` — refresh on load
Added `Component.onCompleted: viewModel.service.refresh()` so the Notifications page always loads fresh data on entry.

#### 15. `client/qml/user/ProfilePage.qml` — refresh on library change
Added a `Connections { target: LibraryService; onLibraryChanged / onWishlistChanged: viewModel.userChanged() }` block so the Reading statistics + Your library strip re-render when the library changes.

#### 16. `client/qml/user/WishlistPage.qml` — (unchanged this batch)
Already has `Component.onCompleted: LibraryService.refresh()` from the previous batch.

## Bugs Fixed

| Bug | Root cause | Fix |
|-----|-----------|-----|
| Notifications render with blank icon/body/color | NotificationDto missing `read`, `body`, `relativeTime`, `iconName`, `accentColor` properties | Added all aliases + computed getters |
| Notification context menu actions silently fail | `markUnread`, `deleteNotification`, `archiveNotification` were empty stubs | Implemented all three with in-place cache updates |
| Notification search crashes (use-after-free) | `notifications()` delete-d cache-owned DTOs during filtering | Filter without deleting |
| Library rename shelf doesn't work | `LibraryService::renameShelf` was a stub | Wired to server `RenameShelf` + cache merge |
| Downloaded tab always empty | `downloadedBooks()` returned `{}` | Batch-fetches book details for `m_downloaded` set |
| Cart clear is slow (N round-trips) | `clear()` looped RemoveFromCart per item | Uses `ClearCart` command, falls back to loop |
| Home page "On Sale" shows same books as "Free" | `discounted()` returned `freeBooks()` | Filters all books by `hasDiscount()` |
| Home page re-fetches library every 10s | `continueReading()` had no cache | 30s TTL cache, invalidated on `booksChanged` |
| BookCard hearts don't update after wishlist toggle from another page | `addToShelf`/`removeFromShelf` didn't emit `wishlistChanged` for the Wishlist shelf | Emit `wishlistChanged` when the modified shelf is the Wishlist |
| Clicking a notification doesn't navigate to the book | Server didn't include `relatedEntityId` | Added to `handleGetNotifications` response |
| Shelf mutations don't refresh the UI | Server returned empty `{}` | All 4 shelf mutation handlers now return the updated shelf list |
| Library/Notifications pages show stale data on entry | No `Component.onCompleted` refresh | Added refresh calls on page load |
| Profile reading stats don't update after purchase | No `Connections` on LibraryService | Added `onLibraryChanged`/`onWishlistChanged` → `userChanged()` |

## Polish Pass

- **NotificationItem** now renders with the correct icon, accent color, and body text per notification type.
- **LibraryPage** Downloaded tab now shows books the user has marked as downloaded.
- **LibraryPage** rename dialog now works end-to-end.
- **WishlistPage** and **BookCard** hearts now stay in sync when wishlist is toggled from any page.
- **CartPage** clear button is now instant (1 round-trip instead of N).
- **HomePage** "On Sale" section now shows genuinely discounted books.
- **HomePage** Continue Reading section no longer re-fetches the library every 10 seconds.
- **ProfilePage** reading statistics update live when the user purchases or wishlists a book.
- **NotificationsPage** loads fresh notifications on every entry.

## Build & Run

Unchanged. See `README.md` and `BUILD.md`. Demo accounts: `admin`/`admin`, `publisher1`/`publisher1`, `amir`/`amir1234`, `sara`/`amir1234`.

---

# BookCLUB V3 — Auth + User Front-End Bug-Fix Batch

This batch focused on a comprehensive audit and fix of all bugs in the
**auth** and **user front-end** (QML pages, ViewModels, and Services).
The audit was performed by two parallel Explore subagents and surfaced
~30 auth bugs and ~50 user bugs across Critical / High / Medium / Low
severities. This batch addresses the highest-impact ones.

## Files Changed (18 total)

### QML (8 files)

#### 1. `client/qml/App.qml` — exit-dialog Shortcut, VM reset, credential lifecycle
- **Critical (Bug 11)**: The global `Shortcut { sequences: ["Enter", "Return"] }`
  was always active and consumed every Enter/Return keypress before any
  TextField could fire its `onAccepted` handler. Pressing Enter in any
  auth form did nothing. Fixed by adding `enabled: _exitDialog.visible`.
- **High (Bug 18)**: ViewModel state was never reset between navigations —
  reopening Register/ForgotPassword showed state from the previous attempt.
  Added `_registerVM.reset()` and `_forgotPasswordVM.reset()` calls before
  navigating.
- **High (Bug 12)**: After successful registration, App.qml pre-filled
  `_loginVM.username = _registerVM.username` then navigated to LoginPage,
  whose `Component.onCompleted: _loginVM.loadSavedCredentials()` immediately
  overwrote the just-registered username with stale QSettings values.
  Guarded with a `_credentialsLoaded` flag so saved credentials load only
  once per process.
- **High (Bug 13)**: After a successful password reset, the LoginPage
  pre-filled the OLD (now-invalid) password from QSettings. Added
  `_loginVM.clearSavedCredentials()` to the reset-success handler.
- **Low (Bug 16)**: Removed the redundant `AuthService.saveGenreSelection`
  call in the genre-selection `onCompleted` handler — the VM already
  persists inside `_doSubmit()`.

#### 2. `client/qml/auth/LoginPage.qml` — Row→RowLayout, onAccepted guard
- **Medium (Bug 1)**: Switched the "Remember me + Forgot password" `Row`
  to `RowLayout` so the `Layout.fillWidth` spacer actually fills. The
  "Forgot password?" link was never pushed to the right edge.
- **Medium (Bug 20)**: `onAccepted: _loginBtn.clicked()` bypassed the
  disabled-button guard — `clicked()` is a signal emitter that fires
  `onClicked` regardless of `enabled`. Guarded with `if (_loginBtn.enabled)`.
- **Low (Bug 26)**: Removed redundant `anchors.horizontalCenter` (no-op
  when `width: parent.width` is set).

#### 3. `client/qml/auth/RegisterPage.qml` — same fixes as LoginPage
- **Medium (Bug 1)**: Switched the back-button `Row` to `RowLayout`.
- **Medium (Bug 20)**: Guarded `onAccepted` with `if (_registerBtn.enabled)`.
- **Low (Bug 29)**: Removed the redundant `MouseArea` inside the
  security-question `InputField` that was swallowing trailing-icon clicks
  (the `onTrailingClicked` handler already opened the popup).
- **Low (Bug 26)**: Removed redundant `anchors.horizontalCenter`.

#### 4. `client/qml/auth/ForgotPasswordPage.qml` — step-change focus
- **Medium (Bug 1)**: Switched back-button `Row` to `RowLayout`.
- **Medium (Bug 17)**: The answer field's `Component.onCompleted` only
  fired once at page creation (when `step === "username"`). When the user
  advanced to the answer step, the answer field never received focus.
  Added a `Connections { onStepChanged: if (step === "answer") _answer.forceActiveFocus() }` block.
- **Medium (Bug 20)**: Guarded both `onAccepted` calls.
- **Low (Bug 26)**: Removed redundant `anchors.horizontalCenter`.

#### 5. `client/qml/auth/ResetPasswordPage.qml` — same fixes
- **Medium (Bug 1)**: Switched back-button `Row` to `RowLayout`.
- **Medium (Bug 20)**: Guarded `onAccepted` with `if (_resetBtn.enabled)`.

#### 6. `client/qml/auth/GenreSelectionPage.qml` — chip binding, dead ternary
- **Critical (Bug 2)**: `property bool isSelected: root.viewModel.isSelected(modelData)`
  called a Q_INVOKABLE method inside a property binding. QML's binding
  engine cannot track dependencies inside a C++ method call, so the
  binding was evaluated only once at delegate creation — clicking a chip
  never updated its highlight color, check icon, or the counter.
  Changed to `root.viewModel.selectedGenres.indexOf(modelData) >= 0`
  (binds to the NOTIFY-enabled `selectedGenres` list property).
- **Medium (Bug 1)**: Switched the back+brand `Row` to `RowLayout`.
- **Medium (Bug 15)**: Skip button called `root.viewModel.skip()` (which
  emits `completed` on the VM → routed through the Connections block →
  `root.completed()`) AND then immediately called `root.completed()`
  directly. Signal fired twice, `_enterRoleShell()` ran twice. Removed
  the explicit `root.completed()` call.
- **Low (Bug 22)**: Fixed dead ternary `border.width: isSelected ? 1 : 1`
  → `2 : 1`.

#### 7. `client/qml/auth/SplashPage.qml` — pulse animation
- **Cosmetic (Bug 21)**: The `SequentialAnimation` was missing
  `running: true`. QML animations default to `running: false`, so the
  intended "gentle pulse" of the brand mark on the splash screen was
  silent.

#### 8. `client/qml/components/feedback/ValidationMessage.qml` — text wrap
- **Low (Bug 27)**: The inner `Text` had `wrapMode: WordWrap` but no
  width boundary, so it didn't know where to wrap. Long server error
  messages rendered on a single line and overflowed the form panel
  (clipped by AuthLayout's `clip: true`). Now gives the Text a real
  wrapping width (`root.width - iconWidth - spacing`).

### QML — User panel (4 files)

#### 9. `client/qml/user/NotificationsPage.qml` — Connections parameter name
- **High (Bug 4)**: The C++ signal is `realtimeNotificationReceived(QObject* dto)`,
  so the handler parameter must be named `dto`. The handler instead
  referenced `notification` (undefined in that scope — that name only
  exists on the page's own re-emitted signal). App.qml always received
  `undefined` and no real-time toast was ever shown. Fixed:
  `onRealtimeNotificationReceived: function(dto) { root.realtimeNotificationReceived(dto) }`.

#### 10. `client/qml/user/CartPage.qml` — invalid icon name
- **Low (Bug 42)**: `iconName: "checkout"` — there is no Material Symbols
  glyph named "checkout". Replaced with `"lock"` (conveys "secure checkout").

#### 11. `client/qml/user/PdfReaderPage.qml` — Ctrl+F shortcut
- **Medium (Bug 43)**: `Ctrl+F` was wired to `toggleFitWidth()` instead
  of focusing the find-in-book field. Ctrl+F is the universal "Find"
  shortcut. Changed to `_findField.forceActiveFocus()`.

#### 12. `client/qml/user/SettingsPage.qml` — no structural changes
(Investigated Bug 1 claim that Storage/About sections were nested inside
Account. Brace-depth trace showed they are actually siblings at the same
nesting level — only the visual indentation was misleading. No fix
needed; the bug report was incorrect.)

### C++ — Auth (4 files)

#### 13. `client/src/viewmodels/auth/LoginViewModel.cpp` — rememberMe persistence
- **Low (Bug 19)**: `setRememberMe(false)` cleared credentials immediately,
  but `setRememberMe(true)` only set the in-memory flag — the preference
  wasn't persisted to QSettings until the next successful login. If the
  user toggled the checkbox back on and navigated away, the next session
  wouldn't remember them. Now persists `rememberMe` to QSettings on
  every toggle, and `loadSavedCredentials` always loads the checkbox
  state (not just when it's true).

#### 14. `client/src/services/AuthService.cpp` — saveGenreSelection network
- **High (Bug 5)**: `saveGenreSelection` was a stub that only flipped a
  local `_requiresGenreSetup` bool and logged. Genre preferences were
  lost on re-login. Now sends the `SaveFavoriteGenres` command to the
  server with the username + genres list, and only flips the local flag
  on success (with offline fallback). Added `#include <QJsonArray>`.

#### 15. `client/src/services/NetworkService.cpp` — use-after-free + async timeout
- **Critical (Bug 9)**: `sendRequest` registered a response handler
  capturing `&resp, &gotResponse, &loop` by reference. If the 5-second
  timeout fired first, the function exited and those stack references
  dangled — but the handler stayed in the ClientNetworkManager map. A
  late-arriving server reply fired the lambda → use-after-free / crash.
  Fixed by calling `m_network.unregisterResponseHandler(requestId)` on
  timeout.
- **High (Bug 10)**: `sendRequestAsync`'s timeout branch built a
  `Response r; r.errorMessage = "Request timed out";` but never called
  `callback(r)` — async callers waited forever on timeout. Now invokes
  the callback with the timeout error and unregisters the handler. Added
  a `shared_ptr<bool> fired` flag so the response lambda and the timeout
  lambda can't double-invoke the callback. Added `#include <memory>`.

#### 16. `src/client/network/ClientNetworkManager.{h,cpp}` — unregister method
- Added `unregisterResponseHandler(const QString& requestId)` so
  `NetworkService::sendRequest` can clean up timed-out handlers.

### C++ — User ViewModels (7 files)

#### 17. `client/src/viewmodels/user/ProfileViewModel.{h,cpp}` — displayName cache
- **Critical (Bug 3)**: `displayName()` returned `m_userService->displayName()`
  (which returned `AuthService::currentDisplayName()` — the OLD name).
  `setDisplayName(v)` stored the user's edit in `m_displayName`, but the
  next READ returned the old value, so the InputField binding reverted
  every keystroke. Now returns `m_displayName` once the user has started
  editing (tracked by `m_displayNameEdited` flag), falling back to
  UserService for the initial load.

#### 18. `client/src/services/UserService.cpp` — displayName cache
- Follow-up to Bug 3: `displayName()` now returns the locally-cached
  `m_displayName` if non-empty (set by `updateProfile`), falling back
  to `AuthService::currentDisplayName()` for the initial load.

#### 19. `client/src/viewmodels/user/ReaderViewModel.cpp` — page 0 crash
- **Critical (Bug 5)**: `pageText()` computed `(p - 1) % openers.size()`
  with `p = 0` (the old initial value in `ReaderService::openBook`),
  yielding `-1 % 5 = -1` in C++ → `openers.at(-1)` → UB/crash. Now
  clamps `p` to `>= 1` via `safePage` before computing the indices.

#### 20. `client/src/services/ReaderService.cpp` — 1-based page indexing
- **Critical (Bug 5) + Medium (Bug 39)**: Switched to 1-based page
  indexing throughout (`openBook` sets `m_currentPage = 1`; `setPage`
  validates `1..pageCount`; `firstPage = 1`; `lastPage = pageCount`).
  The QML reader was already 1-based everywhere (page grid sends
  `goToPage(index + 1)`, indicator shows "page / pageCount", prev/next
  check `page > 1` / `page < pageCount`). With 0-based indexing:
  indicator showed "0 / 100", `setPage(100)` failed (100 >= 100),
  and `pageText()` crashed via the off-by-one.

#### 21. `client/src/viewmodels/user/BookDetailViewModel.cpp` — cache, leak, sort
- **High (Bug 21)**: `totalReviewCount()` called
  `m_bookService->reviewsForBook(m_bookId)` on every QML re-evaluation —
  a blocking `GetBookDetails` round-trip — because it's bound to a
  Q_PROPERTY with `NOTIFY reviewsChanged`. Every scroll, tab switch, or
  `reviewsChanged` emission fired another network request. Now returns
  the count from the cached `m_reviews` list.
- **Medium (Bug 22)**: `_refreshRatingDistribution()` called
  `m_bookService->bookById(m_bookId)` when `m_book` was null — this
  allocates a NEW BookDto that was never deleted → memory leak on every
  call. Now uses `std::unique_ptr` to free the temporary.
- **Medium (Bug 23)**: The sort comparator had no case for `"newest"`
  (the default sort mode) — it fell through to `return false`, leaving
  reviews in server order. The `"oldest"` branch reversed the list,
  which only made sense if the server returned newest-first. Now uses
  `ReviewDto::createdAt()` for both `"newest"` and `"oldest"` so the
  sort is correct regardless of server order. Added
  `#include <memory>`.
- **High (Bug 32)**: Added `if (m_isBusy) return;` guards to
  `submitReview` and `deleteMyReview` to prevent the pending-op race
  condition (see UserViewModelBase below). For `loadBook`, instead of
  bailing out we force-cancel the in-flight timer (the user navigated
  to a new book, so the previous op is irrelevant).

#### 22. `client/include/services/ReviewDto.h` — createdAt() getter
- Added `const QDateTime& createdAt() const` so the BookDetailViewModel
  sort comparator can compare review timestamps chronologically for
  "newest"/"oldest" sort modes. Previously only string-formatted
  getters (`createdAtText`, `relativeTime`) existed.

#### 23. `client/src/viewmodels/user/ShelfViewModel.{h,cpp}` — count, createShelf emit
- **Medium (Bug 18)**: `count()` returned the unfiltered count while
  `shelves()` returned the filtered + sorted list. The count badge in
  the ShelvesPage header showed the total even when the user typed in
  the search field. Now counts the filtered list.
- **Medium (Bug 19)**: `createShelf` emitted `shelfCreated(id)`
  unconditionally, even when `createShelf` returned an empty string
  (server rejected the create). UserShell.qml's `onShelfCreated` showed
  a green "Shelf created" toast even on failure. Now only emits
  `shelfCreated` when the id is non-empty; emits a new
  `shelfCreateFailed(QString)` signal on failure.
- **High (Bug 32)**: Added `if (m_isBusy) return;` guard.

#### 24. `client/src/viewmodels/user/SearchViewModel.{h,cpp}` — debounce, favorites filter
- **Medium (Bug 34)**: `_refreshSuggestions` was called synchronously
  from `setQuery` — typing "hello" fired 4 blocking searches (he, hel,
  hell, hello), each freezing the UI. Added a separate 150ms debounce
  timer (`m_suggestionDebounceTimer`) for suggestions.
- **Low (Bug 35)**: `_scheduleSearch(int delayMs)` had `Q_UNUSED(delayMs)`
  and always used the timer's fixed 300ms interval. Now actually uses
  the `delayMs` argument.
- **Medium (Bug 36)**: The "Favorites" post-filter checked
  `!b->inWishlist()` — but `BookDto::inWishlist` is set at parse time
  from the server's SearchBooks response, which doesn't include wishlist
  membership. Every book was filtered out when "Favorites" was toggled.
  Now cross-references each book against `BookService::isInWishlist()`
  and updates the DTO's `inWishlist` flag before the filter check.

#### 25. `client/src/viewmodels/user/HomeViewModel.{h,cpp}` — refresh guard
- **Medium (Bug 38)**: `setBookService` connects
  `BookService::booksChanged → refresh()`. If a real-time `EvtBookAdded`
  fired during the 500ms wave-2 window, `booksChanged` → `refresh()` →
  another `_loadWave1` (which `qDeleteAll`s the previous DTOs while QML
  may still be binding to them) and restarted `m_wave2Timer`. Added an
  `m_refreshing` re-entrancy guard.

#### 26. `client/src/viewmodels/user/WishlistViewModel.cpp` — idempotent moveToCart
- **Medium (Bug 33)**: `moveToCart` / `moveSelectedToCart` called
  `toggleSaved(bookId)` — a TRUE toggle that re-ADDS the book if it's
  already removed (race, double-click). Now calls the new idempotent
  `LibraryService::removeFromWishlist()` (always sends
  RemoveBookFromShelf).

#### 27. `client/src/viewmodels/user/StudySessionViewModel.cpp` — EvtStudySync match
- **High (Bug 30)**: The `EvtStudySync` handler iterated `m_participants`
  and checked `p.value("userId").toString() == userId`. But
  `makeParticipant` never set a `"userId"` field — only `name`,
  `initials`, `color`, `page`, `pageCount`, `online`, `isHost`. So
  `p.value("userId")` was always empty, and the handler never updated
  any participant's page. Real-time group-reading page sync was broken
  at the participant-list level. Added a `userId` parameter to
  `makeParticipant`; `joinRoom` now passes both the server-reported
  userId per participant and the current user's own userId.

#### 28. `client/src/viewmodels/user/UserViewModelBase.{h,cpp}` — race condition
- **High (Bug 32)**: `beginAsync` returned early if `m_isBusy` was true,
  but subclasses set `m_pending = PendingOp::X` BEFORE calling
  `beginAsync`. If the user clicked "Submit review" then "Delete my
  review" within 450ms, the second call overwrote `m_pending` then
  `beginAsync` returned early. When the submit timer fired,
  `onAsyncReady` saw `m_pending == DeleteReview` and executed the DELETE
  path instead of SUBMIT — the user's review was deleted instead of
  saved. Fixed by making `beginAsync` return `bool`, and adding
  `if (m_isBusy) return;` guards to every subclass method that uses
  `m_pending`.

#### 29. `client/src/viewmodels/user/CartViewModel.cpp` + `LibraryViewModel.cpp`
- **High (Bug 32)**: Same `if (m_isBusy) return;` re-entrancy guard added
  to `CartViewModel::checkout` and `LibraryViewModel::createShelf`.

### C++ — User Services (4 files)

#### 30. `client/src/services/LibraryService.{h,cpp}` — createShelf cache, color, removeFromWishlist
- **Critical (Bug 6)**: `createShelf` did `m_libraryData = resp.payload;`
  after success — but the server's CreateShelf response only contains
  `{ "shelves": [...] }`. This OVERWROTE the cached `purchasedBookIds`,
  `savedBookIds`, and `lastOpenedBookId`. Subsequent calls to
  `purchasedCount()` returned 0, and the Profile/Library "My Books" tabs
  went empty until a manual `refresh()`. Now merges only the `shelves`
  field into the cache (mirroring `renameShelf`/`deleteShelf`/etc.).
- **High (Bug 7)**: `createShelf` silently dropped the `color` and
  `isPrivate` parameters (they were unnamed in the signature). Now
  includes them in the request payload.
- **Medium (Bug 33)**: Added `removeFromWishlist(bookId)` — idempotent
  wishlist removal (always sends RemoveBookFromShelf). Used by
  WishlistViewModel::moveToCart / moveSelectedToCart.

#### 31. `client/src/services/CartService.cpp` — setQuantity rollback
- **High (Bug 14)**: `setQuantity` sent `RemoveFromCart`; if that
  succeeded but the subsequent `AddToCart` (with the new quantity)
  failed (network blip, server 500), the function returned without
  restoring the item — the cart was missing the book, `m_cartData` was
  not updated, and no `cartChanged` was emitted. UI still showed the
  item until the next refresh, at which point it vanished. Now captures
  the original quantity before the remove, and rolls back by re-adding
  with the original quantity if the re-add fails.

#### 32. `client/src/services/BookService.{h,cpp}` — signal rename
- **High (Bug 15)**: Two signals shared the name `wishlistChanged`:
  `void wishlistChanged()` and `void wishlistChanged(const QString& bookId, bool inWishlist)`.
  `BookDetailViewModel::setBookService` used `static_cast` to connect to
  the 2-arg overload — that works in C++. But QML
  `Connections { onWishlistChanged: ... }` can only bind to the no-arg
  overload (QML picks the first overload by signature). So QML components
  that listened to `BookService.onWishlistChanged` only got the "something
  changed" notification, never the `(bookId, inWishlist)` delta. Renamed
  the 2-arg overload to `wishlistItemChanged`. Updated all 3 emission
  sites and the BookDetailViewModel connection (which can now use the
  plain function-pointer connect instead of static_cast).

## Bugs Fixed (summary)

| Severity | Count | Examples |
|----------|-------|----------|
| Critical | 6 | Enter shortcut, genre chip binding, ProfileVM displayName, ReaderService page indexing, LibraryService createShelf cache, NetworkService use-after-free |
| High | 14 | VM state reset, saved-credential leak, sendRequestAsync timeout callback, focus on step change, ValidationMessage wrap, NotificationsPage Connections, CartService setQuantity rollback, BookService wishlistChanged rename, BookDetailVM cache/leak/sort, HomeVM refresh guard, StudySession EvtStudySync, UserViewModelBase race, ShelfVM count/createShelf emit, SearchVM debounce/Favorites filter |
| Medium | 11 | skip double-completion, saveGenreSelection network, focus on step change, dead ternary, MouseArea swallow, Layout.fillWidth in auth pages, createShelf color/isPrivate, PdfReader Ctrl+F, SearchVM _scheduleSearch delayMs, WishlistVM moveToCart idempotency, ReaderService 1-based indexing |
| Low/Cosmetic | 8 | setRememberMe immediate persist, redundant anchors, splash animation, CartPage icon name, SearchVM Favorites filter, splash animation, dead ternary |

## Known issues NOT fixed (would require new server endpoints or major refactor)

- `AuthService::userExists` / `securityQuestionFor` / `verifySecurityAnswer`
  are still stubs (server has no GetSecurityQuestion command).
- `BookService::bySameAuthor` / `bySamePublisher` / `featuredPublishers` /
  `booksByPublisher` are still stubs (server has no such commands).
- `BookService::markHelpful` / `pinReview` / `flagReview` / `addReply` /
  `deleteReply` are still stubs.
- `LibraryService` shelf-management methods (duplicateShelf, setShelfColor,
  setShelfFavorite, setShelfPrivate, moveShelfUp/Down, reorderShelves)
  are still stubs.
- `NotificationService::deleteNotification` / `archiveNotification` are
  local-only (no server command).
- `ReaderService::m_pageCount` is still hardcoded to 100 (would need
  Qt6::Pdf + Qt6::PdfQuick linking + QPdfDocument).
- `NetworkService::sendRequest` still uses a blocking QEventLoop (would
  require a major refactor to make all callers async).
- `Layout.fillWidth: true` inside plain `Row` (not `RowLayout`) is
  widespread across 30+ locations in user QML files. Purely cosmetic
  (right-aligned buttons stay clumped to the left). Fixed in the auth
  pages where the impact was most visible; not fixed in every user page
  due to the sheer number of instances.

## Build & Run

Unchanged. See `README.md` and `BUILD.md`. Demo accounts: `admin`/`admin`,
`publisher1`/`publisher1`, `amir`/`amir1234`, `sara`/`amir1234`.

---

# BookCLUB V3 — Cart / Wishlist / Profile / Settings / Reader / Register Bug-Fix Batch

This batch addresses user-reported issues: cart page showing "1 item" but
blank items list + missing checkout button, wishlist add not persisting,
empty profile/settings pages, reader page needing polish, and the missing
genre-selection page after registration.

## Files Changed (12 total)

### Critical fixes

#### 1. `client/qml/user/CartPage.qml` — items list + checkout button missing
**Root cause**: The two-column `Row` (items list + order summary) inside
the Flickable's `Column` used `anchors.left: parent.left; anchors.right:
parent.right` on a child of a `Column`. Children of a `Column` should use
`width: parent.width` instead of horizontal anchors — the Column manages
vertical placement, and the anchor/width combination produced a zero-width
Row on Qt 6, making the items list and order summary (including the
checkout button) invisible. The header Row rendered fine because it was
simpler.
**Fix**: Replaced the anchor-based `Row` with a `RowLayout` using explicit
`width: parent.width - 2*padding` + `x: padding`. All inner columns now
use `Layout.fillWidth` / `Layout.preferredWidth`. Also added a
`toastRequested` signal + wired it in UserShell so checkout-failed errors
are shown to the user (previously the `onCheckoutFailed` handler was
empty).

#### 2. `src/server/handlers/LibraryRequestHandler.cpp` — wishlist not persisting
**Root cause**: Newly-registered users had no "Wishlist" shelf (only
seeded sample users had one). `BookService::toggleWishlist` queried
`GetLibrary`, found no Wishlist shelf, and fell back to a local-only
toggle that didn't persist — the user clicked the heart, it turned red,
then on next page load the book was gone.
**Fix**: The `handleGetLibrary` handler now auto-creates a default
"Wishlist" system shelf for users who don't have one. The shelf is
created before the response is sent, so the client always sees a real
shelfId to `AddBookToShelf` / `RemoveBookFromShelf` against.

#### 3. `client/qml/App.qml` — no genre selection page after register
**Root cause**: After registration, App.qml navigated to the Login page
(not GenreSelection). The genre selection only appeared on login if the
server returned `requiresGenreSetup=true`, but the server's
`SaveFavoriteGenres` handler never promoted the user from `Pending` to
`Active`, so `requiresFirstGenreSetup()` kept returning true on every
login — but the user had to log in first to see it.
**Fix**: New flow: Register → GenreSelection → (role shell or login).
- `App.qml` `onRegisterSuccess` now navigates directly to
  `GenreSelectionPage` (pre-filling the username from the registration
  form).
- `App.qml` `genreSelection onCompleted` routes based on
  `AuthService.isLoggedIn`: if logged in → role shell; if not → login.

#### 4. `src/server/handlers/AuthRequestHandler.cpp` — register + genre save
**Fix (register)**: `handleRegister` now auto-authenticates the user
(`client->setAuthenticated(true)`) and includes `requiresGenreSetup` +
`status` in the response. This makes the post-registration genre
selection flow work: the user picks genres, the client sends
`SaveFavoriteGenres` (which requires authentication), and the server
accepts it because the session is authenticated.
**Fix (genre save)**: `handleSaveFavoriteGenres` now promotes the user
from `Pending` → `Active` after saving genres. Previously the user
stayed `Pending` forever, so `requiresFirstGenreSetup()` kept returning
true on every subsequent login → the GenreSelection page kept appearing.

#### 5. `client/src/services/AuthService.cpp` — mirror session after register
**Fix**: `registerUser` now reads the registration response (userId,
username, displayName, role, requiresGenreSetup) and sets the client-side
session state (`_currentUserId`, `_currentUsername`, `_currentRole`,
`_requiresGenreSetup`). This mirrors the server's auto-authentication so
subsequent calls to `SaveFavoriteGenres` / `GetLibrary` / etc. succeed.

### High fixes

#### 6. `client/src/viewmodels/user/WishlistViewModel.cpp` — idempotent remove
**Fix**: `remove` and `removeSelected` now call the idempotent
`LibraryService::removeFromWishlist` instead of `toggleSaved` (which
would re-add the book if already removed by a concurrent action).

#### 7. `client/src/viewmodels/user/HomeViewModel.cpp` — heart live update
**Root cause**: When the user toggled wishlist on a BookCard, the heart
icon didn't visually update because the `BookDto::inWishlist` property
was set at parse time and never refreshed. HomeVM listened to
`booksChanged` but not `wishlistItemChanged`.
**Fix**: HomeVM now connects to `BookService::wishlistItemChanged` and
walks every cached book list, calling `b->setInWishlist(inWishlist)` on
the matching DTO so QML bindings re-evaluate.

#### 8. `client/src/viewmodels/user/SearchViewModel.cpp` — heart live update
**Fix**: Same as HomeVM — SearchVM now listens to
`wishlistItemChanged` and updates the matching BookDto in `m_results`.

#### 9. `client/qml/user/ProfilePage.qml` — empty profile
**Root cause**: `Component.onCompleted` only called
`loadGenresFromUser()` — it didn't refresh the library or force a
`userChanged` emission. For new users (0 purchases, no genres), the
page rendered with all zeros and no data.
**Fix**: Added `LibraryService.refresh()` + `root.viewModel.userChanged()`
on entry so all stats (purchases, genres, wishlist count) load fresh
data.

#### 10. `client/qml/user/SettingsPage.qml` — content touching edges
**Root cause**: The header `Row` and two-column body `Row` used
`anchors.leftMargin/rightMargin` without `anchors.left/right` — the
margins were silently ignored, and the content touched the screen edges.
On some Qt 6 builds this also caused layout glitches.
**Fix**: Replaced with explicit `x: root._horizontalPadding` +
`width: parent.width - 2 * root._horizontalPadding` on both Rows.

### Polish

#### 11. `client/qml/user/PdfReaderPage.qml` — reader polish
- **Bottom bar**: Replaced the 4px progress strip with a 44px status bar
  showing book title (left), "Page X of Y" (right), progress % (right,
  accent-colored), and a thin 3px progress strip at the very bottom.
- **Page header**: Added a book icon + book title + page-number badge
  (rounded pill) + divider.
- **Chapter title**: Added a chapter-title heading (derived from the
  TOC) above the page text, in accent color.
- **Nav footer**: Added prev/next page TextButtons below the page text.
- **Keyboard shortcuts**: Added Space/PgDown → next page, PgUp → prev
  page, Home/End → first/last page, Ctrl+B → toggle bookmark,
  Ctrl+= / Ctrl+- → zoom in/out.

#### 12. `client/qml/user/UserShell.qml` — wire CartPage toast
**Fix**: Added `onToastRequested` handler to the CartPage Component so
checkout-failed errors are forwarded to the global ToastManager.

## Bugs Fixed (summary)

| Bug | Root cause | Fix |
|-----|-----------|-----|
| Cart shows "1 item" but items list + checkout are blank | Row anchors conflict inside Column → zero-width Row | Rewrote with RowLayout + explicit width |
| Wishlist add doesn't persist | No Wishlist shelf for new users → local-only toggle fallback | Server auto-creates Wishlist shelf on GetLibrary |
| No genre selection page after register | Register → Login (not GenreSelection); server didn't auto-authenticate | Register → GenreSelection; server auto-authenticates + client mirrors session |
| GenreSelection keeps appearing on every login | SaveFavoriteGenres didn't promote Pending → Active | Server promotes user to Active after saving genres |
| Profile page is empty | No data refresh on entry | Added LibraryService.refresh() + userChanged() on Component.onCompleted |
| Settings content touches screen edges | anchors.leftMargin without anchors.left | Replaced with explicit x + width |
| Checkout failed silently | onCheckoutFailed handler was empty | Added toastRequested signal + wired in UserShell |
| BookCard hearts don't update after wishlist toggle | HomeVM/SearchVM didn't listen to wishlistItemChanged | Connected signal + update BookDto::inWishlist in-place |
| WishlistViewModel.remove uses toggleSaved (re-adds if already removed) | toggleSaved is a true toggle | Use idempotent removeFromWishlist |
| Reader bottom bar is a thin strip with no context | 4px progress bar with no labels | 44px status bar with title + page + % + thin strip |
| Reader page text has no chapter heading | Plain Text + divider | Added chapter title from TOC in accent color |

## Build & Run

Unchanged. See `README.md` and `BUILD.md`. Demo accounts: `admin`/`admin`,
`publisher1`/`publisher1`, `amir`/`amir1234`, `sara`/`amir1234`.

---

# BookCLUB V3 — Runtime Error Fixes + Signal/Slot Debug + Design Polish

This batch addresses runtime errors on launching the client, broken
signal/slot connections, binding-destroying patterns, and design polish
across the auth + user modules.

## Files Changed (21 total)

### Critical runtime error fixes

#### 1. `client/include/viewmodels/auth/GenreSelectionViewModel.h` + `.cpp` — genre selection always failed
**Root cause**: `username` was a plain C++ member with a non-Q_INVOKABLE
setter. QML's `_genreSelectionVM.username = AuthService.currentUsername`
wrote to a JS dynamic property instead of the C++ `m_username`, so
`_doSubmit()`'s `m_username.isEmpty()` guard always fired "Cannot save
preferences — no active session."
**Fix**: Added `Q_PROPERTY(QString username READ username WRITE setUsername
NOTIFY usernameChanged)`, a `username()` getter, and `emit
usernameChanged(m_username)` in `setUsername`.

#### 2. `client/qml/user/BookDetailPage.qml` — Details/Preview tabs broken
**Root cause**: The StackLayout closed prematurely at line 732 (before the
Details and Preview Columns). The Details/Preview content was always
visible below the StackLayout, and clicking the tabs did nothing.
**Fix**: Removed the premature `}` and added the proper StackLayout close
after the Preview Column. The Details and Preview Columns are now children
of the StackLayout — tab switching works.

#### 3. `client/qml/user/UserShell.qml` — `_app` reference error
**Root cause**: `onShareRequested` and `onToastRequested` in the
BookDetailPage Component called `_app.toast(...)` — but `_app` is the id
of App.qml's ApplicationWindow and is NOT visible from UserShell.qml.
Every share/toast from BookDetailPage threw `ReferenceError: _app is not
defined`.
**Fix**: Replaced `_app.toast(...)` with `_shell.toastRequested(...)`.

#### 4. `client/qml/user/WishlistPage.qml` + `ProfilePage.qml` — missing import
**Root cause**: Both files referenced `LibraryService` (a QML singleton
registered in `BookClub.Services`) but neither had `import BookClub.Services
1.0`. Every `LibraryService.xxx` reference threw `ReferenceError:
LibraryService is not defined`.
**Fix**: Added `import BookClub.Services 1.0` to both files.

#### 5. `client/qml/user/BookDetailPage.qml` — review delegate wrong property names
**Root cause**: The review delegate accessed `modelData.initial`,
`modelData.displayName`, and `modelData.currentUserHelpful` — none of which
exist on `ReviewDto`. The avatar initial showed "undefined", the reviewer
name showed "undefined", and the helpful toggle always showed the
un-helpful state.
**Fix**: `modelData.initial` → derived from `modelData.userDisplayName`;
`modelData.displayName` → `modelData.userDisplayName`;
`modelData.currentUserHelpful` → `modelData.userFoundHelpful`.

#### 6. `client/qml/App.qml` — loadSavedCredentials overwrites pre-filled username
**Root cause**: `loadSavedCredentials()` ran on every LoginPage creation,
overwriting the router-pre-filled username (e.g. the post-registration
hand-off). The per-instance `_credentialsLoaded` guard didn't help because
each `StackView.push` creates a fresh LoginPage with the flag reset.
**Fix**: Only load saved credentials when the VM is empty (no router
pre-fill happened).

### High-severity bug fixes

#### 7. `client/qml/auth/SuccessPage.qml` — invisible success icon
**Root cause**: The AppIcon's scale animation used `target: parent` — but
`parent` inside a SequentialAnimation is the SequentialAnimation itself,
which has no `scale` property. The icon stayed at scale 0.0 forever.
**Fix**: Added `id: _checkIcon` to the AppIcon and used `target:
_checkIcon`. Also removed the no-op `PauseAnimation { duration: 0 }`.

#### 8. `client/qml/components/selection/AppCheckbox.qml` — binding break
**Root cause**: `onClicked: { root.checked = !root.checked;
root.toggled(root.checked) }` — the `root.checked = ...` assignment
destroys any external binding on `checked` (e.g. `checked:
viewModel.rememberMe`). After the first click, programmatic VM changes no
longer updated the checkbox UI.
**Fix**: Changed to only emit `root.toggled(!root.checked)` and let the
consumer update the VM (which pushes back via the binding). Updated all 3
consumers (LoginPage, RegisterPage, ShelvesPage) to use `onToggled:
function(checked)` with the signal parameter. Also added
`root.forceActiveFocus()` for keyboard accessibility.

#### 9. `client/qml/components/inputs/InputField.qml` — binding break
**Root cause**: `onTextEdited: { root.text = text; root.textEdited(text)
}` — the `root.text = text` assignment destroys any external binding on
`text` (e.g. `text: viewModel.username`). After the first keystroke,
programmatic VM changes (loadSavedCredentials, reset, router pre-fill) no
longer updated the field.
**Fix**: Changed `onTextEdited` to only emit `root.textEdited(text)`. Added
a `Connections { target: root; onTextChanged: ... }` block that syncs
external `root.text` changes to the TextField (with cursor position
preservation). Applied the same fix to `PasswordField` and `SearchField`.
Also removed the no-op `onActiveFocusChanged: root.state = root.state`.

#### 10. `client/qml/auth/ForgotPasswordPage.qml` — focus timing
**Root cause**: `onStepChanged: if (step === "answer")
_answer.forceActiveFocus()` ran before the answer Column's `visible`
binding re-evaluated — `forceActiveFocus()` on an invisible item is a
no-op.
**Fix**: Wrapped in `Qt.callLater(function() { _answer.forceActiveFocus() })`.

#### 11. `client/src/viewmodels/auth/ResetPasswordViewModel.cpp` + `App.qml`
**Root cause**: ResetPasswordVM was never reset between visits — stale
password/confirm from a previous attempt bled into the new attempt. Also,
`setUsername` didn't call `_recomputeCanSubmit()`, so the Continue button's
enabled state was stale after the router set the username.
**Fix**: Added `_recomputeCanSubmit()` to `setUsername`. Added
`_resetPasswordVM.reset()` in App.qml before assigning username/token.

#### 12. `client/qml/user/ShelvesPage.qml` — missing parentheses
**Root cause**: `const all = BookService.bestsellers || []` — `bestsellers`
is a Q_INVOKABLE method, not a Q_PROPERTY. Referencing it without `()`
evaluates to `undefined`. The book-picker popup always showed an empty
list.
**Fix**: Changed to `BookService.bestsellers()`.

#### 13. `client/src/viewmodels/user/HomeViewModel.cpp` — use-after-free
**Root cause**: `_loadWave1`/`_loadWave2` used `qDeleteAll` on DTOs that
QML might still reference (e.g. a Repeater delegate that hasn't been
destroyed yet).
**Fix**: Changed to transfer ownership to QML GC via
`QQmlEngine::setObjectOwnership(o, QQmlEngine::JavaScriptOwnership)`.

#### 14. `client/src/viewmodels/user/BookDetailViewModel.cpp` — use-after-free
**Root cause**: `_refreshBook` did `delete m_book` before emitting
`bookChanged`. Any QML binding that read `viewModel.book.xxx` between the
delete and the emit would dereference a dangling pointer.
**Fix**: Allocate the new DTO first, transfer the old DTO to QML GC, then
assign. QML never observes a dangling pointer.

#### 15. `client/src/viewmodels/user/WishlistViewModel.cpp` + `.h` — blocking stats
**Root cause**: `totalValueText()`, `discountedCount()`,
`maxDiscountPercent()`, `maxDiscountBookId()`, `maxDiscountBookTitle()`
each fired a blocking `savedBooks()` network round-trip — 5 round-trips
per page render. Also `_filteredSorted` deleted DTOs that QML might still
reference.
**Fix**: Added a cache (`_refreshStatsCache()`) that computes all stats in
one pass and is invalidated when `booksChanged` fires. Changed
`_filteredSorted` to transfer ownership to QML GC instead of delete.

#### 16. `client/qml/user/PdfReaderPage.qml` — sidebar binding break
**Root cause**: `_sidebarOpen` was bound to `_sidebar.visible`, but
`_sidebar.visible` was set imperatively (`_sidebar.visible = false`),
which breaks the binding. After the first close, toggling `_cleanMode` or
`_hasBook` no longer re-showed the sidebar.
**Fix**: Changed to a dedicated `property bool _sidebarOpen: true` and
bound `_sidebar.visible` to it. Updated all close/open buttons + the
Ctrl+T shortcut.

#### 17. `client/qml/user/UserShell.qml` — timer running on all pages
**Root cause**: The 10s Home refresh timer ran on all pages, wasting
bandwidth and causing UI jank.
**Fix**: Gated on `running: _shell.activeRoute === "home"`.

### Medium/low fixes

#### 18. `client/include/services/AuthService.h` — isLoggedIn NOTIFY
**Fix**: `isLoggedIn` declared `NOTIFY currentRoleChanged` but reads
`_currentUsername`. Changed to `NOTIFY currentUsernameChanged`.

#### 19. `client/qml/components/buttons/PrimaryButton.qml` — dead code + double-scale
**Fix**: Removed `enabled: parent ? true : true` (dead ternary that
clobbered external `enabled:` bindings). Removed `scale: control.pressed
? 0.985 : 1.0` from the background (double-scaled with the `transform:
Scale`, causing jitter on press).

#### 20. `client/qml/auth/GenreSelectionPage.qml` — grid height
**Fix**: `height: Math.min(340, count * 50)` used total model count ×
wrong row height. Changed to `Math.ceil(count / 3) * cellHeight +
margins`.

#### 21. `client/qml/App.qml` — exit-dialog double-fire
**Fix**: The Shortcut called `_exitDialog.confirmed()` (which calls
`Qt.quit()`) AND then `Qt.quit()` again. Simplified to just set the guard
flag + `Qt.quit()`.

## Bugs Fixed (summary)

| Severity | Count | Examples |
|----------|-------|----------|
| Critical | 6 | GenreSelectionVM Q_PROPERTY, BookDetailPage StackLayout, UserShell _app ref, missing imports, ReviewDto properties, loadSavedCredentials |
| High | 11 | SuccessPage icon, AppCheckbox binding, InputField binding, ForgotPassword focus, ResetPasswordVM reset, ShelvesPage bestsellers(), HomeViewModel GC, BookDetailVM GC, WishlistViewModel cache, PdfReader sidebar, UserShell timer |
| Medium/Low | 4 | isLoggedIn NOTIFY, PrimaryButton dead code + double-scale, grid height, exit-dialog double-fire |

## Build & Run

Unchanged. See `README.md` and `BUILD.md`. Demo accounts: `admin`/`admin`,
`publisher1`/`publisher1`, `amir`/`amir1234`, `sara`/`amir1234`.

---

# BookCLUB V3 — Build+Run Fixes + Auth/User Polish (v4)

This batch addresses compile errors, runtime errors, signal/slot bugs,
and design polish across the auth + user modules. The goal is a version
that builds and runs cleanly in Qt Creator.

## Files Changed (22 total)

### Critical runtime error fixes

#### 1. `client/qml/user/GroupReadingCreateRoomDialog.qml` — `bestsellers` missing `()`
**Root cause**: `BookService.bestsellers` (no parentheses) — `bestsellers`
is a `Q_INVOKABLE` method, not a Q_PROPERTY. Referencing it without `()`
evaluates to `undefined` in QML, so the book picker was always empty.
**Fix**: Changed to `BookService.bestsellers()`.

#### 2. `client/include/services/NotificationService.h` + `.cpp` — `refresh()` private
**Root cause**: `refresh()` was declared `private void refresh() const`
with no `Q_INVOKABLE`. QML calls `service.refresh()` from
NotificationsPage.qml — threw `TypeError: Property 'refresh' of object
is not a function`.
**Fix**: Made `public Q_INVOKABLE void refresh()` (non-const). Also
changed `qDeleteAll` → JS ownership transfer to prevent use-after-free,
and added `emit notificationsChanged()` at the end.

#### 3. `client/src/viewmodels/user/BookDetailViewModel.cpp` — use-after-free
**Root cause**: `_refreshReviews`, `_refreshRatingDistribution`, and
`_refreshRelated` used `qDeleteAll` on DTOs that QML might still
reference (via Repeater delegates that haven't been destroyed yet).
**Fix**: Replaced `qDeleteAll` with `QQmlEngine::setObjectOwnership(o,
QQmlEngine::JavaScriptOwnership)` + `clear()` for `m_reviews`,
`m_ratingDist`, `m_relatedBooks`, `m_sameAuthor`, `m_samePublisher`.

### High-severity bug fixes

#### 4. `client/qml/components/inputs/InputField.qml` — clear button binding break
**Root cause**: The clear button did `root.text = ""; _textField.text =
""` which destroys the external binding on `text`.
**Fix**: Changed to only emit `root.textEdited("")`. The consumer clears
the VM, which updates `root.text`, and the Connections block pushes the
empty string to the TextField. Same fix for the `clear()` function.

#### 5. `client/qml/components/inputs/PasswordField.qml` + `SearchField.qml` — same
**Fix**: Same binding-break fix applied to `clear()` and the close button.

#### 6. `client/qml/components/selection/AppCheckbox.qml` — toggle() binding break
**Root cause**: `toggle()` did `checked = !checked` which destroys the
external binding on `checked`.
**Fix**: Changed to only emit `toggled(!checked)`.

#### 7. `client/qml/auth/RegisterPage.qml` — Popup positioning
**Root cause**: The security-question Popup's `y:
_securityQuestionCombo.y` was Column-local coords, but the Popup's
parent is `Overlay.overlay` (window-relative). The popup rendered at the
top-left of the window.
**Fix**: Changed to `y: _securityQuestionCombo.mapToItem(null, 0,
_securityQuestionCombo.height + Theme.space.xs).y` so the popup appears
directly below the combo regardless of scroll position.

#### 8. `client/qml/App.qml` — password not cleared after reset/logout
**Root cause**: After `onResetSuccess`, the LoginVM's in-memory password
wasn't cleared → the next LoginPage showed the old (now-invalid)
password. Same issue after `_performLogout` — privacy regression.
**Fix**: Added `_loginVM.password = ""` in both handlers.

#### 9. `client/qml/App.qml` + `ForgotPasswordPage.qml` — Qt 6 Connections syntax
**Root cause**: `onResetPasswordRequested: { /* use username, resetToken */ }`
and `onStepChanged: if (step === "answer") ...` used implicit parameter
injection. Qt 6's QML compiler warns and will remove this in a future
version.
**Fix**: Converted to explicit `function(params)` syntax:
`onResetPasswordRequested: function(username, resetToken) { ... }` and
`function onStepChanged(step) { ... }`.

#### 10. `client/src/viewmodels/user/ShelfViewModel.cpp` — sort values mismatch
**Root cause**: The QML SortDropdown sends `"manual"`, `"name"`,
`"recent"`, `"count"` — but the C++ comparator only handled `"name"`
and `"bookCount"`. Picking "Book count" didn't actually sort by book
count.
**Fix**: Extended the comparator to handle `"count"`, `"manual"`, and
`"recent"` (falling back to name sort for "recent" since ShelfDto has
no createdAt).

#### 11. `client/qml/user/BookDetailPage.qml` — reply state not reset
**Root cause**: `_replyingTo` and `_replyText` were page-level state.
When the user navigated from one book detail to another (same Loader,
just `loadBook(newId)`), the page wasn't re-created and the old reply
state persisted — the inline reply input showed beneath a
nonexistent review.
**Fix**: Added `Connections { target: root.viewModel; function
onBookChanged() { root._replyingTo = ""; root._replyText = "" } }`.

#### 12. `client/qml/user/SearchPage.qml` — price slider min>max + no debounce
**Root cause**: The Min/Max sliders had no clamp — dragging Min past Max
made Min exceed Max. Every `onMoved` also fired a blocking `search()`
round-trip — dragging triggered N searches.
**Fix**: Added clamping (`Math.min(value, maxPrice - 1)` /
`Math.max(value, minPrice + 1)`) and a 300ms debounce Timer that
collapses rapid drags into one search.

#### 13. Layout.fillWidth cleanup across 15 files
**Root cause**: `Item { width: 1; height: 1; Layout.fillWidth: true }`
inside plain `Row` containers. `Layout.fillWidth` is silently ignored
outside RowLayout/ColumnLayout/GridLayout — it generates qmllint
warnings and misleads readers into thinking the spacer expands.
**Fix**: Replaced 40 instances across 15 files with clean
`Item { width: 1; height: 1 }` spacers (no misleading Layout attached
property). For proper right-alignment, the enclosing `Row` would need
to be converted to `RowLayout` — noted as a future enhancement.

### Medium/low polish

#### 14. `client/qml/components/branding/SecurityBadge.qml` — invisible badge
**Fix**: Changed background from 85% white (invisible on white hero
panel) to `accentSoft` with an `accent` border.

#### 15. `client/qml/layouts/AuthLayout.qml` — conflicting width + anchors
**Fix**: Removed `anchors.horizontalCenter` from the `_formSlot` Column
(no-op when `width: parent.width` is set, generates warnings).

#### 16. `client/qml/auth/GenreSelectionPage.qml` — missing LoadingOverlay
**Fix**: Added a `LoadingOverlay` with "Saving your preferences…" label
so the user gets visual feedback during the synchronous
`saveGenreSelection` network call.

#### 17. `client/src/services/AuthService.cpp` — misleading Q_UNUSED
**Fix**: Removed `Q_UNUSED(username)` from `saveGenreSelection` — the
parameter IS used as a fallback when `_currentUsername` is empty.

#### 18. `client/qml/publisher/PublisherProfilePage.qml` — brace imbalance
**Fix**: Added 2 missing closing braces at the end of the file
(pre-existing issue, not in auth/user scope but would block the build).

#### 19. int → Text.text String() wrapping (4 files)
**Fix**: Wrapped int expressions in `String(...)` when assigned to
`Text.text` to silence Qt 6 "Unable to assign int to QString" warnings.

#### 20. `client/src/viewmodels/user/SearchViewModel.cpp` — missing include
**Fix**: Added `#include <QQmlEngine>` (used by
`QQmlEngine::setObjectOwnership`).

## Build & Run

Unchanged. See `README.md` and `BUILD.md`. Demo accounts: `admin`/`admin`,
`publisher1`/`publisher1`, `amir`/`amir1234`, `sara`/`amir1234`.

### Qt Creator setup
1. Open `CMakeLists.txt` in Qt Creator.
2. Select a Qt 6.5+ kit (e.g. MinGW 64-bit or MSVC 2022 64-bit).
3. Configure → Build → Run.
4. Start the server first: `./bin/BookClubServer -p 8080`
5. Run the client: `./bin/BookClubClient`
