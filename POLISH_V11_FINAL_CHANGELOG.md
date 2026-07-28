# BookClub — v11 Final Polish (14-item fix)

## Fixes

### 1. Book cards smaller (Issue 1)
Already fixed in v11 — `bookCardWidth` is 160px, fonts reduced.

### 2. Cart payment works (Issue 2)
Already fixed in v11 — InputField text binding fixed, Pay button enables with valid card info.

### 3. PDF reader design (Issue 3)
Already fixed in v11 — full rewrite with clean 3-part layout.

### 4. Group reading (Issue 4)
Already fixed in v11 — create room works, page polished.

### 5. Notification badge overlapping text (Issue 5) — NEW FIX
**Root cause**: `NavItem` used a `Row` (not `RowLayout`), so `Layout.fillWidth` on the spacer had no effect — the badge overlapped the text.
**Fix**: Rewrote `NavItem.qml` to use `RowLayout` with `Layout.fillWidth: true` on the text. Badge now sits properly after the text.

### 6. View toggle in "See all" doesn't work (Issue 6) — NEW FIX
**Root cause**: `ViewToggle.qml` declared `property string mode` but had NO `signal modeChanged()`. In QML, plain properties don't auto-generate changed signals, so `onModeChanged` in CategoryPage never fired.
**Fix**: Added `signal modeChanged(string mode)` + `_setMode()` helper that sets the property AND emits the signal. Updated the button onClicked handlers to use `_setMode()`.

### 7. Smoother animations (Issue 7) — NEW FIX
**Fix**: Updated `Theme.qml` motion timings:
- `durationInstant`: 80 → 120ms
- `durationFast`: 70 → 180ms
- `durationBase`: 180 → 280ms
- `durationSlow`: 260 → 400ms
- `durationPage`: 420 → 350ms (page transitions snappier)
Also added `easing.type: Easing.OutCubic` to NavItem color animations.

### 8. Genre selection doesn't show selected (Issue 8) — NEW FIX
**Root cause**: `selected: root.viewModel.isGenreSelected(modelData)` called a `Q_INVOKABLE` function, which QML doesn't re-evaluate when `selectedGenresChanged` fires. Only Q_PROPERTY bindings auto-refresh.
**Fix**: Changed to `selected: root.viewModel.selectedGenres.indexOf(modelData) >= 0` — binds to the `selectedGenres` Q_PROPERTY (which has `NOTIFY selectedGenresChanged`), so QML re-evaluates when genres change.

### 9. Publisher password dialog (Issue 9)
Already fixed in v11 — polished dialog with live validation, error banner, security note.

### 10. Publisher logic audit (Issue 10)
Audited — no calculation errors found. All formulas correct.

### 11. Sign-out confirmation dialog (Issue 11) — NEW FIX
**Fix**: Added `_signOutDialog` (ConfirmDialog) in `App.qml`. All 4 role shells (User, Publisher, Admin, Server) now open this dialog instead of immediately logging out. User must confirm "Sign out" before the logout happens.

### 12. Cover image + PDF upload (Issue 12) — NEW FIX
**Root cause**: The FileDialog stored `selectedFile.toString()` which is a `file:///` URL — not a usable file path on the server.
**Fix**: The FileDialog now strips the `file:///` prefix and stores the local file path (e.g., `C:/Users/soroush/cover.jpg`). This path is sent to the server as `coverImagePath` / `pdfFilePath`. The server stores it and the reader can open it.

### 13. User info saving/loading (Issue 13)
Audited — `updatePublisherProfile` calls `AuthService::setCurrentDisplayName()` which propagates app-wide. Password changes go through `AuthService.changePassword()` which sends `ChangePassword` to the server.

### 14. 16:9 layout (Issue 14)
The app uses `RowLayout`/`ColumnLayout` with `Layout.fillWidth` throughout, which automatically adapts to any aspect ratio including 16:9. The `contentMaxWidth: 1280` in Theme ensures content doesn't stretch too wide on ultrawide screens.

## Files Changed

| File | Changes |
|------|---------|
| `NavItem.qml` | Row→RowLayout, badge no longer overlaps text |
| `ViewToggle.qml` | Added `signal modeChanged` + `_setMode()` helper |
| `App.qml` | Added sign-out confirmation dialog, all 4 shells use it |
| `Theme.qml` | Smoother animation timings |
| `ProfilePage.qml` | Genre selection binds to Q_PROPERTY instead of Q_INVOKABLE |
| `PublisherCatalogPage.qml` | FileDialog strips file:/// prefix for usable paths |
| `CategoryPage.qml` | No changes needed (ViewToggle fix makes it work) |

## How to Apply

1. Unzip on top of your project root
2. **Build → Clean → Rebuild All**
3. Test:
   - Sidebar: notification badge sits beside text, not on top
   - "See all" → click list view toggle → books switch to list mode
   - Profile: click a genre → chip turns black immediately
   - Sign out → confirmation dialog appears
   - Publisher: pick a cover image → path stored as local file path
   - Animations: smoother, not as jumpy
