#!/bin/bash
# Copies KiCad WASM build output to test directory
#
# Priority: Use local output/ directory (populated by docker/build.sh)
# Fallback: Copy from Docker volume directly
#
# Copies whichever apps are present (pcbnew, eeschema, calculator).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# WX_PORT=dom serves the DOM-flavor bundles (built by docker/build.sh with
# WX_PORT=dom into output-dom/) from tests/apps-dom/kicad.
if [ "${WX_PORT:-}" = "dom" ]; then
    KICAD_TEST="$PROJECT_ROOT/tests/apps-dom/kicad"
    OUTPUT_DIR="$PROJECT_ROOT/output-dom"
    KICAD_BUILD_SUFFIX="-dom"
else
    KICAD_TEST="$PROJECT_ROOT/tests/apps/kicad"
    OUTPUT_DIR="$PROJECT_ROOT/output"
    KICAD_BUILD_SUFFIX=""
fi

mkdir -p "$KICAD_TEST"

# Smart copy: skip when the destination already exists and is byte-identical to
# the source (cmp -s is portable across macOS/Linux). Makes this a real sync —
# re-running does not rewrite unchanged multi-hundred-MB .wasm files.
smart_cp() {
    local src="$1" destdir="$2"
    [ -f "$src" ] || return 0
    local dst="$destdir/$(basename "$src")"
    if [ -f "$dst" ] && cmp -s "$src" "$dst"; then
        echo "  = $(basename "$src") (up-to-date)"
        return 0
    fi
    cp "$src" "$dst"
    echo "  + $(basename "$src")"
}

# Map an app name to its inner CMake build subdirectory. Most apps share their
# subdir name with the app name; pcb_calculator emits OUTPUT_NAME=calculator
# but lives under the pcb_calculator/ subtree of the build dir, and pl_editor's
# source lives under pagelayout_editor/.
kicad_subdir_for() {
    case "$1" in
        calculator)    echo "pcb_calculator" ;;
        pl_editor)     echo "pagelayout_editor" ;;
        symbol_editor) echo "eeschema" ;;
        *)             echo "$1" ;;
    esac
}

# Copy one app's artifacts (js, wasm, optional debug/map/worker). Returns 0
# if the app was present, 1 if neither output/ nor the docker volume has it.
copy_app() {
    local app="$1"
    local subdir
    subdir=$(kicad_subdir_for "$app")

    if [ -f "$OUTPUT_DIR/${app}.js" ] && [ -f "$OUTPUT_DIR/${app}.wasm" ]; then
        echo "Syncing ${app} WASM files from output directory..."
        smart_cp "$OUTPUT_DIR/${app}.js" "$KICAD_TEST"
        smart_cp "$OUTPUT_DIR/${app}.wasm" "$KICAD_TEST"
        smart_cp "$OUTPUT_DIR/${app}.wasm.map" "$KICAD_TEST"
        smart_cp "$OUTPUT_DIR/${app}.worker.js" "$KICAD_TEST"
        smart_cp "$OUTPUT_DIR/images.tar.gz" "$KICAD_TEST"
        return 0
    fi

    echo "Output ${app} not found locally, trying Docker volume..."
    if docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
         kicad-wasm-builder:/workspace/build-wasm/kicad-${app}${KICAD_BUILD_SUFFIX}/${subdir}/${app}.js "$KICAD_TEST/" 2>/dev/null \
       && docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
         kicad-wasm-builder:/workspace/build-wasm/kicad-${app}${KICAD_BUILD_SUFFIX}/${subdir}/${app}.wasm "$KICAD_TEST/" 2>/dev/null; then
        docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
            kicad-wasm-builder:/workspace/build-wasm/kicad-${app}${KICAD_BUILD_SUFFIX}/${subdir}/${app}.wasm.map "$KICAD_TEST/" 2>/dev/null || true
        docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
            kicad-wasm-builder:/workspace/build-wasm/kicad-${app}${KICAD_BUILD_SUFFIX}/${subdir}/${app}.worker.js "$KICAD_TEST/" 2>/dev/null || true
        docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
            kicad-wasm-builder:/workspace/build-wasm/kicad-${app}${KICAD_BUILD_SUFFIX}/resources/images.tar.gz "$KICAD_TEST/" 2>/dev/null || true
        return 0
    fi

    echo "  (no ${app} artifacts found — skipping)"
    return 1
}

found_any=0
copy_app pcbnew        && found_any=1
copy_app eeschema      && found_any=1
copy_app calculator    && found_any=1
copy_app pl_editor     && found_any=1
copy_app symbol_editor && found_any=1
copy_app gerbview      && found_any=1

if [ "$found_any" -eq 0 ]; then
    echo "Error: no pcbnew/eeschema/calculator/pl_editor/symbol_editor/gerbview artifacts found in output/ or docker volume" >&2
    exit 1
fi

# wxWidgets WASM JavaScript glue code (defines JS functions called from WASM)
echo "Syncing wxWidgets WASM glue code..."
if [ -f "$OUTPUT_DIR/wx.js" ]; then
    smart_cp "$OUTPUT_DIR/wx.js" "$KICAD_TEST"
else
    if docker compose -f "$PROJECT_ROOT/docker/docker-compose.yml" cp \
      kicad-wasm-builder:/workspace/build-wasm/wxwidgets/build/wasm/wx.js "$KICAD_TEST/" 2>/dev/null; then
        :
    else
        smart_cp "$PROJECT_ROOT/wxwidgets/build/wasm/wx.js" "$KICAD_TEST"
    fi
fi

if [ "${WX_PORT:-}" = "dom" ]; then
    # DOM port: the control-layer shim loads after wx.js.
    if [ -f "$OUTPUT_DIR/wx-dom.js" ]; then
        smart_cp "$OUTPUT_DIR/wx-dom.js" "$KICAD_TEST"
    else
        smart_cp "$PROJECT_ROOT/wxwidgets/build/wasm/wx-dom.js" "$KICAD_TEST"
    fi

    # The checked-in kicad pages load wx.js via a <script> tag; the DOM
    # flavor needs wx-dom.js right after it. Sync the pages and inject the
    # tag (idempotent).
    for page in "$PROJECT_ROOT"/tests/apps/kicad/*.html; do
        [ -f "$page" ] || continue
        dest="$KICAD_TEST/$(basename "$page")"
        cp "$page" "$dest"
        if ! grep -q 'wx-dom.js' "$dest"; then
            perl -i -pe 's{(<script src="wx.js"></script>)}{$1\n    <script src="wx-dom.js"></script>}' "$dest"
        fi
    done
    echo "  + kicad pages with wx-dom.js injected"
fi

echo "KiCad WASM files synced to $KICAD_TEST"
ls -lh "$KICAD_TEST"
