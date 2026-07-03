/**
 * Tier-1 scenarios 21-28: MODEL_3D (VBO/IBO path), the navigator spheres
 * gizmo, and camera projection/preset-view coverage.
 */

#include "scene3d_test_ctx.h"
#include "test_board_data.h"

#include "3d_rendering/opengl/3d_model.h"
#include "3d_rendering/opengl/3d_spheres_gizmo.h"
#include "3d_rendering/opengl/opengl_utils.h"
#include "common_ogl/ogl_utils.h"

#include <glm/ext.hpp>
#include <memory>

// 21: opaque + per-vertex-color meshes through the VBO/glDrawElements path.
void Scenario_Model3dOpaque( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    MODEL_3D model( TestS3DModel(), MATERIAL_MODE::NORMAL );

    glPushMatrix();
    glScalef( 2.0f, 2.0f, 2.0f );

    MODEL_3D::BeginDrawMulti( true );
    model.DrawOpaque( false );
    MODEL_3D::EndDrawMulti();

    glPopMatrix();
}

// 22: the transparent-model pass — blend + the glTexEnv COMBINE/INTERPOLATE
// block Redraw() sets up around renderTransparentModels
// (render_3d_opengl.cpp:800-831).
void Scenario_Model3dTransparent( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    MODEL_3D model( TestS3DModel(), MATERIAL_MODE::NORMAL );

    glPushMatrix();
    glScalef( 2.0f, 2.0f, 2.0f );

    MODEL_3D::BeginDrawMulti( true );
    model.DrawOpaque( false );

    // State block replicated from Redraw() lines 800-827.
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_TEXTURE_2D );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, aCtx.GetCircleTexture() );

    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
    glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE );
    glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE );
    glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PRIMARY_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
    glTexEnvi( GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_CONSTANT );
    glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA );

    model.DrawTransparent( 0.55f, false );

    glDisable( GL_BLEND );
    OglResetTextureState();
    glDepthMask( GL_TRUE );

    MODEL_3D::EndDrawMulti();
    glPopMatrix();
}

// 23: MATERIAL_MODE branches — NORMAL / DIFFUSE_ONLY / CAD_MODE side by side.
void Scenario_Model3dMaterialModes( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    const MATERIAL_MODE modes[3] = { MATERIAL_MODE::NORMAL, MATERIAL_MODE::DIFFUSE_ONLY,
                                     MATERIAL_MODE::CAD_MODE };

    for( int i = 0; i < 3; i++ )
    {
        MODEL_3D model( TestS3DModel(), modes[i] );

        glPushMatrix();
        glTranslatef( ( i - 1 ) * 5.2f, 0.0f, 0.0f );
        glScalef( 1.1f, 1.1f, 1.1f );

        MODEL_3D::BeginDrawMulti( true );
        model.DrawOpaque( false );
        MODEL_3D::EndDrawMulti();

        glPopMatrix();
    }
}

// 24: model + mesh bounding boxes (glLineWidth>1 GL_LINES — a known WebGL
// port milestone: line width will need quad emulation there).
void Scenario_Model3dBbox( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    MODEL_3D model( TestS3DModel(), MATERIAL_MODE::NORMAL );

    glPushMatrix();
    glScalef( 2.0f, 2.0f, 2.0f );

    MODEL_3D::BeginDrawMulti( true );
    model.DrawOpaque( false );

    // Same state the show_model_bbox path uses inside renderModel(): unlit
    // blended colored lines, drawn between BeginDrawMulti/EndDrawMulti (the
    // bbox VBO draw needs the client vertex-array state enabled there).
    glDisable( GL_LIGHTING );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glColor4f( 0.4f, 1.0f, 0.4f, 0.75f );
    model.DrawBboxes();

    glColor4f( 1.0f, 0.3f, 0.3f, 0.9f );
    model.DrawBbox();

    glDisable( GL_BLEND );
    glEnable( GL_LIGHTING );

    MODEL_3D::EndDrawMulti();
    glPopMatrix();
}

