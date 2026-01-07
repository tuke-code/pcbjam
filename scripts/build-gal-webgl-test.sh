#!/bin/bash
#
# Build script for the WebGL GAL test harness (WASM)
#
# This builds a WASM module that renders GAL test scenarios using WebGL,
# allowing comparison against native OpenGL rendering.
#
# Uses a Makefile with direct em++ calls (like build-wasm-test.sh)
# to avoid emcmake Python 3.10+ requirement.
#

# Redirect all output to a log file (re-execs script with redirection)
source "$(dirname "$0")/common/logging.sh"

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source common environment (sets up Python 3.10+ for Emscripten)
QUIET=1 source "$SCRIPT_DIR/common/env.sh"

TEST_DIR="$PROJECT_ROOT/tests/gal-regression/wasm"
OUTPUT_DIR="$PROJECT_ROOT/tests/apps/gal-webgl"

echo "Building GAL WebGL Test..."
echo "  Test dir: $TEST_DIR"
echo "  Output dir: $OUTPUT_DIR"
echo "  EMSDK_PYTHON: ${EMSDK_PYTHON:-NOT SET}"
echo "  clang: $(which clang)"

# Verify em++ is available
if ! command -v em++ &> /dev/null; then
    echo "ERROR: em++ not found. Please install via: brew install emscripten"
    exit 1
fi

echo "  Emscripten: $(em++ --version 2>&1 | head -1)"

# Parse arguments
DEBUG_BUILD=0
CLEAN_BUILD=0

for arg in "$@"; do
    if [ "$arg" = "--debug" ]; then
        DEBUG_BUILD=1
    elif [ "$arg" = "--clean" ]; then
        CLEAN_BUILD=1
    fi
done

# Build using Makefile
cd "$TEST_DIR"

if [ "$CLEAN_BUILD" = "1" ]; then
    echo ""
    echo "Cleaning..."
    make clean 2>/dev/null || true
fi

echo ""
echo "Building..."
if [ "$DEBUG_BUILD" = "1" ]; then
    make DEBUG=1
else
    make
fi

echo ""
echo "Build successful!"
echo ""
echo "Files in $OUTPUT_DIR:"
ls -lh "$OUTPUT_DIR"
echo ""
echo "To test locally:"
echo "  cd $PROJECT_ROOT/tests"
echo "  npx serve apps"
echo "  # Open http://localhost:3000/gal-webgl/gal_webgl_test.html"
