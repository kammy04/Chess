cmake_minimum_required(VERSION 3.20)
project(ChessEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # compile_commands.json for VS Code / clangd

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Debug, Release, RelWithDebInfo" FORCE)
endif()

option(CHESS_BUILD_TESTS "Build the GoogleTest suite" ON)
option(CHESS_ENABLE_COVERAGE "Instrument chess_core for llvm-cov" OFF)
option(CHESS_ENABLE_SANITIZERS "Build everything with AddressSanitizer + UBSan" OFF)

if(CHESS_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Warnings live on an INTERFACE target so every target that links it opts in,
# without forcing them onto third-party code like GoogleTest.
add_library(chess_warnings INTERFACE)
if(MSVC)
  target_compile_options(chess_warnings INTERFACE /W4)
else()
  target_compile_options(chess_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# ---- The engine as a library: shared by the CLI and the tests -------------
add_library(chess_core
  src/types.cpp
  src/move.cpp
  src/board.cpp
  src/move_generator.cpp
  src/evaluator.cpp
  src/search.cpp
  src/game.cpp
)
target_include_directories(chess_core
  PUBLIC  ${PROJECT_SOURCE_DIR}/include   # users write #include "chess/board.hpp"
  PRIVATE ${PROJECT_SOURCE_DIR}/src       # private headers such as offsets.hpp
)
target_link_libraries(chess_core PRIVATE chess_warnings)

if(CHESS_ENABLE_COVERAGE)
  # PUBLIC: inline functions in headers are compiled into the test binary, so
  # it must be instrumented too or they would show as 0% covered.
  target_compile_options(chess_core PUBLIC -fprofile-instr-generate -fcoverage-mapping)
  target_link_options(chess_core PUBLIC -fprofile-instr-generate)
endif()

# ---- Terminal front end ------------------------------------------------------
add_executable(chess app/main.cpp)
target_link_libraries(chess PRIVATE chess_core chess_warnings)

# ---- Tests ------------------------------------------------------------------
if(CHESS_BUILD_TESTS)
  enable_testing()
  include(FetchContent)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(chess_tests
    tests/types_test.cpp
    tests/board_test.cpp
    tests/move_generator_test.cpp
    tests/evaluator_test.cpp
    tests/search_test.cpp
    tests/game_test.cpp
  )
  target_link_libraries(chess_tests PRIVATE chess_core chess_warnings GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(chess_tests DISCOVERY_TIMEOUT 30)
endif()
