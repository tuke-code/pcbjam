# 06 — Native wasm-EH: refreshed findings + red-green spike plan (2026-06-22)

> **Status:** plan / decision record, *in refinement* (no code written yet — this is the
> agreed artifact before Phase 0). Produced by a 5-agent research spike on 2026-06-22:
> browser support · toolchain · runtime mechanisms · the Binaryen pass · red-green harness.
> **Supersedes in part** `README.md`, `03-toolchain-status.md`, and the root
> `docs/wasm-exceptions-experiment.md` where called out below. Reads on top of 01–05.

---

## RESULTS — Phases 0 & 1 (2026-06-22, current toolchain, NO emsdk bump)

> **Cross-engine policy:** every spec runs in **all three engines — Firefox, Chrome (V8), and
> Safari/WebKit** (`cd tests && npm run test:asyncify:all`). The eh-spike harness is green in all
> three. Result notes below that say "V8/Firefox" predate the Safari run and now hold in WebKit too.

**Phase 0 — PASS.** em 4.0.2's LLVM emits *parseable, runnable* legacy wasm-EH:
- A trivial `-fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=1` throw/catch builds (finalize on
  bundled v121, **no "popping from empty stack"**) and runs correctly under node
  (`tests/apps/standalone/eh-spike/eh_probe.cpp`, Makefile `eh-probe` target). So the parked
  experiment's finalize failure was **scale/OCC-specific, not a general codegen break**.
- Binaryen **v130 asyncifies** the legacy-wasm-EH module (asyncify_* exports present); bundled
  **v121 crashes** (`UNREACHABLE … Asyncify.cpp:1146 — unexpected expression type`). Confirms
  the v121→v130 split; the post-link asyncify path (already `BINARYEN_VERSION=130` in CI) is the
  one to use. **No emsdk bump required for the spike.**

**Phase 1 — PASS (red-green captured).** One source built two ways
(`scripts/build-eh-spike.sh`; the wasm-EH variant uses the production stub→post-link-v130 flow).
Validated in **both V8 (node) and Firefox** by `tests/asyncify/eh-spike.spec.ts`:

| case | mechanism | JS-EH | native wasm-EH |
|---|---|---|---|
| `throw_across_sleep` | EM_ASYNC_JS sleep + EH | PASS | **PASS** |
| `fiber_then_throw` | `emscripten_fiber_swap` + EH | PASS | **PASS** |
| `suspend_in_catch` | suspend *inside* a catch arm | PASS | **HARD TRAP** — `indirect call to null` / `null function or function signature mismatch` |

→ asyncify + native wasm-EH **works for the sleep and fiber mechanisms**; the *only* failure is
suspend-inside-catch (Binaryen #4470), and it fails **loud and deterministically** (a trap, not a
silent drop) in both engines. This is exactly the hole the catch-arm-hoisting pass (05) closes —
the spike has now made it concrete and pinned it as a regression test.

**Artifacts:** `tests/apps/standalone/eh-spike/{eh_probe.cpp,eh_spike_test.cpp}`,
`scripts/build-eh-spike.sh`, `tests/asyncify/eh-spike.spec.ts`, the `eh-probe` Makefile target,
and `playwright-asyncify.config.ts` `testMatch` widened to include `eh-spike`.

**Phase 1.5 — PASS (the fix).** The catch-arm-hoisting Binaryen pass
(`scripts/binaryen-hoist-pass/HoistCppCatches.cpp`, ~150 LoC + a 39-line registration patch)
flips `suspend_in_catch` from a hard trap to **green** under native wasm-EH — validated in
**V8 + Firefox**. `tests/asyncify/eh-spike.spec.ts` is now a 3-variant ablation harness:
JS-EH green / wasm-EH-no-pass red / wasm-EH+hoist green (pins both disease and fix). The pass
outlines a cpp-tag catch arm containing a suspending call to plain code after the try (capture
payload→local + set flag + `br` out; in the hoisted handler `pop`→`local.get`, no-match
`rethrow`→`throw`), so stock `--asyncify` instruments it for free — exactly the 05 design. It is
a PRE-pass (`--hoist-cpp-catches` before `--asyncify`); `Asyncify.cpp` is unchanged. Built
reproducibly via `scripts/binaryen-hoist-pass/build-wasm-opt.sh` (drops the file into the v130
clone, applies the patch, `ninja`). **MVP scope** (sufficient for the toy): void/unreachable-typed
tries, single cpp catch (single-i32 tag), no catch_all, and a DIRECT suspending-import call in the
arm; the nested-pop / nested-rethrow cases are handled (via an `ExpressionStackWalker` Try-ancestor
guard). **Remaining for real KiCad** (all "fiddly-but-tractable" per 05): concrete-result-typed
tries (route the body value through a temp local), catch_all coexistence, and TRANSITIVE suspend
detection (catch → DisplayErrorMessage → ShowModal → startModal), which needs the asyncify
ModuleAnalyzer rather than the direct-call heuristic — or simply hoist-all-cpp-catches and let
`-O2` clean up.

