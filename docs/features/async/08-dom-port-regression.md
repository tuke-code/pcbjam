# 08 — DOM-port asyncify regression (kicad e2e, 18 red)

> **RESOLVED — see [10-resolution-menubar-uaf.md](10-resolution-menubar-uaf.md).**
> Not an Asyncify bug: a freed `wxMenuBar` left in a live frame's `GetChildren()`
> by `wxMenuBarBase::Detach()` (`SetParent(NULL)` without `RemoveChild`, which
> only dangles in the DOM port because the bar is a real child there). The paint/
> idle walk then virtual-calls the dead object. Fix: unlink in
> `wxMenuBar::Detach()`. The notes below are the (correct) elimination trail that
> got there.

Status: **investigation in progress** (instrumented traces complete; symbolized
builds pending). Continues the dossier 01–07; same disease family, new
producers.

## Symptom

After rebasing `feature/wx-dom-port` onto main (which carries all of 07's
fixes — handlesleep buffer capture, trampoline self-heal, modal LIFO, pump
resolve-on-error, sync clipboard IsSupported), the kicad suite fails 18/54
with `RuntimeError: indirect call to null` (47×), `index out of bounds` (5×),
`signature mismatch` (1×). Main (canvas port) is green with the identical
wx/kicad commits and identical shim set — verified injected in our built apps
(handlesleep `__pendingSleepContexts`, trampoline `try{do{…}}finally`,
`_wxModalResolvers` all present in `tests/apps/kicad/*.js`).