// 25: the navigator spheres gizmo — own corner viewport, gluPerspective,
// gluSphere billboards (RENDER_3D_OPENGL ctor uses SPHERES_GIZMO(4,4)).
void Scenario_SpheresGizmo( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    // Same construction/placement as RENDER_3D_OPENGL (render_3d_opengl.cpp:87,119):
    // corner position (4,4), gizmo square is viewportHeight/8.
    SPHERES_GIZMO gizmo( 4, 4 );
    gizmo.setViewport( 0, 0, aCtx.m_width, aCtx.m_height );

    gizmo.render3dSpheresGizmo( aCtx.m_camera.GetRotationMatrix() );

    glViewport( 0, 0, aCtx.m_width, aCtx.m_height );
}

// Shared asymmetric marker so every camera pose is distinguishable: RGB axis
// triad + an off-axis segment.
static void drawCameraMarker( SCENE3D_CTX& aCtx )
{
    OglSetDiffuseMaterial( SFVEC3F( 0.9f, 0.1f, 0.1f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 3.5f, 0.0f, 0.0f ), 0.4f );

    OglSetDiffuseMaterial( SFVEC3F( 0.1f, 0.9f, 0.1f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, 3.5f, 0.0f ), 0.4f );

    OglSetDiffuseMaterial( SFVEC3F( 0.1f, 0.1f, 0.9f ), 1.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, 0.0f, 3.5f ), 0.4f );

    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );

    const ROUND_SEGMENT_2D segment( SFVEC2F( 1.5f, 1.5f ), SFVEC2F( 4.0f, 4.0f ), 1.0f,
                                    DummyBoardItem() );
    DrawSegment( segment, 24 );
}

// 26: perspective projection.
void Scenario_CameraPersp( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.SetOrtho( false );
    aCtx.BeginFrame();
    aCtx.SetupLights();
    drawCameraMarker( aCtx );
}

// 27: orthographic projection of the identical scene.
void Scenario_CameraOrtho( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.SetOrtho( true );
    aCtx.BeginFrame();
    aCtx.SetupLights();
    drawCameraMarker( aCtx );
}

// 28: the six preset views (ViewCommand_T1 T/B/L/R/F/Back), tiled 3x2.
void Scenario_CameraPresetViews( SCENE3D_CTX& aCtx )
{
    aCtx.ResetCamera();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    const VIEW3D_TYPE views[6] = {
        VIEW3D_TYPE::VIEW3D_TOP,   VIEW3D_TYPE::VIEW3D_BOTTOM, VIEW3D_TYPE::VIEW3D_LEFT,
        VIEW3D_TYPE::VIEW3D_RIGHT, VIEW3D_TYPE::VIEW3D_FRONT,  VIEW3D_TYPE::VIEW3D_BACK,
    };

    const int tileW = aCtx.m_width / 3;
    const int tileH = aCtx.m_height / 2;

    for( int i = 0; i < 6; i++ )
    {
        aCtx.ResetCamera();
        aCtx.SetView( views[i] );

        glViewport( ( i % 3 ) * tileW, ( i / 3 ) * tileH, tileW, tileH );
        glClear( GL_DEPTH_BUFFER_BIT );

        // Re-upload the camera matrices for this tile (same calls Redraw makes).
        glMatrixMode( GL_PROJECTION );
        glLoadMatrixf( glm::value_ptr( aCtx.m_camera.GetProjectionMatrix() ) );
        glMatrixMode( GL_MODELVIEW );
        glLoadMatrixf( glm::value_ptr( aCtx.m_camera.GetViewMatrix() ) );

        aCtx.PositionHeadlight();
        drawCameraMarker( aCtx );
    }

    glViewport( 0, 0, aCtx.m_width, aCtx.m_height );
}
