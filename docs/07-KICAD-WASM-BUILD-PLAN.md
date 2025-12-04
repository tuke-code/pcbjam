# KiCad PCBnew WASM Build Plan

## Goal

Build the full KiCad PCBnew application for WebAssembly with all major features:
- **Target**: PCBnew (PCB Editor)
- **3D/STEP**: OpenCASCADE ported to WASM
- **Simulation**: ngspice ported to WASM
- **Threading**: Emscripten pthreads (true parallelism)
- **Coroutines**: Emscripten Asyncify fibers for libcontext
- **Stub only**: nanodbc (ODBC not available in browser)

## Core Principles

### 1. No Source Modifications to KiCad or wxWidgets

**CRITICAL**: All WASM-specific code must go into compatibility layers, NOT into the KiCad or wxWidgets source trees.

```
kicad-wasm/
├── kicad/                    # Git submodule - DO NOT MODIFY
├── wxwidgets/                # Git submodule - DO NOT MODIFY (except WASM platform)
├── wasm/                     # NEW: All WASM compatibility layers
│   ├── kiplatform/           # Platform layer implementations
│   ├── libcontext/           # Fiber implementation
│   ├── shims/                # Header shims and wrappers
│   └── stubs/                # Feature stubs
├── stubs/                    # (existing) Stub headers/implementations
├── cmake/                    # (existing) CMake find modules
└── patches/                  # Minimal patches ONLY if absolutely necessary
```

### 2. Compatibility Layer Strategy

Instead of patching KiCad source, we:
1. **Override include paths** - Put our headers first in include path
2. **Provide stub libraries** - Link our stubs instead of real libraries
3. **CMake module overrides** - Replace find_package results with our targets
4. **Platform implementations** - Provide WASM versions of platform-specific code

### 3. Dependencies First Approach

Build all dependencies (including OpenCASCADE and ngspice) BEFORE building KiCad to ensure a clean build.

---

## KiCad Submodule Version

```
Commit: 4bfed3f1746e8cc0a7d942767770f56fa28b393c
Version: 8.99 (development)
```

---

## Exact Dependency Versions

These versions are from KiCad's `CMakeLists.txt` and `vcpkg.json`:

### Required Dependencies

| Dependency | Min Version | Pinned Version | Source |
|------------|-------------|----------------|--------|
| wxWidgets | 3.2.0 | 3.3.1 | vcpkg override |
| GLM | 0.9.8 | 0.9.9.8 | vcpkg override |
| Boost | 1.71.0 | latest | CMakeLists.txt |
| FreeType | 2.11.1 | latest | CMakeLists.txt |
| HarfBuzz | - | latest | CMakeLists.txt |
| Fontconfig | - | latest | CMakeLists.txt |
| Cairo | 1.12 | latest | CMakeLists.txt |
| Pixman | 0.30 | latest | CMakeLists.txt |
| zlib | - | latest | CMakeLists.txt |
| Zstd | - | latest | CMakeLists.txt |
| OpenCASCADE | 7.5.0+ | 7.8.0+ preferred | CMakeLists.txt |
| ngspice | - | 45.2 | vcpkg override |
| Protobuf | 3.21.12 | 3.21.12 | vcpkg override |
| libgit2 | 1.5 | latest | CMakeLists.txt |
| CURL | - | latest | CMakeLists.txt |
| Python | 3.6+ | 3.11.5 | vcpkg override |

### What We Build vs Stub

