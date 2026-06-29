# 07 — Native wasm-EH spike: results and engineering opinion (2026-06-22)

> What the spike actually built and proved, then a candid opinion on whether to pursue the
> `-fwasm-exceptions` migration and how. Companion to [`06-spike-plan.md`](06-spike-plan.md)
> (the plan + Phase 0/1/1.5 result log). Verdict up front, evidence and caveats after.

## Verdict

**The migration is viable on the current toolchain, the single blocking limitation is real
and narrow, and a bounded Binaryen pass fixes it. I recommend pursuing it** — but the
schedule risk is the **emsdk bump for OpenCASCADE**, not the exception machinery, and the
pass still needs generalization before full KiCad. This was a genuine de-risking: the core
"can Asyncify and native wasm-EH coexist?" question is now answered **yes, empirically**,
in **all three engines — Chrome (V8), Firefox (SpiderMonkey), and Safari (WebKit)** — with no
emsdk bump required to prove it. (Project policy, set this session: every spec is run in all
three browsers — `cd tests && npm run test:asyncify:all`.)

## What was proven (with evidence, not estimates)

Everything below ran on the **current pinned toolchain** (emscripten 4.0.2, Binaryen v130 via
`BINARYEN_VERSION=130`) — **no emsdk bump**.

1. **em 4.0.2 emits runnable legacy wasm-EH.** A trivial `-fwasm-exceptions
   -sWASM_LEGACY_EXCEPTIONS=1` throw/catch builds (finalize on the bundled v121, *no* "popping
   from empty stack") and runs correctly under node. So the parked experiment's finalize
   failure was scale/OCC-specific, not a general codegen break.
   (`tests/apps/standalone/eh-spike/eh_probe.cpp`.)
2. **Binaryen v130 asyncifies legacy-wasm-EH; v121 cannot.** v130 instruments the module
   (asyncify_* exports appear); the emsdk-bundled v121 crashes (`UNREACHABLE …
   Asyncify.cpp:1146`). We already ship v130 for the post-link asyncify, so this costs nothing.
3. **The three Asyncify mechanisms split exactly as predicted.** A red-green toy
   (`eh_spike_test.cpp`) exercises sleep-across-throw, fiber-swap-then-throw, and
   suspend-inside-catch. Under native wasm-EH: **sleep ✅, fiber ✅, suspend-in-catch ❌
   (hard trap: `indirect call to null`).** Identical in V8 (node) and Firefox. So the *only*
   failure mode is the documented one (Binaryen #4470: AsyncifyFlow skips catch bodies), and
   it fails **loudly and deterministically** — not a silent corruption.
4. **A ~150-line Binaryen pass closes it.** `--hoist-cpp-catches`
   (`binaryen/src/passes/HoistCppCatches.cpp`, our fork) flips suspend-in-catch to **green**
   in both engines. `tests/asyncify/eh-spike.spec.ts` is a 3-variant ablation harness pinning
   JS-EH-green / wasm-EH-red / wasm-EH+hoist-green. Rebuilt and re-verified end-to-end from the
   tracked submodule.

## The artifacts (all reproducible)

| Thing | Where |
|---|---|
| Red-green toy (3 mechanisms) | `tests/apps/standalone/eh-spike/eh_spike_test.cpp` |
| Phase-0 probe | `tests/apps/standalone/eh-spike/eh_probe.cpp` (+ `eh-probe` Makefile target) |
| 3-variant build (stub→post-link-v130, +hoist) | `scripts/build-eh-spike.sh` |
| The Binaryen pass | `binaryen/` submodule (fork, branch `wasm-port` = `version_130` + the pass) |
| Fork build wrapper | `scripts/binaryen-hoist-pass/build-wasm-opt.sh` |
| Red-green-fixed spec | `tests/asyncify/eh-spike.spec.ts` |

## How the pass works (one paragraph)

For a `try` whose cpp-tag `catch` arm contains a suspending call, it rewrites the arm to just
*capture the exception payload into a local and set a flag*, and **hoists the real handler to
plain straight-line code after the try**, guarded by the flag. In the hoisted handler the
payload `pop` becomes a `local.get`, and the personality no-match `rethrow` becomes an explicit
`throw` of the cpp tag with the captured payload. Stock `--asyncify` then instruments the
hoisted handler like any other code — the upstream "no pause/resume inside catchBodies"
invariant becomes true *by construction*. It is a **pre-pass**; `Asyncify.cpp` is unchanged,
which is why it's a clean ~150-line addition and genuinely upstreamable.

## Generalization (follow-up, same session)

The pass was generalized from the MVP (one direct-suspend catch) to **hoist-all-cpp-catches** and
tested against a richer toy covering the real KiCad/wx shapes. **All 7 shapes are green in all
three engines** (Firefox + Chrome + Safari/WebKit):

| shape | covered |
|---|---|
| direct suspend in catch | ✅ |
| transitive (catch → helper → … → suspend) | ✅ (hoist-all; direct detection would miss it) |
| value-returning try/catch | ✅ (LLVM keeps the value in a local → void try, no result routing needed) |
| suspend-in-catch on a fiber/coroutine stack (eeschema Paste) | ✅ |
| nested suspend-in-catch tries | ✅ |
| **catch nested in a catch_all cleanup** (try body has a local with a destructor) | ✅ (escape past the outermost try; see below) |

**The one gap — catch_all-wrapped catches.** When the try body holds a local with a non-trivial
destructor, LLVM lowers the C++ catch *nested inside* the cleanup `catch_all`
(`catch_all { ~g; try { rethrow } catch $cpp { sleep } }`). Hoisting the cpp catch leaves the sleep
inside the `catch_all`; legacy `catch_all` gives no payload to capture/re-raise, so the sleep can
only be freed by hoisting **past the outermost enclosing try** — have the cpp catch capture the
payload and `br` to a `$done` block placed after that try. **Prototyped this session and
reverted:** the escape transform *validates* and fixes the catch_all case *in isolation*, but the
`block` + `br` + flag-dispatch control-flow shape it produces is **not asyncify-rewindable**
(rewind traps with `null function`) and it regressed the simple cases too. So the real work is
finding an asyncify-friendly escape shape — the per-try inline `br_if`-skip form (handler inline
right after the try) rewinds fine; a `br` out to a separate dispatch does not. That's the
fix is now landed (see "catch_all-escape: LANDED" below); all 7 shapes are green. How often it
bites KiCad depends on whether the specific catch's try body constructs a destructible
local/temporary (e.g. a `wxString`); a `try { ptr = Load(fn); } catch(IO_ERROR&)` with a pointer
result has no cleanup pad and is already covered. (`HOIST_ONLY_SUSPEND` switches off hoist-all back
to direct-suspend-only, useful for narrowing blast radius while debugging.)

### catch_all-escape: LANDED (2026-06-22)

**Fixed — all 7 shapes green in Firefox + Chrome + Safari.** When the try body holds a local with a
non-trivial destructor, LLVM lowers the C++ catch *nested inside* the cleanup `catch_all`; the pass
hoists it PAST the outermost enclosing try. Confirmed to occur in real KiCad (`pcbnew/files.cpp:670`
builds `std::map<std::string, UTF8> props` in the try, so its IO/format/bad_alloc catches are
catch_all-nested). Landing it took a from-source Binaryen build + a minimal *multi-function* repro
(`/tmp/eh_min2.cpp`, `/tmp/eh_min3.cpp`); two bugs, both invisible on a single function and only live
once several shapes inline together:

1. **Over-eager deferral → `null function`.** A nested cpp catch is deferred to its ancestor escape
   target, but the test matched ANY ancestor catch body — so a cpp catch in a *regular* catch body
   (the `__cxa_end_catch` cleanup tries LLVM emits everywhere) was deferred to a target that never
   hoisted it; its suspend was dropped and rewind trapped. Fix: defer only when the catch sits in an
   ancestor's `catch_all` cleanup pad (`hasCatchAll() && catchBodies.back() == child`).
2. **Trailing catch_all code → `unreachable`.** The rewritten minimal arm completes, but the
   catch_all body has trailing `(unreachable)` after the nested try (it assumed the handler
   diverged). Fix: wrap the escape target in a `block $esc`; the arm `br $esc`s after capturing,
   landing fall-through just before the dispatch (so Asyncify still rewinds the handler).

The pass is in the `binaryen` submodule (`src/passes/HoistCppCatches.cpp`). The earlier
"not asyncify-rewindable" worry was wrong — Asyncify rewinds the escape form fine; the blockers were
ordinary IR bugs, exactly as the multi-function-debug plan predicted.

### value-typed (concrete-result) tries: LANDED (2026-06-22)

The pass also handles an escape target whose try yields a *value* (i32/i64/…), not just
void/unreachable — it routes the body/handler value through a `$result` local (the no-exception
body value is captured inside `block $esc`; a caught arm br's out and each per-arm dispatch writes
`$result`; the block yields `local.get $result`). Non-defaultable result types are still skipped.
These tries don't arise from normal C++ EH lowering (LLVM keeps catch values in locals →
void/unreachable tries), so they're covered by hand-written modules in
`scripts/binaryen-hoist-pass/tests/` (`run.sh`): `--fuzz-exec` confirms the pass preserves the
result value across the exception / no-exception / payload paths, and a real asyncify unwind+rewind
through a value-typed *suspending* catch yields the correct value (50).

#### Debugging history (superseded)

The notes below trace the path to the fix; their "blocked" conclusions are superseded by the
landing above.

##### catch_all-escape: confirmed real; the fix is asyncify-SOUND, not a wall (2026-06-22)

> **Correction (later same session):** the "not asyncify-rewindable" conclusion below was
> **disproven**. Diffing the *asyncified* output of the per-try vs escape forms on a minimal
> single-function suspend-in-catch (`/tmp/eh_min.cpp`) shows them **structurally identical** — all
> 22 diff hunks are pure local-index renumbering — and **both run cleanly in node**. So Asyncify
> rewinds the escape form fine. The real blocker is ordinary structural bugs in the escape pass on
> the complex *inlined* toy (one found: the skip-to-escape-target coordination drops a catch when
> its escape target is value-typed; fixing that surfaced a load-time trap, so there's ≥1 more).
> That is tractable engineering — methodical per-function isolation like the eh_min repro — **not**
> an asyncify-internals wall. WIP + partial fix preserved in the escape-wip file below.

> **Further localization (same session):** the breakage is **not** fiber-specific (cases 1/3/4/5
> with no fibers still trap) and **not** one bug. It is a **layout-sensitive structural corruption**
> the escape restructure introduces on MULTI-function modules — `null function` / wrong
> `call_indirect`, which V8 then mis-compiles unpredictably (the trap point *moves* with module
> composition). Single-function repros (`eh_min`) work; the corruption only appears once several
> functions/cases compile together. So the next step is NOT more single-function isolation but a
> small **multi-function** repro under a Binaryen **debug build (assertions)** + `--fuzz-exec`, to
> catch the exact expression the restructure corrupts. Deferred to dedicated debugging.

**Confirmed we DO hit the gap.** A spot-check of the audited sites found destructible locals in the
try bodies: e.g. `pcbnew/files.cpp:670` declares `std::map<std::string, UTF8> props;` in the try,
so its three `catch (… ) { DisplayErrorMessage(…) }` arms are lowered nested inside a cleanup
`catch_all`. The file-load sites generally construct `wxString`/`std::map`/smart-pointer locals, so
this is not academic — a real subset of the ~85 sites is affected.

**The fix was attempted extensively and is blocked.** The escape-target restructure (hoist the cpp
catch — own or nested — past the outermost enclosing try, dispatching handlers after it) **validates**
in every variant but is **not asyncify-rewindable**: it traps with `null function` even on the simple
cases the per-try form handles. Tried: inline flag-dispatch (`if (flag==n)` — asyncify skips `if`
bodies on rewind), a bare single handler, `br_if`-skip guards, and `ReFinalize` (for stale `Try`
types). None worked at the time — the actual root causes (over-eager deferral + trailing catch_all code)
were found later with a multi-function repro; the fix landed in the submodule pass (see above).

**Open options:** (1) diff the *asyncified* IR of the working per-try form vs the escape form on one
simple case, to pinpoint exactly what Asyncify mis-handles; (2) hand-refactor the affected KiCad
sites (move the destructible local out of the try body) — a targeted subset, not all 85; (3) the new
`exnref` EH encoding gives `catch_all` a payload (a clean fix) but Asyncify has no `exnref` support.
The per-try pass (6/7 shapes) is the shipped state.

## Opinions (the part you asked for)

**1. Do it — the size/perf prize is real and the risk is now bounded.** −44% download / −35%
module (measured, see 02) plus a large `-O2` build-time drop. The thing everyone feared
(Asyncify ⊥ wasm-EH) is disproven. I would not have said this before the spike; I say it now
because the toy actually runs.

**2. Switch the pass from "selective" to "hoist-all-cpp-catches" before KiCad.** My MVP only
hoists arms with a *direct* suspending-import call. KiCad's real pattern is **transitive** —
`catch (IO_ERROR&) { DisplayErrorMessage(...); }` → `ShowModal` → `startModal` — so direct
detection would miss most of the 85 audited sites. The design doc already recommends hoist-all
+ let `-O2` prune the no-op hoists, and having now written the selective version I agree: it
removes the call-graph analysis entirely, is robust to transitivity, and the only cost is
transforming more tries (which `-O2` coalesces). Selective was the right call for *proving the
concept with minimal blast radius*; hoist-all is the right call for *shipping*.

**3. The remaining pass work is small (~1–2 days), and I know exactly what it is.** (a)
Concrete-result-typed tries — route the body value through a temp local (the toy already
forced me to handle `unreachable`-typed; `i32`/others are the same shape). (b) `catch_all`
coexistence on the same try (cpp catch + cleanup pad). (c) hoist-all gating. None are research;
all are mechanical Binaryen-IR work. The two real-IR gotchas are already solved in the MVP:
**nested-catch pops** (don't clobber a nested catch's payload — fixed with a Try-ancestor guard)
and **nested suspend-in-catch tries** (KiCad will have these; the toy already did, and hoisting
*both* was required).

**4. The schedule risk is the emsdk bump, not exceptions.** Everything above avoided the bump.
Full KiCad cannot: OpenCASCADE miscompiles a `br_table` under wasm-EH on em 4.0.2 (candidate
LLVM fix exists; em 6.0.0 = LLVM 23 should cover it), and the bump changes the compiler for the
*JS-EH build too* → whole-project revalidation. That is the multi-week, cross-cutting item.
Budget the migration as "1–2 days pass + N weeks emsdk-bump-and-revalidate," not the reverse.

**5. Keep the legacy encoding; ignore the exnref/Chrome noise.** Asyncify can only instrument
legacy `try/catch` (no roadmap to change through v130), and legacy ships unflagged everywhere
since 2021. The size win is native-vs-JS EH, independent of the encoding — so legacy is free
and correct. The exnref "Chrome problem" never touches us.

**6. Honest caveat — the toy is small; scale is unproven.** This spike de-risks the *semantic*
interaction, not KiCad-scale behavior. Two known scale hazards remain untested under wasm-EH:
V8's per-function locals limit on huge asyncified functions
([[chrome-asyncify-rewind-crash]]) and unwind-time landing-pad reliability
([[asyncify-eh-unwind-landing-pads-unreliable]] — which *might improve* under wasm-EH, worth
re-checking). The Safari 26.0 transient legacy-EH crash (emscripten #25365, since patched) is a
reminder that even legacy can break on a fresh engine. None of these are blockers; all are
"verify at scale," and the recommended order (toy → wx app → full KiCad) is designed to surface
them cheaply.

## Recommended path forward

1. **Generalize the pass** — DONE: hoist-all, catch_all-escape, and value-typed/concrete-result
   tries are all handled and verified (the value-typed path via `scripts/binaryen-hoist-pass/tests/`).
   No further generalization is needed for the 7 KiCad shapes.
2. **Phase 2 — a wx standalone app** flipped to native EH: the first *real* `ShowModal`-from-catch
   path, and the forcing function for transitive hoisting.
3. **File the design on binaryen #4470** (the pass is a pure addition; upstreaming collapses our
   fork back into stock wasm-opt eventually).
4. **Phase 3 — full KiCad**, gated on the emsdk bump (the real work) + the generalized pass +
   the uniform flag flip + dropping `env.invoke_*` from `apply-asyncify.sh`, with an e2e audit
   under `-sASYNCIFY_ASSERTIONS`.

## Status of the tracked changes (for review)

- `binaryen/` submodule added (fork `emergence-engineering/binaryen`), branch **`wasm-port`** at
  `version_130 + 1` (`58f25ebb2`) — the pass is **committed in the submodule but not pushed**.
  Pushing the branch to the fork (and committing the parent gitlink) is the user's call.
- Parent-repo changes are **uncommitted**, pending review: `.gitmodules` + the `binaryen`
  gitlink, the spike toy/scripts/spec, and these dossier docs.
