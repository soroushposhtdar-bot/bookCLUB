# Bug Fixes Applied — Round 2

This document describes the 8 bugs that were reported and fixed in this
round of changes. All fixes were verified by building the project with
Qt 6.7 on Linux (gcc_64) and running the smoke test (`scripts/smoke_test.py`).

## Bug 1: Settings and Profile pages were empty / not wired to the app

### Settings page
**File:** `client/qml/user/SettingsPage.qml`

The Settings page had a structural QML bug: the **Storage** and **About**
sections were accidentally nested INSIDE the **Account** section's Card
(rather than being siblings of the Account section). This meant:

- When the user clicked "Storage" or "About" in the sidebar, nothing
  appeared (the sections were inside the Account Card, which was only
  visible when `activeSection === 5`).
- The "Sign out" SettingToggleRow was misplaced as a sibling of the
  Column-inside-Card instead of being inside it.

**Fix:** Restructured the brace nesting so all 8 sections (General,
Appearance, Notifications, Privacy, Reading, Account, Storage, About)
are direct children of the content Column at the same depth. Each
section now has `visible: root.viewModel.activeSection === N` and
displays correctly when its sidebar item is clicked.

### Profile page
**Files:**
- `client/include/viewmodels/user/ProfileViewModel.h`
- `client/src/viewmodels/user/ProfileViewModel.cpp`
- `client/qml/user/ProfilePage.qml`

The Profile page appeared empty because `purchaseHistory()` and
`purchaseCount()` were Q_PROPERTY reads that each triggered a blocking
`GetPurchasedBooks` network round-trip. The QML Repeater model
`root.viewModel.purchaseHistory` re-evaluated the property on every
binding refresh, firing multiple blocking calls that froze the UI and
often returned empty before the server responded.

**Fix:**
- Added a `Q_INVOKABLE refresh()` method to `ProfileViewModel` that
  fetches the purchase history ONCE and caches it in
  `m_purchaseHistory`.
- Changed `purchaseHistory()` and `purchaseCount()` to return the
  cached list instead of making a network call on every read.
- Updated `ProfilePage.qml`'s `Component.onCompleted` to call
  `viewModel.refresh()` instead of `loadGenresFromUser()` +
  `userChanged()`.

---

## Bug 2: Could not buy books in the cart — added in-app payment

**File:** `client/qml/user/CartPage.qml`

The "Proceed to checkout" button called `viewModel.checkout()` directly,
which sent a `Checkout` command to the server but provided no payment
UI. The user had no way to "pay" for the books.

**Fix:** Added a full in-app payment dialog (`Popup`) that:
- Shows the order total.
- Collects cardholder name, card number, expiry (MM/YY), and CVC.
- Validates each field (card number 13-19 digits, expiry MM/YY format,
  CVC 3-4 digits, name ≥ 2 chars).
- Disables the "Pay" button until all fields are valid.
- Shows a processing state while the checkout request is in flight.
- Closes on success (the `checkoutSuccessRequested` signal routes the
  user to the library).
- Shows an inline error message on failure.
- Includes a trust footer noting this is a demo (test card:
  4242 4242 4242 4242).

The "Proceed to checkout" button now opens this dialog instead of
calling `checkout()` directly.

**Also:** Added the `credit_card` icon to `AppIcon.qml` (was missing).

---

## Bug 3: Deleted the Share button on the book detail page

**File:** `client/qml/user/BookDetailPage.qml`

The right-side sticky action panel had a "Wishlist + Share" row with
two buttons side-by-side. The Share button opened a clipboard-copy
dialog that wasn't useful.

**Fix:** Removed the Share button. The Wishlist button now takes the
full width of the panel.

---

## Bug 4: Book detail tabs (Reviews / Details / Preview) didn't display content

**Files:**
- `client/qml/user/BookDetailPage.qml` — TabBar binding fix
- `client/src/viewmodels/user/BookDetailViewModel.cpp` — ReviewDto ownership fix

Two bugs prevented the tab content from displaying:

### Tab highlight didn't follow the selected tab
The `TabBar`'s `activeIndex` was hardcoded to `0`, so the visual
highlight never moved when the user clicked a different tab. The
`onTabSelected` handler updated `_stack.currentIndex` (so the
StackLayout DID switch content), but the user couldn't see which tab
was active.

**Fix:** Bound `activeIndex: _stack.currentIndex` so the highlight
follows the selected tab.

### Reviews appeared empty due to QML garbage collection
The `ReviewDto` objects in `m_reviews` were not given C++ ownership,
so QML's garbage collector could collect them while the Repeater was
still iterating over the model. This produced empty review cards or
`TypeError: cannot read property of null`.

**Fix:** Added `QQmlEngine::setObjectOwnership(r, QQmlEngine::CppOwnership)`
for each ReviewDto added to `m_reviews`.

---

## Bug 5: Could not switch between notification topics

**Files:**
- `client/include/viewmodels/user/NotificationsViewModel.h`
- `client/qml/user/NotificationsPage.qml`

