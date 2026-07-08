# client/CMakeLists.txt
#
# NOTE: The real, implemented client code lives under ${CMAKE_CURRENT_SOURCE_DIR}
# (not client/src) — this file was previously pointing at a path that doesn't exist.
# This is a HEADLESS build: MainWindow/LoginWindow/ReaderWindow/RoleDashboardWindow
# (Qt Widgets UI, header-only stubs with no .cpp yet) and the QML pages (empty
# placeholders) are intentionally excluded. main.cpp is a console entry point
# that connects to the server and drives AuthController, useful for testing the
# implemented controllers/network/session code without any UI.

project(bookclub_client)

set(CLIENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---- Collect Sources ----
set(CLIENT_SOURCES
    ${CLIENT_SRC_DIR}/controllers/AuthController.cpp
    ${CLIENT_SRC_DIR}/controllers/BookCatalogController.cpp
    ${CLIENT_SRC_DIR}/controllers/CartController.cpp
    ${CLIENT_SRC_DIR}/controllers/LibraryController.cpp
    ${CLIENT_SRC_DIR}/controllers/PublisherController.cpp
    ${CLIENT_SRC_DIR}/controllers/AdminController.cpp
    ${CLIENT_SRC_DIR}/controllers/NotificationController.cpp
    ${CLIENT_SRC_DIR}/controllers/ReaderController.cpp
    ${CLIENT_SRC_DIR}/controllers/StudySessionController.cpp
    ${CLIENT_SRC_DIR}/network/ClientNetworkManager.cpp
    ${CLIENT_SRC_DIR}/session/SessionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(CLIENT_HEADERS
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

# ---- Build Executable ----
add_executable(BookClubClient ${CLIENT_SOURCES} ${CLIENT_HEADERS})

# ---- Link Libraries ----
target_link_libraries(BookClubClient
    bookclub_common
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)

# ---- Include Directories ----
target_include_directories(BookClubClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# ---- Set Output Name ----
set_target_properties(BookClubClient PROPERTIES
    OUTPUT_NAME "BookClubClient"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)