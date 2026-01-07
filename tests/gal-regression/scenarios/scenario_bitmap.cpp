/**
 * Bitmap Scenario
 *
 * Tests GAL DrawBitmap() method using BITMAP_BASE with various test patterns.
 *
 * DrawBitmap() renders a raster image centered at the current transformation origin.
 * In OPENGL_GAL, it uses GL_BITMAP_CACHE to create GPU textures from wxImage data.
 * Position is controlled via Save/Translate/Restore, not by arguments to DrawBitmap.
 *
 * This scenario demonstrates:
 * 1. Basic bitmap rendering with checkerboard pattern
 * 2. Gradient patterns (horizontal, vertical, radial)
 * 3. Custom KiCad logo-style pattern
 * 4. Different bitmap sizes
 * 5. Multiple bitmaps in a scene
 */

#include <gal/graphics_abstraction_layer.h>
#include "../native/bitmap_base_stub.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderBitmap(GAL* gal, int width, int height) {
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background
    gal->SetFillColor(COLOR4D(0.1, 0.1, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    //=========================================================================
    // Section 1: Basic checkerboard bitmap
    //=========================================================================
    gal->SetLayerDepth(50);

    // Create checkerboard bitmap (64x64)
    auto checkerboard = CreateCheckerboardBitmap(64, 64, 8);

    // Position bitmap at (80, 80) - use transform
    gal->Save();
    gal->Translate(VECTOR2D(100, 100));  // Center position
    gal->DrawBitmap(*checkerboard, 1.0);
    gal->Restore();

    // Section label frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(180, 180));

    //=========================================================================
    // Section 2: Gradient bitmap (horizontal)
    //=========================================================================
    gal->SetLayerDepth(50);

    auto gradient = CreateGradientBitmap(80, 60);

    gal->Save();
    gal->Translate(VECTOR2D(300, 100));
    gal->DrawBitmap(*gradient, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.5, 0.4, 0.4, 0.8));
    gal->DrawRectangle(VECTOR2D(200, 20), VECTOR2D(400, 180));

    //=========================================================================
    // Section 3: KiCad logo pattern
    //=========================================================================
    gal->SetLayerDepth(50);

    auto logo = CreateKiCadLogoBitmap(80, 80);

    gal->Save();
    gal->Translate(VECTOR2D(520, 100));
    gal->DrawBitmap(*logo, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.4, 0.5, 0.3, 0.8));
    gal->DrawRectangle(VECTOR2D(420, 20), VECTOR2D(620, 180));

    //=========================================================================
    // Section 4: Radial gradient
    //=========================================================================
    gal->SetLayerDepth(50);

    auto radial = CreateRadialBitmap(64, 64);

    gal->Save();
    gal->Translate(VECTOR2D(720, 100));
    gal->DrawBitmap(*radial, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.4, 0.4, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(640, 20), VECTOR2D(800, 180));

    //=========================================================================
    // Section 5: Different bitmap sizes
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 200), VECTOR2D(380, 380));

    // Small bitmap (32x32)
    auto small = CreateCheckerboardBitmap(32, 32, 4);
    gal->Save();
    gal->Translate(VECTOR2D(80, 280));
    gal->DrawBitmap(*small, 1.0);
    gal->Restore();

    // Medium bitmap (64x64)
    auto medium = CreateCheckerboardBitmap(64, 64, 8);
    gal->Save();
    gal->Translate(VECTOR2D(180, 290));
    gal->DrawBitmap(*medium, 1.0);
    gal->Restore();

    // Large bitmap (96x96)
    auto large = CreateCheckerboardBitmap(96, 96, 12);
    gal->Save();
    gal->Translate(VECTOR2D(300, 290));
    gal->DrawBitmap(*large, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.4, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 200), VECTOR2D(380, 380));

    //=========================================================================
    // Section 6: Striped patterns
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.15, 0.12, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 200), VECTOR2D(600, 380));

    // Horizontal stripes
    auto hStripes = CreateStripedBitmap(64, 64, true);
    gal->Save();
    gal->Translate(VECTOR2D(460, 290));
    gal->DrawBitmap(*hStripes, 1.0);
    gal->Restore();

    // Vertical stripes
    auto vStripes = CreateStripedBitmap(64, 64, false);
    gal->Save();
    gal->Translate(VECTOR2D(550, 290));
    gal->DrawBitmap(*vStripes, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.5, 0.4, 0.4, 0.8));
    gal->DrawRectangle(VECTOR2D(400, 200), VECTOR2D(600, 380));

    //=========================================================================
    // Section 7: Multiple bitmaps composition - colored checkerboards
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.12, 0.15, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(620, 200), VECTOR2D(800, 380));

    // Create a variety of colored checkerboards in a grid
    wxImage img1 = CreateCheckerboardImage(40, 40, 5, 255, 200, 200, 100, 50, 50);
    wxImage img2 = CreateCheckerboardImage(40, 40, 5, 200, 255, 200, 50, 100, 50);
    wxImage img3 = CreateCheckerboardImage(40, 40, 5, 200, 200, 255, 50, 50, 100);
    wxImage img4 = CreateCheckerboardImage(40, 40, 5, 255, 255, 200, 100, 100, 50);

    auto bmp1 = std::make_unique<BITMAP_BASE>(); bmp1->SetImage(img1);
    auto bmp2 = std::make_unique<BITMAP_BASE>(); bmp2->SetImage(img2);
    auto bmp3 = std::make_unique<BITMAP_BASE>(); bmp3->SetImage(img3);
    auto bmp4 = std::make_unique<BITMAP_BASE>(); bmp4->SetImage(img4);

    gal->Save();
    gal->Translate(VECTOR2D(670, 260));
    gal->DrawBitmap(*bmp1, 1.0);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(750, 260));
    gal->DrawBitmap(*bmp2, 1.0);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(670, 330));
    gal->DrawBitmap(*bmp3, 1.0);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(750, 330));
    gal->DrawBitmap(*bmp4, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.4, 0.5, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(620, 200), VECTOR2D(800, 380));

    //=========================================================================
    // Section 8: Bitmap with surrounding graphics
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.1, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 400), VECTOR2D(400, 580));

    // Central bitmap
    auto central = CreateKiCadLogoBitmap(100, 100);
    gal->Save();
    gal->Translate(VECTOR2D(210, 490));
    gal->DrawBitmap(*central, 1.0);
    gal->Restore();

    // Decorative circles around bitmap
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);

    COLOR4D circleColors[] = {
        COLOR4D(0.9, 0.4, 0.4, 0.7),
        COLOR4D(0.4, 0.9, 0.4, 0.7),
        COLOR4D(0.4, 0.4, 0.9, 0.7),
        COLOR4D(0.9, 0.9, 0.4, 0.7)
    };

    for (int i = 0; i < 4; i++) {
        gal->SetStrokeColor(circleColors[i]);
        double angle = i * M_PI / 2;
        double cx = 210 + cos(angle) * 80;
        double cy = 490 + sin(angle) * 80;
        gal->DrawCircle(VECTOR2D(cx, cy), 15);
    }

    // Connecting lines
    gal->SetLineWidth(1.5);
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.7, 0.5));
    for (int i = 0; i < 4; i++) {
        double angle1 = i * M_PI / 2;
        double angle2 = ((i + 1) % 4) * M_PI / 2;
        VECTOR2D p1(210 + cos(angle1) * 80, 490 + sin(angle1) * 80);
        VECTOR2D p2(210 + cos(angle2) * 80, 490 + sin(angle2) * 80);
        gal->DrawLine(p1, p2);
    }

    // Section frame
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.4, 0.4, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 400), VECTOR2D(400, 580));

    //=========================================================================
    // Section 9: Gradient showcase
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.12, 0.1, 0.12, 1.0));
    gal->DrawRectangle(VECTOR2D(420, 400), VECTOR2D(800, 580));

    // Horizontal gradient
    wxImage hGradImg = CreateGradientHImage(100, 40, 255, 100, 100, 100, 100, 255);
    auto hGradBitmap = std::make_unique<BITMAP_BASE>();
    hGradBitmap->SetImage(hGradImg);
    gal->Save();
    gal->Translate(VECTOR2D(520, 450));
    gal->DrawBitmap(*hGradBitmap, 1.0);
    gal->Restore();

    // Vertical gradient
    wxImage vGradImg = CreateGradientVImage(100, 40, 100, 255, 100, 100, 100, 255);
    auto vGradBitmap = std::make_unique<BITMAP_BASE>();
    vGradBitmap->SetImage(vGradImg);
    gal->Save();
    gal->Translate(VECTOR2D(520, 510));
    gal->DrawBitmap(*vGradBitmap, 1.0);
    gal->Restore();

    // Radial gradient (larger)
    wxImage radialImg = CreateRadialGradientImage(80, 80, 255, 255, 100, 100, 50, 150);
    auto radialBitmap = std::make_unique<BITMAP_BASE>();
    radialBitmap->SetImage(radialImg);
    gal->Save();
    gal->Translate(VECTOR2D(700, 490));
    gal->DrawBitmap(*radialBitmap, 1.0);
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.5, 0.4, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(420, 400), VECTOR2D(800, 580));
}

}  // namespace GALTest
