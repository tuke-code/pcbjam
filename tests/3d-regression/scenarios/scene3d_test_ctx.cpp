#include "scene3d_test_ctx.h"

#include "3d_math.h" // SphericalToCartesian (inline, 3d-viewer/3d_math.h)
#include "3d_rendering/image.h" // IMAGE for the circle texture
#include "common_ogl/ogl_utils.h" // OglResetTextureState, OglDrawBackground, OglLoadTexture

#include <glm/ext.hpp> // value_ptr

// SIZE_OF_CIRCLE_TEXTURE lives in render_3d_opengl.h (too heavy for Stage 1);
// keep the value in sync (render_3d_opengl.h:53).
static constexpr int CIRCLE_TEXTURE_SIZE = 1024;

// Same premultiply the renderer applies before OglDrawBackground
// (render_3d_opengl.cpp:505-508).
static inline SFVEC4F premultiplyAlpha( const SFVEC4F& aInput )
{
    return SFVEC4F( aInput.r * aInput.a, aInput.g * aInput.a, aInput.b * aInput.a, aInput.a );
}


SCENE3D_CTX::SCENE3D_CTX( int aWidth, int aHeight ) :
        m_width( aWidth ),
        m_height( aHeight ),
        m_camera( 2.0f * SCENE3D_RANGE_SCALE_3D )
{
    ResetCamera();
}


void SCENE3D_CTX::ResetCamera()
{
    m_camera.SetProjection( PROJECTION_TYPE::PERSPECTIVE );
    m_camera.SetCurWindowSize( wxSize( m_width, m_height ) );
    m_camera.Reset();
}


void SCENE3D_CTX::SetView( VIEW3D_TYPE aView )
{
    // The settled end state of EDA_3D_CANVAS::SetView3D's animation
    // (eda_3d_canvas.cpp:1455-1481 + the Interpolate(1.0f) at :1354).
    m_camera.SetT0_and_T1_current_T();
    m_camera.ViewCommand_T1( aView );
    m_camera.SetInterpolateMode( CAMERA_INTERPOLATION::LINEAR );
    m_camera.Interpolate( 1.0f );
}


void SCENE3D_CTX::SetIsoView()
{
    ResetCamera();
    // Tilt board toward the viewer, then spin — a deterministic 3/4 view that
    // shows top faces, side walls and lighting gradients at once.
    m_camera.RotateX( -glm::pi<float>() / 3.0f ); // -60°
    m_camera.RotateZ( glm::pi<float>() / 6.0f );  // +30°
}


void SCENE3D_CTX::SetOrtho( bool aOrtho )
{
    m_camera.SetProjection( aOrtho ? PROJECTION_TYPE::ORTHO : PROJECTION_TYPE::PERSPECTIVE );
    // Force a projection rebuild for the (possibly unchanged) window size:
    // SetCurWindowSize only rebuilds on size change, so nudge through Reset-safe API.
    m_camera.SetCurWindowSize( wxSize( m_width, m_height - 1 ) );
    m_camera.SetCurWindowSize( wxSize( m_width, m_height ) );
}


void SCENE3D_CTX::BeginFrame()
{
    // Default viewer background (BOARD_ADAPTER ctor, board_adapter.cpp:132-133).
    BeginFrame( SFVEC4F( 0.8f, 0.8f, 0.9f, 1.0f ), SFVEC4F( 0.4f, 0.4f, 0.5f, 1.0f ) );
}


void SCENE3D_CTX::BeginFrame( const SFVEC4F& aBgTop, const SFVEC4F& aBgBot )
{
    // Per-frame state block replicated verbatim from RENDER_3D_OPENGL::Redraw()
    // (render_3d_opengl.cpp:553-586). Kept in the same order so the state the
    // scenarios render under is auditable against the real renderer.
    glDepthFunc( GL_LESS );
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CCW );
    glEnable( GL_NORMALIZE );
    glViewport( 0, 0, m_width, m_height );
    glEnable( GL_MULTISAMPLE );

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth( 1.0f );
    glClearStencil( 0x00 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    OglResetTextureState();

    OglDrawBackground( premultiplyAlpha( aBgTop ), premultiplyAlpha( aBgBot ) );

    glEnable( GL_DEPTH_TEST );

    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_camera.GetProjectionMatrix() ) );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glLoadMatrixf( glm::value_ptr( m_camera.GetViewMatrix() ) );
}


