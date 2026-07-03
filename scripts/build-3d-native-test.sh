#!/bin/bash
#
# Build script for the native 3D-renderer test harness (tests/3d-regression/native).
#
# Builds a standalone macOS application that renders the shared 3D scenarios
# through KiCad's actual RENDER_3D_OPENGL code paths (real desktop OpenGL) and
# writes golden baseline PNGs for the OpenGL->WebGL port regression suite.
#
# Modeled on scripts/build-gal-native-test.sh.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$PROJECT_ROOT/tests/3d-regression/native"
BUILD_DIR="$TEST_DIR/build"
LOG_FILE="$PROJECT_ROOT/tests/logs/3d-native-build.log"

mkdir -p "$(dirname "$LOG_FILE")"

echo "Building 3D Renderer Native Test..."
echo "  Test dir: $TEST_DIR"
echo "  Build dir: $BUILD_DIR"
echo "  Log file: $LOG_FILE"

mkdir -p "$BUILD_DIR"

echo "Running CMake..."
if ! cmake -S "$TEST_DIR" -B "$BUILD_DIR" >> "$LOG_FILE" 2>&1; then
    echo "CMake configuration failed! Check $LOG_FILE for details."
    echo ""
    echo "Last 50 lines of log:"
    tail -50 "$LOG_FILE"
    exit 1
fi

echo "Building..."
if ! make -C "$BUILD_DIR" -j"$(sysctl -n hw.ncpu 2>/dev/null || echo 4)" >> "$LOG_FILE" 2>&1; then
    echo "Build failed! Check $LOG_FILE for details."
    echo ""
    echo "Last 100 lines of log:"
    tail -100 "$LOG_FILE"
    exit 1
fi

echo "Build successful!"
echo "Executable: $BUILD_DIR/scene3d_native_test"
echo ""
echo "Options:"
echo "  --output <dir>    Output directory for 3d-<name>.png"
echo "  --manifest <file> Write the scenario manifest JSON"
echo "  --filter <substr> Only run scenarios whose name contains <substr>"
echo "  --list            Print scenario names and exit"
echo "  --show            Keep the window open after rendering"
