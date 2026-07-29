# BookClub — Authentication Module

A production-ready Qt5/QML authentication module built on the MVVM pattern.
The visual language follows a modern, minimal, monochrome-with-accent design
inspired by Stripe / Linear / Vercel auth screens.

---

## 1. Overview

The Authentication module implements every screen, state, and component
required by the project's `UserAccount` model:

| Page                  | File                                       | ViewModel                    |
| --------------------- | ------------------------------------------ | ---------------------------- |
| Splash                | `client/qml/auth/SplashPage.qml`           | —                            |
| Welcome               | `client/qml/auth/WelcomePage.qml`          | —                            |
| Login                 | `client/qml/auth/LoginPage.qml`            | `LoginViewModel`             |
| Register              | `client/qml/auth/RegisterPage.qml`         | `RegisterViewModel`          |
| Forgot password       | `client/qml/auth/ForgotPasswordPage.qml`   | `ForgotPasswordViewModel`    |
| Reset password        | `client/qml/auth/ResetPasswordPage.qml`    | `ResetPasswordViewModel`     |
| Genre selection       | `client/qml/auth/GenreSelectionPage.qml`   | `GenreSelectionViewModel`    |
| Success               | `client/qml/auth/SuccessPage.qml`          | —                            |

The module intentionally **excludes** Google Sign-In, social login, and
email-based login — per the project requirements. Only the username +
password flow defined by the existing `UserAccount` model is implemented.

---

## 2. Design system

Every design token lives in a single QML singleton at
`client/qml/theme/Theme.qml`:

| Category      | Examples                                                                 |
| ------------- | ------------------------------------------------------------------------ |
| Color         | `pageBackground`, `cardBackground`, `primary`, `accent`, `textPrimary`, `textSecondary`, `textMuted`, `border`, `success`, `warning`, `error`, `info`, … |
| Typography    | `family`, `familyMono`, `familyIcon`, `sizeCaption`…`sizeHero`, `weightRegular`…`weightBold`, `trackingTight`…`trackingXWide` |
| Spacing (8px) | `xxs`…`mega`                                                              |
| Radius        | `none`, `xs`, `sm`, `md`, `lg`, `xl`, `xxl`, `pill`                       |
| Shadow        | `sm`, `md`, `lg`, `xl`                                                    |
| Size          | `fieldHeight`, `buttonHeight`, `iconSm`…`iconXl`, `logoSize`, `cardMaxWidth` |
| Motion        | `durationFast`…`durationPage`, `easeStandard`…`easePage`                 |
| Z-index       | `base`, `card`, `sticky`, `drawer`, `modal`, `toast`, `tooltip`           |

### Color summary

| Role              | Hex        |
| ----------------- | ---------- |
| Page background   | `#F4F5F7`  |
| Card background   | `#FFFFFF`  |
| Primary (brand)   | `#0A0A0B`  |
| Accent            | `#1A73E8`  |
| Text primary      | `#0A0A0B`  |
| Text secondary    | `#5F6368`  |
| Text muted        | `#9AA0A6`  |
| Border            | `#E2E4E8`  |
| Success           | `#1E8E3E`  |
| Warning           | `#F29900`  |
| Error             | `#D93025`  |

### Typography

A curated sans-serif stack (`Inter, SF Pro Display, SF Pro Text, Segoe UI,
Roboto, Helvetica Neue, Arial`) — falls back gracefully across Windows,
macOS, and Linux.

---

## 3. Icons — Material Symbols

All icons across the module are Google **Material Symbols Outlined**.
The `AppIcon.qml` component maps canonical icon names (`lock`, `visibility`,
`arrow_back`, …) to their codepoints in a single lookup table, keeping
"magic strings" out of the page code.

### Enabling icon rendering

The font file is intentionally **not** bundled (license review required).
To enable icons:

1. Download `MaterialSymbolsOutlined-Regular.ttf` from
   <https://github.com/google/material-design-icons>
2. Save it as `client/resources/fonts/MaterialSymbolsOutlined-Regular.ttf`
3. Uncomment the `<file>` entry in `client/resources/fonts.qrc`
4. Rebuild — `main.cpp` loads the font via `QFontDatabase`.

