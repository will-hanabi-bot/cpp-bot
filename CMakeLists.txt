cmake_minimum_required(VERSION 3.20)
project(hanabi_bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

include(cmake/warnings.cmake)
include(cmake/deps.cmake)

# --- Core library: basics, conventions, endgame -------------------------
add_library(hanabi_core STATIC
  src/basics/placeholder.cpp
)
target_include_directories(hanabi_core PUBLIC include)
target_link_libraries(hanabi_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)
target_compile_definitions(hanabi_core PUBLIC
  HANABI_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)
hanabi_set_warnings(hanabi_core)

# --- Tests --------------------------------------------------------------
enable_testing()
include(GoogleTest)

add_executable(hanabi_tests
  tests/test_smoke.cpp
)
target_link_libraries(hanabi_tests PRIVATE hanabi_core GTest::gtest_main)
hanabi_set_warnings(hanabi_tests)
gtest_discover_tests(hanabi_tests)
