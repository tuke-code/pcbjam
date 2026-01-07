/**
 * GAL WebGL Test - WASM Entry Point
 *
 * This is a test harness that renders GAL test scenarios using WEBGL_GAL
 * and allows Playwright to capture screenshots for comparison against native.
 *
 * Phase 1: Stub that initializes WebGL and renders a test pattern
 * Phase 2: Full WEBGL_GAL implementation with all scenarios
 */

#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstring>

// Canvas dimensions (matching native test)
static const int CANVAS_WIDTH = 800;
static const int CANVAS_HEIGHT = 600;

// Current scenario index
static int g_currentScenario = -1;
static int g_totalScenarios = 28;

// WebGL context
static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_glContext = 0;

/**
 * Initialize WebGL context
 */
bool initWebGL() {
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);

    attrs.majorVersion = 2;  // WebGL 2.0
    attrs.minorVersion = 0;
    attrs.alpha = true;
    attrs.depth = true;
    attrs.stencil = true;
    attrs.antialias = false;  // We handle AA ourselves
    attrs.premultipliedAlpha = false;
    attrs.preserveDrawingBuffer = true;  // Needed for screenshots

    g_glContext = emscripten_webgl_create_context("#canvas", &attrs);
    if (g_glContext <= 0) {
        printf("ERROR: Failed to create WebGL 2.0 context: %d\n", g_glContext);
        return false;
    }

    emscripten_webgl_make_context_current(g_glContext);

    printf("WebGL 2.0 context created successfully\n");
    printf("  GL_VENDOR: %s\n", glGetString(GL_VENDOR));
    printf("  GL_RENDERER: %s\n", glGetString(GL_RENDERER));
    printf("  GL_VERSION: %s\n", glGetString(GL_VERSION));

    return true;
}

/**
 * Render a test pattern (stub for Phase 1)
 * In Phase 2, this will be replaced with actual WEBGL_GAL rendering
 */
void renderTestPattern(int scenarioIndex) {
    // Set viewport
    glViewport(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);

    // Clear with KiCad-like dark background
    glClearColor(0.102f, 0.102f, 0.149f, 1.0f);  // RGB(26, 26, 38)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO Phase 2: Replace with actual WEBGL_GAL rendering
    // For now, just render a different colored rectangle for each scenario
    // to verify the pipeline works

    // This is a placeholder - in Phase 2 we'll call:
    // GALTest::RenderScenario(webglGal, scenarioIndex, CANVAS_WIDTH, CANVAS_HEIGHT);

    printf("Rendered scenario %d (stub)\n", scenarioIndex);
}

/**
 * Run a specific scenario
 * Called from JavaScript: Module.ccall('runScenario', 'number', ['number'], [index])
 */
extern "C" {

EMSCRIPTEN_KEEPALIVE
int runScenario(int scenarioIndex) {
    if (scenarioIndex < 0 || scenarioIndex >= g_totalScenarios) {
        printf("ERROR: Invalid scenario index %d (valid: 0-%d)\n",
               scenarioIndex, g_totalScenarios - 1);
        return -1;
    }

    g_currentScenario = scenarioIndex;
    renderTestPattern(scenarioIndex);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int getTotalScenarios() {
    return g_totalScenarios;
}

EMSCRIPTEN_KEEPALIVE
int getCurrentScenario() {
    return g_currentScenario;
}

EMSCRIPTEN_KEEPALIVE
int getCanvasWidth() {
    return CANVAS_WIDTH;
}

EMSCRIPTEN_KEEPALIVE
int getCanvasHeight() {
    return CANVAS_HEIGHT;
}

}  // extern "C"

/**
 * Main entry point
 */
int main() {
    printf("GAL WebGL Test - Phase 1 Stub\n");
    printf("============================\n\n");

    // Set canvas size
    emscripten_set_canvas_element_size("#canvas", CANVAS_WIDTH, CANVAS_HEIGHT);

    // Initialize WebGL
    if (!initWebGL()) {
        printf("Failed to initialize WebGL\n");
        return 1;
    }

    printf("\nReady for scenarios. Total: %d\n", g_totalScenarios);
    printf("Call runScenario(index) from JavaScript to render.\n");

    // Don't exit - keep runtime alive for JavaScript calls
    emscripten_exit_with_live_runtime();

    return 0;
}
