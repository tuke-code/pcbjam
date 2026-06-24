# Native wasm-EH × pthreads — findings, the pool-refactor plan, and the test gap

> **Status:** active investigation, part of the native-EH port. Authored 2026-06-24.
> This is the **exception-handling-side** companion to the mechanism-deep
> [`../threading/README.md`](../threading/README.md). That doc explains *threading in general*
> (the 3-layer model, the deadlock mechanics, the 3 failure modes). **This** doc is scoped to one
> question: **what does native wasm-EH (`-fwasm-exceptions`) do to pthreads, what's blocking the
> native-EH port's test suite because of it, and what do we do next.**

## Why this exists

We are migrating KiCad-WASM from Emscripten JS exceptions (`-fexceptions`) to native WebAssembly
exceptions (`-fwasm-exceptions`) for the bundle-size win (pcbnew ~64.5 → ~36 MB gz). The port isn't
"done" until the **existing test suite is green under native-EH**. It's almost there — but a cluster
of pthread tests fails, and the failure is *native-EH-specific*. So threading became a required
side-quest for the EH port. **It is not the EH port's goal**; it's a blocker we have to clear (or
quarantine) for the suite to pass.

## TL;DR

- **Native-EH wx suite (chromium): 311 passed / 1 skipped / 5 failed.** All 5 failures are the **raw-thread
  raytracer test** (`coroutine-raytrace.spec.ts`). Everything else — including the dedicated thread-pool
  test and the fiber+pthread coroutine suite — is green. (The earlier "~90 failures" were a build-pipeline
  artifact, now fixed and committed; see §1.)
- **The bug — RESOLVED.** The post-link Asyncify pass (`hoist-and-asyncify.sh`) **omitted
  `emscripten_sleep`** from its asyncify-imports list, so binaryen never instrumented the functions that
  yield via it (the raytracer threading). Un-instrumented, such a function calls `emscripten_sleep` a
  second time *before unwinding* → `Aborted(invalid state: 1)`. **native-EH-specific** because emcc
  auto-adds `emscripten_sleep` to the in-link Asyncify (JS-EH), but our hand-written post-link list didn't.
  **Fixed** by adding it (+ the other emcc async built-ins). `main()` is **NOT** re-entered — an earlier
  reading I had to correct (§2).
- **Result:** the raytracer runs **multi-core under native-EH — `serial 1342ms → parallel 142ms = 9.45×`
  on 16 cores.** All 6 `coroutine-raytrace` tests pass in Chromium; verified multi-core in Firefox too.
  (WebKit is blocked by a *separate*, pre-existing COEP worker-load limitation — §2a.)
- **Implication for the pool:** this also de-risks the refactor — a `multi_future::wait()` that yields via
  `emscripten_sleep` would have hit the identical gap, now closed. The pool-migration plan (§4) stands and
  is now lower-risk.
- **The plan (later, upstreamable):** refactor KiCad's six raw-`std::thread` raytracer sections to
  `GetKiCadThreadPool()` (exactly as upstream already did for `renderTracing`), delete the
  `#ifdef __EMSCRIPTEN__` serial fallbacks, and prefer the pool everywhere. Not much work, but a *later*
  step — after the EH port's suite is green.

---

## 1. Where we are (native-EH port status)

The native-EH wx app suite is **316 / 1 skipped / 0 failed** (chromium) — matching JS-EH. Getting from a
misleading ~90 "failures" to green took clearing a set of build-pipeline gaps that masqueraded as native-EH
test failures (committed this session), then the §2 asyncify-imports fix for the last 5:

- **Post-link Asyncify find too narrow** — the loop matched only `standalone/*/*_test.wasm`, silently
  skipping `apps/minimal_test.wasm` (apps/ root) and the coroutine-pthread repros / wxpt. Those linked but
  were never asyncify-instrumented → `asyncify_start_unwind not found`. Broadened to all freshly-linked
  app wasm.
- **Repro apps mixed EH models** — the coroutine-pthread `*_repro` apps hardcoded JS-EH in their link
  recipes while their compile inherited native-EH → `undefined symbol: __cpp_exception`. Made them
  EH-aware so they build native-EH under `WX_NATIVE_EH` (and JS-EH otherwise).
- **`build-wasm-test.sh` swallowed make failures** — it continued to the post-link after a failed make,
  leaving apps half-instrumented (read as mass test failures). Now aborts loudly.