**Next:** Phase 2 — flip a small wx standalone app to native EH (the real `ShowModal`-from-catch
path). Phase 3 — full KiCad + the emsdk bump (OCC only). Before KiCad: generalize the pass
(result-typed tries + transitive detection) and file the design on binaryen #4470.

---

## 0. What changed since 01–05 (which were authored 2026-06-11/12)

Three corrections that move the decision:

1. **The encoding is resolved: use the LEGACY encoding (`-sWASM_LEGACY_EXCEPTIONS=1`).**
   Not "decide after the emsdk bump" (as README §TL;DR / 03 §experiment / 05 §new-EH frame
   it). Binaryen's Asyncify supports **only** legacy `try`/`catch` — never
   `try_table`/`throw_ref`/`exnref` — through the latest **v130** (Jun 2026), with no
   roadmap, PR, or TODO to change it. The exnref path the parked experiment was forced onto
   is therefore a **dead end**: even with the OCC bug fixed, an exnref module dies at the
   `--asyncify` step. Legacy has shipped *unflagged* in all three engines since 2021–22
   (Chrome 95 / Safari 15.2 / Firefox 100), is still emscripten's own default, and — crucially
   — the size prize comes from **native-vs-JS EH (dropping `invoke_*`), independent of the
   encoding** — so legacy costs us nothing. **The "exnref → TryTable variant" fork is closed:
   legacy + the catch-arm-hoisting pre-pass (05) is the single viable path.**

2. **Binaryen is no longer a blocker — production already ships v130.** README/03 say "our
   emsdk bundles v121, we don't even have the partial support locally." That v121 is only the
   *finalize / test-apps-in-link* copy. **CI and publish pin `BINARYEN_VERSION=130`**
   (`.github/workflows/ci-ubicloud.yml:44`, `.github/workflows/publish-wasm.yml:33`;
   resolved by `scripts/common/get-wasm-opt.sh:50`) for the host-side `--asyncify` + `-O2`.
   v130 is the latest release and carries the v125 partial legacy-EH asyncify support. So the
   migration plan's "step 1: get a newer wasm-opt for the post-link step" is **already done**.

3. **`--pass-arg=asyncify-ignore-unwind-from-catch` is implemented now** (shipped in
   Binaryen v125; `03:23` called it "dead docs"). But it is a **tripwire-silencer, not a
   fix**: it *silently drops* the suspension inside a catch arm, which is semantically wrong
   on paths we actually reach (file-load error dialogs, the eeschema **Paste** handler).
   Correctness still requires the catch-arm-hoisting pass (or a hand-refactor).

**Net:** the long pole shrinks to a single thing — the **emsdk / LLVM compiler bump** (for
*parseable legacy wasm-EH* + the OCC `br_table` miscompile), **not** the Binaryen version.

---

## 1. The decision in one line

> `-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=1`, applied **uniformly**
> across deps + wxWidgets + KiCad + test apps; host-side `--asyncify` on Binaryen ≥ v125
> (we have v130); the ~85 suspend-in-catch sites fixed by the **catch-arm-hoisting pre-pass**
> (05), not by source refactors. Prize unchanged from 02: **64.5 → ~36 MB gz download,
> 187 → ~122 MB module**, plus a large `-O2` wall-time drop on the build critical path.

---

