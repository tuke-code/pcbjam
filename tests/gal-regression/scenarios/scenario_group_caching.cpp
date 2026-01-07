/**
 * Group Caching Scenario
 *
 * Tests GAL group/caching methods:
 * - BeginGroup() / EndGroup() - define cached geometry
 * - DrawGroup() - render cached geometry
 * - ChangeGroupColor() - modify group color
 * - ChangeGroupDepth() - modify group depth
 *
 * Groups allow geometry to be cached in GPU memory and redrawn
 * efficiently, useful for static elements that don't change often.
 */

#include <gal/graphics_abstraction_layer.h>
#include <cmath>
#include <deque>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace GALTest {

using KIGFX::COLOR4D;
using KIGFX::GAL;

void RenderGroupCaching(GAL* gal, int width, int height) {
    double cx = width / 2.0;
    double cy = height / 2.0;

    // Create a group with a simple pattern
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.8, 0.4, 0.2, 1.0));

    int group1 = gal->BeginGroup();
    {
        // Draw a simple component symbol
        gal->DrawRectangle(VECTOR2D(-30, -20), VECTOR2D(30, 20));
        gal->SetFillColor(COLOR4D(0.3, 0.3, 0.3, 1.0));
        gal->DrawCircle(VECTOR2D(-15, 0), 8);
        gal->DrawCircle(VECTOR2D(15, 0), 8);
    }
    gal->EndGroup();

    // Create another group with different geometry
    gal->SetFillColor(COLOR4D(0.2, 0.6, 0.8, 1.0));

    int group2 = gal->BeginGroup();
    {
        // Draw a triangle marker
        std::deque<VECTOR2D> tri = {
            VECTOR2D(0, -25),
            VECTOR2D(-20, 15),
            VECTOR2D(20, 15)
        };
        gal->DrawPolygon(tri);
    }
    gal->EndGroup();

    // Create a more complex group
    gal->SetFillColor(COLOR4D(0.6, 0.2, 0.6, 1.0));
    gal->SetIsStroke(true);
    gal->SetIsFill(false);
    gal->SetLineWidth(2.0);
    gal->SetStrokeColor(COLOR4D(0.8, 0.8, 0.2, 1.0));

    int group3 = gal->BeginGroup();
    {
        // Draw a star outline
        for (int i = 0; i < 5; i++) {
            double angle1 = -M_PI / 2 + i * 2 * M_PI / 5;
            double angle2 = -M_PI / 2 + ((i + 2) % 5) * 2 * M_PI / 5;
            gal->DrawLine(
                VECTOR2D(cos(angle1) * 30, sin(angle1) * 30),
                VECTOR2D(cos(angle2) * 30, sin(angle2) * 30)
            );
        }
    }
    gal->EndGroup();

    // Now draw the groups at different positions

    // Draw group1 multiple times
    gal->Save();
    gal->Translate(VECTOR2D(100, 100));
    gal->DrawGroup(group1);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(250, 100));
    gal->DrawGroup(group1);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(400, 100));
    gal->Rotate(45.0 * M_PI / 180.0);
    gal->DrawGroup(group1);
    gal->Restore();

    // Draw group2 multiple times
    gal->Save();
    gal->Translate(VECTOR2D(100, 250));
    gal->DrawGroup(group2);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(200, 250));
    gal->Scale(VECTOR2D(1.5, 1.5));
    gal->DrawGroup(group2);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(320, 250));
    gal->Rotate(180.0 * M_PI / 180.0);
    gal->DrawGroup(group2);
    gal->Restore();

    // Draw group3 multiple times
    gal->Save();
    gal->Translate(VECTOR2D(500, 200));
    gal->DrawGroup(group3);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(600, 200));
    gal->Scale(VECTOR2D(1.3, 1.3));
    gal->DrawGroup(group3);
    gal->Restore();

    gal->Save();
    gal->Translate(VECTOR2D(700, 200));
    gal->Rotate(36.0 * M_PI / 180.0);
    gal->DrawGroup(group3);
    gal->Restore();

    // Test ChangeGroupColor - redraw with different colors
    gal->ChangeGroupColor(group1, COLOR4D(0.2, 0.8, 0.4, 1.0));
    gal->Save();
    gal->Translate(VECTOR2D(100, 400));
    gal->DrawGroup(group1);
    gal->Restore();

    gal->ChangeGroupColor(group1, COLOR4D(0.4, 0.4, 0.9, 1.0));
    gal->Save();
    gal->Translate(VECTOR2D(250, 400));
    gal->DrawGroup(group1);
    gal->Restore();

    // Test ChangeGroupDepth
    gal->SetLayerDepth(50);
    gal->ChangeGroupDepth(group2, 30);
    gal->Save();
    gal->Translate(VECTOR2D(450, 400));
    gal->DrawGroup(group2);
    gal->Restore();

    // Draw a rectangle to show depth ordering
    gal->SetLayerDepth(40);
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(1.0, 0.5, 0.5, 0.7));
    gal->DrawRectangle(VECTOR2D(430, 385), VECTOR2D(470, 430));

    // Labels using simple geometry (no text)
    gal->SetLayerDepth(10);
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(0.5, 0.5, 0.5, 1.0));

    // "Group 1" marker
    gal->DrawRectangle(VECTOR2D(80, 60), VECTOR2D(180, 75));

    // "Group 2" marker
    gal->DrawRectangle(VECTOR2D(80, 210), VECTOR2D(180, 225));

    // "Group 3" marker
    gal->DrawRectangle(VECTOR2D(480, 160), VECTOR2D(580, 175));

    // Grid pattern to show group reuse efficiency
    gal->SetStrokeColor(COLOR4D(0.3, 0.3, 0.3, 0.3));
    for (int x = 0; x < width; x += 50) {
        gal->DrawLine(VECTOR2D(x, 0), VECTOR2D(x, height));
    }
    for (int y = 0; y < height; y += 50) {
        gal->DrawLine(VECTOR2D(0, y), VECTOR2D(width, y));
    }

    // Test DeleteGroup - create a group, draw it, delete it, create another
    gal->SetIsFill(true);
    gal->SetIsStroke(false);
    gal->SetFillColor(COLOR4D(0.9, 0.2, 0.9, 1.0));

    int tempGroup = gal->BeginGroup();
    {
        gal->DrawCircle(VECTOR2D(0, 0), 20);
    }
    gal->EndGroup();

    // Draw the temporary group
    gal->Save();
    gal->Translate(VECTOR2D(600, 400));
    gal->DrawGroup(tempGroup);
    gal->Restore();

    // Delete the temporary group
    gal->DeleteGroup(tempGroup);

    // Create a new group after deletion (reuses ID potentially)
    gal->SetFillColor(COLOR4D(0.2, 0.9, 0.9, 1.0));
    int newGroup = gal->BeginGroup();
    {
        // Draw a diamond shape
        std::deque<VECTOR2D> diamond = {
            VECTOR2D(0, -20),
            VECTOR2D(15, 0),
            VECTOR2D(0, 20),
            VECTOR2D(-15, 0)
        };
        gal->DrawPolygon(diamond);
    }
    gal->EndGroup();

    // Draw the new group
    gal->Save();
    gal->Translate(VECTOR2D(680, 400));
    gal->DrawGroup(newGroup);
    gal->Restore();

    // Test ClearCache - clears all cached groups
    // We'll draw markers showing the groups existed before clear
    gal->SetIsFill(false);
    gal->SetIsStroke(true);
    gal->SetLineWidth(1.0);
    gal->SetStrokeColor(COLOR4D(1.0, 1.0, 1.0, 0.5));
    gal->DrawRectangle(VECTOR2D(580, 380), VECTOR2D(720, 420));

    // Note: ClearCache() invalidates all groups, so we call it at the end
    // In production code, you'd recreate groups after ClearCache
    gal->ClearCache();
}

}  // namespace GALTest
