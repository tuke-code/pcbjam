#!/bin/bash
# Build the minimal wxWidgets WASM test application
# This script creates library symlinks and builds the test app

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build-wasm/wxwidgets-universal"
TESTS_DIR="$PROJECT_ROOT/tests"
WASM_APP_DIR="$TESTS_DIR/wasm-app"

echo "=== Building wxWidgets WASM Test Application ==="
echo "Project root: $PROJECT_ROOT"
echo "wxWidgets build: $BUILD_DIR"
echo "Test app dir: $WASM_APP_DIR"

# Verify wxWidgets is built
if [ ! -f "$BUILD_DIR/wx-config" ]; then
    echo "ERROR: wxWidgets not built. Run build-wxuniversal-wasm.sh first"
    exit 1
fi

# Create library symlinks
# wx-config outputs paths like libwx_baseu-3.2.a but files are named libwx_baseu-3.2-emscripten.a
echo ""
echo "=== Creating library symlinks ==="
cd "$BUILD_DIR/lib"
for f in libwx_*-emscripten.a; do
    if [ -f "$f" ]; then
        link="${f%-emscripten.a}.a"
        if [ ! -e "$link" ]; then
            ln -s "$f" "$link"
            echo "Created: $link -> $f"
        fi
    fi
done

# Build the test app
echo ""
echo "=== Building test application ==="
cd "$WASM_APP_DIR"

# Clean any previous build
make -f Makefile.wasm clean 2>/dev/null || true

# Build
make -f Makefile.wasm

echo ""
echo "=== Build complete ==="
ls -lh "$WASM_APP_DIR"/minimal_test.* 2>/dev/null

echo ""
echo "To run the tests:"
echo "  cd $TESTS_DIR"
echo "  npm install"
echo "  npm test"
