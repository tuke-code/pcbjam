#!/bin/bash

# GAL WebGL Regression Test - Monitoring Script
# ==============================================
# Single entry point for WebGL GAL testing.
# Always does a clean build, then runs tests and compares against baseline.
#
# Purpose: Detect regressions during WebGL GAL migration to KiCad.
# Strategy: Lock in current rendering behavior as baseline, then monitor
#           for any changes as we integrate WebGL GAL into KiCad build.
#
# Usage:
#   ./scripts/test-gal-webgl.sh           # Clean build and test (default)
#   ./scripts/test-gal-webgl.sh -v        # Verbose output

# Redirect all output to a log file (re-execs script with redirection)
source "$(dirname "$0")/common/logging.sh"

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Directories
GAL_REGRESSION_DIR="$PROJECT_ROOT/tests/gal-regression"
BASELINE_WEBGL_DIR="$GAL_REGRESSION_DIR/baseline-webgl"
OUTPUT_DIR="$GAL_REGRESSION_DIR/output"
WEBGL_OUTPUT_DIR="$OUTPUT_DIR/webgl"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse arguments
VERBOSE=""

for arg in "$@"; do
    case $arg in
        -v|--verbose)
            VERBOSE="-v"
            ;;
    esac
done

# ============================================================================
# Helper Functions
# ============================================================================

log_header() {
    echo ""
    echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
}

log_step() {
    echo -e "${YELLOW}>>> $1${NC}"
}

log_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

