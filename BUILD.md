# BookCLUB — Build Guide

This document explains how to configure, build, and run the BookCLUB Qt6/QML
client + headless C++ server. It is the single source of truth for build
options, supported Qt versions, and platform-specific notes.

---

## 1. Prerequisites

### 1.1 Qt 6.5 or newer (tested up to 6.11.1)

You need a working Qt 6 installation. The project only depends on modules
that ship with the **standard Qt 6 installer**:

| Module         | Required? | Notes                                              |
|----------------|-----------|----------------------------------------------------|
| `Core`         | YES       |                                                    |
| `Gui`          | YES       |                                                    |
| `Network`      | YES       |                                                    |
| `Sql`          | YES       |                                                    |
| `Qml`          | YES (client) |                                                |
| `Quick`        | YES (client) |                                                |
| `QuickControls2` | YES (client) |                                              |
| `ShaderTools`  | optional  | Only if you add `.qsb` shader resources later.     |
| `Qt5Compat`    | optional  | Only if you set `BOOKCLUB_USE_QT5COMPAT=ON`.       |

> **If you are seeing the error**
> `Qt packages are missing — Found package configuration does NOT exist:
>  C:/Qt/6.11.1/llvm-mingw_64/lib/cmake/Qt6Qt5Compat/Qt6Qt5CompatConfig.cmake`
> — you are on a stock Qt 6.11.1 install that does NOT have the Qt5Compat
> add-on module. **This is fine.** Just configure the project with the
> default options. `BOOKCLUB_USE_QT5COMPAT` defaults to `OFF`, so Qt5Compat
> is never required, and the QML client transparently uses the Qt6-native
> `MultiEffect` for drop shadows instead.

### 1.2 CMake ≥ 3.21

Older CMake also works (≥ 3.16) but you will lose the bundled
`CMakePresets.json` feature.

### 1.3 A C++17 compiler

Any of:

- **Windows**: LLVM-MinGW (`clang`/`clang++` from `C:/Qt/Tools/llvm-mingw_64/bin/`)
  or MSVC 2022 64-bit.
- **Linux**: GCC ≥ 11 or Clang ≥ 14.
- **macOS**: Xcode 14+ (Apple Clang).

### 1.4 Ninja (recommended generator)

`CMakePresets.json` uses `Ninja` by default. The Qt installer ships Ninja at
`C:/Qt/Tools/Ninja/ninja.exe` on Windows. If you don't have it, install via
`pip install ninja`, `choco install ninja`, or your distro's package manager.

---

## 2. Quick start (Qt Creator — recommended)

1. **Open** `File → Open File or Project…` and select `CMakeLists.txt` in
   the project root.
2. Qt Creator reads `CMakePresets.json` and shows a list of configurations.
   Pick the one matching your installed Qt:
   - **Windows + Qt 6.11.1 + LLVM-MinGW**: choose
     `Qt 6.11.1 LLVM-MinGW 64-bit (Debug)`.
   - **Windows + Qt 6.11 + MSVC 2022**: choose
     `Qt 6.11 MSVC 2022 64-bit (Debug)`.
   - **Linux**: choose `Linux GCC (Debug)`.
   - **macOS**: choose `macOS (Debug)`.
3. Click **Configure Project**. CMake runs once, then Qt Creator loads the
   project tree.
4. Click **Run** (Ctrl+R) — the BookClubClient executable is built and
   launched.

