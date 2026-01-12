# GAL Regression Test Suite

Visual regression testing for KiCad's Graphics Abstraction Layer (GAL), covering both native OpenGL and WebGL implementations.

## Overview

This test suite validates KiCad's GAL rendering by:
- Running 28 test scenarios that exercise all 70 GAL API methods
- Comparing native OpenGL output against committed baselines
- Comparing WebGL WASM output against native (for parity verification)
- Detecting rendering regressions during development

## WebGL GAL Integration

The WebGL GAL implementation lives in **`kicad/common/gal/webgl/`** (~27,800 lines) and is a full port of KiCad's OPENGL_GAL to WebGL 2.0 / OpenGL ES 3.0.

Key changes from native OpenGL:
- GLSL ES 3.0 shaders (`attribute`→`in`, `varying`→`out`, `texture2D()`→`texture()`)
- VAOs required (WebGL 2.0 requirement)
- No legacy GL (`glBegin/glEnd` replaced with VBOs)
- GLU tesselator replaced with earcut.hpp

The WASM test harness in `wasm/` links against KiCad's WebGL GAL to verify the implementation matches native rendering.

## Test Scripts

| Script | Purpose |
|--------|---------|
| `scripts/build-gal-native-test.sh` | Build native OpenGL test harness (macOS) |
| `scripts/build-gal-webgl-test.sh` | Build WebGL WASM test harness |
| `scripts/test-gal-regression.sh` | **Master script**: builds both, runs tests, compares native vs baseline AND webgl vs native |
| `scripts/test-gal-webgl.sh` | WebGL regression monitor: compares webgl vs baseline-webgl |

### Quick Start

```bash
# Run full regression suite (recommended)
./scripts/test-gal-regression.sh

# Run WebGL-only tests (faster, for WebGL development)
./scripts/test-gal-webgl.sh

# Build native test only
./scripts/build-gal-native-test.sh

# Build WebGL test only
./scripts/build-gal-webgl-test.sh
```

## Test Scenarios

28 scenarios covering all GAL drawing operations:

| # | Scenario | Description |
|---|----------|-------------|
| 0 | basic-lines | DrawLine with various styles |
| 1 | line-widths | SetLineWidth variations |
| 2 | circles | DrawCircle filled/stroked |
| 3 | arcs | DrawArc with different angles |
| 4 | rectangles | DrawRectangle filled/stroked |
| 5 | polygons | DrawPolygon with complex shapes |
| 6 | alpha-blending | Transparency and blending |
| 7 | transforms | Save/Restore/Translate/Rotate/Scale |
| 8 | grid-cursor | Grid and cursor rendering |
| 9 | segments | DrawSegment with endcaps |
| 10 | complex-scene | Combined operations |
| 11 | bezier-curves | DrawCurve (cubic Bezier) |
| 12 | arc-segments | DrawArcSegment with widths |
| 13 | segment-chain | DrawSegmentChain |
| 14 | group-caching | BeginGroup/EndGroup/DrawGroup |
| 15 | polylines-multi | DrawPolyline/DrawPolylines |
| 16 | hole-walls | DrawHoleWalls from SHAPE_SEGMENT |
| 17 | grid-native | DrawGrid (native grid rendering) |
| 18 | cursor-native | DrawCursor (native cursor) |
| 19 | render-targets | SetTarget/GetTarget/ClearTarget |
| 20 | screen-transform | SetScreenSize/ComputeWorldScale |
| 21 | clear-colors | ClearScreen with colors |
| 22 | depth-testing | SetLayerDepth ordering |
| 23 | negative-mode | SetNegativeDrawMode |
| 24 | text-attrs | Text attribute methods (stub) |
| 25 | glyphs | DrawGlyph/DrawGlyphs |
| 26 | bitmap | DrawBitmap (see limitation below) |
| 27 | transform-api | Transform() API documentation |

## Building & Running

### Native Test Harness

