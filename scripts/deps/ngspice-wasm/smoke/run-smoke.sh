#!/bin/bash
# Gate-1 smoke for the wasm ngspice build: compiles smoke.c against the
# installed sysroot artifacts and runs it under node. Proves, in one shot:
# static sharedspice links; RC transient numerics; XSPICE code models resolve
# through the static registry; CIDER (numd) simulates; bg_run/bg_halt work on
# a real pthread. See smoke.c for the assertions.
#
# Usage: scripts/deps/ngspice-wasm/smoke/run-smoke.sh
# (build ngspice first: scripts/deps/build-ngspice.sh)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QUIET=1 source "${SCRIPT_DIR}/../../../common/env.sh"

SMOKE_BUILD="${BUILD_ROOT}/ngspice-smoke"
mkdir -p "${SMOKE_BUILD}"

# NODERAWFS: the wasm module sees the real filesystem, so the NGSPICEDATADIR
# baked into libngspice (the sysroot prefix) resolves and spinit + code-model
# registration run exactly as they will in the service worker.
# PROXY_TO_PTHREAD: main() may block (usleep) while ngspice's bg thread runs.
emcc "${SCRIPT_DIR}/smoke.c" -o "${SMOKE_BUILD}/smoke.js" \
    -I"${SYSROOT}/include" \
    -O1 -g -pthread ${DEPS_EH_FLAGS} \
    "${SYSROOT}/lib/libngspice.a" \
    "${SYSROOT}"/lib/ngspice/*.cm \
    "${SYSROOT}/lib/ngspice/ngcm_common.a" \
    -sENVIRONMENT=node \
    -sNODERAWFS=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=256MB \
    -sPROXY_TO_PTHREAD \
    -sPTHREAD_POOL_SIZE=8 \
    -sEXIT_RUNTIME=1 \
    -sSTACK_SIZE=4MB \
    -sDEFAULT_PTHREAD_STACK_SIZE=2MB

# Optional argument: run a single scenario (rc|xspice|cider|halt).
node "${SMOKE_BUILD}/smoke.js" "$@"
