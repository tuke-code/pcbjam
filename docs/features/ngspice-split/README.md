# ngspice-split — the eeschema simulator as a lazy worker service

> Status: implemented (2026-07-17). The SPICE analog of the [occ split](../occ-split/):
> ngspice's sharedspice engine runs in a dedicated Web Worker module
> (`ngspice_service.wasm`, ~5.4 MB, `-O2`), the editor binds a statically linked
> RPC client, and Inspect → Simulator works with **full native parity** —
> XSPICE (all seven bundled code models), CIDER, the complete BSIM4/B3SOI/
> B4SOI/HSIM parameter tables, and real background-run semantics (`bg_halt`
> interrupts a running simulation).

## Why a worker service

- **No dlopen in wasm.** Native KiCad dlopens libngspice and resolves ~10
  symbols (`eeschema/sim/ngspice.cpp init_dll`). A static emscripten build has
  no dynamic linking, and the occ-split analysis rates MAIN_MODULE dynamic
  linking RED for the editor (wasm-EH + pthreads + asyncify).
- **Crash isolation.** KiCad's native crash recovery installs SIGSEGV/SIGABRT/
  SIGFPE handlers around the simulation thread — emscripten stubs these
  (emscripten#8567, wontfix). In-process, a hard ngspice fault kills the tab;
  in a worker, the provider fails in-flight requests, KiCad's `m_error` →
  `NGSPICE::validate()` path re-inits, and a **fresh worker** boots.
- **The editor stays lean.** kicad_editor.wasm carries zero ngspice; the 5.4 MB
  service is fetched on the first simulator open (lazy boundary asserted in
  e2e).
- **KiCad's usage is RPC-friendly.** It passes nullptr for the streaming
  SendData/SendInitData callbacks and pulls vectors after (or during) the run
  via `ngGet_Vec_Info`; only console/status text and run-state transitions
  stream mid-run.

## Architecture

```
kicad_editor (eeschema, ASYNCIFY=1)                ngspice_service (ASYNCIFY=0, pthreads)
┌────────────────────────────────────┐             ┌──────────────────────────────────┐
│ NGSPICE (upstream; ONE ifdef in    │ postMessage │ ngspice_service_main.cpp (embind)│
│  init_dll binds pcbjam_ngSpice_*)  │◄───────────►│ libngspice.a: sharedspice static,│
│ wasm/stubs/sharedspice_client.cpp: │             │  XSPICE static registry, CIDER   │
│  EM_ASYNC_JS request suspend,      │  {evt}      │ callbacks → MAIN_THREAD_ASYNC_   │
│  event dispatch (fresh entries),   │◄────────────│  EM_ASM → Module.ngspiceEmit     │
│  vec arena, .include shipper,      │             │ bg_run = real ngspice pthread;   │
│  atomic running mirror             │             │ main thread free → bg_halt works │
└────────────────────────────────────┘             └──────────────────────────────────┘
  provider: web/standalone/src/wasm/ngspice-service.ts (lazy blob worker)
  worker wrapper (shared app/tests): web/standalone/src/wasm/ngspice-worker.js
```

### Pieces

| Piece | Where |
|---|---|
| dep build (sharedspice static + code models) | `scripts/deps/build-ngspice.sh` (ngspice 46) |
| static code-model registry sources | `scripts/deps/ngspice-wasm/{ngcm_registry.c,ngcm_dlmain_static.c}` |
| Gate-1 node smoke (rc/xspice/cider/halt) | `scripts/deps/ngspice-wasm/smoke/` |
| service module | `wasm/ngspice-service/{CMakeLists.txt,ngspice_service_main.cpp}` |
| service build (standalone emcmake, NOT the kicad tree) | `scripts/kicad/build-ngspice_service.sh` |
| editor client stub | `wasm/stubs/sharedspice_client.cpp` + decls in `wasm/stubs/ngspice/sharedspice.h` |
| the single kicad-fork edit | `eeschema/sim/ngspice.cpp` `init_dll()` `#ifdef __EMSCRIPTEN__` block |
| model-data tables restored at `-O2` | `eeschema/CMakeLists.txt` EMSCRIPTEN block |
| app provider / bundle | `web/standalone/src/wasm/{ngspice-service.ts,ngspice-worker.js}`, `constants.ts`, `boot.ts` |
| e2e | `tests/kicad/{ngspice-probe,eeschema-sim}.spec.ts`, harness stub `tests/kicad/utils/ngspice-service.ts` |

## The ngspice dep build (`build-ngspice.sh`)

`--with-ngshared --disable-shared --enable-static` builds `libngspice.a` with
the sharedspice API and no CLI (upstream gates `bin_PROGRAMS` on
`!SHARED_MODULE`). Idempotent (marker-guarded) edits to the extracted tarball,
each load-bearing:

1. **libtool static-mode override.** ngspice hardwires libtool's `-shared`
   mode for the ngshared build (`STATIC=-shared` consumed as AM_CFLAGS
   everywhere, plus literal `-shared` in `libngspice_la_{CFLAGS,LDFLAGS}`);
   libtool hard-errors on `-shared` without shared-lib support. Fixed with
   `make STATIC=-static` (command line beats makefile) + a sed on the two
   generated `src/Makefile` lines.
2. **XSPICE code models without dlopen.** Natively each `.cm` is dlopen'd via
   `load_opus()` (`src/spicelib/devices/dev.c`). Statically: the icm build is
   redirected (env `NGCM_STATIC`/`NGCM_DLMAIN`) to compile per-cm renamed
   tables (`ngcm_<cm>_cmDEVices…`, `ngcm_dlmain_static.c`) and `emar` each
   model into a `.cm` archive; a registry appended to dev.c resolves the seven
   bundled basenames straight into `add_device`/`add_udn`, falling through to
   dlopen (→ ngspice's normal error) for unknown paths. dlmain.c's coreitf
   wrapper section is deliberately dropped (it would shadow real core symbols
   with calls through a never-initialized coreitf); its utility tail
   (`fopen_with_path`, `cm_message_printf`, `cm_is_inertial`) is extracted at
   build time into `ngcm_common.a`, with `cm_getvar` bound directly to the
   core's `cp_getvar` (the dllitf mapping, `cmexport.c`).
3. **cmpp runs on the build host** — automatic in ngspice 46 when
   `cross_compiling=yes` (`src/xspice/cmpp/build/`), BUT the icm makefile's
   `$(shell cmpp -p …)` model-list calls hardcode the CROSS-compiled cmpp:
   patched to `$(CMPP)`. Symptom of the unpatched bug: `.cm` archives quietly
   containing only `dlmain.o` ("Permission denied" from the wasm binary).
4. **verilog/vhdl subdirs skipped** (generated `src/xspice/Makefile` sed):
   ivlng/ivlngvpi VPI co-simulation shims are inherently shared objects
   plugging into an external Icarus/GHDL process — impossible in wasm; the
   d_cosim model fails at runtime exactly like a native install without a
   cosimulator.
5. **Upstream 32-bit bug fixed (TODO: upstream).** CIDER card parsing frees
   through `dataType & IF_REALVEC` — a composite mask (0x8004) that also
   matches scalar `IF_SET|IF_REAL` (0x2004) parameters and then frees the
   vec-pointer union member overlaying the parsed scalar double. On 64-bit
   the misread lands in zero padding (free(NULL)); on wasm32 the pointer
   member overlays the HIGH half of the double → heap fault on e.g.
   `.model … numd … defa=1p`. Patched to exact `IF_VARTYPES` tests.
6. `/proc/meminfo` header check forced off (`ac_cv_header__proc_meminfo=no`):
   configure runs on a Linux build host, the browser has no procfs, and
   ngspice treats "0 bytes available" as OOM.
7. `-pthread` everywhere: without `HAVE_LIBPTHREAD`, `bg_run` silently
   degrades to a synchronous blocking call (sharedspice.c `runc()`).
8. spinit installs to the sysroot and is `--embed-file`'d into the service at
   `/ngspice/scripts/spinit`; `main()` sets `SPICE_LIB_DIR=/ngspice` so
   ngspice's env-first search finds it regardless of the baked build prefix.
   Its `codemodel <prefix>/<cm>.cm` lines resolve by basename in the registry.

Gate 1 (`scripts/deps/ngspice-wasm/smoke/run-smoke.sh`, node): static link,
RC transient numerics, XSPICE gain block through the registry, CIDER numd DC
sweep, and bg_run → mid-run bg_halt → BGThreadRunning(finished). **The
emscripten default 64 KB stack overflows in ngspice's parser** — the smoke
and the service both run `-sSTACK_SIZE=4MB -sDEFAULT_PTHREAD_STACK_SIZE=2MB`
(the unchecked overflow corrupts the heap and detonates later as free()
faults; found via `-sSAFE_HEAP=1`).

## The service module

Pure libngspice + a ~350-line embind shim — **no KiCad/wx code**, so it
builds standalone with emcmake (`build-ngspice_service.sh`, seconds, no
docker) rather than through the kicad CMake tree like occ_service; the
artifact lands in the standard `build-wasm/kicad-ngspice_service/ngspice_service/`
layout so docker copy / test staging / publish treat it like any app.

- RPC surface mirrors sharedspice 1:1: `init/circ/command/getVecInfo/curPlot/
  allPlots/allVecs/running/cmInputPath`.
- **Events**: callbacks fire on ngspice's bg pthread → `strdup` +
  `MAIN_THREAD_ASYNC_EM_ASM` → `Module.ngspiceEmit` (per-target FIFO keeps
  order; the service main thread is idle during a run so it drains promptly);
  the worker wrapper batches char/stat lines per microtask into one `{evt}`
  postMessage (flood guard) and flushes the batch before bg/exit events.
- **bg_halt works mid-run** because the simulation occupies its own pthread —
  the module main thread stays free to service the halt RPC. Never shrink the
  pthread pool (occ lesson: a blocked browser thread cannot spawn workers).
- **Vector reads are copied under `ngSpice_LockRealloc`** inside the service —
  this replaces KiCad's client-side RAII lock (a no-op across RPC; the ifdef
  leaves `m_ngSpice_LockRealloc` null) and makes the UI's mid-run plot
  refresh safe against the growing tran vectors.
- getVecInfo returns heap views; the worker wrapper copies them into fresh
  arrays before postMessage (**a SAB-backed view cannot be transferred**).

## The editor side

- `NGSPICE::init_dll()` gets one `#ifdef __EMSCRIPTEN__` block binding the
  `m_ngSpice_*` pointers to `pcbjam_ngSpice_*` (prefix required: the class-
  scope typedef names would shadow same-named globals inside the member),
  plus a second `#ifndef __EMSCRIPTEN__` guard skipping the client-side
  spinit/codemodel staging. That staging is NOT harmless in wasm: its
  `wxSetWorkingDirectory( exe dir )` always fails in MEMFS (error 44), the
  queued wxLogError then flushes as a MODAL dialog over the freshly opened
  simulator frame, and the modal event pump dies with an asyncify-corruption
  signature ("index out of bounds" / "indirect call to null" — the known
  nested-modal-inside-doRewind limitation documented in
  wxwidgets/src/wasm/dialog.cpp; the pump's cancel-recovery keeps the app
  alive, but the corruption gate in eeschema-sim.spec.ts rightly fails). The
  service embeds its own spinit + code models, so the staging has nothing to
  do here anyway. Still the ONLY kicad-fork source file touched.
- `wasm/stubs/sharedspice_client.cpp`: EM_ASYNC_JS request bridge (suspends
  the editor; `__asyncjs__*` is pre-covered by asyncify-imports.txt — do NOT
  add any of this path to the removelist), a dedicated get_vec bridge that
  mallocs vector doubles straight into the editor heap, a per-call
  `vector_info` arena (every NGSPICE consumer copies within the same call),
  an atomic `ngSpice_running` mirror (the UI polls on a timer; zero RPC per
  poll), and the **`.include`/`.lib` shipper**: `NETLIST_EXPORTER_SPICE`
  emits absolute `.include` paths (Sim.Library models, the IBIS cache) that
  ngspice must open in ITS filesystem — the stub scans the deck recursively
  (depth ≤ 4), reads the files from editor MEMFS and ships `{path,text}`
  pairs for the service to stage at identical paths.
- **Events into a suspended editor**: the provider hands `{evt}` frames to
  `globalThis.__ngspiceOnEvent` (installed by the stub at first init), which
  calls the exported `pcbjam_ngspice_event` — a fresh wasm entry from JS,
  the same mechanism every wx-dom DOM event uses while the main loop is
  asyncify-suspended. KiCad's callbacks only take a mutex + `wxQueueEvent`,
  so nothing on the path can suspend.
- The four giant model-data files (bsim4/b3soi/b4soi/hsim) are restored with
  per-source `-O2` (the "too many locals" limit was an -O0 artifact; same
  workaround as the msys block in eeschema/CMakeLists.txt) — full parameter
  tables in the model dialogs.

## Traps (learned the hard way)

- **`MIF*/` inside a C block comment terminates it** — the registry's early
  drafts broke the build with prose. (clangd flagged it; the diagnostics were
  right.)
- The dev.c registry hook must be inserted BEFORE the registry body is
  appended, and its idempotency guard must match the CALL (`ngcm_static_load(name)`),
  not the name — the appended definition otherwise masks the hook forever.
- ngspice's `BGThreadRunning` callback argument is *"not running"* (true =
  finished) — KiCad treats it as `aFinished`; keep the polarity.
- The `running` mirror flips true at `bg_run` ACCEPTANCE (not at the bg
  'started' event) so an immediate `IsRunning()` poll already sees it.
- Emscripten's `HEAPF64.set` after `_malloc` inside EM_ASYNC_JS is safe under
  memory growth (the views are refreshed), but always re-read the global
  after allocating.
- sharedspice error recovery longjmps (`errbufm`/`errbufc`) — fine under the
  tree-wide wasm-SjLj model and an ASYNCIFY=0 module; never let it meet an
  asyncify-instrumented stack.

## Known limitations (parity-consistent)

- User-compiled `.cm` code models and `.osdi` (OpenVAF) binaries cannot load —
  wasm can't dlopen user binaries. ngspice reports them with its native error
  text. (Native parity minus the ability to install binary plugins.)
- Verilog/GHDL co-simulation (`d_cosim`) needs an external simulator process —
  same failure text as a native install without Icarus.
- OpenMP is off (unsupported in emscripten) — BSIM model evaluation runs
  single-threaded per timestep; the simulation itself still runs on its own
  background thread.
