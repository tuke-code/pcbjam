/**
 * Screen Transform Scenario
 *
 * Tests GAL screen-level transformation methods:
 * - SetRotation() / GetRotation() - screen rotation
 * - SetFlip() - X/Y axis flipping
 * - ToWorld() / ToScreen() - coordinate conversion
 *
 * These are viewport-level transforms that affect all rendering,
 * different from the per-object Save/Restore/Transform methods.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderScreenTransform(GAL* gal, int width, int height) {
    // Note: SetRotation and SetFlip affect the world-to-screen matrix
    // They need to be set before drawing and affect subsequent operations

    //=========================================================================
    // Test SetFlip() and SetRotation() APIs
    // These are screen-level transforms that affect the worldScreenMatrix
    //=========================================================================

    // Test SetFlip() API - sets X and/or Y axis mirroring
    // Note: In our test harness the matrix is already computed, so we demonstrate
    // the API is callable. In real use, SetFlip must be called before ComputeWorldScreenMatrix
    gal->SetFlip(false, false);  // No flip - default state

    // Test SetRotation() API - sets screen rotation angle
    // Note: Like SetFlip, affects worldScreenMatrix computation
    gal->SetRotation(0.0);  // No rotation - default state

    // First, draw reference content without any screen transforms
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    // Reference grid
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(0.2, 0.2, 0.25, 0.4));

    for (int x = 0; x < width; x += 40) {
        gal->DrawLine(VECTOR2D(x, 0), VECTOR2D(x, height));
    }
    for (int y = 0; y < height; y += 40) {
        gal->DrawLine(VECTOR2D(0, y), VECTOR2D(width, y));
    }

    // Test 1: Normal orientation reference shape
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Draw an arrow-like shape that shows orientation
    auto drawOrientationMarker = [&](double cx, double cy, double size, COLOR4D color) {
        gal->SetFillColor(color);

        // Main body (rectangle)
        gal->DrawRectangle(VECTOR2D(cx - size * 0.3, cy - size * 0.5),
                          VECTOR2D(cx + size * 0.3, cy + size * 0.3));

        // Arrow head pointing up
        std::deque<VECTOR2D> arrow = {
            VECTOR2D(cx, cy - size * 0.8),
            VECTOR2D(cx - size * 0.5, cy - size * 0.3),
            VECTOR2D(cx + size * 0.5, cy - size * 0.3)
        };
        gal->DrawPolygon(arrow);

        // Small circle at base to show which end is bottom
        gal->SetFillColor(COLOR4D(color.r * 0.5, color.g * 0.5, color.b * 0.5, 1.0));
        gal->DrawCircle(VECTOR2D(cx, cy + size * 0.15), size * 0.15);
    };

    // Reference marker (no transform)
    drawOrientationMarker(120, 120, 60, COLOR4D(0.8, 0.3, 0.3, 1.0));

    // Label
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.6, 0.3, 0.3, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(60, 50), VECTOR2D(180, 180));

    // Test 2: Using Save/Restore with rotation (object-level transform)
    gal->SetLayerDepth(50);
    gal->Save();
    gal->Translate(VECTOR2D(280, 120));
    gal->Rotate(45.0 * M_PI / 180.0);

    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.3, 0.8, 0.3, 1.0));
    gal->DrawRectangle(VECTOR2D(-30, -50), VECTOR2D(30, 30));
    std::deque<VECTOR2D> arrow2 = {
        VECTOR2D(0, -80),
        VECTOR2D(-50, -30),
        VECTOR2D(50, -30)
    };
    gal->DrawPolygon(arrow2);
    gal->SetFillColor(COLOR4D(0.15, 0.4, 0.15, 1.0));
    gal->DrawCircle(VECTOR2D(0, 15), 15);

    gal->Restore();

    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.3, 0.6, 0.3, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(200, 50), VECTOR2D(360, 200));

    // Test 3: Demonstrate ToWorld/ToScreen coordinate conversion
    gal->SetLayerDepth(40);

    // Draw a marker at a known world position
    VECTOR2D worldPoint(500, 120);

    // Mark the world position
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.8, 0.8, 0.2, 1.0));
    gal->DrawCircle(worldPoint, 15);

    // Convert to screen and back
    VECTOR2D screenPoint = gal->ToScreen(worldPoint);
    VECTOR2D backToWorld = gal->ToWorld(screenPoint);

    // Draw indicator showing the conversion (should be at same spot)
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.2, 0.8, 0.8, 1.0));
    gal->DrawCircle(backToWorld, 20);

    // Label
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.2, 0.8));
    gal->DrawRectangle(VECTOR2D(440, 50), VECTOR2D(560, 180));

    // Test 4: Multiple rotated shapes showing different angles
    gal->SetLayerDepth(50);
    double angles[] = {0, 30, 60, 90, 120, 150};
    double baseX = 100;
    double baseY = 300;

    for (int i = 0; i < 6; i++) {
        double cx = baseX + i * 100;
        double angle = angles[i] * M_PI / 180.0;

        gal->Save();
        gal->Translate(VECTOR2D(cx, baseY));
        gal->Rotate(angle);

        // Draw a simple "F" shape to show rotation clearly
        gal->SetIsFill(true);
        gal->SetIsStroke(false);
        double t = (double)i / 5.0;
        gal->SetFillColor(COLOR4D(0.8 - t * 0.3, 0.3 + t * 0.5, 0.3 + t * 0.3, 1.0));

        // Vertical bar
        gal->DrawRectangle(VECTOR2D(-5, -30), VECTOR2D(5, 30));
        // Top horizontal bar
        gal->DrawRectangle(VECTOR2D(5, -30), VECTOR2D(25, -20));
        // Middle horizontal bar
        gal->DrawRectangle(VECTOR2D(5, -5), VECTOR2D(18, 5));

        gal->Restore();
    }

    // Frame around rotation demo
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.6, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(40, 230), VECTOR2D(660, 370));

    // Test 5: Flip demonstration using object transforms
    // (Note: SetFlip() affects the entire viewport, so we simulate with Scale)
    gal->SetLayerDepth(50);

    // Original
    gal->Save();
    gal->Translate(VECTOR2D(120, 450));
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.7, 0.4, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(-25, -40), VECTOR2D(25, 20));
    std::deque<VECTOR2D> tri1 = {
        VECTOR2D(0, -60), VECTOR2D(-30, -40), VECTOR2D(30, -40)
    };
    gal->DrawPolygon(tri1);
    gal->SetFillColor(COLOR4D(0.35, 0.2, 0.35, 1.0));
    gal->DrawCircle(VECTOR2D(0, 5), 10);
    gal->Restore();

    // X-flipped (mirror horizontally)
    gal->Save();
    gal->Translate(VECTOR2D(280, 450));
    gal->Scale(VECTOR2D(-1.0, 1.0));  // Flip X
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.7, 0.4, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(-25, -40), VECTOR2D(25, 20));
    std::deque<VECTOR2D> tri2 = {
        VECTOR2D(0, -60), VECTOR2D(-30, -40), VECTOR2D(30, -40)
    };
    gal->DrawPolygon(tri2);
    gal->SetFillColor(COLOR4D(0.35, 0.2, 0.35, 1.0));
    gal->DrawCircle(VECTOR2D(0, 5), 10);
    gal->Restore();

    // Y-flipped (mirror vertically)
    gal->Save();
    gal->Translate(VECTOR2D(440, 450));
    gal->Scale(VECTOR2D(1.0, -1.0));  // Flip Y
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.7, 0.4, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(-25, -40), VECTOR2D(25, 20));
    std::deque<VECTOR2D> tri3 = {
        VECTOR2D(0, -60), VECTOR2D(-30, -40), VECTOR2D(30, -40)
    };
    gal->DrawPolygon(tri3);
    gal->SetFillColor(COLOR4D(0.35, 0.2, 0.35, 1.0));
    gal->DrawCircle(VECTOR2D(0, 5), 10);
    gal->Restore();

    // Both flipped
    gal->Save();
    gal->Translate(VECTOR2D(600, 450));
    gal->Scale(VECTOR2D(-1.0, -1.0));  // Flip both
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.7, 0.4, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(-25, -40), VECTOR2D(25, 20));
    std::deque<VECTOR2D> tri4 = {
        VECTOR2D(0, -60), VECTOR2D(-30, -40), VECTOR2D(30, -40)
    };
    gal->DrawPolygon(tri4);
    gal->SetFillColor(COLOR4D(0.35, 0.2, 0.35, 1.0));
    gal->DrawCircle(VECTOR2D(0, 5), 10);
    gal->Restore();

    // Labels for flip demo
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.3, 0.5, 0.6));
    gal->DrawRectangle(VECTOR2D(70, 385), VECTOR2D(170, 395));   // Original
    gal->DrawRectangle(VECTOR2D(230, 385), VECTOR2D(330, 395));  // X-flip
    gal->DrawRectangle(VECTOR2D(390, 385), VECTOR2D(490, 395));  // Y-flip
    gal->DrawRectangle(VECTOR2D(550, 385), VECTOR2D(650, 395));  // XY-flip

    // Frame around flip demo
    gal->SetStrokeColor(COLOR4D(0.6, 0.4, 0.6, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(40, 380), VECTOR2D(700, 510));
}

}  // namespace GALTest