First fault fires **before any test interaction** (1–5 s after "Creating
app"), during startup modal / first schematic-load coroutine cycles.

## Instrumentation used

`SHIM_DIAGNOSTICS=1 scripts/common/inject-dyncall-shims.sh <app.js>` —
extended this session with (all logging-only, kept in
`scripts/common/shims/diagnostics.js`):

- `DIAG_IMPORT` — wraps `emscripten_fiber_swap` + every `__asyncjs__*` import
  on `wasmImports` (state/currData/SP/exportCallStack on entry+exit; names
  the sleeping import, which DIAG_SLEEP could not).
- `DIAG_ASMCONST` — wraps `emscripten_asm_const_*`; logs ONLY when state ≠ 0
  at entry or changes across the call (detects "unwind escaped through a
  non-asyncify import").
- `DIAG_CCALL` — wraps `ccall`; watches `wx_dom_event`, `wx_dom_mouse`,
  `ProcessEvents`; flags entry at state ≠ 0 and state changes across sync
  calls.
- `DIAG_DOREWIND` — now also logs SP, the fiber's saved-SP slot, and
  `Asyncify.exportCallStack`.
- The old GL/dynCall tracer (sections 7–8) is now opt-in via
  `Module.__DIAG_GL_TRACE` (solved-bug tooling, floods boot logs).
- Temp `[DOM_DIAG]` logging in `wx-dom.js` `dispatch()`/`wxForwardMouse`
  (TEMPDBG, to be removed).

Reproduction: 30-second single-spec runs, fully deterministic:
`npx playwright test --config=playwright-kicad.config.ts --project=firefox
--grep "Backspace deletes the selection"` (after `npm run setup:kicad`;
note `npm run test:kicad -- --grep=…` does NOT forward args through the
nested npm scripts).

## Established facts (from traces)

1. **Bookkeeping is clean.** Unwinds/rewinds of the main-context buffer
   strictly alternate (25 U / 25 R, no double-unwind), SPs match the fiber's
   saved-SP slot at every rewind, `exportCallStack` is rebuilt correctly by
   rewind replays. The shims (trampoline heal, handlesleep) behave as
   designed. No EM_ASM state-escape ever fired (DIAG_ASMCONST silent).

2. **The discriminator is the activation that hosts the park.** Fiber-swap
   cycles work in the dozens when the parked activation is rooted at
   `__main_argc_argv` (pre-park boot), `dynCall_iii` (embind/collab entry),
   or `dynCall_vi` (coroutine entries / timers). The first cycle rooted at a
   *DOM-port-era* root faults immediately after its rewind completes:
   - **eeschema** (`Backspace` spec): first cycle rooted at `dynCall_v` — the
     parked main loop's rAF tick (`iterFunc = () => dynCall_v(func)`;
     `func = ProcessEvents`, see `wxGUIEventLoop::DoRun` →
     `emscripten_set_main_loop(ProcessEvents, 0, 1)`). Rewind completes
     (state 2→0 at the swap site), main continues, and ~40 ms later wasm
     traps on an indirect call through a null pointer inside wx event
     dispatch (`invoke_iii → dynCall_iii → … call_indirect null`). Every
     subsequent tick faults at the same site. 100% reproducible (3/3 runs).
   - **pcbnew** (`load-pcb` spec): unwind roots observed include
     `ProcessEvents` ×4 (modal pump ccall) and **`wx_dom_event`** ×2 (DOM
     listener dispatch ccall hosting a board-load coroutine swap). The crash
     follows the first **rewind of the `wx_dom_event`-rooted** parked
     activation.
   - **gerbview** (passing-but-faulted): startup modal parks the boot stack
     via `__asyncjs__startModal` (sleep buffer in `Asyncify.currData`), and
     the **first modal pump tick** (`ccall ProcessEvents {async:true}` with
     the sleep still parked) traps on a null indirect call inside
     `ProcessEvents`. Main's pump resolve-on-error catches it, cancels the
     modal (→ `library_manager.cpp(451) assert "table"`), app survives, spec
     passes. Same null-call disease, caught instead of fatal.

3. **The fatal eeschema chain is deep and exception-wrapped**: the fatal
   unwind's `exportCallStack` is `["dynCall_v","dynCall_iii"×5]` — five
   re-entrant `invoke_iii→dynCall_iii` hops (C++ try/catch glue) between the
   tick root and the fiber swap. The red-green harness's
   `post_park_fiber_swap` ALSO swaps from a `CallAfter` inside the parked
   tick and is GREEN — but its chain is shallow (no invoke hops, small
   frames). So the root alone is not sufficient; chain content matters.

4. The crash is always **post-rewind, in normally-executing code, through a
   pointer that should not be null** — i.e. memory/state corruption planted
   during the unwind/rewind of these specific chains, not a JS-level
   machinery failure.

## Symbolized fault (named build: `ASYNCIFY_KEEP_NAMES=1`)

Rebuilt eeschema/gerbview from the docker volume's pre-finalize wasm with the
name section kept, deployed over `tests/apps/kicad/*.wasm` (no JS rebuild), and
re-ran. The faults are now fully named.

**eeschema (`Backspace` spec) — crash stack (`.errors.log`):**

```
RuntimeError: indirect call to null
  at wxWindowBase::SendIdleEvents(wxIdleEvent&)
  at dynCall_iii  /  invoke_iii
  at wxAppBase::ProcessIdle()
  at ProcessEvents
  at dynCall_v
  at iterFunc                       ← the parked main loop's rAF tick
  at callUserCallback → runIter → MainLoop_runner → requestAnimationFrame
  …
  at wxGUIEventLoop::DoRun()         ← _emscripten_set_main_loop(ProcessEvents,0,1)
  at wxEventLoopBase::Run() → wxAppConsoleBase::MainLoop() → OnRun()
```

So the crash is a **null `call_indirect` inside `wxWindowBase::SendIdleEvents`**,
on a clean rAF tick (`dynCall_v → ProcessEvents → ProcessIdle`), i.e. in the
**parked main loop's continuation**. State is Normal at the crash; the
corruption was planted earlier and detonates on the next idle walk.

**The fatal cycle (named `[DIAG_SWAPSTACK]`, unwind side):** a normally
dispatched wx event drives a KiCad tool-coroutine fiber swap —

```
wxEvtHandler::ProcessEvent → EDA_BASE_FRAME::ProcessEvent
  → SCH_EDIT_FRAME::KiwayMailIn(KIWAY_EXPRESS&) → SCH_BASE_FRAME::SyncView()
  → TOOL_MANAGER::ResetTools(RESET_REASON) → CancelTool()
  → TOOL_MANAGER::processEvent → dispatchInternal → COROUTINE::Resume()
  → COROUTINE::jumpIn → jump_fcontext              [emscripten_fiber_swap, state 0→1]
```

and the matching yield back —

```
SCH_SELECTION_TOOL::Main → TOOL_INTERACTIVE::Wait(TOOL_EVENT_LIST const&)
  → COROUTINE::jumpOut → jump_fcontext             [fiber_swap]
… returning into: doRewind → finishContextSwitch → trampoline → maybeStopUnwind
  → dynCall_v → iterFunc (the rAF tick)
```

KiCad runs tools as libcontext coroutines; the wasm libcontext shim
(`libcontext::wasm_fcontext_entry`) maps each `jump_fcontext` to an
`emscripten_fiber_swap`. The "main caller" context (the fiber tools yield back
to) is buffer `9553280+20`; rewind roots progress `__main_argc_argv` (pre-park)
→ `dynCall_iii` (event-dispatch ticks) → `dynCall_v` (the bare rAF tick). The
crash detonates right after a `dynCall_v`-rooted (rAF-tick) context is rewound
following a tool round-trip.

**gerbview (faulted-but-passing) — same family, earlier trigger:** a startup
modal (`__asyncjs__startModal`, the "no libraries" dialog) parks its sleep;
the modal pump's **first** `ProcessEvents` (`{async:true}`, sleep buffer still
in `currData`) hits the identical null `call_indirect` inside event/idle/paint
processing. Main's pump-resolve-on-error catches it → `EndModal 5101` →
`library_manager.cpp(451) assert "table"` (a *consequence* of the forced
cancel, not the cause). App survives, spec passes — same disease, caught.

