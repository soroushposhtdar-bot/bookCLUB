# Build & Run Roadmap — Qt 6.11 / MinGW / Windows

This roadmap tracks the work needed to get BookCLUB to compile and run
cleanly in Qt Creator on Windows with Qt 6.11 + MinGW. **No new features.**

## Environment

- **Qt**: 6.11.x
- **Compiler**: MinGW (LLVM-MinGW or GCC MinGW)
- **OS**: Windows
- **IDE**: Qt Creator

---

## Phase 1: Fix Server Compilation ✅ (DONE)

Goal: `BookClubServer.exe` builds without errors.

### Checklist

- [x] 1.1 `ServerCore.h` — add `#include <QTimer>` for `m_discountExpiryTimer`
- [x] 1.2 `ServerCore.cpp` — verify all repository factory includes are present
- [x] 1.3 `CartRequestHandler.cpp` — verify `DatabaseManager` include + `db()` usage
- [x] 1.4 `BookRequestHandler.h/.cpp` — verify constructor signature matches `ServerCore.cpp` call
- [x] 1.5 `PublisherRequestHandler.h/.cpp` — verify constructor signature matches
- [x] 1.6 `CartRequestHandler.h/.cpp` — verify constructor signature matches
- [x] 1.7 `RequestStats.h/.cpp` — verify it's in `src/server/CMakeLists.txt`
- [x] 1.8 `ServerInfoRequestHandler.h/.cpp` — verify it's in CMakeLists
- [x] 1.9 `src/server/CMakeLists.txt` — cross-check all new files are listed
- [x] 1.10 `common/Utils/Logger.cpp` — verify `<QFile>` and `<QFileInfo>` includes for rotation
- [x] 1.11 `common/Interfaces/IBookService.cpp` — verify `<QSet>` include for multi-field search
- [x] 1.12 `common/CMakeLists.txt` — verify all new `.cpp`/`.h` files are listed

### Verification

```bat
cd bookCLUB
mkdir build && cd build
cmake .. -DBOOKCLUB_BUILD_CLIENT=OFF -DBOOKCLUB_BUILD_SERVER=ON
cmake --build . -j8
.\bin\BookClubServer.exe -p 8080
```

**Success**: Server starts, prints "Server initialized successfully on port 8080".

---

## Phase 2: Fix Client Compilation ✅ (DONE)

Goal: `BookClubClient.exe` builds without errors.

### Checklist

- [x] 2.1 `ClientNetworkManager.h` — add `#include <QHash>`
- [x] 2.2 `ClientNetworkManager.cpp` — verify all new members are declared in `.h`
- [x] 2.3 `NetworkService.h` — add `#include <functional>` if missing
- [x] 2.4 `NetworkService.cpp` — verify `Message` constructor + `sendMessage` usage
- [x] 2.5 `BookService.cpp` — verify event subscription signatures
- [x] 2.6 `BookService.cpp` — verify `wishlistShelfId()` static function compiles
- [x] 2.7 `NotificationService.cpp` — verify `NetworkService::subscribeEvent` include
- [x] 2.8 `ReaderService.cpp` — verify event subscription
- [x] 2.9 `LibraryService.cpp` — verify `QJsonArray` operations
- [x] 2.10 `UserService.cpp` — verify `PurchaseDto` include
- [x] 2.11 `PublisherService.cpp` — verify all `QJsonObject`/`QJsonArray` operations
- [x] 2.12 `AdminService.cpp` — verify `GetHomeSections` usage
- [x] 2.13 `ServerService.cpp` — verify `qint64` Q_INVOKABLE compatibility
- [x] 2.14 `StudySessionViewModel.cpp` — verify all includes
- [x] 2.15 `ServerViewModel.cpp` — verify `ServerService::dataChanged` exists
- [x] 2.16 `LibraryDtos.h` — verify MOC processes new `bookId` property
- [x] 2.17 `client/CMakeLists.txt` — verify `MockDataStore.cpp` is removed
- [x] 2.18 `App.qml` — verify no `_dataStore` references remain
- [x] 2.19 `UserShell.qml` — verify `dto.message` property
- [x] 2.20 `main.cpp` — verify `AuthService::instance()` static method

