cmake_minimum_required(VERSION 3.16)

project(BookClubAuth VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(BookClubAuth
    main.cpp
    src/auth/AuthManager.cpp
    src/windows/MainWindow.cpp
    src/windows/LoginPage.cpp
    src/windows/RegisterPage.cpp
    src/windows/ResetPasswordPage.cpp
    resources.qrc
)

target_include_directories(BookClubAuth PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(BookClubAuth PRIVATE Qt6::Widgets)

qt_finalize_executable(BookClubAuth)
