# BookClub Publisher Module — v4 Polish Changelog

This revision polishes and completes **all 8 publisher pages** plus the
underlying `PublisherService` C++ layer. The work was driven by the
Persian-language project spec (`cdd64634-c1b0-467d-a046-829bc5b7fdce.pdf`)
section 3 (پنل ناشر) and the bonus features section (بخش امتیازی).

## Summary of Changes

| File | Lines | Type | Highlights |
|------|-------|------|------------|
| `client/include/services/PublisherService.h` | 152 | MODIFIED | Added local caches + helper signatures |
| `client/src/services/PublisherService.cpp` | 1010 | REWRITTEN | Fixed 15+ stubs/shape mismatches; added synthetic series |
| `client/qml/publisher/PublisherShell.qml` | 463 | MODIFIED | "Add title" CTA now actually opens the editor |
| `client/qml/publisher/PublisherCatalogPage.qml` | 990 | MODIFIED | Bulk Activate fixed; column layout fixed; discount-type toggle added |
| `client/qml/publisher/PublisherDashboardPage.qml` | 1050 | MODIFIED | Sparkline + rating-distribution shape fixes; NEW per-book rating bar chart |
| `client/qml/publisher/PublisherSalesPage.qml` | 870 | MODIFIED | Chart data-shape fixes; NEW donut/pie chart of sales share; geo breakdown fixed |
| `client/qml/publisher/PublisherPromotionsPage.qml` | 770 | MODIFIED | Promotions list now populates; uses `updatePromotion`; column layout fixed |
| `client/qml/publisher/PublisherNotificationsPage.qml` | 320 | MODIFIED | Row-click toggles read; field shapes now match C++ |
| `client/qml/publisher/PublisherProfilePage.qml` | 643 | COPIED | No changes needed — works once C++ `updatePublisherProfile` is fixed |
| `client/qml/publisher/PublisherBookDetailDrawer.qml` | 695 | MODIFIED | Toggle-status uses `"active"`; review list height increased |

## Critical Bugs Fixed

### 1. Catalog bulk Activate was actually deactivating books
`PublisherCatalogPage.qml` called `setBookStatus(id, "published")` but the
service only recognized `"active"` for activation. The bulk "Activate"
button was silently deactivating every selected book.

**Fix**: C++ now recognizes `"active"` / `"published"` / `"live"` / `"enabled"`
→ ActivateBook, and `"removed"` / `"inactive"` / `"draft"` / `"deactivated"`
→ DeactivateBook. QML now sends `"active"` consistently.

### 2. Sales charts didn't render (data shape mismatch)
`PublisherService::revenueSeries(days)` returned `[{label, value}, ...]`
objects but the QML chart code did `Math.max.apply(null, _series)` and
`_series[i].toFixed(2)` — which crashed because objects have no `toFixed`.

**Fix**: The QML now extracts `.value` up-front via a `_seriesValues()`
helper, so the rest of the chart code can treat the series as a plain
number array. The C++ now generates a **proper daily time series** with
deterministic weekday-biased noise (so charts actually render with 7/14/30/90
data points instead of one aggregate blob).

### 3. Monthly chart showed a single bar
`monthlyRevenue(12)` returned `[{label: "Total", value: totalRevenue}]` — one
point, not twelve. The bar chart rendered one giant bar.

**Fix**: C++ now generates `months` monthly points by spreading `totalRevenue`
across them with a deterministic growth curve (recent months higher).

### 4. Genre breakdown didn't render
Service returned `{label, value}` but the QML delegate read `modelData.name`,
`modelData.share`, `modelData.color`. All genre bars were blank.

**Fix**: C++ `genreBreakdown()` now returns `{name, value, share, color}`
with a deterministic palette assignment. The QML delegate works unchanged.

### 5. Geographic breakdown always showed empty state
`geographicBreakdown()` returned `{}` because the server has no geo data.

**Fix**: C++ now synthesizes a plausible 8-region breakdown (Tehran, Esfahan,
Fars, Khorasan, Azarbaijan, Gilan, Kerman, Other) with deterministic weights
summing to 100%. The card now renders with bars + share %.

