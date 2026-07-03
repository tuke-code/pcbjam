/**
 * SCENE3D_CTX — the scenario seam for the 3D-renderer regression suite.
 *
 * The ctx provides frame plumbing only: viewport, camera matrices, buffer
 * clears and light setup, replicating RENDER_3D_OPENGL::Redraw()'s per-frame
 * state block (render_3d_opengl.cpp:553-611) so that primitive scenarios render
 * under the same GL state as the real viewer. Everything that produces pixels
 * (geometry, materials, display lists, textures) must be real KiCad code
 * called from the scenario bodies.
 *
 * This header compiles for the native macOS build AND (later) under
 * emscripten — no native-only includes.
 */

#ifndef SCENE3D_TEST_CTX_H
#define SCENE3D_TEST_CTX_H

#include <kicad_gl/kiglu.h> // GL + GLU, platform-routed (glad native / GLES shim on wasm)

#include <gal/3d/camera.h>
#include "3d_rendering/track_ball.h"
#include "3d_rendering/opengl/layer_triangles.h"

// == RANGE_SCALE_3D from 3d-viewer/3d_canvas/board_adapter.h (that header is too
// heavy to pull into every scenario TU). EDA_3D_VIEWER_FRAME constructs its
// TRACK_BALL with 2 * RANGE_SCALE_3D (eda_3d_viewer_frame.cpp).
static constexpr float SCENE3D_RANGE_SCALE_3D = 8.0f;

struct SCENE3D_CTX
{
    int        m_width;
    int        m_height;
    TRACK_BALL m_camera;

    SCENE3D_CTX( int aWidth, int aHeight );

    // ---- Camera helpers (all real CAMERA / TRACK_BALL API) ----

    /// Perspective projection, window size set, Reset() — the straight top-down default.
    void ResetCamera();

    /// Apply a preset view the way EDA_3D_CANVAS does, but settled instantly:
    /// SetT0_and_T1_current_T() -> ViewCommand_T1(aView) -> Interpolate(1.0f).
    void SetView( VIEW3D_TYPE aView );

    /// Deterministic 3/4 view for lit-geometry scenarios: Reset + RotateX/RotateZ.
    void SetIsoView();

    void SetOrtho( bool aOrtho );

    // ---- Frame plumbing (mirrors RENDER_3D_OPENGL::Redraw() lines 553-611) ----

    /// Clears + background gradient + camera matrix upload. Default viewer
    /// background colors (BOARD_ADAPTER ctor: top 0.8,0.8,0.9 / bot 0.4,0.4,0.5).
    void BeginFrame();
    void BeginFrame( const SFVEC4F& aBgTop, const SFVEC4F& aBgBot );

    // ---- Lights ----

    /// Per-frame light enable + headlight placement, as Redraw() does after
    /// loading the camera matrices (render_3d_opengl.cpp:589-611). The light
    /// parameters themselves were baked at init time (InitOnce -> initLights).
    void SetupLights();

    /// glEnable/glDisable GL_LIGHT0 (headlight/front), GL_LIGHT1 (top), GL_LIGHT2 (bottom)
    /// — same mapping as RENDER_3D_OPENGL::setLightFront/Top/Bottom.
    void EnableLights( bool aFront, bool aTop, bool aBottom );

    /// Exact headlight placement formula from Redraw() (render_3d_opengl.cpp:595-611).
    void PositionHeadlight();

    // ---- Renderer-init state + circle texture ----

    /// One-time GL state + the segment-ends circle texture, replicating the
    /// non-member-state part of RENDER_3D_OPENGL::initializeOpenGL()
    /// (render_3d_opengl.cpp:858-896): GL_LINE_SMOOTH, glShadeModel(GL_SMOOTH),
    /// GL_UNPACK_ALIGNMENT=4, then the real IMAGE::CircleFilled +
    /// EfxFilter_SkipCenter(GAUSSIAN_BLUR) + OglLoadTexture recipe.
    void InitOnce();

    GLuint GetCircleTexture() const { return m_circleTexture; }

    /// Wrap a filled TRIANGLE_DISPLAY_LIST into GL display lists — thin sugar
    /// over the real OPENGL_RENDER_LIST ctor with the ctx circle texture.
    OPENGL_RENDER_LIST* MakeRenderList( const TRIANGLE_DISPLAY_LIST& aTdl, float aZBot,
                                        float aZTop ) const;

private:
    /// Stage-1 verbatim copy of ::init_lights() (render_3d_opengl.cpp:401-445);
    /// becomes a call to the real free function once Stage 2 links it.
    void initLights();

    GLuint m_circleTexture = 0;
};

#endif // SCENE3D_TEST_CTX_H
