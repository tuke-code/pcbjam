# Experiment: native wasm exceptions (-fwasm-exceptions) to shrink the asyncify/-O2 critical path

**Status: PARKED** (2026-06-11). Compiles and links end-to-end, but blocked by an LLVM
codegen bug in emscripten 4.0.2 — see "The blocker" below. The code changes were tried
locally and then dropped; the full patch is preserved in the appendix of this doc,
together with everything needed to resume.

## Why this matters

After the 3.3x CI win (see `ci-build-slowness-findings.md`), the critical path of the
full build is pcbnew's host-side wasm-opt chain: **asyncify ~5 min + `-O2` ~52 min**
(self-built Binaryen v130 on the Hetzner ccx53). That `-O2` time is a direct function of
how big the asyncified module is — and the module is big because of how exceptions work.

Emscripten's default **JS-based exception handling** routes every potentially-throwing
call that appears inside a try/catch through a JavaScript `invoke_*` trampoline.
Asyncify must treat every JS round-trip as a potential suspension point, so in an
exception-heavy codebase (KiCad + OpenCASCADE + wxWidgets) it ends up instrumenting
nearly every function: pcbnew is **338 MB pre-`-O2`**.

**Native wasm exceptions** (`-fwasm-exceptions`) keep throw/catch entirely inside wasm:

- no `invoke_*` trampolines, no `dynCall` machinery (verified gone in the experiment),
- asyncify's instrumentation set collapses to the genuinely-suspending call graph,
- raw linked pcbnew.wasm measured at **92 MB** (vs 338 MB) before any wasm-opt pass.

Expected payoff: a several-fold reduction in the asyncify + `-O2` wall time on the
critical path, plus smaller shipped binaries.

Browser support is not a concern: Chrome 95+, Firefox 100+, Safari 15.2+.

## What it takes (no KiCad C++ changes)

Pure compile/link-flag change. The one hard rule: **every C++ object in the link must
agree on the EH model** — deps (boost, anything with setjmp like cairo), wxWidgets, and
all KiCad targets. The working flag set, threaded through every compile AND link:

```
-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0
```

The appendix below contains the full plumbing as a patch, gated behind `KICAD_WASM_EH=1`
(default 0 = byte-identical to today): `scripts/common/env.sh`, `scripts/deps/build-boost.sh`,
`scripts/deps/build-cairo.sh`, `scripts/build-wxuniversal-wasm.sh`,
`scripts/kicad/build-kicad-target.sh`, and `docker/build.sh` (env passthrough into the
container). Save the appendix diff to a file and `git apply` it to restore.

### Why each flag, and the failures that taught us (in order hit)

1. **`-fwasm-exceptions` alone fails immediately**: harfbuzz (C, uses setjmp) dies with
   `clang: error: invalid argument '-fwasm-exceptions' not allowed with
   '-enable-emscripten-sjlj'`. JS-based setjmp/longjmp and wasm EH can't mix →
   add `-sSUPPORT_LONGJMP=wasm` to every compile.
2. **Stale deps bite at link**: pcbnew link failed with `undefined symbol:
   emscripten_longjmp`, traced via `llvm-nm` to `libcairo.a` (its png path uses
   setjmp/longjmp). The deps scripts skip-if-stamped, so flag changes do NOT trigger
   rebuilds — `rm build-wasm/stamps/cairo.stamp` (and any other C dep with sjlj) and
   rebuild. A full `--clean` deps build is the safe path.
