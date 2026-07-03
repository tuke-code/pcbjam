/**
 * Tier-1 scenarios 1-12: common_ogl/ogl_utils + opengl_utils free functions,
 * FFP materials and lights. Standalone KiCad TUs only — no RENDER_3D_OPENGL
 * members.
 */

#include "scene3d_test_ctx.h"
#include "test_board_data.h"

#include "3d_rendering/opengl/opengl_utils.h"
#include "3d_rendering/raytracing/shapes3D/bbox_3d.h"
#include "common_ogl/ogl_utils.h"

#include <glm/ext.hpp>

// 1: the default viewer background gradient — the simplest possible render
// (no geometry; identity matrices inside OglDrawBackground).
void Scenario_BgGradient( SCENE3D_CTX& aCtx )
{
    aCtx.ResetCamera();
    aCtx.BeginFrame();
}

// 2: translucent background colors exercising the premultiplied-alpha path
// Redraw() feeds through (render_3d_opengl.cpp:576-577).
void Scenario_BgGradientAlpha( SCENE3D_CTX& aCtx )
{
    aCtx.ResetCamera();
    aCtx.BeginFrame( SFVEC4F( 0.9f, 0.3f, 0.1f, 0.5f ), SFVEC4F( 0.1f, 0.3f, 0.9f, 0.8f ) );
}

// 3: DrawBoundingBox — GL_LINE_LOOP/GL_LINE_STRIP wireframe, unlit colored lines.
void Scenario_BoundingBox( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();

    glDisable( GL_LIGHTING );

    glColor4f( 0.9f, 0.9f, 0.2f, 1.0f );
    DrawBoundingBox( BBOX_3D( SFVEC3F( -3.0f, -2.0f, -1.0f ), SFVEC3F( 3.0f, 2.0f, 1.0f ) ) );

    glColor4f( 0.2f, 0.9f, 0.9f, 1.0f );
    DrawBoundingBox( BBOX_3D( SFVEC3F( -1.0f, -1.0f, -2.0f ), SFVEC3F( 1.0f, 1.0f, 2.0f ) ) );
}

// 4: DrawHalfOpenCylinder — TRIANGLE_FAN caps + QUAD_STRIP wall, smooth normals, lit.
void Scenario_HalfOpenCylinder( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.35f, 0.15f ), 1.0f );

    // Unit-sized primitive (d=1, h=1, base at origin) — scale up to fill the
    // frame; GL_NORMALIZE (set in BeginFrame, as in Redraw) fixes the normals.
    // Laid on its side so the curved wall catches the near-vertical
    // directional lights (upright walls are almost unlit under init_lights()).
    glPushMatrix();
    glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );  // axis along +X (screen horizontal)
    glRotatef( -90.0f, 0.0f, 0.0f, 1.0f ); // convex half toward camera+top light
    glScalef( 5.0f, 5.0f, 8.0f );
    glTranslatef( 0.0f, 0.0f, -0.5f );
    DrawHalfOpenCylinder( 32 );
    glPopMatrix();
}

// 5: DrawSegment — one thick rounded-end track segment (quads + half-cylinders
// + matrix stack inside the helper).
void Scenario_SegmentSingle( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );

    const ROUND_SEGMENT_2D segment( SFVEC2F( -4.0f, -2.0f ), SFVEC2F( 4.0f, 2.0f ), 2.0f,
                                    DummyBoardItem() );

    DrawSegment( segment, 32 );
}

// 6: a star of DrawSegment calls with varying widths/angles.
void Scenario_SegmentsStar( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );

    for( int i = 0; i < 12; i++ )
    {
        const float a = 2.0f * glm::pi<float>() * i / 12.0f;
        const float r = 5.5f;
        const float width = 0.25f + 0.09f * i;

        const ROUND_SEGMENT_2D segment( SFVEC2F( 1.2f * std::cos( a ), 1.2f * std::sin( a ) ),
                                        SFVEC2F( r * std::cos( a ), r * std::sin( a ) ), width,
                                        DummyBoardItem() );

        DrawSegment( segment, 24 );
    }
}

// 7: DrawRoundArrow — GLU cylinder + cone + disk + sphere quadrics.
void Scenario_RoundArrow( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    OglSetDiffuseMaterial( SFVEC3F( 0.2f, 0.7f, 0.3f ), 1.0f );
    DrawRoundArrow( SFVEC3F( -2.0f, -2.0f, 0.0f ), SFVEC3F( 3.0f, 2.5f, 1.5f ), 0.5f );
}

