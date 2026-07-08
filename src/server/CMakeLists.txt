# server/CMakeLists.txt
#
# NOTE: The real, implemented server code lives under ${CMAKE_CURRENT_SOURCE_DIR}
# (not server/src) — this file was previously pointing at a path that doesn't exist.
# This is a HEADLESS build: ServerDashboardWindow (Qt Widgets UI) is intentionally
# excluded so the server can be built/run/tested without any UI dependency.

project(bookclub_server)

set(SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---- Collect Sources ----
set(SERVER_SOURCES
    ${SERVER_SRC_DIR}/main.cpp
    ${SERVER_SRC_DIR}/ServerApplication.cpp
    ${SERVER_SRC_DIR}/ServerCore.cpp
    ${SERVER_SRC_DIR}/RequestRouter.cpp
    ${SERVER_SRC_DIR}/RequestHandlerBase.cpp
    ${SERVER_SRC_DIR}/ClientConnection.cpp
    ${SERVER_SRC_DIR}/ConnectionManager.cpp
    ${SERVER_SRC_DIR}/DatabaseManager.cpp
    ${SERVER_SRC_DIR}/NotificationDispatcher.cpp
    ${SERVER_SRC_DIR}/StatisticsService.cpp
    ${SERVER_SRC_DIR}/StudySessionManager.cpp
    ${SERVER_SRC_DIR}/ThreadPool.cpp
    ${SERVER_SRC_DIR}/handlers/AdminRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/AuthRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/BookRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/CartRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/LibraryRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/NotificationRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/PublisherRequestHandler.cpp
    ${SERVER_SRC_DIR}/handlers/StudySessionRequestHandler.cpp
)

set(SERVER_HEADERS
    ${SERVER_SRC_DIR}/ServerApplication.h
    ${SERVER_SRC_DIR}/ServerCore.h
    ${SERVER_SRC_DIR}/RequestRouter.h
    ${SERVER_SRC_DIR}/RequestHandlerBase.h
    ${SERVER_SRC_DIR}/ClientConnection.h
    ${SERVER_SRC_DIR}/ConnectionManager.h
    ${SERVER_SRC_DIR}/DatabaseManager.h
    ${SERVER_SRC_DIR}/NotificationDispatcher.h
    ${SERVER_SRC_DIR}/StatisticsService.h
    ${SERVER_SRC_DIR}/StudySessionManager.h
    ${SERVER_SRC_DIR}/ThreadPool.h
    ${SERVER_SRC_DIR}/handlers/AdminRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/AuthRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/BookRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/CartRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/LibraryRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/NotificationRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/PublisherRequestHandler.h
    ${SERVER_SRC_DIR}/handlers/StudySessionRequestHandler.h
)

# ---- Build Executable ----
add_executable(BookClubServer ${SERVER_SOURCES} ${SERVER_HEADERS})

# ---- Link Libraries (headless: no Qt5::Widgets needed) ----
target_link_libraries(BookClubServer
    bookclub_common
    Qt5::Core
    Qt5::Network
    Qt5::Sql
)

# ---- Include Directories ----
target_include_directories(BookClubServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# ---- Set Output Name ----
set_target_properties(BookClubServer PROPERTIES
    OUTPUT_NAME "BookClubServer"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)