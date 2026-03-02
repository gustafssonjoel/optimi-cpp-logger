#!/usr/bin/env ksh
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
BUILD_DIR="${1:-$REPO_ROOT/build}"

echo "[run_all_tests] Repo root: $REPO_ROOT"
echo "[run_all_tests] Build dir: $BUILD_DIR"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR"
ctest --test-dir "$BUILD_DIR" --output-on-failure
