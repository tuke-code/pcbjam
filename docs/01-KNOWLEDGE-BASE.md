# KiCad WebAssembly Port - Knowledge Base

## Architecture Overview

KiCad is a modular EDA suite with these main components:
- **Project Manager** (`kicad/`) - launches other tools
- **PCB Editor** (`pcbnew/`) - board layout
- **Schematic Editor** (`eeschema/`) - circuit design
- **3D Viewer** (`3d-viewer/`) - 3D visualization

Shared code lives in:
- `common/` - shared library (kicommon), includes GUI, I/O, git, curl
- `libs/core/` - utilities
- `libs/kimath/` - geometry and math (pure C++, no deps)

## Key Dependencies

| Dependency | Purpose | Wasm Strategy |
|------------|---------|---------------|
| **wxWidgets** | All GUI | Keep native initially; replace with web UI later |
| **OpenGL** | 2D rendering via GAL | WebGL (Emscripten handles this) |
| **Cairo** | Fallback 2D rendering | Compile with Emscripten or skip |
| **OpenCASCADE** | STEP import/export, 3D | Disable for MVP; huge (~40MB) |
| **libcurl** | PCM, update check, HTTP libs | Stub out; replace with Fetch API |
| **libgit2** | Version control integration | Stub out; optional feature |
| **ngspice** | Circuit simulation | Stub out; separate concern |
| **nanoodbc** | Database libraries | Stub out; enterprise feature |
| **Freetype/HarfBuzz** | Font rendering | Compile with Emscripten (works) |
| **Boost** | Locale, unit tests | Minimize; locale needed for nanoodbc only |

## Graphics Abstraction Layer (GAL)

Location: `common/gal/`, `include/gal/`

KiCad abstracts rendering through GAL with two backends:
- `OPENGL_GAL` (`common/gal/opengl/`) - primary, uses GLSL shaders
- `CAIRO_GAL` (`common/gal/cairo/`) - fallback, vector graphics

Key files:
- `include/gal/graphics_abstraction_layer.h` - base interface
- `common/gal/opengl/opengl_gal.cpp` - OpenGL implementation
- `common/gal/shaders/` - GLSL shaders (need ES conversion for WebGL)

For Wasm: OpenGL ES subset via Emscripten maps to WebGL. Shaders need `#version` removal and precision qualifiers.

## File I/O System

Location: `common/io/`, `include/io/`

Plugin-based architecture supporting multiple formats:
- KiCad native (`.kicad_pcb`, `.kicad_sch`)
- Eagle, Altium, CADSTAR, EasyEDA imports

Key class: `IO_BASE` in `include/io/io_base.h`

Parsers are mostly pure C++ - good candidates for Wasm core.

## Python Scripting

Location: `scripting/`, `pcbnew/python/`

Uses SWIG to generate CPython bindings (~3,500 lines of `.i` files).

For Wasm: SWIG bindings won't work. Options:
1. Use Emscripten's `embind` instead
2. Use Pyodide with custom FFI
3. Skip Python for MVP

## Optional Features to Disable

These have minimal impact on core editing functionality:

| Feature | CMake Area | Files |
|---------|-----------|-------|
| Git integration | `common/git/` | 15 handler files |
| Network (PCM, updates) | `common/kicad_curl/`, `common/http_lib/` | ~5 files |
| Database libraries | `common/database/` | 2-3 files |
| STEP/3D export | `pcbnew/exporters/step/`, `plugins/3d/oce/` | Isolated |
| SPICE simulation | `eeschema/sim/` | Isolated subsystem |

## Build System Notes

Main CMake: `CMakeLists.txt`

Currently all deps are REQUIRED (lines 820-892). No options exist to disable curl/git/OCC.

Libraries link in `common/CMakeLists.txt:316-343`:
```cmake
target_link_libraries( kicommon
    CURL::libcurl
    ${LIBGIT2_LIBRARIES}
    ...
)
```

## IPC/Communication

`KIWAY` system (`include/kiway.h`, `include/kiway_holder.h`) handles inter-frame communication. Frames inherit from `KIWAY_PLAYER`.

For Wasm worker architecture: This could be adapted for message-passing between native GUI and Wasm worker.

## Potential Problem Areas

1. **Threading**: KiCad uses threads for DRC, rendering. Wasm has Web Workers but different threading model.

2. **File dialogs**: `wxFileDialog` throughout - needs abstraction for browser File API.

3. **Memory**: Large boards can use 1GB+. Wasm has 4GB limit, but browser tabs may have lower practical limits.

4. **Clipboard**: Native clipboard integration in multiple places.

5. **Printing**: Cairo-based printing system won't work in browser.

## Files of Interest for Core Extraction

Pure computation, no GUI deps - good Wasm candidates:
- `libs/kimath/` - all geometry code
- `libs/core/` - utilities
- `common/io/kicad/` - native format parser
- `pcbnew/router/` - Push & Shove routing algorithms
- `pcbnew/drc/` - Design Rule Check engine
- `eeschema/erc/` - Electrical Rule Check

## Reference Paths

| Component | Path |
|-----------|------|
| Main CMake | `CMakeLists.txt` |
| Common library | `common/CMakeLists.txt` |
| GAL system | `common/gal/`, `include/gal/` |
| File I/O | `common/io/` |
| PCB data model | `pcbnew/board.h`, `pcbnew/footprint.h` |
| Git integration | `common/git/` |
| Curl wrapper | `common/kicad_curl/` |
| Python bindings | `scripting/`, `pcbnew/python/swig/` |
| 3D/STEP | `pcbnew/exporters/step/`, `3d-viewer/` |