Without the font, glyphs render as the `.notdef` box — the rest of the
module remains fully functional.

---

## 4. Components

All reusable components live under `client/qml/components/` and share the
same visual language:

```
components/
├── AppIcon.qml                — Material Symbols glyph
├── effects/
│   └── DropShadowBase.qml     — wrapper around QtGraphicalEffects.DropShadow
├── buttons/
│   ├── PrimaryButton.qml      — solid black, white text, loading + focus ring
│   ├── SecondaryButton.qml    — white, bordered
│   ├── IconButton.qml         — circular, transparent
│   └── TextButton.qml         — inline link
├── inputs/
│   ├── InputField.qml         — labelled text field with helper/error states
│   ├── PasswordField.qml      — InputField + visibility toggle + strength meter
│   ├── OtpInput.qml           — N-cell OTP/verification code input
│   └── SearchField.qml        — search-styled input with clear button
├── selection/
│   ├── AppCheckbox.qml        — square checkbox + label
│   ├── AppRadioButton.qml     — circular radio + label
│   └── AppToggleButton.qml    — pill-shaped switch
├── surfaces/
│   ├── Card.qml               — rounded white card with elevation
│   └── Divider.qml            — horizontal/vertical separator (optional label)
├── feedback/
│   ├── ValidationMessage.qml  — inline icon + text row
│   ├── Toast.qml              — single toast notification
│   ├── ToastManager.qml       — host for the toast stack
│   └── ConfirmationPopup.qml  — modal confirm dialog
├── progress/
│   ├── Spinner.qml            — circular indeterminate/determinate
│   ├── ProgressBar.qml        — linear indeterminate/determinate
│   └── LoadingOverlay.qml     — full-card translucent blocking overlay
└── branding/
    ├── BrandLogo.qml          — hexagonal BookClub mark with book glyph
    └── SecurityBadge.qml      — pill-shaped "Secure & Private" badge
```

Every component handles all standard states — `normal`, `hover`, `pressed`,
`disabled`, `focus`, `loading`, `error`, `success`, `warning` — and uses
subtle `ColorAnimation` / `NumberAnimation` transitions (140–220 ms).

---

## 5. MVVM architecture

```
┌─────────────────────────┐     Q_PROPERTY / Q_INVOKABLE     ┌──────────────────────────┐
│  QML View               │ ──────────────────────────────▶  │  C++ ViewModel           │
│  (LoginPage.qml)        │ ◀──────────────────────────────  │  (LoginViewModel)        │
└─────────────────────────┘           signals                 └────────────┬─────────────┘
                                                                            │ uses
                                                                            ▼
                                                                  ┌────────────────────┐
                                                                  │  AuthService       │
                                                                  │  (mocked, in-proc) │
                                                                  └────────────────────┘
```

### AuthViewModelBase

`AuthViewModelBase` provides shared infrastructure:

- `authService` (Q_PROPERTY) — injected from QML, shared singleton instance
- `isSubmitting`, `canSubmit`, `formError` — common state
- `submit()` (Q_INVOKABLE) — begins a mocked async operation; after a 900 ms
  latency, calls `_doSubmit()` (subclass hook)
- `reset()` (Q_INVOKABLE) — clears all state via `_doReset()` (subclass hook)
- `_onAuthServiceChanged()` — virtual hook for subclasses to load reference
  data (security questions, genres) when the AuthService is attached

### Per-screen ViewModels

| ViewModel                  | Key properties                                                                                                          | Signals                       |
| -------------------------- | ----------------------------------------------------------------------------------------------------------------------- | ----------------------------- |
| `LoginViewModel`           | `username`, `password`, `rememberMe`, `usernameError`, `passwordError`                                                  | `loginSucceeded`, `loginFailed`           |
| `RegisterViewModel`        | `username`, `displayName`, `password`, `confirmPassword`, `securityQuestion`, `securityAnswer`, `acceptTerms`, `usernameAvailable`, `passwordStrength`, `strengthLabel`, `availableSecurityQuestions` | `registerSucceeded`, `registerFailed`     |
| `ForgotPasswordViewModel`  | `step`, `username`, `securityQuestion`, `securityAnswer`, `resetToken`, `usernameError`, `securityAnswerError`           | `recoverySucceeded`, `recoveryFailed`     |
| `ResetPasswordViewModel`   | `username`, `resetToken`, `password`, `confirmPassword`, `passwordStrength`, `strengthLabel`, `requirementsStatus`, `passwordError`, `confirmPasswordError` | `resetSucceeded`, `resetFailed`           |
| `GenreSelectionViewModel`  | `availableGenres`, `selectedGenres`, `selectedCount`, `minSelection`, `canGoBack`                                       | `completed`                              |

