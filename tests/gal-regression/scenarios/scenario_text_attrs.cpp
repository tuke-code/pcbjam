/**
 * Text Attributes Scenario
 *
 * Tests GAL text attribute methods:
 * - SetGlyphSize() / GetGlyphSize()
 * - SetFontBold() / IsFontBold()
 * - SetFontItalic() / IsFontItalic()
 * - SetFontUnderlined() / IsFontUnderlined()
 * - SetTextMirrored() / IsTextMirrored()
 * - SetHorizontalJustify() / GetHorizontalJustify()
 * - SetVerticalJustify() / GetVerticalJustify()
 * - ResetTextAttributes()
 *
 * Note: These methods set internal m_attributes member variables.
 * Actual text rendering requires KIFONT infrastructure which we don't stub.
 * This scenario tests the APIs are callable and demonstrates their purpose
 * by drawing visual indicators of what the attributes would affect.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderTextAttrs(GAL* gal, int width, int height) {
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    //=========================================================================
    // Section 1: SetGlyphSize() / GetGlyphSize()
    //=========================================================================
    gal->SetLayerDepth(50);

    // Test SetGlyphSize with different sizes
    VECTOR2D smallSize(10, 12);
    VECTOR2D mediumSize(20, 24);
    VECTOR2D largeSize(40, 48);

    gal->SetGlyphSize(smallSize);
    VECTOR2D currentSize = gal->GetGlyphSize();
    // currentSize should now be (10, 12)

    gal->SetGlyphSize(mediumSize);
    gal->SetGlyphSize(largeSize);

    // Visual: Draw rectangles showing glyph sizes
    double baseX = 50, baseY = 60;
    gal->SetFillColor(COLOR4D(0.3, 0.6, 0.8, 0.8));
    gal->DrawRectangle(VECTOR2D(baseX, baseY),
                       VECTOR2D(baseX + smallSize.x, baseY + smallSize.y));

    gal->SetFillColor(COLOR4D(0.4, 0.7, 0.8, 0.8));
    gal->DrawRectangle(VECTOR2D(baseX + 30, baseY),
                       VECTOR2D(baseX + 30 + mediumSize.x, baseY + mediumSize.y));

    gal->SetFillColor(COLOR4D(0.5, 0.8, 0.8, 0.8));
    gal->DrawRectangle(VECTOR2D(baseX + 80, baseY),
                       VECTOR2D(baseX + 80 + largeSize.x, baseY + largeSize.y));

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.4, 0.6, 0.7, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(200, 130));

    //=========================================================================
    // Section 2: SetFontBold() / SetFontItalic() / SetFontUnderlined()
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Test font style APIs
    gal->SetFontBold(false);
    bool isBold = gal->IsFontBold();  // Should be false

    gal->SetFontItalic(false);
    bool isItalic = gal->IsFontItalic();  // Should be false

    gal->SetFontUnderlined(false);
    bool isUnderlined = gal->IsFontUnderlined();  // Should be false

    // Set all to true
    gal->SetFontBold(true);
    gal->SetFontItalic(true);
    gal->SetFontUnderlined(true);

    // Visual: Draw styled "text" indicators
    // Normal (N)
    gal->SetFillColor(COLOR4D(0.7, 0.7, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(240, 40), VECTOR2D(280, 80));

    // Bold (B) - thicker
    gal->SetFillColor(COLOR4D(0.9, 0.9, 0.9, 1.0));
    gal->DrawRectangle(VECTOR2D(300, 40), VECTOR2D(350, 80));

    // Italic (I) - slanted parallelogram
    std::deque<VECTOR2D> italic = {
        VECTOR2D(380, 80),
        VECTOR2D(370, 40),
        VECTOR2D(410, 40),
        VECTOR2D(420, 80)
    };
    gal->DrawPolygon(italic);

    // Underlined (U)
    gal->SetFillColor(COLOR4D(0.7, 0.7, 0.9, 1.0));
    gal->DrawRectangle(VECTOR2D(440, 40), VECTOR2D(480, 80));
    gal->DrawRectangle(VECTOR2D(440, 85), VECTOR2D(480, 90));  // Underline

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(220, 20), VECTOR2D(500, 110));

    //=========================================================================
    // Section 3: SetTextMirrored()
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Test mirroring API
    gal->SetTextMirrored(false);
    bool isMirrored = gal->IsTextMirrored();  // Should be false

    gal->SetTextMirrored(true);
    isMirrored = gal->IsTextMirrored();  // Should be true

    // Visual: Show normal vs mirrored "F" shape
    // Normal F
    gal->SetFillColor(COLOR4D(0.7, 0.5, 0.8, 1.0));
    gal->DrawRectangle(VECTOR2D(560, 40), VECTOR2D(570, 100));  // Vertical
    gal->DrawRectangle(VECTOR2D(570, 40), VECTOR2D(600, 50));   // Top horizontal
    gal->DrawRectangle(VECTOR2D(570, 60), VECTOR2D(590, 70));   // Middle horizontal

    // Mirrored F
    gal->SetFillColor(COLOR4D(0.8, 0.5, 0.7, 1.0));
    gal->DrawRectangle(VECTOR2D(680, 40), VECTOR2D(690, 100));  // Vertical
    gal->DrawRectangle(VECTOR2D(650, 40), VECTOR2D(680, 50));   // Top horizontal (mirrored)
    gal->DrawRectangle(VECTOR2D(660, 60), VECTOR2D(680, 70));   // Middle horizontal (mirrored)

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.6, 0.5, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(540, 20), VECTOR2D(720, 120));

    //=========================================================================
    // Section 4: SetHorizontalJustify() / SetVerticalJustify()
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Test justify APIs
    gal->SetHorizontalJustify(GR_TEXT_H_ALIGN_LEFT);
    gal->SetVerticalJustify(GR_TEXT_V_ALIGN_TOP);

    gal->SetHorizontalJustify(GR_TEXT_H_ALIGN_CENTER);
    gal->SetVerticalJustify(GR_TEXT_V_ALIGN_CENTER);

    gal->SetHorizontalJustify(GR_TEXT_H_ALIGN_RIGHT);
    gal->SetVerticalJustify(GR_TEXT_V_ALIGN_BOTTOM);

    // Visual: Show alignment positions
    double alignBaseX = 120;
    double alignBaseY = 200;
    double boxSize = 100;

    // Alignment box
    gal->SetFillColor(COLOR4D(0.2, 0.2, 0.25, 1.0));
    gal->DrawRectangle(VECTOR2D(alignBaseX, alignBaseY),
                       VECTOR2D(alignBaseX + boxSize, alignBaseY + boxSize));

    // Alignment indicators
    gal->SetFillColor(COLOR4D(0.9, 0.6, 0.3, 1.0));

    // Top-left
    gal->DrawCircle(VECTOR2D(alignBaseX + 10, alignBaseY + 10), 6);
    // Top-center
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize/2, alignBaseY + 10), 6);
    // Top-right
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize - 10, alignBaseY + 10), 6);

    // Center-left
    gal->DrawCircle(VECTOR2D(alignBaseX + 10, alignBaseY + boxSize/2), 6);
    // Center-center
    gal->SetFillColor(COLOR4D(1.0, 0.8, 0.3, 1.0));
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize/2, alignBaseY + boxSize/2), 8);
    gal->SetFillColor(COLOR4D(0.9, 0.6, 0.3, 1.0));
    // Center-right
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize - 10, alignBaseY + boxSize/2), 6);

    // Bottom-left
    gal->DrawCircle(VECTOR2D(alignBaseX + 10, alignBaseY + boxSize - 10), 6);
    // Bottom-center
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize/2, alignBaseY + boxSize - 10), 6);
    // Bottom-right
    gal->DrawCircle(VECTOR2D(alignBaseX + boxSize - 10, alignBaseY + boxSize - 10), 6);

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.7, 0.5, 0.3, 0.8));
    gal->DrawRectangle(VECTOR2D(alignBaseX - 20, alignBaseY - 30),
                       VECTOR2D(alignBaseX + boxSize + 20, alignBaseY + boxSize + 20));

    //=========================================================================
    // Section 5: ResetTextAttributes()
    //=========================================================================
    gal->SetLayerDepth(50);

    // Set various attributes
    gal->SetGlyphSize(VECTOR2D(50, 60));
    gal->SetFontBold(true);
    gal->SetFontItalic(true);
    gal->SetTextMirrored(true);
    gal->SetHorizontalJustify(GR_TEXT_H_ALIGN_RIGHT);

    // Reset all attributes to defaults
    gal->ResetTextAttributes();

    // After reset, attributes should be back to defaults
    VECTOR2D resetSize = gal->GetGlyphSize();  // Should be default
    bool resetBold = gal->IsFontBold();        // Should be false
    bool resetItalic = gal->IsFontItalic();    // Should be false

    // Visual: Show "reset" indicator
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.3, 0.7, 0.3, 0.8));
    gal->DrawCircle(VECTOR2D(350, 250), 30);

    // Checkmark inside circle
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(4.0);
    gal->SetStrokeColor(COLOR4D(1.0, 1.0, 1.0, 1.0));
    std::vector<VECTOR2D> check = {
        VECTOR2D(335, 250),
        VECTOR2D(345, 262),
        VECTOR2D(368, 235)
    };
    gal->DrawPolyline(check);

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.3, 0.6, 0.3, 0.8));
    gal->DrawRectangle(VECTOR2D(280, 180), VECTOR2D(420, 310));

    //=========================================================================
    // Section 6: Comprehensive API coverage visual
    //=========================================================================
    gal->SetLayerDepth(40);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 340), VECTOR2D(760, 560));

    // Show all text attribute concepts visually
    double rowY = 380;
    double colSpacing = 150;

    // Column 1: Size variations
    gal->SetFillColor(COLOR4D(0.5, 0.7, 0.9, 0.9));
    double sizes[] = {8, 16, 24, 32};
    for (int i = 0; i < 4; i++) {
        double s = sizes[i];
        gal->DrawRectangle(VECTOR2D(40, rowY + i * 40),
                           VECTOR2D(40 + s * 2, rowY + i * 40 + s));
    }

    // Column 2: Style variations (bold = filled, italic = slant, underline = line below)
    double col2X = 40 + colSpacing;

    // Regular
    gal->SetFillColor(COLOR4D(0.6, 0.6, 0.6, 0.9));
    gal->DrawRectangle(VECTOR2D(col2X, rowY), VECTOR2D(col2X + 40, rowY + 30));

    // Bold (wider/heavier)
    gal->SetFillColor(COLOR4D(0.9, 0.9, 0.9, 1.0));
    gal->DrawRectangle(VECTOR2D(col2X, rowY + 45), VECTOR2D(col2X + 50, rowY + 80));

    // Italic (parallelogram)
    std::deque<VECTOR2D> italicShape = {
        VECTOR2D(col2X + 10, rowY + 125),
        VECTOR2D(col2X, rowY + 90),
        VECTOR2D(col2X + 40, rowY + 90),
        VECTOR2D(col2X + 50, rowY + 125)
    };
    gal->SetFillColor(COLOR4D(0.7, 0.7, 0.9, 0.9));
    gal->DrawPolygon(italicShape);

    // Underlined
    gal->SetFillColor(COLOR4D(0.7, 0.9, 0.7, 0.9));
    gal->DrawRectangle(VECTOR2D(col2X, rowY + 140), VECTOR2D(col2X + 40, rowY + 165));
    gal->DrawRectangle(VECTOR2D(col2X, rowY + 170), VECTOR2D(col2X + 40, rowY + 175));

    // Column 3: Mirror demonstration
    double col3X = 40 + colSpacing * 2;

    // Normal "R"
    gal->SetFillColor(COLOR4D(0.8, 0.6, 0.9, 0.9));
    gal->DrawRectangle(VECTOR2D(col3X, rowY), VECTOR2D(col3X + 10, rowY + 50));
    gal->DrawRectangle(VECTOR2D(col3X + 10, rowY), VECTOR2D(col3X + 35, rowY + 10));
    gal->DrawRectangle(VECTOR2D(col3X + 10, rowY + 20), VECTOR2D(col3X + 30, rowY + 30));
    gal->DrawRectangle(VECTOR2D(col3X + 25, rowY + 25), VECTOR2D(col3X + 40, rowY + 50));

    // Mirrored "R"
    gal->SetFillColor(COLOR4D(0.9, 0.6, 0.8, 0.9));
    double mirrorX = col3X + 80;
    gal->DrawRectangle(VECTOR2D(mirrorX + 30, rowY + 80), VECTOR2D(mirrorX + 40, rowY + 130));
    gal->DrawRectangle(VECTOR2D(mirrorX + 5, rowY + 80), VECTOR2D(mirrorX + 30, rowY + 90));
    gal->DrawRectangle(VECTOR2D(mirrorX + 10, rowY + 100), VECTOR2D(mirrorX + 30, rowY + 110));
    gal->DrawRectangle(VECTOR2D(mirrorX, rowY + 105), VECTOR2D(mirrorX + 15, rowY + 130));

    // Column 4: Justify grid
    double col4X = 40 + colSpacing * 3;
    double gridSize = 80;

    gal->SetFillColor(COLOR4D(0.25, 0.25, 0.3, 1.0));
    gal->DrawRectangle(VECTOR2D(col4X, rowY), VECTOR2D(col4X + gridSize, rowY + gridSize));

    // 3x3 justify positions
    gal->SetFillColor(COLOR4D(0.9, 0.7, 0.4, 1.0));
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            double px = col4X + 10 + col * 30;
            double py = rowY + 10 + row * 30;
            double radius = (row == 1 && col == 1) ? 8 : 5;
            gal->DrawCircle(VECTOR2D(px, py), radius);
        }
    }

    // Column 5: Rotation demonstration
    double col5X = 40 + colSpacing * 4;

    for (int i = 0; i < 4; i++) {
        double angle = i * M_PI / 6;  // 0, 30, 60, 90 degrees
        double cx = col5X + 40;
        double cy = rowY + 40 + i * 45;

        gal->Save();
        gal->Translate(VECTOR2D(cx, cy));
        gal->Rotate(angle);

        gal->SetFillColor(COLOR4D(0.6 + i * 0.1, 0.8 - i * 0.1, 0.5, 0.9));
        gal->DrawRectangle(VECTOR2D(-15, -8), VECTOR2D(15, 8));

        gal->Restore();
    }

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 340), VECTOR2D(760, 560));
}

}  // namespace GALTest
