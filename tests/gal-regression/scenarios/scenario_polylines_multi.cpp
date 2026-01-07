/**
 * Polylines Multi Scenario
 *
 * Tests GAL::DrawPolylines() - drawing multiple polylines in a single call
 *
 * This is more efficient than calling DrawPolyline() multiple times
 * when rendering many polylines with the same style.
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

void RenderPolylinesMulti(GAL* gal, int width, int height) {
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);

    // Test 1: Multiple horizontal lines at once
    gal->SetStrokeColor(COLOR4D(1.0, 0.3, 0.3, 1.0));
    std::vector<std::vector<VECTOR2D>> horizontalLines;
    for (int i = 0; i < 5; i++) {
        std::vector<VECTOR2D> line = {
            VECTOR2D(50, 50 + i * 25),
            VECTOR2D(250, 50 + i * 25)
        };
        horizontalLines.push_back(line);
    }
    gal->DrawPolylines(horizontalLines);

    // Test 2: Multiple zigzag patterns
    gal->SetStrokeColor(COLOR4D(0.3, 1.0, 0.3, 1.0));
    std::vector<std::vector<VECTOR2D>> zigzags;
    for (int i = 0; i < 3; i++) {
        std::vector<VECTOR2D> zig;
        double baseY = 200 + i * 60;
        for (int j = 0; j <= 6; j++) {
            double x = 50 + j * 40;
            double y = baseY + ((j % 2 == 0) ? 0 : 30);
            zig.push_back(VECTOR2D(x, y));
        }
        zigzags.push_back(zig);
    }
    gal->DrawPolylines(zigzags);

    // Test 3: Multiple wave patterns
    gal->SetStrokeColor(COLOR4D(0.3, 0.3, 1.0, 1.0));
    std::vector<std::vector<VECTOR2D>> waves;
    for (int w = 0; w < 4; w++) {
        std::vector<VECTOR2D> wave;
        double baseY = 420 + w * 30;
        for (int i = 0; i <= 20; i++) {
            double t = (double)i / 20.0;
            double x = 50 + t * 300;
            double y = baseY + sin(t * M_PI * 3 + w * 0.5) * 10;
            wave.push_back(VECTOR2D(x, y));
        }
        waves.push_back(wave);
    }
    gal->DrawPolylines(waves);

    // Test 4: Grid pattern using polylines
    gal->SetStrokeColor(COLOR4D(0.8, 0.8, 0.2, 1.0));
    std::vector<std::vector<VECTOR2D>> gridLines;

    // Vertical grid lines
    for (int i = 0; i < 8; i++) {
        double x = 400 + i * 40;
        gridLines.push_back({VECTOR2D(x, 50), VECTOR2D(x, 250)});
    }

    // Horizontal grid lines
    for (int i = 0; i < 6; i++) {
        double y = 50 + i * 40;
        gridLines.push_back({VECTOR2D(400, y), VECTOR2D(680, y)});
    }

    gal->DrawPolylines(gridLines);

    // Test 5: Multiple concentric shapes
    gal->SetStrokeColor(COLOR4D(0.8, 0.3, 0.8, 1.0));
    std::vector<std::vector<VECTOR2D>> concentricSquares;
    double cx = 550;
    double cy = 400;

    for (int i = 1; i <= 4; i++) {
        double size = i * 25;
        std::vector<VECTOR2D> square = {
            VECTOR2D(cx - size, cy - size),
            VECTOR2D(cx + size, cy - size),
            VECTOR2D(cx + size, cy + size),
            VECTOR2D(cx - size, cy + size),
            VECTOR2D(cx - size, cy - size)  // close
        };
        concentricSquares.push_back(square);
    }
    gal->DrawPolylines(concentricSquares);

    // Test 6: Star burst pattern
    gal->SetStrokeColor(COLOR4D(1.0, 0.5, 0.0, 1.0));
    std::vector<std::vector<VECTOR2D>> starBurst;
    double starCx = 720;
    double starCy = 150;
    double innerR = 20;
    double outerR = 60;

    for (int i = 0; i < 12; i++) {
        double angle = i * M_PI / 6;
        std::vector<VECTOR2D> ray = {
            VECTOR2D(starCx + cos(angle) * innerR, starCy + sin(angle) * innerR),
            VECTOR2D(starCx + cos(angle) * outerR, starCy + sin(angle) * outerR)
        };
        starBurst.push_back(ray);
    }
    gal->DrawPolylines(starBurst);

    // Test 7: Parallel diagonal lines
    gal->SetStrokeColor(COLOR4D(0.5, 0.8, 0.8, 1.0));
    gal->SetLineWidth(1.5);
    std::vector<std::vector<VECTOR2D>> diagonals;

    for (int i = 0; i < 10; i++) {
        double offset = i * 15;
        diagonals.push_back({
            VECTOR2D(400 + offset, 280),
            VECTOR2D(520 + offset, 370)
        });
    }
    gal->DrawPolylines(diagonals);

    // Test 8: Different line widths (multiple calls needed)
    double lineWidths[] = {1.0, 2.0, 3.0, 5.0};
    for (int w = 0; w < 4; w++) {
        double t = (double)w / 3.0;
        gal->SetStrokeColor(COLOR4D(1.0 - t * 0.5, 0.3 + t * 0.4, 0.2 + t * 0.6, 1.0));
        gal->SetLineWidth(lineWidths[w]);

        std::vector<std::vector<VECTOR2D>> widthDemo;
        double y = 280 + w * 25;
        widthDemo.push_back({VECTOR2D(50, y), VECTOR2D(180, y)});
        widthDemo.push_back({VECTOR2D(200, y), VECTOR2D(330, y)});
        gal->DrawPolylines(widthDemo);
    }
}

}  // namespace GALTest