```bash
# Build
./scripts/build-gal-native-test.sh

# Run all scenarios
./tests/gal-regression/native/build/gal_native_test \
    --output ./tests/gal-regression/output/native

# Run specific scenario (by number)
./tests/gal-regression/native/build/gal_native_test \
    --output ./tests/gal-regression/output/native 5

# Show window (non-headless)
./tests/gal-regression/native/build/gal_native_test --show
```

### WebGL Test Harness

```bash
# Build
./scripts/build-gal-webgl-test.sh

# Run via Playwright (headless)
cd tests && npx playwright test gal-webgl.spec.ts
```

## Baselines

Two sets of baseline screenshots:

| Folder | Purpose |
|--------|---------|
| `baseline/` | Native OpenGL reference (28 PNGs) |
| `baseline-webgl/` | WebGL reference (29 PNGs) |

### Updating Baselines

```bash
# Update native baseline (after verifying output looks correct)
cp tests/gal-regression/output/native/*.png tests/gal-regression/baseline/

# Update WebGL baseline
cp tests/gal-regression/output/webgl/*.png tests/gal-regression/baseline-webgl/
```

## Known Limitations

### DrawBitmap (Scenario 26)

The DrawBitmap test shows empty panels because OPENGL_GAL::DrawBitmap uses legacy OpenGL immediate mode (`glBegin`/`glVertex3f`/`glEnd`) which is incompatible with the shader-based rendering pipeline used by the test harness.

In KiCad's production code, DrawBitmap works because the VIEW rendering system orchestrates buffer flushes between render targets.

This is acceptable because:
- DrawBitmap is primarily used for reference images in schematics
- The WebGL port has its own bitmap rendering implementation
- All other 69 GAL methods are fully tested

### Transform() API (Scenario 27) - EXCLUDED FROM COMPARISON

The Transform() method is **dead code in KiCad** - never called anywhere in the codebase. KiCad uses Rotate(), Translate(), Scale() instead. The scenario is kept for documentation but excluded from comparisons.

## Directory Structure

```
tests/gal-regression/
├── README.md                 # This file
├── baseline/                 # Native OpenGL reference (28 PNGs)
├── baseline-webgl/           # WebGL reference (29 PNGs)
├── output/
│   ├── native/               # Fresh native test output
│   └── webgl/                # Fresh WebGL test output
├── native/
│   ├── CMakeLists.txt
│   ├── gal_native_test.cpp   # Main test driver
│   ├── gal_test_accessor.cpp # Private member accessors
│   ├── kicad_stubs.cpp       # KiCad symbol stubs
│   ├── bitmap_base_stub.h    # Bitmap test patterns
│   ├── kifont_stub.h         # Glyph factory helpers
│   └── generated/            # Shader source files
├── wasm/
│   ├── Makefile              # WebGL WASM build
│   ├── gal_webgl_test.cpp    # WASM entry point
│   ├── gal_webgl_test.html   # Test page with canvas
│   ├── wasm_stubs.cpp        # WASM-specific stubs
│   └── generated/            # ES 3.0 shader sources
└── scenarios/
    ├── gal_test_scenarios.cpp # Scenario registry
    └── scenario_*.cpp         # Individual test scenarios (shared by native & wasm)

kicad/common/gal/webgl/       # WebGL GAL implementation (in KiCad repo)
├── webgl_gal.cpp             # Main implementation
├── webgl_gal.h               # Class declaration
├── gpu_manager.cpp           # VBO/VAO management
├── vertex_manager.cpp        # Vertex accumulation
├── shader.cpp                # GLSL ES 3.0 compilation
├── glu_tess_impl.cpp         # GLU tesselator (earcut.hpp)
└── ...                       # ~20 files total
```

## Comparison Thresholds

- **Native vs Baseline**: 0% difference (exact match expected)
- **WebGL vs Native**: ~7/28 exact matches typical (minor anti-aliasing differences acceptable)
- **WebGL vs Baseline-WebGL**: 1% threshold (catches actual regressions)