### Verification

```bat
cd build
cmake .. -DBOOKCLUB_BUILD_CLIENT=ON -DBOOKCLUB_BUILD_SERVER=ON
cmake --build . -j8
```

**Success**: Both executables build without errors.

---

## Phase 3: Fix Database Initialization ✅ (DONE)

Goal: Server finds `schema.sql` and creates the 21-table schema.

### Checklist

- [x] 3.1 `DatabaseManager::locateDatabaseFile` — add source-tree path as candidate
- [x] 3.2 Delete stale `bookclub.db` before first run (old schema conflicts)
- [x] 3.3 Verify seed data `ON CONFLICT` / `OR IGNORE` syntax works

### Verification

```bat
del bookclub.db
.\build\bin\BookClubServer.exe -p 8080
```

**Success**: Server logs "Database initialized" + "Schema created successfully".

---

## Phase 4: Fix QML Loading ✅ (DONE)

Goal: Client loads `App.qml` without runtime errors.

### Checklist

- [x] 4.1 Verify `BOOKCLUB_QT_QML_DIR` CMake variable matches Qt 6.11 path
- [x] 4.2 Cross-check `qml.qrc` lists every QML file
- [x] 4.3 Verify QML import URIs match `main.cpp` registrations
- [x] 4.4 Verify `theme/qmldir` is correct
- [x] 4.5 Verify all components are in the qrc

### Verification

```bat
.\build\bin\BookClubServer.exe -p 8080 &
.\build\bin\BookClubClient.exe
```

**Success**: Splash screen → welcome page → login works with `admin`/`admin`.

---

## Phase 5: Fix Runtime Data Flow ✅ (DONE)

Goal: Core user flow works end-to-end.

### Checklist

- [x] 5.1 Verify password hashes in seed data match `PasswordHasher::verify`
- [x] 5.2 Verify `GetHomeSections` returns books
- [x] 5.3 Verify `CartService::add` works post-login
- [x] 5.4 Verify `GetBooksByIds` batch fetch works
- [x] 5.5 Verify `NotificationService` event subscription doesn't crash
- [x] 5.6 Verify `BookService` event subscriptions don't crash
- [x] 5.7 Verify `ServerService` KPIs work (login as admin)
- [x] 5.8 Verify real-time events fire (publish book → notification)

### Verification

```bat
python scripts\e2e_test.py
```

**Success**: All 18 e2e test cases pass.

---

## Phase 6: Qt Creator Project Setup ✅ (DONE)

Goal: Project opens and runs cleanly from Qt Creator.

### Checklist

- [x] 6.1 Open `CMakeLists.txt` in Qt Creator — configure with Qt 6.11 MinGW kit
- [x] 6.2 Verify `CMakePresets.json` matches Qt 6.11 MinGW path
- [x] 6.3 Set working directory to project root (so `database/schema.sql` is found)
- [x] 6.4 Configure Run settings (server + client)
- [x] 6.5 Verify `windeployqt` runs as post-build step

### Verification

- Open Qt Creator → File → Open File or Project → select `CMakeLists.txt`
- Configure with Qt 6.11 MinGW kit
- Build → Run
- Login works

**Success**: App builds, runs, and core flow works from Qt Creator.

---

## Out of Scope (not needed for build & run)

These are **features** that are NOT required to just build and run:

- ThreadPool wiring (single-threaded is fine for testing)
- Async NetworkService (blocking is fine for testing)
- PDF reader (placeholder works)
- Pagination (works fine with small data)
- N+1 query fixes (works fine with few books)
- Connection-state UI (not needed for testing)
- File upload UI (not needed for testing)
- Unit tests (not needed for testing)
