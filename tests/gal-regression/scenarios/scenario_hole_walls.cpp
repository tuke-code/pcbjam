/**
 * Hole Walls Scenario
 *
 * Tests GAL::DrawHoleWall() - ring-shaped holes
 *
 * This is used for drawing plated through holes (PTH) in PCBs
 * where there's a drill hole surrounded by copper plating.
 * Parameters: center point, inner radius (hole), wall width
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderHoleWalls(GAL* gal, int width, int height) {
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Row 1: Different hole sizes with same wall width
    gal->SetFillColor(COLOR4D(0.9, 0.7, 0.3, 1.0));  // Copper color
    double holeRadii[] = {5.0, 10.0, 15.0, 20.0, 25.0};
    double wallWidth = 8.0;

    for (int i = 0; i < 5; i++) {
        double x = 100 + i * 100;
        gal->DrawHoleWall(VECTOR2D(x, 80), holeRadii[i], wallWidth);
    }

    // Row 2: Same hole size with different wall widths
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    double holeRadius = 12.0;
    double wallWidths[] = {3.0, 6.0, 10.0, 15.0, 20.0};

    for (int i = 0; i < 5; i++) {
        double x = 100 + i * 100;
        gal->DrawHoleWall(VECTOR2D(x, 180), holeRadius, wallWidths[i]);
    }

    // Row 3: Various colors (like different PCB layers)
    double coloredRadius = 15.0;
    double coloredWall = 10.0;

    // Front copper
    gal->SetFillColor(COLOR4D(0.9, 0.2, 0.2, 1.0));
    gal->DrawHoleWall(VECTOR2D(100, 280), coloredRadius, coloredWall);

    // Back copper
    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.9, 1.0));
    gal->DrawHoleWall(VECTOR2D(200, 280), coloredRadius, coloredWall);

    // Inner layer 1
    gal->SetFillColor(COLOR4D(0.2, 0.8, 0.2, 1.0));
    gal->DrawHoleWall(VECTOR2D(300, 280), coloredRadius, coloredWall);

    // Inner layer 2
    gal->SetFillColor(COLOR4D(0.8, 0.8, 0.2, 1.0));
    gal->DrawHoleWall(VECTOR2D(400, 280), coloredRadius, coloredWall);

    // Via (smaller)
    gal->SetFillColor(COLOR4D(0.6, 0.6, 0.6, 1.0));
    gal->DrawHoleWall(VECTOR2D(500, 280), 8.0, 6.0);

    // Row 4: Semi-transparent overlapping (like layer stack view)
    gal->SetFillColor(COLOR4D(0.9, 0.3, 0.3, 0.5));
    gal->DrawHoleWall(VECTOR2D(150, 380), 20.0, 15.0);

    gal->SetFillColor(COLOR4D(0.3, 0.9, 0.3, 0.5));
    gal->DrawHoleWall(VECTOR2D(180, 380), 18.0, 13.0);

    gal->SetFillColor(COLOR4D(0.3, 0.3, 0.9, 0.5));
    gal->DrawHoleWall(VECTOR2D(210, 380), 16.0, 11.0);

    // Row 4 continued: Grid of small vias
    gal->SetFillColor(COLOR4D(0.7, 0.7, 0.7, 1.0));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 5; col++) {
            double x = 320 + col * 30;
            double y = 350 + row * 30;
            gal->DrawHoleWall(VECTOR2D(x, y), 5.0, 4.0);
        }
    }

    // Row 5: Very thin walls (micro vias)
    gal->SetFillColor(COLOR4D(0.8, 0.5, 0.2, 1.0));
    for (int i = 0; i < 8; i++) {
        double x = 100 + i * 70;
        gal->DrawHoleWall(VECTOR2D(x, 470), 8.0, 2.0);
    }

    // Large mounting hole example
    gal->SetFillColor(COLOR4D(0.6, 0.6, 0.3, 1.0));
    gal->DrawHoleWall(VECTOR2D(650, 150), 30.0, 25.0);

    // Very thick annular ring
    gal->SetFillColor(COLOR4D(0.4, 0.7, 0.4, 1.0));
    gal->DrawHoleWall(VECTOR2D(650, 300), 15.0, 35.0);

    // Stroked hole wall (outline mode)
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(1.0, 1.0, 0.0, 1.0));
    gal->DrawHoleWall(VECTOR2D(650, 430), 20.0, 15.0);

    // Labels area markers
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.5, 0.5));
    gal->SetLineWidth(1.0);

    // Row markers
    gal->DrawLine(VECTOR2D(50, 40), VECTOR2D(550, 40));
    gal->DrawLine(VECTOR2D(50, 140), VECTOR2D(550, 140));
    gal->DrawLine(VECTOR2D(50, 240), VECTOR2D(550, 240));
    gal->DrawLine(VECTOR2D(50, 320), VECTOR2D(550, 320));
    gal->DrawLine(VECTOR2D(50, 440), VECTOR2D(650, 440));
}

}  // namespace GALTest
