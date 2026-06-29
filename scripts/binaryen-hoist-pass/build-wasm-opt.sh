#!/bin/bash
# Build a wasm-opt that includes the catch-arm-hoisting pass (--hoist-cpp-catches).
#
# Source of truth is the tracked Binaryen submodule (binaryen/, branch wasm-port =
# upstream version_130 + src/passes/HoistCppCatches.cpp). This configures an out-of-source
# build into the gitignored build-wasm/ tree and prints the wasm-opt path on stdout
# (build progress to stderr). See docs/features/wasm-exceptions/06-spike-plan.md (Phase 1.5).
set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SRC="${PROJECT_ROOT}/binaryen"
BUILD="${PROJECT_ROOT}/build-wasm/tools/binaryen-hoist-build"

if [ ! -f "${SRC}/src/passes/HoistCppCatches.cpp" ]; then
    echo "ERROR: binaryen submodule is missing the hoist pass." >&2
    echo "       Run: git submodule update --init binaryen" >&2
    exit 1
fi

# Configure once (mirrors scripts/common/get-wasm-opt.sh's from-source flags).
if [ ! -f "${BUILD}/build.ninja" ]; then
    echo "Configuring Binaryen submodule build (one-time, ~5 min to build)..." >&2
    cmake -S "${SRC}" -B "${BUILD}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-Wno-maybe-uninitialized" \
        -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON -DBUILD_TESTS=OFF >&2
fi

ninja -C "${BUILD}" wasm-opt >&2

echo "${BUILD}/bin/wasm-opt"
