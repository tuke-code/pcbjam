/**
 * Tier-1 scenarios 13-20: TRIANGLE_DISPLAY_LIST / OPENGL_RENDER_LIST — display
 * lists, extruded plates, the segment-ends texture + alpha-test path and the
 * stencil hole-subtraction (DrawCulled).
 */

#include "scene3d_test_ctx.h"
#include "test_board_data.h"

#include "common_ogl/ogl_utils.h"

#include <cmath>
#include <memory>

// Fan-triangulate a closed convex contour into a TRIANGLE_LIST at height aZ.
// Data-filling only (mirrors what create_scene.cpp's generators feed the
// containers); the winding follows addTopAndBottomTriangles: top faces CCW in
// XY, bottom faces reversed.
static void addFanTriangles( TRIANGLE_LIST* aDst, const std::vector<SFVEC2F>& aClosedContour,
                             float aZ, bool aTopFace )
{
    const SFVEC2F& v0 = aClosedContour.front();

    for( size_t i = 1; i + 1 < aClosedContour.size(); i++ )
    {
        const SFVEC2F& v1 = aClosedContour[i];
        const SFVEC2F& v2 = aClosedContour[i + 1];

        if( aTopFace )
            aDst->AddTriangle( SFVEC3F( v0.x, v0.y, aZ ), SFVEC3F( v1.x, v1.y, aZ ),
                               SFVEC3F( v2.x, v2.y, aZ ) );
        else
            aDst->AddTriangle( SFVEC3F( v2.x, v2.y, aZ ), SFVEC3F( v1.x, v1.y, aZ ),
                               SFVEC3F( v0.x, v0.y, aZ ) );
    }
}


// An extruded plate: top + bottom fans and real AddToMiddleContours walls.
static std::unique_ptr<TRIANGLE_DISPLAY_LIST> makePlateTdl( const std::vector<SFVEC2F>& aContour,
                                                            float aZBot, float aZTop )
{
    auto tdl = std::make_unique<TRIANGLE_DISPLAY_LIST>( 2 * aContour.size() );

    addFanTriangles( tdl->m_layer_top_triangles, aContour, aZTop, true );
    addFanTriangles( tdl->m_layer_bot_triangles, aContour, aZBot, false );

    // Our contours are CCW in 3D space; board outlines arrive effectively CW
    // (the BIU->3D conversion mirrors Y), so invert to get outward-facing walls.
    tdl->AddToMiddleContours( aContour, aZBot, aZTop, true );

    return tdl;
}


static std::unique_ptr<OPENGL_RENDER_LIST> makeHexPlateList( SCENE3D_CTX& aCtx, float aZBot,
                                                             float aZTop )
{
    auto tdl = makePlateTdl( MakeCircleContour( 4.5f, 6 ), aZBot, aZTop );
    return std::unique_ptr<OPENGL_RENDER_LIST>( aCtx.MakeRenderList( *tdl, aZBot, aZTop ) );
}


static void beginTdlScene( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();
    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );
}

// 13: only the top-face display list.
void Scenario_TdlDrawTop( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );
    makeHexPlateList( aCtx, -0.6f, 0.6f )->DrawTop();
}

// 14: only the bottom-face display list, seen from below.
void Scenario_TdlDrawBot( SCENE3D_CTX& aCtx )
{
    aCtx.ResetCamera();
    aCtx.SetView( VIEW3D_TYPE::VIEW3D_BOTTOM );
    aCtx.BeginFrame();
    aCtx.SetupLights();
    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );

    makeHexPlateList( aCtx, -0.6f, 0.6f )->DrawBot();
}

// 15: only the extruded side walls (middle contour quads, per-vertex normals).
void Scenario_TdlDrawMiddle( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );
    makeHexPlateList( aCtx, -1.2f, 1.2f )->DrawMiddle();
}

// 16: the closed extruded plate — all five sub-lists.
void Scenario_TdlDrawAll( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );
    makeHexPlateList( aCtx, -0.9f, 0.9f )->DrawAll();
}

