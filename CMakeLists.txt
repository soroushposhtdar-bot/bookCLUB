cmake_minimum_required(VERSION 3.16)

project(BookClub VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Find Qt5 (Headless Build: No Widgets Required) ----
find_package(Qt5 REQUIRED COMPONENTS Core Network Sql)

# ---- Include Directories ----
include_directories(${CMAKE_SOURCE_DIR})

# ---- Subdirectories ----
add_subdirectory(common)
add_subdirectory(src/client)
add_subdirectory(src/server)