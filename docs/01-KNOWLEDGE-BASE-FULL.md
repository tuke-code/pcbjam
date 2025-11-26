# KiCad WebAssembly Port - Complete Knowledge Base

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Dependencies Deep Dive](#dependencies-deep-dive)
3. [Graphics System (GAL)](#graphics-system-gal)
4. [File I/O System](#file-io-system)
5. [Python Scripting](#python-scripting)
6. [Build System](#build-system)
7. [Features to Disable](#features-to-disable)
8. [Code Extraction Candidates](#code-extraction-candidates)
9. [Technical Challenges](#technical-challenges)
10. [Design Decisions](#design-decisions)
11. [File Reference](#file-reference)

---

## Architecture Overview

### Main Applications

| Application | Directory | Purpose |
|-------------|-----------|---------|
| `kicad` | `kicad/` | Project manager, launcher |
| `pcbnew` | `pcbnew/` | PCB layout editor |
| `eeschema` | `eeschema/` | Schematic capture |
| `gerbview` | `gerbview/` | Gerber file viewer |
| `cvpcb` | `cvpcb/` | Component-footprint association |
| `bitmap2component` | `bitmap2component/` | Image to footprint converter |
| `pcb_calculator` | `pcb_calculator/` | Engineering calculators |
| `pagelayout_editor` | `pagelayout_editor/` | Drawing sheet editor |
| `kicad-cli` | `kicad/cli/` | Command-line interface |

### Shared Libraries

| Library | Location | Purpose |
|---------|----------|---------|
| `kicommon` | `common/` | Shared UI, I/O, utilities (SHARED lib) |
| `kigal` | `common/gal/` | Graphics Abstraction Layer (SHARED lib) |
| `kimath` | `libs/kimath/` | Geometry and math (STATIC lib) |
| `core` | `libs/core/` | Base utilities (STATIC lib) |
| `sexpr` | `libs/sexpr/` | S-expression parser (STATIC lib) |
| `kiplatform` | `libs/kiplatform/` | Platform abstraction |

### Plugin Architecture (KIFACE)

Each major application is built as both:
- Standalone executable
- KIFACE module (`.kiface` on Linux/Mac, `.dll` on Windows)

This allows:
- Standalone operation
- Project manager integration via dynamic loading

Key files:
- `include/kiway.h` - KIWAY system for inter-module communication
- `include/kiway_holder.h` - Mixin for frames that participate in KIWAY
- `include/kiway_player.h` - Frame base class (`KIWAY_PLAYER : public wxFrame`)

### Frame Hierarchy

```
wxFrame
  └── KIWAY_PLAYER (include/kiway_player.h)
        └── EDA_BASE_FRAME (include/eda_base_frame.h)
              ├── PCB_BASE_FRAME (pcbnew/)
              │     └── PCB_EDIT_FRAME
              ├── SCH_BASE_FRAME (eeschema/)
              │     └── SCH_EDIT_FRAME
              └── ...other frames
```

---

## Dependencies Deep Dive

### wxWidgets (GUI Framework)

**Version**: 3.2.0+
**Components used**: `gl aui adv html core net base propgrid xml stc richtext webview`
**Location**: Found via `find_package(wxWidgets)` in `CMakeLists.txt:1089`

KiCad requires GTK3 port on Linux. All GUI code depends on wxWidgets.

Key wxWidgets classes used:
- `wxFrame`, `wxDialog`, `wxPanel` - windows
- `wxGLCanvas` - OpenGL context
- `wxFileDialog` - file selection
- `wxAuiManager` - dockable panes
- `wxPropertyGrid` - property editors

**Wasm strategy**: Keep native initially. Eventually replace with web framework (React/Vue/Svelte).

### OpenGL / GLEW

**Purpose**: Hardware-accelerated 2D rendering via GAL
**Location**: `common/gal/opengl/`

Uses OpenGL 2.1+ with shaders. GLEW handles extension loading.

**Wasm strategy**: Emscripten maps OpenGL ES to WebGL automatically. Need to:
- Use OpenGL ES subset
- Convert shaders to GLSL ES (remove `#version`, add `precision` qualifiers)

### Cairo

**Version**: 1.12+
**Purpose**: Software 2D rendering fallback, printing, PDF export
**Location**: `common/gal/cairo/`

**Wasm strategy**: Can compile Cairo with Emscripten. Or skip for MVP (OpenGL-only).

### OpenCASCADE (OCC/OCCT)

**Version**: 7.5.0+
**Purpose**:
- STEP file import/export (mechanical CAD interchange)
- 3D model loading for component visualization
- Boolean operations on 3D geometry

**Files using OCC**:
- `pcbnew/exporters/step/step_pcb_model.cpp` - STEP export
- `pcbnew/exporters/step/exporter_step.cpp` - Export orchestration
- `plugins/3d/oce/loadmodel.cpp` - 3D model loading
- `plugins/3d/oce/oce.cpp` - Plugin entry point

**Headers imported** (from `step_pcb_model.cpp`):
```cpp
#include <IGESCAFControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TopoDS.hxx>
#include <XCAFDoc_ShapeTool.hxx>
// ... ~30 more OCC headers
```

**Wasm strategy**: Disable for MVP. OCC is huge (~40MB compiled). Alternative: [opencascade.js](https://github.com/nicholaseasmith/opencascade.js) exists but experimental.

### libcurl

**Purpose**: HTTP requests for:
1. Plugin Content Manager (PCM) - `kicad/pcm/pcm.cpp`
2. Update checker - `kicad/update_manager.cpp`
3. HTTP component libraries - `common/http_lib/http_lib_connection.cpp`

**Wrapper**: `common/kicad_curl/kicad_curl_easy.cpp` (~400 lines)

**Key class**: `KICAD_CURL_EASY`
```cpp
class KICAD_CURL_EASY {
    void SetURL(const std::string& url);
    void Perform();
    std::string GetBuffer();
};
```

**Wasm strategy**: Stub out. Replace with Emscripten Fetch API or JavaScript fetch via embind.

### libgit2

**Version**: 1.5+
**Purpose**: Built-in version control for projects

**Files** (`common/git/`):
| File | Purpose |
|------|---------|
| `git_clone_handler.cpp` | Clone repositories |
| `git_commit_handler.cpp` | Create commits |
| `git_push_handler.cpp` | Push to remote |
| `git_pull_handler.cpp` | Pull from remote |
| `git_branch_handler.cpp` | Branch management |
| `git_status_handler.cpp` | Status display |
| `git_revert_handler.cpp` | Revert changes |
| `kicad_git_common.cpp` | Common utilities |
| `kigit_pcb_merge.cpp` | Custom PCB merge driver |

**UI integration**:
- Project tree shows git status icons
- Menus for git operations
- Conflict resolution dialogs

**Wasm strategy**: Stub out entirely. Optional feature. Could use isomorphic-git in browser later.

### ngspice

**Purpose**: SPICE circuit simulation in eeschema
**Location**: `eeschema/sim/`

**Wasm strategy**: Stub out. Simulation is a separate concern. Could compile ngspice to Wasm later.

### nanoodbc (ODBC)

**Purpose**: Database Libraries feature - fetch component data from SQL databases
**Location**: `common/database/database_connection.cpp`

**What it does**: Connects to external databases (MySQL, PostgreSQL, SQLite, SQL Server) to fetch component information instead of using local `.kicad_sym` files.

**Enterprise feature** - most users don't use this.

**Wasm strategy**: Stub out. Would need REST API backend in browser.

### Boost

**Version**: 1.71.0+
**Components**: `locale`, `unit_test_framework`

`boost::locale` is used by nanoodbc for Unicode handling.

**Wasm strategy**: Minimize. If we disable database libraries, we may not need boost::locale.

### Freetype / HarfBuzz / Fontconfig

**Purpose**: Text rendering with outline fonts
**Versions**: Freetype 2.11.1+, HarfBuzz (any), Fontconfig (any)

**Location**: `common/font/`

**Wasm strategy**: Compile with Emscripten. These work. May need to bundle fonts or use browser fonts.

### Protobuf

**Purpose**: IPC API for external tool integration
**Location**: `api/`

**Wasm strategy**: Can compile Protobuf to Wasm. Or stub out IPC API for MVP.

---

## Graphics System (GAL)

### Architecture

```
┌─────────────────────────────────────────┐
│              VIEW (common/view/)         │
│  Manages what's visible, handles zoom   │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│      PAINTER (include/gal/painter.h)     │
│  Converts board objects to draw calls   │
└─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────┐
│        GAL (graphics_abstraction_layer)  │
│  Abstract interface for drawing         │
└─────────────────────────────────────────┘
          │                    │
          ▼                    ▼
┌─────────────────┐  ┌─────────────────┐
│   OPENGL_GAL    │  │    CAIRO_GAL    │
│ (Hardware accel)│  │ (Software/print)│
└─────────────────┘  └─────────────────┘
```

### GAL Base Class

**File**: `include/gal/graphics_abstraction_layer.h`

```cpp
namespace KIGFX {
class GAL : public GAL_DISPLAY_OPTIONS_OBSERVER {
    // Primitives
    virtual void DrawLine(const VECTOR2D& start, const VECTOR2D& end);
    virtual void DrawCircle(const VECTOR2D& center, double radius);
    virtual void DrawArc(const VECTOR2D& center, double radius, ...);
    virtual void DrawRectangle(const VECTOR2D& start, const VECTOR2D& end);
    virtual void DrawPolygon(const std::deque<VECTOR2D>& points);

    // State
    virtual void SetFillColor(const COLOR4D& color);
    virtual void SetStrokeColor(const COLOR4D& color);
    virtual void SetLineWidth(float width);

    // Transformations
    virtual void Transform(const MATRIX3x3D& matrix);
    virtual void Translate(const VECTOR2D& translation);
    virtual void Scale(const VECTOR2D& scale);
    virtual void Rotate(double angle);

    // Layers
    virtual void SetLayerDepth(double depth);
};
}
```

### OpenGL GAL

**Files**:
- `include/gal/opengl/opengl_gal.h`
- `common/gal/opengl/opengl_gal.cpp`
- `common/gal/opengl/shader.cpp` - GLSL shader management
- `common/gal/opengl/vertex_manager.cpp` - Vertex buffer management
- `common/gal/opengl/gpu_manager.cpp` - GPU memory management
- `common/gal/opengl/cached_container.cpp` - Geometry caching

**Canvas**: `HIDPI_GL_CANVAS` wraps `wxGLCanvas`

**Shader files** (`common/gal/shaders/`):
| File | Purpose |
|------|---------|
| `kicad_vert.glsl` | Main vertex shader |
| `kicad_frag.glsl` | Main fragment shader |
| `smaa_base.glsl` | SMAA antialiasing base |
| `smaa_pass_1_frag.glsl` | SMAA edge detection |
| `smaa_pass_2_frag.glsl` | SMAA blending weights |
| `smaa_pass_3_frag.glsl` | SMAA neighborhood blending |

Shaders are embedded as C strings at build time.

**For WebGL**: Need to convert shaders:
```glsl
// Before (desktop GLSL)
#version 120
varying vec4 color;

// After (WebGL/GLSL ES)
precision mediump float;
varying vec4 color;
```

### Cairo GAL

**Files**:
- `include/gal/cairo/cairo_gal.h`
- `common/gal/cairo/cairo_gal.cpp`
- `common/gal/cairo/cairo_compositor.cpp` - Layer compositing
- `common/gal/cairo/cairo_print.cpp` - Printing support

Used for:
- Software rendering fallback
- Printing
- PDF/SVG export

### Draw Panel

**File**: `include/class_draw_panel_gal.h`

`EDA_DRAW_PANEL_GAL` wraps GAL and handles:
- Mouse events
- Keyboard events
- Tool dispatching
- View management

---

## File I/O System

### Architecture

Plugin-based system with base class `IO_BASE`:

**File**: `include/io/io_base.h`

```cpp
class IO_BASE {
    struct IO_FILE_DESC {
        wxString m_Description;
        std::vector<std::string> m_FileExtensions;
        bool m_CanRead;
        bool m_CanWrite;
    };

    virtual std::vector<IO_FILE_DESC> GetFileDescriptors();
    virtual void SetReporter(REPORTER* reporter);
    virtual void SetProgressReporter(PROGRESS_REPORTER* reporter);
};
```

### Format Plugins

**KiCad Native** (`common/io/kicad/`):
- S-expression based format
- `.kicad_pcb`, `.kicad_sch`, `.kicad_sym`, `.kicad_mod`

**Import Plugins** (`common/io/`):
| Plugin | Location | Formats |
|--------|----------|---------|
| Eagle | `common/io/eagle/` | `.brd`, `.sch` |
| Altium | `common/io/altium/` | `.PcbDoc`, `.SchDoc` |
| CADSTAR | `common/io/cadstar/` | `.cpa`, `.csa` |
| EasyEDA | `common/io/easyeda/` | `.json` |
| EasyEDA Pro | `common/io/easyedapro/` | `.epro` |

### S-Expression Parser

**Location**: `libs/sexpr/`

KiCad native files use S-expressions:
```lisp
(kicad_pcb (version 20221018)
  (generator pcbnew)
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal))
  (footprint "Package_SO:SOIC-8"
    (at 100 100)
    (pad "1" smd rect (at -1.905 -2.475) (size 0.6 1.5))))
```

Parser is pure C++, good candidate for Wasm.

### PCB Data Model

**Key classes** (in `pcbnew/`):

| Class | File | Purpose |
|-------|------|---------|
| `BOARD` | `board.h` | Top-level PCB container |
| `FOOTPRINT` | `footprint.h` | Component footprint |
| `PAD` | `pad.h` | Footprint pad |
| `PCB_TRACK` | `pcb_track.h` | Trace segment |
| `PCB_VIA` | `pcb_track.h` | Via |
| `ZONE` | `zone.h` | Copper pour |
| `PCB_SHAPE` | `pcb_shape.h` | Graphical shape |
| `PCB_TEXT` | `pcb_text.h` | Text |

**Hierarchy**:
```
BOARD
├── FOOTPRINT[]
│   ├── PAD[]
│   ├── PCB_SHAPE[]
│   └── PCB_TEXT[]
├── PCB_TRACK[]
├── PCB_VIA[]
├── ZONE[]
├── PCB_SHAPE[]
└── PCB_TEXT[]
```

---

## Python Scripting

### SWIG Bindings

**Location**: `scripting/`, `common/swig/`, `pcbnew/python/swig/`

**Interface files** (`.i`):
| File | Lines | Purpose |
|------|-------|---------|
| `kicadplugins.i` | 695 | Plugin framework |
| `wx.i` | 348 | wxWidgets types |
| `board.i` | 201 | BOARD class |
| `board_item.i` | 222 | Base item class |
| `footprint.i` | 200 | FOOTPRINT class |
| `pad.i` | 128 | PAD class |
| `pcbnew.i` | 148 | Main pcbnew module |
| Others | ~1600 | Various classes |
| **Total** | ~3500 | |

### How SWIG Works

1. SWIG reads `.i` interface files
2. Generates C++ wrapper code for CPython
3. Wrapper compiled to shared library (`_pcbnew.so`)
4. Python imports the module

```python
import pcbnew
board = pcbnew.GetBoard()
for track in board.GetTracks():
    print(track.GetStart(), track.GetEnd())
```

### Plugin Types

| Type | Purpose | Interface |
|------|---------|-----------|
| FootprintWizard | Generate footprints programmatically | `FootprintWizardPlugin` |
| ActionPlugin | Custom toolbar actions | `ActionPlugin` |
| FilePlugin | Custom file formats | `FilePlugin` |

### Wasm Strategy

SWIG generates CPython-specific code. Options for Wasm:

1. **Emscripten embind**: Rewrite bindings (~3500 lines to port)
   ```cpp
   #include <emscripten/bind.h>
   EMSCRIPTEN_BINDINGS(pcbnew) {
       class_<BOARD>("Board")
           .function("GetTracks", &BOARD::Tracks);
   }
   ```

2. **Pyodide**: If we want Python in browser, use Pyodide with custom FFI

3. **Skip for MVP**: Python scripting is optional for basic editing

---

## Build System

### Main CMakeLists.txt Structure

```
CMakeLists.txt
├── Project setup (lines 1-100)
├── Options (lines 110-290)
│   ├── KICAD_SPICE_QA
│   ├── KICAD_USE_SENTRY
│   ├── KICAD_BUILD_I18N
│   ├── KICAD_BUILD_QA_TESTS
│   ├── KICAD_SCRIPTING_WXPYTHON
│   ├── KICAD_UPDATE_CHECK
│   └── ... more options
├── Compiler setup (lines 300-800)
├── Dependencies (lines 800-1200)
│   ├── find_package(ZLIB)
│   ├── find_package(CURL)         # Line 825 - REQUIRED
│   ├── find_package(libgit2)      # Line 842 - REQUIRED
│   ├── find_package(ngspice)      # Line 877 - REQUIRED
│   ├── find_package(OCC)          # Line 880 - FATAL if not found
│   └── ... more deps
├── wxWidgets setup (lines 1080-1140)
└── Subdirectories (lines 1250+)
```

### Common Library Build

**File**: `common/CMakeLists.txt`

```cmake
# KICOMMON_SRCS includes:
# - git/*.cpp (lines 75-95)
# - kicad_curl/*.cpp (lines 140-141)
# - database/*.cpp
# - All UI code

target_link_libraries(kicommon
    CURL::libcurl           # Line 325
    ${LIBGIT2_LIBRARIES}    # Line 330
    # ...
)
```

### Adding CMake Options for Optional Deps

Need to add after line 107:
```cmake
option( KICAD_USE_CURL "Enable network features" ON )
option( KICAD_USE_GIT "Enable git integration" ON )
option( KICAD_USE_OCC "Enable STEP/3D via OpenCASCADE" ON )
option( KICAD_USE_NGSPICE "Enable SPICE simulation" ON )
option( KICAD_USE_DATABASE "Enable database libraries" ON )
```

Then wrap find_package calls:
```cmake
if( KICAD_USE_CURL )
    find_package( CURL REQUIRED )
    add_compile_definitions( KICAD_USE_CURL )
else()
    add_library( CURL::libcurl INTERFACE IMPORTED )
endif()
```

---

## Features to Disable

### Git Integration

**Files to stub** (`common/git/`):
- `git_add_to_index_handler.cpp`
- `git_branch_handler.cpp`
- `git_clone_handler.cpp`
- `git_commit_handler.cpp`
- `git_config_handler.cpp`
- `git_compare_handler.cpp`
- `git_init_handler.cpp`
- `git_pull_handler.cpp`
- `git_push_handler.cpp`
- `git_remove_from_index_handler.cpp`
- `git_remove_vcs_handler.cpp`
- `git_resolve_conflict_handler.cpp`
- `git_revert_handler.cpp`
- `git_status_handler.cpp`
- `git_switch_branch_handler.cpp`
- `git_sync_handler.cpp`
- `kicad_git_common.cpp`
- `git_backend.cpp`
- `libgit_backend.cpp`
- `project_git_utils.cpp`

**Stub header needed**: `stubs/include/git2.h`

### Network Features (curl)

**Files to stub** (`common/kicad_curl/`):
- `kicad_curl.cpp`
- `kicad_curl_easy.cpp`

**Files to stub** (`common/http_lib/`):
- `http_lib_connection.cpp`

**Files affected** (`kicad/`):
- `pcm/pcm.cpp` - Plugin Content Manager
- `pcm/pcm_task_manager.cpp`
- `update_manager.cpp`

**Stub header needed**: `stubs/include/curl/curl.h`

### OpenCASCADE (STEP/3D)

**Files to exclude**:
- `pcbnew/exporters/step/*.cpp`
- `plugins/3d/oce/*.cpp`

**Approach**: Don't build these targets rather than stubbing.

### SPICE Simulation

**Files affected**: `eeschema/sim/`

**Approach**: Disable simulator UI, don't build sim targets.

### Database Libraries

**Files to stub** (`common/database/`):
- `database_connection.cpp`
- `database_cache.cpp`

---

## Code Extraction Candidates

### Tier 1: Pure Computation (No Dependencies)

| Component | Location | Lines (approx) | Notes |
|-----------|----------|----------------|-------|
| Math library | `libs/kimath/src/` | ~5000 | Vectors, matrices, geometry |
| Core utilities | `libs/core/src/` | ~2000 | String utils, exceptions |
| S-expr parser | `libs/sexpr/` | ~1500 | Pure parsing |

### Tier 2: File I/O (Minimal Dependencies)

| Component | Location | Notes |
|-----------|----------|-------|
| KiCad PCB parser | `common/io/kicad/` | Needs kimath, sexpr |
| Board data model | `pcbnew/*.cpp` | Core classes only |
| Schematic parser | `common/io/eeschema/` | Needs kimath |

### Tier 3: Algorithms (May Need Adaptation)

| Component | Location | Notes |
|-----------|----------|-------|
| DRC engine | `pcbnew/drc/` | May use threading |
| Router | `pcbnew/router/` | Push & Shove |
| ERC | `eeschema/erc/` | Electrical checks |
| Connectivity | `pcbnew/connectivity/` | Net analysis |

---

## Technical Challenges

### Threading

**Current usage**:
- DRC runs checks in parallel
- Router uses threading for optimization
- Background jobs system

**Wasm limitation**: Single-threaded by default.

**Solutions**:
1. **Web Workers**: Spawn separate Wasm instances
2. **Wasm threads** (experimental): SharedArrayBuffer + pthreads
3. **Sequential fallback**: Slower but works

**Decision**: Use Web Workers. Design for async message-passing.

### Memory Management

**Challenge**: Need to share board state between native GUI and Wasm core.

**Options**:
1. **Serialize/deserialize**: Simple, proof of concept
2. **Shared memory**: Complex, requires careful synchronization
3. **Authoritative Wasm copy**: GUI requests views

**Decision**: Serialize/deserialize. Proof of concept, simplicity over performance.

### File Access

**Native**: Direct filesystem, `wxFileDialog`

**Browser**: No filesystem access without user interaction

**Solutions**:
- Emscripten virtual filesystem (MEMFS, IDBFS)
- File System Access API (Chrome)
- IndexedDB for persistence
- Drag & drop / file picker

### Clipboard

**Native**: `wxClipboard`, platform integration

**Browser**: Async Clipboard API (permissions required)

### Fonts

**Native**: System fonts via Fontconfig

**Browser**: Bundle fonts or use CSS fonts

---

## Design Decisions

### Memory Management

**Decision**: Serialize/deserialize on every operation

**Rationale**:
- Proof of concept phase
- Simplicity over performance
- Clean interface between GUI and core
- Easy to debug
- Browser-friendly (no shared memory complexity)

**Implementation**:
```cpp
// GUI → Wasm: Send operation + serialized state
std::string boardJson = SerializeBoard(board);
wasmCore.ApplyOperation(boardJson, operation);
std::string newBoardJson = wasmCore.GetBoardState();
board = DeserializeBoard(newBoardJson);
```

### Threading Model

**Decision**: Web Workers

**Rationale**:
- Target is browser
- Clean message-passing interface
- Each worker is isolated Wasm instance
- No shared memory complexity

**Implementation**:
```javascript
// Main thread
const worker = new Worker('kicad-core-worker.js');
worker.postMessage({ type: 'runDRC', board: boardData });
worker.onmessage = (e) => { handleDRCResults(e.data); };

// Worker
importScripts('kicad_core.js');
onmessage = async (e) => {
    const core = await KicadCore();
    if (e.data.type === 'runDRC') {
        const results = core.runDRC(e.data.board);
        postMessage(results);
    }
};
```

### Incremental Updates

**Decision**: Re-serialize entire board (for now)

**Rationale**:
- Simple implementation
- Board files are typically <10MB
- Performance acceptable for proof of concept
- Can optimize later with deltas if needed

---

## File Reference

### Core Headers

| Purpose | File |
|---------|------|
| GAL interface | `include/gal/graphics_abstraction_layer.h` |
| OpenGL GAL | `include/gal/opengl/opengl_gal.h` |
| Cairo GAL | `include/gal/cairo/cairo_gal.h` |
| Draw panel | `include/class_draw_panel_gal.h` |
| Base frame | `include/eda_base_frame.h` |
| KIWAY | `include/kiway.h` |
| KIWAY holder | `include/kiway_holder.h` |
| I/O base | `include/io/io_base.h` |
| Board | `pcbnew/board.h` |
| Footprint | `pcbnew/footprint.h` |
| Track | `pcbnew/pcb_track.h` |

### Build Files

| Purpose | File |
|---------|------|
| Main build | `CMakeLists.txt` |
| Common lib | `common/CMakeLists.txt` |
| GAL lib | `common/gal/CMakeLists.txt` |
| PCBnew | `pcbnew/CMakeLists.txt` |
| Eeschema | `eeschema/CMakeLists.txt` |
| 3D viewer | `3d-viewer/CMakeLists.txt` |

### SWIG Bindings

| Purpose | File |
|---------|------|
| Main entry | `scripting/kicadplugins.i` |
| Common | `common/swig/kicad.i` |
| wxWidgets | `common/swig/wx.i` |
| Math | `common/swig/math.i` |
| Shapes | `common/swig/shape.i` |
| Board | `pcbnew/python/swig/board.i` |
| PCBnew main | `pcbnew/python/swig/pcbnew.i` |

### Git Integration

| Purpose | File |
|---------|------|
| Common class | `common/git/kicad_git_common.cpp` |
| Clone | `common/git/git_clone_handler.cpp` |
| Commit | `common/git/git_commit_handler.cpp` |
| Push | `common/git/git_push_handler.cpp` |
| Pull | `common/git/git_pull_handler.cpp` |
| Status | `common/git/git_status_handler.cpp` |
| PCB merge | `pcbnew/git/kigit_pcb_merge.cpp` |

### Curl/Network

| Purpose | File |
|---------|------|
| Curl wrapper | `common/kicad_curl/kicad_curl_easy.cpp` |
| Curl init | `common/kicad_curl/kicad_curl.cpp` |
| HTTP lib | `common/http_lib/http_lib_connection.cpp` |
| PCM | `kicad/pcm/pcm.cpp` |
| Updates | `kicad/update_manager.cpp` |

### OpenCASCADE/STEP

| Purpose | File |
|---------|------|
| STEP model | `pcbnew/exporters/step/step_pcb_model.cpp` |
| STEP export | `pcbnew/exporters/step/exporter_step.cpp` |
| OCC plugin | `plugins/3d/oce/oce.cpp` |
| Model loader | `plugins/3d/oce/loadmodel.cpp` |