// 17: the segment-ends path — circle texture + glAlphaFunc(GL_GREATER,0.2)
// inside generate_top_or_bot_seg_ends (layer_triangles.cpp:600-624). The
// triangle pattern mirrors addObjectTriangles(FILLED_CIRCLE_2D)
// (create_scene.cpp:42-70): two triangles per circle whose UVs map the
// blurred-circle texture into a round disc.
void Scenario_TdlSegEndsTexture( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );

    TRIANGLE_DISPLAY_LIST tdl( 8 );

    const float texture_factor = ( 8.0f / 1024.0f ) + 1.0f; // SIZE_OF_CIRCLE_TEXTURE

    const SFVEC2F centers[3] = { { -3.5f, 0.0f }, { 0.0f, 0.0f }, { 3.5f, 0.0f } };
    const float   radii[3] = { 1.2f, 1.7f, 2.2f };

    for( int i = 0; i < 3; i++ )
    {
        const SFVEC2F& center = centers[i];
        const float    radius = radii[i] * 2.0f; // doubled like the generator
        const float    f = ( std::sqrt( 2.0f ) / 2.0f ) * radius * texture_factor;
        const float    z = 0.4f;

        tdl.m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( center.x + f, center.y, z ), SFVEC3F( center.x - f, center.y, z ),
                SFVEC3F( center.x, center.y - f, z ) );
        tdl.m_layer_top_segment_ends->AddTriangle(
                SFVEC3F( center.x - f, center.y, z ), SFVEC3F( center.x + f, center.y, z ),
                SFVEC3F( center.x, center.y + f, z ) );
    }

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, 0.0f, 0.4f ) );
    list->DrawTop();
}

// 18: DrawCulled — the stencil-based hole subtraction (layer_triangles.cpp:459-543).
// A plate with two hole volumes stenciled out of it.
void Scenario_TdlCulledStencil( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );

    const float zBot = -0.5f, zTop = 0.5f;

    auto plateTdl = makePlateTdl( MakeCircleContour( 4.5f, 6 ), zBot, zTop );
    std::unique_ptr<OPENGL_RENDER_LIST> plate( aCtx.MakeRenderList( *plateTdl, zBot, zTop ) );

    // Hole volumes: same z-range plates (a round one and a square one), like
    // the outer-through-holes subtract lists Redraw passes to DrawCulled.
    auto holesTdl = makePlateTdl( MakeCircleContour( 1.1f, 16, -1.8f, 0.0f ), zBot, zTop );
    auto holes2Tdl = makePlateTdl( MakeSquareContour( 0.9f, 1.8f, 0.9f ), zBot, zTop );

    std::unique_ptr<OPENGL_RENDER_LIST> holes( aCtx.MakeRenderList( *holesTdl, zBot, zTop ) );
    std::unique_ptr<OPENGL_RENDER_LIST> holes2( aCtx.MakeRenderList( *holes2Tdl, zBot, zTop ) );

    plate->DrawCulled( true, holes.get(), holes2.get() );
}

// 19: ApplyScalePosition — the z-translate/z-scale transform used to place
// layer plates at their board Z (layer_triangles.cpp beginTransformation).
void Scenario_TdlZScale( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );

    auto tdl = makePlateTdl( MakeCircleContour( 3.5f, 6 ), 0.0f, 1.0f );
    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( *tdl, 0.0f, 1.0f ) );

    // Thin plate below...
    list->ApplyScalePosition( -1.6f, 0.25f );
    list->DrawAll();

    // ...thick plate above.
    OglSetDiffuseMaterial( SFVEC3F( 0.2f, 0.5f, 0.8f ), 1.0f );
    list->ApplyScalePosition( 0.6f, 1.8f );
    list->DrawAll();
}

// 20: SetItIsTransparent + blended DrawAll over an opaque plate.
void Scenario_TdlTransparent( SCENE3D_CTX& aCtx )
{
    beginTdlScene( aCtx );

    auto baseTdl = makePlateTdl( MakeSquareContour( 3.0f ), -1.0f, -0.4f );
    std::unique_ptr<OPENGL_RENDER_LIST> base( aCtx.MakeRenderList( *baseTdl, -1.0f, -0.4f ) );
    base->DrawAll();

    // Epoxy-like translucent plate above it (renderBoardBody pattern:
    // material transparency + SetItIsTransparent, render_3d_opengl.cpp:468-501).
    SMATERIAL epoxy;
    epoxy.m_Ambient = SFVEC3F( 0.1f, 0.1f, 0.12f );
    epoxy.m_Diffuse = SFVEC3F( 0.4f, 0.4f, 0.5f ); // BOARD_ADAPTER default board body
    epoxy.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    epoxy.m_Specular = SFVEC3F( 0.2f, 0.2f, 0.2f );
    epoxy.m_Shininess = 0.3f;
    epoxy.m_Transparency = 0.1f; // == 1 - default body alpha 0.9

    OglSetMaterial( epoxy, 0.6f );

    auto topTdl = makePlateTdl( MakeCircleContour( 4.2f, 6 ), 0.0f, 0.8f );
    std::unique_ptr<OPENGL_RENDER_LIST> top( aCtx.MakeRenderList( *topTdl, 0.0f, 0.8f ) );

    top->SetItIsTransparent( true );
    top->DrawAll();
}
