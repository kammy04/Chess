# =============================================================================
# CMakeLists.txt: instructions for BUILDING the project (not C++ code)
# =============================================================================
# The compiler turns each .cpp file into machine code, then a LINKER joins
# the pieces into runnable programs. With many files, typing those commands
# by hand is painful. CMake reads this file and generates the build commands
# for us. Lines starting with # are comments.
#
# Typical use (from the project folder):
#   cmake -S . -B build        # "configure": read this file, prepare the build/ folder
#   cmake --build build -j     # compile everything (-j = use all CPU cores)
#   ctest --test-dir build     # run the tests

cmake_minimum_required(VERSION 3.20)
project(ChessEngine VERSION 0.1.0 LANGUAGES CXX)   # CXX = C++

set(CMAKE_CXX_STANDARD 20)             # use the C++20 version of the language
set(CMAKE_CXX_STANDARD_REQUIRED ON)    # fail instead of silently using an older version
set(CMAKE_CXX_EXTENSIONS OFF)          # standard C++ only, no compiler-specific extras
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # writes build/compile_commands.json, which VS Code reads

# Release = optimised for speed. Debug = slower, but easier to step through
# in a debugger, and assert() checks are switched on.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Debug, Release, RelWithDebInfo" FORCE)
endif()

# Switches you can flip when configuring, e.g.  cmake -S . -B build -DCHESS_BUILD_TESTS=OFF
option(CHESS_BUILD_TESTS "Build the GoogleTest suite" ON)
option(CHESS_ENABLE_COVERAGE "Instrument chess_core for llvm-cov" OFF)
option(CHESS_ENABLE_SANITIZERS "Build everything with AddressSanitizer + UBSan" OFF)

# SANITIZERS add run-time checks that catch memory mistakes (reading outside
# a list, using memory after it's freed) and undefined behaviour, and report
# exactly where they happened. Slower, so off by default.
if(CHESS_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# Compiler WARNINGS: extra checks that point out suspicious code. We collect
# them in a pseudo-target and attach it to our own targets only.
#   -Wall -Wextra   most common warnings
#   -Wpedantic      warn about non-standard C++
#   -Wshadow        warn when a variable hides another with the same name
add_library(chess_warnings INTERFACE)
if(MSVC)
  target_compile_options(chess_warnings INTERFACE /W4)
else()
  target_compile_options(chess_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# ---- The engine as a LIBRARY -------------------------------------------------
# A library is compiled code that isn't a program by itself; programs LINK to
# it to use it. Both the game (chess) and the tests (chess_tests) use it,
# so the engine is compiled once and shared.
add_library(chess_core
  src/types.cpp
  src/move.cpp
  src/board.cpp
  src/move_generator.cpp
  src/evaluator.cpp
  src/search.cpp
  src/game.cpp
)
# Where to look for #include "..." files.
#   PUBLIC  include/  also given to anything that links chess_core
#   PRIVATE src/      only for the library itself (e.g. offsets.hpp)
target_include_directories(chess_core
  PUBLIC  ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(chess_core PRIVATE chess_warnings)

# COVERAGE: record which lines the tests actually run (see scripts/coverage.sh).
if(CHESS_ENABLE_COVERAGE)
  # PUBLIC so the test program is instrumented too: small functions written
  # in headers get compiled into it, and would otherwise show as 0% covered.
  target_compile_options(chess_core PUBLIC -fprofile-instr-generate -fcoverage-mapping)
  target_link_options(chess_core PUBLIC -fprofile-instr-generate)
endif()

# ---- The playable program ----------------------------------------------------
# add_executable makes a runnable program (build/chess) from app/main.cpp,
# linked with the engine library.
add_executable(chess app/main.cpp)
target_link_libraries(chess PRIVATE chess_core chess_warnings)

# ---- Tests -------------------------------------------------------------------
if(CHESS_BUILD_TESTS)
  enable_testing()
  # FetchContent downloads a library during configure. Here: GoogleTest,
  # Google's widely used C++ testing library, at a pinned version.
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
  # gtest_main supplies a ready-made main() that runs every TEST.
  target_link_libraries(chess_tests PRIVATE chess_core chess_warnings GTest::gtest_main)

  # Registers each individual test with ctest.
  include(GoogleTest)
  gtest_discover_tests(chess_tests DISCOVERY_TIMEOUT 30)
endif()