void SCENE3D_CTX::SetupLights()
{
    // Per-frame part only: Redraw() enables the lights and repositions the
    // headlight after loading the camera matrices (render_3d_opengl.cpp:589-611).
    // The light PARAMETERS were set once by initLights() at init time — GL bakes
    // directional-light positions in eye space using the modelview current at
    // the glLightfv call, and the real renderer sets them under the identity
    // matrix of a fresh context (initializeOpenGL -> init_lights).
    EnableLights( true, true, true );
    glEnable( GL_LIGHTING );

    PositionHeadlight();
}


// The real light-rig initializer from render_3d_opengl.cpp:401 (free function
// with external linkage; linked since Stage 2).
void init_lights();

void SCENE3D_CTX::initLights()
{
    ::init_lights();
}


void SCENE3D_CTX::EnableLights( bool aFront, bool aTop, bool aBottom )
{
    // Same GL_LIGHTx mapping as RENDER_3D_OPENGL::setLightFront/Top/Bottom
    // (render_3d_opengl.cpp:128-148).
    if( aFront )
        glEnable( GL_LIGHT0 );
    else
        glDisable( GL_LIGHT0 );

    if( aTop )
        glEnable( GL_LIGHT1 );
    else
        glDisable( GL_LIGHT1 );

    if( aBottom )
        glEnable( GL_LIGHT2 );
    else
        glDisable( GL_LIGHT2 );
}


void SCENE3D_CTX::PositionHeadlight()
{
    // Exact headlight placement from Redraw() (render_3d_opengl.cpp:595-611).
    const SFVEC3F& cameraPos = m_camera.GetPos();

    float zpos;

    if( cameraPos.z > 0.0f )
        zpos = glm::max( cameraPos.z, 0.5f ) + cameraPos.z * cameraPos.z;
    else
        zpos = glm::min( cameraPos.z, -0.5f ) - cameraPos.z * cameraPos.z;

    const GLfloat headlight_pos[] = { cameraPos.x, cameraPos.y, zpos, 1.0f };

    glLightfv( GL_LIGHT0, GL_POSITION, headlight_pos );
}


void SCENE3D_CTX::InitOnce()
{
    if( m_circleTexture )
        return;

    // Replicates RENDER_3D_OPENGL::initializeOpenGL() (render_3d_opengl.cpp:858-896)
    // minus init_lights() (SetupLights) and m_canvasInitialized bookkeeping.
    glEnable( GL_LINE_SMOOTH );
    glShadeModel( GL_SMOOTH );
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

    IMAGE circleImage( CIRCLE_TEXTURE_SIZE, CIRCLE_TEXTURE_SIZE );

    const unsigned int circleRadius = ( CIRCLE_TEXTURE_SIZE / 2 ) - 4;

    circleImage.CircleFilled( ( CIRCLE_TEXTURE_SIZE / 2 ) - 0, ( CIRCLE_TEXTURE_SIZE / 2 ) - 0,
                              circleRadius, 0xFF );

    IMAGE circleImageBlured( circleImage.GetWidth(), circleImage.GetHeight() );

    circleImageBlured.EfxFilter_SkipCenter( &circleImage, IMAGE_FILTER::GAUSSIAN_BLUR,
                                            circleRadius - 8 );

    m_circleTexture = OglLoadTexture( circleImageBlured );

    // initializeOpenGL() ends with init_lights(): the directional lights are
    // baked in EYE space under the fresh context's identity modelview — that is
    // why the viewer's scene lighting follows the camera. Keep that semantic.
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    initLights();
}


OPENGL_RENDER_LIST* SCENE3D_CTX::MakeRenderList( const TRIANGLE_DISPLAY_LIST& aTdl, float aZBot,
                                                 float aZTop ) const
{
    return new OPENGL_RENDER_LIST( aTdl, m_circleTexture, aZBot, aZTop );
}