**Refined reading:** the disease is the documented "indirect call to null"
asyncify-rewind family (07), surfacing in wx event/idle processing that runs
while a suspension is in flight (parked main rAF tick, or a modal sleep). The
KiCad tool-coroutine swap machinery and the wx event loop are byte-identical to
the canvas port — so the DOM port either (a) moved a removelist'd
(uninstrumented) function onto one of these suspend chains, or (b) the
parked-main / object-lifetime interaction (dossier 02 §7 "de-park main") now
gets exercised by the DOM port's boot/event timing where canvas didn't.

## REFRAMING — it is NOT asyncify memory corruption

Two decisive experiments collapsed the hypothesis space:

1. **`ASYNCIFY_REMOVE` is resource-untestable here, but also exonerated by
   evidence.** A no-removelist build is unbuildable on this machine: with `-O2`
   it OOMs (instrument-everything is too big — exactly why the list exists);
   without `-O2` it won't load (`CompileError: too many locals`, V8's
   per-function limit — the very stall `-O2` cures). Independently, *no
   removelisted function appears on any captured fatal swap chain*, and the
   gerbview fault throws **synchronously inside `ProcessEvents` with no
   fiber-swap/sleep between ccall-enter and throw** — a removelist gap can only
   corrupt *during* an unwind/rewind, so it cannot explain a synchronous throw.

2. **The wasm table is fully intact at the crash.** A table-integrity monitor
   (diagnostics.js §12: baseline snapshot after boot + 60 ms scans for
   non-null→null slot flips and length changes) reports `baseline len=49105
   nonNull=49104` (only index 0, the conventional null) and **zero** flips and
   **zero** length changes through the crash. So the "indirect call to null" is
   **not** a nulled table slot and **not** an addFunction/asyncify table bug.