log_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Compare WebGL output against baseline-webgl
# Usage: compare_webgl_screenshots [threshold]
# Returns 0 if match within threshold, 1 if different
compare_webgl_screenshots() {
    local threshold="${1:-1.0}"  # Default 1% difference allowed

    echo ""
    echo "Comparing WebGL output against baseline:"
    echo "  Baseline:  $BASELINE_WEBGL_DIR"
    echo "  Current:   $WEBGL_OUTPUT_DIR"
    echo "  Threshold: ${threshold}% pixel difference"
    echo ""

    if [ ! -d "$BASELINE_WEBGL_DIR" ]; then
        log_error "Baseline directory not found: $BASELINE_WEBGL_DIR"
        log_step "Run full test suite first to create baseline: ./scripts/test-gal-regression.sh"
        return 1
    fi

    if [ ! -d "$WEBGL_OUTPUT_DIR" ]; then
        log_error "WebGL output directory not found: $WEBGL_OUTPUT_DIR"
        return 1
    fi

    # Check for ImageMagick
    if ! command -v compare &> /dev/null; then
        log_error "ImageMagick 'compare' command not found. Install with: brew install imagemagick"
        return 1
    fi

    local total=0
    local matching=0
    local different=0
    local missing=0

    # Create temp directory for normalized images
    local tmpdir=$(mktemp -d)
    trap "rm -rf '$tmpdir'" EXIT

    # Files to exclude from comparison (documented as broken/dead code)
    local excluded_files="gal-transform-api.png"

    # Compare all baseline screenshots
    for baseline in "$BASELINE_WEBGL_DIR"/*.png; do
        [ -e "$baseline" ] || continue  # Handle no matches

        local filename=$(basename "$baseline")

        # Skip excluded files
        if echo "$excluded_files" | grep -q "$filename"; then
            if [ -n "$VERBOSE" ]; then
                echo "  SKIPPED: $filename (excluded - see README.md)"
            fi
            continue
        fi

        local current="$WEBGL_OUTPUT_DIR/$filename"
        total=$((total + 1))

        if [ ! -f "$current" ]; then
            echo "  MISSING: $filename"
            missing=$((missing + 1))
            continue
        fi

        # Get dimensions
        local baseline_dims=$(identify -format "%wx%h" "$baseline" 2>/dev/null)
        local current_dims=$(identify -format "%wx%h" "$current" 2>/dev/null)

        # If dimensions differ, resize current to match baseline
        local compare_file="$current"
        if [ "$baseline_dims" != "$current_dims" ]; then
            compare_file="$tmpdir/resized_$filename"
            convert "$current" -resize "$baseline_dims!" "$compare_file" 2>/dev/null
            if [ -n "$VERBOSE" ]; then
                echo "  RESIZED: $filename ($current_dims -> $baseline_dims)"
            fi
        fi

        # Normalize both images to TrueColor RGB for consistent comparison
        local baseline_normalized="$tmpdir/baseline_$filename"
        local current_normalized="$tmpdir/current_$filename"
        convert "$baseline" -flatten -colorspace sRGB -type TrueColor "$baseline_normalized" 2>/dev/null
        convert "$compare_file" -flatten -colorspace sRGB -type TrueColor "$current_normalized" 2>/dev/null

        # Compare with fuzz factor (allows small pixel differences from anti-aliasing)
        # Use AE (Absolute Error) metric - counts differing pixels
        local compare_output=$(compare -metric AE -fuzz 2% "$baseline_normalized" "$current_normalized" null: 2>&1 || true)
        local diff_pixels=$(echo "$compare_output" | awk '{print $1}')

        # Handle scientific notation (e.g., 1.92e+06)
        if [[ "$diff_pixels" =~ [eE] ]]; then
            diff_pixels=$(printf "%.0f" "$diff_pixels")
        fi

        # Handle error cases (non-numeric output)
        if ! [[ "$diff_pixels" =~ ^[0-9]+\.?[0-9]*$ ]]; then
            echo "  ERROR: $filename (comparison failed: $compare_output)"
            different=$((different + 1))
            continue
        fi

        # Calculate percentage
        local total_pixels=$(identify -format "%[fx:w*h]" "$baseline_normalized" 2>/dev/null)
        # Handle scientific notation
        if [[ "$total_pixels" =~ [eE] ]]; then
            total_pixels=$(printf "%.0f" "$total_pixels")
        fi
        if [ -z "$total_pixels" ] || [ "$total_pixels" = "0" ]; then
            total_pixels=1  # Avoid division by zero
        fi

        # Use awk for floating point arithmetic
        local diff_pct=$(awk "BEGIN {printf \"%.4f\", ($diff_pixels * 100.0) / $total_pixels}")

        # Compare against threshold
        local is_match=$(awk "BEGIN {print ($diff_pct < $threshold) ? 1 : 0}")

        if [ "$is_match" -eq 1 ]; then
            matching=$((matching + 1))
            if [ -n "$VERBOSE" ] || [ "$diff_pixels" -gt 0 ]; then
                echo "  MATCH: $filename (${diff_pct}% different)"
            fi
        else
            different=$((different + 1))
            echo "  DIFFERENT: $filename (${diff_pct}% different, threshold: ${threshold}%)"

            # Show diagnostic info for failing scenario
            if [ -n "$VERBOSE" ]; then
                echo "    Content bounds:"
                local baseline_bounds=$(magick "$baseline" -flatten -fuzz 1% -trim -format "%w x %h at %O" info: 2>/dev/null || echo "N/A")
                local current_bounds=$(magick "$current" -fuzz 1% -trim -format "%w x %h at %O" info: 2>/dev/null || echo "N/A")
                echo "      Baseline: $baseline_bounds"
                echo "      Current:  $current_bounds"
            fi
        fi
    done

    # Check for extra files
    local extra=0
    for current in "$WEBGL_OUTPUT_DIR"/*.png; do
        [ -e "$current" ] || continue
        local filename=$(basename "$current")
        local baseline="$BASELINE_WEBGL_DIR/$filename"
        if [ ! -f "$baseline" ]; then
            extra=$((extra + 1))
            echo "  EXTRA: $filename (not in baseline)"
        fi
    done

    echo ""
    echo "  Results: $matching/$total matching, $different different, $missing missing, $extra extra"

    if [ "$different" -gt 0 ] || [ "$missing" -gt 0 ]; then
        log_error "WebGL vs Baseline: FAILED"
        echo ""
        echo "MIGRATION REGRESSION DETECTED!"
        echo "WebGL output has changed from baseline. This indicates the migration"
        echo "has altered rendering behavior. Please investigate before proceeding."
        echo ""
        echo "To update baseline (if changes are intentional):"
        echo "  cp $WEBGL_OUTPUT_DIR/*.png $BASELINE_WEBGL_DIR/"
        echo "  git add $BASELINE_WEBGL_DIR/"
        echo "  git commit -m 'Update WebGL baseline after migration'"
        return 1
    else
        log_success "WebGL vs Baseline: PASSED"
        return 0
    fi
}

# ============================================================================
# Build Function
# ============================================================================

build_webgl() {
    log_header "Building WebGL Test"

    if [ ! -f "$SCRIPT_DIR/build-gal-webgl-test.sh" ]; then
        log_error "WebGL build script not found: $SCRIPT_DIR/build-gal-webgl-test.sh"
        exit 1
    fi

    # Check if Emscripten is available
    if ! command -v emcmake &> /dev/null; then
        log_error "Emscripten not available - WebGL build requires emsdk or Docker"
        log_step "Activate emsdk first or run inside Docker container"
        exit 1
    fi

    log_step "Running scripts/build-gal-webgl-test.sh..."

    if "$SCRIPT_DIR/build-gal-webgl-test.sh"; then
        log_success "WebGL build succeeded"
    else
        log_error "WebGL build failed"
        exit 1
    fi
}

# ============================================================================
# Run Function
# ============================================================================

run_webgl() {
    log_header "Running WebGL Test"

    local spec_file="$PROJECT_ROOT/tests/e2e/gal-webgl.spec.ts"

    if [ ! -f "$spec_file" ]; then
        log_error "WebGL test spec not found: $spec_file"
        exit 1
    fi

    # Create output directory
    mkdir -p "$WEBGL_OUTPUT_DIR"

    log_step "Running Playwright test..."

    cd "$PROJECT_ROOT/tests"
    if npx playwright test gal-webgl.spec.ts; then
        local count=$(ls -1 "$WEBGL_OUTPUT_DIR"/*.png 2>/dev/null | wc -l | tr -d ' ')
        log_success "WebGL test completed: $count screenshots generated"
    else
        log_error "WebGL test failed"
        exit 1
    fi
    cd "$PROJECT_ROOT"
}

# ============================================================================
# Main
# ============================================================================

log_header "GAL WebGL Regression Test (Monitoring)"
echo "Project Root:    $PROJECT_ROOT"
echo "Baseline WebGL:  $BASELINE_WEBGL_DIR"
echo "Current Output:  $WEBGL_OUTPUT_DIR"
echo ""
echo "Purpose: Detect regressions during WebGL GAL migration to KiCad"
echo "Strategy: Compare current WebGL output against baseline-webgl"

# Build phase (always clean build to avoid stale object issues)
build_webgl

# Run phase
run_webgl

# Compare phase
log_header "Comparing WebGL vs Baseline"
if compare_webgl_screenshots; then
    COMPARE_STATUS=0
else
    COMPARE_STATUS=1
fi

# Final summary
log_header "Final Result"

if [ "$COMPARE_STATUS" -eq 0 ]; then
    log_success "WebGL output matches baseline perfectly!"
    echo ""
    echo "Migration safety check PASSED. Rendering fidelity preserved."
    exit 0
else
    log_error "WebGL output differs from baseline!"
    echo ""
    echo "Migration safety check FAILED. Review changes before proceeding."
    exit 1
fi
