# 13 — Design B: engineering spec & work log

> The granular, file-by-file implementation spec for [`12`](12-design-b-asyncify-implementation-plan.md)
> (the plan/phases/test-matrix). This document is the **engineering design + per-phase checklists**,
> and is updated as a **work log** as the phases land. Build it on Asyncify — no JSPI.

## 0. State of the world (2026-06-23)

- The **de-park is live**: `wxwidgets/src/wasm/evtloop.cpp` `DoRun` at depth 0 calls `wxWasmParkMainLoop()` (an `EM_ASYNC_JS`/`Asyncify.handleAsync` suspend driving an rAF `await ccall('ProcessEvents',{async:true})` pump). The old `emscripten_set_main_loop(...,1)` throw is gone (it was fatal under native wasm-EH; see `wasm-exceptions/08`+`09`).
- **Measured regression** (`wasm-exceptions/09`): config 1 (no de-park) passes all; config 3 (JS-EH + de-park) fails the 6 `coroutine`/`coroutine-nested`/`coroutine-pthread` specs; config 2 (native-EH + de-park) fails those 6 + raytracer(5) + main-app(10). The **6 coroutine failures are the de-park's**, both EH models.
- **The gate** for this work = those 6 specs going green again, plus a minimal unit repro (Phase 0).
- **Stale code to clean up in Phase 1:** `scripts/common/shims/handlesleep.js` still has the `"unwind"`-sentinel swallow (lines ~57-68) referencing `set_main_loop(...,1)` — dead under the de-park; the rewrite subsumes it.

## 1. The scheduler — JS design (the heart of the fix)