## 2. What works / what doesn't (verdicts from this spike)

| | Verdict | Why |
|---|---|---|
| Native wasm-EH, **legacy** encoding, + Asyncify v130, + catch-arm fix | ✅ **viable path** | Mechanically sound per all five agents; legacy is the only encoding Asyncify can instrument. |
| **exnref** (`=0`) + Asyncify | ❌ architecturally broken | Asyncify has zero `TryTable`/`exnref` support in *every* released Binaryen incl. v130; no roadmap. The experiment's `=0` never reached asyncify (blocked earlier at finalize). |
| `--asyncify-ignore-unwind-from-catch` as a *fix* | ❌ not a fix | Exists (v125+), but silently drops the suspend → our catch→modal dialogs misbehave. A tripwire-silencer only. |
| Bundled Binaryen **v121** + Asyncify + any wasm-EH | ❌ crashes | `Asyncify.cpp:998 UNREACHABLE`. Need ≥ v125 — already satisfied for the host-side pass (v130). |
| JSPI instead of Asyncify | ❌ closed for us | Fibers don't exist under JSPI (emscripten #18180); KiCad tools are fibers. (`03`) |

---

## 3. The three asyncify mechanisms vs. the EH switch

All three suspension mechanisms + handle-sleep are **orthogonal** to the C++ EH model — none
touches `__cxa_*`/landing pads in its own implementation. (Full detail: this session's
mechanism map + `docs/features/async/`.)

| Mechanism | EH-model dependence | Verdict |
|---|---|---|
| **Fibers** (`emscripten_fiber_swap`, libcontext/`coroutine.h`) | none — no try/catch in the coroutine layer | just works |
| **Main-loop park** (`emscripten_set_main_loop(...,1)` → `throw "unwind"`) | none — it's a **JS string** throw, not `__cxa_throw`; swallowed in JS glue | just works |
| **EM_ASYNC_JS sleeps + handle-sleep engine** (modal/nested-loop/clipboard/fonts/DOM-popup/PCBJam fetch) | implementation: none. **Callers** are the risk. | at-risk *only* via the catch-arm caller pattern |

The §3c trampoline-heal (`inject-dyncall-shims.sh`) and `handlesleep.js` "unwind" catches are
**JS-level and EH-independent** — unaffected by the switch.

**The entire exposure** is the **~85 sites where a C++ `catch` arm opens a modal →
asyncify-suspends** (e.g. `pcbnew/files.cpp:674/684/692`; 6 of them on a coroutine/fiber
stack including the eeschema Paste handler). This trap has **never fired at runtime** — it is
a static conclusion (brace-match audit `04` + Binaryen `AsyncifyFlow` skipping catch bodies
`05`). **Making it concrete and proving the fix is the spike's core job.**

---

## 4. Blockers, ordered (corrected)

| # | Blocker | Status / fix | Confidence |
|---|---|---|---|
| 1 | **emsdk 4.0.2 → 6.0.0 (LLVM 23) compiler bump** | em 4.0.2 emitted legacy wasm-EH that failed Binaryen parse at finalize (`popping from empty stack`) with **both** v121 and official v130 → an **LLVM-output bug, not a Binaryen-version bug**; a newer LLVM should fix it. Also lifts the bundled finalize-binaryen to ~v130. **Changes the JS-EH build too → must revalidate the whole project.** | parse-failure cause **unconfirmed** (Phase 0 resolves) |
| 2 | **OpenCASCADE** | (a) invalid `br_table` arity in `ShapeUpgrade_SplitSurface::Build` under wasm-EH — candidate upstream fix is **LLVM PR #123915** (Jan 2025, "add unreachable before catch destinations"), so the LLVM-23 bump likely covers it. (b) Separate `OCC_CONVERT_SIGNALS` setjmp↔exception-in-one-function conflict — **likely already sidestepped** by `-sSUPPORT_LONGJMP=wasm`; verify, else drop the flag. | both **to verify** in Phase 3 |
| 3 | **~85 suspend-in-catch sites** | catch-arm-hoisting Binaryen pre-pass — ~400–800 LoC, 1–2 wk, upstreamable, **keeps KiCad pristine** and obsoletes the hand-refactor + CI gate (05, 04). Alternative: hand-refactor (2–3 wk + permanent `catch_audit.py` CI gate + fights our upstream-closeness policy). | design **sound** (05); not yet built |
| 4 | **Safari 26.0 regression** (watch) | `-fwasm-exceptions` *legacy* apps transiently crashed at startup on Safari 26.0's initial release (in-place-interpreter bug, emscripten #25365), since patched. Track Safari point releases. | external, **patched** |

