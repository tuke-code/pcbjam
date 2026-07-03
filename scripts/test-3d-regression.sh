#!/bin/bash
#
# 3D-renderer regression suite orchestrator (tests/3d-regression).
#
# Usage:
#   ./scripts/test-3d-regression.sh            # native: build -> run -> compare vs baseline;
#                                              #   webgl phase too once the wasm harness exists
#   ./scripts/test-3d-regression.sh native     # native phase only
#   ./scripts/test-3d-regression.sh webgl      # wasm build -> playwright -> webgl-self + parity
#   ./scripts/test-3d-regression.sh compare    # comparisons only (skip builds/runs)
#   ./scripts/test-3d-regression.sh promote    # promote output/native -> baseline/ (byte-diff
#                                              #   guarded) + manifest.json
#
# Comparison engine: the pixelmatch CI tooling (tests/tools/screenshots/compare-dirs.ts),
# NOT ImageMagick. Levels/floors: tests/3d-regression/floors.json.
#

source "$(dirname "${BASH_SOURCE[0]}")/common/logging.sh"

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
THREED_DIR="$PROJECT_ROOT/tests/3d-regression"
BASELINE_DIR="$THREED_DIR/baseline"
BASELINE_WEBGL_DIR="$THREED_DIR/baseline-webgl"
NATIVE_OUT="$THREED_DIR/output/native"
WEBGL_OUT="$THREED_DIR/output/webgl"
NATIVE_BIN="$THREED_DIR/native/build/scene3d_native_test"

MODE="${1:-all}"

NATIVE_COMPARE_STATUS=0
WEBGL_COMPARE_STATUS=0

run_native() {
    echo "=== Native phase: build + render ==="
    "$SCRIPT_DIR/build-3d-native-test.sh"

    mkdir -p "$NATIVE_OUT"
    "$NATIVE_BIN" --output "$NATIVE_OUT" --manifest "$NATIVE_OUT/manifest.json"

    # Anti-drift guard: the scenario registry (names/size) must match the
    # committed manifest; a mismatch means scenarios changed and the baselines
    # need review + re-promote.
    if [ -f "$THREED_DIR/manifest.json" ]; then
        if ! cmp -s "$THREED_DIR/manifest.json" "$NATIVE_OUT/manifest.json"; then
            echo "ERROR: scenario registry changed (manifest.json differs from committed copy)."
            echo "Review the change, then: ./scripts/test-3d-regression.sh promote"
            exit 1
        fi
    else
        echo "NOTE: no committed manifest yet (first run) — promote will create it."
    fi
}

compare_native() {
    if [ ! -d "$BASELINE_DIR" ] || [ -z "$(ls -A "$BASELINE_DIR" 2>/dev/null)" ]; then
        echo "SKIP native-self compare: no committed baseline yet (run promote first)."
        return
    fi

    echo "=== Compare: native-self (baseline/ vs output/native/) ==="
    ( cd "$PROJECT_ROOT/tests" && npm run --silent 3d:check ) || NATIVE_COMPARE_STATUS=$?
}

run_webgl() {
    if [ ! -f "$THREED_DIR/wasm/Makefile" ]; then
        echo "SKIP webgl phase: tests/3d-regression/wasm not present yet."
        return
    fi

    if [ ! -x "$PROJECT_ROOT/build-wasm/wxwidgets/wx-config" ]; then
        echo "SKIP webgl phase: wxWidgets WASM build missing (scripts/build-wx-wasm.sh)."
        return
    fi

    echo "=== WebGL phase: build + playwright capture ==="
    "$SCRIPT_DIR/build-3d-webgl-test.sh"
    ( cd "$PROJECT_ROOT/tests" && npx playwright test e2e/3d-webgl.spec.ts )
}

compare_webgl() {
    if [ ! -d "$WEBGL_OUT" ] || [ -z "$(ls -A "$WEBGL_OUT" 2>/dev/null)" ]; then
        echo "SKIP webgl compares: no webgl renders in output/webgl."
        return
    fi

    if [ -d "$BASELINE_WEBGL_DIR" ] && [ -n "$(ls -A "$BASELINE_WEBGL_DIR" 2>/dev/null)" ]; then
        echo "=== Compare: webgl-self (baseline-webgl/ vs output/webgl/) ==="
        ( cd "$PROJECT_ROOT/tests" && npm run --silent 3d:check:webgl ) || WEBGL_COMPARE_STATUS=$?
    else
        echo "SKIP webgl-self compare: no baseline-webgl yet."
    fi

    # Port-parity is the TDD progress meter: informational, never gates.
    echo "=== Compare: parity (baseline/ vs output/webgl/, informational) ==="
    ( cd "$PROJECT_ROOT/tests" && npm run --silent 3d:check:parity ) || true
}

promote() {
    if [ ! -d "$NATIVE_OUT" ] || [ -z "$(ls -A "$NATIVE_OUT" 2>/dev/null)" ]; then
        echo "ERROR: nothing to promote — run the native phase first."
        exit 1
    fi

    mkdir -p "$BASELINE_DIR"
    local changed=0

    for png in "$NATIVE_OUT"/3d-*.png; do
        local name
        name="$(basename "$png")"

        if ! cmp -s "$png" "$BASELINE_DIR/$name"; then
            cp "$png" "$BASELINE_DIR/$name"
            echo "  promoted: $name"
            changed=$((changed + 1))
        fi
    done

    if ! cmp -s "$NATIVE_OUT/manifest.json" "$THREED_DIR/manifest.json"; then
        cp "$NATIVE_OUT/manifest.json" "$THREED_DIR/manifest.json"
        echo "  promoted: manifest.json"
        changed=$((changed + 1))
    fi

    # Baselines removed from the registry linger in baseline/ — report them.
    for png in "$BASELINE_DIR"/3d-*.png; do
        [ -e "$png" ] || continue
        if [ ! -f "$NATIVE_OUT/$(basename "$png")" ]; then
            echo "  STALE baseline (not in registry anymore): $(basename "$png")"
        fi
    done

    echo "Promote done: $changed file(s) updated (byte-diff guarded, zero churn)."
    echo "Review + git add tests/3d-regression/{baseline,manifest.json}."
}

case "$MODE" in
    all)
        run_native
        compare_native
        run_webgl
        compare_webgl
        ;;
    native)
        run_native
        compare_native
        ;;
    webgl)
        run_webgl
        compare_webgl
        ;;
    compare)
        compare_native
        compare_webgl
        ;;
    promote)
        promote
        exit 0
        ;;
    *)
        echo "Usage: $0 [all|native|webgl|compare|promote]"
        exit 2
        ;;
esac

echo ""
if [ $NATIVE_COMPARE_STATUS -ne 0 ]; then
    echo "RESULT: FAIL (native-self comparison found changes — see tests/3d-regression/output/diff/native-self/)"
    exit 1
fi

if [ $WEBGL_COMPARE_STATUS -ne 0 ]; then
    echo "RESULT: FAIL (webgl-self comparison found changes — see tests/3d-regression/output/diff/webgl-self/)"
    exit 1
fi

echo "RESULT: PASS"
