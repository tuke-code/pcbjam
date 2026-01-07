/**
 * Arc Segments Scenario
 *
 * Tests GAL::DrawArcSegment() - thick arc strokes
 *
 * Unlike DrawArc() which draws pie slices when filled,
 * DrawArcSegment() draws thick arc strokes (like a PCB trace).
 */

#include <gal/graphics_abstraction_layer.h>
#include <geometry/eda_angle.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderArcSegments(GAL* gal, int width, int height) {
    double cx = width / 2.0;
    double cy = height / 2.0;
    double maxError = 0.01;  // Segment approximation error

    // Thick arc segments with varying widths
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // 90-degree arcs at different widths
    double widths[] = {5.0, 10.0, 20.0, 30.0};
    double radius = 80;

    for (int i = 0; i < 4; i++) {
        double t = (double)i / 3.0;
        gal->SetFillColor(COLOR4D(1.0 - t * 0.5, 0.3 + t * 0.4, 0.2 + t * 0.6, 1.0));

        EDA_ANGLE startAngle(i * 90, DEGREES_T);
        EDA_ANGLE arcAngle(90, DEGREES_T);

        gal->DrawArcSegment(
            VECTOR2D(cx - 200, cy - 100),
            radius,
            startAngle,
            arcAngle,
            widths[i],
            maxError
        );
    }

    // Full ring using arc segments
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx, cy - 100),
        60,
        EDA_ANGLE(0, DEGREES_T),
        EDA_ANGLE(360, DEGREES_T),
        15,
        maxError
    );

    // Half rings
    gal->SetFillColor(COLOR4D(0.2, 0.8, 0.4, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx + 180, cy - 100),
        70,
        EDA_ANGLE(0, DEGREES_T),
        EDA_ANGLE(180, DEGREES_T),
        12,
        maxError
    );

    gal->SetFillColor(COLOR4D(0.4, 0.2, 0.8, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx + 180, cy - 100),
        70,
        EDA_ANGLE(180, DEGREES_T),
        EDA_ANGLE(180, DEGREES_T),
        8,
        maxError
    );

    // Concentric arc segments (like a spiral)
    double baseRadius = 40;
    for (int i = 0; i < 5; i++) {
        double t = (double)i / 4.0;
        gal->SetFillColor(COLOR4D(t, 0.5, 1.0 - t, 0.9));

        gal->DrawArcSegment(
            VECTOR2D(cx - 150, cy + 150),
            baseRadius + i * 25,
            EDA_ANGLE(i * 30, DEGREES_T),
            EDA_ANGLE(270, DEGREES_T),
            6,
            maxError
        );
    }

    // Thin arc segments (like copper traces)
    gal->SetFillColor(COLOR4D(0.9, 0.7, 0.3, 1.0));
    for (int i = 0; i < 8; i++) {
        gal->DrawArcSegment(
            VECTOR2D(cx + 100, cy + 150),
            30 + i * 15,
            EDA_ANGLE(45, DEGREES_T),
            EDA_ANGLE(90, DEGREES_T),
            3,
            maxError
        );
    }

    // Small angle arc segments
    gal->SetFillColor(COLOR4D(1.0, 0.4, 0.4, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx, cy + 100),
        100,
        EDA_ANGLE(0, DEGREES_T),
        EDA_ANGLE(30, DEGREES_T),
        20,
        maxError
    );

    gal->SetFillColor(COLOR4D(0.4, 1.0, 0.4, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx, cy + 100),
        100,
        EDA_ANGLE(60, DEGREES_T),
        EDA_ANGLE(30, DEGREES_T),
        20,
        maxError
    );

    gal->SetFillColor(COLOR4D(0.4, 0.4, 1.0, 1.0));
    gal->DrawArcSegment(
        VECTOR2D(cx, cy + 100),
        100,
        EDA_ANGLE(120, DEGREES_T),
        EDA_ANGLE(30, DEGREES_T),
        20,
        maxError
    );

    // Stroked arc segment outline
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(1.0, 1.0, 0.0, 1.0));

    gal->DrawArcSegment(
        VECTOR2D(cx + 200, cy + 100),
        50,
        EDA_ANGLE(0, DEGREES_T),
        EDA_ANGLE(270, DEGREES_T),
        25,
        maxError
    );
}

}  // namespace GALTest
