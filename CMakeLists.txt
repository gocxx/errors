cmake_minimum_required(VERSION 3.15)

project(gocxx_errors VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- gocxx_errors header-only interface library ----
add_library(gocxx_errors INTERFACE)

target_include_directories(gocxx_errors INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---- Installation: headers + pkg-config ----
install(DIRECTORY include/ DESTINATION include)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gocxx_errors.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/gocxx_errors.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gocxx_errors.pc
        DESTINATION lib/pkgconfig)

if(ENABLE_TESTS)

# ---- GoogleTest via FetchContent ----
include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # Match your toolchain
    FetchContent_MakeAvailable(googletest)

    # ---- Enable tests ----
    enable_testing()

    # ---- Test Executable ----
    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    )

    add_executable(errors_test ${TEST_SOURCES})

    target_link_libraries(errors_test
        gocxx_errors
        gtest
        gtest_main
    )

    target_include_directories(errors_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    # ---- Test Discovery ----
    include(GoogleTest)
    gtest_discover_tests(errors_test)

else()
    message(STATUS "Tests are disabled")
endif()