# BookClub Client — QML Load Fix (Round 2)

## Symptom

After applying the Round 1 fix (which resolved `AppIcon is not a type`),
the app still failed to start, now with:

```
qrc:/qt/qml/bookclub/client/qml/App.qml:417:9: Type PublisherShell unavailable
qrc:/qt/qml/bookclub/client/qml/publisher/PublisherShell.qml:401:41:
    Type PublisherNotificationsPage unavailable
qrc:/qt/qml/bookclub/client/qml/publisher/PublisherNotificationsPage.qml:165:25:
    GenreChip is not a type
Failed to load QML root — aborting.
```

## Root Cause

Two separate issues, both at play:

1. **Missing `import "../components/book"` in `PublisherNotificationsPage.qml`**
   The page uses `GenreChip` (defined in `components/book/GenreChip.qml`) but
   only imports `../components` (the parent folder). `import "../components"`
   makes `AppIcon` and `NetworkImage` available — *not* the contents of its
   sub-directories. So `GenreChip` was unresolved.

2. **Qt 6.11 implicit-module fragility in `components/*` sub-directories.**
   Even when a page *did* import `../components/book` correctly, Qt 6.11's
   implicit-module resolution for the `book/` folder can still fail because
   the *parent* `components/` folder is a hybrid (mixes top-level `.qml`
   files with sub-directories). This is the same class of bug that broke
   `AppIcon` in Round 1 — but now at the sub-directory level.

## Files Changed (this round)

### NEW: 11 qmldir files in `client/qml/components/*`

Each one explicitly lists the QML types defined directly in that
sub-directory, so Qt 6.11 doesn't have to rely on implicit-module
resolution:

| File | Types declared |
|------|----------------|
| `components/book/qmldir` | BookCard, BookCarousel, BookCover, BookRow, GenreChip, RatingStars, StarInput |
| `components/branding/qmldir` | BrandLogo, SecurityBadge |
| `components/buttons/qmldir` | FloatingActionButton, IconButton, PrimaryButton, SecondaryButton, TextButton |
| `components/data/qmldir` | AnimatedCounter, EmptyIllustration, EmptyState, ErrorState, FilterChip, HeroBanner, NotificationItem, RatingDistribution, ReviewItem, ReviewReactionButton, SectionCarousel, SectionHeader, SettingToggleRow, ShelfCard, ShelfRow, StatCard |
| `components/effects/qmldir` | DropShadowBase, RippleEffect |
| `components/feedback/qmldir` | ConfirmDialog, ConfirmationPopup, ContextMenu, OfflineBanner, Toast, ToastManager, UndoToast, ValidationMessage |
| `components/inputs/qmldir` | CalendarGrid, InputField, OtpInput, PasswordField, SearchField |
| `components/navigation/qmldir` | Avatar, Breadcrumb, NavItem, Pagination, Sidebar, SortDropdown, TabBar, TabPanel, TopBar, ViewToggle |
| `components/progress/qmldir` | LoadingOverlay, ProgressBar, SkeletonLoader, Spinner |
| `components/selection/qmldir` | AppCheckbox, AppRadioButton, AppToggleButton |
| `components/surfaces/qmldir` | Card, Divider, StickyPanel |

### MODIFIED: `client/qml/qml.qrc`

Added `<file>` entries for all 11 new qmldir files so they are bundled
into the Qt resource file. (Without this, the qmldir files would not
ship inside the `.exe` and the fix would not work.)

### MODIFIED: `client/qml/publisher/PublisherNotificationsPage.qml`

Added `import "../components/book"` so the page can use `GenreChip`.
This was a genuine bug — the other publisher pages (`PublisherCatalogPage`,
`PublisherPromotionsPage`, `PublisherSalesPage`) already imported
`../components/book`, but `PublisherNotificationsPage` was missed.

## Combined Round 1 + Round 2 file list

If you're applying this fix fresh (without Round 1), the full set of
changed files is:

| File | Status |
|------|--------|
| `client/qml/components/qmldir` | NEW (Round 1) — declares AppIcon + NetworkImage |
| `client/qml/components/AppIcon.qml` | MODIFIED (Round 1) — `font.weight` enum mapping |
| `client/qml/components/NetworkImage.qml` | MODIFIED (Round 1) — added `import "."` |
| `client/qml/components/book/qmldir` | NEW (Round 2) |
| `client/qml/components/branding/qmldir` | NEW (Round 2) |
| `client/qml/components/buttons/qmldir` | NEW (Round 2) |
| `client/qml/components/data/qmldir` | NEW (Round 2) |
| `client/qml/components/effects/qmldir` | NEW (Round 2) |
| `client/qml/components/feedback/qmldir` | NEW (Round 2) |
| `client/qml/components/inputs/qmldir` | NEW (Round 2) |
| `client/qml/components/navigation/qmldir` | NEW (Round 2) |
| `client/qml/components/progress/qmldir` | NEW (Round 2) |
| `client/qml/components/selection/qmldir` | NEW (Round 2) |
| `client/qml/components/surfaces/qmldir` | NEW (Round 2) |
| `client/qml/publisher/PublisherNotificationsPage.qml` | MODIFIED (Round 2) — added missing `import "../components/book"` |
| `client/qml/qml.qrc` | MODIFIED (Round 1 + Round 2) — registered all new qmldir files |

## How to Apply

1. Unzip this archive **on top of your existing project root** (the
   `bookclub/` folder that contains `client/`, `src/`, `common/`, …).
   Confirm overwrite when asked.
2. In Qt Creator, run **Build → Clean** then **Build → Rebuild All**.
   The qml.qrc change forces Qt's RCC to re-scan, so a clean rebuild is
   required.
3. Run `BookClubClient.exe`.

## Why This Should Be the Last Round

The pattern of failure was:

- Round 1: `AppIcon` (top-level file in `components/`)
- Round 2: `GenreChip` (file in `components/book/`)

After Round 2, **every** QML directory in the project either:

- Has an explicit `qmldir` (the `components/` tree, all 12 folders), OR
- Contains only `.qml` files with no sub-directories (every other folder:
  `auth/`, `user/`, `publisher/`, `admin/`, `server/`, `layouts/`,
  `theme/`), so implicit-module resolution is unambiguous and works
  reliably on Qt 6.11.

There are no remaining hybrid directories in the project, so no further
"X is not a type" errors of this class should occur.
