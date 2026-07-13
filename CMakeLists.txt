cat > /home/workdir/artifacts/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.16)
project(bookCLUB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Qml Quick)

# Common library
add_subdirectory(common)

# Server
add_subdirectory(server)

# Client
add_subdirectory(client)

# Tests (optional)
# add_subdirectory(tests)
EOF