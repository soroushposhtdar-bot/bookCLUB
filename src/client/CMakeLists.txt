# client/CMakeLists.txt
# =============================================================================
#  BookClub Qt5 client — QML + MVVM build target.
#
#  This file replaces the previous "headless" client build. It now compiles:
#      • All QML files (via qml.qrc + Client.qrc)
#      • All auth ViewModels (MVVM, registered with QML via qmlRegisterType)
#      • The mock AuthService (QML singleton)
#      • main.cpp entry point with QML engine setup
#
#  Required Qt5 modules: Core, Gui, Quick, QuickControls2, Qml, GraphicalEffects
# =============================================================================

project(bookclub_client)

set(CLIENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---- Collect Sources ----
set(CLIENT_SOURCES
    # View models (MVVM)
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/AuthViewModelBase.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/LoginViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/RegisterViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/ForgotPasswordViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/ResetPasswordViewModel.cpp
    ${CLIENT_SRC_DIR}/src/viewmodels/auth/GenreSelectionViewModel.cpp

    # Services (mocked auth)
    ${CLIENT_SRC_DIR}/src/services/AuthService.cpp

    # Existing controllers (kept for parity with the rest of the project)
    ${CLIENT_SRC_DIR}/controllers/AuthController.cpp
    ${CLIENT_SRC_DIR}/controllers/BookCatalogController.cpp
    ${CLIENT_SRC_DIR}/controllers/CartController.cpp
    ${CLIENT_SRC_DIR}/controllers/LibraryController.cpp
    ${CLIENT_SRC_DIR}/controllers/PublisherController.cpp
    ${CLIENT_SRC_DIR}/controllers/AdminController.cpp
    ${CLIENT_SRC_DIR}/controllers/NotificationController.cpp
    ${CLIENT_SRC_DIR}/controllers/ReaderController.cpp
    ${CLIENT_SRC_DIR}/controllers/StudySessionController.cpp

    # Networking / session
    ${CLIENT_SRC_DIR}/network/ClientNetworkManager.cpp
    ${CLIENT_SRC_DIR}/session/SessionManager.cpp

    # Entry point
    ${CLIENT_SRC_DIR}/main.cpp
)

set(CLIENT_HEADERS
    # View models
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/AuthViewModelBase.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/LoginViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/RegisterViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/ForgotPasswordViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/ResetPasswordViewModel.h
    ${CLIENT_SRC_DIR}/include/viewmodels/auth/GenreSelectionViewModel.h

    # Services
    ${CLIENT_SRC_DIR}/include/services/AuthService.h

    # Existing controllers
    ${CLIENT_SRC_DIR}/controllers/AuthController.h
    ${CLIENT_SRC_DIR}/controllers/BookCatalogController.h
    ${CLIENT_SRC_DIR}/controllers/CartController.h
    ${CLIENT_SRC_DIR}/controllers/LibraryController.h
    ${CLIENT_SRC_DIR}/controllers/PublisherController.h
    ${CLIENT_SRC_DIR}/controllers/AdminController.h
    ${CLIENT_SRC_DIR}/controllers/NotificationController.h
    ${CLIENT_SRC_DIR}/controllers/ReaderController.h
    ${CLIENT_SRC_DIR}/controllers/StudySessionController.h
    ${CLIENT_SRC_DIR}/network/ClientNetworkManager.h
    ${CLIENT_SRC_DIR}/session/SessionManager.h
)

# ---- Resources (QML + fonts) ----
set(CLIENT_RESOURCES
    ${CLIENT_SRC_DIR}/resources/qml.qrc
    ${CLIENT_SRC_DIR}/resources/fonts.qrc
)

# ---- Build Executable ----
add_executable(BookClubClient
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
    ${CLIENT_RESOURCES}
)

# ---- Qt5 modules ----
# Note: the parent CMakeLists.txt currently finds only Core/Network/Sql.
# For the QML client we additionally need Gui, Quick, QuickControls2, Qml,
# and GraphicalEffects. We find them here so the existing headless server
# build is unaffected.
find_package(Qt5 REQUIRED COMPONENTS Gui Quick QuickControls2 Qml Network Sql)

set_target_properties(BookClubClient PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

# ---- Link Libraries ----
target_link_libraries(BookClubClient
    bookclub_common
    Qt5::Core
    Qt5::Gui
    Qt5::Quick
    Qt5::QuickControls2
    Qt5::Qml
    Qt5::Network
    Qt5::Sql
)

# ---- Include Directories ----
target_include_directories(BookClubClient PRIVATE
    ${CLIENT_SRC_DIR}
    ${CLIENT_SRC_DIR}/include
    ${CMAKE_SOURCE_DIR}
)

# ---- Set Output Name ----
set_target_properties(BookClubClient PROPERTIES
    OUTPUT_NAME "BookClubClient"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

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
