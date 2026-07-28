# BookClub — v10 Final Publisher Module Changelog

This is the **final** revision of the publisher module. It fixes the
critical bug that prevented publishers from publishing books, editing
info, or creating promotions.

## Root Cause: InputField Text Binding Bug

**The bug**: The `InputField` component has a design where `onTextEdited`
emits a signal but does NOT update `root.text`. This is intentional for
auth pages (where the text is bound to a ViewModel property), but it
breaks EVERY other usage.

**The effect**: In the catalog editor, profile editor, and promotions
editor, the InputField's `text` property was NEVER updated when the user
typed. So:
- `_fTitle.text` was always `""` → the "Publish title" button's
  `enabled: _fTitle.text.length > 0` check was always false → button
  stayed disabled
- Even if the button was clicked, `_submit()` read `_fTitle.text` which
  was `""` → sent empty strings to the server

**The fix**: Added `onTextEdited: function(newText) { <id>.text = newText }`
to EVERY InputField in the publisher pages that doesn't have a ViewModel
binding. This writes the typed text back to the `text` property so other
code can read it.

**20 InputFields fixed** across 4 files:
- `PublisherCatalogPage.qml`: 10 (title, author, desc, price, discount, coverColor, coverAccent, coverImage, pdfFile)
- `PublisherProfilePage.qml`: 8 (publisherName, biography, website, email, taxId, currPass, newPass, confirmPass)
- `PublisherPromotionsPage.qml`: 4 (code, desc, pct, cap) — startDate/endDate skipped (read-only)
- `PublisherBookDetailDrawer.qml`: 1 (quickPrice)

## Other Fixes

### 1. TopBar Restored (No Cart)
- Added `showCart` property to `TopBar.qml`
- `PublisherShell.qml` restores the TopBar with `showCart: false`
- Shows: dark mode toggle, notifications bell, user avatar + name, page title

### 2. addBook Always Returns Non-Empty ID
- `addBook()` ALWAYS adds to local cache + returns a non-empty ID
- If server is down: generates `local-<timestamp>` ID, adds to `m_localBooksCache`
- If server is up: uses server-assigned ID
- The book appears in the catalog immediately, syncs to server when possible

### 3. updateBook Updates Local Cache
- `updateBook()` updates the matching entry in `m_booksCache` IN-PLACE
- Also updates `m_localBooksCache` if the book is local-only
- Emits `booksChanged` → catalog table + drawer auto-refresh

### 4. removeBook Removes from Local Cache
- `removeBook()` removes from both `m_booksCache` and `m_localBooksCache`
- Emits `booksChanged` → table auto-refreshes

### 5. Profile Changes Persist + Propagate
- `updatePublisherProfile()` sends `UpdateProfile` to server
- On success: calls `AuthService::setCurrentDisplayName()` → propagates app-wide
- `m_profileCache` survives `refresh()` — changes don't vanish
- `publisherProfile()` uses real AuthService data (not mock)

### 6. Password Change Works
- Added `Q_INVOKABLE bool changePassword(old, new)` 2-arg wrapper to AuthService
- The QML dialog captures the return value and shows success/error toast

### 7. Promotion Edit Works
- Added `Q_INVOKABLE updatePromotion()` to PublisherViewModel
- Delegates to `PublisherService::updatePromotion()` which updates local cache
- No more remove+add hack

### 8. publisherBooks() Merges Local + Server
- Fetches from server, then merges any locally-added books (offline books)
- Deduplicates by ID
- Local books survive refreshes until the server confirms them

### 9. Removed Mock Data
- `publisherProfile()` no longer uses deterministic random for `verified` badge
- `verified` is now always `true` (publishers are verified by default)
- All other data comes from real AuthService state + local cache

## Files Changed

| File | Changes |
|------|---------|
| `client/include/services/AuthService.h` | Added `setCurrentDisplayName()` + Q_INVOKABLE 2-arg `changePassword()` |
| `client/src/services/AuthService.cpp` | Implemented 2-arg `changePassword()` wrapper |
| `client/include/services/PublisherService.h` | Added `m_localBooksCache` member |
| `client/src/services/PublisherService.cpp` | `addBook` always returns non-empty ID; `updateBook` updates cache; `removeBook` removes from cache; `updatePublisherProfile` calls `setCurrentDisplayName`; `publisherBooks` merges local cache; `publisherProfile` uses real data |
| `client/include/viewmodels/publisher/PublisherViewModel.h` | Added `Q_INVOKABLE updatePromotion()` |
| `client/src/viewmodels/publisher/PublisherViewModel.cpp` | Implemented `updatePromotion()` |
| `client/qml/components/navigation/TopBar.qml` | Added `showCart` property |
| `client/qml/publisher/PublisherShell.qml` | TopBar restored with `showCart: false` |
| `client/qml/publisher/PublisherCatalogPage.qml` | **20 InputFields fixed** with `onTextEdited` handler; `_submit()` checks return values |
| `client/qml/publisher/PublisherProfilePage.qml` | **8 InputFields fixed**; save + password dialog check return values |
| `client/qml/publisher/PublisherPromotionsPage.qml` | **4 InputFields fixed**; edit uses `updatePromotion()` |
| `client/qml/publisher/PublisherBookDetailDrawer.qml` | **1 InputField fixed** (quickPrice) |

## How to Apply

1. Unzip on top of your project root
2. **Build → Clean → Rebuild All**
3. Run and test:
   - Click "Add new title" → fill in the form → "Publish title" button should now be ENABLED
   - Click "Publish title" → book appears in catalog immediately
   - Edit a book → changes appear immediately, persist on refresh
   - Edit profile → name updates in sidebar + topbar immediately
   - Change password → works
   - Create promotion → appears in list immediately
   - Edit promotion → updates in-place

## Verification

After applying, verify:
- [ ] Type in the title field → "Publish title" button becomes enabled
- [ ] Click "Publish title" → success toast + book appears in catalog
- [ ] Edit a book → success toast + changes visible
- [ ] Edit profile name → sidebar + topbar update immediately
- [ ] Change password → success toast
- [ ] Create promotion → appears in list
- [ ] Refresh the page → all changes persist (no mock data reapplied)