After those, the only remaining failures were the 5 raytracer threading tests — the real,
native-EH-specific signal, root-caused and fixed in §2.

---

## 2. The root cause: a missing `emscripten_sleep` in the post-link asyncify-imports

`coroutine-raytrace.spec.ts` aborted with `Aborted(invalid state: 1)`. Traced to the bottom:

- `invalid state: 1` is `Asyncify.handleSleep` aborting because the state is **Unwinding** — a second
  suspend starting before the first rewinds.
- Logging every `handleSleep`, the state sequence at the abort is exactly **`0,1`**: two `emscripten_sleep`s
  back-to-back with **no rewind between**. So a function calls `emscripten_sleep`, the unwind arms
  (state→Unwinding), and the **same function calls `emscripten_sleep` again before returning**. A correctly
  Asyncify-instrumented function has a post-call "if Unwinding, save locals and return" check after every
  suspend point; this one doesn't → **Asyncify never instrumented it.**
- **`main()` is NOT re-entered.** (An earlier reading of mine was wrong: a `[MAINCALL]` probe fired exactly
  once. The abort stack only *shows* `main`'s frames because Asyncify's unwind/rewind runs inside a
  `setTimeout`-driven `doRewind` that keeps the JS stack live.)
- **Why un-instrumented:** binaryen's Asyncify instruments only functions that can reach a *listed* async
  import. The post-link list in `hoist-and-asyncify.sh` was `startModal, js_*, invoke_*, __asyncjs__*,
  emscripten_fiber_swap` — curated for the wx apps, which yield via **fibers**. It **omitted
  `emscripten_sleep`**, which the raytracer (and B1/B2/m4) yield via. `env.emscripten_sleep` *is* a wasm
  import, so binaryen can match it — it just wasn't told to.

### The exact JS-EH ↔ native-EH difference

Under **JS-EH**, Asyncify runs **in-link** and **emcc auto-adds** `emscripten_sleep` (+
`idb_*`/`wget`/`scan_registers`/`lazy_load`) to the imports. Under **native-EH** we run Asyncify
**post-link by hand**, with an explicit list that dropped those auto-imports. That is the *entire*
difference — **not** a fundamental native-EH × pthread incompatibility, and **not** the
handleSleep-vs-arbiter question (the currData shim was never involved). The verification matrix had hinted
native-EH "introduced" a raytracer failure (config 1 JS-EH = 0, config 2 native-EH = 5) — correct, but the
mechanism is this imports gap, not a spawn-topology problem.

### The fix + result

Added `env.emscripten_sleep` (+ `scan_registers`, `lazy_load_code`, `wget`, `wget_data`, `idb_*`) to
`hoist-and-asyncify.sh`. Rebuilt + verified:

| Check | Result |
|---|---|
| Full wx suite, Chromium | **316 / 1 skipped / 0 failed** (was 311/5) |
| `coroutine-raytrace.spec.ts` — all 6 (B1/B2/B1-local/B3 + speedup + A neg-control) | **6/6 pass** |
| multi-core speedup test | **serial 1342ms → parallel 142ms = 9.45× on 16 cores** |
| raytrace `#m=5` default (drains pool → on-demand creation) / `#m=1`, Chromium + Firefox | **SUCCESS, workersRan=16** |

The default `m=5` — which *drains* the pre-warmed pool and forces on-demand Worker creation — now succeeds
too, so the fix **also resolves the threading-doc's mode-(a) "deadlock"**: the `sleep_for` join now yields
via an instrumented `emscripten_sleep` instead of busy-spinning the event loop and starving the worker
handshake. (The earlier empirical notes — "pre-warmed spawn still aborts", "B3 doesn't escape", "non-wx
`main_repro` passes" — were all real, but symptoms of the missing import: `main_repro` yields via fibers,
which *were* listed.)

### 2a. The remaining WebKit issue (separate, pre-existing)

In WebKit the asyncify side runs (threads spawn) but the **pthread worker `.js` load is refused on COEP**
(`Refused to load worker because of Cross-Origin-Embedder-Policy`) even with COOP + COEP + CORP all served
and `crossOriginIsolated:true`. It's a WebKit/playwright-headless COEP-worker strictness issue affecting
**all** pthread apps — which the chromium-only wx config never ran in WebKit — and is unrelated to the
asyncify fix. Tracked separately.

---

## 3. The pivotal contrast: the pool pattern survives native-EH

Two wx apps, two outcomes:

