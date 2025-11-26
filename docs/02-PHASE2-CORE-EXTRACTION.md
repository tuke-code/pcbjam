# Phase 2: Core Library Extraction

## Overview

Extract KiCad's core computation code into a standalone library that compiles to both native and WebAssembly. This enables running the native wxWidgets GUI while delegating computation to a WASM module.

## Architecture: Native GUI + WASM Core

```
┌─────────────────────────────────────────────────────────┐
│                    Native GUI (wxWidgets)               │
│  - PCB Editor canvas, menus, dialogs                    │
│  - File dialogs, clipboard                              │
│  - User interaction                                     │
└─────────────────────┬───────────────────────────────────┘
                      │ S-expression serialization
                      ▼
┌─────────────────────────────────────────────────────────┐
│                    WASM Bridge                          │
│  - Wasmtime/wasm3 runtime (future)                      │
│  - Serialize board → S-expr → WASM                      │
│  - Deserialize results ← S-expr ← WASM                  │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│                    kicad_core.wasm                      │
│  - libs/kimath (geometry)                               │
│  - libs/sexpr (parsing)                                 │
│  - Board data model                                     │
│  - DRC engine                                           │
│  - Router (Push & Shove)                                │
└─────────────────────────────────────────────────────────┘
```

## Why This Works: wxWidgets is NOT Required for Core

**Important**: The core libs don't use wxWidgets for GUI - they just use utility macros that happen to come from wx. We provide drop-in replacements via a ~50 line shim header:

| Library | wx Usage | Count | Shim Solution |
|---------|----------|-------|---------------|
| libs/kimath | wxASSERT, wxLogTrace | ~34 | Macros → assert/no-op |
| libs/sexpr | wxFFile, wxString | ~4 | Classes → std equivalents |
| libs/core | wxString | ~17 | typedef → std::string |

### wx_shim.h

```cpp
// core/include/wx_shim.h
#ifndef KICAD_WX_SHIM_H
#define KICAD_WX_SHIM_H

#include <cassert>
#include <string>
#include <cstdio>

// Assertions - just use standard assert
#define wxASSERT(x) assert(x)
#define wxASSERT_MSG(x, msg) assert((x) && (msg))
#define wxCHECK(x, ret) do { if(!(x)) return ret; } while(0)
#define wxCHECK_MSG(x, ret, msg) do { if(!(x)) return ret; } while(0)
#define wxFAIL_MSG(msg) assert(false && (msg))

// Logging - no-op or stderr
#define wxLogTrace(...) ((void)0)
#define wxLogDebug(...) ((void)0)
#define wxLogWarning(...) fprintf(stderr, __VA_ARGS__)

// String - just use std::string
using wxString = std::string;
using wxChar = char;

// File I/O - use standard C++
#include <fstream>
class wxFFile {
    std::ifstream m_file;
public:
    bool Open(const std::string& name) { m_file.open(name); return m_file.is_open(); }
    bool IsOpened() const { return m_file.is_open(); }
    size_t Read(void* buf, size_t count) { m_file.read((char*)buf, count); return m_file.gcount(); }
    bool Eof() const { return m_file.eof(); }
};

#endif
```

## Directory Structure

```
kicad-wasm/
├── core/
│   ├── CMakeLists.txt              # Standalone build without wxWidgets
│   ├── include/
│   │   ├── wx_shim.h               # wx compatibility layer (~50 lines)
│   │   └── kicad_core_api.h        # C API for WASM
│   ├── src/
│   │   └── api.cpp                 # API implementation
│   └── wasm/
│       └── CMakeLists.txt          # Emscripten-specific settings
├── test/
│   ├── test_geometry.cpp           # Test kimath without wx
│   ├── test_board_io.cpp           # Test board load/save
│   └── test.kicad_pcb              # Sample board file
```

## Implementation Steps

### Step 1: Foundation + Shim Layer

- Create `core/` directory structure
- Create `wx_shim.h` with standard C++ replacements
- Create CMakeLists.txt that builds kimath, core, sexpr
- **Test**: Compiles without wxWidgets

### Step 2: Board Model Extraction

- Identify minimal BOARD dependencies
- Create C API: `kicad_load_board()`, `kicad_save_board()`
- Handle S-expression serialization
- **Test**: Load a .kicad_pcb file via API

