#!/usr/bin/env bash
# Builds an instrumented copy of the engine, runs the test suite, and prints
# line/branch coverage for the engine library (src/ and include/).
#
#   ./scripts/coverage.sh          summary table
#   ./scripts/coverage.sh --html   also writes coverage/index.html
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-coverage"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug -DCHESS_ENABLE_COVERAGE=ON >/dev/null
cmake --build "$BUILD" --target chess_tests -j

LLVM_PROFILE_FILE="$BUILD/tests.profraw" "$BUILD/chess_tests" --gtest_brief=1
xcrun llvm-profdata merge -sparse "$BUILD/tests.profraw" -o "$BUILD/tests.profdata"

SOURCES=("$ROOT/src" "$ROOT/include")
xcrun llvm-cov report "$BUILD/chess_tests" -instr-profile="$BUILD/tests.profdata" "${SOURCES[@]}"

if [[ "${1:-}" == "--html" ]]; then
  xcrun llvm-cov show "$BUILD/chess_tests" -instr-profile="$BUILD/tests.profdata" \
    -format=html -output-dir="$ROOT/coverage" "${SOURCES[@]}"
  echo "HTML report: $ROOT/coverage/index.html"
fi
