# Collaborative editing under native wasm-EH — the virtual-call mis-dispatch: root cause & fix

> **Finalization note:** the `futex_yield.c` shim and the `vcall_*` / `pool-callafter` investigation
> repros referenced below were **removed** during feature finalization — native-EH needs none of them.
> This doc is retained as the root-cause record.

> **Status: RESOLVED 2026-06-28.** Fixed with a one-line build flag.
> **One-line:** native-EH pcbnew's collab **apply** hung at virtual method calls because the **embind
> translation unit was compiled without `-DDEBUG` while the core TU had it**. A `#if defined(DEBUG)`
> virtual (`EDA_ITEM::Show`) takes a vtable slot, so the two TUs' vtable layouts differed by one slot;
> every embind virtual call past that slot read the wrong slot and `call_indirect`-trapped on a
> signature mismatch — swallowed by the apply coroutine's `catch_all` → silent loop = "hang." **Fix:**
> define `DEBUG` for the embind TU in Debug builds (`scripts/kicad/build-kicad-target.sh`). No
> devirtualization; the A/B decision in Part 5 is moot. See [Resolution](#resolution--root-cause--fix).
>
> *Parts 1–5 below are the investigation as it unfolded; it concluded the `vii` correlation was a
> "confound" and weighed an A/B decision. That was right that the dispatch **mechanism** wasn't broken
> — but it stopped one step short of the dispatch **input**: the vtable **slot offset** the embind
> computed was wrong. A runtime vtable probe + a named-binary offset check (the user's "check the
> offsets, find the wrong one") closed it. Kept as the reasoning trail; the Resolution is the answer.*
>
> Companion to [`01-background-two-eh-models.md`](01-background-two-eh-models.md) (EH models),
> [`10-pthreads-native-eh.md`](10-pthreads-native-eh.md) (pthreads), and the `currData` dossier in
> [`docs/features/async/`](../async/).

---

## Resolution — root cause & fix

**Root cause (verified 2026-06-28).** The embind TU (`wasm/bindings/pcbnew_embind.cpp`, compiled
*outside* CMake in `build-kicad-target.sh` step 7) and the core/vtable-emitting TU disagreed on the
`PCB_TRACK` vtable layout by exactly one slot:

- `EDA_ITEM::Show(int, std::ostream&)` is declared `#if defined(DEBUG)` (`kicad/include/eda_item.h:471`).
- The **core** is built Debug → CMake `add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)`
  (`kicad/CMakeLists.txt:351`) defines `DEBUG` → `Show` occupies vtable **slot 35** → `PCB_TRACK::SetWidth`
  lands at **byte offset 320**.
- The **embind TU** was compiled **without `-DDEBUG`** → no `Show` slot → it computed `SetWidth` at
  **offset 316**, which in the emitted vtable is `BOARD_CONNECTED_ITEM::GetEffectiveNetClass()` (wasm
  type `ii`). Dispatching it as `vii` = a **`call_indirect` signature-mismatch trap**, swallowed by the
  apply COROUTINE's `catch_all` → silent retry loop = the observed "hang."
- Asyncify state at the park was **Normal** (confirmed via the real `asyncify_get_state()` export) — a
  trap, never a suspend. Every embind virtual call past slot 35 (`SetWidth`/`GetPosition`/the rebaseline
  snapshot getters) mis-dispatched; `Type`/`GetClass` (slots < 35) and core-TU calls
  (`commit.Modify`'s `Clone`) worked — which is what made it *look* signature-specific (`vii` fails,
  `ii` works).

**Fix.** `scripts/kicad/build-kicad-target.sh`: `EMBIND_CONFIG_DEFINES="-DDEBUG"` in the Debug branch
(empty in Release — Release defines no `DEBUG` in either TU, so the layouts already match), added to the
embind `em++` compile. Both TUs now agree on the layout; **all** embind virtual calls dispatch
correctly. The per-site devirtualizations tried during the hunt were reverted (unnecessary). *Hygiene
follow-up (recommended, not yet applied):* also give the embind TU `-DKICAD_USE_PLATFORM_WASM=1` and the
`-include char_traits_uint16_workaround.h` force-include, so its preprocessor/ABI environment matches
the core's exactly and this class of skew can't recur.

**How it was found.** A runtime probe (a fresh `PCB_TRACK` constructed at the park) showed
`tr.vtbl == fresh.vtbl` — the vtable *pointer* was correct — yet the fresh object hung identically,
ruling out a dead/wrong instance and pointing at the *slot offset*. `wasm-dis` of the named debug
binary then showed offset 316 holds `GetEffectiveNetClass`, not `SetWidth`. Exactly the user's
instruction: *"check the offsets of all instances, find the wrong one."*

**Independently-real fix kept** (not caused by the skew): the **COROUTINE** in `kicadCollabApply`
(the `commit.Modify`→`Clone` trap needs the fiber) and `-Xclang -fno-pch-timestamp`.

**`wasm/shims/futex_yield.c` — NOT needed; kept-but-not-compiled.** It was added during the hunt under
the (mistaken) theory that the apply hung on the thread-pool futex; the real cause was the vtable skew.
A **no-futex build passes collab 8/8** (Firefox + Chromium) — the connectivity recompute is bounded by
the pre-warmed pthread pool (`PTHREAD_POOL_SIZE = hardwareConcurrency`), so it never needs an on-demand
Worker, so there's no main-thread futex deadlock to fix here. The shim file + the `pool-callafter` repro
are kept as a *documented, validated* fix for the on-demand-Worker futex deadlock **if it ever surfaces**
(heavy board / cold pool hanging at `commit.Push`'s `RecalculateRatsnest`); the re-enable steps live in
`scripts/kicad/build-kicad-target.sh` (the "AVAILABLE BUT NOT COMPILED" block) and the shim's header.

---

## 0. How to read this document

This is written to be understood without prior knowledge of WebAssembly internals, C++ dynamic
dispatch, Asyncify, fibers, or futexes. **Part 1** explains every concept from scratch. **Part 2**
walks the actual call chains. **Part 3** lists the four bugs we found and the fixes that work.
**Part 4** is the investigation that proved the headline bug is a confound. **Part 5** is the A/B
decision and the concrete effort estimate for A. Skim the TL;DR, then dive into whatever you want.

### TL;DR

- KiCad's collaborative editing broadcasts each local edit to peers; a peer **applies** the change
  by running the same `BOARD_COMMIT` machinery a native edit uses.
- Under **native wasm-EH** (our target), that apply **hangs** at a C++ **virtual** method call
  (`SetWidth`, `GetPosition`, …). The same calls work fine under the old **legacy JS-EH** build.
- We found and fixed three real sub-bugs (a dispatch *trap*, a thread-pool *futex deadlock*, a fiber
  *finalization* hang). The fourth — the virtual-call hang — looked signature-specific (calls whose
  wasm signature is `vii` hang; `ii`/`viii` don't), but **five controlled repros prove `vii` dispatch
  is not actually broken**. It only hangs inside the real, huge module.
- **Devirtualizing** the call (telling the compiler the exact function so it emits a direct call
  instead of an indirect one) makes the hang vanish at that site — but that's treating a symptom, and
  it has to be repeated at ~10–15 call sites and re-done whenever new code adds a by-value getter.

---

# Part 1 — The concepts

## 1.1 Two ways C++ exceptions become WebAssembly

WebAssembly can't just "throw" like native code. Emscripten offers two lowerings:

- **Legacy JS exceptions (`-fexceptions`)** — every call that might throw is wrapped in a JavaScript
  helper called `invoke_<sig>` that does a JS `try/catch` around a `dynCall_<sig>` into wasm. The
  control flow for exceptions detours *through JavaScript*. Big and slow, but battle-tested.
- **Native wasm exceptions (`-fwasm-exceptions`)** — uses the WebAssembly exception-handling
  instructions (`try`/`catch`/`throw`) directly in wasm. No `invoke_*`, no JS detour. Smaller and
  faster — this is what we're migrating to. (See [`01-background-two-eh-models.md`](01-background-two-eh-models.md).)

This distinction matters later: under native-EH there are **no `invoke_*`/`dynCall_*` wrappers around
ordinary calls**, so the JS-side "self-heal" tricks that exist for legacy-EH don't apply.

## 1.2 Virtual method calls, vtables, and `call_indirect`

When you write `item->GetPosition()` and `GetPosition()` is declared `virtual`, the compiler does
**not** know which function to run. A `PCB_TRACK` returns its start point; a `PCB_VIA` returns its
centre; a `FOOTPRINT` returns its origin. The decision is made **at runtime** based on the object's
real type. This is **dynamic dispatch**, and it works via a **vtable**:

```
A PCB_TRACK object in memory          The PCB_TRACK vtable (one per class)
┌───────────────────────────┐         ┌────────────────────────────────────┐
│ vptr  ───────────────────────────►  │ slot 0: &PCB_TRACK::Type            │
│ m_Start = (10, 20)         │         │ slot 1: &PCB_TRACK::GetPosition     │
│ m_End   = (50, 20)         │         │ slot 2: &PCB_TRACK::SetWidth        │
│ m_width = 200000           │         │ ...                                 │
└───────────────────────────┘         └────────────────────────────────────┘
```

Every object of a polymorphic class starts with a hidden pointer (`vptr`) to its class's vtable. A
virtual call compiles to: *load the vptr → load the function pointer in the right slot → call it.*

In WebAssembly there are no raw function pointers; instead there is a single **function table** (an
array of functions) and a `call_indirect N` instruction that means "call the function at table index
N." So the C++ virtual call becomes, in wasm:

```wat
local.get $item            ;; the object pointer
i32.load offset=0          ;; load vptr (the vtable address)
i32.load offset=8          ;; load the function index from the SetWidth slot
;; ... push the call args ...
call_indirect (type $vii)  ;; call the function at that table index, expecting signature "vii"
```

`(type $vii)` is the *expected signature* baked into the instruction. **Signature notation:** the
first letter is the return type, the rest are arguments. `i`=i32, `v`=void.
- `ii` = `(i32) -> i32` — e.g. `KICAD_T Type()` (takes the hidden `this`, returns an int-like value).
- `vii` = `(i32, i32) -> void` — e.g. `void SetWidth(int)` (`this`, the int; returns nothing).
- `viii` = `(i32, i32, i32) -> void` — e.g. `view->Update(item, flags)`.

**A subtlety that matters here — struct returns (the "sret" ABI).** A method that *returns a small
struct by value*, like `VECTOR2I GetPosition()`, can't return two ints in one wasm value. The
compiler rewrites it so the caller passes a hidden pointer to a return slot, and the function writes
through it and returns nothing: `void GetPosition(this, VECTOR2I* out)`. That's also signature `vii`.
So **all of `GetPosition`, `GetClass`, `GetText`, `GetTextSize` (struct/string returns) AND
`SetWidth`, `SetPosition` (void setters) are `vii`** — which is why the bug *looked* signature-specific.

## 1.3 Devirtualization (the fix technique, and its trade-off)

If you class-qualify the call — `static_cast<PCB_TRACK*>(item)->PCB_TRACK::GetPosition()` — you tell
the compiler *exactly* which function to run. It no longer needs the vtable; it emits a plain `call`
(a direct call to a known function), not a `call_indirect`. We already do this elsewhere: see
`itemLayer()` / `itemPosition()` / `itemClass()` in `wasm/bindings/pcbnew_embind.cpp`.

**Why it helps the bug:** the hang fires *at the instrumented `call_indirect`*. A direct `call`
isn't wrapped the same way, so the symptom doesn't appear there.

**The trade-off / why it's not free:** class-qualifying picks *one* class's version. For a generic
getter like `GetPosition` whose answer depends on the type, you must dispatch on the type yourself
(`switch (item->Type()) { case PCB_TRACE_T: ...; case PCB_VIA_T: ...; }`) and class-qualify each arm
— otherwise you call the wrong override and get wrong geometry. That's why devirtualization here is a
**helper per generic getter** plus a hand edit per setter, and why it's "finite but spread."

## 1.4 Asyncify — making synchronous C++ pause and resume

KiCad's C++ is written **synchronously**: it calls `sleep`, it *blocks* waiting for worker threads,
it pops up a modal dialog and waits for the user. In a browser you **cannot block the main thread** —
if you do, the page freezes (no rendering, no input, no timers). The reconciliation is **Asyncify**, a
Binaryen transform that rewrites the wasm so a deep synchronous call stack can be **suspended**
(unwound back to the JS event loop) and later **rewound** (rebuilt exactly where it left off).

Mechanically, Asyncify instruments functions so that:
- on **unwind**, each function saves its locals + a "where was I" call-index into a memory buffer and
  returns up the stack until it reaches the event loop; the buffer pointer is `Asyncify.currData`.
- on **rewind**, each function restores its locals and jumps back to the saved call-index, rebuilding
  the stack until execution resumes at the suspend point.

There is **one** `currData` slot at a time. If a second suspend starts while the first's buffer is
still occupied, things collide — that's the "nested currData contention" family
([`docs/features/async/`](../async/)). The three states are **Normal (0)**, **Unwinding (1)**,
**Rewinding (2)** (`Asyncify.state` / the wasm export `asyncify_get_state()`).

Why we can't avoid Asyncify: native edits already rely on it (tool interactions suspend mid-drag, the
event loop yields each frame). The collab apply runs the same `BOARD_COMMIT` code, so it inherits the
same instrumentation.

## 1.5 Coroutines and libcontext fibers (KiCad's `COROUTINE`)

A **fiber** is a *second call stack* you can switch to and from cooperatively (no OS thread). KiCad
ships its own `COROUTINE` (built on `libcontext`'s `jump_fcontext`) and runs **tool interactions** on
a fiber so a tool can "yield" in the middle of an operation and be resumed later. The fiber has its
own stack memory; switching is just swapping the stack pointer.

Why it's in the collab apply: KiCad-WASM has a long-standing rule that the heavy edit machinery
(`BOARD_COMMIT::Modify` → `item->Clone()`, the GAL `view->Add`) **only dispatches correctly when run
on the tool-coroutine fiber stack** — running it from a bare `CallAfter`/`ccall` trapped with
"indirect call signature mismatch." So `kicadCollabApply` wraps `doApply` in a `COROUTINE`. (This is
fix #1 below; it's real and necessary.)

## 1.6 Futexes — and why they deadlock on the browser main thread

A **futex** ("fast userspace mutex") is the low-level OS primitive a thread uses to **wait until
another thread signals it**. `std::mutex`, `std::condition_variable`, and `std::future::get()` are all
built on it. The pattern: thread A wants thread B's result, so A does `futex_wait(addr, val)` — "sleep
until the value at `addr` changes" — and B does `futex_wake(addr)` when it's done.

KiCad's **connectivity recompute** (rebuilding the ratsnest/net graph after an edit, in
`commit.Push`) is parallelised across a thread pool. The main thread submits work and then
`futex_wait`s for the workers' results (`std::future::wait_for` → `pthread_cond_wait` →
`emscripten_futex_wait`).

**The browser problem:** on the **main browser thread**, `Atomics.wait` (the real blocking wait) is
*forbidden* — blocking it would freeze the page. Emscripten's fallback is to **busy-spin** in
`futex_wait_main_browser_thread()`, calling `_emscripten_yield()` — but that only services the
internal proxy queue, it **never returns to the JS event loop**. So if the worker the main thread is
waiting on still needs the event loop to run (e.g. an **on-demand Web Worker** has to finish its
`loaded → run` handshake), it never gets to — and the busy-spin spins forever. **Deadlock.**

```
main thread:  submit work ──► futex_wait(result) ──► busy-spin _emscripten_yield()  ──► (spins forever)
                                                         │ never pumps the JS event loop
worker boot:  'loaded' ─X─► 'run'   (needs the event loop, which never runs) ──► never produces result
```

**Our solution — `wasm/shims/futex_yield.c`** (fix #2 below): a *strong override* of
`emscripten_futex_wait` that, **on the main thread only**, polls the futex word and between polls does
an **Asyncify yield** (`await setTimeout(0)`) instead of busy-spinning. Yielding pumps the JS event
loop, so the on-demand Worker boots, finishes, wakes the futex, and the wait returns. (On worker
threads it keeps the real blocking `memory.atomic.wait32`.) It's the sibling of the existing
`nanosleep_yield.c`, which covers the `sleep_for`/`nanosleep` path but not the futex path.

## 1.7 The function table and `-sDYNCALLS=1` (one caveat to retire a red herring)

There is a single wasm function table; `call_indirect` indexes it; the type section lists the
distinct signatures (`ii`, `vii`, `viii`, …). With `-sDYNCALLS=1` emscripten also exports per-signature
`dynCall_<sig>` trampolines for JS↔wasm calls. A known hazard (`dyncall-binding.js.tmpl`) is that
*post-asyncify+O2 a `dynCall_<sig>` JS trampoline can carry a stale expected type* — but that is a
**legacy-EH** mechanism (the `invoke_*`→`dynCall` path). Under native-EH a C++ virtual call is a
**raw `call_indirect`**, not a `dynCall`, so that hazard does not apply. (Verified by disassembly — see
Part 4.)

---

# Part 2 — The collab apply call chains

A peer edit arrives as JSON and is applied like this:

```
JS: window.Module.kicadCollabApply(jsonDelta)
  └─ kicadCollabApply(std::string)                         [pcbnew_embind.cpp]
       └─ parse JSON → fr->CallAfter([...])                (defer to the wx main-loop drain)
            └─ COROUTINE cor([]{ doApply(fr, delta); })    (run on a libcontext fiber — §1.5)
                 └─ cor.Call(0)
                      └─ doApply(frame, delta)             [pcbnew_embind.cpp]
                           ├─ for removed:  commit.Remove(item)
                           ├─ for changed:  commit.Modify(item)  ──► item->Clone()   (virtual, fix #1)
                           │                applyChanged(item, j)
                           │                  └─ tr->SetStart/SetEnd (non-virtual, fine)
                           │                  └─ tr->SetWidth(w)     ◄── VIRTUAL "vii"  ★ THE HANG
                           ├─ for added:    makeItem(...) → commit.Add(item)
                           └─ commit.Push("Collaborative edit")     [board_commit.cpp]
                                └─ connectivity->RecalculateRatsnest(...)
                                     └─ thread-pool results.get() ──► emscripten_futex_wait  (fix #2)
       └─ (back on the main stack, after cor.Call returns)
            └─ rebaseline()                                (fix #3 — moved out of the fiber)
                 └─ snapshotByUuid(board)
                      └─ for each item: itemToJson(item)
                           └─ itemPosition/itemClass/GetText…  ◄── VIRTUAL "vii"  ★ more of THE HANG
```

Three things in this chain are independently load-bearing, and each was a distinct bug:

1. `item->Clone()` (inside `commit.Modify`) and `view->Add` (inside `commit.Push`) **must** run on the
   fiber or they trap → **fix #1 (COROUTINE)**.
2. `commit.Push`'s connectivity recompute **futex-deadlocks** on the main thread → **fix #2
   (`futex_yield.c`)**.
3. `rebaseline()` (the post-apply snapshot) at the end of `doApply` **must** run after the fiber
   finalizes, not inside it → **fix #3 (move to main stack)**.

And then the headline: the `vii` virtual calls (`SetWidth`, the snapshot getters) **hang**.

---

# Part 3 — The four bugs and the fixes that work

| # | Bug | Symptom | Fix | Status |
|---|-----|---------|-----|--------|
| 1 | `Clone`/`view->Add` dispatch off the fiber | "indirect call signature mismatch" **trap** | run `doApply` in a `COROUTINE` (`kicadCollabApply`) | ✅ validated |
| 2 | connectivity recompute futex on main thread | busy-spin **deadlock** (Worker can't boot) | `wasm/shims/futex_yield.c` (Asyncify-yield the main-thread futex wait) | ✅ validated (red→green in the `pool-callafter` repro) |
| 3 | `rebaseline()` inside the fiber after a suspend | fiber **never finalizes**, blocks the next apply | move `rebaseline()` to the main stack after `cor.Call` | ✅ validated |
| 4 | `vii` virtual calls (`SetWidth`, snapshot getters) | **hang** (asyncify suspend-without-resume) | devirtualize the call (class-qualify) **OR** … (see Part 4) | ⚠️ confound; see below |

Fixes 1–3 are real and should be kept regardless of the Part-5 decision. Build-plumbing fix to keep
too: `-Xclang -fno-pch-timestamp` in `build-kicad-target.sh` (a PCH-staleness workaround).

---

# Part 4 — The `vii` hang is a confound (the investigation)

### The symptom and the obvious (wrong) theory
The apply hangs at `SetWidth` (a `vii` call). Devirtualize it → the apply progresses to the next
`vii` call (`GetPosition` in the snapshot) → devirtualize that → the next `vii` (`GetClass`, then the
text getters) … Meanwhile value-returning `ii` virtuals (`Type`, `GetLayer`, `GetWidth`, `Clone`) and
3-arg-void `viii` virtuals (`view->Update`) work. **Obvious theory: the `vii` signature is broken.**

### Five controlled repros — all pass
We built a minimal libcontext-fiber app
(`tests/apps/standalone/coroutine-pthread/vcall_fiber_repro.cpp`) that calls all four signatures on a
non-devirtualizable polymorphic object (99 genuine `call_indirect`s, confirmed not optimized away),
native-EH + asyncify, and progressively added every suspected ingredient:

| Repro | Added ingredient | Result |
|-------|------------------|--------|
| `vcall_fiber_repro` | fiber + 4 signatures | **all pass** |
| + interleaved suspend | `emscripten_sleep` before each call | **all pass** |
| `vcall_mainloop_repro` | rAF `set_main_loop` → `dynCall_v` → COROUTINE | **all pass** |
| `vcall_ehloop_repro` | `try`/`catch_all` + RAII dtor + suspend-in-try + loop | **all pass** |
| FIX-D test (real pcbnew) | save+clear `Asyncify.currData` before the apply | `currData` was **null**; still hangs |

A `vii` `call_indirect` dispatches correctly under *every* condition we could isolate — fiber,
suspend/rewind, the rAF/`dynCall_v` boundary, even nested native-EH `try/catch_all` (the
HoistCppCatches regime) with a suspend inside the try. The signature is **not** the cause.

### Two binary-level investigations — dispatch ruled out
Disassembling the actual `pcbnew.wasm` and the repro:
- The asyncify pass is **type-agnostic** (`binaryen/src/passes/Asyncify.cpp`): the void path is the
  *simpler* subset; `vii` and `viii` get a byte-for-byte identical guard.
- The function **table is not reordered/re-indexed** (the only fork pass, `HoistCppCatches`, is
  intra-function; `-O2` `directize` preserves index→function bindings).
- The **embind-vs-core compile flags match** (same EH model, `-O`, RTTI, struct-ABI) — no ABI
  divergence that could move a vtable slot or change a `call_indirect` type.
- The parking `SetWidth` call site has the **same asyncify guard** as the repro's working `setVii`;
  no trampoline, no `i64` legalization, no stale type.

The single structural difference round-2 could point to was that the real call sits inside `doApply`'s
**deeply nested native-EH `try`/`catch_all`** (48 `try` / 47 `catch_all` — every throwing JSON access
has RAII cleanup), in a **loop**, with the suspend landing inside hoisted catch scopes — but the
`vcall_ehloop_repro` reproduced exactly that and **passed**.

### Conclusion
The `vii` correlation is a **confound**: devirtualizing removes the instrumented `call_indirect` and
shifts the symptom to the next one. The dispatch is provably fine. The hang is a property of the **full
180 MB asyncify+O2 module's runtime** (the real KiCad vtables/table in the live process) that no
isolation reproduces and that the disassembly couldn't byte-verify. The one stubborn fact that resists
*every* named mechanism: `SetWidth` parks with asyncify state **Normal** and `currData` **null** —
identical to the repros that pass.

---

# Part 5 — The decision: (A) devirtualize-through vs (B) defer collab

Everything else in native-EH pcbnew is green (core e2e, 3D, and every other app —
[`10-pthreads-native-eh.md`](10-pthreads-native-eh.md)). Collab apply is the lone holdout.

## Option A — devirtualize-through (treat the confound, get collab green)

Replace every `vii` virtual call in the apply path with a class-qualified (direct) call, dispatching
on `Type()` where the override matters.

**Where the remaining work is** (the snapshot getters are already done via `itemPosition`/`itemClass`/
the text-getter edits):

| File | Site | Kind |
|------|------|------|
| `wasm/bindings/pcbnew_embind.cpp` | `applyChanged` else-branch `aItem->SetPosition(...)` | 1 setter (type-dispatched) |
| `wasm/bindings/pcbnew_embind.cpp` | `makeItem` added-item setters (`SetWidth`, `SetPosition`, `SetText`, …; `SetStart/SetEnd` already non-virtual, `SetLayer` already done) | ~3–5 setters |
| `wasm/bindings/eeschema_embind.cpp` | `itemToJson` snapshot getters (`GetPosition`/`GetClass`/text — mirror of pcbnew's helpers) | ~3–5 getters (1 helper pair) |
| `wasm/bindings/eeschema_embind.cpp` | `doApply`/`applyChanged` + `makeItem` setters (`Move` already devirtualized) | ~3–5 setters |

**Effort estimate (concrete):**
- **Code edits:** ~10–15 sites, each a 1-line class-qualify or a small `switch(Type())` helper.
  Mechanically small — call it **2–4 hours of editing**, including writing the eeschema helper pair to
  match pcbnew's.
- **The real cost is the build/test loop, not the edits.** Each un-devirtualized `vii` surfaces *one
  at a time* (the apply hangs at the first one; you fix it, rebuild ~10–35 min, it hangs at the next).
  Doing it reactively ⇒ **~10–15 build cycles**. You can cut that by proactively grepping every
  by-value getter/void setter in the two apply paths and devirtualizing them in one pass, but you
  still need a few full builds + the **3-browser** e2e (Firefox/Chrome/WebKit) per the project rule.
  Realistically **~1–2 days wall-clock**, dominated by builds + cross-browser verification.
- **Fragility (the ongoing cost):** this fixes nothing structural. Any *new* by-value getter or void
  setter added to the apply/snapshot path later — a new field synced, a KiCad upstream change — will
  **silently re-hang** the apply under native-EH. Mitigation: a prominent comment + ideally a tiny
  lint/grep in CI flagging un-class-qualified virtual calls in the embind apply paths. Without that,
  it's a latent foot-gun.

**Net:** A is a known, finite, *certain* path to green, but it's symptom-treatment with a maintenance
tail.

## Option B — defer collab (recommended)

Ship native-EH as the default for everything that's green (core, 3D, gerbview, pl_editor,
symbol_editor, eeschema, footprint_editor). Leave the **collab apply** path as a documented native-EH
limitation; it continues to work on the legacy-EH build. Revisit if/when the large-module root ever
surfaces (e.g. a future Binaryen/emscripten bump changes the picture, or someone reproduces it
minimally).

**Why recommended:** the root resisted 2 deep binary investigations + 5 controlled repros + the FIX-D
test; the dispatch is provably correct; A is fragile work treating a confound. The cost/benefit of
chasing a non-reproducible large-module runtime bug — or maintaining a hand-devirtualized apply path
forever — is poor relative to shipping the 95% that's done.

---

## Appendix — artifacts and key locations

**Validated fixes (keep):**
- `wasm/shims/futex_yield.c` — main-thread futex Asyncify-yield (fix #2).
- `wasm/bindings/pcbnew_embind.cpp` — `kicadCollabApply` COROUTINE (fix #1) + rebaseline-on-main-stack
  (fix #3) + the `itemPosition`/`itemClass`/`itemLayer` devirtualized snapshot helpers.
- `scripts/kicad/build-kicad-target.sh` — `futex_yield.o` wired into the link; `-fno-pch-timestamp`.

**Isolation repros (temporary — remove before final staging):**
- `tests/apps/standalone/coroutine-pthread/vcall_fiber_repro.cpp` (signature isolation, +suspend)
- `tests/apps/standalone/coroutine-pthread/vcall_mainloop_repro.cpp` (rAF/`dynCall_v` context)
- `tests/apps/standalone/coroutine-pthread/vcall_ehloop_repro.cpp` (try/catch_all + RAII + loop)
- `tests/apps/standalone/pool-callafter/` (the futex deadlock red→green repro for fix #2)
- `tests/e2e/coroutine-vcall.spec.ts`, `tests/e2e/coroutine-poolwait.spec.ts`
- The `[collab-diag]`/`[push-diag]` `EM_ASM` markers in `pcbnew_embind.cpp` + `board_commit.cpp` and
  the `#include <emscripten.h>` in `board_commit.cpp` are temporary diagnostics to revert.

**Key reading:**
- `binaryen/src/passes/Asyncify.cpp` — the suspend/rewind instrumentation (type-agnostic).
- `scripts/common/inject-dyncall-shims.sh`, `scripts/common/shims/{handlesleep.js,dyncall-binding.js.tmpl}`
  — the JS-side asyncify/dynCall plumbing.
- [`docs/features/async/`](../async/) — the `currData` contention dossier.
