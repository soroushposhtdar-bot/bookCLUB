# BookClub Publisher Module — v5 Layout Fix Changelog

This revision fixes the **layout issues** reported across all publisher pages.
The root cause was a single bug in the shared `Card.qml` component that
caused every Card without an explicit height to render at only `2 * padding`
pixels tall — crushing all content into a tiny strip.

## Root Cause

`Card.qml` used `_content.implicitHeight` to auto-size, but
`Item.implicitHeight` does **NOT** auto-compute from children — it defaults
to 0 and stays 0. This meant:

```
Card.implicitHeight = _content.implicitHeight + 2 * padding
                    = 0 + 2 * padding
                    = ~16px
```

Every Card without an explicit `Layout.preferredHeight` or `height:` binding
rendered at ~16px tall. Content (tables, lists, forms, charts) either
overflowed visibly ("text out of boxes") or was clipped to 0 height
("catalog page is basically empty").

## The Fix

### 1. `client/qml/components/surfaces/Card.qml` (ROOT CAUSE)

Changed the auto-sizing mechanism:
- **Old**: `implicitHeight: _content.implicitHeight + 2 * padding` (always 0 + 2*padding)
- **New**: `implicitHeight: _content.childrenRect.height + 2 * padding` (actual content height)

`childrenRect.height` IS computed automatically by QtQuick from the bounding
box of all children. This makes Cards auto-size correctly.

Also changed `_content` to use `anchors.fill: parent` (anchoring to all 4
sides) so that Cards WITH explicit heights still have their content fill the
Card properly.

### 2. Publisher QML pages — `anchors.fill: parent` → `width: parent.width`

With the new Card.qml, Cards without explicit heights auto-size from their
content. But if the content uses `anchors.fill: parent`, its height depends
on the Card's height — creating a circular dependency. The fix is to use
`width: parent.width` instead, which lets the content's height be driven by
its own children.

**14 ColumnLayouts** across 6 pages were changed from
`anchors.fill: parent` to `width: parent.width` (with `id:` added):

| File | ColumnLayouts fixed |
|------|-------------------|
| `PublisherCatalogPage.qml` | Catalog table content |
| `PublisherDashboardPage.qml` | Top books, Top viewed, Least selling, Rating distribution |
| `PublisherSalesPage.qml` | Top books table, Geographic distribution |
| `PublisherProfilePage.qml` | Account editor, Catalog composition, Contact, Security |
| `PublisherPromotionsPage.qml` | Promo editor, Promotions table |
| `PublisherNotificationsPage.qml` | Notifications list |
| `PublisherBookDetailDrawer.qml` | Genres, Description, Reviews |

ColumnLayouts inside Cards WITH explicit heights (e.g.
`Layout.preferredHeight: 300`) were left unchanged — `anchors.fill: parent`
works correctly there because the Card has a fixed height.

### 3. StatCard heights

**12 StatCards** across 3 pages were missing explicit heights. StatCard
inherits from Card and has an internal `Row { anchors.fill: parent }` that
needs a fixed height to render. Without it, StatCards rendered at ~16px and
the 44px icon + text overflowed.

Added `Layout.preferredHeight: 100` to every StatCard in:
- `PublisherDashboardPage.qml` (1 KPI Repeater → 4 cards)
- `PublisherSalesPage.qml` (4 KPI cards)
- `PublisherProfilePage.qml` (4 KPI cards)

### 4. Catalog page — `_colX` scope fix

The column-width properties (`_colCheckbox`, `_colTitle`, etc.) were
declared INSIDE the table's ColumnLayout but referenced as `page._colX`
(root scope). Since `page` is the root Item, `page._colX` was `undefined` —
causing all column widths to be 0 and the table to collapse into an
unreadable mess.

**Fix**: Moved all 8 `_colX` properties to the root `Item { id: page }`
level so `page._colX` resolves correctly.

### 5. Profile page — "Edit profile" button now scrolls

The button previously only called `_fPublisherName.forceActiveFocus()`,
which focuses the field but doesn't scroll it into view. If the editor was
below the fold, nothing visible happened — so the button appeared broken.

**Fix**: The `onClicked` handler now also sets
`_scroll.ScrollBar.vertical.position` to scroll the editor card into view.

## Files Changed (this revision)

| File | Change |
|------|--------|
| `client/qml/components/surfaces/Card.qml` | **ROOT CAUSE**: `childrenRect.height` instead of `implicitHeight`; `_content` anchors to all 4 sides |
| `client/qml/publisher/PublisherCatalogPage.qml` | Moved `_colX` to root; table ColumnLayout uses `width: parent.width` |
| `client/qml/publisher/PublisherDashboardPage.qml` | 4 ColumnLayouts → `width: parent.width`; StatCards get `Layout.preferredHeight: 100` |
| `client/qml/publisher/PublisherSalesPage.qml` | 2 ColumnLayouts → `width: parent.width`; 4 StatCards get height |
| `client/qml/publisher/PublisherProfilePage.qml` | 4 ColumnLayouts → `width: parent.width`; 4 StatCards get height; edit button scrolls |
| `client/qml/publisher/PublisherPromotionsPage.qml` | 2 ColumnLayouts → `width: parent.width`; 3 StatCards get height |
| `client/qml/publisher/PublisherNotificationsPage.qml` | 1 ColumnLayout → `width: parent.width` |
| `client/qml/publisher/PublisherBookDetailDrawer.qml` | 3 ColumnLayouts → `width: parent.width` |

