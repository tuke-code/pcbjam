/**
 * GAL WebGL Test - WASM Entry Point
 *
 * This test harness renders GAL test scenarios using WEBGL_GAL
 * and allows Playwright to capture screenshots for comparison against native.
 */

// Include kiglew.h first to set GLEW guard before anything includes <GL/glew.h>
#include "webgl/kiglew.h"

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include <emscripten.h>
#include <emscripten/html5.h>

// KiCad GAL headers
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2d.h>
#include "webgl/webgl_gal.h"

// Test scenarios (shared with native test)
#include "gal_test_scenarios.h"

// Stubs
#include "kicad_stubs.h"

#include <cstdio>

// Canvas dimensions (matching native test)
static const int CANVAS_WIDTH = 800;
static const int CANVAS_HEIGHT = 600;

// Global state
static int g_currentScenario = -1;
static int g_totalScenarios = 28;
static KIGFX::WEBGL_GAL* g_gal = nullptr;
static wxFrame* g_frame = nullptr;

/**
 * Render the current scenario
 */
void renderCurrentScenario() {
    if (!g_gal || g_currentScenario < 0) {
        return;
    }

    printf("Rendering scenario %d: %s\n", g_currentScenario,
           GALTest::GetScenarioName(g_currentScenario));

    // Begin drawing
    g_gal->BeginDrawing();

    // Clear the screen
    g_gal->ClearScreen();

    // Render the scenario
    GALTest::RenderScenario(g_gal, g_currentScenario, CANVAS_WIDTH, CANVAS_HEIGHT);

    // End drawing and present
    g_gal->EndDrawing();

    printf("Scenario %d rendered\n", g_currentScenario);
}

/**
 * C API for JavaScript calls
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
    renderCurrentScenario();

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
 * wxWidgets Application
 */
class GALTestApp : public wxApp {
public:
    virtual bool OnInit() override;
};

wxIMPLEMENT_APP_NO_MAIN(GALTestApp);

bool GALTestApp::OnInit() {
    printf("GAL WebGL Test - Initializing wxWidgets\n");

    // Create a frame (invisible, just hosts the GL canvas)
    g_frame = new wxFrame(nullptr, wxID_ANY, "GAL WebGL Test",
                          wxDefaultPosition, wxSize(CANVAS_WIDTH, CANVAS_HEIGHT));

    // Create display options
    KIGFX::GAL_DISPLAY_OPTIONS displayOptions;
    KIGFX::VC_SETTINGS vcSettings;

    printf("Creating WEBGL_GAL instance...\n");

    try {
        g_gal = new KIGFX::WEBGL_GAL(
            vcSettings,
            displayOptions,
            g_frame,            // parent window
            nullptr,            // mouse listener
            nullptr,            // paint listener
            "GAL WebGL Test"    // name
        );
    } catch (const std::exception& e) {
        printf("ERROR: Failed to create WEBGL_GAL: %s\n", e.what());
        return false;
    }

    printf("WEBGL_GAL created successfully\n");

    // Set up the GAL
    g_gal->SetScreenSize(VECTOR2I(CANVAS_WIDTH, CANVAS_HEIGHT));
    g_gal->ResizeScreen(CANVAS_WIDTH, CANVAS_HEIGHT);

    // Set default world coordinates (matching native test)
    double worldScale = 1.0 / 10000.0;  // Convert nm to screen units
    g_gal->SetWorldUnitLength(worldScale);
    g_gal->SetScreenDPI(96);

    // Set world boundaries
    VECTOR2D worldSize(CANVAS_WIDTH / worldScale, CANVAS_HEIGHT / worldScale);
    BOX2D worldBounds(VECTOR2D(0, 0), worldSize);
    g_gal->SetWorldScreenMatrix(g_gal->GetWorldScreenMatrix());

    // Initialize the compositor
    g_gal->BeginDrawing();
    g_gal->ClearScreen();
    g_gal->EndDrawing();

    printf("\nReady for scenarios. Total: %d\n", g_totalScenarios);
    printf("Call runScenario(index) from JavaScript to render.\n");

    return true;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    printf("GAL WebGL Test\n");
    printf("==============\n\n");

    // Initialize wxWidgets
    wxEntryStart(argc, argv);

    if (!wxTheApp->OnInit()) {
        printf("ERROR: wxApp initialization failed\n");
        return 1;
    }

    // Don't exit - keep runtime alive for JavaScript calls
    emscripten_exit_with_live_runtime();

    return 0;
}
