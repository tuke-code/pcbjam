# `-fexceptions` vs `-fwasm-exceptions` in KiCad-WASM — research dossier

> **Status:** research / decision record. A parallel session attempted the migration
> end-to-end and **parked it** on an emscripten-4.0.2 LLVM codegen bug — see
> [`docs/wasm-exceptions-experiment.md`](../../wasm-exceptions-experiment.md) (full
> build-plumbing patch preserved in its appendix; flags
> `-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0`, raw linked
> pcbnew 92 MB and zero `invoke_*`/`dynCall`). This dossier is the research companion:
> mechanism, measurements, audit, and the asyncify catch-block design.
> Authored 2026-06-11/12. Companion to [`docs/features/async/`](../async/) (the Asyncify
> `currData` contention dossier) — this dossier covers the *exception-handling* axis of
> the same machine.

> **UPDATE 2026-06-22 (see [`06-spike-plan.md`](06-spike-plan.md)).** A 5-agent spike refreshed
> this dossier and corrected three things below: (1) **the encoding is resolved to LEGACY**
> (`WASM_LEGACY_EXCEPTIONS=1`) — Asyncify can't consume exnref in any released Binaryen, so the
> "exnref → TryTable variant" fork is closed; the experiment's `=0` was a dead end. (2) **Binaryen
> is not a blocker** — CI/publish already pin `BINARYEN_VERSION=130` (the "v121 locally" note below
> is only the finalize/in-link copy). (3) The long pole is the **emsdk/LLVM compiler bump** for
> parseable legacy wasm-EH + the OCC `br_table` fix, *not* a newer wasm-opt. The phased red-green
> plan lives in 06.

## Why this exists

The whole build is on **`-fexceptions`** (Emscripten's JavaScript-based exception
handling). That choice is not cosmetic — it is the single largest driver of our Asyncify
cost: it forces `env.invoke_*` into `ASYNCIFY_IMPORTS`, which makes nearly the whole call
graph "suspension-capable" and therefore instrumented. We **measured** the consequence on
our own binary: 59% of the raw asyncify tax (64% of the gzipped tax) on pcbnew exists only
because of the invoke machinery. Migrating to native **`-fwasm-exceptions`** would cut the
shipped pcbnew download from **64.5 MB to ~36 MB (−44%)** and the module from
**187 MB to ~122 MB (−35%)**, plus an unmeasured-but-real runtime win on every try-region
hot path.

The migration is currently blocked by one upstream limitation — Binaryen's Asyncify pass
cannot handle a suspension *inside a catch handler* — and KiCad triggers exactly that
pattern (modal error dialogs from catch blocks) in **85 audited places**. This dossier
records the mechanism, the measurements, the toolchain status, the audit, and a concrete
**fork design (catch-arm hoisting)** that would remove the blocker without refactoring
KiCad at all.

## TL;DR / decision

- **Stay on `-fexceptions` for now.** Asyncify stays under any design (fibers + EM_ASYNC_JS
  have no wasm-EH replacement); this is purely about how much it must instrument.
- The prize is measured, not estimated: **−44% download, −35% module size** (see 02).
- Binaryen merged *partial* asyncify+wasm-EH support in **v125** (2025-11-19). Our emsdk
  bundles **v121** — we don't even have the partial support locally.
- The remaining hole — unwind-from-catch — is fixable with a **bounded new Binaryen pass**
  (catch-arm hoisting, ~400–800 lines, 1–2 weeks, genuinely upstreamable; see 05). It is a
  *pre-pass* before stock `--asyncify` — `Asyncify.cpp` itself needs zero changes, so
  "fork" overstates it (one added file; `get-wasm-opt.sh` already has the
  build-from-source deployment path). It **obsoletes the 85-site KiCad refactor** entirely.
- **Blocker ordering (learned from the parked experiment):** the catch-block limitation
  is blocker #2. Blocker #1 is an **LLVM codegen bug in emscripten 4.0.2** (invalid
  `br_table` arity in OpenCASCADE code under wasm-EH) — needs an emsdk bump first. And the
  experiment had to force the **exnref encoding** (`WASM_LEGACY_EXCEPTIONS=0`, because
  4.0.2's legacy encoding output doesn't even parse in Binaryen), which collides with the
  fact that Binaryen's asyncify has **zero `TryTable` support**: the encoding choice after
  the emsdk bump decides which variant of the catch fix applies (see 03 §experiment, 05).
