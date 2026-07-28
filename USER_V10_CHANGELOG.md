# BookClub — v10 User Panel: Settings Removal + Profile Polish

## What Changed

### 1. Settings Page Removed

The project doesn't need a separate settings page. All settings functionality (dark mode toggle, sign out) has been moved into the profile page's "Account" section.

**Files modified:**
- `Sidebar.qml` — removed the Settings NavItem (only Profile + Sign out remain in the footer)
- `UserShell.qml` — removed `SettingsViewModel` instantiation, the `"settings"` route from `_routeMeta`, the `"settings"` entry from `_componentMap`, and the entire `_settingsComp` Component block
- `qml.qrc` — removed `<file>user/SettingsPage.qml</file>` entry

**Dead code left in place (harmless):**
- `SettingsViewModel.h/.cpp` and `SettingsPage.qml` still exist on disk and in CMakeLists.txt, but are no longer referenced by any QML. They compile but are never instantiated. This avoids breaking the build while keeping the code available if needed later.

### 2. Profile Page Rewritten (Clean + Polished)

The old profile page (849 lines) was messy — sections overlapped, layout used fragile `Row`/`Column` with manual width arithmetic, and the settings section duplicated the (now-removed) settings page.

The new profile page (520 lines) uses `ColumnLayout` + `RowLayout` throughout for robust, resizable layout. Six clear sections in a single vertical scroll:

| # | Section | Content |
|---|---------|---------|
| 1 | **Identity header** | Large avatar + display name + @username + favorite-genres chip + purchases count |
| 2 | **Account information** | Display name editor with Save button |
| 3 | **Favorite genres** | 1–3 genre selection grid with Save/Reset |
| 4 | **Change password** | 3 password fields (current/new/confirm) with strength meter + Update button |
| 5 | **Purchase history** | List of past orders with icon, title, date, total |
| 6 | **Account** | Dark mode toggle + Sign out button |

### 3. Signals & Slots — All Wired to Server

Every action in the profile page connects to the `ProfileViewModel` which talks to the server via `UserService`:

| Action | QML → VM | VM → Server | Server → DB |
|--------|----------|-------------|-------------|
| Save display name | `viewModel.saveProfile()` | `UpdateProfile` command | `users` table |
| Save favorite genres | `viewModel.saveGenres()` | `SaveFavoriteGenres` command | `user_genres` table |
| Change password | `viewModel.changePassword()` | `ChangePassword` command | `users.password_hash` column |
| Refresh | `viewModel.refresh()` | `GetCurrentUser` + `GetPurchasedBooks` | `users` + `orders` tables |
| Library sync | `LibraryService.refresh()` | `GetLibrary` | `shelves` + `shelf_books` tables |

**Real-time updates:**
- `Connections { target: LibraryService; onLibraryChanged / onWishlistChanged }` → calls `viewModel.userChanged()` → triggers `NOTIFY userChanged` → QML re-reads all properties → UI refreshes
- `viewModel.saveProfile()` → server updates `users` table → `NOTIFY userChanged` → display name updates in sidebar + topbar immediately

### 4. InputField Text Binding Fix

The profile page's display name InputField now has `onTextEdited: function(newText) { _displayNameField.text = newText; ... }` so the Save button's `enabled: _displayNameField.text.length > 0` check works correctly. (Same fix as the publisher pages in v10.)

## Files Changed

| File | Change |
|------|--------|
| `client/qml/components/navigation/Sidebar.qml` | Removed Settings NavItem |
| `client/qml/user/UserShell.qml` | Removed SettingsViewModel, settings route, settings component |
| `client/qml/qml.qrc` | Removed SettingsPage.qml entry |
| `client/qml/user/ProfilePage.qml` | **Full rewrite** — clean 6-section layout, all signals wired |

## How to Apply

1. Unzip on top of your project root
2. **Build → Clean → Rebuild All**
3. Log in as a user (e.g., `amir` / `amir1234`)
4. Click Profile in the sidebar — no more Settings item
5. Verify: edit display name → Save → name updates in sidebar/topbar
6. Verify: pick genres → Save → genres persist
7. Verify: change password → Update → works
8. Verify: dark mode toggle works
9. Verify: sign out works
