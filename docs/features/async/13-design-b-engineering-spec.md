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

## 6c. IMPLEMENTED & verified (2026-06-23): the per-frame-yield while-loop

The fix is **simpler than "an explicit libcontext root fiber."** `DoRun` (top level) is now a plain C++ loop on the real main stack (`evtloop.cpp`):
```cpp
while (!m_shouldExit) { ProcessEvents(); wxWasmYieldToBrowser(); }
```
`wxWasmYieldToBrowser` is `EM_ASYNC_JS(void, …, { await new Promise(r => requestAnimationFrame(r)); })` — an Asyncify suspend that **completes every frame**. Because nothing is permanently suspended, the Asyncify slot is free (`state==Normal`) whenever `ProcessEvents` runs, so a tool-coroutine fiber swap inside it succeeds; and `ProcessEvents` runs on the real main C stack (= libcontext's `g_main_context`), so swaps are from the right context. `ScheduleExit` just sets `m_shouldExit` for the top level (nested/quasi-modal loops still use the `setTimeout` pump + `wxWasmExitNestedLoop`). `wxWasmParkMainLoop` is removed. No explicit fiber API or scheduler was needed for the *main-loop* fix — the key was only that the suspension **completes** each frame instead of being permanent.

**Result (JS-EH):** coroutine in-app suite **13/13 pass, 0 fail** (was: abort after case 1); `coroutine` + `coroutine-nested` e2e specs **green**; dialog renders + modals **green** (no regression). Only `coroutine-pthread` outstanding — but its `coroutine_test_wxpt.wasm` was **stale** (the `coroutine-pthread` make target didn't rebuild it); all apps are being rebuilt to confirm.

**Still likely needed later (Phase 1 scheduler / Phase 3):** modal/clipboard waits that genuinely suspend across the loop still use the nested `setTimeout` pump; if overlapping suspensions there prove fragile, layer the scheduler on. But the *coroutine regression itself is fixed by this main-loop change alone.*

## 6d. Phase-2 exposes a SECOND coupling: context-menu re-entrancy needs Phase 1 (2026-06-23)

The while-loop main loop (§6c) fixed the coroutines but **regressed the context menu** (2 e2e specs). Right-click → choose *Cut* → `Aborted(RuntimeError: unreachable)` / `memory access out of bounds`. Stack: a DOM mouse event (`mouseEventHandlerFunc` → the Asyncify export wrapper → `wasm-function[…]`) **re-enters wasm while `DoPopupMenu`'s `wxDomPopupMenuModal` context is suspended on the deep main stack** — a single-slot re-entrancy fault. The de-park's *permanent-context* loop masked it (its menu context was shallow — a fresh-ccall `ProcessEvents` — and always "in flight"); the while-loop's no-permanent-context, deep-stack suspend exposes it.

**Three targeted fixes, all empirically REJECTED (don't retry these):**
1. **C++ pump in `wxDomPopupMenuModal`** (mirror `startModal`) — *redundant*: `wx-dom.js`'s `wxShowContextMenu` **already** runs the same `setTimeout` ProcessEvents pump. No effect. (Reverted.)
2. **`ASYNCIFY_STACK_SIZE` 8192→65536** — not a buffer-size fault (still crashes at 65536; emscripten appends that hint to *every* `unreachable`). (Kept anyway — the while-loop genuinely deepens every suspension, so 65536 ≈ the coroutine apps + KiCad is the right call for all wx apps.)
3. **DOM backdrop blocking canvas pointer events** (`wx-dom.js`) — confirmed present in the rebuilt glue; still crashes. So the re-entry is **not** a canvas leak — the wx DOM port's document-level mouse handler re-enters wasm regardless. (Reverted.)

**Conclusion — the hard tension, stated plainly:**
- **de-park** (permanent-context loop): menus ✅, coroutines ❌
- **while-loop** (no permanent context): coroutines ✅, menus ❌

Neither is clean alone. Both faults are the SAME single-slot `currData`/state arbiter problem — Design B's **scheduler (Phase 1)** — now *proven necessary, not optional*. The §6c while-loop is the correct **foundation** (it removes the permanent park that blocked coroutine swaps); Phase 1 must layer on top so a wasm re-entry during ANY suspension (coroutine swap, menu/modal `handleAsync`, main-loop yield) is coordinated (deferred/queued or serialized) rather than misfiring a rewind. Kept in-tree: `evtloop.cpp` while-loop + the 65536 bump. Reverted: the redundant C++ pump and the backdrop.

## 6e. The precise mechanism (export-wrapper diagnostic, 2026-06-23)

Instrumented the Asyncify export wrapper to log every wasm entry while `state != Normal`. The menu crash is **not** a one-shot bad rewind — it's an **infinite busy unwind/rewind loop** on one buffer:
```
asyncify_start_unwind  state=1(Unwinding)  currData=1240280   ← main suspends
asyncify_start_rewind  state=2(Rewinding)  currData=1240280   ← …immediately resumed
__main_argc_argv       state=2             currData=1240280   ← main runs a few dynCall_ii deep
…repeats forever (currData unchanged) until the OOB crash
```
Buffer `1240280` (the parked main stack, suspended at the menu) is **suspended then immediately re-resumed, over and over**. Only one context exists, but it is being re-driven in a tight spin: its continuation re-suspends instantly (the menu promise is still pending), and something re-rewinds it each cycle.

**Two drivers fight over the single slot:** with the while-loop, the main-loop structure AND the **menu's own `setTimeout` ProcessEvents pump** (`wx-dom.js`) both try to drive the parked main stack — one re-rewinds what the other parked. Under the de-park there was a *single* pump chain (the rAF pump *was* the loop; the menu pump nested inside its `await`), so nothing double-drove the slot.

**Scheduler invariant this pins (the central requirement):** exactly ONE unwind/rewind transition in flight; a pump tick runs a **fresh** `ProcessEvents` (new stack) and must NEVER re-rewind an already-parked context — only that context's own `wakeUp` (its promise resolving) may resume it. The scheduler must enforce this across the main-loop yield, the menu/modal/nested pumps, and fiber swaps. (A plausible smaller first cut: a single shared "is a transition in flight / is a context parked" guard the pumps consult before re-driving — test it against the contextmenu specs before committing to the full registry.)

## 6f. RESOLVED (2026-06-23): the arbiter already existed — it just wasn't injected

A gated export-wrapper + `start_rewind` probe nailed the proximate cause: the menu's wakeUp fires `_asyncify_start_rewind(Asyncify.currData)` with **`currData == null`** → reads address 0 → OOB. The cause: the **`handlesleep.js` currData save/restore shim** — the existing Design-A / Emscripten #9153 arbiter (`scripts/common/shims/handlesleep.js`, which restores `currData` to the parked context's buffer before every rewind) — was **NOT injected into the contextmenu glue** (`pendingSleepContexts` count 0, vs 9 in the working coroutine app). Appending it manually → crash gone, `[CTXMENU_EVENT] Cut chosen` fires, spec 4/4 green.

**Why it was missing:** `inject-dyncall-shims.sh` gates the handleSleep shim on the libcontext fiber marker (`_emscripten_fiber_swap.isAsync = true;`), and `build-wasm-test.sh` only ran the injector under `WX_NATIVE_EH=1`. So plain (non-fiber) wx apps under JS-EH never received the currData arbiter. They didn't crash *before* the while-loop because the de-park's shallow menu context (a fresh-ccall `ProcessEvents`) never hit the null-rewind path; the while-loop's deeper main-stack suspend exposes it.

**Fix — build-system only, NO new runtime code:**
1. `build-wasm-test.sh` injects the shim into every freshly-linked app for **both** EH models (idempotent — the Makefile-injected coroutine apps are skipped).
2. `inject-dyncall-shims.sh` appends the handleSleep shim at EOF when there's no fiber glue (Asyncify is defined by then; it wraps `handleSleep` at load, before any runtime sleep).

**So §6c–6e's "build the single-owner currData arbiter" conclusion was right about the diagnosis but the arbiter already exists (`handlesleep.js`) — it only needed to reach these apps.** The scheduler invariant in §6e *is* what `handlesleep.js` implements (each parked context owns its buffer; `currData` is restored before its own rewind). The while-loop (coroutine fix) + this injection fix together resolve both regressions. §7's open question is therefore moot: no Phase-1 scheduler nor Phase-2 root fiber was needed — the while-loop main loop + the pre-existing currData shim suffice.

## 7. Open decisions (resolve during implementation)
- Is Phase 1 (scheduler treating the `handleAsync` park as a tracked parked context) sufficient, or is Phase 2 (root fiber) required? — answered by the Phase-0 harness against the Phase-1 build.
- One scheduler file injected post-link (like today's shim) vs an emscripten `--js-library` (link-time, cleaner, survives JS regen). Lean js-library for durability.
- Whether to keep an Asyncify-only "no scheduler" fast path for apps with no coroutines (most standalone tests) to avoid scheduler overhead — likely yes, gated on a runtime "any non-main context registered?" check.
- Native-EH interaction: confirm the scheduler's transitions compose with `HoistCppCatches` (suspend-inside-catch) — a matrix test, not a design change expected.
