# KiCad Wasm Port - Implementation Plan

## Goal

Run KiCad with native wxWidgets GUI, but with core logic (file parsing, geometry, DRC, routing) executing in a WebAssembly module. This validates the Wasm build before tackling the browser UI.

---

## Phase 1: Minimal Build with Stubs

**Objective**: Get KiCad compiling with optional deps disabled via preprocessor guards.

### 1.1 Create Project Structure

```
kicad-wasm/
├── kicad/                    # Git submodule → upstream KiCad
├── patches/
│   └── 0001-optional-deps.patch
├── stubs/
│   └── include/
│       ├── git2.h            # Minimal stub headers
│       └── curl/curl.h
├── cmake/
│   └── KicadWasmOptions.cmake
├── CMakeLists.txt
└── scripts/
    ├── prepare.sh            # Apply patches
    └── update-kicad.sh       # Update to new KiCad version
```

### 1.2 Create the Patch

Add CMake options to `CMakeLists.txt` (after line 107):

```cmake
option( KICAD_USE_CURL "Enable network features" ON )
option( KICAD_USE_GIT "Enable git integration" ON )
option( KICAD_USE_OCC "Enable OpenCASCADE for STEP" ON )
option( KICAD_USE_NGSPICE "Enable SPICE simulation" ON )
option( KICAD_USE_DATABASE "Enable database libraries" ON )
```

Wrap `find_package` calls conditionally:

```cmake
if( KICAD_USE_CURL )
    find_package( CURL REQUIRED )
    add_compile_definitions( KICAD_USE_CURL )
else()
    add_library( CURL::libcurl INTERFACE IMPORTED )
endif()
```

### 1.3 Create Stub Headers

Minimal headers that let code compile without the actual libraries:

```cpp
// stubs/include/git2.h
#pragma once
typedef struct git_repository git_repository;
// Stub functions return error codes
```

### 1.4 Add Preprocessor Guards to Source

Wrap implementations in `common/git/*.cpp`, `common/kicad_curl/*.cpp`:

```cpp
#ifdef KICAD_USE_GIT
// actual implementation
#else
// return error or throw "feature disabled"
#endif
```

### 1.5 Verify Native Build

```bash
cd kicad-wasm
./scripts/prepare.sh
mkdir build && cd build
cmake .. -DKICAD_USE_CURL=OFF -DKICAD_USE_GIT=OFF -DKICAD_USE_OCC=OFF
make -j$(nproc)
```

Confirm KiCad launches and can open/edit PCB files (without git/network/STEP features).

---

## Phase 2: Extract Core Library

**Objective**: Build core computation code as a standalone library that can be compiled to both native and Wasm.

### 2.1 Identify Core Components

Create `kicad-wasm/core/CMakeLists.txt` targeting:

| Component | Source Location | Notes |
|-----------|-----------------|-------|
| Math/Geometry | `libs/kimath/` | Pure C++, no deps |
| Core utilities | `libs/core/` | Pure C++ |
| KiCad file parser | `common/io/kicad/` | Needs minimal deps |
| S-expression parser | `libs/sexpr/` | Pure C++ |
| Board data model | `pcbnew/board*.cpp` | Extract carefully |

### 2.2 Define API Boundary

Create a C API for the core (easier Wasm interop than C++):

```cpp
// kicad-wasm/core/include/kicad_core_api.h
extern "C" {
    // File operations
    void* kicad_load_pcb(const char* data, size_t len);
    void kicad_free_pcb(void* board);
    char* kicad_serialize_pcb(void* board);

    // Query
    int kicad_get_track_count(void* board);

    // Modification
    void kicad_add_track(void* board, /* params */);
}
```

### 2.3 Build Core as Static Library

```cmake
# kicad-wasm/core/CMakeLists.txt
add_library(kicad_core STATIC
    ${KIMATH_SOURCES}
    ${SEXPR_SOURCES}
    ${IO_KICAD_SOURCES}
    api/kicad_core_api.cpp
)

target_compile_definitions(kicad_core PRIVATE
    KICAD_CORE_ONLY=1
)
```

---

## Phase 3: Compile Core to WebAssembly

**Objective**: Build `kicad_core.wasm` using Emscripten.

### 3.1 Emscripten Build

```bash
source /path/to/emsdk/emsdk_env.sh

cd kicad-wasm
mkdir build-wasm && cd build-wasm

emcmake cmake ../core \
    -DCMAKE_BUILD_TYPE=Release \
    -DKICAD_WASM_BUILD=ON

emmake make
```

### 3.2 Export Functions