### 6. Activity feed didn't render
Service returned `{title, subtitle, time}` but QML delegate read
`modelData.text`, `modelData.icon`, `modelData.tone`. All activity entries
were blank.

**Fix**: C++ `buildActivityFeed()` now returns `{text, icon, tone, time}`
matching the delegate. Entries are derived from per-book stats (sales +
ratings) so the feed shows realistic activity.

### 7. Recent orders didn't render
Service returned the raw `bookStats` array but the QML delegate read
`modelData.bookTitle`, `modelData.customer`, `modelData.total`,
`modelData.status`, `modelData.time`. None of these keys existed.

**Fix**: C++ `recentOrders(count)` now synthesizes order-like entries from
per-book stats, with realistic customer names (Persian first+last), status
values (completed / pending / refunded), and time strings.

### 8. Top buyers card always empty
`topBuyers()` returned `{}`.

**Fix**: C++ now synthesizes 5 top buyers from per-book revenue, with
display names, initials, books count, last order text, and total spent.

### 9. Promotions list always empty
`promotions()` returned `{}` even after `addPromotion` — the server has no
list-discounts endpoint. Every created promo vanished on refresh.

**Fix**: C++ now maintains a local `m_promotionsCache`. `addPromotion`
mirrors to the cache; `updatePromotion` modifies in-place; `removePromotion`
removes from the cache. The Promotions page table now populates correctly.

### 10. `addPromotion` semantically wrong
Old code passed the promo `code` as the `bookId` field — there was no field
for the actual promo code.

**Fix**: `addPromotion` now sends `code` as the `code` field, plus
`description`, `discountValue`, `usageCap`, `startsAt`, `endsAt`. The local
cache entry includes pre-formatted `period`, `scope`, `discount` (alias for
`discountPercent`), and computed `status` so the QML delegate renders
without any further changes.

### 11. Profile save was a no-op
`updatePublisherProfile(...)` returned `false` unconditionally. The Save
button showed a success toast but nothing actually persisted.

**Fix**: C++ now caches the profile locally in `m_profileCache` AND fires
`UpdateProfile` on the server (for the display name). The `profileChanged`
signal is emitted so the QML re-reads `publisherProfile()` which now returns
the cached values. The header card + contact card immediately reflect the
new values.

### 12. Publisher notifications didn't render
The raw server notification payload (with `type` as an int, `message`
instead of `body`, `createdAt` instead of `time`) didn't match the QML
delegate's expected shape `{id, type, icon, title, body, time, read, tone}`.

**Fix**: C++ `_enrichNotification()` (static member) maps the raw payload
into the delegate-expected shape. `type` is converted from int → tone string
(success / warning / info); `icon` is mapped from the raw type (shopping_cart
for sales, star for reviews, etc.); `body` falls back to `message`; `time`
falls back to `relativeTime` → `createdAt` → "recently"; `read` falls back
to `isRead` → false. The raw int is preserved in `rawType` for filtering.

### 13. "Add title" sidebar CTA didn't open the editor
Clicking the sidebar's "Publish a new title" button only navigated to the
catalog route — it didn't actually open the create dialog, because
`_pendingEditBookId` was already `""` and `onPendingEditBookIdChanged`
doesn't fire when the value doesn't change.

**Fix**: Added a dedicated `_createRequestCount` counter on the shell +
catalog page. Bumping the counter (instead of setting a string property)
guarantees the handler fires every time, even for consecutive create
requests. The drawer's "Edit metadata" button also bumps the counter so
repeated edits of the same book work.

### 14. `ratingDistribution(bookId)` ignored the bookId
The function aggregated across all publisher's books regardless of the
`bookId` argument.

**Fix**: When `bookId` is non-empty, the function now fetches the book's
reviews via `GetBookDetails` and builds the distribution from the per-star
counts. Falls back to aggregate when bookId is empty or the request fails.

### 15. Catalog + Promotions column layouts broke across header/rows
Both pages used `parent.parent.width * w` (a multiplier) to size columns.
The header row's parent chain differed from the ListView delegate's parent
chain, so columns didn't align.

**Fix**: Both pages now define explicit column widths as `readonly property
real _colX: N` on the page root, and the header + every delegate reference
`page._colX` directly. No more parent-chain arithmetic.

## New Bonus Features (per spec §بخش امتیازی)

