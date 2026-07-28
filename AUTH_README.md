# BookCLUB — Authentication Module

Complete documentation for the BookCLUB authentication subsystem. This module
is **production-ready for the mock-backed demo** and structured so the real
backend can be dropped in by implementing `bookclub::common::IAuthService`.

---

## 1. Overview

The auth module implements the full account lifecycle:

```
   ┌─────────────┐     ┌──────────────┐     ┌─────────────────┐
   │  Register   │ ──▶ │ GenreSelect  │ ──▶ │      Login      │
   │  (5 fields) │     │  (1-3 picks) │     │  (user+pass)    │
   └─────────────┘     └──────────────┘     └────────┬────────┘
                                                   │
            ┌──────────────────────────────────────┘
            ▼
   ┌──────────────────────────────────────────────────────────┐
   │                    Authenticated session                 │
   │                                                          │
   │   Role: user | publisher | admin | server                │
   │   → routed to the matching dashboard shell               │
   └──────────────────────────────────────────────────────────┘

   Recovery flow (reachable from Login page):

   ┌──────────────┐     ┌──────────────────┐     ┌──────────────┐
   │ ForgotPwd    │ ──▶ │ SecurityQuestion │ ──▶ │ ResetPwd     │
   │ (username)   │     │ (verify answer)  │     │ (new pass)   │
   └──────────────┘     └──────────────────┘     └──────────────┘
                              │
                              ▼ issues
                       one-time reset token
                       (consumed on reset)
```

---

## 2. Architecture (MVVM)

Every auth screen follows the same MVVM split:

```
┌─────────────────────────┐        ┌──────────────────────────┐
│  QML Page (View)        │  data  │  C++ ViewModel           │
│  - LoginPage.qml        │ ◀────▶ │  - LoginViewModel        │
│  - RegisterPage.qml     │ binds  │  - RegisterViewModel     │
│  - ForgotPasswordPage   │        │  - ForgotPasswordViewModel│
│  - ResetPasswordPage    │        │  - ResetPasswordViewModel │
│  - GenreSelectionPage   │        │  - GenreSelectionViewModel│
└─────────────────────────┘        └────────────┬─────────────┘
                                                │ calls
                                                ▼
                                ┌──────────────────────────────┐
                                │  AuthService (singleton)     │
                                │  - in-memory MockUser table  │
                                │  - PasswordHasher (salt+SHA) │
                                │  - reset tokens              │
                                └──────────────────────────────┘
```

| Layer         | Files                                                              |
|---------------|-------------------------------------------------------------------|
| **View (QML)**| `client/qml/auth/*.qml` + `client/qml/layouts/AuthLayout.qml`     |
| **ViewModel** | `client/include/viewmodels/auth/*.h` + `client/src/viewmodels/auth/*.cpp` |
| **Service**   | `client/include/services/AuthService.h` + `client/src/services/AuthService.cpp` |
| **Hashing**   | `common/Utils/PasswordHasher.{h,cpp}`                             |
| **Real backend contract** | `common/Interfaces/IAuthService.h`                    |
| **Server handler** | `src/server/handlers/AuthRequestHandler.{h,cpp}`            |
| **Network client (legacy)** | `src/client/controllers/AuthController.{h,cpp}` + `src/client/session/SessionManager.{h,cpp}` |
| **Tests**     | `tests/server/test_auth.cpp`                                      |

---

## 3. Auth flows in detail

### 3.1 Registration

**Page:** `RegisterPage.qml` → **ViewModel:** `RegisterViewModel`

Fields and validation:

| Field             | Rule                                                    |
|-------------------|---------------------------------------------------------|
| `username`        | 3–20 chars, `[A-Za-z0-9_]` only, must be unique        |
| `displayName`     | 1–50 chars                                              |
| `password`        | 6–64 chars; strength meter (0–4) computed live         |
| `confirmPassword` | must equal `password`                                  |
| `securityQuestion`| picked from 5 predefined questions                      |
| `securityAnswer`  | ≥2 chars; stored hashed (case-insensitively normalised) |
| `acceptTerms`     | must be `true` to submit                               |

On submit → `AuthService::registerUser()` creates a `MockUser` with a UUID,
hashes the password (`PasswordHasher::hash` → `salt$sha256hex`), hashes the
security answer, marks `requiresGenreSetup=true`, and inserts into the
in-memory user table. On success → emits `registerSucceeded()` → router
navigates to `GenreSelectionPage`.

### 3.2 Genre selection (post-registration onboarding)

