#!/usr/bin/env bash
# Regression test for the value-typed (concrete-result) path of --hoist-cpp-catches.
#
# Value-typed cpp-catch tries do not arise from normal C++ EH lowering (LLVM keeps catch values in
# locals → void/unreachable tries), so this case can't live in a C++ EH toy. These
# hand-written modules exercise it directly:
#   (1) fuzz-exec — the pass must preserve the result value of value-typed cpp-catch tries
#       (exception path, no-exception path, and exception-payload routing).
#   (2) a real asyncify unwind/rewind through a value-typed catch that SUSPENDS (must yield 50).
#
# Requires wasm-opt built from the binaryen submodule (scripts/binaryen-hoist-pass/build-wasm-opt.sh);
# that one binary (version_130 + our hoist pass) does both --hoist-cpp-catches and --asyncify. Run from anywhere.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
DIR="$ROOT/scripts/binaryen-hoist-pass/tests"
WASMOPT="$ROOT/build-wasm/tools/binaryen-hoist-build/bin/wasm-opt"
V130="$WASMOPT"   # the submodule fork IS version_130 (asyncify unchanged), so it does --asyncify too
[ -x "$WASMOPT" ] || { echo "build wasm-opt first: scripts/binaryen-hoist-pass/build-wasm-opt.sh"; exit 1; }

echo "== (1) value semantics preserved (fuzz-exec) =="
"$WASMOPT" --hoist-cpp-catches -all -all --fuzz-exec "$DIR/value-typed-cpp-catch.wat" -o /dev/null 2>&1 \
  | grep -E 'comparing|=>'

echo "== (2) asyncify unwind/rewind through a value-typed suspending catch =="
"$WASMOPT" --hoist-cpp-catches -all -all "$DIR/value-typed-suspend.wat" -o /tmp/vt_s.hoisted.wasm
"$V130" --asyncify -all --pass-arg=asyncify-imports@env.sleep /tmp/vt_s.hoisted.wasm -o /tmp/vt_s.async.wasm
node "$DIR/asyncify-harness.js" /tmp/vt_s.async.wasm
echo "OK — value-typed path verified"

# Regression for the PGM_BASE::HandleException shape: a cpp catch nested in an outer try's catch_all
# cleanup pad whose arm br's to an intervening block ($blk). Before the fix the hoisted arm's br was
# orphaned -> "all break targets must be valid". (3) validates + checks value semantics; (4) drives a
# real unwind/rewind through the hoisted arm with the retargeted br.
echo "== (3) nested-catchall exit-block hoists + validates (fuzz-exec: caught=>42, normal=>7) =="
"$WASMOPT" --hoist-cpp-catches -all -all --fuzz-exec "$DIR/nested-catchall-exit-block.wat" -o /dev/null 2>&1 \
  | grep -E 'comparing|=>'

echo "== (4) asyncify unwind/rewind through a suspending nested-catchall arm =="
"$WASMOPT" --hoist-cpp-catches -all -all "$DIR/nested-catchall-suspend.wat" -o /tmp/ncb_s.hoisted.wasm
"$V130" --asyncify -all --pass-arg=asyncify-imports@env.sleep /tmp/ncb_s.hoisted.wasm -o /tmp/ncb_s.async.wasm
node "$DIR/asyncify-harness.js" /tmp/ncb_s.async.wasm
echo "OK — nested-catchall path verified"

# Regression for the SYMBOL_EDIT_FRAME::DuplicateSymbol shape: a cpp catch arm hoisted PAST an ancestor
# catch_all carries a nested __cxa_end_catch cleanup (try (do ..) (delegate $M)) whose delegate target
# sits inside that catch_all -> orphaned by hoisting ("all delegate targets must be valid"). The fix
# retargets it to DELEGATE_CALLER_TARGET. (5) validates + checks value semantics; (6) drives a real
# unwind/rewind through the hoisted arm with the retargeted delegate.
echo "== (5) delegate-orphan hoists + validates (fuzz-exec: caught=>42, normal=>7) =="
"$WASMOPT" --hoist-cpp-catches -all -all --fuzz-exec "$DIR/delegate-orphan.wat" -o /dev/null 2>&1 \
  | grep -E 'comparing|=>'

echo "== (6) asyncify unwind/rewind through a suspending delegate-orphan arm =="
"$WASMOPT" --hoist-cpp-catches -all -all "$DIR/delegate-orphan-suspend.wat" -o /tmp/dlg_s.hoisted.wasm
"$V130" --asyncify -all --pass-arg=asyncify-imports@env.sleep /tmp/dlg_s.hoisted.wasm -o /tmp/dlg_s.async.wasm
node "$DIR/asyncify-harness.js" /tmp/dlg_s.async.wasm
echo "OK — delegate-orphan path verified"
