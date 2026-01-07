/**
 * Transform() API Documentation Scenario
 *
 * Tests GAL::Transform(const MATRIX3x3D& aTransformation)
 *
 * IMPORTANT FINDING: Transform() is DEAD CODE in KiCad!
 *
 * Research revealed that:
 * 1. Transform() is declared in GAL base class with empty default implementation
 * 2. Implemented in both OPENGL_GAL (glMultMatrixd) and CAIRO_GAL
 * 3. NEVER called anywhere in the entire KiCad codebase
 * 4. KiCad uses Rotate(), Translate(), Scale() instead - which work correctly
 *
 * Why Transform() doesn't produce visible output in OPENGL_GAL:
 * - Uses glMultMatrixd() which affects the legacy GL_MODELVIEW matrix stack
 * - OPENGL_GAL renders via VERTEX_MANAGER which has its own m_transform member
 * - The two transformation systems are completely independent
 *
 * This scenario:
 * 1. Calls Transform() API to verify it doesn't crash
 * 2. Documents the limitation
 * 3. Shows what the expected result WOULD be if it worked
 */

#include <gal/graphics_abstraction_layer.h>
#include <math/matrix3x3.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderTransformAPI(GAL* gal, int width, int height) {
    gal->SetLayerDepth(100);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background
    gal->SetFillColor(COLOR4D(0.12, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(0, 0), VECTOR2D(width, height));

    //=========================================================================
    // Section 1: Transform() API call demonstration
    //=========================================================================
    gal->SetLayerDepth(50);

    // Create a 3x3 identity transformation matrix
    MATRIX3x3D identity;
    identity.SetIdentity();

    // Call Transform() with identity - should be a no-op
    // This verifies the API is callable without crashing
    gal->Transform(identity);

    // Create a rotation matrix (45 degrees)
    MATRIX3x3D rotationMatrix;
    rotationMatrix.SetIdentity();
    double angle = 45.0 * M_PI / 180.0;
    rotationMatrix.m_data[0][0] = cos(angle);
    rotationMatrix.m_data[0][1] = -sin(angle);
    rotationMatrix.m_data[1][0] = sin(angle);
    rotationMatrix.m_data[1][1] = cos(angle);

    // Call Transform() with rotation matrix
    // NOTE: In OPENGL_GAL this affects GL_MODELVIEW but NOT the VERTEX_MANAGER
    // So subsequent drawing will NOT be transformed
    gal->Transform(rotationMatrix);

    // Draw a shape - it will NOT be rotated because VERTEX_MANAGER ignores GL_MODELVIEW
    gal->SetFillColor(COLOR4D(0.8, 0.3, 0.3, 0.5));
    gal->DrawRectangle(VECTOR2D(100, 100), VECTOR2D(200, 150));

    // Create a translation matrix
    MATRIX3x3D translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.m_data[0][2] = 50;  // Translate X by 50
    translationMatrix.m_data[1][2] = 30;  // Translate Y by 30

    // Call Transform() with translation
    gal->Transform(translationMatrix);

    // Draw another shape - also NOT translated
    gal->SetFillColor(COLOR4D(0.3, 0.8, 0.3, 0.5));
    gal->DrawRectangle(VECTOR2D(100, 180), VECTOR2D(200, 230));

    // Reset with identity
    gal->Transform(identity);

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.6, 0.3, 0.3, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 20), VECTOR2D(300, 280));

    //=========================================================================
    // Section 2: What Transform() SHOULD do (simulated with working APIs)
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Reference shape (no transform)
    gal->SetFillColor(COLOR4D(0.5, 0.5, 0.5, 0.7));
    gal->DrawRectangle(VECTOR2D(400, 100), VECTOR2D(500, 150));

    // Using working transforms (Rotate/Translate/Scale via VERTEX_MANAGER)
    gal->Save();
    gal->Translate(VECTOR2D(450, 200));
    gal->Rotate(45.0 * M_PI / 180.0);

    gal->SetFillColor(COLOR4D(0.3, 0.6, 0.9, 0.9));
    gal->DrawRectangle(VECTOR2D(-50, -25), VECTOR2D(50, 25));
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetStrokeColor(COLOR4D(0.3, 0.6, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(320, 20), VECTOR2D(580, 280));

    //=========================================================================
    // Section 3: Matrix operations visualization
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.15, 0.15, 0.18, 1.0));
    gal->DrawRectangle(VECTOR2D(600, 20), VECTOR2D(780, 280));

    // Show matrix element positions visually (3x3 grid)
    double matrixX = 640;
    double matrixY = 60;
    double cellSize = 40;

    // Grid cells
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            double x = matrixX + col * cellSize;
            double y = matrixY + row * cellSize;

            // Diagonal elements (scale/rotation) in one color
            if (row == col) {
                gal->SetFillColor(COLOR4D(0.4, 0.6, 0.8, 0.8));
            }
            // Translation column (last column) in another
            else if (col == 2 && row < 2) {
                gal->SetFillColor(COLOR4D(0.8, 0.6, 0.4, 0.8));
            }
            // Other elements
            else {
                gal->SetFillColor(COLOR4D(0.3, 0.3, 0.35, 0.8));
            }

            gal->DrawRectangle(VECTOR2D(x, y), VECTOR2D(x + cellSize - 2, y + cellSize - 2));
        }
    }

    // Frame around matrix
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.6, 0.6, 0.7, 0.8));
    gal->DrawRectangle(VECTOR2D(matrixX - 5, matrixY - 5),
                       VECTOR2D(matrixX + cellSize * 3 + 5, matrixY + cellSize * 3 + 5));

    // Section frame
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.6, 0.8));
    gal->DrawRectangle(VECTOR2D(600, 20), VECTOR2D(780, 280));

    //=========================================================================
    // Section 4: Why Transform() doesn't work - architecture diagram
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.15, 0.12, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(20, 300), VECTOR2D(380, 580));

    // "GL_MODELVIEW" box (what Transform affects)
    gal->SetFillColor(COLOR4D(0.6, 0.3, 0.3, 0.7));
    gal->DrawRectangle(VECTOR2D(50, 340), VECTOR2D(170, 400));

    // "VERTEX_MANAGER" box (what actually renders)
    gal->SetFillColor(COLOR4D(0.3, 0.6, 0.3, 0.7));
    gal->DrawRectangle(VECTOR2D(50, 440), VECTOR2D(170, 500));

    // Arrow from Transform to GL_MODELVIEW
    gal->SetFillColor(COLOR4D(0.8, 0.4, 0.4, 0.9));
    gal->DrawSegment(VECTOR2D(240, 340), VECTOR2D(175, 365), 3);

    // X mark showing no connection to VERTEX_MANAGER
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(4.0);
    gal->SetStrokeColor(COLOR4D(1.0, 0.3, 0.3, 1.0));
    gal->DrawLine(VECTOR2D(200, 430), VECTOR2D(220, 450));
    gal->DrawLine(VECTOR2D(220, 430), VECTOR2D(200, 450));

    // Arrow from Rotate/Scale/Translate to VERTEX_MANAGER
    gal->SetIsFill(true);
    gal->SetFillColor(COLOR4D(0.4, 0.8, 0.4, 0.9));
    gal->DrawSegment(VECTOR2D(240, 520), VECTOR2D(175, 475), 3);

    // "Transform()" label box
    gal->SetFillColor(COLOR4D(0.5, 0.3, 0.3, 0.6));
    gal->DrawRectangle(VECTOR2D(240, 320), VECTOR2D(350, 360));

    // "Rotate/Scale/Translate" label box
    gal->SetFillColor(COLOR4D(0.3, 0.5, 0.3, 0.6));
    gal->DrawRectangle(VECTOR2D(240, 500), VECTOR2D(350, 540));

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.3, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(20, 300), VECTOR2D(380, 580));

    //=========================================================================
    // Section 5: Cairo comparison (where Transform DOES work)
    //=========================================================================
    gal->SetLayerDepth(50);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // Background panel
    gal->SetFillColor(COLOR4D(0.12, 0.15, 0.15, 1.0));
    gal->DrawRectangle(VECTOR2D(400, 300), VECTOR2D(780, 580));

    // OpenGL box (shows limitation)
    gal->SetFillColor(COLOR4D(0.4, 0.25, 0.25, 0.8));
    gal->DrawRectangle(VECTOR2D(430, 350), VECTOR2D(560, 420));

    // Cairo box (shows working)
    gal->SetFillColor(COLOR4D(0.25, 0.4, 0.25, 0.8));
    gal->DrawRectangle(VECTOR2D(610, 350), VECTOR2D(740, 420));

    // "OpenGL: NO-OP" indicator
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(3.0);
    gal->SetStrokeColor(COLOR4D(1.0, 0.4, 0.4, 1.0));
    gal->DrawLine(VECTOR2D(470, 380), VECTOR2D(520, 390));
    gal->DrawLine(VECTOR2D(520, 380), VECTOR2D(470, 390));

    // "Cairo: WORKS" indicator (checkmark)
    gal->SetStrokeColor(COLOR4D(0.4, 1.0, 0.4, 1.0));
    std::vector<VECTOR2D> check = {
        VECTOR2D(650, 385),
        VECTOR2D(670, 400),
        VECTOR2D(710, 360)
    };
    gal->DrawPolyline(check);

    // Show result shapes
    gal->SetIsFill(true);
    gal->SetIsStroke(false);

    // OpenGL result (not transformed)
    gal->SetFillColor(COLOR4D(0.7, 0.5, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(460, 460), VECTOR2D(530, 510));

    // Cairo result (would be transformed)
    gal->Save();
    gal->Translate(VECTOR2D(675, 485));
    gal->Rotate(30.0 * M_PI / 180.0);
    gal->SetFillColor(COLOR4D(0.5, 0.7, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(-35, -25), VECTOR2D(35, 25));
    gal->Restore();

    // Section frame
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.4, 0.5, 0.5, 0.8));
    gal->DrawRectangle(VECTOR2D(400, 300), VECTOR2D(780, 580));
}

}  // namespace GALTest