// 8: the RGB axis triad the viewer draws — three arrows with per-axis materials.
void Scenario_RoundArrowsAxes( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    // Same layout as RENDER_3D_OPENGL::Redraw()'s show_axis block.
    const float arrow_size = SCENE3D_RANGE_SCALE_3D * 0.30f;

    OglSetDiffuseMaterial( SFVEC3F( 0.9f, 0.0f, 0.0f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( arrow_size, 0.0f, 0.0f ), 0.275f );

    OglSetDiffuseMaterial( SFVEC3F( 0.0f, 0.9f, 0.0f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, arrow_size, 0.0f ), 0.275f );

    OglSetDiffuseMaterial( SFVEC3F( 0.0f, 0.0f, 0.9f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, 0.0f, arrow_size ), 0.275f );
}

// Shared geometry for the material scenarios: cylinder + star spokes.
static void drawMaterialTestGeometry( SCENE3D_CTX& aCtx )
{
    glPushMatrix();
    glScalef( 3.0f, 3.0f, 2.5f );
    DrawHalfOpenCylinder( 32 );
    glPopMatrix();

    for( int i = 0; i < 6; i++ )
    {
        const float a = 2.0f * glm::pi<float>() * i / 6.0f;

        const ROUND_SEGMENT_2D segment( SFVEC2F( 2.2f * std::cos( a ), 2.2f * std::sin( a ) ),
                                        SFVEC2F( 5.5f * std::cos( a ), 5.5f * std::sin( a ) ),
                                        0.8f, DummyBoardItem() );

        DrawSegment( segment, 24 );
    }
}

// 9: full SMATERIAL via OglSetMaterial — ambient/diffuse/specular/shininess (copper-like).
void Scenario_MaterialCopper( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    SMATERIAL copper;
    copper.m_Ambient = SFVEC3F( 0.26f, 0.23f, 0.11f );
    copper.m_Diffuse = SFVEC3F( 0.75f, 0.61f, 0.23f ); // BOARD_ADAPTER default copper
    copper.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    copper.m_Specular = SFVEC3F( 0.70f, 0.55f, 0.35f );
    copper.m_Shininess = 0.4f;
    copper.m_Transparency = 0.0f;

    OglSetMaterial( copper, 1.0f );
    drawMaterialTestGeometry( aCtx );
}

// 10: OglSetDiffuseMaterial — flat matte look, same geometry as #9.
void Scenario_MaterialDiffuseOnly( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );
    drawMaterialTestGeometry( aCtx );
}

// 11: transparent material + blending over opaque geometry.
void Scenario_MaterialTransparent( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    // Opaque base plate of segments.
    OglSetDiffuseMaterial( SFVEC3F( 0.3f, 0.3f, 0.35f ), 1.0f );

    for( int i = -2; i <= 2; i++ )
    {
        const ROUND_SEGMENT_2D segment( SFVEC2F( -5.0f, i * 1.6f ), SFVEC2F( 5.0f, i * 1.6f ),
                                        1.2f, DummyBoardItem() );
        DrawSegment( segment, 24 );
    }

    // Translucent solder-mask-like material on top (same blend state the
    // renderer uses for transparent passes, layer_triangles.cpp setBlendfunction).
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask( GL_FALSE );

    SMATERIAL mask;
    mask.m_Ambient = SFVEC3F( 0.1f, 0.2f, 0.1f );
    mask.m_Diffuse = SFVEC3F( 0.1f, 0.6f, 0.2f );
    mask.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    mask.m_Specular = SFVEC3F( 0.2f, 0.4f, 0.2f );
    mask.m_Shininess = 0.5f;
    mask.m_Transparency = 0.5f; // diffuse alpha = (1-transparency)*opacity

    OglSetMaterial( mask, 1.0f );

    glPushMatrix();
    glTranslatef( 0.0f, 0.0f, 1.0f );
    glScalef( 4.5f, 4.5f, 1.5f );
    DrawHalfOpenCylinder( 32 );
    glPopMatrix();

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
}

// 12-14: GL_LIGHT0/1/2 isolated — one scenario per light (front/headlight
// point light, top directional, bottom directional; the eye-space-anchored
// directions init_lights() bakes at context init). Same sideways cylinder so
// the three renders are directly comparable.
static void drawLightTestGeometry( SCENE3D_CTX& aCtx, bool aFront, bool aTop, bool aBottom )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();
    aCtx.EnableLights( aFront, aTop, aBottom );

    OglSetDiffuseMaterial( SFVEC3F( 0.7f, 0.7f, 0.75f ), 1.0f );

    glPushMatrix();
    glRotatef( 90.0f, 0.0f, 1.0f, 0.0f );
    glRotatef( -90.0f, 0.0f, 0.0f, 1.0f );
    glScalef( 4.5f, 4.5f, 8.0f );
    glTranslatef( 0.0f, 0.0f, -0.5f );
    DrawHalfOpenCylinder( 32 );
    glPopMatrix();

    aCtx.EnableLights( true, true, true );
}

void Scenario_LightFront( SCENE3D_CTX& aCtx )
{
    drawLightTestGeometry( aCtx, true, false, false );
}

void Scenario_LightTop( SCENE3D_CTX& aCtx )
{
    drawLightTestGeometry( aCtx, false, true, false );
}

void Scenario_LightBottom( SCENE3D_CTX& aCtx )
{
    drawLightTestGeometry( aCtx, false, false, true );
}