### 1.1 What exists today (`handlesleep.js`)
Per-sleep `currData` capture/restore for **one** level of nesting, **`handleSleep` only**:
- Wraps `Asyncify.allocateData` to record which buffer pointer the active `handleSleep` allocated (`ctx.capturedData`).
- In the `wakeUp` callback, restores `Asyncify.currData = ctx.capturedData` before `handleSleep` does `_asyncify_start_rewind`+`doRewind`, so a fiber swap that clobbered the slot during the `await` doesn't make the sleep rewind the wrong buffer.
- **Blind to `handleAsync`** (the de-park park + the per-tick ccall) **and to fibers** (libcontext buffers don't come from `allocateData`).

### 1.2 The scheduler object (`AsyncifyScheduler`, replaces the shim)
A single JS authority that is the **only** writer of `Asyncify.currData` during managed transitions. State:
```
contexts:   Map<id, ctx>          // every parked/running suspendable thing
readyQueue: id[]                  // FIFO of contexts whose wake fired
running:    id | null             // the one context currently executing
transitionRunning: bool           // an unwind or rewind is in flight
trampolineRunning: bool           // a fiber-swap trampoline is mid-flight
```
`ctx = { id, kind: 'main'|'modal'|'nested'|'coroutine'|'sleep', buffer /*dataPtr*/, status: 'running'|'parked'|'ready', wakeReason, result, cancel }`.

**Governing rule (from doc 05):** `Asyncify.currData` is *not* durable state — it is a register loaded from the current context only at the instant of a managed transition. The `contexts` records are the truth. **Many parked; at most one unwinding-or-rewinding.**

### 1.3 The four hooks (intercept every `currData` writer)
1. **`Asyncify.handleSleep`** — register a `sleep` ctx (today's capture), but route its wakeup through `drain()` (below), not an inline `doRewind`.
2. **`Asyncify.handleAsync`** — wrap it the same way. **New, load-bearing:** the de-park park and the per-tick `await ccall` are `handleAsync`; they must be tracked contexts, not invisible slot-writers.
3. **`_emscripten_fiber_swap`** — *track* (not allocate) the per-fiber buffers `oldFiber+20` / `newFiber+20` so a coroutine swap is a managed transition the scheduler knows about.
4. **`Fibers.trampoline`** — own it (and the `trampolineRunning` guard); keep `inject-dyncall-shims §3c` self-heal as backstop.

### 1.4 The transitions (the only code that writes `currData`)
```
park(ctx):    assert state==Normal;  currData=ctx.buffer;  start_unwind   // ctx now parked, slot free
resume(ctx):  assert state==Normal;  currData=ctx.buffer;  start_rewind; doRewind
drain():      if (transitionRunning || trampolineRunning || state!=Normal || !readyQueue.length) return;
              resume(contexts[readyQueue.shift()])
```
**Deferred wakeup:** a Promise/event resolution **marks a ctx ready and calls `scheduleDrain()`** — it never calls `doRewind` inline (because `doRewind` can re-enter wasm and unwind again before returning). `drain` runs only when the slot is provably free, and receives **explicit transition-completion signals** by wrapping `_asyncify_stop_rewind` / `Asyncify.maybeStopUnwind` (clear `transitionRunning`, then `scheduleDrain()`), not a JS `finally`.

### 1.5 Invariants (assert in dev builds; doc 05 §invariants)
(1) only the scheduler writes `currData` during managed transitions; (2) ≤1 context unwinding-or-rewinding; (3) Promise resolution never `doRewind`s directly while a transition runs; (4) `currData` may be null while contexts are parked — records are truth; (5) every `allocateData`/fiber buffer belongs to exactly one ctx; (6) the scheduler owns the trampoline; (7) a parked ctx's buffer is never reused until it resumes-and-completes; (8) FIFO readyQueue (no starvation).

> **Correction (verified in the glue during Phase 0, 2026-06-23):** `handleAsync` routes through the wrapped `handleSleep` — `handleAsync(fn) = handleSleep(wakeUp => fn().then(wakeUp))` (`coroutine_test.js:9989`) — so the shim **already covers** the de-park park (`wxWasmParkMainLoop`) and the per-tick `await ccall`. §1.3's "hook 2 (handleAsync) is load-bearing/new" is therefore **wrong**: no separate `handleAsync` hook is needed. The genuine *uncovered* `currData` writer is the **fiber swap** (libcontext buffers come from `emscripten_fiber_init`, not `allocateData`). **So Phase 1's scheduler should focus on fiber tracking (§1.3 hook 3) + the deferred drain (§1.4) + single-transition serialization — not handleAsync coverage.** The Phase-0 red gate is confirmed (6 `coroutine*` specs fail on the de-park build); this sharpens where the fix lives.

## 2. The C++ yield API (Phase 3 surface)
```cpp
using WAKE_TOKEN = int;
WAKE_TOKEN wasm_begin_async_wait(int kind);     // EM_JS → scheduler.beginWait(kind) → token
int        wasm_yield_until(WAKE_TOKEN token);  // EM_ASYNC_JS → park current ctx, return result on resume
void       wasm_resolve_wait(WAKE_TOKEN, int);  // EM_JS → mark ctx ready + scheduleDrain
```
Reimplement each wait on top of it: `wxDialog::ShowModal` (replaces `dialog.cpp:startModal` `EM_ASYNC_JS` + `_wxModalResolvers`), `wxGUIEventLoop` nested `DoRun` (replaces `evtloop.cpp:wxWasmRunNestedLoop`), `wxClipboard::GetData`, font enum. Each becomes "begin wait → yield_until → (JS event) resolve_wait". The existing LIFO resolver stacks fold into scheduler ready/wait bookkeeping.

## 3. The root fiber (Phase 2, B2)
Run `main → wxEntry → OnRun → DoRun` inside a managed **root fiber** (via libcontext's `emscripten_fiber_init_from_current_context`, already used for the coroutine main stack at `libcontext.cpp:202-217` — generalize it to the app root). At depth 0, `DoRun` **yields the root fiber to the scheduler** instead of `handleAsync`-parking. The browser tick (rAF or `set_main_loop` callback) **resumes the root fiber**, which calls `ProcessEvents` **directly (wasm-side), not via `await ccall(...,{async:true})`** — that JS-awaits-a-suspending-export boundary is the Emscripten #13302 corruption hazard. Now the main loop is a sibling fiber to coroutines/modals; a coroutine swap is fiber↔fiber under the scheduler — no nested unwind.

## 4. File-by-file change map
| File | Change | Phase |
|---|---|---|
| `scripts/common/shims/handlesleep.js` | → `asyncify-scheduler.js`: the scheduler (1.2–1.5); cover `handleAsync` + fiber tracking + deferred drain; drop the stale `"unwind"` swallow | 1 |
| `scripts/common/inject-dyncall-shims.sh` | inject the new scheduler; keep §3c self-heal | 1 |
| `wxwidgets/src/wasm/evtloop.cpp` | `DoRun` top-level → root-fiber yield; `ProcessEvents` driven wasm-side; `ScheduleExit` → scheduler wake | 2,3 |
| `wxwidgets/src/wasm/dialog.cpp` | `ShowModal`/`EndModal` → `wasm_yield_until`/`wasm_resolve_wait` | 3 |
| `kicad/thirdparty/libcontext/libcontext.cpp` | register fiber create/swap with the scheduler; expose the root-fiber init | 2 |
| wx clipboard/font wasm files | → yield API | 3 |
| `tests/apps/standalone/coroutine*`, `*raytrace*` | the integration gate (already exist) | 0 |
| `tests/apps/standalone/sched-nest/` (new) | the minimal Phase-0 unit repro | 0 |
| `tests/asyncify/*.spec.ts` | red-green specs for the harness, 3 engines, both EH | 0,1 |

## 5. Test harness
- **Phase 0 minimal repro:** a tiny `wxIMPLEMENT_APP` that, from a `CallAfter`/timer (i.e. inside the parked rAF pump), does a libcontext fiber swap and swaps back; assert no `invalid state: 1`, correct round-trip value. RED under the current de-park; the unit gate for Phase 1.
- **Integration gate:** the 6 `coroutine*` specs (already RED under de-park).
- **Full matrix (12 §test-matrix):** `ShowModal` from root & from coroutine; nested modal in quasi-modal; coroutine swap while a modal pumps; clipboard from root & coroutine; raytracer multi-core; exit/unload cleanup — in **Firefox+Chrome+Safari**, under **both `-fexceptions` and `-fwasm-exceptions`** (incl. a modal from inside a `catch`, to prove composition with the hoist pass), with a `-sASYNCIFY_ASSERTIONS=1` pass.

## 6. Phase checklist (work log — update as landed)

- [ ] **Phase 0 — red harness** (2–3 d). Minimal `sched-nest` repro RED in 3 engines; the 6 `coroutine*` specs confirmed RED under de-park; CI/local script to run them.
- [ ] **Phase 1 — scheduler core** (1–2 wk). `asyncify-scheduler.js` with the 4 hooks + deferred drain; covers `handleAsync` + fibers. **Gate:** `sched-nest` + the 6 `coroutine*` specs GREEN, all 3 engines, both EH. (If the permanent `handleAsync` park can't be a clean parked context, escalate to Phase 2.)
- [ ] **Phase 2 — root fiber** (≈1 wk). Main loop = scheduler root fiber; `ProcessEvents` wasm-side (no JS async ccall). **Gate:** Phase-1 gate still green + no `handleAsync` park remains.
- [ ] **Phase 3 — migrate waits** (1–2 wk). `wasm_yield_until` API; `ShowModal`/nested loop/clipboard/font on it. **Gate:** full matrix green.
- [ ] **Phase 4 — lifetime** (few d). Cleanup ordering vs the scheduler; teardown deferred to unload. **Gate:** exit/unload tests green; no cleanup during steady-state pumping.

## 6b. Phase-0 finding — Phase 1 is insufficient; Phase 2 (root fiber) is REQUIRED (2026-06-23)

**Exact failure** (coroutine_test, de-park build): the first case `yield_resume_preserves_state` **passes** (it runs during the startup burst, *before* the main-loop park), then a later fiber swap aborts with **`Aborted(Assertion failed: We cannot stop an async operation in flight)`**, surfacing as `[wxWasm] main loop pump error`.

**Why:** `wxWasmParkMainLoop` is `Asyncify.handleAsync(...)` — a **permanently in-flight async operation** for the app's whole life. A coroutine `emscripten_fiber_swap` inside the rAF pump calls `stop_unwind`, but the park's async op is in flight → abort. Under the old `throw`, the top loop was *not* an async op (`throw "unwind"` is a plain JS exception), so swaps from a clean base worked.

**Tested & ruled out:** changing the rAF pump's `await ccall('ProcessEvents',{async:true})` to a **synchronous** `ccall` does NOT help — the in-flight op is the *park*, not the per-tick ccall. And the park is **permanent** (never completes until exit), so no scheduler serialization can let a coroutine swap "wait for the slot." **So §6's Phase-1 escalation condition is met.**

**The fix (Phase 2, now confirmed required):** the main loop must not be a `handleAsync` park. Make the main stack a **libcontext fiber** (Ruby/Julia pattern): the main fiber runs `ProcessEvents` on its own stack and **yields to the browser by a fiber swap / return-through-`set_main_loop(...,0)`**, not a `handleAsync` suspend — so there is no permanent in-flight async operation, and a coroutine swap is a sibling fiber↔fiber switch from the same `g_main_context`. `ProcessEvents` must run on `g_main_context` (the main fiber), not the fresh rAF-ccall stack. The Phase-1 scheduler is still needed to coordinate modal/clipboard waits that *do* suspend — but **the main-loop park must move off `handleAsync` first.**

**Open Phase-2 design point:** how the main fiber yields to / resumes from the browser each frame (rAF resumes `g_main_context` to run one `ProcessEvents` tick, then the main fiber yields back) without re-introducing a permanent asyncify operation. Candidate: `set_main_loop(tick,0,0)` where `tick` resumes the main fiber via libcontext, the main fiber runs `ProcessEvents` then swaps back, and wx teardown is suppressed until unload (Phase 4 lifetime).

## 7. Open decisions (resolve during implementation)
- Is Phase 1 (scheduler treating the `handleAsync` park as a tracked parked context) sufficient, or is Phase 2 (root fiber) required? — answered by the Phase-0 harness against the Phase-1 build.
- One scheduler file injected post-link (like today's shim) vs an emscripten `--js-library` (link-time, cleaner, survives JS regen). Lean js-library for durability.
- Whether to keep an Asyncify-only "no scheduler" fast path for apps with no coroutines (most standalone tests) to avoid scheduler overhead — likely yes, gated on a runtime "any non-main context registered?" check.
- Native-EH interaction: confirm the scheduler's transitions compose with `HoistCppCatches` (suspend-inside-catch) — a matrix test, not a design change expected.
