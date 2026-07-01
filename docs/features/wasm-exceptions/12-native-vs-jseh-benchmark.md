# Benchmark: native-EH vs JS-EH (eeschema + pcbnew)

Measured 2026-07-01. Quantifies the WASM exception-handling migration
(`c1ef489`, native `-fwasm-exceptions`) vs its parent (`b8c8dee`, legacy `-fexceptions`) for
**eeschema** and **pcbnew**, across the post-link `wasm-opt` level (`-O1` vs `-O2`), in Chrome +
Firefox, **headless and headed**.

## TL;DR

- **native-EH ships much less to download** — eeschema.wasm gzip **28.9 vs 37.2 MB (−22%)**;
  pcbnew.wasm gzip **52.4 vs 70.6 MB (−26%, −18 MB)**. pcbnew (bigger, far more exception-heavy) shows
  the *stronger* win, in both % and absolute MB.
- **native-EH loads ~30–35% faster** (Chrome cold) and **opens documents 20–35% faster**, with the
  gap **widening under CPU throttle** — i.e. it genuinely executes less CPU per operation.
- **native-EH sustains higher interaction FPS** under throttle (clearest in headed/real-GPU runs).
- **Ship post-link `-O1`, not `-O2`.** `-O1` is the smaller *download* (it compresses better),
  **~4–15× faster to build**, and faster to load. `-O2` even has a pathology: **native pcbnew `-O2`
  takes ~11 s to load in Firefox** (vs 3.8 s for `-O1`). `-O2` buys nothing that matters.
- **headed ≠ headless** and both are informative: headed = real GPU/compositor (FPS pinned near the
  120 Hz cap, throttle-resilient); headless = CPU-bound (FPS collapses under throttle, exposing raw
  render cost). Load/open are ~mode-independent.

## Setup & methodology

| | native-EH | JS-EH |
|---|---|---|
| commit | `c1ef489` (the migration) | `b8c8dee` (its parent) |
| C++ exceptions | `-fwasm-exceptions` (legacy encoding) | `-fexceptions` (JS invoke + emscripten SjLj) |
| kicad submodule | `032540ab` | `032540ab` (**identical**) |
| wxWidgets | `67f28fb3` | `cca8eed9` |
| emsdk | 4.0.2 | 4.0.2 |
| Binaryen (asyncify + shrink) | **v130** | **v130** (forced; see caveats) |

- **Clean, cold, from-scratch builds** (per-version Docker Compose project → empty build-cache
  volume → deps incl. OpenCASCADE rebuilt; ccache verified 0% on the first build). Isolated git
  worktree, branch per submodule, main checkout untouched.
- **Debug builds (`-O1` compile).** Release (`-O2` compile) can't be built on this 64 GB machine —
  after asyncify a few functions carry tens of thousands of spilled locals and the mandatory
  `wasm-opt` CoalesceLocals shrink OOMs >64 GB even at 1 core (CI uses a 128 GB box). Debug is what
  the product ships anyway, so it's the representative artifact. Both sides carry `-DDEBUG`
  identically, so the native-vs-js deltas are valid; absolute FPS is lower than a release build.
- **pcbnew built with `BUILD_3D_VIEWER=ON` on both sides** (real CPU raytracer linked). The migration
  commit flips the 3D viewer on by default; holding it ON for both isolates the EH variable while
  testing the real, 3D-capable pcbnew. eeschema has no 3D viewer.
- **O1/O2 axis = post-link `wasm-opt` level** on the same opt-independent asyncified base. Compile is
  `-O1` (debug) for all cells.
- **Machine**: Apple-Silicon Mac, 64 GB. Docker 10 CPU. Core counts identical across versions:
  container compile `-j10`; asyncify `BINARYEN_CORES=8`; shrink `BINARYEN_CORES=2` (RAM ceiling —
  the build container is stopped for the host post-process; measured shrink peaks: e.g. js-pcb-O1 = 32 GB).