- Trigger to act: when we are ready to invest ~2 weeks of toolchain work, or if upstream
  lands full support on binaryen #4470 first. Until then the Asyncify arbiter work
  (docs/features/async/) is the priority — it fixes shipping bugs and is needed either way.

## Document index

| File | Contents |
|---|---|
| [`01-background-two-eh-models.md`](01-background-two-eh-models.md) | How JS-EH (`invoke_*`) and wasm-EH actually work, and the three concrete couplings into our Asyncify machine. |
| [`02-measurements.md`](02-measurements.md) | Our controlled size experiment on pcbnew (methodology + numbers) and the published third-party benchmarks. |
| [`03-toolchain-status.md`](03-toolchain-status.md) | Compatibility matrix: emcc checks, binaryen history (what merged in v125, what didn't), JSPI/fibers, setjmp/longjmp, mixing modes. |
| [`04-kicad-audit.md`](04-kicad-audit.md) | The brace-matching catch-block audit: 85 direct / 93 review / 458 trivial of 636; libpng/libjpeg setjmp story; refactor effort if done by hand. |
| [`05-asyncify-fork-design.md`](05-asyncify-fork-design.md) | Asyncify.cpp internals, why catch arms are structurally hard, and the catch-arm-hoisting fork design with limits and effort. |
| [`catch_audit.py`](catch_audit.py) | The audit tool (re-runnable; suitable as a CI gate on the kicad submodule). |
| [`audit-results.txt`](audit-results.txt) | Full audit output incl. all 85 direct-suspend sites. |
| [`06-spike-plan.md`](06-spike-plan.md) | **(2026-06-22)** Refreshed findings + the phased red-green spike plan; supersedes the encoding/Binaryen-version framing above. |

## Relationship to docs/features/async/

Independent axes of the same machine. The async dossier is about *correctness* (one global
`Asyncify.currData` shared by overlapping suspensions → crash/hang); this dossier is about
*cost* (how much code Asyncify instruments). Fixing one does not fix the other. Sequencing:
async arbiter first (shipping bugs), wasm-EH migration second (size/speed), and the
migration plan below assumes the arbiter exists.

## Migration plan (when triggered)

0. Resume the parked experiment (`docs/wasm-exceptions-experiment.md`): bump emsdk past
   the 4.0.2 LLVM `br_table` bug, `git apply` its appendix patch (`KICAD_WASM_EH=1`
   gated), full clean deps rebuild (stamp-skip gotcha: stale sjlj objects in cairo etc.).
   Then decide the EH encoding: if newer LLVM emits parseable **legacy** encoding, the
   catch-arm-hoisting pre-pass (05) applies; if **exnref** stays forced, asyncify needs
   `TryTable` support + exnref spilling instead (05 §new-EH variant).
1. Newer Binaryen for the post-link step only: `scripts/common/get-wasm-opt.sh` already
   abstracts the binary — point it at a ≥ v125 build carrying the hoisting patch (05).
2. Validate partial support first: rebuild one app `-fwasm-exceptions` + asyncify-asserts,
   run the e2e suites; the asserts tripwire makes any missed unwind-from-catch a
   deterministic trap.
3. Uniform flag flip: `-fexceptions` → `-fwasm-exceptions` in
   `scripts/build-wxuniversal-wasm.sh:141-142`, `scripts/kicad/build-kicad-target.sh`
   (lines ~203/207/211/214), `tests/apps/Makefile.wasm` (all occurrences) — plus
   `-sSUPPORT_LONGJMP=wasm` (default with wasm-EH; the `emscripten` flavor is a hard error).
4. Drop `env.invoke_*` from `ASYNCIFY_IMPORTS` in `scripts/common/apply-asyncify.sh:33`.
5. Expect to delete/shrink shim machinery that exists only for the invoke world
   (`inject-dyncall-shims.sh` phases 1–2) — verify, don't assume.
6. Keep `catch_audit.py` as a CI gate only if shipping *without* the fork (i.e., the
   hand-refactor path); with the fork it is informational.