| Dependency | Action | Reason |
|------------|--------|--------|
| wxWidgets | Already ported | WASM platform in wxwidgets submodule |
| GLM | Header-only | Just include |
| Boost | Header-only subset | Only need headers for most parts |
| FreeType | Emscripten port | `-sUSE_FREETYPE=1` |
| HarfBuzz | Build for WASM | Text shaping needed |
| zlib | Emscripten port | `-sUSE_ZLIB=1` |
| Zstd | Build for WASM | Compression needed |
| OpenCASCADE | Build for WASM | 3D/STEP support |
| ngspice | Build for WASM | Simulation support |
| Cairo | Build for WASM | 2D rendering fallback |
| Pixman | Build for WASM | Cairo dependency |
| libgit2 | **STUB** | No git in browser |
| CURL | **STUB** | Use fetch API instead |
| nanodbc | **STUB** | No ODBC in browser |
| Python/SWIG | **DISABLE** | No Python scripting |
| nng | **STUB** | No IPC in browser |
| SPNAV | **STUB** | No 3D mouse in browser |

---

## Compatibility Layer Structure

### Directory Layout

```
wasm/
├── CMakeLists.txt                    # Master WASM compat build
├── kiplatform/                       # Platform layer for WASM
│   ├── CMakeLists.txt
│   ├── app.cpp                       # App lifecycle
│   ├── drivers.cpp                   # GPU detection ("WebGL")
│   ├── environment.cpp               # Env vars via localStorage
│   ├── io.cpp                        # Virtual filesystem
│   ├── policy.cpp                    # Permissions (always allow)
│   ├── secrets.cpp                   # Credentials (localStorage)
│   ├── sysinfo.cpp                   # System info
│   └── printing.cpp                  # Browser print()
├── libcontext/                       # Coroutine implementation
│   ├── CMakeLists.txt
│   └── fcontext_wasm.cpp             # Asyncify fiber impl
├── shims/                            # Header overrides
│   ├── CMakeLists.txt
│   ├── kiplatform_redirect.h         # Redirect to our impl
│   └── libcontext_redirect.h         # Redirect to our impl
└── config/                           # Build configuration
    ├── kicad_wasm_config.h           # Version/feature config
    └── setup.h                       # Platform setup
```

### Existing Stubs (Already Done)

```
stubs/
├── include/
│   ├── curl/curl.h, easy.h           # CURL stubs
│   ├── git2.h                        # libgit2 stub
│   ├── git2/sys/errors.h, merge.h    # libgit2 internals
│   ├── ngspice/sharedspice.h         # ngspice header (for stub build)
│   └── Standard_Version.hxx          # OCC version stub
└── src/
    ├── disabled_features_stubs.cpp   # CURL/git function stubs
    ├── kicad_git_stubs.cpp           # Git feature stubs
    ├── kicad_git_all_stubs.cpp       # Complete git stubs
    ├── occ_stubs.cpp                 # OpenCASCADE stubs
    └── panel_git_repos_stub.cpp      # Git UI stubs
```

### CMake Overrides (Already Done)

```
cmake/
├── FindCURL.cmake                    # Returns stub target
├── FindOCC.cmake                     # Configurable real/stub
├── Findlibgit2.cmake                 # Returns stub target
├── Findngspice.cmake                 # Configurable real/stub
└── KicadWasmOptions.cmake            # Feature flags
```

---

## Build Phases

### Phase 1: Build Infrastructure

Create common utilities:

```bash
scripts/
├── common/
│   ├── env.sh           # Emscripten environment, paths
│   ├── functions.sh     # Error handling, logging
│   └── versions.sh      # Dependency versions (from above table)
├── build-kicad-wasm.sh  # Master orchestrator
└── build-deps/          # Per-dependency scripts
```

**versions.sh** - Pin to KiCad's required versions:
```bash
#!/bin/bash
# Versions matching KiCad 8.99 requirements

export KICAD_COMMIT="4bfed3f1746e8cc0a7d942767770f56fa28b393c"

# From vcpkg.json overrides
export GLM_VERSION="0.9.9.8"
export NGSPICE_VERSION="45.2"
export PROTOBUF_VERSION="3.21.12"

# From CMakeLists.txt minimums
export WXWIDGETS_MIN="3.2.0"
export GLM_MIN="0.9.8"
export BOOST_MIN="1.71.0"
export FREETYPE_MIN="2.11.1"
export CAIRO_MIN="1.12"
export PIXMAN_MIN="0.30"
export LIBGIT2_MIN="1.5"
export OCC_MIN="7.5.0"

# Recommended versions for WASM build
export OCC_VERSION="7.8.0"
export ZSTD_VERSION="1.5.5"
export HARFBUZZ_VERSION="8.3.0"
```

