# BookClubAuth

A Qt Widgets authentication demo for a book club app.

## Structure

- `main.cpp` launches the app and opens the main window.
- `include/auth` and `src/auth` contain `AuthManager`, which stores user data in `QSettings`.
- `include/windows` and `src/windows` contain the login, register, reset-password, and main window UI.
- `assets/qss/login.qss` stores the shared stylesheet.
- `assets/images`, `assets/icons`, and `assets/fonts` are kept for future visual assets.

## Build

### Qt Creator / qmake
Open `BookClubAuth.pro` and build the project normally.

### CMake
A `CMakeLists.txt` is included for Qt 6 builds as an alternative.

## Notes

The repository was cleaned for GitHub use:
- generated build artifacts were removed,
- the stylesheet was moved into `assets/qss/login.qss`,
- empty asset folders are preserved with `.gitkeep` files.