```cmake
# For Emscripten
if(EMSCRIPTEN)
    set_target_properties(kicad_core PROPERTIES
        LINK_FLAGS "-s EXPORTED_FUNCTIONS='[_kicad_load_pcb,_kicad_free_pcb,...]' \
                    -s EXPORTED_RUNTIME_METHODS='[ccall,cwrap]' \
                    -s MODULARIZE=1 \
                    -s EXPORT_NAME='KicadCore'"
    )
endif()
```

### 3.3 Test Wasm Module

Create a simple test that loads a `.kicad_pcb` file:

```javascript
// test/test_core.mjs
import KicadCore from './kicad_core.js';

const core = await KicadCore();
const pcbData = fs.readFileSync('test.kicad_pcb', 'utf8');
const board = core.ccall('kicad_load_pcb', 'number', ['string', 'number'],
                          [pcbData, pcbData.length]);
console.log('Track count:', core.ccall('kicad_get_track_count', 'number', ['number'], [board]));
```

---

## Phase 4: Native App with Wasm Worker

**Objective**: Run native KiCad GUI but delegate core operations to the Wasm module.

### 4.1 Choose Wasm Runtime

Options for running Wasm in native app:
- **Wasmtime** - Rust-based, mature, good C API
- **wasm3** - Small, fast interpreter
- **WAMR** - WebAssembly Micro Runtime, lightweight

Recommend: **Wasmtime** for development (better debugging), **wasm3** for size.

### 4.2 Create Wasm Bridge

```cpp
// kicad-wasm/bridge/wasm_bridge.h
class WasmBridge {
public:
    WasmBridge(const std::string& wasmPath);

    BOARD* LoadPCB(const std::string& data);
    std::string SerializePCB(BOARD* board);
    void RunDRC(BOARD* board);

private:
    wasmtime_instance_t* m_instance;
};
```

### 4.3 Integrate with KiCad

Modify KiCad to optionally use the Wasm bridge:

```cpp
// In pcbnew loading code
#ifdef USE_WASM_CORE
    auto board = WasmBridge::Get().LoadPCB(fileContent);
#else
    auto board = IO_KICAD::Load(filename);
#endif
```

### 4.4 Validate Correctness

1. Load same PCB file via native code and via Wasm bridge
2. Compare serialized output (should be identical)
3. Run DRC via both paths, compare results
4. Profile performance difference

---

## Phase 5: Incremental Migration

**Objective**: Move more functionality to Wasm core, validate stability.

### Priority Order

1. **File I/O** - parsing and serialization
2. **DRC engine** - computationally intensive, isolated
3. **Router** - Push & Shove algorithms
4. **ERC** - schematic checks

### Validation Strategy

For each migrated component:
1. Keep both native and Wasm implementations
2. Add flag to switch between them
3. Run test suite with both
4. Benchmark performance
5. Remove native implementation once confident

---

## Milestones

| Milestone | Deliverable | Validation |
|-----------|-------------|------------|
| M1 | KiCad builds with deps disabled | Opens PCB files, basic editing works |
| M2 | Core library extracts cleanly | Compiles as standalone static lib |
| M3 | Core compiles to Wasm | Passes unit tests via Node.js |
| M4 | Native app loads Wasm worker | Can load/save PCB via Wasm bridge |
| M5 | DRC runs in Wasm | Results match native DRC |

---

## Design Decisions

### Memory Management

**Decision**: Serialize/deserialize on every operation

- Proof of concept - simplicity over performance
- Clean interface between GUI and Wasm core
- Easy to debug
- Browser-friendly (no shared memory complexity)
- Can optimize later if needed

```
GUI                           Wasm Core
 │                                │
 │──serialize(board)─────────────▶│
 │                                │──process
 │◀─────────────serialize(board)──│
 │                                │
```

### Threading Model

**Decision**: Web Workers

- Target is browser, design for it from the start
- Each heavy operation (DRC, routing) runs in dedicated worker
- Clean message-passing interface
- No shared memory complexity

```javascript
// Main thread spawns workers for heavy operations
const drcWorker = new Worker('drc-worker.js');
drcWorker.postMessage({ board: serializedBoard });
drcWorker.onmessage = (e) => updateUI(e.data.violations);
```

### Incremental Updates

**Decision**: Re-serialize entire board

- Simple for proof of concept
- Board files typically <10MB, acceptable latency
- Can add delta operations later if performance requires it

---

## Next Steps

1. Fork KiCad repo, set up as submodule
2. Write the minimal patch for optional deps
3. Create stub headers
4. Verify native build with features disabled
5. Begin core library extraction