### Phase 2: Dependencies (In Order)

Build these BEFORE KiCad:

| Order | Dependency | Script | Notes |
|-------|------------|--------|-------|
| 1 | zlib | Emscripten port | `-sUSE_ZLIB=1` |
| 2 | Zstd | `build-zstd-wasm.sh` | Compression |
| 3 | FreeType | Emscripten port | `-sUSE_FREETYPE=1` |
| 4 | HarfBuzz | `build-harfbuzz-wasm.sh` | Text shaping |
| 5 | Pixman | `build-pixman-wasm.sh` | Cairo dep |
| 6 | Cairo | `build-cairo-wasm.sh` | 2D rendering |
| 7 | Boost | Headers only | Copy headers |
| 8 | Protobuf | `build-protobuf-wasm.sh` | If IPC needed |
| 9 | OpenCASCADE | `build-occ-wasm.sh` | 3D/STEP (large) |
| 10 | ngspice | `build-ngspice-wasm.sh` | Simulation |
| 11 | wxWidgets | Already done | `build-wxuniversal-wasm.sh` |

### Phase 3: WASM Compatibility Layer

Create `wasm/` directory with platform implementations.

#### kiplatform WASM (wasm/kiplatform/)

These files provide WASM implementations of KiCad's platform abstraction:

```cpp
// wasm/kiplatform/app.cpp
#include <kiplatform/app.h>

namespace KIPLATFORM::APP {
    bool Init() { return true; }
    wxString GetUserConfigPath() { return "/home/kicad"; }
    wxString GetUserDataPath() { return "/home/kicad"; }
    // ... etc
}
```

```cpp
// wasm/kiplatform/environment.cpp
#include <kiplatform/environment.h>
#include <emscripten.h>

namespace KIPLATFORM::ENV {
    wxString GetEnv(const wxString& var) {
        // Use localStorage via JS
        char* val = (char*)EM_ASM_PTR({
            var key = UTF8ToString($0);
            var val = localStorage.getItem('env_' + key) || '';
            return stringToNewUTF8(val);
        }, var.c_str());
        wxString result(val);
        free(val);
        return result;
    }
}
```

#### libcontext Asyncify Fibers (wasm/libcontext/)

```cpp
// wasm/libcontext/fcontext_wasm.cpp
#ifdef __EMSCRIPTEN__
#include <emscripten/fiber.h>

// Provide same interface as libcontext but using Emscripten fibers
struct fcontext_transfer {
    void* fctx;
    void* data;
};

static emscripten_fiber_t main_fiber;
static bool main_fiber_initialized = false;

extern "C" {
    fcontext_transfer jump_fcontext(void* to, void* vp);
    void* make_fcontext(void* sp, size_t size, void (*fn)(fcontext_transfer));
}

// Implementation using emscripten_fiber_* APIs
#endif
```

### Phase 4: KiCad Build

**Script**: `scripts/build-pcbnew-wasm.sh`