- **Runtime**: headless + headed Chromium (CDP CPU throttling 1/4/6×) and Firefox (load only —
  throttling is Chromium-only), served locally with COOP/COEP. Cold load = navigation → fully-booted
  editor (visible `#canvas` + populated wx registry + editor Frame + `kicadOpenFile` + GL canvas),
  median of 5 fresh contexts (all cells 5/5 OK). Interaction workload = the bundled `demo.kicad_sch` /
  `demo.kicad_pcb`. Input driven on `#canvas` (the emscripten input surface).

## Build time (seconds)

| stage (cores) | ee-native | ee-js | pcb-native | pcb-js |
|---|---|---|---|---|
| compile | 654 | 677 | 365¹ | 384¹ |
| asyncify (v130, c8) | 14 | 24 | 75 | 65 |
| shrink **-O1** (c2) | **191** | **292** | **198** | **421** |
| shrink -O2 (c2) | 840 | 961 | **3082** | **3295** |

¹ pcbnew reused the (identical) deps; native = app-only, js includes a ccache-fast wx rebuild I had
to force (a `rsync --delete` wx-source quirk removed a generated pcre table). native-EH is faster at
every stage (smaller module). **`-O2` shrink is 4× (eeschema) to ~15× (pcbnew) slower than `-O1`.**

## Bundle size — app.wasm (decimal MB, = 10⁶ bytes)

| | ee-native-O1 | ee-native-O2 | ee-js-O1 | ee-js-O2 | pcb-native-O1 | pcb-native-O2 | pcb-js-O1 | pcb-js-O2 |
|---|---|---|---|---|---|---|---|---|
| raw | 85.6 | 85.6 | 110.1 | 109.2 | 153.5 | 153.1 | 208.8 | 206.8 |
| **gzip** | **28.9** | 30.0 | **37.2** | 37.8 | **52.4** | 53.9 | **70.6** | 71.4 |
| brotli | 15.4 | 16.6 | 19.6 | 20.2 | 28.3 | 30.0 | 37.4 | 38.4 |

- **native-EH vs JS-EH (gzip)**: eeschema −22% (−8.3 MB), **pcbnew −26% (−18.2 MB)**. JS-EH's
  `invoke_*` trampolines + SjLj + heavier asyncify instrumentation are the cost (asyncified module:
  ee 146 vs 190 MB; **pcb 250 vs 357 MB**).
- **`-O1` vs `-O2`**: `-O2` yields a marginally smaller *raw* wasm but a **larger compressed** one
  (it cuts instructions but raises byte entropy). Since downloads are compressed, **`-O1` is the
  smaller download** everywhere.

## Runtime — cold load (median of 5, ms)

Headless shown (headed within ~3%). **↓ is better.**

| | Chrome | Firefox |
|---|---|---|
| ee-native-O1 | **1182** | **2656** |
| ee-js-O1 | 1796 | 3172 |
| ee-native-O2 | 1505 | 2622 |
| ee-js-O2 | 2007 | 3098 |
| pcb-native-O1 | **1689** | **3796** |
| pcb-js-O1 | 2501 | 4531 |
| pcb-native-O2 | 1998 | **11141 ⚠** |
| pcb-js-O2 | 3060 | 4516 |

- **native-EH loads ~30–35% faster on Chrome** (ee 1182 vs 1796; pcb 1689 vs 2501), ~16–20% on Firefox.
- **⚠ `-O2` Firefox pathology**: native-pcb-`-O2` loads **~11 s** in Firefox (all 5 samples
  10.8–11.3 s) vs **3.8 s** for `-O1` — a ~3× penalty. Firefox's wasm compiler chokes on the large
  `-O2` output; Chrome is unaffected (2.0 s). Another concrete reason to ship `-O1`.
- `-O1` loads faster than `-O2` across the board.

## Runtime — open+render a document under CPU throttle (Chrome, openMs)