### Validation

Frontend validation reuses the existing `bookclub::common::ValidationUtils`
(3–20 char username, 6–64 char password, strong-password check for the
strength meter). Every field exposes a `validateX()` Q_INVOKABLE so QML
can trigger validation on each `onTextEdited` event. Submit re-validates
the whole form before calling the AuthService.

### Mocked AuthService

`client/include/services/AuthService.h` declares a self-contained in-memory
auth service — no network or DB required. It:

- Seeds two demo accounts:
  - `alice` / `password123` (security answer: `whiskers`)
  - `bob`   / `password123` (security answer: `london`)
- Hashes passwords via the existing `PasswordHasher` (SHA-256 + per-user salt)
- Issues random 32-byte hex reset tokens
- Stores selected genres and exposes `requiresGenreSetup(username)`

To swap in a real backend, replace `AuthService.cpp` with an implementation
that hits the project's `ClientNetworkManager` — the ViewModels require no
changes.

---

## 6. Build & run

### Prerequisites

- Qt 5.15+ with modules: `Core`, `Gui`, `Network`, `Sql`, `Qml`, `Quick`,
  `QuickControls2` (and `QtGraphicalEffects`, which ships with Qt5)
- CMake 3.16+
- C++17 compiler

### Build

```bash
cd bookCLUB
mkdir build && cd build
cmake ..
cmake --build . -j8
```

### Run

```bash
./bin/BookClubClient
```

### Demo accounts

| Username | Password       | Security answer |
| -------- | -------------- | --------------- |
| `alice`  | `password123`  | `whiskers`      |
| `bob`    | `password123`  | `london`        |

---

## 7. Project structure

```
bookCLUB/
├── CMakeLists.txt                  # top-level — finds Qt5 modules
├── client/
│   ├── main.cpp                    # QML engine setup + VM registration
│   ├── include/
│   │   ├── services/AuthService.h
│   │   └── viewmodels/auth/
│   │       ├── AuthViewModelBase.h
│   │       ├── LoginViewModel.h
│   │       ├── RegisterViewModel.h
│   │       ├── ForgotPasswordViewModel.h
│   │       ├── ResetPasswordViewModel.h
│   │       └── GenreSelectionViewModel.h
│   ├── src/
│   │   ├── services/AuthService.cpp
│   │   └── viewmodels/auth/*.cpp
│   ├── qml/
│   │   ├── App.qml                 # root window + router + VMs + Toast host
│   │   ├── theme/                  # Theme singleton + qmldir
│   │   ├── components/             # 18 reusable components (see §4)
│   │   ├── layouts/AuthLayout.qml  # split-screen card layout
│   │   └── auth/                   # 8 auth pages
│   └── resources/
│       ├── qml.qrc                 # registers every QML file
│       ├── fonts.qrc               # placeholder for Material Symbols font
│       └── Client.qrc              # bundles the above
└── src/client/CMakeLists.txt       # client build target (QML + VMs + main)
```

---

## 8. What's left for future work

- **Main dashboard** — after a successful login / registration, the router
  currently shows a success toast. Wire `onLoginSuccess` /
  `onCompleted` to push the dashboard StackView item once implemented.
- **Real backend** — replace `AuthService.cpp` with a network-backed
  implementation that uses `ClientNetworkManager` to call the server's
  `AuthRequestHandler`. ViewModel API stays unchanged.
- **Theme dark mode** — the `Theme` singleton currently exposes a single
  light palette. Add a `dark` variant and wire it to `ThemeManager` for
  runtime switching.
- **Localization** — strings are currently hard-coded English; port to
  `qsTr()` and integrate with `LocalizationManager`.
- **Material Symbols font** — bundle the font (with license review) so
  icons render out of the box without a manual setup step.