### Step 3: DRC Engine Extraction

- Extract DRC_ENGINE and test providers
- Create C API: `kicad_drc_run()` returns violations as JSON
- **Test**: Run DRC on test board

### Step 4: Router Extraction

- Extract PNS::ROUTER and supporting classes
- Create minimal ROUTER_IFACE implementation
- Create C API: `kicad_router_start()`, `_move()`, `_commit()`
- **Test**: Route traces via API

### Step 5: Emscripten Build

- Set up emsdk toolchain
- Build `kicad_core.wasm`
- Create JavaScript bindings
- **Test**: Load board in Node.js, run DRC, route traces

### Step 6: Browser Demo (Future)

- Simple HTML page with file upload
- Load .kicad_pcb, display stats
- Run DRC, show violations

## C API Design

### Board I/O

```cpp
extern "C" {
    // Load board from S-expression string
    void* kicad_load_board(const char* sexpr_data, size_t len);

    // Serialize board to S-expression
    char* kicad_save_board(void* board);

    // Free memory
    void kicad_free_board(void* board);
    void kicad_free_string(char* str);

    // Query operations
    int kicad_get_track_count(void* board);
    int kicad_get_footprint_count(void* board);
}
```

### DRC Engine

```cpp
extern "C" {
    // Initialize DRC with rules
    void* kicad_drc_create(void* board, const char* rules_sexpr);

    // Run DRC, returns JSON array of violations
    char* kicad_drc_run(void* drc_engine);

    // Query specific clearance
    int kicad_drc_query_clearance(void* drc, int item_a, int item_b);

    void kicad_drc_free(void* drc);
}
```

### Router

```cpp
extern "C" {
    // Create router with board data
    void* kicad_router_create(void* board);

    // Start routing from point
    int kicad_router_start(void* router, int x, int y, int layer);

    // Move to point, returns preview geometry as S-expr
    char* kicad_router_move(void* router, int x, int y);

    // Commit route
    char* kicad_router_commit(void* router);

    void kicad_router_free(void* router);
}
```

## Emscripten Build

```bash
source /path/to/emsdk/emsdk_env.sh

cd kicad-wasm
mkdir build-wasm && cd build-wasm

emcmake cmake ../core \
    -DCMAKE_BUILD_TYPE=Release \
    -DKICAD_WASM_BUILD=ON

emmake make
```

### Emscripten CMake Settings

```cmake
if(EMSCRIPTEN)
    set_target_properties(kicad_core PROPERTIES
        LINK_FLAGS "-s EXPORTED_FUNCTIONS='[_kicad_load_board,_kicad_save_board,...]' \
                    -s EXPORTED_RUNTIME_METHODS='[ccall,cwrap,UTF8ToString]' \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME='KicadCore' \
                    -s ALLOW_MEMORY_GROWTH=1"
    )
endif()
```

## Key KiCad Source Files

**Libraries to include (via shim, no modification):**
- `kicad/libs/kimath/src/**/*.cpp` - 17.5k lines, geometry
- `kicad/libs/core/*.cpp` - 870 lines, utilities
- `kicad/libs/sexpr/*.cpp` - 734 lines, parser
- `kicad/pcbnew/board*.cpp` - Board data model
- `kicad/pcbnew/pcb_io/kicad_sexpr/*.cpp` - S-expr I/O
- `kicad/pcbnew/drc/*.cpp` - DRC engine
- `kicad/pcbnew/router/pns_*.cpp` - Router

**Key headers:**
- `kicad/libs/kimath/include/geometry/shape_poly_set.h` - Polygon ops
- `kicad/pcbnew/board.h` - BOARD class (1510 lines)
- `kicad/pcbnew/drc/drc_engine.h` - DRC entry point
- `kicad/pcbnew/router/pns_router.h` - Router entry point

## Progress

### ✅ Step 1: Foundation + Shim Layer (COMPLETE)

**Date**: 2025-11-26

Successfully compiled kimath standalone without wxWidgets:

```
core/
├── include/
│   ├── wx_shim.h           # ~170 lines (more than expected, but still minimal)
│   ├── config.h            # Platform configuration
│   ├── advanced_config.h   # Default values for triangulation etc.
│   └── wx/                 # Stub wx headers
│       ├── debug.h
│       ├── log.h
│       ├── string.h
│       └── confbase.h
├── src/
│   └── test_kimath.cpp
├── CMakeLists.txt
└── build/
    ├── libkimath.a         # 1.4 MB static library
    ├── libclipper2.a
    ├── libkicad_core_utils.a
    └── test_kimath         # Working test executable
```

**Key learnings:**
- wx_shim.h needed to be ~170 lines, not ~50, due to:
  - `wxString::Format()` with varargs required a proper class with template Format method
  - `FormatArg<T>` template needed to convert string args to `c_str()` for snprintf
  - `wxLog::EnableLogging()` used in polygon_triangulation.h
  - `wxString::RemoveLast()` used for string manipulation
  - `ADVANCED_CFG` class needed for triangulation settings
- C++20 required (not C++17) due to KiCad's use of concepts
- Build order: Our `core/include/` must come FIRST in include paths

**Test output:**
```
Testing kimath standalone build...
Created VECTOR2I: (0,0) and (100,100)
SEG length: 141
Created polygon with 1 outline(s)
Polygon area: 1e+06
kimath standalone build: SUCCESS!
```

---

### ✅ Step 5 (partial): Emscripten Build (COMPLETE)

**Date**: 2025-11-26

Successfully compiled kimath to WebAssembly:

```bash
$ emcmake cmake .. && emmake make
$ node test_kimath.js

Testing kimath standalone build...
Created VECTOR2I: (0,0) and (100,100)
SEG length: 141
Created polygon with 1 outline(s)
Polygon area: 1e+06
kimath standalone build: SUCCESS!
```

**Build artifacts:**
```
build-wasm/
├── test_kimath.wasm    # 845KB - WASM module
├── test_kimath.js      # 154KB - JS glue code
├── libkimath.a         # 5.1MB - Static WASM library
├── libclipper2.a       # 1.2MB
└── libkicad_core_utils.a # 106KB
```

**Key findings:**
- No code changes needed between native and WASM builds
- Same wx_shim.h works for both targets
- WASM module runs identically to native in Node.js

---

### ✅ Step 2 (partial): S-expression Parser (COMPLETE)

**Date**: 2025-11-26

Added libs/sexpr to the standalone build:

**Additional stubs needed:**
- `wx/file.h`, `wx/ffile.h` - file I/O stubs
- `wxFFile` class in wx_shim.h (~80 lines)
- `string_utils.h` - minimal stub for `From_UTF8()`

**Test output (both native and WASM):**
```
Testing S-expression parser...
Parsed S-expr with 4 elements
Root element: kicad_pcb
```

**WASM sizes:**
```
test_kimath.wasm  - 867KB (geometry + sexpr parser)
libkimath.a       - 5.1MB
libsexpr.a        - 159KB
libclipper2.a     - 1.2MB
```

---

## Success Criteria

- [x] wx_shim.h provides all needed wx replacements
- [x] libs/kimath compiles with shim (no wxWidgets linked)
- [x] kicad_core.wasm builds with Emscripten (kimath portion)
- [x] libs/sexpr compiles with shim (added wxFFile, string_utils stubs)
- [x] S-expression parser can parse .kicad_pcb format strings
- [ ] S-expression parser can load .kicad_pcb from string
- [ ] Board data model extracts cleanly
- [ ] C API wrapper builds as native static library
- [ ] kicad_core.wasm builds with Emscripten
- [ ] Node.js can load a .kicad_pcb file via WASM
- [ ] DRC runs in WASM, outputs violation list
- [ ] Router API works in WASM

## First Concrete Step

Start with Step 1: Create `core/` directory with `wx_shim.h` and attempt to compile just `libs/kimath` standalone. This proves the shim approach works before tackling the larger board model.

## Dependencies

**Must include in WASM build:**
- Clipper2 library (polygon boolean operations) - pure C++
- RTree (spatial indexing) - header-only

**Not needed:**
- wxWidgets (replaced by shim)
- Boost (only header-only templates used)
- OpenCASCADE, ngspice, curl, libgit2 (already disabled in Phase 1)