| App / test | Thread pattern | native-EH |
|---|---|---|
| `threadpool_test.cpp` (`threadpool.spec.ts`) | create `hwc` `std::thread`s into the **pre-warmed** pool, run a short body, **`join()`** each | **PASS** |
| `raytrace_threads_test.cpp` (`coroutine-raytrace.spec.ts`) | raw detached/persistent `std::thread`, **busy-wait** join, default **drains** the pool → on-demand creation | **FAIL** (`invalid state: 1`) |

The difference is the *pattern*: **create + clean join into a pre-warmed pool works; persistent raw
threads + busy-wait (and on-demand creation) abort.** This is direct evidence that moving KiCad off raw
threads and onto the standing pool is the right cure — *if* the actual pool API behaves like
`threadpool` (clean) rather than like the raytracer (abort). See the gap in §6.

---

## 4. The plan (later, upstreamable): KiCad raw-threads → the pool

This is the destination, not the immediate task. Per [`../threading` §8](../threading/README.md), upstream
KiCad has migrated only **1 of 7** raytracer parallel sections to `GetKiCadThreadPool()` (`renderTracing`,
and that one accidentally); the other six are untouched **2018 OpenMP-translation** raw-thread code:

| Site | Pass | Today |
|---|---|---|
| `render_3d_raytrace_base.cpp:764` `shadeWorker` | post-process shading | raw `std::thread` + busy-wait, `#ifdef`'d serial in WASM |
| `render_3d_raytrace_base.cpp:835` `blurWorker` | blur/finish | same |
| `render_3d_raytrace_base.cpp:1456` `previewWorker` | preview | same |
| `image.cpp:525` `filterWorker` | `EfxFilter` AA/blur | same |
| `create_layer_items.cpp:848` `zoneWorker` | zone-fill geometry | same |
| `create_layer_items.cpp:1311` `simplifyWorker` | polygon simplify | same |

**Refactor = migrate these six to `submit_task()` + `multi_future::wait()`** (the `renderTracing` shape,
refined by upstream `bccf36538` to wait on *own* tasks only), and **delete the `#ifdef __EMSCRIPTEN__`
serial fallbacks**. Why this is the right move:

- **Upstreamable, not a wasm hack** — precedent in the same file, a filed upstream issue
  ([GitLab #20911](https://gitlab.com/kicad/code/kicad/-/issues/20911), "ray tracing high system load"),
  and it removes dead OpenMP-era code. If accepted upstream, our fork carries **zero** divergence here.
- **Fixes the deadlock for free** — pool reuse ⇒ no on-demand Worker creation (the threading-doc mode-a
  cure), and per §3 the clean pool pattern also dodges the native-EH spawn abort.
- **Less divergence, not more** — it lets us drop the raytracer `#ifdef`s; combined with native-EH
  letting us drop the `detach_task` shim, net fork divergence goes *down* while threads come *on*.

**The caveat that gates it:** this only works if the **real `BS::thread_pool`** (persistent workers +
`submit_task` + `multi_future::wait()`) survives native-EH. §3 proves *raw create+join* survives; it does
**not** yet prove the *submit+yield-wait* pattern does. That's the test gap (§6) — close it first.

**Effort:** small and mechanical (six call-sites, one well-known target API). Schedule: **after** the
native-EH suite is otherwise green.

---

## 5. The 3D viewer (correcting the record)

The 3D viewer is **live and single-threaded**, not "parked." History: the original was commented out →
we enabled it → it errored (the threading deadlock/abort) → we made it **single-threaded** (the
`#ifdef`'d serial fallbacks), which is what ships. A separate **multi-threaded** version exists
(the `WASM_RAYTRACE_POOL` work, ~6–7×) but isn't the active path. The §4 refactor is what turns the live
viewer multi-threaded *cleanly* (pool-based, upstream-shaped) instead of via the parked raw-thread spike.

---

## 6. pthread test inventory, coverage, and the gap

### What exists

| Spec | App(s) | What it exercises | native-EH |
|---|---|---|---|
| `coroutine-pthread.spec.ts` (8) | `*_repro`, `coroutine_test_wxpt` | Asyncify **fiber** (tool coroutine) **+ pthreads**, across activation paths: main, nested `invoke_*`/dynCall boundary, wx event loop, main-loop(rAF), WebGL2, WebGL2+pthreads, post-coroutine virtual call, embind | **PASS** (after §1 fixes) |
| `threadpool.spec.ts` (2) | `threadpool_test` | create `hwc` raw `std::thread`s + **`join()`** (pre-warmed pool); "no deadlock", "all threads started" | **PASS** |
| `coroutine-raytrace.spec.ts` (6) | `raytrace_threads_test` | raw-thread raytracer mechanisms by `#m=`: A(0) detached+busy-wait negative control, B1(1) detached+`emscripten_sleep`, B2(2) persistent+`emscripten_sleep`, C(3) serial, B1-local(4) stack-local atomics, B3(5) persistent+busy-wait | **FAIL (5)** + the A `test.fail()` disrupted |
| `asyncify/asyncify-races.spec.ts`, `asyncify/eh-spike.spec.ts` | (asyncify / eh-spike) | Asyncify re-entrancy "park throw" races; Asyncify×wasm-EH `HoistCppCatches` probe. **Not pthread-focused**, but adjacent. | PASS |

### Coverage read

- **Covered + green:** fiber+pthread (coroutine-pthread); raw **create+join** into a pre-warmed pool
  (threadpool).
- **Covered + red (the signal):** raw **detached/persistent + sleep/busy-wait**, and **on-demand** Worker
  creation (raytracer). This is the native-EH spawn regression (§2).
- **The gap (uncovered):** the **actual `BS::thread_pool` API** — persistent workers + `submit_task()` +
  `multi_future::wait()` — under native-EH. `threadpool` is the closest but is raw create+join (not
  submit/yield-wait); the raytracer's persistent+yield (B2) is raw `std::thread`, not the BS pool, and it
  *aborts*. **So the exact pattern the §4 refactor targets is currently untested under native-EH.**

### Tests to add (immediate side-quest)

1. **BS-pool-API test** — a standalone app that includes `bs_thread_pool.hpp` (or faithfully mirrors
   `GetKiCadThreadPool()`), `submit_task`s a batch of pure-compute tasks, and `multi_future::wait()`s,
   under native-EH. This is the decisive test for §4: green ⇒ the refactor is safe; red ⇒ the pool's
   own join is hit by the nesting and we need Design B first. Run in all three engines.
2. **Re-entrant-`main` minimal repro** — smallest wx app that spawns one main-thread `std::thread` and
   instruments `main`'s entry + Asyncify state, to answer §2's open question (fixable artifact vs
   fundamental). Pure diagnostic; can be deleted after.
3. (Optional) **negative-control hygiene** — make the raytracer A(`m=0`) `test.fail()` robust to an
   `invalid state` abort vs a clean `workersRan=0`, so it reports the expected failure rather than a
   timeout.

---

## 7. Next steps (ordered)

1. **DONE — `coroutine-raytrace` root-caused and fixed (§2).** The JS-EH↔native-EH difference is exact and
   documented: the post-link asyncify-imports list omitted `emscripten_sleep`. Fixed; suite 316/0, the
   raytracer runs multi-core (9.45×). It was **not** the arbiter-vs-shim question — the currData shim was
   never involved. (Remaining sub-item: the §2a WebKit COEP worker-load limitation for pthread apps.)
2. **Continue the wx suite under native-EH** (the EH port's actual goal) — keep the 311 green across
   Firefox/Chrome/WebKit; the raytracer 5 are the known, isolated exception.
3. **Close the test gap (§6.1):** write the BS-pool-API native-EH test — decisive for whether §4 is
   sufficient, and a green target for the refactor.
4. **Later — the §4 refactor:** migrate the six raw-thread raytracer sections to the pool, delete the
   `#ifdef`s, prefer the pool everywhere. Upstream it.
5. **Track-only:** `PROXY_TO_PTHREAD` (DOM-bound GUI can't leave the main thread) and JSPI (incompatible
   with our main-loop architecture) — see [`../threading` §6–7](../threading/README.md).

## Cross-references

- [`../threading/README.md`](../threading/README.md) — the 3-layer model, deadlock mechanics, 3 failure
  modes, the full raw-thread inventory, and the upstream pool-migration analysis.
- [`../async/11-asyncify-nesting-raytracer.md`](../async/11-asyncify-nesting-raytracer.md),
  [`../async/12`](../async/12-design-b-asyncify-implementation-plan.md),
  [`../async/13`](../async/13-design-b-engineering-spec.md) — Asyncify nesting + the Design B scheduler.
- `tests/e2e/{coroutine-pthread,threadpool,coroutine-raytrace}.spec.ts`,
  `tests/apps/standalone/{coroutine-pthread,threadpool,raytrace-threads}/` — the apps/tests above.
