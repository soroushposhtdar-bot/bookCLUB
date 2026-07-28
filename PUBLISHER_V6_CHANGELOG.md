# BookClub — v6 Polish Changelog

This revision fixes **7 issues** across the publisher module and the
entire app: Persian-style numbers, broken catalog search, dead CSV code,
chart polish, profile header layout, notification "Clear read" bug,
hover animations, and half-rounded corners.

## 1. Persian Numbers + Device Font Issue (GLOBAL FIX)

**Problem**: On systems with a Persian locale, all numbers in the app
rendered as Persian-style digits (۰۱۲۳۴۵۶۷۸۹) instead of English (0-9).
The app also fell back to device-local fonts, causing inconsistent
typefaces across platforms.

**Root cause**: Qt uses the system default locale for number formatting.
`Qt.locale()` in QML returns this default locale. On a Persian-locale
system, `toLocaleString()` produces Persian digits.

**Fix** (2 files):

### `client/main.cpp`
- Added `QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates))` BEFORE `QGuiApplication` is created. This forces ALL number rendering in the entire app (QML Text elements, `toLocaleString()`, chart labels, KPI values) to use English digits regardless of the user's system locale.
- Bundled **DejaVu Sans** font (Regular + Bold + Mono) via `fonts.qrc`.
- Registered DejaVu Sans via `QFontDatabase::addApplicationFont()` and set it as the application-wide default font via `QGuiApplication::setFont()`. This ensures the app uses the bundled font instead of falling back to device-local fonts.

### `client/resources/fonts.qrc`
- Added `DejaVuSans.ttf`, `DejaVuSans-Bold.ttf`, `DejaVuSansMono.ttf` to the resource file.

### `client/resources/fonts/` (NEW DIRECTORY)
- Copied 3 DejaVu Sans font files from the system.

### `client/qml/theme/Theme.qml`
- Changed `font.family` from `"Inter, SF Pro Display, SF Pro Text, Segoe UI, Roboto, Helvetica Neue, Arial"` to `"DejaVu Sans"`.
- Changed `font.familyMono` from `"JetBrains Mono, SF Mono, Menlo, Consolas, monospace"` to `"DejaVu Sans Mono"`.
- The icon font (`Material Symbols Outlined`) is unchanged.

## 2. Catalog Search Button Not Working

**Problem**: Typing in the catalog search field did not filter the book list.

**Root cause**: In `PublisherCatalogPage.qml`, the `SearchField.onTextEdited` handler used `page._searchText = text` where `text` refers to the SearchField's `text` **property** (which is bound to `page._searchText` and hasn't been updated yet at handler time) — NOT the signal parameter. The signal parameter is named `newText`. So `page._searchText = text` was setting it to its own current value — a no-op.

**Fix** (`PublisherCatalogPage.qml`):
```qml
// BEFORE (broken):
onTextEdited: { page._searchText = text; ... }

// AFTER (fixed):
onTextEdited: function(newText) { page._searchText = newText; ... }
```
Same fix applied to `onAccepted`.

## 3. Removed "Copy CSV" Buttons + Dead Code

**Problem**: The "Copy CSV" buttons on the Sales page were unnecessary and cluttered the UI.

**Fix** (`PublisherSalesPage.qml`):
- Removed the `_copyCsv()` function (13 lines)
- Removed the "Copy CSV" TextButton from the monthly revenue chart footer
- Removed the "Copy CSV" TextButton from the daily revenue chart footer
- Cleaned up the `Item { Layout.fillWidth: true; height: 1 }` spacers that were only there to push the CSV buttons to the right

## 4. Chart Polish

**Problem**: Charts lacked Y-axis value labels, numbers weren't formatted with commas, and grid lines were too faint.

**Fix** (`PublisherSalesPage.qml` + `PublisherDashboardPage.qml`):
- **Monthly bar chart**: Added Y-axis labels (`$<max>` at top, `$0` at bottom) with comma formatting. Increased left padding to 48px for labels. Grid opacity 0.5 → 0.7.
- **Revenue line chart**: Same Y-axis labels + comma formatting. Updated padding + hover math.
- **Pie chart center**: Total units formatted with `toLocaleString(Qt.locale("en_US"), "f", 0)`.
- **Per-book rating bar chart**: Grid opacity 0.5 → 0.7. Y-axis labels (0-5) already existed — improved text baseline alignment.
- **All chart footers** (Peak, Avg, Total): Numbers now use `toLocaleString(Qt.locale("en_US"), "f", 0)` for comma formatting.

## 5. Profile Page Header Layout

**Problem**: The top header Card was "messed" — content overflowed or misaligned.

**Fix** (`PublisherProfilePage.qml`):
- Changed the header RowLayout from `anchors.fill: parent` to `width: parent.width` so the Card auto-sizes from content
- Added `Layout.fillWidth: true` + `elide: Text.ElideRight` to the publisher name Text so long names ellipsize
- Added `Layout.alignment: Qt.AlignVCenter` to the "Edit profile" button

## 6. Notification "Clear Read" Not Working

**Problem**: The "Clear read" button removed read notifications momentarily, but they reappeared on the next UI refresh.

