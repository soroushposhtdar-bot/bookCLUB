# Fixes Applied — BookCLUB V4 FIXED

This document summarizes the bugs that were identified and fixed in this
version of the BookCLUB project. All fixes were verified by building the
project with Qt 6.7 on Linux (gcc_64) and running the smoke test
(`scripts/smoke_test.py`).

## Build fixes (C++ / Qt 6 compatibility)

### 1. `QSysInfo::logicalCpuCount()` removed (Qt 6.7 incompatible)
**File:** `src/server/handlers/ServerInfoRequestHandler.cpp`

`QSysInfo::logicalCpuCount()` was removed from Qt 6 (it was a Qt 5
internal API). The compiler error was:

```
error: 'logicalCpuCount' is not a member of 'QSysInfo'
```

**Fix:** replaced with `QThread::idealThreadCount()` (the documented
Qt 6 replacement) and added `#include <QThread>`.

---

## QML syntax / runtime fixes

### 2. `iconPosition` property on `TextButton` removed
**File:** `client/qml/user/PdfReaderPage.qml` (line ~735)

`PdfReaderPage` was setting `iconPosition: "trailing"` on a `TextButton`.
Only `PrimaryButton` and `SecondaryButton` declare that property —
`TextButton` doesn't. The QML engine refused to load the page, which
made the entire `UserShell` (and therefore the whole post-login UI)
unavailable:

```
qrc:/qt/qml/bookclub/client/qml/user/UserShell.qml:488:9: Type PdfReaderPage unavailable
qrc:/qt/qml/bookclub/client/qml/user/PdfReaderPage.qml:735:29: Cannot assign to non-existent property "iconPosition"
```

**Fix:** removed the `iconPosition` line — the default icon-leading
layout is fine for "Next page" / "Previous page" buttons.

### 3. Duplicate closing braces in `PublisherProfilePage.qml`
**File:** `client/qml/publisher/PublisherProfilePage.qml`

The file ended with two extra `}` characters, causing a QML syntax
error that prevented the publisher shell from loading:

```
qrc:/qt/qml/bookclub/client/qml/publisher/PublisherProfilePage.qml:585:5: Syntax error
```

**Fix:** removed the two stray closing braces.

### 4. Duplicate closing brace in `BookDetailPage.qml`
**File:** `client/qml/user/BookDetailPage.qml`

Same issue — one extra `}` at the end of the file. Although the QML
engine was tolerating it (the rest of the file was syntactically
valid), it generated a parser warning and could mask future real
syntax errors.

**Fix:** removed the stray closing brace.

### 5. Deprecated Qt 5 signal-handler syntax in `AppIcon.qml`
**File:** `client/qml/components/AppIcon.qml` (line ~59)

The `onTextChanged` handler used the implicit-parameter Qt 5 syntax:

```qml
onTextChanged: {
    if (text.length === 0 && root.name.length > 0) { ... }
}
```

Qt 6 emits a deprecation warning for this and the implicit `text`
parameter will be removed in a future Qt version.

**Fix:** declared the parameter explicitly:

```qml
onTextChanged: function(text) {
    if (text.length === 0 && root.name.length > 0) { ... }
}
```

### 6. Deprecated `Connections` syntax in `App.qml`
**File:** `client/qml/App.qml` (line ~187)

The `onConnectionFailed` handler used the deprecated implicit form
instead of the `function(...)` form required by Qt 6:

```qml
Connections {
    target: AuthService
    onConnectionFailed: function(reason) { ... }   // ❌ deprecated
}
```

**Fix:**

```qml
Connections {
    target: AuthService
    function onConnectionFailed(reason) { ... }    // ✅ Qt 6 form
}
```

---

## Project / build-system fixes

### 7. `tests/CMakeLists.txt` referenced non-existent test files
**File:** `tests/CMakeLists.txt`

The previous `tests/CMakeLists.txt` referenced `test_user_services.cpp`,
`test_publisher_services.cpp`, and `test_admin_services.cpp` — none of
which exist in the `tests/` directory. Building with
`-DBOOKCLUB_BUILD_TESTS=ON` would fail with a "Cannot find source file"
error.