3. **`wasm-emscripten-finalize` / `wasm-opt -all` parse failure**: `[parse exception:
   popping from empty stack]` on the linked module, with both the emsdk-bundled
   binaryen and official v130. emscripten 4.0.2 defaults `WASM_LEGACY_EXCEPTIONS=true`
   (the old, pre-standard `exnref`-less encoding) → add `-sWASM_LEGACY_EXCEPTIONS=0`
   (it's a `[compile+link]` setting — must be on every compile too, full rebuild).

## The blocker: LLVM codegen bug in emscripten 4.0.2

With all three flags and a fully clean rebuild, pcbnew compiles and links (92 MB module,
no `invoke_*`/`dynCall`), but finalize still fails with the same parse exception — and
this time Binaryen is not at fault. Validating the raw module with V8 itself
(container node 22.16, `node --experimental-wasm-exnref`, `WebAssembly.compile`) gives
the definitive error:

```
Compiling function #96546:"ShapeUpgrade_SplitSurface::Build(bool)" failed:
br_table: label arity inconsistent with previous arity 0 @+52067104
```

The clang/LLVM shipped in emscripten **4.0.2** emits an invalid `br_table` instruction
(branch targets with mismatched stack arity) in OpenCASCADE code when compiling under
wasm EH. The module is malformed at the source; no Binaryen version or flag can fix it.
Latest Binaryen release is still v130, so there is no newer tarball to try either.

## How to resume

1. Bump `ARG EMSCRIPTEN_VERSION=4.0.2` in `docker/Dockerfile` — emsdk **5.0.7** (latest
   5.x, conservative) or 6.0.0 — to pick up a newer LLVM with the br_table fix.
   Expect ~2.5–3h for image + full clean deps/wx/kicad rebuild locally.
2. `git apply` the patch from the appendix, then
   `KICAD_WASM_EH=1 BINARYEN_VERSION=130 ./docker/build.sh pcbnew --build-deps -j 8`
   (use a separate `COMPOSE_PROJECT_NAME` to keep the main build volume intact).
3. If finalize + asyncify + `-O2` succeed: measure the wasm-opt chain vs the current
   5 min + 52 min, then a **small Hetzner repro** (never `all` for experiments), then
   full e2e.
4. **e2e audit required even if green**: asyncify cannot suspend from inside a `catch`
   block (binaryen issue #4470). Any KiCad path that calls a suspending function
   (file dialogs, sleeps, network) inside a catch handler will trap. Build once with
   `-sASYNCIFY_ASSERTIONS` / asyncify-asserts and exercise the e2e suite to flush
   these out before adopting.

## Risk notes

- An emsdk bump changes the compiler for the *whole* project — it must be validated for
  the normal (JS-EH) build too, not just this experiment.
- JSPI (the long-term replacement for asyncify) was evaluated and is not viable yet:
  Firefox still flag-gates it.

## Appendix: the full plumbing patch

Applies cleanly on top of `a563746`. Save the block below to a file and `git apply` it.

````diff
diff --git a/docker/build.sh b/docker/build.sh
index 268b03d..456f906 100755
--- a/docker/build.sh
+++ b/docker/build.sh
@@ -182,7 +182,8 @@ compile_app() {
     # -e EMSDK=/emsdk: `docker compose exec` bypasses the entrypoint that sources
     # emsdk_env.sh, so the build shell would lack emcc/embuilder on PATH. Setting
     # EMSDK lets scripts/common/env.sh source /emsdk/emsdk_env.sh and activate the toolchain.
-    docker compose -f docker/docker-compose.yml exec -e EMSDK=/emsdk kicad-wasm-builder \
+    docker compose -f docker/docker-compose.yml exec -e EMSDK=/emsdk \
+        -e KICAD_WASM_EH="${KICAD_WASM_EH:-0}" kicad-wasm-builder \
         "/workspace/scripts/kicad/build-${app}.sh" "${ARGS[@]}"
 
     # Copy output to host-accessible directory.
diff --git a/scripts/build-wxuniversal-wasm.sh b/scripts/build-wxuniversal-wasm.sh
index 29b7f20..385b356 100755
--- a/scripts/build-wxuniversal-wasm.sh
+++ b/scripts/build-wxuniversal-wasm.sh
@@ -137,9 +137,16 @@ if [ $NEEDS_CONFIGURE -eq 1 ]; then
         echo "Building wxWidgets in RELEASE mode"
     fi
 
+    # EH model must match the rest of the build (see env.sh KICAD_WASM_EH).
+    if [ "${KICAD_WASM_EH:-0}" = "1" ]; then
+        WX_EH_FLAG="-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0"
+    else
+        WX_EH_FLAG="-fexceptions"
+    fi
+
     # Include emscripten cache sysroot for zlib headers
-    export CFLAGS="-DZ_HAVE_UNISTD_H=1 -I$EM_CACHE_SYSROOT/include ${WX_DEBUG_FLAGS} -fexceptions -pthread -matomics -mbulk-memory"
-    export CXXFLAGS="-DZ_HAVE_UNISTD_H=1 -I$EM_CACHE_SYSROOT/include -I$PCRE2_INCLUDE ${WX_DEBUG_FLAGS} -fexceptions -pthread -matomics -mbulk-memory"
+    export CFLAGS="-DZ_HAVE_UNISTD_H=1 -I$EM_CACHE_SYSROOT/include ${WX_DEBUG_FLAGS} ${WX_EH_FLAG} -pthread -matomics -mbulk-memory"
+    export CXXFLAGS="-DZ_HAVE_UNISTD_H=1 -I$EM_CACHE_SYSROOT/include -I$PCRE2_INCLUDE ${WX_DEBUG_FLAGS} ${WX_EH_FLAG} -pthread -matomics -mbulk-memory"
     export LDFLAGS="-L$EM_CACHE_SYSROOT/lib/wasm32-emscripten"
 
     emconfigure "$WX_SOURCE/configure" \
diff --git a/scripts/common/env.sh b/scripts/common/env.sh
index 90fb61a..0445f99 100755
--- a/scripts/common/env.sh
+++ b/scripts/common/env.sh
@@ -82,7 +82,22 @@ else
     export DEBUG_LDFLAGS=""
 fi
 
-export DEBUG_BUILD BUILD_TYPE DEBUG_CFLAGS DEBUG_LDFLAGS
+# KICAD_WASM_EH=1 (EXPERIMENTAL): native WebAssembly exceptions instead of
+# emscripten's JS-based EH. JS-EH routes every potentially-throwing call inside
+# a try/catch through a JS invoke_* trampoline, which forces asyncify to
+# instrument nearly the whole exception-heavy codebase (pcbnew: 338 MB pre-O2).
+# Wasm EH (-fwasm-exceptions, Chrome 95+/Firefox 100+/Safari 15.2+) keeps
+# exceptions inside wasm — no invoke_*, far smaller asyncify set. ALL C++ must
+# agree on the EH model (deps + wx + kicad): this var feeds every compile.
+# Known limit: asyncify cannot suspend from inside a catch block (binaryen #4470).
+if [ "${KICAD_WASM_EH:-0}" = "1" ]; then
+    # -sSUPPORT_LONGJMP=wasm: setjmp/longjmp must use the same (wasm) machinery;
+    # without it emcc injects -enable-emscripten-sjlj, which clang rejects in
+    # combination with -fwasm-exceptions.
+    export DEBUG_CFLAGS="${DEBUG_CFLAGS} -fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0"
+fi
+
+export DEBUG_BUILD BUILD_TYPE DEBUG_CFLAGS DEBUG_LDFLAGS KICAD_WASM_EH
 
 # Parallel jobs (default to 1 for memory-constrained environments like Docker)
 # Can be overridden with -j N flag or by setting JOBS/PARALLEL_JOBS env vars
diff --git a/scripts/deps/build-boost.sh b/scripts/deps/build-boost.sh
index 8905e79..04da340 100755
--- a/scripts/deps/build-boost.sh
+++ b/scripts/deps/build-boost.sh
@@ -72,6 +72,11 @@ else
     BOOST_DEBUG_FLAGS="-O2"
 fi
 
+# EH model must match the rest of the build (see env.sh KICAD_WASM_EH).
+if [ "${KICAD_WASM_EH:-0}" = "1" ]; then
+    BOOST_DEBUG_FLAGS="${BOOST_DEBUG_FLAGS} -fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0"
+fi
+
 # Create user-config.jam for Emscripten
 cat > user-config.jam << EOF
 using clang : emscripten
diff --git a/scripts/deps/build-cairo.sh b/scripts/deps/build-cairo.sh
index 2580320..9d17e15 100755
--- a/scripts/deps/build-cairo.sh
+++ b/scripts/deps/build-cairo.sh
@@ -65,6 +65,13 @@ else
     MESON_DEBUG_FLAGS="'-O2'"
 fi
 
+# Cairo is C, but its png path uses setjmp/longjmp — the SJLJ machinery must
+# match the link (see env.sh KICAD_WASM_EH): without this, libcairo.a keeps
+# emscripten_longjmp references that are undefined under -sSUPPORT_LONGJMP=wasm.
+if [ "${KICAD_WASM_EH:-0}" = "1" ]; then
+    MESON_DEBUG_FLAGS="${MESON_DEBUG_FLAGS}, '-sSUPPORT_LONGJMP=wasm', '-sWASM_LEGACY_EXCEPTIONS=0'"
+fi
+
 # Cairo uses meson
 cat > cross-file.txt << EOF
 [binaries]
diff --git a/scripts/kicad/build-kicad-target.sh b/scripts/kicad/build-kicad-target.sh
index 6ace02b..0dc10f2 100755
--- a/scripts/kicad/build-kicad-target.sh
+++ b/scripts/kicad/build-kicad-target.sh
@@ -193,25 +193,34 @@ log_info "Building KiCad ${APP_NAME} ${KICAD_VERSION} for WASM..."
 
 # Step 5: Set build type
 # Use environment DEBUG_BUILD if set, otherwise check local --debug flag
-# -fexceptions is required because wxWidgets is built with exceptions enabled
+# Exceptions are required because wxWidgets is built with exceptions enabled.
+# EH model: JS-based (-fexceptions, default) or native wasm EH
+# (-fwasm-exceptions, KICAD_WASM_EH=1 — see env.sh). Must match deps + wx.
 # -matomics -mbulk-memory are required for shared memory (pthreads)
 # NOTE: We use -O1 for debug builds because -O0 produces WASM with too many
 # locals for V8/Chrome to compile (error: "local count too large").
 # -O1 keeps debug info but optimizes enough to stay under V8's limits.
+if [ "${KICAD_WASM_EH:-0}" = "1" ]; then
+    EH_FLAG="-fwasm-exceptions -sSUPPORT_LONGJMP=wasm -sWASM_LEGACY_EXCEPTIONS=0"
+    log_info "Using native WebAssembly exceptions (-fwasm-exceptions)"
+else
+    EH_FLAG="-fexceptions"
+fi
+
 if [ "${DEBUG_BUILD:-0}" = "1" ] || [ $DEBUG -eq 1 ]; then
     BUILD_TYPE="Debug"
-    EXTRA_FLAGS="-g -O1 -fexceptions -matomics -mbulk-memory"
+    EXTRA_FLAGS="-g -O1 ${EH_FLAG} -matomics -mbulk-memory"
     # -gseparate-dwarf puts debug info in a separate .debug.wasm file
     # This keeps the main WASM small (~200MB) while preserving full debug info
     # DevTools loads the debug file on-demand when debugging
-    LINKER_DEBUG_FLAGS="-O1 -g -gseparate-dwarf -fexceptions"
+    LINKER_DEBUG_FLAGS="-O1 -g -gseparate-dwarf ${EH_FLAG}"
     log_info "Building KiCad in DEBUG mode (separate DWARF for smaller main binary)"
 else
     BUILD_TYPE="Release"
-    EXTRA_FLAGS="-O2 -fexceptions -matomics -mbulk-memory"
+    EXTRA_FLAGS="-O2 ${EH_FLAG} -matomics -mbulk-memory"
     # -O0 at link time skips wasm-opt (which can OOM on large WASM files)
     # Compilation is still -O2 for optimized code, but we skip post-link wasm-opt
-    LINKER_DEBUG_FLAGS="-O0 -fexceptions"
+    LINKER_DEBUG_FLAGS="-O0 ${EH_FLAG}"
     log_info "Building KiCad in RELEASE mode (skipping wasm-opt due to memory limits)"
 fi
 
````
