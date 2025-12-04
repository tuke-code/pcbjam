#!/bin/bash
# Build all KiCad dependencies for WebAssembly
# This script builds dependencies in the correct order

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../common/env.sh"
source "${SCRIPT_DIR}/../common/functions.sh"

log_info "Building all KiCad dependencies for WASM..."
log_info "Using ${JOBS} parallel jobs"

# Parse arguments
CLEAN=""
WITH_OCC=0
WITH_NGSPICE=0
for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN="--clean"
            ;;
        --with-occ)
            WITH_OCC=1
            ;;
        --with-ngspice)
            WITH_NGSPICE=1
            ;;
        --all)
            WITH_OCC=1
            WITH_NGSPICE=1
            ;;
    esac
done

# Build dependencies in order

# 1. Header-only libraries (no dependencies)
log_info "=== Phase 1: Header-only libraries ==="
"${SCRIPT_DIR}/build-glm.sh" ${CLEAN}

# 2. Basic libraries (minimal dependencies)
log_info "=== Phase 2: Basic compression/serialization ==="
"${SCRIPT_DIR}/build-zstd.sh" ${CLEAN}
"${SCRIPT_DIR}/build-protobuf.sh" ${CLEAN}

# 3. Font rendering stack
log_info "=== Phase 3: Font rendering ==="
"${SCRIPT_DIR}/build-freetype.sh" ${CLEAN}
"${SCRIPT_DIR}/build-harfbuzz.sh" ${CLEAN}

# 4. Graphics stack (optional, for Cairo rendering)
log_info "=== Phase 4: Graphics libraries ==="
"${SCRIPT_DIR}/build-pixman.sh" ${CLEAN}
"${SCRIPT_DIR}/build-cairo.sh" ${CLEAN}

# 5. Optional heavy dependencies
if [ $WITH_OCC -eq 1 ]; then
    log_info "=== Phase 5a: OpenCASCADE (3D/STEP support) ==="
    "${SCRIPT_DIR}/build-opencascade.sh" ${CLEAN}
fi

if [ $WITH_NGSPICE -eq 1 ]; then
    log_info "=== Phase 5b: ngspice (SPICE simulation) ==="
    "${SCRIPT_DIR}/build-ngspice.sh" ${CLEAN}
fi

log_info "============================================"
log_info "All dependencies built successfully!"
log_info "Install prefix: ${SYSROOT}"
log_info "============================================"
