/**
 * Segment Chain Scenario
 *
 * Tests GAL::DrawSegmentChain() - connected thick segments with round joins
 *
 * This is used for drawing thick polylines like PCB traces with proper
 * join handling between segments.
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

void RenderSegmentChain(GAL* gal, int width, int height) {
    double cx = width / 2.0;
    double cy = height / 2.0;

    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Simple zigzag chain
    gal->SetFillColor(COLOR4D(0.9, 0.6, 0.2, 1.0));
    std::vector<VECTOR2D> zigzag = {
        VECTOR2D(80, 80),
        VECTOR2D(180, 150),
        VECTOR2D(280, 80),
        VECTOR2D(380, 150),
        VECTOR2D(480, 80)
    };
    gal->DrawSegmentChain(zigzag, 12.0);

    // Smooth wave chain
    gal->SetFillColor(COLOR4D(0.2, 0.7, 0.9, 1.0));
    std::vector<VECTOR2D> wave;
    for (int i = 0; i <= 20; i++) {
        double t = (double)i / 20.0;
        double x = 80 + t * 400;
        double y = 220 + sin(t * M_PI * 3) * 40;
        wave.push_back(VECTOR2D(x, y));
    }
    gal->DrawSegmentChain(wave, 8.0);

    // Sharp corners (tests join handling)
    gal->SetFillColor(COLOR4D(0.8, 0.3, 0.5, 1.0));
    std::vector<VECTOR2D> sharp = {
        VECTOR2D(550, 80),
        VECTOR2D(650, 80),
        VECTOR2D(650, 180),
        VECTOR2D(550, 180),
        VECTOR2D(550, 130),
        VECTOR2D(700, 130)
    };
    gal->DrawSegmentChain(sharp, 10.0);

    // Spiral chain
    gal->SetFillColor(COLOR4D(0.5, 0.8, 0.3, 1.0));
    std::vector<VECTOR2D> spiral;
    double spiralCx = cx - 150;
    double spiralCy = cy + 100;
    for (int i = 0; i <= 30; i++) {
        double angle = i * M_PI / 6;
        double radius = 20 + i * 3;
        spiral.push_back(VECTOR2D(
            spiralCx + cos(angle) * radius,
            spiralCy + sin(angle) * radius
        ));
    }
    gal->DrawSegmentChain(spiral, 6.0);

    // Varying width segments (multiple chains)
    double widths[] = {3.0, 6.0, 10.0, 15.0, 20.0};
    for (int w = 0; w < 5; w++) {
        double t = (double)w / 4.0;
        gal->SetFillColor(COLOR4D(1.0 - t * 0.5, 0.3 + t * 0.4, 0.3 + t * 0.5, 1.0));

        std::vector<VECTOR2D> line = {
            VECTOR2D(80, 320 + w * 45),
            VECTOR2D(200, 340 + w * 45),
            VECTOR2D(280, 310 + w * 45)
        };
        gal->DrawSegmentChain(line, widths[w]);
    }

    // Acute angle chain (stress test for joins)
    gal->SetFillColor(COLOR4D(1.0, 0.5, 0.2, 1.0));
    std::vector<VECTOR2D> acute = {
        VECTOR2D(400, 350),
        VECTOR2D(500, 350),
        VECTOR2D(420, 380),  // Very acute angle back
        VECTOR2D(520, 380),
        VECTOR2D(440, 410),
        VECTOR2D(540, 410)
    };
    gal->DrawSegmentChain(acute, 8.0);

    // Star pattern chain
    gal->SetFillColor(COLOR4D(0.9, 0.9, 0.2, 1.0));
    std::vector<VECTOR2D> star;
    double starCx = cx + 180;
    double starCy = cy + 120;
    double outerR = 60;
    double innerR = 25;
    for (int i = 0; i <= 10; i++) {
        double angle = i * M_PI / 5 - M_PI / 2;
        double r = (i % 2 == 0) ? outerR : innerR;
        star.push_back(VECTOR2D(
            starCx + cos(angle) * r,
            starCy + sin(angle) * r
        ));
    }
    gal->DrawSegmentChain(star, 5.0);

    // Single segment (degenerate case)
    gal->SetFillColor(COLOR4D(0.6, 0.4, 0.8, 1.0));
    std::vector<VECTOR2D> single = {
        VECTOR2D(600, 280),
        VECTOR2D(720, 320)
    };
    gal->DrawSegmentChain(single, 15.0);

    // Very thin chain
    gal->SetFillColor(COLOR4D(0.3, 0.3, 0.3, 1.0));
    std::vector<VECTOR2D> thin;
    for (int i = 0; i <= 15; i++) {
        double t = (double)i / 15.0;
        double x = 550 + t * 180;
        double y = 450 + sin(t * M_PI * 4) * 20;
        thin.push_back(VECTOR2D(x, y));
    }
    gal->DrawSegmentChain(thin, 2.0);
}

}  // namespace GALTest