The category tabs (All / Purchase / Review / Discount / etc.) didn't
switch the filtered list because `setActiveCategory` was the WRITE
function of a Q_PROPERTY but was NOT marked `Q_INVOKABLE`. QML's
`viewModel.setActiveCategory(key)` call silently failed.

**Fix:**
- Marked `setActiveCategory` and `setSearchQuery` as `Q_INVOKABLE` so
  QML can call them directly.
- Updated `NotificationsPage.qml`'s `Component.onCompleted` to reset
  to the "all" category on entry and refresh the service.

---

## Bug 6: Reader sidebar couldn't be reopened after closing

**File:** `client/qml/user/PdfReaderPage.qml`

The sidebar toggle button was only visible when the sidebar was closed
(`visible: !root._sidebarOpen`). But the toolbar's `anchors.left` was
bound to `root._sidebarOpen ? _sidebar.right : parent.left` — and an
invisible Item still has its original geometry, so the toolbar stayed
anchored to the sidebar's right edge (x=260) even when the sidebar was
hidden. This pushed the toggle button to the middle of the screen
where the user couldn't find it.

**Fix:**
- Changed the toolbar's `anchors.left` to `parent.left` unconditionally
  and added a `leftMargin` that matches the sidebar width when the
  sidebar is visible. Same fix applied to the page Flickable.
- Made the sidebar toggle button ALWAYS visible (icon swaps between
  `menu` and `menu_open` to indicate state) so the user can always
  reopen the sidebar.

---

## Bug 7: Wishlist heart didn't stay red across all pages

**Files:**
- `client/include/services/BookService.h`
- `client/src/services/BookService.cpp`

The `BookDto::inWishlist` flag was only set on the BookDetailViewModel's
book (via `wishlistItemChanged`). All other BookDto instances (on the
Home page, Search page, etc.) had `inWishlist = false` even when the
book was in the user's wishlist, so the heart icon appeared white.

**Fix:**
- Added a private `ensureWishlistCache()` helper that fetches the
  user's Wishlist shelf from the server and caches the book IDs in
  `m_wishlist`.
- Updated `parseBookList()` (used by every catalog query) to call
  `ensureWishlistCache()` and set `dto->setInWishlist(m_wishlist.contains(dto->id()))`
  on every BookDto.
- Updated `bookById()` (used by BookDetailPage) to do the same.
- Updated `toggleWishlist()` to emit `booksChanged` + `catalogChanged`
  after a successful toggle, so the Home + Search pages re-fetch their
  book lists with the updated `inWishlist` flag.
- Made `parseBookList` non-static (was `static`) so it can call the
  non-static `ensureWishlistCache()` helper.

Now when a user adds a book to the wishlist from ANY page, the heart
on every BookCard across the app turns red immediately (and stays red
until the user removes the book from the wishlist).

---

## Bug 8: Removed "Skip for now" button on genre selection after registration

**File:** `client/qml/auth/GenreSelectionPage.qml`

The "Skip for now" button allowed users to bypass the genre selection
step after registration, which violated the project spec ("the user
must pick 1-3 favourite genres on first login").

**Fix:** Removed the "Skip for now" SecondaryButton. The "Continue"
PrimaryButton now takes the full width and is the only action. The
button is disabled until the user selects at least 1 genre
(`canSubmit` requires `selectedCount >= minSelection`).

---

## How to verify

```bash
# 1. Build
cd bookclub
mkdir -p build && cd build
cmake ..
cmake --build . -j8

# 2. Run the smoke test (verifies the app launches cleanly)
cd ..
python3 scripts/smoke_test.py

# 3. Run the server-side e2e test (verifies the server works)
./build/bin/BookClubServer -p 8080 &
python3 scripts/e2e_test.py
kill %1

# 4. Run the app interactively
./scripts/run_server.sh   # in one terminal
./scripts/run_client.sh   # in another terminal
# Log in with admin/admin, publisher1/publisher1, or amir/amir1234
```

### Manual verification checklist

1. **Settings page** — click each sidebar item (General, Appearance,
   Notifications, Privacy, Reading, Account, Storage, About). Each
   section should display its content.
2. **Profile page** — should show the user's display name, avatar,
   purchase count, favorite genres, and purchase history.
3. **Cart checkout** — add a book to the cart, click "Proceed to
   checkout", fill in the payment form (test card: 4242 4242 4242
   4242, any expiry, any CVC), click "Pay". The dialog should close
   and the user should be routed to the library.
4. **Book detail tabs** — open a book, click each tab (Overview,
   Reviews, Details, Preview). The tab highlight should follow the
   selected tab and the content should switch.
5. **Notification topics** — open the notifications page, click each
   category chip. The list should filter to that category.
6. **Reader sidebar** — open a purchased book in the reader, click the
   "menu_open" icon to close the sidebar, then click the "menu" icon
   to reopen it. The sidebar should reappear.
7. **Wishlist heart** — add a book to the wishlist from the Home page.
   The heart should turn red. Navigate to Search — the heart on that
   book should also be red. Open the book detail — the Wishlist button
   should say "Saved".
8. **Genre selection** — register a new account. The genre selection
   page should show only a "Continue" button (no "Skip for now"). The
   button should be disabled until at least 1 genre is selected.
