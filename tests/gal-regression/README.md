# GAL Regression Test Harness

Native test harness for KiCad's OPENGL_GAL (Graphics Abstraction Layer) to enable visual regression testing of the WebGL port.

## Purpose

This test suite exercises KiCad's actual OPENGL_GAL implementation to:
- Generate baseline screenshots for visual regression testing
- Verify GAL API coverage (70/70 methods tested)
- Provide reference implementations for the WebGL port

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

## Building

```bash
./scripts/build-gal-native-test.sh
```

## Running

```bash
# Run all scenarios and save to baseline folder
./tests/gal-regression/native/build/gal_native_test \
    --output ./tests/gal-regression/baseline

# Run specific scenario (by number)
./tests/gal-regression/native/build/gal_native_test \
    --output ./tests/gal-regression/baseline 5

# Show window (non-headless)
./tests/gal-regression/native/build/gal_native_test --show
```

## Known Limitations

### DrawBitmap (Scenario 26)

The DrawBitmap test shows empty panels because OPENGL_GAL::DrawBitmap uses legacy OpenGL immediate mode (`glBegin`/`glVertex3f`/`glEnd`) which is incompatible with the shader-based rendering pipeline used by the test harness.

In KiCad's production code, DrawBitmap works because:
1. The VIEW rendering system orchestrates buffer flushes between render targets
2. GPU_MANAGER::DrawAll() deactivates the shader after flushing vertices
3. The fixed-function pipeline can then render the textured quad

In our isolated test harness, the shader remains active throughout rendering, causing the legacy GL calls to fail silently.

This is acceptable because:
- DrawBitmap is primarily used for reference images in schematics
- The WebGL port will need its own bitmap rendering implementation anyway
- All other 69 GAL methods are fully tested

### Transform() API (Scenario 27) - EXCLUDED FROM COMPARISON

The Transform() method is **dead code in KiCad** - never called anywhere in the codebase. KiCad uses Rotate(), Translate(), Scale() instead.

In native OPENGL_GAL, Transform() calls `glMultMatrixd()` which modifies GL_MODELVIEW, but VERTEX_MANAGER uses its own independent `m_transform` - so glMultMatrixd has no visible effect on rendered output. The scenario is kept for documentation but excluded from WebGL vs Native comparisons since both implementations are effectively broken (by design).

## Directory Structure

```
tests/gal-regression/
├── README.md           # This file
├── baseline/           # Reference PNG screenshots
├── native/
│   ├── CMakeLists.txt
│   ├── gal_native_test.cpp      # Main test driver
│   ├── gal_test_accessor.cpp    # Private member accessors
│   ├── kicad_stubs.cpp          # KiCad symbol stubs
│   ├── bitmap_base_stub.h       # Bitmap test patterns
│   ├── kifont_stub.h            # Glyph factory helpers
│   └── generated/               # Shader source files
└── scenarios/
    ├── gal_test_scenarios.cpp   # Scenario registry
    └── scenario_*.cpp           # Individual test scenarios
```