```bash
#!/bin/bash
set -e

source "$(dirname "$0")/common/env.sh"

# Key: Override include paths to use our compatibility layers FIRST
WASM_INCLUDES="-I$PROJECT_ROOT/wasm/kiplatform"
WASM_INCLUDES="$WASM_INCLUDES -I$PROJECT_ROOT/wasm/libcontext"
WASM_INCLUDES="$WASM_INCLUDES -I$PROJECT_ROOT/wasm/shims"
WASM_INCLUDES="$WASM_INCLUDES -I$PROJECT_ROOT/stubs/include"

emcmake cmake ../kicad \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="$WASM_INCLUDES" \
    \
    # Use our CMake modules for stubs
    -DCMAKE_MODULE_PATH="$PROJECT_ROOT/cmake" \
    \
    # Feature flags
    -DKICAD_USE_OCC=ON \
    -DKICAD_USE_NGSPICE=ON \
    -DKICAD_USE_GIT=OFF \
    -DKICAD_USE_CURL=OFF \
    -DKICAD_SCRIPTING_WXPYTHON=OFF \
    -DKICAD_IPC_API=OFF \
    -DKICAD_BUILD_QA_TESTS=OFF \
    -DKICAD_BUILD_I18N=OFF \
    \
    # Point to our builds
    -DwxWidgets_CONFIG_EXECUTABLE="$WX_BUILD/wx-config" \
    -DOCC_INCLUDE_DIR="$SYSROOT/include/opencascade" \
    -DNGSPICE_LIBRARY="$SYSROOT/lib/libngspice.a"

emmake make pcbnew -j$(nproc)
```

### Phase 5: Testing

Follow existing patterns in `tests/`:

```
tests/wasm-app/standalone/pcbnew/
├── pcbnew_test.cpp      # Minimal PCBnew test app
├── pcbnew_test.html     # Generated
├── pcbnew_test.js       # Generated
└── pcbnew_test.wasm     # Generated

tests/e2e/
└── pcbnew.spec.ts       # Playwright E2E tests
```

---

## Link Flags

```bash
# Core flags
-sALLOW_MEMORY_GROWTH=1
-sINITIAL_MEMORY=256MB
-sSTACK_SIZE=5MB

# Async/modal support
-sASYNCIFY=1
-sASYNCIFY_STACK_SIZE=16384

# OpenGL/WebGL
-sLEGACY_GL_EMULATION
-sMAX_WEBGL_VERSION=2

# Threading
-pthread
-sPROXY_TO_PTHREAD=1
-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency
-sOFFSCREENCANVAS_SUPPORT=1
```

Server headers for pthreads:
```
Cross-Origin-Embedder-Policy: require-corp
Cross-Origin-Opener-Policy: same-origin
```

---

## File Organization Summary

### What Goes Where

| Code Type | Location | Reason |
|-----------|----------|--------|
| WASM platform impl | `wasm/kiplatform/` | Don't touch kicad/ |
| Fiber implementation | `wasm/libcontext/` | Don't touch kicad/ |
| Header overrides | `wasm/shims/` | Include path override |
| Stub headers | `stubs/include/` | Already exists |
| Stub implementations | `stubs/src/` | Already exists |
| CMake finders | `cmake/` | Already exists |
| Build scripts | `scripts/` | Reproducible |
| Test apps | `tests/wasm-app/` | Existing pattern |
| E2E tests | `tests/e2e/` | Existing pattern |

### Files to Create

```
NEW:
├── wasm/
│   ├── CMakeLists.txt
│   ├── kiplatform/*.cpp (8 files)
│   ├── libcontext/fcontext_wasm.cpp
│   ├── shims/*.h
│   └── config/*.h
├── scripts/
│   ├── common/{env,functions,versions}.sh
│   ├── build-kicad-wasm.sh
│   ├── build-pcbnew-wasm.sh
│   └── build-deps/*.sh
├── stubs/src/nanodbc_stub.cpp
└── tests/
    ├── wasm-app/standalone/pcbnew/*
    └── e2e/pcbnew.spec.ts
```

### Files NOT to Modify

```
DO NOT MODIFY:
├── kicad/           # Git submodule - use compatibility layers instead
└── wxwidgets/       # Git submodule - WASM platform already added
```

---

## Patches (Only If Absolutely Necessary)

If patches are unavoidable, they go in `patches/` with clear documentation:

```
patches/
├── kicad/
│   ├── 0001-*.patch
│   ├── checksums.sha256
│   └── README.md       # Explain WHY each patch is needed
├── opencascade/
│   └── *.patch         # OCC WASM compatibility
└── ngspice/
    └── *.patch         # Remove fork/exec
```

