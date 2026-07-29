# BookCLUB

A production-quality Qt 6 / QML desktop application — a client-server
electronic library platform with real-time updates, role-based access
control, and a SQLite-backed data layer.

## Features

- **Three user roles**: Regular User, Publisher, Admin
- **Real-time**: live notifications, review updates, discount alerts,
  group-reading page sync via server-pushed events
- **Socket-based**: persistent TCP connection with length-prefixed JSON
  framing
- **SQL database**: 21 normalised tables with FK relationships, CHECK
  constraints, and 65 indexes
- **MVVM architecture**: QML views → C++ ViewModels → Services → NetworkService
- **Production logging**: thread-safe logger with 10 MB rotation
- **Input validation**: server-side validation on every write endpoint
- **Transactions**: checkout is wrapped in a DB transaction
- **Timed discounts**: auto-expire via a 60-second housekeeping timer

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      QML Frontend                            │
│  (auth / user / publisher / admin / server panels)           │
└──────────────────────────┬──────────────────────────────────┘
                           │ Q_PROPERTY / Q_INVOKABLE
┌──────────────────────────▼──────────────────────────────────┐
│                    C++ ViewModels                            │
│  (LoginVM, HomeVM, CartVM, PublisherVM, AdminVM, ServerVM)   │
└──────────────────────────┬──────────────────────────────────┘
                           │ calls Q_INVOKABLE methods
┌──────────────────────────▼──────────────────────────────────┐
│                      Services                                │
│  (Auth, Book, Cart, Library, Notification, Publisher, Admin) │
└──────────────────────────┬──────────────────────────────────┘
                           │ NetworkService (requestId routing)
┌──────────────────────────▼──────────────────────────────────┐
│                 ClientNetworkManager                         │
│  (TCP socket, reconnect, event subscription)                │
└──────────────────────────┬──────────────────────────────────┘
                           │ TCP / length-prefixed JSON
┌──────────────────────────▼──────────────────────────────────┐
│                       Server                                 │
│  ConnectionManager → RequestRouter → 9 handlers              │
│  + NotificationDispatcher (real-time push)                   │
│  + RequestStats (live metrics)                               │
│  + DatabaseManager (SQLite, shared connection)               │
└─────────────────────────────────────────────────────────────┘
```

## Build

### Prerequisites

- **Qt 6.5+** (Core, Gui, Network, Sql, Qml, Quick, QuickControls2)
- **CMake 3.16+**
- **C++17 compiler** (GCC 11+, Clang 14+, MSVC 2022)

### Build commands

```bash
cd bookCLUB
mkdir build && cd build
cmake ..
cmake --build . -j8
```

### Run

```bash
# Terminal 1: start the server
./bin/BookClubServer -p 8080

# Terminal 2: run the client
./bin/BookClubClient
```

### Demo accounts

| Username    | Password     | Role     |
|-------------|--------------|----------|
| `admin`     | `admin`      | Admin    |
| `publisher1`| `publisher1` | Publisher|
| `amir`      | `amir1234`   | User     |
| `sara`      | `amir1234`   | User     |

## Testing

```bash
# Start the server, then run the e2e test
python3 scripts/e2e_test.py
```

The test covers 18 cases: login (3 roles), home sections, search, book
details, batch fetch, cart add/get/checkout, library refresh, shelf
creation, notifications, publisher analytics, admin user list, permission
checks (403), and logout.

## Project structure

```
bookCLUB/
├── client/              # QML frontend + C++ ViewModels + services
│   ├── qml/             # 123 QML files (auth, user, publisher, admin, server)
│   ├── include/         # ViewModel + service headers
│   ├── src/             # ViewModel + service implementations
│   └── main.cpp         # QML entry point
├── common/              # Shared library (linked by both client + server)
│   ├── Models/          # 20 domain model classes
│   ├── Network/         # Protocol, Message, PacketParser, Serializer
│   ├── Interfaces/      # 14 repository interfaces (SQL-backed impls)
│   └── Utils/           # Logger, PasswordHasher, DbConnection, ValidationUtils
├── src/
│   ├── client/          # ClientNetworkManager (TCP socket + reconnect)
│   └── server/          # Server application
│       ├── handlers/    # 9 request handlers (Auth, Book, Cart, Library,
│       │                #   Publisher, Admin, Notification, StudySession,
│       │                #   ServerInfo)
│       ├── ServerCore   # orchestrates DB + connections + router + dispatcher
│       ├── ConnectionManager  # TCP server + client tracking
│       ├── RequestRouter      # command → handler dispatch + stats
│       ├── RequestStats       # real-time request rate / latency / errors
│       ├── NotificationDispatcher  # real-time event push
│       └── DatabaseManager    # SQLite + schema + seed loading
├── database/
│   ├── schema.sql       # 21 tables, 65 indexes, FK + CHECK constraints
│   └── seeds/sample_data.sql  # 4 users, 5 books, reviews, ratings, etc.
├── scripts/
│   ├── e2e_test.py      # 18-case end-to-end smoke test
│   ├── gen_hash.py      # password hash generator
│   ├── build_linux.sh   # Linux build script
│   └── build_windows.ps1  # Windows build script
├── installer/
│   ├── linux/           # .desktop file + install.sh
│   └── windows/         # NSIS installer script
└── docs/                # Architecture docs + UML diagrams
```

## Documentation

- `BUILD.md` — detailed build guide (Qt 6.5+, CMake, platform notes)
- `AUTH_README.md` — authentication module
- `USER_README.md` — regular user module
- `PUBLISHER_README.md` — publisher module
- `ADMIN_README.md` — admin module
- `PRODUCTION_REVIEW.md` — Step 1 code review (23 issues identified)
- `BACKEND_CHANGES.md` — backend rewrite changelog
- `FRONTEND_CHANGES.md` — frontend connection changelog

## Security

- **Password storage**: SHA-256 + per-user salt (`salt$hash` format)
- **Role-based access**: every admin/publisher command checks `requireRole`
  / `requireAnyRole` before processing
- **Input validation**: `ValidationUtils` validates username, password,
  email, phone, review text length, book title/price, etc.
- **SQL injection**: all queries use `?` placeholders (no string concat)
- **Ownership checks**: review edit/delete verifies the caller is the author
- **Transactions**: checkout is wrapped in begin/commit/rollback

## Real-time events

The server pushes 7 event types to connected clients:

| Event | Fired when | Client reaction |
|-------|-----------|-----------------|
| `EvtNotification` | any notification is created | prepend to cache, show toast |
| `EvtBookAdded` | publisher publishes a new book | refresh home feed |
| `EvtDiscountApplied` | publisher applies a discount | refresh book prices |
| `EvtReviewUpdated` | user submits/edits a review | refresh book detail page |
| `EvtStudySync` | group-reading participant turns page | sync reader page |
| `EvtUserBlocked` | admin blocks a user | (reserved) |
| `EvtServerShutdown` | server is shutting down | (reserved) |

## License

This project is a course project. No license granted for redistribution.