Time to open+process the demo doc (parse+build+render through the EH/asyncify paths). Headless, ms @1×/4×/6×:

| | 1× | 4× | 6× |
|---|---|---|---|
| ee-native-O1 | 695 | 1526 | 2085 |
| ee-js-O1 | 878 | 2229 | 3191 |
| pcb-native-O1 | 889 | 1950 | 2713 |
| pcb-js-O1 | 1024 | 2585 | 3570 |

**native-EH is 22% faster at 1× and ~24–35% faster at 6×** — the advantage *widens* with throttle,
proving less CPU per operation (not just a smaller module). O1 vs O2 open times are near-identical
(ee-native-O2 743/1643/2299; pcb-native-O2 904/2041/2846).

## Runtime — sustained pan/zoom FPS (Chrome, @1×/4×/6×)

Real-input pan/zoom, frames/sec. **Headed = real GPU/compositor; headless = CPU-bound.**

| | headed 1/4/6× | headless 1/4/6× |
|---|---|---|
| ee-native-O1 | 120 / 107 / **84** | 102 / 58 / 39 |
| ee-js-O1 | 120 / 90 / **69** | 101 / 47 / 34 |
| pcb-native-O1 | 120 / 115 / **84** | 82 / 15 / 10 |
| pcb-js-O1 | 120 / 103 / **77** | 81 / 45 / 11 |

- **Headed** (the meaningful GPU number): native-EH holds higher FPS under throttle — ee 84 vs 69 at
  6× (+22%), pcb 84 vs 77 (+9%). At 1× everything pins to the ~120 Hz display cap (GPU does the work
  off the throttled CPU thread).
- **Headless** is CPU-bound: FPS collapses under throttle, and for the big pcbnew module the numbers
  get noisy (10–50 fps) — treat headless FPS as a rough CPU-render indicator and `openMs` as the
  clean compute metric. Note even at 1× pcbnew headless is ~81 fps (below cap) — its board render is
  genuinely heavier than a schematic.

## Verdict

1. **The native-EH migration is a clear, broad win** — smaller download (−22% ee, **−26% pcb**),
   faster load (~30–35%), faster document open (widening under load), higher sustained FPS, and
   faster builds. Nothing regressed; every cell boots 5/5. pcbnew, the harder app, wins bigger.
2. **Ship post-link `-O1`.** Smaller *download*, 4–15× faster to build, faster to load (and no
   `-O2` Firefox-compile pathology), for equal runtime. `-O2`'s smaller-raw-wasm is a mirage once gzipped.

## Caveats

- **Debug, not release** (release OOMs on 64 GB). Absolute FPS carries `-DDEBUG` overhead, but the
  native-vs-js and O1/O2 deltas are valid, and debug is what ships.
- **Binaryen v130 forced on JS-EH** (as-shipped it'd use emsdk v121, whose `-O2` is ~9× slower) to
  isolate the EH variable from the toolchain. The v130 bump actually shipped *with* the migration.
- **wxWidgets differs** (part of the migration); **kicad is byte-identical**. pcbnew held at 3D-ON on
  both; the as-shipped native additionally defaults 3D on (already included here for both).
- Shrink ran at `cores=2` for both (RAM ceiling); absolute shrink times would drop at more cores, but
  the comparisons are at matched settings.

## Reproduce

Throwaway worktree (`bench/native-eh` @ `c1ef489`, `bench/jseh` @ `b8c8dee`), per-version Docker
projects for clean isolated deps. Per cell: `docker/build.sh <app> --debug --full/--clean-kicad
--compile-only` (pcbnew adds `BUILD_3D_VIEWER=ON`) → `ASYNCIFY_ONLY=1 BINARYEN_CORES=8 …
--postprocess-only` (js-EH adds `BINARYEN_VERSION=130`) → `wasm-opt -O{1,2} [-all]` at
`BINARYEN_CORES=2`. Perf via a headless/headed Playwright harness (COOP/COEP server, CDP CPU throttling).
