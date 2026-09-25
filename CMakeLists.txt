cmake_minimum_required(VERSION 3.24)

project(coins_db
  VERSION 0.1.0
  DESCRIPTION "Database and app for cataloging a personal coin collection"
  LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Language standard: C++23, required, no compiler extensions.
# (C++23 gives us std::expected for the validation-result convention.)
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for clang-tidy / editors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Debug if no build type is set (single-config generators).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Shared warning/interface target. Link this into project targets so every
# component gets consistent, strict warnings (warnings-as-errors).
# ---------------------------------------------------------------------------
add_library(coins_warnings INTERFACE)
add_library(coins::warnings ALIAS coins_warnings)

if(MSVC)
  target_compile_options(coins_warnings INTERFACE /W4 /permissive- /WX)
else()
  target_compile_options(coins_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused
    -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion
    -Werror)
endif()

# ---------------------------------------------------------------------------
# Dependencies (provided by Conan via CMakeDeps).
# ---------------------------------------------------------------------------
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)
find_package(CLI11 REQUIRED)

# ---------------------------------------------------------------------------
# Testing (GoogleTest). Enabled by default; toggle with -DCOINS_BUILD_TESTS=OFF.
# ---------------------------------------------------------------------------
option(COINS_BUILD_TESTS "Build the coins-db test suite" ON)
if(COINS_BUILD_TESTS)
  find_package(GTest REQUIRED)
  find_package(rapidcheck REQUIRED)  # property-based testing (QuickCheck-style)
  enable_testing()
  include(GoogleTest)
endif()

# ---------------------------------------------------------------------------
# Components.
# ---------------------------------------------------------------------------
add_subdirectory(core)
add_subdirectory(cli)
add_subdirectory(server)

if(COINS_BUILD_TESTS)
  add_subdirectory(tests)
endif()
