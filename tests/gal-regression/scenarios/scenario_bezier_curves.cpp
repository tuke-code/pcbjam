/**
 * Bezier Curves Scenario
 *
 * Tests GAL::DrawCurve() - cubic bezier splines
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderBezierCurves(GAL* gal, int width, int height) {
    double cx = width / 2.0;
    double cy = height / 2.0;

    gal->SetIsFill(false);
    gal->SetIsStroke(true);

    // Simple S-curve
    gal->SetStrokeColor(COLOR4D(1.0, 0.3, 0.3, 1.0));
    gal->SetLineWidth(3.0);
    gal->DrawCurve(
        VECTOR2D(100, cy - 100),           // start
        VECTOR2D(200, cy - 200),           // control 1
        VECTOR2D(300, cy),                 // control 2
        VECTOR2D(400, cy - 100),           // end
        0.0                                // filter value
    );

    // Inverse S-curve below
    gal->SetStrokeColor(COLOR4D(0.3, 1.0, 0.3, 1.0));
    gal->DrawCurve(
        VECTOR2D(100, cy + 100),
        VECTOR2D(200, cy + 200),
        VECTOR2D(300, cy),
        VECTOR2D(400, cy + 100),
        0.0
    );

    // Loop curve (tight control points)
    gal->SetStrokeColor(COLOR4D(0.3, 0.3, 1.0, 1.0));
    gal->SetLineWidth(2.0);
    gal->DrawCurve(
        VECTOR2D(450, cy - 50),
        VECTOR2D(550, cy - 150),
        VECTOR2D(550, cy + 150),
        VECTOR2D(450, cy + 50),
        0.0
    );

    // Straight-ish curve (control points on line)
    gal->SetStrokeColor(COLOR4D(1.0, 1.0, 0.3, 1.0));
    gal->DrawCurve(
        VECTOR2D(500, 80),
        VECTOR2D(550, 80),
        VECTOR2D(650, 80),
        VECTOR2D(700, 80),
        0.0
    );

    // Wave pattern using multiple curves
    gal->SetStrokeColor(COLOR4D(0.8, 0.5, 0.8, 1.0));
    gal->SetLineWidth(2.5);
    double waveY = height - 100;
    double segWidth = 100;
    double amplitude = 40;

    for (int i = 0; i < 6; i++) {
        double x0 = 100 + i * segWidth;
        double x1 = x0 + segWidth;
        double dir = (i % 2 == 0) ? 1.0 : -1.0;

        gal->DrawCurve(
            VECTOR2D(x0, waveY),
            VECTOR2D(x0 + segWidth * 0.33, waveY + amplitude * dir),
            VECTOR2D(x0 + segWidth * 0.66, waveY + amplitude * dir),
            VECTOR2D(x1, waveY),
            0.0
        );
    }

    // Heart shape using two curves
    gal->SetStrokeColor(COLOR4D(1.0, 0.2, 0.4, 1.0));
    gal->SetLineWidth(3.0);
    double hx = cx + 150;
    double hy = cy - 50;
    double hs = 50;  // heart scale

    // Left half of heart
    gal->DrawCurve(
        VECTOR2D(hx, hy + hs * 0.5),        // bottom point
        VECTOR2D(hx - hs * 1.5, hy - hs),   // left control
        VECTOR2D(hx - hs * 0.5, hy - hs * 1.5), // top control
        VECTOR2D(hx, hy - hs * 0.3),        // top middle
        0.0
    );

    // Right half of heart
    gal->DrawCurve(
        VECTOR2D(hx, hy - hs * 0.3),        // top middle
        VECTOR2D(hx + hs * 0.5, hy - hs * 1.5), // top control
        VECTOR2D(hx + hs * 1.5, hy - hs),   // right control
        VECTOR2D(hx, hy + hs * 0.5),        // bottom point
        0.0
    );

    // Control point visualization (draw handles)
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.5, 0.5));
    gal->SetLineWidth(1.0);

    // Show control points for the first curve
    VECTOR2D p0(100, cy - 100);
    VECTOR2D c0(200, cy - 200);
    VECTOR2D c1(300, cy);
    VECTOR2D p1(400, cy - 100);

    gal->DrawLine(p0, c0);
    gal->DrawLine(p1, c1);

    // Draw control point markers
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(1.0, 0.5, 0.0, 0.8));
    gal->DrawCircle(c0, 5);
    gal->DrawCircle(c1, 5);
    gal->SetFillColor(COLOR4D(0.0, 0.5, 1.0, 0.8));
    gal->DrawCircle(p0, 5);
    gal->DrawCircle(p1, 5);
}

}  // namespace GALTest