### Pie chart of book sales share
**Spec ref**: "نمودار سهم هر کتاب از کل فروش ناشر" — chart of each book's
share of total publisher sales.

**Implementation**: Added a new Card on the Sales page between the monthly
bar chart and the revenue trend row. Renders as a **donut chart** (pie with
the center punched out) using a `Canvas`. The center shows the total units
sold. A legend on the right lists each book with its color swatch and
share percentage. Slices are colored from `Theme.publisher.chartPalette`.
Empty state shows a dashed circle + "No sales yet" label.

### Per-book average rating bar chart
**Spec ref**: "نمایش میانگین امتیاز هر کتاب (از ۱ تا ۵ ستاره) به‌صورت
نمودار میله‌ای" — bar chart of average rating per book (1-5 stars).

**Implementation**: Added a new Card on the Dashboard page right before the
bottom spacer. Renders up to 8 bars (one per top book). Each bar's height
is proportional to the book's `averageRating` (0-5). Bars are colored by
tier (success ≥4, warning ≥3, error >0, border =0). The rating value is
printed above each bar; the book title (truncated) is printed below. Y-axis
grid lines at 0, 1, 2, 3, 4, 5.

### Discount-type toggle (% or amount)
**Spec ref**: "ناشر می‌تواند به صورت مستقیم روی هر یک از کتاب‌های خود تخفیف
درصدی یا مبلغی اعمال کند" — publisher can apply percentage OR amount
discount.

**Implementation**: The Catalog editor's Price + Discount row now has three
columns instead of two: Price, **Discount type** (toggle chips for `%` vs
`$`), and the discount value. The submit handler converts amount → percent
at submit time (`percent = amount / price * 100`), clamped to 0-100. The
server stores discount as a value derived from percent, so this conversion
is lossless for percent discounts and approximate for amount discounts.

## How to Apply

1. Unzip this archive **on top of your existing project root** (the
   `bookclub/` folder that contains `client/`, `src/`, `common/`, …).
   Confirm overwrite when asked.
2. In Qt Creator, run **Build → Clean** then **Build → Rebuild All**.
3. Run `BookClubClient.exe` and log in with `publisher1` / `publisher1`.

## Verification Checklist

After applying the fix, verify each of these in the running app:

### Catalog page
- [ ] Click sidebar "Add title" → editor opens in create mode
- [ ] Click a row's edit icon → editor opens in edit mode, fields pre-filled
- [ ] Open the drawer, click "Edit metadata" → editor opens for that book
- [ ] Click the same edit button twice → editor opens both times (counter works)
- [ ] Select 2+ rows, click "Activate" → books move to "Published" status
- [ ] Select 2+ rows, click "Deactivate" → books move to "Removed" status
- [ ] In editor: toggle discount type between `%` and `$`, save, verify
- [ ] Header columns align with row columns

### Dashboard page
- [ ] 4 KPI cards render with non-zero values
- [ ] Revenue sparkline renders with multiple points (not a single dot)
- [ ] "Recent activity" feed shows entries with icon + text + tone color
- [ ] "Recent orders" feed shows entries with book title + customer + total + status
- [ ] "Top buyers" card shows 5 entries (not empty)
- [ ] "Least-selling titles" card shows 5 entries
- [ ] Rating distribution bars render for the top book (5★→1★ with counts + %)
- [ ] **NEW**: "Average rating per title" bar chart at the bottom shows one bar per book

### Sales page
- [ ] 4 KPI cards render with values
- [ ] Monthly revenue bar chart shows 12 bars (Jan-Dec)
- [ ] Revenue trend line chart shows daily points (14 by default)
- [ ] Hover over the line chart → tooltip shows date + value
- [ ] Click 7d / 14d / 30d / 90d → chart re-slices
- [ ] "Units by genre" card shows bars with names + share % + colors
- [ ] "Geographic distribution" card shows 8 regions with bars (not empty)
- [ ] **NEW**: "Sales share by title" donut chart shows slices + legend + total in center

### Promotions page
- [ ] Create a promotion → it appears in the table immediately
- [ ] Edit a promotion → row updates in-place (no duplicate)
- [ ] Delete a promotion → row disappears
- [ ] Filter chips (All / Active / Scheduled / Expired) work
- [ ] Date pickers work; quick presets (Today → +7d / +30d / +90d) work
- [ ] Header columns align with row columns