> If Qt Creator doesn't show the presets, make sure `CMakePresets.json`
> is present in the project root and that your Qt Creator version is
> ≥ 9.0 (older versions don't support presets).
>
> **If you see "No CMAKE_CXX_COMPILER could be found"**, your Qt Creator
> Kit is missing a compiler binding — skip straight to **§5.0** below for
> the step-by-step fix. The project itself is fine; only the Kit needs
> configuring.

---

## 3. Quick start (command line)

### 3.1 Windows + Qt 6.11.1 LLVM-MinGW 64-bit

```bat
:: 1) Make sure Qt, Ninja and the MinGW compiler are on PATH
set PATH=C:\Qt\6.11.1\llvm-mingw_64\bin;C:\Qt\Tools\llvm-mingw_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\CMake_64\bin;%PATH%

:: 2) Configure (use the matching preset)
cmake --preset qt-6.11-mingw-64-debug

:: 3) Build
cmake --build --preset qt-6.11-mingw-64-debug

:: 4) Run
build\qt-6.11-mingw-64-debug\bin\BookClubClient.exe
```

### 3.2 Windows + Qt 6.11 MSVC 2022 64-bit

```bat
:: Open the "x64 Native Tools Command Prompt for VS 2022" first.
set PATH=C:\Qt\6.11.1\msvc2022_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;%PATH%

cmake --preset qt-6.11-msvc-2022-64-debug
cmake --build --preset qt-6.11-msvc-2022-64-debug
```

### 3.3 Linux + GCC

```bash
export CMAKE_PREFIX_PATH=/opt/Qt/6.11.1/gcc_64    # or wherever Qt is installed
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
./build/linux-gcc-debug/bin/BookClubClient
```

### 3.4 macOS

```bash
export CMAKE_PREFIX_PATH=/opt/Qt/6.11.1/macos
cmake --preset macos-debug
cmake --build --preset macos-debug
./build/macos-debug/bin/BookClubClient.app/Contents/MacOS/BookClubClient
```

---

## 4. Build options

All options are exposed as CMake cache variables. Override them on the
command line with `-D<option>=<value>` or in `CMakePresets.json`.

| Option                       | Default | Description                                                              |
|------------------------------|---------|--------------------------------------------------------------------------|
| `BOOKCLUB_BUILD_CLIENT`      | `ON`    | Build the QML client (`BookClubClient`).                                 |
| `BOOKCLUB_BUILD_SERVER`      | `ON`    | Build the headless C++ server (`BookClubServer`).                        |
| `BOOKCLUB_USE_QT5COMPAT`     | `OFF`   | When `ON`, link Qt5Compat and enable the legacy `Qt5Compat.GraphicalEffects` shadow path. Falls back to `MultiEffect` automatically if Qt5Compat is not installed. |

### Examples

**Client only (skip server build):**
```bash
cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_BUILD_SERVER=OFF
```

**Enable legacy Qt5Compat shadows (requires Qt5Compat module installed):**
```bash
cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_USE_QT5COMPAT=ON
```

---

## 5. Troubleshooting

### 5.0 "No CMAKE_CXX_COMPILER could be found" (Qt Creator)

This is **the most common error** when first opening the project in Qt Creator.
It means CMake could not find a C++ compiler. **The project is fine** — your
Qt Creator Kit is missing a compiler binding.

**Step-by-step fix:**

1. In Qt Creator, open **Edit → Preferences → Kits** (Windows/Linux) or
   **Qt Creator → Settings → Kits** (macOS).
2. Select the Kit you're using (e.g. `Desktop Qt 6.11.1 LLVM-MinGW 64-bit`).
   Make sure it has **no red warning icon**.
3. Check the **Compiler** tab inside the Kit:
   - **C++ compiler** must point to a real `.exe` — typically
     `C:\Qt\Tools\llvm-mingw_64\bin\clang++.exe` (LLVM-MinGW) **or**
     `C:\Qt\Tools\mingw1310_64\bin\g++.exe` (GCC MinGW).
   - If the field is empty or red, click the dropdown and pick the compiler.
   - If the dropdown is empty, you need to install a compiler — see step 7.
4. Check the **Qt version** field inside the Kit:
   - Should point at `C:\Qt\6.11.1\llvm-mingw_64\bin\qmake.exe` (or
     `.../mingw_64/bin/qmake.exe` for GCC MinGW).
5. Check the **CMake** field inside the Kit:
   - Should point at `C:\Qt\Tools\CMake_64\bin\cmake.exe`.
6. Check the **Ninja** field inside the Kit:
   - Should point at `C:\Qt\Tools\Ninja\ninja.exe`.
7. If no compiler is shown in step 3, you need to install one:
   - Open the **Qt Maintenance Tool** (in `C:\Qt\`).
   - Choose **Add or remove components**.
   - Expand **Qt → Developer and Designer Tools**.
   - Tick **MinGW 13.2.0 64-bit** (or the matching LLVM-MinGW entry for your
     Qt version) and install.
   - Restart Qt Creator.
8. After the Kit is green, **delete the stale build directory**:
   ```bat
   rmdir /s /q build
   ```
9. In Qt Creator: **Build → Run CMake** (or just close and reopen the
   project). The configure step should now succeed.

> **Important**: A Qt installation with the `llvm-mingw_64` architecture
> REQUIRES the LLVM-MinGW compiler. A `mingw_64` (GCC) Qt installation
> REQUIRES the GCC MinGW compiler. **They are NOT interchangeable.**
> Check the folder name under `C:\Qt\6.11.1\` — that's your architecture.

### 5.1 "Qt packages are missing … Qt6Qt5CompatConfig.cmake"

**Cause**: your Qt installation does not include the `Qt 5 Compatibility`
add-on module. The original `CMakeLists.txt` listed `Qt5Compat` as a
**required** Qt6 component, so the configure step aborted.

**Fix**: Already fixed in this version. `Qt5Compat` is now optional and
defaults to OFF. Just reconfigure with the matching preset:

```bat
cmake --preset qt-6.11-mingw-64-debug
```

If you previously tried to configure and a stale `build/` directory exists,
**delete it first**:

```bat
rmdir /s /q build
cmake --preset qt-6.11-mingw-64-debug
```

### 5.2 "No CMake configuration for build type 'Debug' found"

This is a downstream symptom of any configure-step failure. The real error
appears earlier in the **General Messages** panel — usually one of:

- Missing Qt5Compat module (see 5.1)
- Wrong `CMAKE_PREFIX_PATH` (Qt not found)
- Wrong compiler (e.g. 32-bit MinGW used against a 64-bit Qt)

Fix the underlying error, delete `build/`, then re-configure.

### 5.3 The QML client runs but drop shadows look different

The new default uses Qt6-native `MultiEffect` (from `QtQuick.Effects`,
available since Qt 6.5). It produces a slightly softer shadow than the
legacy `Qt5Compat.GraphicalEffects.DropShadow`. If you prefer the legacy
look, install the `Qt 5 Compatibility Module` via the Qt Maintenance Tool
and reconfigure with `-DBOOKCLUB_USE_QT5COMPAT=ON`.

### 5.4 "windeployqt not found" or runtime DLL errors on Windows

If you build with a shared Qt (the default on Windows), the .exe needs
the Qt DLLs next to it. The CMake script automatically invokes
`windeployqt` as a post-build step when it's discoverable.

If `windeployqt` isn't on PATH, add `C:\Qt\6.11.1\llvm-mingw_64\bin`
to your `PATH` and rebuild.

### 5.5 Qt Creator keeps re-running CMake on every build

This is normal the first time, but if it happens on every keystroke,
check that the `binaryDir` in the chosen preset matches the path Qt
Creator is using. Delete the build directory and let Qt Creator
re-configure once with the preset.

---

## 6. Project layout (build-relevant only)

```
bookCLUB/
├── CMakeLists.txt              ← top-level: options + find_package(Qt6 …)
├── CMakePresets.json           ← Windows/Linux/macOS presets
├── BUILD.md                    ← this file
├── .gitignore
├── common/                     ← static lib: DTOs, network protocol, utils
│   └── CMakeLists.txt
├── client/                     ← QML client (BookClubClient)
│   ├── CMakeLists.txt          ← Qt6 modules + optional Qt5Compat link
│   ├── main.cpp                ← QML entry point
│   ├── include/                ← viewmodels + services headers
│   ├── src/                    ← viewmodels + services implementations
│   ├── qml/                    ← QML UI
│   │   └── components/effects/DropShadowBase.qml  ← MultiEffect-based shadow
│   └── resources/              ← qml.qrc + fonts.qrc
└── src/server/                 ← headless C++ server (BookClubServer)
    └── CMakeLists.txt
```

---

## 7. Cleaning up

```bash
# Remove all build outputs
rm -rf build/        # Linux / macOS
rmdir /s /q build    # Windows
```

Then re-configure from a clean slate.

---

## 8. Reporting a build issue

When reporting a build issue, attach:

1. The **exact** `cmake` command you ran.
2. The full output of `cmake --version` and `qmake --version` (or
   `qmake6 --version`).
3. The contents of `build/<preset>/CMakeCache.txt` (or the relevant
   excerpt).
4. The complete CMake error log — *not* just the last line.
