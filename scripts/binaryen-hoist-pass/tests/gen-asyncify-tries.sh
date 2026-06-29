#!/usr/bin/env bash
# gen-asyncify-tries.sh N [M]  ->  emits (to stdout) a .wat module with ONE function that contains
# N sequential native try/catch regions plus a suspend, using M live locals (default 8).
#
# This is the controlled reproduction of the Binaryen Asyncify memory/CPU blowup on native wasm-EH:
# KiCad's worst function (BuildBitmapInfo) has ~4,986 native tries in a single function, and asyncify's
# per-function cost (it builds a CFG + liveness over every try) is superlinear in the try count. Scaling
# N here lets us measure peak RSS / wall-time vs N and profile a moderate N without OOM.
#
# Each try is value-typed (mimics RAII-wrapped construction) and threads M locals through its body so
# the liveness analysis has real long-range live variables (as in the real function). The single
# `call $sleep` makes the whole function asyncify-instrumented.
set -euo pipefail
N="${1:-1000}"
M="${2:-8}"

printf '(module\n'
printf ' (import "env" "sleep" (func $sleep (param i32) (result i32)))\n'
printf ' (memory (export "memory") 1)\n'
printf ' (tag $cpp (param i32))\n'
printf ' (func $big (export "big") (param $p i32) (result i32)\n'
printf '  (local $acc i32)\n'
printf '  (local $t i32)\n'
for ((k=0; k<M; k++)); do printf '  (local $v%d i32)\n' "$k"; done
# one suspend so the function is asyncify-instrumented
printf '  (local.set $acc (call $sleep (i32.const 1)))\n'
for ((k=0; k<M; k++)); do printf '  (local.set $v%d (i32.add (local.get $p) (i32.const %d)))\n' "$k" "$k"; done
for ((i=0; i<N; i++)); do
  vi=$(( i % M ))
  vj=$(( (i + 1) % M ))
  # value-typed try: body mixes several live locals; caught arm yields the payload. The result feeds
  # $v<vi>, keeping every $v live across the whole sequence (long live ranges -> liveness pressure).
  printf '  (local.set $v%d\n' "$vi"
  printf '   (try (result i32)\n'
  printf '    (do (i32.add (i32.add (local.get $v%d) (local.get $v%d)) (local.get $acc)))\n' "$vi" "$vj"
  printf '    (catch $cpp (i32.add (pop i32) (local.get $v%d)))))\n' "$vj"
  printf '  (local.set $acc (i32.add (local.get $acc) (local.get $v%d)))\n' "$vi"
done
printf '  (local.get $acc))\n'
printf ')\n'
