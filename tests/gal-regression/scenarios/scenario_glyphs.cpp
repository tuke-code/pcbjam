/**
 * Glyphs Scenario
 *
 * Tests GAL DrawGlyph() and DrawGlyphs() methods using STROKE_GLYPH.
 *
 * STROKE_GLYPH inherits from std::vector<std::vector<VECTOR2D>>, where each
 * inner vector is a stroke path (pen down to pen up). DrawGlyph() for stroke
 * glyphs internally calls DrawPolylines() to render the strokes.
 *
 * This scenario demonstrates:
 * 1. Single glyph rendering with DrawGlyph()
 * 2. Multiple glyph rendering with DrawGlyphs()
 * 3. Different glyph sizes and positions
 * 4. "KICAD" text spelling using stroke glyphs
 */

#include <gal/graphics_abstraction_layer.h>
#include "../native/kifont_stub.h"
#include <cmath>
#include <vector>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderGlyphs(GAL* gal, int width, int height) {
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    //=========================================================================
    // Section 1: Single DrawGlyph() calls
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(3.0);

    // Letter F
    gal->SetStrokeColor(COLOR4D(0.9, 0.3, 0.3, 1.0));
    auto glyphF = KIFONT::MakeLetterF(0.8, VECTOR2D(50, 50));
    gal->DrawGlyph(*glyphF, 0, 1);

    // Letter L
    gal->SetStrokeColor(COLOR4D(0.3, 0.9, 0.3, 1.0));
    auto glyphL = KIFONT::MakeLetterL(0.8, VECTOR2D(130, 50));
    gal->DrawGlyph(*glyphL, 0, 1);

    // Letter K
    gal->SetStrokeColor(COLOR4D(0.3, 0.3, 0.9, 1.0));
    auto glyphK = KIFONT::MakeLetterK(0.8, VECTOR2D(200, 50));
    gal->DrawGlyph(*glyphK, 0, 1);

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.4, 0.4, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(280, 180));

    //=========================================================================
    // Section 2: DrawGlyphs() with vector of glyphs
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetLineWidth(3.0);
    gal->SetStrokeColor(COLOR4D(0.9, 0.7, 0.2, 1.0));

    // Create "KICAD" using stroke glyphs
    std::vector<std::unique_ptr<KIFONT::GLYPH>> kicadGlyphs;

    kicadGlyphs.push_back(KIFONT::MakeLetterK(0.7, VECTOR2D(320, 60)));
    kicadGlyphs.push_back(KIFONT::MakeLetterI(0.7, VECTOR2D(390, 60)));
    kicadGlyphs.push_back(KIFONT::MakeLetterC(0.7, VECTOR2D(440, 60)));
    kicadGlyphs.push_back(KIFONT::MakeLetterA(0.7, VECTOR2D(510, 60)));
    kicadGlyphs.push_back(KIFONT::MakeLetterD(0.7, VECTOR2D(590, 60)));

    // Draw all glyphs at once
    gal->DrawGlyphs(kicadGlyphs);

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.6, 0.5, 0.2, 0.8));
    gal->DrawRectangle(VECTOR2D(300, 20), VECTOR2D(680, 180));

    //=========================================================================
    // Section 3: Different sizes demonstration
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetLineWidth(2.0);

    double scales[] = {0.3, 0.5, 0.8, 1.2};
    COLOR4D colors[] = {
        COLOR4D(0.6, 0.8, 0.9, 1.0),
        COLOR4D(0.7, 0.9, 0.8, 1.0),
        COLOR4D(0.9, 0.8, 0.7, 1.0),
        COLOR4D(0.9, 0.7, 0.8, 1.0)
    };

    double xPos = 50;
    for (int i = 0; i < 4; i++) {
        gal->SetStrokeColor(colors[i]);
        gal->SetLineWidth(1.5 + i * 0.5);

        auto glyph = KIFONT::MakeLetterF(scales[i], VECTOR2D(xPos, 220));
        gal->DrawGlyph(*glyph, i, 4);  // Pass aNth and aTotal

        xPos += 60 * scales[i] + 30;
    }

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.6, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 200), VECTOR2D(380, 360));

    //=========================================================================
    // Section 4: Complex glyph composition
    //=========================================================================
    gal->SetLayerDepth(50);

    // Background panel
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.15, 0.18, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 200), VECTOR2D(760, 360));

    // Draw multiple letters in a grid pattern
    gal->SetIsFill(false);
    gal->SetIsStroke(true);

    std::vector<std::unique_ptr<KIFONT::GLYPH>> gridGlyphs;

    // Row 1: FLKA
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.7, 0.9, 0.6, 1.0));
    gridGlyphs.push_back(KIFONT::MakeLetterF(0.5, VECTOR2D(420, 220)));
    gridGlyphs.push_back(KIFONT::MakeLetterL(0.5, VECTOR2D(480, 220)));
    gridGlyphs.push_back(KIFONT::MakeLetterK(0.5, VECTOR2D(540, 220)));
    gridGlyphs.push_back(KIFONT::MakeLetterA(0.5, VECTOR2D(600, 220)));

    gal->DrawGlyphs(gridGlyphs);
    gridGlyphs.clear();

    // Row 2: ICDA
    gal->SetStrokeColor(COLOR4D(0.6, 0.7, 0.9, 1.0));
    gridGlyphs.push_back(KIFONT::MakeLetterI(0.5, VECTOR2D(420, 290)));
    gridGlyphs.push_back(KIFONT::MakeLetterC(0.5, VECTOR2D(480, 290)));
    gridGlyphs.push_back(KIFONT::MakeLetterD(0.5, VECTOR2D(540, 290)));
    gridGlyphs.push_back(KIFONT::MakeLetterA(0.5, VECTOR2D(600, 290)));

    gal->DrawGlyphs(gridGlyphs);

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.4, 0.6, 0.4, 0.8));
    gal->DrawRectangle(VECTOR2D(400, 200), VECTOR2D(760, 360));

    //=========================================================================
    // Section 5: Styled text with transforms
    //=========================================================================
    gal->SetLayerDepth(50);

    // Background panel
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.18, 0.15, 0.18, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 380), VECTOR2D(380, 560));

    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(3.0);

    // Rotated letters
    double angles[] = {-15, 0, 15, 30};
    double xPositions[] = {60, 130, 200, 270};

    for (int i = 0; i < 4; i++) {
        gal->Save();
        gal->Translate(VECTOR2D(xPositions[i] + 30, 470));
        gal->Rotate(angles[i] * M_PI / 180.0);

        double t = (double)i / 3.0;
        gal->SetStrokeColor(COLOR4D(0.9 - t * 0.3, 0.4 + t * 0.4, 0.6 + t * 0.3, 1.0));

        auto glyph = KIFONT::MakeLetterF(0.6, VECTOR2D(-20, -40));
        gal->DrawGlyph(*glyph, i, 4);

        gal->Restore();
    }

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.6, 0.4, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 380), VECTOR2D(380, 560));

    //=========================================================================
    // Section 6: Full "KICAD" banner
    //=========================================================================
    gal->SetLayerDepth(40);

    // Background panel
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.1, 0.15, 0.2, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 380), VECTOR2D(760, 560));

    // Large KICAD text
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(4.0);
    gal->SetStrokeColor(COLOR4D(1.0, 0.85, 0.3, 1.0));

    std::vector<std::unique_ptr<KIFONT::GLYPH>> bannerGlyphs;
    double bannerScale = 1.0;
    double bannerX = 420;
    double bannerY = 420;
    double spacing = 70;

    bannerGlyphs.push_back(KIFONT::MakeLetterK(bannerScale, VECTOR2D(bannerX, bannerY)));
    bannerGlyphs.push_back(KIFONT::MakeLetterI(bannerScale, VECTOR2D(bannerX + spacing, bannerY)));
    bannerGlyphs.push_back(KIFONT::MakeLetterC(bannerScale, VECTOR2D(bannerX + spacing * 2, bannerY)));
    bannerGlyphs.push_back(KIFONT::MakeLetterA(bannerScale, VECTOR2D(bannerX + spacing * 3, bannerY)));
    bannerGlyphs.push_back(KIFONT::MakeLetterD(bannerScale, VECTOR2D(bannerX + spacing * 4, bannerY)));

    gal->DrawGlyphs(bannerGlyphs);

    // Underline decoration
    gal->SetLineWidth(3.0);
    gal->SetStrokeColor(COLOR4D(0.8, 0.6, 0.2, 0.8));
    gal->DrawLine(VECTOR2D(bannerX, bannerY + 110), VECTOR2D(bannerX + spacing * 4 + 50, bannerY + 110));

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.3, 0.8));
    gal->DrawRectangle(VECTOR2D(400, 380), VECTOR2D(760, 560));
}

}  // namespace GALTest