## Also included (from v4, unchanged)

These files from the previous v4 polish are included for completeness —
they contain the C++ service fixes and feature additions:

| File | v4 highlights |
|------|--------------|
| `client/include/services/PublisherService.h` | Local caches, helper signatures |
| `client/src/services/PublisherService.cpp` | 15+ stub fixes, synthetic series, promotions cache, profile save |
| `client/qml/publisher/PublisherShell.qml` | "Add title" CTA counter, create-request wiring |

## How to Apply

1. Unzip this archive **on top of your existing project root** (the
   `bookclub/` folder). Confirm overwrite when asked.
2. In Qt Creator, run **Build → Clean** then **Build → Rebuild All**.
3. Run `BookClubClient.exe` and log in with `publisher1` / `publisher1`.

## Verification Checklist

After applying, verify each page renders correctly:

### Catalog page
- [ ] Top toolbar Card (search + filter chips + "Add new title" button) renders at full height
- [ ] Catalog table Card shows the header row + book rows with aligned columns
- [ ] Column widths are correct (Title is wide, Status/Price/Units/Rating/Updated/Actions are narrower)
- [ ] Empty state shows when no books match the filter
- [ ] "Add new title" button opens the editor popup
- [ ] Editor popup shows all fields including the discount-type toggle

### Dashboard page
- [ ] 4 KPI StatCards render at ~100px tall (icon + value + label + delta all visible)
- [ ] Revenue sparkline Card renders at 300px tall with the chart visible
- [ ] Recent activity feed shows entries with icon + text + tone color
- [ ] Top performing titles list shows cover + title + sales + rating
- [ ] Rating distribution bars render (5★→1★ with counts + percentages)
- [ ] Per-book rating bar chart renders at the bottom

### Sales page
- [ ] 4 KPI StatCards render at ~100px tall
- [ ] Monthly revenue bar chart renders 12 bars
- [ ] Pie chart (sales share by title) renders with donut + legend
- [ ] Revenue trend line chart renders with hover tooltip
- [ ] Genre breakdown bars render with names + share % + colors
- [ ] Geographic distribution bars render (8 regions)
- [ ] Top performing titles table renders

### Profile page
- [ ] Header Card shows avatar + name + verified badge + plan + joined date
- [ ] 4 KPI StatCards render at ~100px tall
- [ ] "Edit profile" button scrolls the editor card into view
- [ ] Account editor Card shows all fields (name, bio, website, email, tax ID)
- [ ] "Save changes" button persists and shows success toast
- [ ] Catalog composition Card shows bars (Published/Draft/Pending/Removed)
- [ ] Contact Card shows email + website + country
- [ ] Security Card shows password + 2FA rows

### Promotions page
- [ ] 3 KPI StatCards render at ~100px tall
- [ ] Create/edit promotion Card shows all fields (code, desc, %, cap, dates)
- [ ] Date picker popups work
- [ ] Existing promotions table renders with aligned columns
- [ ] Filter chips (All/Active/Scheduled/Expired) work
- [ ] Edit/delete buttons work

### Notifications page
- [ ] Header Card shows unread count + total count
- [ ] Filter chips (All/Unread/Sales/Reviews/System) work
- [ ] Notifications list renders entries with icon + title + body + time
- [ ] Click a row → toggles read state
- [ ] "Mark all as read" works
- [ ] "Clear read" works

### Book detail drawer
- [ ] Drawer slides in from the right
- [ ] Cover + title + author + status render
- [ ] Stats grid (Price/Sales/Rating/Reviews) renders
- [ ] Genres Card shows chips
- [ ] Description Card shows text
- [ ] Reviews Card shows review entries (not clipped)
- [ ] "Edit metadata" + "Toggle status" buttons work

## Why this fix is safe for other pages

The `Card.qml` change is backward-compatible:
- Cards with explicit heights (`Layout.preferredHeight` or `height:`) are
  unaffected — their height is set explicitly, and `_content` fills via
  `anchors.fill: parent`.
- Cards without explicit heights now auto-size from `childrenRect.height`
  instead of being stuck at `2 * padding`. This only IMPROVES rendering —
  no previously-working Card will break.
- Existing pages that use the `height: contentId.implicitHeight + 2*padding`
  pattern (like `ShelvesPage.qml`) continue to work — the explicit height
  takes precedence over the new `implicitHeight` binding.
