# WebGL GAL Port - Master Plan

## Goal
Port KiCad's GAL (Graphics Abstraction Layer) from OpenGL to WebGL to enable full KiCad functionality in the browser. Use the existing 28-scenario test suite to verify visual parity between native OpenGL and WebGL implementations.

## Current State
- **GAL Test Harness**: 28 scenarios covering 100% of GAL API (~70 methods)
- **Native Test**: C++ binary using real OPENGL_GAL, outputs PNGs to `tests/gal-regression/baseline/`
- **KiCad WASM**: Already runs in browser but crashes on OpenGL compatibility issues
- **OpenGL GAL**: ~9,500 lines of C++/GLSL using mix of legacy GL and modern VBOs

## Key Decisions
- **Approach**: Copy and modify existing OpenGL GAL code
- **Scope**: Full feature parity (all 28 scenarios)
- **Location**: Develop in `tests/gal-regression/` first, move to KiCad later

## Architecture

**Two-backend test architecture:**
```
┌─────────────────────────────────────────────────────────────┐
│                   SAME 28 SCENARIO FILES                     │
│            (scenarios/*.cpp - pure GAL API calls)            │
└─────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
┌─────────────────────────────┐     ┌─────────────────────────┐
│   NATIVE TEST HARNESS       │     │   WEBGL TEST HARNESS    │
│   (gal_native_test.cpp)     │     │   (gal_webgl_test.cpp)  │
│                             │     │                         │
│   Uses: OPENGL_GAL          │     │   Uses: WEBGL_GAL       │
│   Runs: macOS native        │     │   Runs: Browser/WASM    │
└─────────────────────────────┘     └─────────────────────────┘
              │                               │
              ▼                               ▼
┌─────────────────────────────┐     ┌─────────────────────────┐
│  output/native/gal-*.png    │     │  output/webgl/gal-*.png │
└─────────────────────────────┘     └─────────────────────────┘
```

## Master Test Script: `scripts/test-gal-regression.sh`

**This is the only script we run.** Single command to build, test, and compare everything:

```bash
#!/bin/bash
# Single script to build, run, and compare both backends

# 1. BUILD BOTH
scripts/build-gal-native-test.sh
scripts/build-gal-webgl-test.sh

# 2. RUN BOTH TESTS
./tests/gal-regression/native/build/gal_native_test --output tests/gal-regression/output/native/
npx playwright test gal-webgl.spec.ts  # outputs to tests/gal-regression/output/webgl/

# 3. COMPARE (two-level)
compare_screenshots output/native/ baseline/      # Catch native regressions
compare_screenshots output/webgl/  output/native/ # Verify WebGL matches native

# 4. REPORT
# Exit 0 if all match, exit 1 if any differ
```

**Two-level comparison:**
1. **native vs baseline** → Catches if native code regressed
2. **webgl vs native** → Verifies WebGL implementation matches

**Output structure:**
```
tests/gal-regression/
├── baseline/           # Committed reference screenshots
├── output/
│   ├── native/         # Fresh native run
│   └── webgl/          # WebGL run via Playwright
```

## Phases

### Phase 1: Master Test Script & WebGL Harness Infrastructure
Create the unified test script and WASM-based test harness.

**Deliverables:**
- [x] `scripts/test-gal-regression.sh` - Master build/test/compare script
- [x] `tests/gal-regression/wasm/` directory structure
- [x] `tests/gal-regression/wasm/Makefile` - Emscripten build (use Makefile, not CMake)
- [x] `tests/gal-regression/wasm/gal_webgl_test.cpp` - WASM entry point
- [x] `tests/gal-regression/wasm/gal_webgl_test.html` - Test page with canvas
- [x] `scripts/build-gal-webgl-test.sh` - WASM build script (uses Makefile)
- [x] `tests/e2e/gal-webgl.spec.ts` - Playwright spec for screenshots

