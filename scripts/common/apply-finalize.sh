#!/bin/bash
# Apply wasm-emscripten-finalize transformation on host.
#
# Usage: ./scripts/common/apply-finalize.sh <input.wasm> <output.wasm>

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# wasm-emscripten-finalize comes from our Binaryen submodule (version_130 + --hoist-cpp-catches),
# built alongside wasm-opt by build-wasm-opt.sh. Pinning finalize and the asyncify wasm-opt to ONE
# Binaryen version removes the host's emsdk dependency from the post-process entirely (it is now
# dyncall=node + finalize/asyncify=submodule), which is what was breaking on the ephemeral CI host.
# HOIST_WASMOPT, when the build driver pre-warms the submodule build, points at the already-built
# wasm-opt so we don't re-invoke the build per app; finalize sits next to it in the same bin/.
WASM_OPT="${HOIST_WASMOPT:-$("${SCRIPT_DIR}/../binaryen-hoist-pass/build-wasm-opt.sh")}"
FINALIZE="$(dirname "${WASM_OPT}")/wasm-emscripten-finalize"

INPUT_WASM="${1:-output/pcbnew.wasm}"
OUTPUT_WASM="${2:-${INPUT_WASM}}"

if [ ! -f "${INPUT_WASM}" ]; then
    echo "ERROR: Input file not found: ${INPUT_WASM}"
    exit 1
fi

echo "Applying wasm-emscripten-finalize..."
echo "  Input:  ${INPUT_WASM}"
echo "  Output: ${OUTPUT_WASM}"
echo "  Tool:   ${FINALIZE}"

# Run finalize with the same flags Emscripten would use
# NOTE: --dwarf removed because debug info is in separate .debug.wasm file
"${FINALIZE}" \
    -g \
    --bigint \
    --no-legalize-javascript-ffi \
    --detect-features \
    "${INPUT_WASM}" \
    -o "${OUTPUT_WASM}"

echo "Finalize complete: ${OUTPUT_WASM}"
ls -lh "${OUTPUT_WASM}"
