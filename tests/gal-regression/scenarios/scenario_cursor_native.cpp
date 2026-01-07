/**
 * Cursor Native Scenario
 *
 * Tests GAL cursor-related methods:
 * - SetCursorEnabled() / IsCursorEnabled()
 * - SetCursorColor()
 * - DrawCursor()
 *
 * The cursor is typically a crosshair drawn at a specific location.
 * This scenario demonstrates different cursor styles and colors.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderCursorNative(GAL* gal, int width, int height) {
    // Background
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    // Draw a grid to give context for cursor positions
    gal->SetLayerDepth(90);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(0.25, 0.25, 0.3, 0.5));

    for (int x = 0; x < width; x += 40) {
        gal->DrawLine(VECTOR2D(x, 0), VECTOR2D(x, height));
    }
    for (int y = 0; y < height; y += 40) {
        gal->DrawLine(VECTOR2D(0, y), VECTOR2D(width, y));
    }

    // Test 1: Default cursor (white)
    gal->SetLayerDepth(10);
    gal->SetCursorEnabled(true);
    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 1.0));
    gal->DrawCursor(VECTOR2D(100, 100));

    // Label
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->SetLineWidth(1.0);
    gal->DrawRectangle(VECTOR2D(60, 60), VECTOR2D(140, 70));

    // Test 2: Red cursor
    gal->SetCursorColor(COLOR4D(1.0, 0.3, 0.3, 1.0));
    gal->DrawCursor(VECTOR2D(250, 100));

    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(210, 60), VECTOR2D(290, 70));

    // Test 3: Green cursor
    gal->SetCursorColor(COLOR4D(0.3, 1.0, 0.3, 1.0));
    gal->DrawCursor(VECTOR2D(400, 100));

    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(360, 60), VECTOR2D(440, 70));

    // Test 4: Blue cursor
    gal->SetCursorColor(COLOR4D(0.3, 0.3, 1.0, 1.0));
    gal->DrawCursor(VECTOR2D(550, 100));

    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(510, 60), VECTOR2D(590, 70));

    // Test 5: Yellow cursor (selection color)
    gal->SetCursorColor(COLOR4D(1.0, 1.0, 0.2, 1.0));
    gal->DrawCursor(VECTOR2D(700, 100));

    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(660, 60), VECTOR2D(740, 70));

    // Test 6: Semi-transparent cursors
    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 0.3));
    gal->DrawCursor(VECTOR2D(100, 250));

    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 0.5));
    gal->DrawCursor(VECTOR2D(200, 250));

    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 0.7));
    gal->DrawCursor(VECTOR2D(300, 250));

    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 1.0));
    gal->DrawCursor(VECTOR2D(400, 250));

    // Label for alpha row
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawRectangle(VECTOR2D(60, 210), VECTOR2D(440, 220));

    // Test 7: Cursor positions along a path
    gal->SetCursorColor(COLOR4D(0.8, 0.5, 0.2, 1.0));
    for (int i = 0; i < 8; i++) {
        double t = (double)i / 7.0;
        double x = 100 + t * 600;
        double y = 380 + sin(t * M_PI * 2) * 50;
        gal->DrawCursor(VECTOR2D(x, y));
    }

    // Draw the path itself
    gal->SetStrokeColor(COLOR4D(0.4, 0.4, 0.4, 0.5));
    gal->SetLineWidth(1.0);
    std::vector<VECTOR2D> path;
    for (int i = 0; i <= 50; i++) {
        double t = (double)i / 50.0;
        double x = 100 + t * 600;
        double y = 380 + sin(t * M_PI * 2) * 50;
        path.push_back(VECTOR2D(x, y));
    }
    gal->DrawPolyline(path);

    // Test 8: Cursor with different context - on objects
    // Draw some objects
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Pad
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    gal->DrawCircle(VECTOR2D(600, 250), 30);

    // Trace
    gal->DrawSegment(VECTOR2D(550, 250), VECTOR2D(650, 250), 8);

    // Cursor on the pad
    gal->SetLayerDepth(5);
    gal->SetCursorColor(COLOR4D(1.0, 1.0, 1.0, 1.0));
    gal->DrawCursor(VECTOR2D(600, 250));

    // Test 9: Multiple cursors showing cursor enabled/disabled
    gal->SetLayerDepth(5);

    // Enabled cursor
    gal->SetCursorEnabled(true);
    gal->SetCursorColor(COLOR4D(0.3, 1.0, 0.3, 1.0));
    gal->DrawCursor(VECTOR2D(700, 350));

    // Label
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.3, 0.8, 0.3, 0.8));
    gal->SetLineWidth(1.0);
    gal->DrawRectangle(VECTOR2D(660, 310), VECTOR2D(740, 320));

    // "Disabled" cursor (we still draw it but with different color to show the state)
    // Note: SetCursorEnabled(false) would prevent DrawCursor from rendering
    // So we show it as a dimmed cursor instead
    gal->SetCursorColor(COLOR4D(0.5, 0.5, 0.5, 0.3));
    gal->DrawCursor(VECTOR2D(700, 450));

    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.5, 0.5));
    gal->DrawRectangle(VECTOR2D(660, 410), VECTOR2D(740, 420));

    // Border around entire test area
    gal->SetLayerDepth(1);
    gal->SetStrokeColor(COLOR4D(0.4, 0.4, 0.4, 1.0));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(10, 10), VECTOR2D(width - 10, height - 10));
}

}  // namespace GALTest