**Fix:** rewrote `tests/CMakeLists.txt` to only reference the test
files that actually exist in the project (`test_auth.cpp`,
`test_book_service.cpp`, `test_models.cpp`, `test_result.cpp`,
`test_session.cpp`). Also added the missing `Qt6::Qml` link dependency
for `test_auth` (because `AuthService.h` pulls in `<QQmlEngine>`).

### 8. Empty `run_server.sh` and `run_client.sh` scripts
**Files:** `scripts/run_server.sh`, `scripts/run_client.sh`

Both files were placeholders (`# TODO: script placeholder`).

**Fix:** implemented both scripts to:
- Locate the build output directory (`build/bin/`).
- Run the executable from the project root (so the server can find
  `database/schema.sql`).
- Print a helpful error if the binaries haven't been built yet.

### 9. Duplicate font file removed
**File:** `client/assets/fonts/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf`

This 10 MB file was an exact duplicate of
`client/resources/fonts/MaterialSymbolsOutlined-Regular.ttf` (the one
that's actually referenced by `fonts.qrc` and loaded by `main.cpp`).
The duplicate was wasting 10 MB in the source tree and the final zip.

**Fix:** deleted `client/assets/` (the file was not referenced
anywhere).

### 10. Empty `third_party/` directory removed
**Directory:** `third_party/`

Empty directory with no contents and no references in the build system.

**Fix:** removed.

---

## Smoke test added

### 11. `scripts/smoke_test.py`
**File:** `scripts/smoke_test.py` (new)

A new end-to-end smoke test that:
1. Starts the server in the background.
2. Verifies the server is listening on port 8080.
3. Starts the client with `QT_QPA_PLATFORM=offscreen`.
4. Waits up to 10 seconds for the "BookClub client ready" log line.
5. Reports PASS / FAIL with a clear summary.

This is useful for verifying that the app builds and runs correctly
without needing a display. The full auth flow (login → role shell →
dashboard navigation) should be verified manually by running the app
in Qt Creator and logging in with one of the demo accounts:

| Username    | Password     | Role     |
|-------------|--------------|----------|
| `admin`     | `admin`      | Admin    |
| `publisher1`| `publisher1` | Publisher|
| `amir`      | `amir1234`   | User     |

---

## Auth / user page wiring (verified, no changes needed)

After the fixes above, all auth and user pages were verified to be
properly connected through `App.qml`'s `StackView` router:

**Auth flow:**
- `SplashPage` → `WelcomePage` (after 1600 ms timer)
- `WelcomePage` → `LoginPage` (Login button) or `RegisterPage` (Create account)
- `LoginPage` → `GenreSelectionPage` (if `requiresGenreSetup`) or role shell
- `LoginPage` → `ForgotPasswordPage` (Forgot password link)
- `LoginPage` → `RegisterPage` (Create account link)
- `RegisterPage` → `GenreSelectionPage` (after successful registration)
- `ForgotPasswordPage` → `ResetPasswordPage` (after security answer verified)
- `ResetPasswordPage` → `LoginPage` (after password reset)
- `GenreSelectionPage` → role shell (if logged in) or `LoginPage` (if not)

**User shell navigation (sidebar routes):**
- Home, Discover (search), Library, Shelves, Group Reading, Cart,
  Notifications, Wishlist, Profile, Settings — all wired through
  `UserShell._navigateTo(route)` and the `_componentMap` lookup.
- BookDetailPage is pushed as an overlay via `_openBookDetail(bookId)`.
- PdfReaderPage is rendered as a full-screen overlay via `_openReader(bookId)`.
- CategoryPage is rendered as a full-screen overlay via `_openCategory(section)`.

**Role dispatch:**
- `App._enterRoleShell()` reads `AuthService.currentRole` and pushes
  the matching shell: `UserShell`, `PublisherShell`, `AdminShell`,
  or `ServerShell`.

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