With the table intact, a null `call_indirect` is an index of **0** — and
`wxWindowBase::SendIdleEvents` (wincmn.cpp:2836) makes exactly these indirect
calls per window: `OnInternalIdle()` (virtual), `HandleWindowEvent(event)`
(virtual `ProcessEvent` + handler functors), then recurses `child->
SendIdleEvents` over `GetChildren()`. **The null is a virtual call on a bad
`wxWindow`** — a freed/dangling child still linked in the tree (its vtable slot
reads 0), or a half-constructed one — reached when the idle walk recurses into
it.

### Working root cause

A **use-after-free / dangling `wxWindow` in the window tree**, detonated by the
idle-event tree walk that the parked main loop runs (`ProcessEvents →
wxAppBase::ProcessIdle → SendIdleEvents`). The asyncify/coroutine activity is
the **trigger/timing**, not the corruption: a tool-coroutine round-trip (or
modal teardown) during `ProcessEvents` frees or invalidates a window, and the
`ProcessIdle` later in that same pump walks the now-dangling child. gerbview is
the same shape with a modal pump driving `ProcessEvents` over a tree perturbed
by the startup modal.

Ruled OUT, with evidence: de-park teardown (noExitRuntime=true), missing/мис-
injected shims (all present), EM_ASM re-entry (DIAG_ASMCONST silent), asyncify
buffer/SP corruption (clean alternation, matching SPs), wasm-table corruption
(monitor clean), `ASYNCIFY_REMOVE` gap (synchronous throw + no removed fn on
chains).

This relocates the bug from the **asyncify substrate** (where 01–07 lived) to
**`wxWindow` lifetime under the parked-loop's re-entrant idle/event
processing** — i.e. our wasm event-loop + DOM-window-lifecycle layer
(`src/wasm/evtloop.cpp` `ProcessEvents`/`ProcessIdle`, `src/wasm/window.cpp`
window destruction, the element registry), not Asyncify itself. The DOM port's
event/lifecycle timing opens the window the canvas port's did not.

### Next experiments (need a kicad relink — not yet run)

- Build eeschema with `-sASSERTIONS=1` (and/or `-sSAFE_HEAP=1`): turns the bare
  "indirect call to null" into a named bad-pointer / freed-window report and
  should fire AT the offending `wxWindow`, naming its class + free site.
- Audit `~wxWindowWasm` / `wxDomDestroyControl` ordering vs the wx child-list
  unlink (`wxWindowBase` dtor `RemoveChild`/`DestroyChildren`), and whether DOM
  teardown fires a synchronous `focusout`/`blur` that re-enters `wx_dom_event`
  mid-destruction.
- Check `ProcessEvents` (evtloop.cpp) for re-entrancy: a setTimeout-driven
  modal/nested pump `ProcessEvents` can run while a rAF-tick `ProcessEvents` is
  asyncify-suspended at a fiber swap, re-entering `ProcessIdle` /
  `DeletePendingObjects` (which frees windows) — classic UAF source. The pump
  has no re-entrancy guard.

## Suspects under test (symbolized builds running)

- **ASYNCIFY_REMOVE list** (`scripts/common/apply-asyncify.sh`): functions
  excluded from instrumentation (V8-size workaround, predates the committed
  post-asyncify `-O2`). An uninstrumented function on a suspend chain
  silently corrupts: it cannot save/restore its frame; its post-call code
  runs during the unwind and re-runs from the top during the rewind. The
  list is identical on main — the DOM port may have moved one of these onto
  a suspend chain (e.g. `PCB_EDIT_FRAME::setupUIConditions`, `match`,
  `COLOR_SETTINGS::COLOR_SETTINGS`, `BuildBitmapInfo`).
- Two debug knobs added to `apply-asyncify.sh`:
  `ASYNCIFY_KEEP_NAMES=1` (keep name section → named wasm stack traces) and
  `ASYNCIFY_REMOVE_OVERRIDE=""` (instrument everything). Named eeschema +
  gerbview builds are being produced from the docker volume's pre-finalize
  artifacts to (a) name the fatal chain + crash site, (b) test whether an
  empty removelist cures the fault.

