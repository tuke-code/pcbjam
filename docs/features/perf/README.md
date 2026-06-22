# Cross-browser performance: why Firefox > Chrome > Safari, and how to close the gap

> Research notes, **2026-06-18**. The KiCad WASM port runs fastest in Firefox,
> slower in Chrome, slowest in Safari. This document explains *why* at the
> browser-engine level and lays out a ranked, build-specific plan to speed up
> Chrome and Safari. Web claims are dated and linked in [Sources](#sources);
> codebase claims carry `file:line` refs. Companion work lives in
> [`../async/`](../async/) (Asyncify) and [`../wasm-exceptions/`](../wasm-exceptions/)
> (the `-fwasm-exceptions` migration).

---

## TL;DR

The Firefox lead is **not** a Firefox trick. Our binary is dominated by
**Asyncify** instrumentation, and Firefox's compilers simply tolerate Asyncify's
pathological code far better than Chrome's or Safari's do. So the highest-leverage
work for Chrome *and* Safari is to **shrink/attack the Asyncify footprint**, plus a
handful of cheap, orthogonal wins.

There are **two independent axes**, and both need attention:

1. **WASM compile/execute** — Asyncify-dominated. This explains the
   Firefox > Chrome > Safari **ordering**.
2. **WebGL rendering** — Safari's Metal/ANGLE overhead. This is *extra* Safari
   slowness on top of axis 1, and several fixes are one-liners.

### Ranked levers

| # | Lever | Axis | Effort | Impact | Where |
|---|---|---|---|---|---|
| 1 | Confirm/force `instantiateStreaming` + `Content-Type: application/wasm` + stable URL/ETag | startup | hours | ~1.5–1.8× cold start (FF); arms V8 cache | `web/standalone/src/wasm/boot.ts` |
| 2 | **Brotli** instead of gzip-9 on R2 | startup | hours | ~15–25% smaller transfer | R2 / edge config |
| 3 | `powerPreference: 'high-performance'` + context-lost handlers | WebGL (Safari/Chrome) | hours | discrete GPU instead of integrated | `wxwidgets/src/wasm/glcanvas.cpp:524-535` |
| 4 | Audit GAL shaders for the `flat` qualifier | WebGL (Safari) | hours–days | up to *seconds/frame* in worst case | `kicad/common/gal/shaders/` |
| 5 | Remove `glGetError()` from the render loop | WebGL (Safari) | hours | avoids per-call Metal flush | GAL compositor |
| 6 | Test `antialias: false` | WebGL (Safari) | hours | cuts MSAA resolve cost | `glcanvas.cpp:524-535` |
| 7 | Enable `-msimd128` | WASM exec (all) | days | 1.5–2.5× geometry/render hot loops | build flags |
| 8 | `ASYNCIFY_ADVISE` → `ASYNCIFY_IGNORE_INDIRECT` + extend `REMOVE` | WASM exec (all, esp. Chrome/Safari) | days | smaller binary + faster tier-up | `scripts/common/apply-asyncify.sh` |
| 9 | `-fwasm-exceptions` (size) | WASM (all) | weeks | 64.5 → 36 MB gz | tracked — see [§ Structural bets](#structural-bets-track--prototype) |
| 10 | JSPI (delete Asyncify) | WASM (all, esp. Safari) | weeks | ~40–50% smaller, removes JIT pressure | tracked — see [§ Structural bets](#structural-bets-track--prototype) |
| — | wasm-split, WebGPU GAL backend | startup / WebGL | weeks+ | deferred (see [§ Deferred](#deferred--not-now)) | — |

---

## Current build (the baseline)

Verified from the build scripts and runtime glue:

| Knob | Value | Location |
|---|---|---|
| Asyncify | `-sASYNCIFY=1`, `ASYNCIFY_STACK_SIZE=65536` | `scripts/kicad/build-kicad-target.sh:~400` |
| Exceptions | **legacy `-fexceptions`** (not `-fwasm-exceptions`) | `build-kicad-target.sh:240-255` |
| SIMD | **none** (`-msimd128` absent) | — |
| Threads | `-sUSE_PTHREADS=1`, pool = `navigator.hardwareConcurrency` (+ COOP/COEP) | `build-kicad-target.sh`, `web/.../preflight/capabilities.ts` |
| Memory | `INITIAL_MEMORY=256MB`, `MAXIMUM_MEMORY=4GB`, `ALLOW_MEMORY_GROWTH=1` | `build-kicad-target.sh` |
| Opt | clang `-O2` (release); link `-O0` then **host `wasm-opt -O2` after `--asyncify`** | `apply-asyncify.sh:88-157` |
| WebGL | WebGL2 (`-sMAX_WEBGL_VERSION=2`), `antialias:true`, **`powerPreference:DEFAULT`** | `glcanvas.cpp:524-535` |
| Loading | Emscripten script-glue; **streaming not confirmed**; gzip-9, **no Brotli** | `boot.ts:145-294` |
| Artifact | pcbnew **186 MB raw / 64 MB gzip**; eeschema 99/34; pl_editor 52/17; gerbview 49/16 | `output/` |

Note: all three modern browsers support `SharedArrayBuffer`/threads under COOP+COEP
(the app demonstrably runs in each) — capability gating is in
`capabilities.ts`, not UA sniffing.

---

## Why the ordering exists (engine internals)

### The villain: Asyncify

Asyncify rewrites every instrumented function with unwind/rewind state checks and
saves/restores all locals to linear memory. That expands each local's live range
across the *whole* function, producing a nearly fully-connected interference graph
— exactly the input that is catastrophic for optimizing register allocators.
Asyncify's own docs warn: *"VMs may also limit compilation to the baseline tier on
such pathological code."* Result: ~+70% binary, giant functions, and the
186 MB-raw pcbnew. See [`../async/02-asyncify-internals.md`](../async/02-asyncify-internals.md).

### How each engine copes

| Engine | Baseline tier | Optimizing tier | On Asyncify's giant functions |
|---|---|---|---|
| **Firefox / SpiderMonkey** | Rabaldr, **~25 ns/byte**, eager whole-module, multithreaded (30–60 MB/s) | **Ion** — [75× large-function fix, Oct 2024](https://spidermonkey.dev/blog/2024/10/16/75x-faster-optimizing-the-ion-compiler-backend.html) (sorted live ranges, Semi-NCA dominators, sparse bitsets) targeting *exactly* the huge-CFG/high-vreg shape Asyncify creates (ONNX: 5 min → 3.9 s) | **Best.** Whole module baseline-compiled before download finishes; Ion swallows the big functions. No OSR gap. |
| **Chrome / V8** | Liftoff, **~50 ns/byte** (½ Firefox) | **TurboFan** — chokes on huge fns (a 1.96 MB fn → 95 s, 7.4 GB RAM, 87% in regalloc); falls back to mid-tier allocator or **skips optimization** | **Middle.** **V8 has no OSR for wasm** — a function in a long loop (Asyncify rewind/unwind loops!) finishes that whole call in Liftoff; only the *next* call gets TurboFan. |
| **Safari / JSC** | **Lazy everything**: IPInt (interpreter) → BBQ → OMG. Nothing eager. | **OMG** (B3) — did *not* get Ion's 2024 large-fn treatment | **Worst.** First run executes at interpreter speed; Asyncify ~doubles fn count → huge OMG backlog → documented **300–400% CPU spike for 30 s+** after a workload. **No persistent compiled-code cache**, so it re-pays every session; above ~10 MB it switches to a slower JIT mode. |

### Two corollaries that bite us specifically

- **Chrome's V8 wasm code cache is effectively unavailable.** It only caches
  modules under ~150 MB *compiled*, and compiled code is 5–7× the `.wasm`. Our
  186 MB pcbnew → ~1 GB compiled — far over the ceiling. So Chrome **re-runs
  TurboFan on every cold load** today. Shrinking the binary (levers 7–10) is the
  only way to get Chrome's repeat-load cache back. See
  [V8 wasm code caching](https://v8.dev/blog/wasm-code-caching).
- **Benchmark trap:** with DevTools open, V8 tiers all wasm *down* to Liftoff.
  Never measure Chrome speed with DevTools open (except via an actual Performance
  recording, which forces tier-up). This likely makes Chrome look worse than it is
  in casual testing.

---

## The ranked plan

### Tier 1 — cheap, do now (days, low risk)

**1. Confirm + force streaming instantiation and cache headers.** The loader
injects the Emscripten JS glue via `<script>` (`boot.ts`); whether the *runtime*
then streams the `.wasm` depends on serving conditions. In DevTools → Network,
confirm the `.wasm` returns **`Content-Type: application/wasm`** with no console
"falling back to ArrayBuffer instantiation" warning. Streaming is ~1.5–1.8× faster
cold-start on Firefox and is the *only* path that arms V8's code cache. Serve the
`.wasm` from a **stable URL** (no content-hash in the path; use a stable alias)
with `ETag`/`304`. With `-pthread`, confirm the module is compiled once and shared
to workers (Emscripten does this via the shared `WebAssembly.Module`), not
recompiled per worker.

**2. Brotli instead of gzip-9 on R2.** Brotli is ~15–25% smaller on wasm (our
64 MB pcbnew → ~50 MB). Verify Cloudflare actually Brotli-compresses it at the
edge — it often *skips* large binary types — and if not, precompress and serve
with `Content-Encoding: br` + `Content-Type: application/wasm`. Smaller transfer
also means less to compile, so it compounds with everything below.

**3. `powerPreference: 'high-performance'` for WebGL.** We default to
`EM_WEBGL_POWER_PREFERENCE_DEFAULT` (`glcanvas.cpp:524-535`). **Safari (and Chrome
on dual-GPU Macs) defaults WebGL to the integrated GPU.** Requesting
high-performance switches to the discrete GPU — often the single biggest
GPU-bound framerate win on MacBook Pros. Caveat: Safari only honors it if you also
register `webglcontextlost`/`webglcontextrestored` handlers.

**4. Audit GAL shaders for the `flat` interpolation qualifier.** This is the big
Safari sleeper. `flat` triggers a provoking-vertex workaround in Safari's
Metal/ANGLE backend that has cost real apps *seconds per frame*. PCB renderers
commonly use `flat` for per-primitive net/layer colors. Grep the GAL shaders
(`kicad/common/gal/shaders/`, source GLSL 1.20 before `convert_glsl_es3.py`); if
present, replace with regular interpolation or restructure. Potentially a massive
Safari-only win.

**5. Remove `glGetError()` from the render loop.** On Safari each call forces a
Metal pipeline flush. Restrict to init/debug builds only. (Note: the WebGL
compositor already drains stale `glGetError()` once before draws — that's fine;
the concern is *per-call* error checks inside the hot path.)

**6. Test `antialias: false`.** We default MSAA on (`antialias:true`). KiCad's GAL
does much of its line AA in-shader (SMAA) and has its own AA setting; if MSAA is
redundant, dropping it cuts Metal's resolve cost on Safari. Quality/perf tradeoff —
A/B it on a dense board; consider exposing it as a setting.

### Tier 2 — medium effort, high impact

**7. Enable `-msimd128`.** Expect **1.5–2.5×** on the geometry/render hot loops
(polygon booleans in `shape_poly_set`, DRC overlap checks, vertex-buffer fills) via
LLVM autovectorization at `-O2`+. Safe on all three engines (Chrome 91 / FF 89 /
Safari 16.4). Helps absolute Chrome *and* Safari speed. Verify `v128.*` actually
appears in the disassembly for the hot functions, and prefer `pmin`/`pmax` over
min/max (the SSE→wasm emulation table has slow paths). **Do not** ship Relaxed SIMD
yet (Safari still flags it). Minor interaction to watch: SIMD slightly grows
per-function size, which feeds the Asyncify/locals pressure — measure after the
`wasm-opt -O2` pass.

**8. Shrink the Asyncify surface.** This directly attacks the root cause for Chrome
and Safari. Run **`ASYNCIFY_ADVISE`** to see which functions get instrumented and
why — it surfaces the biggest instrumented functions (the JIT pressure points).
Then:
   - **`ASYNCIFY_IGNORE_INDIRECT=1`** is the high-impact one: our wxWidgets/GAL
     code is vtable-heavy, and Asyncify conservatively instruments *every* indirect
     call site, which is why instrumentation spreads everywhere. With our
     understanding of the suspend paths (the park-throw work), we may be able to
     assert no indirect call is on the suspend stack and add specific ones back via
     `ASYNCIFY_ADD`.
   - Extend the existing 12-function `ASYNCIFY_REMOVE` list (`apply-asyncify.sh:92-104`)
     with cold/startup-only large functions ADVISE flags.
   - Smaller instrumented set → smaller functions → better tier-up in *all* engines
     and a smaller Safari OMG backlog.
   - ⚠️ Error-prone (wrong config = silent runtime breakage). Gate behind the
     red/green Asyncify harness in [`../asyncify-arbiter/redgreen.md`](../asyncify-arbiter/redgreen.md)
     / `tests/asyncify/`.

### Structural bets (track / prototype)

**9. `-fwasm-exceptions`** (we're on legacy `-fexceptions`). Biggest *size* lever —
[`../wasm-exceptions/02-measurements.md`](../wasm-exceptions/02-measurements.md)
puts pcbnew at **64.5 → 36 MB gzip**, which would also start bringing Chrome back
under the code-cache ceiling and fix the unreliable catch/destructor landing-pad
behavior we've documented. **Two blockers to track before committing:**
   - a **Safari 26.0 startup regression** for `-fwasm-exceptions` modules
     ([emscripten #25365](https://github.com/emscripten-core/emscripten/issues/25365))
     — verify whether it's fixed in a 26.x / Safari 27 beta before shipping, since
     Safari is the browser we're trying to help;
   - the asyncify-EH unwind-from-catch interaction
     ([`../wasm-exceptions/05-asyncify-fork-design.md`](../wasm-exceptions/05-asyncify-fork-design.md)).

**10. JSPI** — the eventual *real* fix for Safari, because it deletes Asyncify
entirely (no instrumentation → no giant functions → no OMG backlog → the
300–400% Safari spike goes away) and cuts ~40–50% of binary size. Status:

| Engine | JSPI status |
|---|---|
| Chrome / V8 | **shipped, Chrome 137** (May 2025) |
| Firefox / SpiderMonkey | **Firefox 153** intent-to-ship (June 2026); Nightly now, stable ~late summer/fall 2026 |
| Safari / JSC | **Safari 27 beta** (WWDC26), enabled by default; stable Fall 2026 |

Don't migrate wholesale yet:
   - unresolved **~350× regression on the `JS→C→JS` re-entry pattern**
     ([emscripten #21081](https://github.com/emscripten-core/emscripten/issues/21081))
     — exactly what a GUI event loop hits constantly;
   - static-init `SuspendError`
     ([emscripten #24302](https://github.com/emscripten-core/emscripten/issues/24302));
   - `invoke_*` over-tagging under our legacy exceptions.

   **Recommended path:** prototype JSPI on a *small* tool (calculator or pl_editor)
   behind feature detection (`'Suspending' in WebAssembly`) with Asyncify fallback,
   profile the re-entry pattern, and watch #21081. By the time it's safe, all three
   engines will support it.

### Deferred / not now

- **`wasm-split` / `-sSPLIT_MODULE`** — the secondary module can't be loaded lazily
  *and* asynchronously on the main thread, which is incompatible with our
  main-thread Asyncify model unless we move to `-sPROXY_TO_PTHREAD`.
- **WebGPU GAL backend** — the structural exit from Safari's Metal/ANGLE overhead
  (Safari 26 ships WebGPU), but it's a multi-week GLSL→WGSL port with no upstream
  KiCad support. See the GAL history in [`../archive/webgl/`](../archive/webgl/).
- **Global `-O3`** — its inlining bloats the binary and makes the Chrome
  cache/compile problem *worse*. If anything, compile cold/utility units at `-Os`.

---

## What to do first

Quick, visible, near-zero-risk wins this week:

- **Safari:** #3 (high-performance GPU) + #4 (`flat` audit) + #5 (`glGetError`).
- **Startup everywhere:** #2 (Brotli) + #1 (streaming/headers).
- **Runtime everywhere:** #7 (`-msimd128`).

Then invest in **#8 (Asyncify ADVISE + IGNORE_INDIRECT)** as the real lever against
the Chrome/Safari gap, and keep **#9 / #10** on a tracking list.

---

## Sources

**Engine internals**
- [75× faster: optimizing the Ion compiler backend — SpiderMonkey, Oct 2024](https://spidermonkey.dev/blog/2024/10/16/75x-faster-optimizing-the-ion-compiler-backend.html)
- [Understanding WebAssembly code generation throughput — wingolog, 2020](https://wingolog.org/archives/2020/04/14/understanding-webassembly-code-generation-throughput)
- [V8 WebAssembly compilation pipeline](https://v8.dev/docs/wasm-compilation-pipeline) · [Dynamic tiering](https://v8.dev/blog/wasm-dynamic-tiering) · [Liftoff](https://v8.dev/blog/liftoff)
- [Code caching for WebAssembly developers — V8](https://v8.dev/blog/wasm-code-caching)
- [Introducing the JetStream 3 Benchmark Suite — WebKit, 2024](https://webkit.org/blog/17899/introducing-the-jetstream-3-benchmark-suite/) (IPInt/BBQ/OMG)
- [Pause and Resume WebAssembly with Binaryen's Asyncify — kripken, 2019](https://kripken.github.io/blog/wasm/2019/07/16/asyncify.html)

**Startup / size / SIMD**
- [Optimizing WebAssembly Startup Time — Nutrient](https://www.nutrient.io/blog/optimize-webassembly-startup-performance/)
- [MDN: WebAssembly.instantiateStreaming](https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/JavaScript_interface/instantiateStreaming_static)
- [Module Splitting — Emscripten](https://emscripten.org/docs/optimizing/Module-Splitting.html)
- [Using SIMD with WebAssembly — Emscripten](https://emscripten.org/docs/porting/simd.html) · [V8 SIMD](https://v8.dev/features/simd) · [caniuse wasm-simd](https://caniuse.com/wasm-simd)
- [Asynchronous Code (Asyncify settings) — Emscripten](https://emscripten.org/docs/porting/asyncify.html)

**Safari WASM + WebGL**
- [WebGL Performance on Safari & Apple Vision Pro — Wonderland Engine](https://wonderlandengine.com/news/webgl-performance-safari-apple-vision-pro/) (`flat`, UBO timing)
- [WebKit features in Safari 26.0](https://webkit.org/blog/17333/webkit-features-in-safari-26-0/) · [News from WWDC26 — Safari 27 beta](https://webkit.org/blog/17967/news-from-wwdc26-webkit-in-safari-27-beta/)
- [emscripten #25365 — Safari 26.0 wasm-exceptions regression](https://github.com/emscripten-core/emscripten/issues/25365)
- [emscripten #26027 — Safari + Asyncify + unaligned-load leak](https://github.com/emscripten-core/emscripten/issues/26027)
- [ONNX Runtime #26827 — Safari WebKit 26 OMG CPU loop](https://github.com/microsoft/onnxruntime/issues/26827)

**JSPI**
- [V8: Introducing the WebAssembly JavaScript Promise Integration API](https://v8.dev/blog/jspi) · [new API](https://v8.dev/blog/jspi-newapi)
- [caniuse: JSPI](https://caniuse.com/wf-wasm-jspi) · [Chrome 137 release notes](https://developer.chrome.com/release-notes/137)
- [Mozilla dev-platform: Intent to Ship JSPI (Fx153), June 2026](http://www.mail-archive.com/dev-platform@mozilla.org/msg01810.html)
- [emscripten #21081 — JSPI 350× slower for JS→C→JS](https://github.com/emscripten-core/emscripten/issues/21081) · [#24302 — JSPI static-init SuspendError](https://github.com/emscripten-core/emscripten/issues/24302)

### Local cross-references
- [`../async/`](../async/) — Asyncify internals, single-slot contention, park-throw.
- [`../wasm-exceptions/`](../wasm-exceptions/) — `-fwasm-exceptions` measurements, toolchain status, asyncify-EH fork design.
- [`../asyncify-arbiter/redgreen.md`](../asyncify-arbiter/redgreen.md) — the harness to gate Asyncify-surface changes.
- [`../archive/webgl/`](../archive/webgl/) — GAL → WebGL2 history (context attrs, compositor, shaders).
- Build: `scripts/kicad/build-kicad-target.sh`, `scripts/common/apply-asyncify.sh`.
- WebGL context: `wxwidgets/src/wasm/glcanvas.cpp:489-535`. Loader: `web/standalone/src/wasm/boot.ts`.
