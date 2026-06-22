#!/usr/bin/env bash
# Regression test for the value-typed (concrete-result) path of --hoist-cpp-catches.
#
# Value-typed cpp-catch tries do not arise from normal C++ EH lowering (LLVM keeps catch values in
# locals → void/unreachable tries), so this case can't live in the C++ eh-spike toy. These
# hand-written modules exercise it directly:
#   (1) fuzz-exec — the pass must preserve the result value of value-typed cpp-catch tries
#       (exception path, no-exception path, and exception-payload routing).
#   (2) a real asyncify unwind/rewind through a value-typed catch that SUSPENDS (must yield 50).
#
# Requires wasm-opt built from the binaryen submodule (scripts/binaryen-hoist-pass/build-wasm-opt.sh)
# and the pinned v130 wasm-opt (build-wasm/tools/binaryen-130). Run from anywhere.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
DIR="$ROOT/scripts/binaryen-hoist-pass/tests"
WASMOPT="$ROOT/build-wasm/tools/binaryen-hoist-build/bin/wasm-opt"
V130="$ROOT/build-wasm/tools/binaryen-130/bin/wasm-opt"
[ -x "$WASMOPT" ] || { echo "build wasm-opt first: scripts/binaryen-hoist-pass/build-wasm-opt.sh"; exit 1; }

echo "== (1) value semantics preserved (fuzz-exec) =="
"$WASMOPT" --hoist-cpp-catches -all -all --fuzz-exec "$DIR/value-typed-cpp-catch.wat" -o /dev/null 2>&1 \
  | grep -E 'comparing|=>'

echo "== (2) asyncify unwind/rewind through a value-typed suspending catch =="
"$WASMOPT" --hoist-cpp-catches -all -all "$DIR/value-typed-suspend.wat" -o /tmp/vt_s.hoisted.wasm
"$V130" --asyncify -all --pass-arg=asyncify-imports@env.sleep /tmp/vt_s.hoisted.wasm -o /tmp/vt_s.async.wasm
node "$DIR/asyncify-harness.js" /tmp/vt_s.async.wasm
echo "OK — value-typed path verified"