## Pinpointing the window — live-set probe + the fix under test

Followed up with a C++ live-window probe (temp, in `wincmn.cpp`): `std::unordered_set`
of every `wxWindowBase` (insert in ctor, erase in dtor) + a ptr→class map recorded
while alive; checked at `SendIdleEvents` ENTRY (membership needs no deref, so it
flags a dead `this` reached as a top-level OR child window, in any vtable state).

Two results, both informative:

1. **The probe never fired** — every window the idle walk enters is "alive" by
   ctor/dtor accounting. But this does **not** exonerate the window-tree
   hypothesis: a child-list entry pointing at freed-then-**reused** memory has its
   address back in the set (the new owner), so membership passes. The live-set is
   blind to address reuse.
2. **The crash signature flipped `indirect call to null` → `indirect call
   signature mismatch`** across the rebuild (same spec, same path:
   `rAF → dynCall_v(ProcessEvents) → … → trap`). A stable structural null would
   not move; a function index **read from freed memory** does — null when the
   slot was zeroed, a valid-but-wrong-type index when the bytes were recycled.

Together these are the fingerprint of a **use-after-free on reused memory**: the
bad `call_indirect` index comes from a freed object whose contents shift with heap
layout (so adding instrumentation perturbs the symptom). This is exactly the
window-lifetime UAF of doc 09, and the reuse behavior reinforces — not weakens —
the early-detach fix: removing a window from its parent's `GetChildren()` list
*before* it can be freed and its address recycled removes the stale entry the idle
walk trips over.

**Fix under test** (wasm layer only — doc 09's first experiment):
- `src/wasm/window.cpp` `~wxWindowWasm`: do `SendDestroyEvent()` (sets
  `m_isBeingDeleted`) and `GetParent()->RemoveChild(this)` FIRST, before
  `wxDomDestroyControl`/`UnregisterElement` — so a synchronous browser focus/blur
  from DOM-node removal (or a parked-loop idle pass) can no longer reach this
  half-destroyed window through its parent. `RemoveChild` nulls `m_parent`, so
  `~wxWindowBase` won't double-detach (verified in wincmn.cpp).
- `src/wasm/domevents.cpp` `wx_dom_event`: ignore events when
  `window->IsBeingDeleted()`.

If still red after this, the remaining viable shapes (doc 09 §"If that fails")
are a duplicate child-list entry, re-entrant `ProcessEvents`/`DeletePendingObjects`,
or child-list iteration mutated mid-walk — and the right tool to name the exact
object becomes `-fsanitize=address` (defeats reuse-aliasing), resources permitting.

## De-park hypothesis — KILLED

The generated JS sets `noExitRuntime = Module["noExitRuntime"] || true`
(eeschema.js:3560), so `keepRuntimeAlive()` is always true. When
`_emscripten_set_main_loop(ProcessEvents,0,1)` throws `"unwind"` to park, the
exception is swallowed by `handleException` (`e == "unwind"` → return
EXITSTATUS, js:1552) **without** running `exit()`/atexit/`wxEntryCleanup`
(`maybeExit` is gated on `!keepRuntimeAlive()`). So the park does NOT tear down
main's C++ objects — the dossier 02 §7 "throw orphans a live buffer / app torn
down under the browser loop" concern does not apply here. Corroborated by the
trace: ~25 event/idle ticks run cleanly *after* the park before the crash; a
free-at-park bug would fault on the first idle walk, not the 25th. The fault is
**planted by a specific later suspend chain**, not by the park itself.

## Killed hypotheses

- Missing shim injection (all three shims verified present).
- Unwind escaping through EM_ASM DOM side effects (DIAG_ASMCONST silent in
  all traces; the synchronous `focusin → dispatch → wx_dom_event` re-entry
  observed at boot runs entirely at state 0).
- Buffer overwrite by overlapping activations (strict U/R alternation).
- Stack-pointer mismatch between unwind and rewind (SPs equal everywhere,
  fiber saved-SP slots consistent).
