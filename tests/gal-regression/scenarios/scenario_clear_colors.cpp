/**
 * Clear Colors Scenario
 *
 * Tests GAL::SetClearColor() - background color setting
 *
 * Note: SetClearColor affects the background when ClearScreen is called.
 * Since we can only have one background per frame, this demonstrates
 * the concept by drawing colored rectangles to show different clear colors.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderClearColors(GAL* gal, int width, int height) {
    // Demonstrate SetClearColor by showing what different backgrounds look like
    // We simulate this with filled rectangles since we can only clear once per frame

    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetLayerDepth(100);

    double boxW = 180;
    double boxH = 120;
    double margin = 20;

    // Row 1: Standard backgrounds
    // Dark theme
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(margin, margin), VECTOR2D(margin + boxW, margin + boxH));

    // Light theme
    gal->SetFillColor(COLOR4D(0.95, 0.95, 0.95, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + boxW + 20, margin),
                       VECTOR2D(margin + boxW * 2 + 20, margin + boxH));

    // Blue-gray
    gal->SetFillColor(COLOR4D(0.15, 0.18, 0.22, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 2, margin),
                       VECTOR2D(margin + boxW * 3 + 40, margin + boxH));

    // Green tint (PCB style)
    gal->SetFillColor(COLOR4D(0.08, 0.15, 0.08, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 3, margin),
                       VECTOR2D(margin + boxW * 4 + 60, margin + boxH));

    // Row 2: More color options
    double row2Y = margin + boxH + 30;

    // Warm gray
    gal->SetFillColor(COLOR4D(0.2, 0.18, 0.16, 1.0));
    gal->DrawRectangle(VECTOR2D(margin, row2Y), VECTOR2D(margin + boxW, row2Y + boxH));

    // Deep blue
    gal->SetFillColor(COLOR4D(0.05, 0.08, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + boxW + 20, row2Y),
                       VECTOR2D(margin + boxW * 2 + 20, row2Y + boxH));

    // Pure white
    gal->SetFillColor(COLOR4D(1.0, 1.0, 1.0, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 2, row2Y),
                       VECTOR2D(margin + boxW * 3 + 40, row2Y + boxH));

    // Pure black
    gal->SetFillColor(COLOR4D(0.0, 0.0, 0.0, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 3, row2Y),
                       VECTOR2D(margin + boxW * 4 + 60, row2Y + boxH));

    // Row 3: Demonstrate content on different backgrounds
    double row3Y = row2Y + boxH + 30;

    // Dark background with light content
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(margin, row3Y), VECTOR2D(margin + boxW, row3Y + boxH));

    gal->SetLayerDepth(50);
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    gal->DrawCircle(VECTOR2D(margin + boxW/2 - 30, row3Y + boxH/2), 20);
    gal->DrawCircle(VECTOR2D(margin + boxW/2 + 30, row3Y + boxH/2), 20);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.9, 0.9, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + 30, row3Y + 30), VECTOR2D(margin + boxW - 30, row3Y + boxH - 30));

    // Light background with dark content
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.95, 0.95, 0.92, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + boxW + 20, row3Y),
                       VECTOR2D(margin + boxW * 2 + 20, row3Y + boxH));

    gal->SetLayerDepth(50);
    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.3, 1.0));
    gal->DrawCircle(VECTOR2D(margin + boxW * 1.5 + 20 - 30, row3Y + boxH/2), 20);
    gal->DrawCircle(VECTOR2D(margin + boxW * 1.5 + 20 + 30, row3Y + boxH/2), 20);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.1, 0.1, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + boxW + 50, row3Y + 30),
                       VECTOR2D(margin + boxW * 2 - 10, row3Y + boxH - 30));

    // PCB green background
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.05, 0.2, 0.05, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 2, row3Y),
                       VECTOR2D(margin + boxW * 3 + 40, row3Y + boxH));

    gal->SetLayerDepth(50);
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.2, 1.0));
    double pcbX = margin + (boxW + 20) * 2 + boxW/2;
    gal->DrawSegment(VECTOR2D(pcbX - 50, row3Y + 40), VECTOR2D(pcbX + 50, row3Y + 40), 6);
    gal->DrawSegment(VECTOR2D(pcbX - 50, row3Y + boxH - 40), VECTOR2D(pcbX + 50, row3Y + boxH - 40), 6);
    gal->SetFillColor(COLOR4D(0.9, 0.7, 0.3, 1.0));
    gal->DrawCircle(VECTOR2D(pcbX - 30, row3Y + 40), 10);
    gal->DrawCircle(VECTOR2D(pcbX + 30, row3Y + 40), 10);
    gal->DrawCircle(VECTOR2D(pcbX - 30, row3Y + boxH - 40), 10);
    gal->DrawCircle(VECTOR2D(pcbX + 30, row3Y + boxH - 40), 10);

    // Blue schematic background
    gal->SetLayerDepth(100);
    gal->SetFillColor(COLOR4D(0.9, 0.95, 1.0, 1.0));
    gal->DrawRectangle(VECTOR2D(margin + (boxW + 20) * 3, row3Y),
                       VECTOR2D(margin + boxW * 4 + 60, row3Y + boxH));

    gal->SetLayerDepth(50);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.0, 0.4, 0.0, 1.0));
    double schX = margin + (boxW + 20) * 3 + boxW/2;
    gal->DrawLine(VECTOR2D(schX - 60, row3Y + boxH/2), VECTOR2D(schX - 20, row3Y + boxH/2));
    gal->DrawLine(VECTOR2D(schX + 20, row3Y + boxH/2), VECTOR2D(schX + 60, row3Y + boxH/2));
    gal->DrawRectangle(VECTOR2D(schX - 20, row3Y + boxH/2 - 25), VECTOR2D(schX + 20, row3Y + boxH/2 + 25));
    gal->SetStrokeColor(COLOR4D(0.8, 0.0, 0.0, 1.0));
    gal->DrawCircle(VECTOR2D(schX - 60, row3Y + boxH/2), 5);
    gal->DrawCircle(VECTOR2D(schX + 60, row3Y + boxH/2), 5);

    // Row 4: Test actual SetClearColor API (affects next frame)
    double row4Y = row3Y + boxH + 30;
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Show the API being called (visual marker)
    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.25, 1.0));
    gal->DrawRectangle(VECTOR2D(margin, row4Y), VECTOR2D(width - margin, row4Y + 60));

    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(margin, row4Y), VECTOR2D(width - margin, row4Y + 60));

    // Demonstrate SetClearColor API call
    COLOR4D clearColor(0.1, 0.1, 0.15, 1.0);
    gal->SetClearColor(clearColor);
    // Note: The clear color will be used on next ClearScreen() call
}

}  // namespace GALTest
