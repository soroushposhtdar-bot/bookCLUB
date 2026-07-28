cmake_minimum_required(VERSION 3.16)

project(BookClub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# =============================================================================
#  Build options
# =============================================================================
option(BOOKCLUB_BUILD_CLIENT "Build the QML client (BookClubClient)" ON)
option(BOOKCLUB_BUILD_SERVER "Build the headless server (BookClubServer)" ON)
option(BOOKCLUB_BUILD_TESTS  "Build the Qt Test suites" OFF)
option(BOOKCLUB_USE_QT5COMPAT "Prefer Qt5Compat.GraphicalEffects for drop shadows" OFF)

# =============================================================================
#  Find Qt6 — REQUIRED (no Qt5 fallback, user is fully on Qt 6)
# =============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Network Sql)

set(QT_DEFAULT_MAJOR_VERSION 6)
message(STATUS "BookClub: using Qt 6")

if(BOOKCLUB_BUILD_CLIENT)
    find_package(Qt6 REQUIRED COMPONENTS
        Gui
        Qml
        Quick
        QuickControls2
    )
endif()

# Qt5Compat is optional
find_package(Qt6 COMPONENTS Qt5Compat)

if(BOOKCLUB_USE_QT5COMPAT AND NOT Qt6_Qt5Compat_FOUND)
    message(WARNING "BOOKCLUB_USE_QT5COMPAT=ON but Qt6::Qt5Compat was not found.")
    set(BOOKCLUB_USE_QT5COMPAT OFF)
endif()

set(BOOKCLUB_HAVE_QT5COMPAT ${Qt6_Qt5Compat_FOUND} CACHE INTERNAL "")
set(BOOKCLUB_HAVE_SHADER_TOOLS ${Qt6_ShaderTools_FOUND} CACHE INTERNAL "")

# ---- Include Directories ----
include_directories(${CMAKE_SOURCE_DIR})

# ---- Subdirectories ----
add_subdirectory(common)

if(BOOKCLUB_BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BOOKCLUB_BUILD_SERVER)
    add_subdirectory(src/server)
endif()

# ---- Tests (optional) ----
if(BOOKCLUB_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# ---- Install ----
if(BOOKCLUB_BUILD_CLIENT)
    install(TARGETS BookClubClient
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
endif()

if(BOOKCLUB_BUILD_SERVER)
    install(TARGETS BookClubServer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
endif()
