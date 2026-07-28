# BookClub — Complete Build-Ready Project (v8 merge)

This is the **complete, ready-to-build-and-run** BookClub project.

It is the merge of:
- **Server**: from `v4-pfp-fixed.zip` (real `DiscountCodes` table, real
  admin review moderation, real cart-level discount lookup).
- **Client**: from `bookclub_build_ready.zip` (v7 polish — DejaVu Sans
  font, English locale, qmldir files, fixed dark mode, password change
  dialog, live cover preview, all UI fixes through v7).
- **v8 changes**: removed every mock-data fallback from the publisher
  module; added 4 new server endpoints (`GetDiscountCodes`,
  `CreateDiscountCode`, `UpdateDiscountCode`, `DeleteDiscountCode`) so
  the Promotions page runs on the real `DiscountCodes` table.

See `MERGE_V8_CHANGELOG.md` for the full diff against the input zips.

## Quick Start

### Prerequisites
- **Qt 6.11+** (with `llvm-mingw_64` on Windows, or `gcc_64` on Linux)
- **CMake 3.16+**
- **Qt Creator** (recommended) or command-line build

### Build Steps (Qt Creator)
1. Open `CMakeLists.txt` in Qt Creator.
2. Configure the project with your Qt 6 kit.
3. **Build → Build All** (or Ctrl+B).
4. Run `BookClubServer` first (the server must be running for the
   client to function — there is no offline mock fallback anymore).
5. Run `BookClubClient`.

### Build Steps (Command Line)
```bash
cd bookclub
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt/6.11.1/llvm-mingw_64
cmake --build . -j8
```

### Run
```bash
# Start the server first (creates bookclub.db with schema v3 + seeds)
./build/bin/BookClubServer -p 8080

# Then start the client
./build/bin/BookClubClient
```

### Demo Accounts
| Username    | Password     | Role     |
|-------------|--------------|----------|
| `admin`     | `admin`      | Admin    |
| `publisher1`| `publisher1` | Publisher|
| `amir`      | `amir1234`   | User     |

## What's New in v8

- **Real DB-backed promo codes**: The Promotions page now lists codes
  from the `DiscountCodes` table (4 seeded for `publisher1`), and
  creating/updating/deleting codes writes to the same table via 4 new
  server endpoints.
- **Real admin review moderation**: Admin can now actually delete and
  approve any review via the v4 server's `AdminDeleteReview` /
  `AdminApproveReview` endpoints.
- **Real cart-level discounts**: The `ApplyDiscount` cart command now
  looks up real `DiscountCodes` rows, validates usage limits and
  minimum-cart totals, increments the `usedCount` counter, and returns
  the actual discount amount.
- **No more mock data**: Every publisher page now reflects the real
  database state. The recent-orders feed shows per-book sales rows,
  charts show real per-book sales/revenue, and the geographic-breakdown
  card shows the EmptyState rather than fabricated 8-region data.
- **Proper error handling**: The catalog and promotions pages now show
  error toasts when the server rejects a request (e.g. duplicate promo
  code, validation error, insufficient permissions) instead of always
  showing a success toast.

## Project Structure
```
bookclub/
├── CMakeLists.txt              ← top-level build
├── CMakePresets.json
├── MERGE_V8_CHANGELOG.md       ← what changed in v8
├── README_BUILD_READY.md       ← this file
├── client/                     ← Qt6 QML client (v7 polish + v8 real-DB fixes)
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── include/                ← C++ headers
│   ├── src/                    ← C++ sources
│   │   ├── services/PublisherService.cpp  ← v8: real DB only
│   │   ├── services/AdminService.cpp      ← v4: real review moderation
│   │   └── viewmodels/
│   ├── qml/                    ← all QML pages
│   │   ├── App.qml
│   │   ├── theme/Theme.qml
│   │   ├── components/         ← 12 subdirs with qmldir files
│   │   ├── auth/
│   │   ├── user/
│   │   ├── publisher/          ← error-handling for addBook/addPromotion
│   │   ├── admin/
│   │   ├── server/
│   │   └── qml.qrc
│   └── resources/
│       ├── fonts.qrc
│       └── fonts/              ← DejaVu Sans + Material Symbols
├── src/
│   ├── client/                 ← legacy controllers + network
│   └── server/                 ← v4 server (handlers, DB, threading)
│       ├── DatabaseManager.cpp ← schema version 3
│       ├── ServerCore.cpp      ← wires 4 new discount-code handlers
│       └── handlers/
│           ├── AdminRequestHandler.{h,cpp}    ← v4: review moderation
│           ├── CartRequestHandler.cpp         ← v4: real discount lookup
│           └── PublisherRequestHandler.{h,cpp} ← v8: 4 new discount-code handlers
├── common/                     ← shared models, network protocol, utils
│   ├── Constants.h             ← v4: was empty TODO in bookclub
│   ├── Network/Protocol.{h,cpp} ← v8: 4 new discount-code commands
│   └── ...
├── database/                   ← SQL schema + seeds
│   ├── schema.sql              ← v4: DiscountCodes table + v8: description column
│   └── seeds/sample_data.sql   ← v8: 4 seeded promo codes
├── tests/                      ← unit tests
├── scripts/                    ← build/run helpers
├── docs/                       ← architecture docs
└── installer/                  ← NSIS + Linux install scripts
```

## Build Notes

- The bundled **DejaVu Sans** font ensures consistent rendering across
  all platforms — no Persian digits, no device-local font fallback.
- The **English locale** is set globally in `main.cpp` so all numbers
  render as `0-9` regardless of system locale.
- The `qml.qrc` file includes all 13 `qmldir` files needed for Qt 6.11+
  explicit module resolution.
- The server's schema version is **3**. If an existing `bookclub.db`
  from a previous version is present, the server will delete and
  recreate it on first run.
- **The client requires a running server**. Unlike v7, there is no
  offline mock fallback — `PublisherService` queries the real server
  on every call. If the server is down, pages show empty states or
  error toasts.

## Troubleshooting

**"Type X unavailable" / "X is not a type"**: Do **Build → Clean →
Rebuild All** to force RCC to re-scan `qml.qrc`.

**Icons not rendering**: Verify `MaterialSymbolsOutlined-Regular.ttf`
is in `client/resources/fonts/` and registered in `fonts.qrc`.

**Persian digits appearing**: Verify `main.cpp` has the
`QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates))`
line BEFORE `QGuiApplication app(...)`.

**Dark mode buttons invisible**: The `Theme.qml` dark palette has been
fixed with lighter borders (#3A3C42) and field backgrounds (#22232A).

**Pages show empty states**: This is intentional in v8 — pages now
reflect the real DB. Make sure the server is running and the demo
publisher (`publisher1`) has data. The 4 seeded promo codes show on
the Promotions page immediately.

**"Promo code already exists" error**: This means the `code` you
entered collides with a row in `DiscountCodes`. Either pick a unique
code or delete the existing one first. The `WELCOME10` / `SUMMER20`
codes are seeded — use a different one.

**Catalog "Publish failed" error**: Server validation rejected the
new book. Common causes: empty title, title >200 chars, author name
>100 chars, price < 0, description >5000 chars.
