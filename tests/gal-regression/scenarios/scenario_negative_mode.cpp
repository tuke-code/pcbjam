/**
 * Negative Mode / Diff Layer Scenario
 *
 * Tests Gerber-style negative rendering concepts.
 *
 * IMPORTANT: In OPENGL_GAL these are mostly no-ops:
 * - SetNegativeDrawMode() - NO-OP (Cairo uses CAIRO_OPERATOR_CLEAR)
 * - StartNegativesLayer() / EndNegativesLayer() - NO-OP
 * - StartDiffLayer() / EndDiffLayer() - Requires m_tempBuffer (compositor setup)
 *
 * This test:
 * 1. Calls the APIs to verify they don't crash
 * 2. Demonstrates what negative mode LOOKS like (simulated with layered drawing)
 * 3. Shows PCB thermal relief patterns (common use case for negative mode)
 *
 * In Gerbview, negative objects "cut out" from the copper layer.
 * The polarity is determined by: item->GetLayerPolarity() XOR image->m_ImageNegative
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderNegativeMode(GAL* gal, int width, int height) {
    // Enable depth testing for proper layering
    gal->EnableDepthTest(true);

    //=========================================================================
    // Section 1: Simulated thermal relief (what negative mode produces)
    //=========================================================================
    // This shows the RESULT of negative mode - holes cut in copper pour

    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Dark background panel
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(380, 280));

    // Copper pour (ground plane)
    gal->SetLayerDepth(80);
    gal->SetFillColor(COLOR4D(0.7, 0.5, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(40, 40), VECTOR2D(360, 260));

    // Thermal relief pattern - simulated by drawing background color
    // (In real Gerbview, SetNegativeDrawMode(true) + draw = erase)
    gal->SetLayerDepth(60);
    COLOR4D clearColor(0.12, 0.12, 0.15, 1.0);  // "Cut through" to background
    gal->SetFillColor(clearColor);

    // First pad thermal - cross pattern
    double pad1X = 120, pad1Y = 150;
    double spokeLen = 35, spokeW = 5, clearance = 18;

    // Clearance ring
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(8.0);
    gal->SetStrokeColor(clearColor);
    gal->DrawCircle(VECTOR2D(pad1X, pad1Y), clearance);

    // Thermal spokes (horizontal and vertical)
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(clearColor);
    gal->DrawRectangle(VECTOR2D(pad1X - clearance - spokeLen, pad1Y - spokeW/2),
                       VECTOR2D(pad1X - clearance, pad1Y + spokeW/2));
    gal->DrawRectangle(VECTOR2D(pad1X + clearance, pad1Y - spokeW/2),
                       VECTOR2D(pad1X + clearance + spokeLen, pad1Y + spokeW/2));
    gal->DrawRectangle(VECTOR2D(pad1X - spokeW/2, pad1Y - clearance - spokeLen),
                       VECTOR2D(pad1X + spokeW/2, pad1Y - clearance));
    gal->DrawRectangle(VECTOR2D(pad1X - spokeW/2, pad1Y + clearance),
                       VECTOR2D(pad1X + spokeW/2, pad1Y + clearance + spokeLen));

    // Pad on top
    gal->SetLayerDepth(40);
    gal->SetFillColor(COLOR4D(0.85, 0.65, 0.25, 1.0));
    gal->DrawCircle(VECTOR2D(pad1X, pad1Y), 14);

    // Second pad thermal - diagonal spokes
    double pad2X = 280, pad2Y = 150;
    gal->SetLayerDepth(60);
    gal->SetFillColor(clearColor);

    // Diagonal thermal spokes
    for (int i = 0; i < 4; i++) {
        double angle = i * M_PI / 2 + M_PI / 4;
        double x1 = pad2X + cos(angle) * clearance;
        double y1 = pad2Y + sin(angle) * clearance;
        double x2 = pad2X + cos(angle) * (clearance + spokeLen);
        double y2 = pad2Y + sin(angle) * (clearance + spokeLen);
        gal->DrawSegment(VECTOR2D(x1, y1), VECTOR2D(x2, y2), spokeW);
    }

    // Clearance ring
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(8.0);
    gal->SetStrokeColor(clearColor);
    gal->DrawCircle(VECTOR2D(pad2X, pad2Y), clearance);

    // Pad
    gal->SetLayerDepth(40);
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.85, 0.65, 0.25, 1.0));
    gal->DrawCircle(VECTOR2D(pad2X, pad2Y), 14);

    //=========================================================================
    // Section 2: API calls test (verify they don't crash)
    //=========================================================================
    // These are NO-OPs in OpenGL but we call them to test the API

    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.15, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 20), VECTOR2D(780, 280));

    // Draw base content
    gal->SetLayerDepth(80);
    gal->SetFillColor(COLOR4D(0.6, 0.3, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(420, 40), VECTOR2D(760, 260));

    // Test SetNegativeDrawMode API (NO-OP in OpenGL)
    gal->SetNegativeDrawMode(true);

    // In Cairo, this would ERASE. In OpenGL, it draws normally.
    gal->SetLayerDepth(60);
    gal->SetFillColor(COLOR4D(0.15, 0.12, 0.15, 1.0));
    gal->DrawCircle(VECTOR2D(500, 150), 35);
    gal->DrawCircle(VECTOR2D(590, 150), 35);
    gal->DrawCircle(VECTOR2D(680, 150), 35);

    // Disable negative mode
    gal->SetNegativeDrawMode(false);

    // Draw something after to show mode was reset
    gal->SetLayerDepth(50);
    gal->SetFillColor(COLOR4D(0.9, 0.9, 0.3, 0.9));
    gal->DrawCircle(VECTOR2D(500, 150), 15);
    gal->DrawCircle(VECTOR2D(590, 150), 15);
    gal->DrawCircle(VECTOR2D(680, 150), 15);

    //=========================================================================
    // Section 3: Negatives layer API test
    //=========================================================================

    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.12, 0.15, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 300), VECTOR2D(380, 580));

    // Base content before negatives layer
    gal->SetLayerDepth(80);
    gal->SetFillColor(COLOR4D(0.3, 0.6, 0.3, 1.0));
    gal->DrawRectangle(VECTOR2D(40, 320), VECTOR2D(360, 560));

    // Test StartNegativesLayer / EndNegativesLayer (NO-OP in OpenGL)
    gal->StartNegativesLayer();

    // Content that would be on negatives layer
    gal->SetLayerDepth(60);
    gal->SetFillColor(COLOR4D(0.12, 0.15, 0.12, 1.0));

    // Via clearances (simulated)
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            double x = 80 + col * 80;
            double y = 380 + row * 100;
            gal->DrawCircle(VECTOR2D(x, y), 20);
        }
    }

    gal->EndNegativesLayer();

    // Vias on top
    gal->SetLayerDepth(40);
    gal->SetFillColor(COLOR4D(0.85, 0.65, 0.25, 1.0));
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            double x = 80 + col * 80;
            double y = 380 + row * 100;
            gal->DrawCircle(VECTOR2D(x, y), 12);
        }
    }

    // Drill holes
    gal->SetLayerDepth(20);
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.1, 1.0));
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 4; col++) {
            double x = 80 + col * 80;
            double y = 380 + row * 100;
            gal->DrawCircle(VECTOR2D(x, y), 5);
        }
    }

    //=========================================================================
    // Section 4: What "show negative objects" mode looks like
    //=========================================================================
    // In Gerbview, there's an option to show negative objects in a highlight color
    // instead of actually cutting them out

    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 300), VECTOR2D(780, 580));

    // Copper layer
    gal->SetLayerDepth(80);
    gal->SetFillColor(COLOR4D(0.6, 0.5, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(420, 320), VECTOR2D(760, 560));

    // "Negative objects" shown as semi-transparent overlay (like Gerbview's show_negative_objects mode)
    gal->SetLayerDepth(60);
    gal->SetFillColor(COLOR4D(0.2, 0.8, 0.8, 0.5));  // Cyan highlight for negatives

    // Trace clearance
    gal->DrawSegment(VECTOR2D(440, 380), VECTOR2D(740, 380), 25);

    // Pad clearances
    gal->DrawCircle(VECTOR2D(480, 380), 20);
    gal->DrawCircle(VECTOR2D(590, 380), 20);
    gal->DrawCircle(VECTOR2D(700, 380), 20);

    // Via clearances
    for (int i = 0; i < 3; i++) {
        gal->DrawCircle(VECTOR2D(480 + i * 110, 480), 18);
    }

    // Actual traces and pads (drawn on top)
    gal->SetLayerDepth(40);
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    gal->DrawSegment(VECTOR2D(440, 380), VECTOR2D(740, 380), 10);

    gal->SetFillColor(COLOR4D(0.9, 0.7, 0.3, 1.0));
    gal->DrawCircle(VECTOR2D(480, 380), 14);
    gal->DrawCircle(VECTOR2D(590, 380), 14);
    gal->DrawCircle(VECTOR2D(700, 380), 14);
    gal->DrawCircle(VECTOR2D(480, 480), 12);
    gal->DrawCircle(VECTOR2D(590, 480), 12);
    gal->DrawCircle(VECTOR2D(700, 480), 12);

    // Holes
    gal->SetLayerDepth(20);
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.1, 1.0));
    gal->DrawCircle(VECTOR2D(480, 380), 5);
    gal->DrawCircle(VECTOR2D(590, 380), 5);
    gal->DrawCircle(VECTOR2D(700, 380), 5);
    gal->DrawCircle(VECTOR2D(480, 480), 4);
    gal->DrawCircle(VECTOR2D(590, 480), 4);
    gal->DrawCircle(VECTOR2D(700, 480), 4);

    //=========================================================================
    // Section frames
    //=========================================================================
    gal->SetLayerDepth(5);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(380, 280));
    gal->DrawRectangle(VECTOR2D(400, 20), VECTOR2D(780, 280));
    gal->DrawRectangle(VECTOR2D(20, 300), VECTOR2D(380, 580));
    gal->DrawRectangle(VECTOR2D(400, 300), VECTOR2D(780, 580));
}

}  // namespace GALTest
