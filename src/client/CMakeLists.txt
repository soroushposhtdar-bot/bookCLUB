# src/client/CMakeLists.txt
# Headless build: MainWindow/LoginWindow are intentionally excluded.

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
    ${CLIENT_SRC_DIR}/main.cpp
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

# ---- Link Libraries (No Qt5::Widgets needed) ----
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