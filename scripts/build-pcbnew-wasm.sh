#!/bin/bash
# Build KiCad PCBnew for WebAssembly
# This builds the PCB editor as a standalone WASM application

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common/env.sh"
source "${SCRIPT_DIR}/common/versions.sh"
source "${SCRIPT_DIR}/common/functions.sh"

KICAD_DIR="${PROJECT_ROOT}/kicad"
KICAD_BUILD="${BUILD_ROOT}/kicad"
KICAD_STAMP="${BUILD_ROOT}/stamps/kicad-pcbnew.stamp"
WASM_LAYER="${PROJECT_ROOT}/wasm"

# Parse arguments
CLEAN=0
SKIP_DEPS=0
DEBUG=0
for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=1
            ;;
        --skip-deps)
            SKIP_DEPS=1
            ;;
        --debug)
            DEBUG=1
            ;;
    esac
done

if [ $CLEAN -eq 1 ]; then
    log_info "Cleaning KiCad build..."
    rm -rf "${KICAD_BUILD}" "${KICAD_STAMP}"
fi

# Build dependencies first
if [ $SKIP_DEPS -eq 0 ]; then
    log_info "Building dependencies..."
    "${SCRIPT_DIR}/deps/build-all-deps.sh" --all
fi

# Check if already built
if check_stamp "${KICAD_STAMP}"; then
    log_info "KiCad PCBnew already built, skipping..."
    exit 0
fi

# Ensure wxWidgets is built
if [ ! -f "${SYSROOT}/lib/libwx_baseu-3.2.a" ]; then
    log_error "wxWidgets not found. Please build wxWidgets first with:"
    log_error "  ./scripts/build-wxuniversal-wasm.sh"
    exit 1
fi

log_info "Building KiCad PCBnew ${KICAD_VERSION} for WASM..."

# Set build type
if [ $DEBUG -eq 1 ]; then
    BUILD_TYPE="Debug"
    EXTRA_FLAGS="-g -O0"
else
    BUILD_TYPE="Release"
    EXTRA_FLAGS="-O2"
fi

mkdir -p "${KICAD_BUILD}"
cd "${KICAD_BUILD}"

# Configure KiCad with WASM-specific options
# We use CMAKE_MODULE_PATH to inject our compatibility layer
emcmake cmake "${KICAD_DIR}" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="${SYSROOT}" \
    -DCMAKE_MODULE_PATH="${WASM_LAYER}/cmake" \
    -DCMAKE_CXX_FLAGS="${EXTRA_FLAGS} -pthread -DKICAD_USE_PLATFORM_WASM=1" \
    -DCMAKE_C_FLAGS="${EXTRA_FLAGS} -pthread" \
    -DCMAKE_EXE_LINKER_FLAGS="-pthread -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=65536 -sUSE_PTHREADS=1 -sPTHREAD_POOL_SIZE=4 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sMAXIMUM_MEMORY=4GB" \
    -DCMAKE_PREFIX_PATH="${SYSROOT}" \
    -DwxWidgets_CONFIG_EXECUTABLE="${SYSROOT}/bin/wx-config" \
    \
    -DKICAD_BUILD_QA_TESTS=OFF \
    -DKICAD_SCRIPTING=OFF \
    -DKICAD_SCRIPTING_PYTHON3=OFF \
    -DKICAD_SCRIPTING_WXPYTHON=OFF \
    -DKICAD_SPICE=ON \
    -DKICAD_USE_OCC=ON \
    -DKICAD_USE_EGL=OFF \
    -DKICAD_USE_BUNDLED_GLEW=ON \
    \
    -DOCC_INCLUDE_DIR="${SYSROOT}/include/opencascade" \
    -DOCC_LIBRARY_DIR="${SYSROOT}/lib" \
    -DNGSPICE_INCLUDE_DIR="${SYSROOT}/include" \
    -DNGSPICE_LIBRARY="${SYSROOT}/lib/libngspice.a" \
    \
    -DBUILD_GITHUB_PLUGIN=OFF \
    -DKICAD_PCM=OFF \
    \
    -DKICAD_LIBRARY_DATA="${PROJECT_ROOT}/kicad-library"

# Build only pcbnew and its dependencies
# Note: We build specific targets to avoid building unnecessary components
emmake make -j${JOBS} pcbnew

create_stamp "${KICAD_STAMP}"
log_info "KiCad PCBnew build complete!"
log_info "Output: ${KICAD_BUILD}/pcbnew/pcbnew.js"