Browser support is otherwise a non-issue: legacy EH ≈ Chrome 95+/Safari 15.2+/Firefox 100+,
>96% of traffic, a 4-year tail. The "Chrome problem" (V8 slow to ship *exnref*) doesn't touch
us because we never emit exnref.

---

## 5. The phased red-green plan

**Key enabler (agent 5):** the Asyncify×wasm-EH interaction is **toy-testable now, without the
emsdk bump** — the bump's blockers are OCC-specific, and the toy has no OCC. So we separate the
two risks: *compiler bump* (full-KiCad only) vs *asyncify×wasm-EH semantics* (provable on a toy
today). We reuse the existing ablation harness pattern in
`tests/apps/standalone/{asyncify-races,coroutine}/` + `tests/asyncify/`.

### Phase 0 — micro-probe (hours, current toolchain)
**Question:** can em 4.0.2's LLVM emit *parseable* legacy wasm-EH on a tiny no-OCC program?
- Minimal standalone C++ (no wx, no OCC): a `try { throw } catch(...) {}` + a trivial sleep.
- Build `-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=1`, **`-sASYNCIFY=0`
  at link**; does it link + `wasm-emscripten-finalize` (bundled v121) cleanly? (i.e. reproduce
  the experiment's `=1` failure, or not).
- Then standalone `wasm-opt --asyncify` via `get-wasm-opt.sh` with `BINARYEN_VERSION=130` —
  does v130's asyncify parse + instrument the legacy-EH module?
- **Gate:** both clean → Phase 1 proceeds on current toolchain, bump deferred to Phase 3.
  Finalize fails the same way → em-4.0.2 legacy codegen is generally broken → **front-load the
  bump** (Phase 0′) before the toy.

### Phase 1 — red-green toy (days)
**Goal:** prove all three mechanisms survive native wasm-EH, and that **suspend-in-catch is the
only failure** (red) — then green once the fix lands.
- Add `tests/apps/standalone/eh-spike/eh_spike_test.cpp`, console protocol
  `[EH_SPIKE] PASS/FAIL/SUMMARY` (model on `races_test.cpp`). Three cases:
  (a) **throw across a sleep**; (b) **suspend inside a catch** (the #4470 case);
  (c) **fiber-swap then throw** (include `../coroutine/kicad_coroutine_harness.h`).
- One source, two builds via a make var `EH_FLAGS`: `-fexceptions` (green baseline) vs
  `-fwasm-exceptions … =1`. No-wx/no-OCC (template on the `coroutine-pthread` no-wx
  `LDFLAGS`), so we skip the wx-EH rebuild and the OCC bug.
- **Decouple asyncify**: link `-sASYNCIFY=0`, then standalone `wasm-opt --asyncify`
  (`get-wasm-opt.sh`, v130) so the in-link v121 doesn't poison case (b). Re-inject shims.
- Playwright spec `tests/asyncify/eh-spike.spec.ts` (model on `asyncify-races.spec.ts`,
  reuse `findSummary`/`crashLines`): JS-EH build **all green**; wasm-EH build **green on (a)/(c),
  red on (b)**. Build commands: `scripts/build-wasm-test.sh eh-spike-{js,wasm}`; run
  `cd tests && npx playwright test --config=playwright-asyncify.config.ts --project=firefox …`
  (widen `testMatch` or name it `asyncify-races-eh.spec.ts`).
- **Bonus data:** characterize what (b) does under wasm-EH — hard trap vs silent drop (the
  `ignore-unwind-from-catch` behavior). Informs decision-point #1.

### Phase 1.5 — the catch-arm-hoisting pass (1–2 wk; only if Phase 1 confirms (b) is the sole gap)
- Implement `src/passes/HoistCppCatches.cpp` + 2 registration lines in a Binaryen fork branch
  (05 §transform). Wire via `get-wasm-opt.sh` `BINARYEN_BUILD_FROM_SOURCE=1` pointed at the
  fork branch (`get-wasm-opt.sh:59-90`; one-line URL/branch change).
- Prove case (b) goes **green** on the toy. File the design on **binaryen #4470** first
  (upstreamability).

### Phase 2 — wx standalone (days)
- Flip a small wx test app to native EH (wx native-EH rebuild via the `KICAD_WASM_EH` gate;
  flag set patch-ready in the experiment appendix — but with `=1`, **not** the experiment's
  `=0`). Exercises the **real** `ShowModal`-from-catch suspend in miniature; confirms the pass
  handles the actual wx modal, not just a toy sleep.

### Phase 3 — full KiCad (weeks)
- emsdk bump → 6.0.0 (LLVM 23): **revalidate the JS-EH build first** (whole-project risk).
- OCC: verify `br_table` fixed; verify (and if needed remove) `OCC_CONVERT_SIGNALS`.
- Apply the experiment appendix patch (`KICAD_WASM_EH=1`) **corrected to `=1`**; uniform flag
  flip across deps/wx/kicad/tests; **drop `env.invoke_*` from `ASYNCIFY_IMPORTS`**
  (`apply-asyncify.sh` — README cites `:33`, current read ~`:88`; verify).
- Build host-side asyncify with the hoisting pass (fork). e2e audit under
  `-sASYNCIFY_ASSERTIONS` to flush any missed suspend-in-catch. Measure the gz/module win + the
  `-O2` wall-time drop.

---

## 6. Open decision points (to refine together)

1. **Catch-arm fix strategy:** hoisting pass (rec — pristine KiCad, upstreamable) vs
   hand-refactor 85 sites (CI gate, fights upstream-closeness) vs ship-with-`ignore-flag` and
   accept degraded error dialogs (fast, semantically wrong). *Phase 1 data informs this.*
2. **emsdk target:** 6.0.0 (latest, LLVM 23) vs a more conservative 5.0.x — 6.0.0 carries
   other breaking changes (startup `async/await`, compiler-rt naming) needing JS-EH-side
   revalidation.
3. **Binaryen fork hosting:** real git submodule (sibling to kicad/wxwidgets, pinned) vs a
   lighter `BINARYEN_BUILD_FROM_SOURCE` branch URL. (Fork is a *build tool*, not conveyed code
   — no GPLv3 `BUILD_SHA` treatment needed.)
4. **Run Phase 1 on current em 4.0.2** (if Phase 0 passes) **vs bump first regardless.**
5. **Sequencing vs the Asyncify-arbiter work** (`docs/features/async/`): README §relationship
   says arbiter-first (it fixes shipping bugs, needed under either EH model). Still the priority?

---

## 7. Risks / unknowns (honest)

- The `popping from empty stack` cause is **unconfirmed** — Phase 0 is the cheap decider.
- Whether case (b) under wasm-EH **traps vs silently drops** — Phase 1 characterizes.
- Whether **LLVM 23 actually fixes** our OCC `br_table` (PR #123915 is a strong candidate, not
  confirmed against our exact OCC code) — Phase 3 verifies.
- Safari 26.0 legacy-EH regression — patched upstream, but a reminder to track point releases.
- File:line refs in 03/05/README and the experiment appendix may have **drifted** — re-verify
  before editing (e.g. `apply-asyncify.sh` `ASYNCIFY_IMPORTS` line, build-script EH-flag lines).

---

## 8. Provenance

5-agent spike, 2026-06-22 — browser support, toolchain, runtime mechanisms, the pass design,
red-green harness. Key external refs: WebAssembly/binaryen **#4470** (open) / **#5475** (merged
v125), **LLVM PR #123915**, **emscripten #25365** (Safari 26.0), webassembly.org **Wasm 3.0**
(Sep 2025). Internal: this dossier 01–05, `docs/wasm-exceptions-experiment.md`,
`docs/features/async/`, `docs/features/perf/README.md` (lever #9), and memory
`wasm-eh-migration-assessment`.
