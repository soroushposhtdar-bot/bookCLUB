# =============================================================================
#  tests/CMakeLists.txt
# =============================================================================
#  Wires up the Qt Test suites. Enable by configuring with
#  -DBOOKCLUB_BUILD_TESTS=ON:
#
#      cmake --preset qt-6.11-mingw-64-debug -DBOOKCLUB_BUILD_TESTS=ON
#      cmake --build --preset qt-6.11-mingw-64-debug
#      ctest --test-dir build/qt-6.11-mingw-64-debug --output-on-failure
#
#  (The top-level CMakeLists.txt already does enable_testing() and
#  find_package(Qt6 COMPONENTS Test) before adding this subdirectory.)
# =============================================================================

# -----------------------------------------------------------------------------
#  test_auth — PasswordHasher + AuthService unit tests
# -----------------------------------------------------------------------------
add_executable(test_auth
    server/test_auth.cpp
)

target_link_libraries(test_auth
    PRIVATE
        bookclub_common
        bookclub_client               # for AuthService + MockUser
        Qt6::Core
        Qt6::Test
)

target_include_directories(test_auth PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/client/include
)

# Qt Test's QTEST_MAIN generates a main(); AUTOMOC picks up the Q_OBJECT moc.
set_target_properties(test_auth PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME "test_auth"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_test(NAME test_auth COMMAND test_auth)