**Page:** `GenreSelectionPage.qml` → **ViewModel:** `GenreSelectionViewModel`

User picks 1–3 favourite genres from a 15-item list. The grid enforces the
hard cap of 3 (extra chips are disabled). User can also `Skip` (emits
`completed()` without saving). On submit → `AuthService::saveGenreSelection()`
persists the genres and clears `requiresGenreSetup`.

### 3.3 Login

**Page:** `LoginPage.qml` → **ViewModel:** `LoginViewModel`

Username + password (+ optional "remember me" checkbox, currently cosmetic).
On submit → `AuthService::login()`:

1. Looks up user by `username.trimmed().toLower()`.
2. If user not found → `errorMessage = "No account found with that username."`
3. If `user.status == "Blocked"` → `"This account has been blocked. Please contact support."`
4. If `PasswordHasher::verify(password, user.passwordHash)` fails → `"Incorrect password. Please try again."`
5. On success → sets `_currentUsername`, `_currentDisplayName`, `_currentRole`
   and emits the three `currentXxxChanged` signals. QML reads `currentRole`
   and routes to the matching dashboard shell (user / publisher / admin / server).

### 3.4 Forgot password (two-step)

**Page:** `ForgotPasswordPage.qml` → **ViewModel:** `ForgotPasswordViewModel`

| Step        | Action                                                    |
|-------------|-----------------------------------------------------------|
| `"username"`| User enters username. VM looks it up, loads security question, advances to `"answer"`. |
| `"answer"`  | User enters security answer. VM verifies via `AuthService::verifySecurityAnswer()` (case-insensitive, trimmed). On success → `AuthService::issueResetToken()` returns a 32-byte hex token. VM emits `recoverySucceeded(resetToken)`. |

The reset token is passed to `ResetPasswordViewModel` via the router.

### 3.5 Reset password

**Page:** `ResetPasswordPage.qml` → **ViewModel:** `ResetPasswordViewModel`

Requires `username` + `resetToken` (both set by the router from
ForgotPasswordViewModel). User enters new password + confirm. Live strength
meter + 4-requirement checklist (`minLength`, `caseMix`, `digit`, `special`).
On submit → `AuthService::resetPassword()`:

1. Verifies the token matches what was issued.
2. Re-hashes the new password and stores it.
3. **Consumes** the token (one-time use — a second reset attempt with the
   same token fails with `"Invalid or expired reset token."`).

On success → emits `resetSucceeded()` → router navigates back to LoginPage
with a success toast.

---

## 4. Security model

### 4.1 Password hashing

`bookclub::common::PasswordHasher` uses a per-password random 16-char salt
combined with SHA-256:

```
stored = salt + "$" + sha256(salt + plaintext)
```

- `verify()` splits on `$`, re-derives `sha256(salt + input)`, constant-time
  string compare.
- Salts are unique per password (same password → different stored hashes).
- **Production note:** SHA-256 is fast — fine for a prototype. For
  production, swap `PasswordHasher`'s implementation to bcrypt / argon2 /
  scrypt without changing the public API.

### 4.2 Security answers

Stored hashed with the same `PasswordHasher`, but **normalised first**:
`answer.trimmed().toLower()`. This makes answer comparison case-insensitive
and whitespace-insensitive, matching user expectations for security questions.

### 4.3 Reset tokens

- 32 random bytes → 64-char hex string.
- One-time use — consumed on successful reset.
- Stored in-memory (`QHash<QString, QString>`) — tokens do not survive
  process restart. (A production backend would persist with an expiry.)

### 4.4 Blocked users

`MockUser::status` defaults to `"Active"`. The seeded `blocked` demo account
has `status="Blocked"` and is rejected at login time with a clear error
message. The login flow never reveals whether the username exists vs. the
password is wrong for *normal* failures, but the blocked status is disclosed
by design (so the user knows to contact support).

### 4.5 Session state

