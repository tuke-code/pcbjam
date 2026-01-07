/**
 * Grid Native Scenario
 *
 * Tests GAL grid-related methods:
 * - SetGridVisibility() / GetGridVisibility()
 * - SetGridOrigin()
 * - SetGridSize()
 * - SetGridColor()
 * - SetAxesEnabled() / SetAxesColor()
 * - SetCoarseGrid()
 * - DrawGrid()
 *
 * Note: The grid system in GAL is designed for the viewport,
 * so we demonstrate grid properties through explicit DrawGrid() calls
 * with different settings applied to different regions.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderGridNative(GAL* gal, int width, int height) {
    // First, let's demonstrate the grid API by setting up and drawing
    // different grid configurations

    //=========================================================================
    // Test GetGridPoint() API - snaps a world point to nearest grid point
    //=========================================================================
    gal->SetGridSize(VECTOR2D(20, 20));
    gal->SetGridOrigin(VECTOR2D(0, 0));

    // Test GetGridPoint - should snap (55, 47) to nearest grid point (60, 40)
    VECTOR2D testPoint(55, 47);
    VECTOR2D snappedPoint = gal->GetGridPoint(testPoint);
    // snappedPoint should now be (60, 40) or similar based on grid settings

    // Default background
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    // Region 1: Fine grid with axes
    gal->SetLayerDepth(50);
    gal->SetGridVisibility(true);
    gal->SetGridOrigin(VECTOR2D(100, 100));
    gal->SetGridSize(VECTOR2D(20, 20));  // 20-pixel grid
    gal->SetGridColor(COLOR4D(0.3, 0.3, 0.5, 0.5));
    gal->SetAxesEnabled(true);
    gal->SetAxesColor(COLOR4D(0.8, 0.3, 0.3, 0.8));
    gal->SetCoarseGrid(5);  // Every 5th line is coarse

    // Draw the grid (this uses the internal grid renderer)
    gal->DrawGrid();

    // Mark region 1 boundary
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.8, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(320, 220));

    // Since DrawGrid() renders based on viewport, let's also
    // manually draw grid patterns to show different configurations

    // Region 2: Custom fine grid pattern
    gal->SetLayerDepth(40);
    gal->SetStrokeColor(COLOR4D(0.2, 0.5, 0.2, 0.4));
    gal->SetLineWidth(1.0);

    double gridX = 360;
    double gridY = 20;
    double gridW = 300;
    double gridH = 200;
    double step = 15;

    // Vertical lines
    for (double x = gridX; x <= gridX + gridW; x += step) {
        gal->DrawLine(VECTOR2D(x, gridY), VECTOR2D(x, gridY + gridH));
    }
    // Horizontal lines
    for (double y = gridY; y <= gridY + gridH; y += step) {
        gal->DrawLine(VECTOR2D(gridX, y), VECTOR2D(gridX + gridW, y));
    }

    // Coarse grid overlay
    gal->SetStrokeColor(COLOR4D(0.3, 0.7, 0.3, 0.6));
    gal->SetLineWidth(2.0);
    double coarseStep = step * 5;

    for (double x = gridX; x <= gridX + gridW; x += coarseStep) {
        gal->DrawLine(VECTOR2D(x, gridY), VECTOR2D(x, gridY + gridH));
    }
    for (double y = gridY; y <= gridY + gridH; y += coarseStep) {
        gal->DrawLine(VECTOR2D(gridX, y), VECTOR2D(gridX + gridW, y));
    }

    // Region 2 boundary
    gal->SetStrokeColor(COLOR4D(0.5, 0.8, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(gridX - 5, gridY - 5), VECTOR2D(gridX + gridW + 5, gridY + gridH + 5));

    // Region 3: Dot grid (alternative grid style)
    gal->SetLayerDepth(30);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.6, 0.6, 0.8, 0.6));

    double dotGridX = 20;
    double dotGridY = 260;
    double dotStep = 25;

    for (double x = dotGridX; x <= dotGridX + 280; x += dotStep) {
        for (double y = dotGridY; y <= dotGridY + 200; y += dotStep) {
            gal->DrawCircle(VECTOR2D(x, y), 2);
        }
    }

    // Mark region 3
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.8, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(dotGridX - 10, dotGridY - 10),
                       VECTOR2D(dotGridX + 290, dotGridY + 210));

    // Region 4: Non-square grid (different X and Y spacing)
    gal->SetLayerDepth(30);
    gal->SetStrokeColor(COLOR4D(0.8, 0.5, 0.3, 0.5));
    gal->SetLineWidth(1.0);

    double nsGridX = 360;
    double nsGridY = 260;
    double nsGridW = 300;
    double nsGridH = 200;
    double xStep = 30;
    double yStep = 15;

    // Vertical lines (wide spacing)
    for (double x = nsGridX; x <= nsGridX + nsGridW; x += xStep) {
        gal->DrawLine(VECTOR2D(x, nsGridY), VECTOR2D(x, nsGridY + nsGridH));
    }
    // Horizontal lines (tight spacing)
    for (double y = nsGridY; y <= nsGridY + nsGridH; y += yStep) {
        gal->DrawLine(VECTOR2D(nsGridX, y), VECTOR2D(nsGridX + nsGridW, y));
    }

    // Mark region 4
    gal->SetStrokeColor(COLOR4D(0.8, 0.6, 0.4, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(nsGridX - 5, nsGridY - 5),
                       VECTOR2D(nsGridX + nsGridW + 5, nsGridY + nsGridH + 5));

    // Region 5: Grid with different origin (offset)
    gal->SetLayerDepth(30);
    gal->SetStrokeColor(COLOR4D(0.7, 0.3, 0.7, 0.5));
    gal->SetLineWidth(1.0);

    double oGridX = 700;
    double oGridY = 20;
    double oGridW = 100;
    double oGridH = 200;
    double oStep = 20;
    double originOffsetX = 7;  // Origin offset from edge
    double originOffsetY = 12;

    for (double x = oGridX + originOffsetX; x <= oGridX + oGridW; x += oStep) {
        gal->DrawLine(VECTOR2D(x, oGridY), VECTOR2D(x, oGridY + oGridH));
    }
    for (double y = oGridY + originOffsetY; y <= oGridY + oGridH; y += oStep) {
        gal->DrawLine(VECTOR2D(oGridX, y), VECTOR2D(oGridX + oGridW, y));
    }

    // Mark origin point
    gal->SetFillColor(COLOR4D(1.0, 0.3, 0.3, 1.0));
    gal->SetIsFill(true);
    gal->DrawCircle(VECTOR2D(oGridX + originOffsetX, oGridY + originOffsetY), 5);

    // Mark region 5
    gal->SetIsFill(false);
    gal->SetStrokeColor(COLOR4D(0.7, 0.4, 0.7, 0.8));
    gal->SetLineWidth(2.0);
    gal->DrawRectangle(VECTOR2D(oGridX - 5, oGridY - 5),
                       VECTOR2D(oGridX + oGridW + 5, oGridY + oGridH + 5));

    // Axes demonstration in center-bottom region
    gal->SetLayerDepth(20);
    double axesCx = 700;
    double axesCy = 380;

    // X axis (red)
    gal->SetStrokeColor(COLOR4D(1.0, 0.2, 0.2, 1.0));
    gal->SetLineWidth(2.0);
    gal->DrawLine(VECTOR2D(axesCx - 80, axesCy), VECTOR2D(axesCx + 80, axesCy));

    // Y axis (green)
    gal->SetStrokeColor(COLOR4D(0.2, 1.0, 0.2, 1.0));
    gal->DrawLine(VECTOR2D(axesCx, axesCy - 80), VECTOR2D(axesCx, axesCy + 80));

    // Origin marker
    gal->SetFillColor(COLOR4D(1.0, 1.0, 0.2, 1.0));
    gal->SetIsFill(true);
    gal->DrawCircle(VECTOR2D(axesCx, axesCy), 6);
}

}  // namespace GALTest
