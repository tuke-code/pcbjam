#!/bin/bash
# Build the minimal native-EH Safari reproduction (emscripten #25365) in BOTH exception encodings,
# using the host emsdk (the toolchain KiCad-WASM ships). No asyncify / pthreads / embind — just
# -fwasm-exceptions + a real throw/catch — so a Safari load failure can only be the EH codegen.
#   repro-legacy.*  -sWASM_LEGACY_EXCEPTIONS=1  (what we ship today)
#   repro-new.*     -sWASM_LEGACY_EXCEPTIONS=0  (standardized exnref encoding)
set -e
cd "$(dirname "$0")"
ROOT="$(cd ../../.. && pwd)"
source "$ROOT/tools/emsdk/emsdk_env.sh" 2>/dev/null
echo "emcc: $(emcc --version | head -1)"
COMMON="-O1 -fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sMODULARIZE -sEXPORT_NAME=createRepro -sENVIRONMENT=web -sEXPORTED_RUNTIME_METHODS=[]"
echo "building legacy encoding..."; emcc repro.cpp $COMMON -sWASM_LEGACY_EXCEPTIONS=1 -o repro-legacy.js
echo "building new (exnref) encoding..."; emcc repro.cpp $COMMON -sWASM_LEGACY_EXCEPTIONS=0 -o repro-new.js
echo "built:"; ls -1 repro-legacy.js repro-legacy.wasm repro-new.js repro-new.wasm