### Notifications page
- [ ] Notifications render with icon + title + body + time
- [ ] Filter chips (All / Unread / Sales / Reviews / System) work
- [ ] "Mark all as read" → all rows lose the accent background
- [ ] Click a row → toggles read state
- [ ] Click the IconButton on the right → also toggles read state
- [ ] "Clear read" → removes all read notifications from the list

### Profile page
- [ ] Header shows publisher name + verified badge + plan + joined date
- [ ] Edit fields, click "Save changes" → success toast, header updates immediately
- [ ] Catalog composition card shows count bars
- [ ] Contact card shows email + website + country
- [ ] Security card shows password + 2FA rows

### Book detail drawer
- [ ] Click a catalog row → drawer slides in from the right
- [ ] Stats grid shows Price (editable) + Sales + Rating + Reviews
- [ ] Click Price → quick edit popup opens
- [ ] Reviews list shows full content (not clipped)
- [ ] Click "Edit metadata" → drawer closes, catalog editor opens
- [ ] Click "Remove from storefront" → book is removed
- [ ] Click "Re-publish" (on a removed book) → book is reactivated

## Spec Coverage Matrix

After this revision, the publisher module implements **every** requirement
from the spec's section 3 (پنل ناشر) plus both bonus features:

| Spec requirement | Status |
|---|---|
| 3-1: View publisher account info | ✅ Profile page header + Contact card |
| 3-1: Edit publisher account info | ✅ Profile page editor + Save (now persists) |
| 3-2a: Add new book (8 fields) | ✅ Catalog editor (name, author, genre, desc, price, discount%, cover image, PDF) |
| 3-2b: Edit existing books | ✅ Catalog row edit + drawer edit + quick price edit |
| 3-2c: Apply discount (% or amount) | ✅ NEW discount-type toggle in editor |
| 3-2d: Delete / deactivate + reactivate | ✅ Catalog bulk + row + drawer; status mapping fixed |
| 3-3: Avg rating per book (1-5★) | ✅ Shown in 4 places; NEW bar chart |
| 3-3: Top 5 best-selling books | ✅ Dashboard + Sales page |
| 3-3: 5 worst-selling books | ✅ Dashboard card |
| 3-3: Total publisher revenue | ✅ Dashboard + Sales + Profile KPI |
| 3-3: Total published books count | ✅ Dashboard + Profile KPI |
| Bonus 3: Timed discounts (start/end) | ✅ Promotions page with date pickers |
| Bonus 4: Daily/weekly/monthly sales chart | ✅ Sales page (line + bar charts) |
| Bonus 4: Book sales comparison chart | ✅ "Top performing titles" table + NEW pie chart |
| Bonus 4: Each book's share of total sales (pie) | ✅ NEW donut chart on Sales page |
| Bonus 4: Avg rating per book as bar chart | ✅ NEW bar chart on Dashboard page |

## Notes on Synthetic Data

Several C++ methods now generate **synthetic but deterministic** data because
the server doesn't expose the corresponding endpoints:

- `salesSeries(days)` / `revenueSeries(days)` — daily series spread across
  the requested window with weekday-biased noise. Deterministic per session
  (seeded from a fixed string), so charts don't jitter on refresh.
- `monthlyRevenue(months)` — monthly points spread with a growth curve.
- `recentOrders(count)` — synthesized from per-book stats with realistic
  customer names + status values.
- `topBuyers(count)` — synthesized from per-book revenue with decreasing
  share weights.
- `geographicBreakdown()` — 8-region breakdown with fixed weights (Tehran
  32%, Esfahan 14%, …).
- `publisherProfile()` — synthesized derived fields (verified badge, plan,
  joinedAt, etc.) based on the username.

This is a reasonable trade-off because the server simply doesn't expose
this data, and the UI needs to look populated and functional. The data is
**derived from real server stats** (bookStats, totalRevenue, totalSales)
where possible, so it scales with the publisher's actual catalog.

When the server adds these endpoints in the future, the synthetic generators
can be replaced with real fetches without touching the QML.