`bookclub::client::SessionManager` (legacy/networked path, thread-safe
singleton with `QMutex`) tracks `userId`, `username`, `displayName`, `email`,
`role`, `sessionToken`, `loginTime`, and `sessionExpiry`. The mock-backed QML
path uses `AuthService::currentRole` / `currentUsername` /
`currentDisplayName` directly (no token needed since there's no server).

---

## 5. Demo accounts

The `AuthService` constructor seeds these accounts:

| Username   | Password      | Role      | Display name       | Security answer |
|------------|---------------|-----------|--------------------|-----------------|
| `alice`    | `password123` | user      | Alice Reader       | `whiskers`      |
| `bob`      | `password123` | user      | Bob Bibliophile    | `london`        |
| `publisher`| `password123` | publisher | Penguin Press      | `alice`         |
| `admin`    | `password123` | admin     | System Admin       | `volvo`         |
| `server`   | `password123` | server    | Server Operator    | `smith`         |
| `blocked`  | `password123` | user      | Blocked User       | `blocked`       |

The `blocked` account is rejected at login — use it to test the blocked-user
error path.

---

## 6. Tests

`tests/server/test_auth.cpp` contains a Qt Test suite covering:

- `PasswordHasher` — hash format, verify round-trip, wrong-password rejection,
  salt uniqueness, malformed-hash rejection.
- `AuthService` — login (success, wrong password, unknown user, blocked user),
  logout, registration (success, duplicate, availability, requiresGenreSetup),
  security question lookup, security answer verification (case-insensitive,
  wrong answer rejection), reset-token issuance (known/unknown user), password
  reset (valid token, invalid token, token cleared after use), and genre
  selection persistence.

**Build & run:**

```bash
cmake --preset qt-6.11-mingw-64-debug
cmake --build --preset qt-6.11-mingw-64-debug --target test_auth
./build/qt-6.11-mingw-64-debug/bin/test_auth
```

---

## 7. File manifest (auth-only)

If you want to commit **only the auth part** to GitHub, here is the complete
file list:

### 7.1 C++ / headers

```
client/include/services/AuthService.h
client/include/viewmodels/auth/AuthViewModelBase.h
client/include/viewmodels/auth/LoginViewModel.h
client/include/viewmodels/auth/RegisterViewModel.h
client/include/viewmodels/auth/ForgotPasswordViewModel.h
client/include/viewmodels/auth/ResetPasswordViewModel.h
client/include/viewmodels/auth/GenreSelectionViewModel.h

client/src/services/AuthService.cpp
client/src/viewmodels/auth/AuthViewModelBase.cpp
client/src/viewmodels/auth/LoginViewModel.cpp
client/src/viewmodels/auth/RegisterViewModel.cpp
client/src/viewmodels/auth/ForgotPasswordViewModel.cpp
client/src/viewmodels/auth/ResetPasswordViewModel.cpp
client/src/viewmodels/auth/GenreSelectionViewModel.cpp

common/Interfaces/IAuthService.h
common/Utils/PasswordHasher.h
common/Utils/PasswordHasher.cpp
common/Utils/CryptoUtils.h          # transitive dependency
common/Utils/CryptoUtils.cpp
common/Utils/IdGenerator.h          # transitive dependency
common/Utils/IdGenerator.cpp
common/Models/Genre.h
common/Models/Genre.cpp

src/server/handlers/AuthRequestHandler.h
src/server/handlers/AuthRequestHandler.cpp

src/client/controllers/AuthController.h
src/client/controllers/AuthController.cpp
src/client/session/SessionManager.h
src/client/session/SessionManager.cpp
```

### 7.2 QML pages & components

```
client/qml/auth/LoginPage.qml
client/qml/auth/RegisterPage.qml
client/qml/auth/ForgotPasswordPage.qml
client/qml/auth/ResetPasswordPage.qml
client/qml/auth/GenreSelectionPage.qml
client/qml/auth/SuccessPage.qml

client/qml/layouts/AuthLayout.qml
client/qml/components/inputs/OtpInput.qml
client/qml/components/inputs/PasswordField.qml
client/qml/components/book/GenreChip.qml
```

### 7.3 Tests

```
tests/server/test_auth.cpp
```

### 7.4 Docs

```
docs/AUTH_MODULE.md
AUTH_README.md   ← this file
```

---

## 8. How to commit to GitHub

### 8.1 First-time setup (if you don't have a repo yet)

```bash
# Inside the project root (bookCLUB/)
git init
git add .
git commit -m "feat(auth): complete authentication module

- MVVM architecture: 5 ViewModels + AuthViewModelBase
- Mocked AuthService with 6 seeded demo accounts
- Login / Register / Forgot password / Reset password / Genre selection
- PasswordHasher: salt + SHA-256, verify round-trip
- One-time reset tokens (32-byte hex, consumed on use)
- Blocked-user handling
- Qt Test suite (PasswordHasher + AuthService)
- AuthLayout.qml split-screen hero + form panel
- PasswordField with visibility toggle + strength meter
- OtpInput for future email/OTP verification
- GenreChip reusable component"
```

### 8.2 Committing only auth files to an existing repo

If the project repo already has other modules and you want to add just the
auth part as a single commit:

```bash
# Stage every auth-related path explicitly:
git add \
  client/include/services/AuthService.h \
  client/include/viewmodels/auth/ \
  client/src/services/AuthService.cpp \
  client/src/viewmodels/auth/ \
  client/qml/auth/ \
  client/qml/layouts/AuthLayout.qml \
  client/qml/components/inputs/OtpInput.qml \
  client/qml/components/inputs/PasswordField.qml \
  client/qml/components/book/GenreChip.qml \
  common/Interfaces/IAuthService.h \
  common/Utils/PasswordHasher.h \
  common/Utils/PasswordHasher.cpp \
  common/Utils/CryptoUtils.h \
  common/Utils/CryptoUtils.cpp \
  common/Utils/IdGenerator.h \
  common/Utils/IdGenerator.cpp \
  common/Models/Genre.h \
  common/Models/Genre.cpp \
  src/server/handlers/AuthRequestHandler.h \
  src/server/handlers/AuthRequestHandler.cpp \
  src/client/controllers/AuthController.h \
  src/client/controllers/AuthController.cpp \
  src/client/session/SessionManager.h \
  src/client/session/SessionManager.cpp \
  tests/server/test_auth.cpp \
  docs/AUTH_MODULE.md \
  AUTH_README.md

# Verify what's staged before committing
git status

git commit -m "feat(auth): complete authentication module (MVVM + tests)"
```

### 8.3 Suggested commit message format

Follow Conventional Commits so your history stays readable:

```
feat(auth): <short summary>

<detailed body explaining what + why>

- <bullet point 1>
- <bullet point 2>

Refs: <issue number if applicable>
```

Example:

```
feat(auth): implement complete authentication module

- MVVM architecture with AuthViewModelBase + 5 concrete VMs
- Mocked AuthService (6 seeded demo accounts, in-memory user table)
- Flows: Register → GenreSelection → Login, ForgotPassword → ResetPassword
- PasswordHasher with per-password salt + SHA-256
- One-time reset tokens (32-byte hex, consumed on use)
- Blocked-user handling
- Qt Test suite: PasswordHasher + AuthService (20+ test cases)
- Polish: split-screen AuthLayout, PasswordField w/ strength meter, OtpInput

Refs: #42
```

### 8.4 `.gitignore` essentials

The repo already includes a `.gitignore` covering build artifacts, Qt Creator
user files, and IDE directories. The auth-relevant additions:

```gitignore
# Build outputs (already in .gitignore)
build/
bin/
lib/

# Qt Creator per-user settings (NEVER commit these)
*.user
*.user.*
CMakeLists.txt.user*

# Test binaries
test_auth
test_auth.exe
```

---

## 9. Integration notes (for the real backend)

When the real server is ready, the migration path is:

1. **Implement `bookclub::common::IAuthService`** on the server side
   (the contract is already defined in `common/Interfaces/IAuthService.h`).
2. **Wire `AuthRequestHandler`** to your implementation — it already
   dispatches `Login` / `Register` / `ResetPassword` / `ChangePassword` /
   `Logout` commands from the wire protocol.
3. **On the client**: replace the mocked `AuthService` singleton with a
   network-backed implementation that talks to `AuthController` (which is
   already wired to `ClientNetworkManager`). The ViewModels do not need to
   change — they only call `AuthService` methods, which can be re-routed
   through `AuthController` → network → server.
4. **SessionManager** already exists and is thread-safe — the network path
   just needs to call `SessionManager::instance().startSession(...)` on
   successful login (already done in `AuthController::handleLoginResponse`).

The mock can stay in the tree as a test fixture / offline-mode fallback.

---

## 10. Known limitations

- **Password hashing** uses SHA-256 (fast). Acceptable for prototype; switch
  to bcrypt/argon2 for production.
- **Reset tokens** are in-memory only — they don't survive a process restart.
  A production backend should persist them with an expiry timestamp.
- **No email/OTP verification yet** — the `OtpInput` component is ready but
  not wired into any flow. Add an `OtpViewModel` + `OtpPage` when needed.
- **"Remember me"** checkbox on `LoginPage` is currently cosmetic — wire it
  to `QSettings` or similar persistent storage if you want it to actually
  pre-fill the username on next launch.
- **Rate limiting** is not implemented. The mock has no concept of failed-
  attempt counters. Add this in `AuthService::login()` (mock) and
  `AuthRequestHandler::handleLogin()` (real) before production.