**Root cause**: `PublisherService::notifications()` checked `if (!m_notificationsCache.isEmpty()) return m_notificationsCache;`. When `clearReadNotifications()` removed all read items and the cache became empty, the next call to `notifications()` saw an empty cache → re-fetched from the server → got all notifications back (including the read ones that were just cleared).

**Fix** (`PublisherService.h` + `PublisherService.cpp`):
- Added `mutable bool m_notificationsCacheValid` flag
- `notifications()` now checks `if (m_notificationsCacheValid) return m_notificationsCache;` instead of `!isEmpty()` — so an empty cache (after clearing) is returned as-is without re-fetching
- `clearReadNotifications()` updates the cache in-place but does NOT clear `m_notificationsCacheValid`
- `refresh()` sets `m_notificationsCacheValid = false` to allow a fresh server fetch
- Constructor initializes `m_notificationsCacheValid(false)`

## 7. Smoother Hover Animations

**Problem**: Hover transitions felt abrupt.

**Fix** (6 publisher QML files):
- All `Behavior on color` blocks: duration changed from `Theme.motion.durationFast` (~100ms) to `200ms`
- Added `easing.type: Easing.OutCubic` where missing
- Applied to: dashboard row hover, catalog row hover, promotions row hover, drawer cards, profile cards

## 8. Half-Rounded Corners Fixed

**Problem**: Some pills, dots, and circles used a fixed radius instead of being fully rounded.

**Fix** (5 publisher QML files, 9 Rectangles):
- Status dots (6×6, 8×8): `radius: 3` / `radius: 4` → `radius: width / 2` (full circle)
- Status pills (h=22, h=24): `radius: 11` / `radius: 12` → `radius: height / 2` (pill shape)
- Avatar circle (36×36): `radius: 18` → `radius: width / 2`
- Verified badges: `radius: 11` / `radius: 9` → `radius: height / 2`

## Files Changed

| File | Changes |
|------|---------|
| `client/main.cpp` | English locale + DejaVu Sans font loading |
| `client/resources/fonts.qrc` | Added 3 DejaVu Sans font entries |
| `client/resources/fonts/DejaVuSans.ttf` | NEW — bundled text font |
| `client/resources/fonts/DejaVuSans-Bold.ttf` | NEW — bundled bold font |
| `client/resources/fonts/DejaVuSansMono.ttf` | NEW — bundled mono font |
| `client/qml/theme/Theme.qml` | Font family → DejaVu Sans |
| `client/qml/publisher/PublisherCatalogPage.qml` | Search fix + corner fix |
| `client/qml/publisher/PublisherSalesPage.qml` | CSV removal + chart polish + corner fix |
| `client/qml/publisher/PublisherDashboardPage.qml` | Chart polish + animation + corner fix |
| `client/qml/publisher/PublisherProfilePage.qml` | Header layout + animation + corner fix |
| `client/qml/publisher/PublisherPromotionsPage.qml` | Animation + corner fix |
| `client/qml/publisher/PublisherBookDetailDrawer.qml` | Animation + corner fix |
| `client/qml/publisher/PublisherNotificationsPage.qml` | (unchanged this round) |
| `client/include/services/PublisherService.h` | Added `m_notificationsCacheValid` flag |
| `client/src/services/PublisherService.cpp` | Fixed `clearReadNotifications` cache logic |

## How to Apply

1. Unzip this archive **on top of your existing project root** (the `bookclub/` folder). Confirm overwrite when asked.
2. In Qt Creator, run **Build → Clean** then **Build → Rebuild All** (the new font files + fonts.qrc changes require RCC to re-scan).
3. Run `BookClubClient.exe`.

## Verification Checklist

### Numbers + Fonts (global)
- [ ] All numbers in the app render as English digits (0-9), not Persian (۰-۹)
- [ ] All text uses the DejaVu Sans font (consistent across platforms)
- [ ] Chart axis labels, KPI values, and tooltips all show English numbers
- [ ] Dollar amounts have commas (e.g., "$1,234" not "$1234")

### Catalog search
- [ ] Type in the search field → catalog filters live as you type
- [ ] Press Enter → catalog filters + no toast spam

### CSV removal
- [ ] No "Copy CSV" buttons on the Sales page
- [ ] No leftover spacer items pushing content

### Charts
- [ ] Monthly bar chart shows Y-axis labels ($max at top, $0 at bottom)
- [ ] Revenue line chart shows Y-axis labels
- [ ] Pie chart center shows comma-formatted total
- [ ] Grid lines are more visible

### Profile header
- [ ] Header Card renders at proper height (not collapsed)
- [ ] Publisher name ellipsizes if too long
- [ ] "Edit profile" button is vertically centered
- [ ] Avatar + name + badge + button are all aligned

### Notifications
- [ ] "Clear read" removes read notifications permanently (they don't reappear)
- [ ] "Mark all as read" works
- [ ] Row click toggles read state

### Animations + Corners
- [ ] Hover transitions feel smooth (200ms, not abrupt)
- [ ] Status dots are full circles
- [ ] Pills/badges are fully rounded (pill shape)
