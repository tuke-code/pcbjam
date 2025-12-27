#!/bin/bash
# Download and cache Binaryen wasm-opt
#
# Usage: ./scripts/tools/get-wasm-opt.sh
# Output: Prints path to wasm-opt executable
#
# Binaryen v121 is used because v125 has a regression causing asyncify crashes.
# The binary is cached in tools/binaryen-121/

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

BINARYEN_VERSION="121"
BINARYEN_DIR="${PROJECT_ROOT}/build-wasm/tools/binaryen-${BINARYEN_VERSION}"
WASM_OPT="${BINARYEN_DIR}/bin/wasm-opt"

download_binaryen() {
    # Detect platform
    local os=$(uname -s)
    local arch=$(uname -m)
    local platform=""

    case "${os}-${arch}" in
        Darwin-arm64)  platform="arm64-macos" ;;
        Darwin-x86_64) platform="x86_64-macos" ;;
        Linux-aarch64) platform="aarch64-linux" ;;
        Linux-x86_64)  platform="x86_64-linux" ;;
        *)
            echo "ERROR: Unsupported platform: ${os}-${arch}" >&2
            exit 1
            ;;
    esac

    local url="https://github.com/WebAssembly/binaryen/releases/download/version_${BINARYEN_VERSION}/binaryen-version_${BINARYEN_VERSION}-${platform}.tar.gz"
    local tarball="${PROJECT_ROOT}/build-wasm/tools/binaryen-${BINARYEN_VERSION}.tar.gz"

    echo "Downloading Binaryen v${BINARYEN_VERSION} for ${platform}..." >&2
    mkdir -p "${PROJECT_ROOT}/build-wasm/tools"
    curl -L -o "${tarball}" "${url}"

    echo "Extracting..." >&2
    tar -xzf "${tarball}" -C "${PROJECT_ROOT}/build-wasm/tools"
    mv "${PROJECT_ROOT}/build-wasm/tools/binaryen-version_${BINARYEN_VERSION}" "${BINARYEN_DIR}"
    rm "${tarball}"

    echo "Binaryen v${BINARYEN_VERSION} installed to ${BINARYEN_DIR}" >&2
}

# Download Binaryen if not cached
if [ ! -x "${WASM_OPT}" ]; then
    download_binaryen
fi

# Verify version
INSTALLED_VERSION=$("${WASM_OPT}" --version 2>&1 | grep -o '[0-9]\+' | head -1)
if [ "${INSTALLED_VERSION}" != "${BINARYEN_VERSION}" ]; then
    echo "WARNING: wasm-opt version mismatch (got ${INSTALLED_VERSION}, expected ${BINARYEN_VERSION})" >&2
    echo "Re-downloading..." >&2
    rm -rf "${BINARYEN_DIR}"
    download_binaryen
fi

# Output path to wasm-opt (this is the only stdout output)
echo "${WASM_OPT}"