**Rule**: Before creating a patch, ask "Can this be done with a compatibility layer instead?"

---

## Success Criteria

### MVP (Milestone 1)
- [ ] All dependencies built for WASM
- [ ] PCBnew window opens in browser
- [ ] Menu bar and toolbars visible
- [ ] Can load .kicad_pcb file
- [ ] Board renders in WebGL
- [ ] Pan/zoom works

### Full Features (Milestone 2)
- [ ] All editing tools work
- [ ] Interactive router works (via Asyncify fibers)
- [ ] Zone filling works (via pthreads)
- [ ] DRC runs
- [ ] Save/export works
- [ ] 3D viewer works (OCC)
- [ ] Simulation works (ngspice)

---

## Quick Start

```bash
# 1. Set up Emscripten (already available via homebrew)
# Emscripten is at /opt/homebrew/bin/emcc

# 2. Build wxWidgets (if not already done)
./scripts/build-wxuniversal-wasm.sh

# 3. Build all dependencies
./scripts/deps/build-all-deps.sh --all

# 4. Build PCBnew for WASM
./scripts/build-pcbnew-wasm.sh

# 5. Test
cd tests && npm test

# 6. Serve (with COOP/COEP headers for SharedArrayBuffer)
cd build-wasm && npx serve -p 8080
```

---

## Implementation Status

### Created Files

#### Build Infrastructure (`scripts/common/`)
- `env.sh` - Environment setup (paths, Emscripten config)
- `functions.sh` - Utility functions (logging, downloads, stamps)
- `versions.sh` - Pinned dependency versions from KiCad

#### Dependency Build Scripts (`scripts/deps/`)
- `build-all-deps.sh` - Master dependency builder
- `build-zstd.sh` - Compression library
- `build-freetype.sh` - Font rendering
- `build-harfbuzz.sh` - Text shaping
- `build-pixman.sh` - Pixel manipulation
- `build-cairo.sh` - 2D graphics
- `build-glm.sh` - Math library (header-only)
- `build-protobuf.sh` - Protocol buffers
- `build-opencascade.sh` - 3D geometry/STEP
- `build-ngspice.sh` - SPICE simulation

#### WASM Compatibility Layer (`wasm/`)
- `CMakeLists.txt` - Main CMake configuration
- `README.md` - Documentation
- `kiplatform/CMakeLists.txt`
- `kiplatform/app.cpp` - Application lifecycle
- `kiplatform/drivers.cpp` - 3D mouse (stub)
- `kiplatform/environment.cpp` - Environment/paths with localStorage
- `kiplatform/io.cpp` - File I/O for virtual filesystem
- `kiplatform/policy.cpp` - Enterprise policies (stub)
- `kiplatform/secrets.cpp` - Credential storage via localStorage
- `kiplatform/sysinfo.cpp` - System info via WebGL/navigator
- `kiplatform/ui.cpp` - UI utilities (theme detection, etc.)
- `libcontext/CMakeLists.txt`
- `libcontext/libcontext_wasm.h` - Asyncify fiber header
- `libcontext/libcontext_wasm.cpp` - Asyncify fiber implementation
- `cmake/KiCadWASMConfig.cmake` - CMake config for WASM
- `cmake/FindKiplatformWASM.cmake` - Find module for kiplatform
- `cmake/FindLibcontextWASM.cmake` - Find module for libcontext

#### PCBnew Build
- `scripts/build-pcbnew-wasm.sh` - Main PCBnew build script

#### Tests (`tests/kicad/`)
- `pcbnew.html` - Test app HTML
- `pcbnew.spec.ts` - Playwright E2E tests

### Known Issues

1. **CMake Policy**: Zstd and some older libraries need `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`
   to work with modern CMake

2. **Shell Environment**: When sourcing scripts, use a fresh bash (`bash -c '...'`) to avoid
   conflicts with existing environment variables
