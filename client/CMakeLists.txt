# =============================================================================
#  client/CMakeLists.txt
# =============================================================================
#  BookClub Qt6 client — QML + MVVM build target.
#
#  Compiles:
#    * All QML files (via qml.qrc + fonts.qrc)
#    * Auth + User ViewModels (MVVM, registered with QML via qmlRegisterType)
#    * Mock AuthService + 6 user-role services (QML singletons)
#    * Shared DTO wrappers
#    * Legacy socket-based controllers + ClientNetworkManager + SessionManager
#      (kept for parity, not used by the QML client today)
#    * main.cpp — the QML entry point
#
#  Required Qt6 modules: Core, Gui, Quick, QuickControls2, Qml, Network, Sql
#  Optional Qt6 modules:
#    * Qt5Compat       — used ONLY when BOOKCLUB_USE_QT5COMPAT=ON, to get the
#                        legacy Qt5Compat.GraphicalEffects DropShadow. When
#                        absent, DropShadowBase.qml falls back to MultiEffect
#                        from QtQuick.Effects (Qt 6.5+).
#    * ShaderTools     — only needed if the project ever adds .qsb shaders.
# =============================================================================

project(bookclub_client)

# QML/MVVM tree lives under client/
set(CLIENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Legacy socket tree lives under src/client/ (controllers, network, session)
set(LEGACY_SRC_DIR ${CMAKE_SOURCE_DIR}/src/client)

# ---- Sources: View models (MVVM) ----
set(CLIENT_SOURCES
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/AuthViewModelBase.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/LoginViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/RegisterViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/ForgotPasswordViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/ResetPasswordViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/GenreSelectionViewModel.cpp

    ${CLIENT_SRC_DIR}/src/viewmodels/user/UserViewModelBase.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/HomeViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/SearchViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/BookDetailViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/CartViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/LibraryViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/ReaderViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/NotificationsViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/ProfileViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/WishlistViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/ShelfViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/user/StudySessionViewModel.cpp

    # Role ViewModels (Publisher / Admin / Server)
    ${CLIENT_SRC_DIR}/src/viewmodels/publisher/PublisherViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/admin/AdminViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/server/ServerViewModel.cpp

    # Services
    ${CLIENT_SRC_DIR}/src/services/AuthService.cpp
    ${CLIENT_SRC_DIR}/src/services/NetworkService.cpp
    ${CLIENT_SRC_DIR}/src/services/BookService.cpp
    ${CLIENT_SRC_DIR}/src/services/CartService.cpp
    ${CLIENT_SRC_DIR}/src/services/LibraryService.cpp
    ${CLIENT_SRC_DIR}/src/services/NotificationService.cpp
    ${CLIENT_SRC_DIR}/src/services/ReaderService.cpp
    ${CLIENT_SRC_DIR}/src/services/UserService.cpp
    ${CLIENT_SRC_DIR}/src/services/PublisherService.cpp
    ${CLIENT_SRC_DIR}/src/services/AdminService.cpp
    ${CLIENT_SRC_DIR}/src/services/ServerService.cpp
    ${CLIENT_SRC_DIR}/src/services/BookDto.cpp
    ${CLIENT_SRC_DIR}/src/services/LibraryDtos.cpp

    # Legacy socket-based controllers (kept for parity)
    ${LEGACY_SRC_DIR}/controllers/AuthController.cpp
    ${LEGACY_SRC_DIR}/controllers/BookCatalogController.cpp
    ${LEGACY_SRC_DIR}/controllers/CartController.cpp
    ${LEGACY_SRC_DIR}/controllers/LibraryController.cpp
    ${LEGACY_SRC_DIR}/controllers/PublisherController.cpp
    ${LEGACY_SRC_DIR}/controllers/AdminController.cpp
    ${LEGACY_SRC_DIR}/controllers/NotificationController.cpp
    ${LEGACY_SRC_DIR}/controllers/ReaderController.cpp
    ${LEGACY_SRC_DIR}/controllers/StudySessionController.cpp

    # Networking / session (legacy)
    ${LEGACY_SRC_DIR}/network/ClientNetworkManager.cpp
    ${LEGACY_SRC_DIR}/session/SessionManager.cpp

    # QML entry point
    ${CLIENT_SRC_DIR}/main.cpp
)

# ---- Headers ----
set(CLIENT_HEADERS
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/AuthViewModelBase.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/LoginViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/RegisterViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/ForgotPasswordViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/ResetPasswordViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/GenreSelectionViewModel.h

    ${CLIENT_SRC_DIR}/include/viewmodels/user/UserViewModelBase.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/HomeViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/SearchViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/BookDetailViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/CartViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/LibraryViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/ReaderViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/NotificationsViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/ProfileViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/WishlistViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/ShelfViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/user/StudySessionViewModel.h

    # Role ViewModels
    ${CLIENT_SRC_DIR}/include/viewmodels/publisher/PublisherViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/admin/AdminViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/server/ServerViewModel.h

    ${CLIENT_SRC_DIR}/include/services/AuthService.h
    ${CLIENT_SRC_DIR}/include/services/NetworkService.h
    ${CLIENT_SRC_DIR}/include/services/BookService.h
    ${CLIENT_SRC_DIR}/include/services/CartService.h
    ${CLIENT_SRC_DIR}/include/services/LibraryService.h
    ${CLIENT_SRC_DIR}/include/services/NotificationService.h
    ${CLIENT_SRC_DIR}/include/services/ReaderService.h
    ${CLIENT_SRC_DIR}/include/services/UserService.h
    ${CLIENT_SRC_DIR}/include/services/PublisherService.h
    ${CLIENT_SRC_DIR}/include/services/AdminService.h
    ${CLIENT_SRC_DIR}/include/services/ServerService.h
    ${CLIENT_SRC_DIR}/include/services/BookDto.h
    ${CLIENT_SRC_DIR}/include/services/ReviewDto.h
    ${CLIENT_SRC_DIR}/include/services/CartItemDto.h
    ${CLIENT_SRC_DIR}/include/services/LibraryDtos.h
    ${CLIENT_SRC_DIR}/include/services/RatingDistDto.h
    ${CLIENT_SRC_DIR}/include/services/FilterChipDto.h

    # Legacy controllers / network / session headers
    ${LEGACY_SRC_DIR}/controllers/AuthController.h
    ${LEGACY_SRC_DIR}/controllers/BookCatalogController.h
    ${LEGACY_SRC_DIR}/controllers/CartController.h
    ${LEGACY_SRC_DIR}/controllers/LibraryController.h
    ${LEGACY_SRC_DIR}/controllers/PublisherController.h
    ${LEGACY_SRC_DIR}/controllers/AdminController.h
    ${LEGACY_SRC_DIR}/controllers/NotificationController.h
    ${LEGACY_SRC_DIR}/controllers/ReaderController.h
    ${LEGACY_SRC_DIR}/controllers/StudySessionController.h
    ${LEGACY_SRC_DIR}/network/ClientNetworkManager.h
    ${LEGACY_SRC_DIR}/session/SessionManager.h
)

# ---- Resources (QML + fonts) ----
set(CLIENT_RESOURCES
    ${CLIENT_SRC_DIR}/qml/qml.qrc
    ${CLIENT_SRC_DIR}/resources/fonts.qrc
)

# ---- Build Executable ----
add_executable(BookClubClient
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
    ${CLIENT_RESOURCES}
)

# ---- Qt6 modules (REQUIRED) ----
# Note: Core/Network/Sql are already found by the top-level CMakeLists.txt.
# We re-issue find_package here so this directory can also be configured
# standalone (e.g. inside Qt Creator with a single-subdir setup).
#
# v14: Pdf + PdfQuick are required for the in-app PDF reader
# (QtQuick.Pdf's PdfDocument + PdfMultiPageView used by PdfReaderPage.qml).
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Sql
    Pdf
    PdfQuick
)