**Build Approach:**
- Use a Makefile with direct `em++` calls (like `tests/apps/Makefile.wasm`)
- Avoid `emcmake cmake` which requires Python 3.10+ (system has 3.9.6)
- Follow the same pattern as `build-wasm-test.sh`

**Verification:** Master script builds both, runs native successfully, WebGL loads empty page.

### Phase 2: WEBGL_GAL Implementation
Copy and modify OpenGL GAL to create pure WebGL implementation.

**Approach:** Start with `kicad/common/gal/opengl/opengl_gal.cpp`, then:
1. Replace legacy immediate mode (`glBegin/glEnd`) with VBO-based rendering
2. Replace GL matrix stack with glm matrices (already used internally)
3. Adapt shaders for WebGL 2.0 / GLSL ES 3.0
4. Handle WebGL-specific limitations

**Key Files:**
- [ ] `tests/gal-regression/wasm/webgl_gal.h` - Class declaration
- [ ] `tests/gal-regression/wasm/webgl_gal.cpp` - Main implementation
- [ ] `tests/gal-regression/wasm/webgl_shaders.cpp` - Shader sources
- [ ] Adapt vertex_manager, gpu_manager as needed

**Verification:** Scenario 0 (basic-lines) renders, master script compares successfully.

### Phase 3: Complete API Coverage
Implement all GAL methods to pass all 28 scenarios.

**Method Groups (incremental):**
1. Basic drawing: DrawLine, DrawSegment, DrawCircle, DrawArc
2. Shapes: DrawRectangle, DrawPolygon, DrawPolyline
3. Advanced: DrawBezier, DrawBezierArc, DrawArcSegment, DrawSegmentChain
4. State: Colors, transforms, depth testing, render targets
5. Groups: BeginGroup, EndGroup, DrawGroup, ChangeGroupColor/Depth
6. Text: DrawGlyph, DrawGlyphs, BitmapText
7. Special: DrawGrid, DrawCursor, DrawBitmap

**Verification:** Run master script after each group - all implemented scenarios match.

### Phase 4: Integration with KiCad
Move WEBGL_GAL into KiCad source and enable for browser builds.

**Deliverables:**
- [ ] Move `webgl_gal.*` to `kicad/common/gal/webgl/`
- [ ] CMake integration for Emscripten builds
- [ ] Runtime GAL selection based on platform
- [ ] KiCad WASM builds successfully with WEBGL_GAL

**Verification:** KiCad loads in browser, opens PCB, renders correctly.

## File Structure

```
tests/gal-regression/
├── baseline/              # Committed reference (from native)
├── output/
│   ├── native/            # Fresh native run
│   └── webgl/             # WebGL run via Playwright
├── native/                # Native C++ harness (existing)
│   ├── gal_native_test.cpp
│   └── ...
├── wasm/                  # WASM harness (new)
│   ├── Makefile           # Use Makefile, not CMake (avoids Python issues)
│   ├── gal_webgl_test.cpp
│   ├── gal_webgl_test.html
│   ├── webgl_gal.h
│   ├── webgl_gal.cpp
│   └── webgl_shaders.cpp
└── scenarios/             # Shared scenarios (existing)

scripts/
├── build-gal-native-test.sh   # existing
├── build-gal-webgl-test.sh    # new
└── test-gal-regression.sh     # new - MASTER SCRIPT
```

## Critical Files to Modify/Create

**New files:**
- `scripts/test-gal-regression.sh` - Master test script
- `scripts/build-gal-webgl-test.sh` - WASM build
- `tests/gal-regression/wasm/*` - All WASM harness files
- `tests/e2e/gal-webgl.spec.ts` - Playwright test

**Reference files (copy from):**
- `kicad/common/gal/opengl/opengl_gal.cpp` (3098 lines)
- `kicad/common/gal/opengl/opengl_gal.h` (614 lines)
- `kicad/common/gal/opengl/shader.cpp` (298 lines)
- `kicad/common/gal/opengl/vertex_manager.cpp` (318 lines)
- `kicad/common/gal/opengl/gpu_manager.cpp` (340 lines)
