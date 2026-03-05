#!/usr/bin/env ksh
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
BUILD_DIR="${1:-$REPO_ROOT/build-asan}"

echo "[run_asan_leak_check] Repo root: $REPO_ROOT"
echo "[run_asan_leak_check] Build dir: $BUILD_DIR"

cmake \
  -S "$REPO_ROOT" \
  -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

cmake --build "$BUILD_DIR"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" ctest --test-dir "$BUILD_DIR" --output-on-failure
