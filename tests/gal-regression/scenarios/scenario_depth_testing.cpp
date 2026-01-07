/**
 * Depth Testing Scenario
 *
 * Tests GAL::EnableDepthTest() - explicit depth test control
 *
 * Depth testing determines whether fragments are drawn based on their
 * depth value. When enabled with GL_LESS, closer fragments overwrite
 * farther ones. This test demonstrates depth ordering with overlapping shapes.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderDepthTesting(GAL* gal, int width, int height) {
    // Enable depth testing
    gal->EnableDepthTest(true);

    // Test SetDepthRange() API - sets the near/far depth range for rendering
    // x = near, y = far - this maps layer depths to NDC z-values
    // Default is typically VECTOR2D(0.1, 100) for KiCad
    gal->SetDepthRange(VECTOR2D(0.1, 100));

    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Section 1: Overlapping circles with explicit depth ordering
    // Using SetLayerDepth - lower values are closer to camera

    // Background reference
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.18, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(280, 200));

    // Draw circles from back to front
    gal->SetLayerDepth(80);
    gal->SetFillColor(COLOR4D(0.8, 0.2, 0.2, 0.9));
    gal->DrawCircle(VECTOR2D(100, 100), 50);

    gal->SetLayerDepth(60);
    gal->SetFillColor(COLOR4D(0.2, 0.8, 0.2, 0.9));
    gal->DrawCircle(VECTOR2D(140, 110), 50);

    gal->SetLayerDepth(40);
    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.8, 0.9));
    gal->DrawCircle(VECTOR2D(180, 100), 50);

    gal->SetLayerDepth(20);
    gal->SetFillColor(COLOR4D(0.8, 0.8, 0.2, 0.9));
    gal->DrawCircle(VECTOR2D(220, 90), 50);

    // Section 2: Depth ordering with rectangles
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.18, 1.0));
    gal->DrawRectangle(VECTOR2D(300, 20), VECTOR2D(560, 200));

    // Stack of rectangles
    for (int i = 0; i < 5; i++) {
        double t = (double)i / 4.0;
        gal->SetLayerDepth(90 - i * 15);
        gal->SetFillColor(COLOR4D(0.3 + t * 0.5, 0.3 + t * 0.2, 0.8 - t * 0.3, 0.9));
        gal->DrawRectangle(
            VECTOR2D(320 + i * 25, 40 + i * 20),
            VECTOR2D(420 + i * 25, 120 + i * 20)
        );
    }

    // Section 3: Complex depth scene (PCB-like)
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.1, 0.2, 0.1, 1.0));
    gal->DrawRectangle(VECTOR2D(580, 20), VECTOR2D(780, 200));

    // Traces at depth 70
    gal->SetLayerDepth(70);
    gal->SetFillColor(COLOR4D(0.7, 0.5, 0.2, 1.0));
    gal->DrawSegment(VECTOR2D(600, 60), VECTOR2D(760, 60), 8);
    gal->DrawSegment(VECTOR2D(600, 110), VECTOR2D(760, 110), 8);
    gal->DrawSegment(VECTOR2D(600, 160), VECTOR2D(760, 160), 8);

    // Pads at depth 50 (above traces)
    gal->SetLayerDepth(50);
    gal->SetFillColor(COLOR4D(0.85, 0.65, 0.25, 1.0));
    gal->DrawCircle(VECTOR2D(620, 60), 15);
    gal->DrawCircle(VECTOR2D(680, 60), 15);
    gal->DrawCircle(VECTOR2D(740, 60), 15);
    gal->DrawCircle(VECTOR2D(620, 110), 15);
    gal->DrawCircle(VECTOR2D(680, 110), 15);
    gal->DrawCircle(VECTOR2D(740, 110), 15);
    gal->DrawCircle(VECTOR2D(620, 160), 15);
    gal->DrawCircle(VECTOR2D(680, 160), 15);
    gal->DrawCircle(VECTOR2D(740, 160), 15);

    // Holes at depth 30 (above pads)
    gal->SetLayerDepth(30);
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.1, 1.0));
    gal->DrawCircle(VECTOR2D(620, 60), 6);
    gal->DrawCircle(VECTOR2D(680, 60), 6);
    gal->DrawCircle(VECTOR2D(740, 60), 6);
    gal->DrawCircle(VECTOR2D(620, 110), 6);
    gal->DrawCircle(VECTOR2D(680, 110), 6);
    gal->DrawCircle(VECTOR2D(740, 110), 6);
    gal->DrawCircle(VECTOR2D(620, 160), 6);
    gal->DrawCircle(VECTOR2D(680, 160), 6);
    gal->DrawCircle(VECTOR2D(740, 160), 6);

    // Section 4: Interleaved depths
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.18, 0.15, 0.18, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 220), VECTOR2D(280, 400));

    // Create a checkerboard-like depth pattern
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int depth = ((row + col) % 2 == 0) ? 60 : 40;
            double t = (row * 3 + col) / 8.0;
            gal->SetLayerDepth(depth);
            gal->SetFillColor(COLOR4D(0.8 * t + 0.2, 0.3 + 0.5 * (1 - t), 0.5, 0.9));
            gal->DrawCircle(VECTOR2D(70 + col * 70, 270 + row * 50), 25);
        }
    }

    // Section 5: AdvanceDepth demonstration
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.15, 0.18, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(300, 220), VECTOR2D(560, 400));

    // Use AdvanceDepth to automatically increment depth
    gal->SetLayerDepth(90);
    double cx = 430;
    double cy = 310;

    for (int i = 0; i < 8; i++) {
        double angle = i * M_PI / 4;
        double r = 60;
        double x = cx + cos(angle) * r;
        double y = cy + sin(angle) * r;

        double t = (double)i / 7.0;
        gal->SetFillColor(COLOR4D(1.0 - t * 0.5, 0.3 + t * 0.4, 0.3 + t * 0.5, 0.9));
        gal->DrawCircle(VECTOR2D(x, y), 30);
        gal->AdvanceDepth();  // Each subsequent circle is closer
    }

    // Center circle (closest)
    gal->SetFillColor(COLOR4D(1.0, 1.0, 1.0, 0.95));
    gal->DrawCircle(VECTOR2D(cx, cy), 25);

    // Section 6: Depth test off comparison
    // This section shows what happens without proper depth ordering
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.18, 0.18, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(580, 220), VECTOR2D(780, 400));

    // All at same depth - draw order determines visibility
    gal->SetLayerDepth(50);

    gal->SetFillColor(COLOR4D(0.8, 0.2, 0.2, 0.9));
    gal->DrawCircle(VECTOR2D(640, 300), 40);

    gal->SetFillColor(COLOR4D(0.2, 0.8, 0.2, 0.9));
    gal->DrawCircle(VECTOR2D(680, 310), 40);

    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.8, 0.9));
    gal->DrawCircle(VECTOR2D(720, 300), 40);

    // Labels/frames
    gal->SetLayerDepth(10);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);

    gal->SetStrokeColor(COLOR4D(0.4, 0.4, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(280, 200));
    gal->DrawRectangle(VECTOR2D(300, 20), VECTOR2D(560, 200));
    gal->DrawRectangle(VECTOR2D(580, 20), VECTOR2D(780, 200));
    gal->DrawRectangle(VECTOR2D(20, 220), VECTOR2D(280, 400));
    gal->DrawRectangle(VECTOR2D(300, 220), VECTOR2D(560, 400));
    gal->DrawRectangle(VECTOR2D(580, 220), VECTOR2D(780, 400));

    // Bottom info bar
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 420), VECTOR2D(780, 480));

    gal->SetIsFill(false);
    gal->SetStrokeColor(COLOR4D(0.3, 0.3, 0.4, 0.8));
    gal->SetLineWidth(1.0);
    gal->DrawRectangle(VECTOR2D(20, 420), VECTOR2D(780, 480));
}

}  // namespace GALTest