# ---- Qt6 modules (OPTIONAL) ----
# Qt5Compat and ShaderTools are looked up silently. They enable extra
# features but are NOT required to build the client.
find_package(Qt6 COMPONENTS Qt5Compat)
find_package(Qt6 COMPONENTS ShaderTools)

set_target_properties(BookClubClient PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    OUTPUT_NAME "BookClubClient"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ---- Link Libraries (always required) ----
target_link_libraries(BookClubClient
    bookclub_common
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Network
    Qt6::Sql
    Qt6::Pdf
    Qt6::PdfQuick
)

# ---- Optional: Qt5Compat link + compile flag ----
# Only link when present AND when the user explicitly opted in
# (BOOKCLUB_USE_QT5COMPAT=ON). When OFF, DropShadowBase.qml uses the
# Qt6-native MultiEffect path which requires NO extra modules.
if(BOOKCLUB_USE_QT5COMPAT AND Qt6_Qt5Compat_FOUND)
    target_link_libraries(BookClubClient Qt6::Qt5Compat)
    target_compile_definitions(BookClubClient PRIVATE BOOKCLUB_USE_QT5COMPAT)
    message(STATUS "BookClubClient: Qt5Compat.GraphicalEffects shadows ENABLED.")
else()
    message(STATUS "BookClubClient: using Qt6-native MultiEffect shadows (Qt5Compat optional, not used).")
endif()

# ---- Optional: ShaderTools ----
if(BOOKCLUB_HAVE_SHADER_TOOLS OR Qt6_ShaderTools_FOUND)
    # No shaders are currently added; this is forward-compatibility only.
endif()

# ---- Include Directories ----
target_include_directories(BookClubClient PRIVATE
    ${CLIENT_SRC_DIR}
    ${CLIENT_SRC_DIR}/include
    ${LEGACY_SRC_DIR}
    ${CMAKE_SOURCE_DIR}
)

# Bake the source directory into the binary so the client can find
# database files if needed (e.g., DbConnection fallback).
target_compile_definitions(BookClubClient PRIVATE
    BOOKCLUB_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# ---- Pass Qt's QML imports path as a compile-time constant ----
# windeployqt creates a qt.conf next to the exe that overrides
# QLibraryInfo paths. To make sure the QML engine can always find
# QtQuick, QtQuick.Controls, etc., we ask CMake for the real Qt
# QML directory and bake it into the binary.
get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
# QML modules live in <qt>/qml/ — one level up from bin/
get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
set(_qt_qml_dir "${_qt_root_dir}/qml")
if(EXISTS "${_qt_qml_dir}")
    target_compile_definitions(BookClubClient PRIVATE
        BOOKCLUB_QT_QML_DIR="${_qt_qml_dir}"
    )
    message(STATUS "BookClubClient: Qt QML dir = ${_qt_qml_dir}")
else()
    message(WARNING "BookClubClient: Qt QML dir not found at ${_qt_qml_dir}")
endif()

# ---- Windows: deploy runtime DLLs next to the executable ----
# When building on Windows with a shared Qt, the .exe needs the Qt
# DLLs (and the MinGW runtime) next to it to run from Qt Creator.
# windeployqt automatically copies them into the bin/ folder.
if(WIN32)
    # Locate windeployqt.exe from the Qt6 prefix.
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET BookClubClient POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    --no-system-d3d-compiler
                    --no-opengl-sw
                    --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
                    "$<TARGET_FILE:BookClubClient>"
            COMMENT "Running windeployqt on BookClubClient (with QML deployment)")
    else()
        message(STATUS "windeployqt not found; skipping DLL deployment. "
                       "Add Qt's bin/ directory to PATH to run BookClubClient.")
    endif()
endif()

# ---- macOS bundle hints (optional) ----
if(APPLE)
    set_target_properties(BookClubClient PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.bookclub.client"
    )
endif()

# ---- Linux desktop file (optional) ----
if(UNIX AND NOT APPLE)
    install(FILES ${CMAKE_SOURCE_DIR}/installer/linux/bookclub.desktop
            DESTINATION share/applications)
endif()
