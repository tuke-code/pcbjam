#!/bin/bash
# Build KiCad WASM inside Docker container, then apply asyncify on host
#
# The build is split into two phases:
# 1. Docker: Compile KiCad to WASM (without asyncify)
# 2. Host: Apply asyncify transformation (uses Binaryen v121)
#
# Binaryen is downloaded automatically - no prerequisites needed.

set -e

cd "$(dirname "$0")/.."

# Get wasm-opt (downloads Binaryen v121 if not cached)
echo "Checking wasm-opt..."
WASM_OPT=$(./scripts/common/get-wasm-opt.sh)
echo "Using: ${WASM_OPT}"

# Start container if not running
docker compose -f docker/docker-compose.yml up -d

# Run build command (without asyncify - handled on host due to memory requirements)
docker compose -f docker/docker-compose.yml exec kicad-wasm-builder \
    /workspace/scripts/kicad/build-pcbnew.sh "$@"

# Copy output to host-accessible directory
echo "Copying build output to ./output/..."
docker compose -f docker/docker-compose.yml exec kicad-wasm-builder \
    bash -c "mkdir -p /workspace/output && cp /workspace/build-wasm/kicad-pcbnew/pcbnew/pcbnew.{js,wasm,wasm.map,worker.js} /workspace/output/ 2>/dev/null || cp /workspace/build-wasm/kicad-pcbnew/pcbnew/pcbnew.{js,wasm} /workspace/output/"

# Apply asyncify transformation on host
# This is done on the host because wasm-opt --asyncify needs 50GB+ RAM for KiCad,
# which exceeds typical Docker memory limits
echo ""
echo "Applying asyncify transformation on host..."
echo "This may take several minutes and use significant RAM..."

# Asyncify import patterns (functions that trigger async suspension)
# - env.invoke_* : Exception handling trampolines
# - env.__asyncjs__* : EM_ASYNC_JS functions
ASYNCIFY_IMPORTS="env.invoke_*,env.__asyncjs__*"

"${WASM_OPT}" --asyncify \
    --pass-arg=asyncify-imports@${ASYNCIFY_IMPORTS} \
    --pass-arg=asyncify-propagate-addlist \
    output/pcbnew.wasm -o output/pcbnew.wasm

echo ""
echo "Build complete. Output files in ./output/"
ls -lh output/
