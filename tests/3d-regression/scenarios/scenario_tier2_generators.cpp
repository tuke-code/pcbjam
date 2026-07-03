/**
 * Tier-2 scenarios: RENDER_3D_OPENGL's private geometry generators, grids and
 * material setters, reached through the rob-template accessor
 * (native/render3d_test_accessor.h) over a synthetic BOARD_ADAPTER
 * (native/board_adapter_test_impl.cpp InitSettings).
 */

#include "scene3d_test_ctx.h"
#include "scene3d_test_rig.h"
#include "test_board_data.h"

#include "3d_rendering/opengl/opengl_utils.h"
#include "3d_rendering/raytracing/shapes2D/4pt_polygon_2d.h"
#include "3d_rendering/raytracing/shapes2D/filled_circle_2d.h"
#include "3d_rendering/raytracing/shapes2D/ring_2d.h"
#include "3d_rendering/raytracing/shapes2D/round_segment_2d.h"
#include "3d_rendering/raytracing/shapes2D/triangle_2d.h"
#include "common_ogl/ogl_utils.h"

#include <base_units.h> // pcbIUScale
#include <glm/ext.hpp>
#include <memory>

using TIER2_RIG = SCENE3D_TEST_RIG;


static void beginTier2Scene( SCENE3D_CTX& aCtx )
{
    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();
    OglSetDiffuseMaterial( SFVEC3F( 0.75f, 0.61f, 0.23f ), 1.0f );
}

// 31: generateCylinder — the via/pad barrel wall generator.
void Scenario_GenCylinder( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    TRIANGLE_DISPLAY_LIST tdl( 256 );
    R3D_GenerateCylinder( *rig, SFVEC2F( 0.0f, 0.0f ), 2.2f, 3.0f, 1.8f, -1.8f, 32, &tdl );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, -1.8f, 1.8f ) );
    list->DrawAll();
}

// 32: generateInvCone — the countersink cone generator.
void Scenario_GenInvCone( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    TRIANGLE_DISPLAY_LIST tdl( 256 );
    R3D_GenerateInvCone( *rig, SFVEC2F( 0.0f, 0.0f ), 1.2f, 3.2f, 1.5f, -1.5f, 32, &tdl,
                         EDA_ANGLE( 90.0, DEGREES_T ) );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, -1.5f, 1.5f ) );
    list->DrawAll();
}

// 33: generateDisk — hole caps / annulus disks, top and bottom variants.
void Scenario_GenDisk( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    TRIANGLE_DISPLAY_LIST tdl( 256 );
    R3D_GenerateDisk( *rig, SFVEC2F( -2.6f, 0.0f ), 2.0f, 0.8f, 32, &tdl, true );
    R3D_GenerateDisk( *rig, SFVEC2F( 2.6f, 0.0f ), 2.0f, -0.8f, 32, &tdl, false );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, -0.8f, 0.8f ) );
    list->DrawAll();
}

// 34: generateDimple — the plated-hole cover bump.
void Scenario_GenDimple( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    TRIANGLE_DISPLAY_LIST tdl( 1024 );
    R3D_GenerateDimple( *rig, SFVEC2F( 0.0f, 0.0f ), 3.0f, 0.0f, 1.2f, 48, &tdl, true );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, 0.0f, 1.2f ) );
    list->DrawAll();
}

// 35: all five addObjectTriangles overloads in a row.
void Scenario_AddObjAllShapes( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    const float zTop = 0.5f, zBot = -0.5f;
    TRIANGLE_DISPLAY_LIST tdl( 1024 );

    const FILLED_CIRCLE_2D circle( SFVEC2F( -5.4f, 0.0f ), 1.1f, DummyBoardItem() );
    R3D_AddObjTriangles( *rig, &circle, &tdl, zTop, zBot );

    const RING_2D ring( SFVEC2F( -2.7f, 0.0f ), 0.6f, 1.2f, DummyBoardItem() );
    R3D_AddObjTriangles( *rig, &ring, &tdl, zTop, zBot );

    const POLYGON_4PT_2D poly( SFVEC2F( -1.0f, -1.0f ), SFVEC2F( 1.0f, -1.1f ),
                               SFVEC2F( 1.1f, 1.0f ), SFVEC2F( -0.9f, 1.1f ), DummyBoardItem() );
    R3D_AddObjTriangles( *rig, &poly, &tdl, zTop, zBot );

    const TRIANGLE_2D tri( SFVEC2F( 1.8f, -1.1f ), SFVEC2F( 3.6f, -1.1f ), SFVEC2F( 2.7f, 1.2f ),
                           DummyBoardItem() );
    R3D_AddObjTriangles( *rig, &tri, &tdl, zTop, zBot );

    const ROUND_SEGMENT_2D seg( SFVEC2F( 4.4f, -1.0f ), SFVEC2F( 6.0f, 1.0f ), 0.9f,
                                DummyBoardItem() );
    R3D_AddObjTriangles( *rig, &seg, &tdl, zTop, zBot );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, zBot, zTop ) );
    list->DrawAll();
}

// 36: appendPostMachiningGeometry — counterbore and countersink profiles.
void Scenario_PostMachining( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    const float unitScale = static_cast<float>( rig.m_adapter.BiuTo3dUnits() );
    float zEnd = 0.0f;

    // Separate lists on purpose. UPSTREAM BUG (create_scene.cpp countersink
    // path): the cone quads are added with AddQuad but no AddNormal, so the
    // middle-quads normals array is half the vertex count and
    // generate_middle_triangles rejects the WHOLE list — a countersink batched
    // with other geometry kills that list's walls in the real viewer too.
    // Keeping them separate makes the counterbore render correctly while the
    // countersink half documents the buggy (empty) upstream output.
    TRIANGLE_DISPLAY_LIST cbTdl( 4096 );
    const bool cb = R3D_AppendPostMachining( *rig, &cbTdl, SFVEC2F( -3.2f, 0.0f ),
                                             PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE,
                                             pcbIUScale.mmToIU( 14 ), pcbIUScale.mmToIU( 6 ),
                                             1.0f, 1.0f, true, 0.4f, unitScale, &zEnd );

    TRIANGLE_DISPLAY_LIST csTdl( 4096 );
    const bool cs = R3D_AppendPostMachining( *rig, &csTdl, SFVEC2F( 3.2f, 0.0f ),
                                             PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK,
                                             pcbIUScale.mmToIU( 14 ), pcbIUScale.mmToIU( 6 ),
                                             1.0f, 1.0f, true, 0.4f, unitScale, &zEnd );

    wxASSERT( cb && cs );
    (void) cb;
    (void) cs;

    std::unique_ptr<OPENGL_RENDER_LIST> cbList( aCtx.MakeRenderList( cbTdl, -1.0f, 1.0f ) );
    cbList->DrawAll();

    std::unique_ptr<OPENGL_RENDER_LIST> csList( aCtx.MakeRenderList( csTdl, -1.0f, 1.0f ) );
    csList->DrawAll();
}

// 37: a complete via cross-section composed the way generateViaBarrels /
// generateViaCovers do: barrel + annular disks + cover dimple.
void Scenario_ViaComposite( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    R3D_SetupMaterials( *rig );
    R3D_SetLayerMaterial( *rig, F_Cu );

    TRIANGLE_DISPLAY_LIST tdl( 2048 );

    R3D_GenerateCylinder( *rig, SFVEC2F( 0.0f, 0.0f ), 1.6f, 2.0f, 1.4f, -1.4f, 32, &tdl );
    R3D_GenerateDisk( *rig, SFVEC2F( 0.0f, 0.0f ), 2.6f, 1.4f, 32, &tdl, true );
    R3D_GenerateDisk( *rig, SFVEC2F( 0.0f, 0.0f ), 2.6f, -1.4f, 32, &tdl, false );
    R3D_GenerateDimple( *rig, SFVEC2F( 0.0f, 0.0f ), 1.6f, 1.4f, 0.5f, 32, &tdl, true );

    std::unique_ptr<OPENGL_RENDER_LIST> list( aCtx.MakeRenderList( tdl, -1.4f, 1.4f ) );
    list->DrawAll();
}

// 38-41: generate3dGrid at each density — display list of blended GL_LINES
// sized from the synthetic board adapter, drawn like Redraw()'s grid block.
static void renderGridScenario( SCENE3D_CTX& aCtx, GRID3D_TYPE aType )
{
    TIER2_RIG rig( aCtx );

    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    R3D_Generate3dGrid( *rig, aType );

    glDisable( GL_LIGHTING );

    const GLuint grid = R3D_GetGridList( *rig );

    if( glIsList( grid ) )
        glCallList( grid );

    glEnable( GL_LIGHTING );
}

void Scenario_Grid1mm( SCENE3D_CTX& aCtx )
{
    renderGridScenario( aCtx, GRID3D_TYPE::GRID_1MM );
}

void Scenario_Grid2p5mm( SCENE3D_CTX& aCtx )
{
    renderGridScenario( aCtx, GRID3D_TYPE::GRID_2P5MM );
}

void Scenario_Grid5mm( SCENE3D_CTX& aCtx )
{
    renderGridScenario( aCtx, GRID3D_TYPE::GRID_5MM );
}

void Scenario_Grid10mm( SCENE3D_CTX& aCtx )
{
    renderGridScenario( aCtx, GRID3D_TYPE::GRID_10MM );
}

// Local extruded-plate builder (same construction as the tier-1 TDL scenarios).
static void addFan( TRIANGLE_LIST* aDst, const std::vector<SFVEC2F>& aContour, float aZ,
                    bool aTop )
{
    const SFVEC2F& v0 = aContour.front();

    for( size_t i = 1; i + 1 < aContour.size(); i++ )
    {
        const SFVEC2F& v1 = aContour[i];
        const SFVEC2F& v2 = aContour[i + 1];

        if( aTop )
            aDst->AddTriangle( SFVEC3F( v0.x, v0.y, aZ ), SFVEC3F( v1.x, v1.y, aZ ),
                               SFVEC3F( v2.x, v2.y, aZ ) );
        else
            aDst->AddTriangle( SFVEC3F( v2.x, v2.y, aZ ), SFVEC3F( v1.x, v1.y, aZ ),
                               SFVEC3F( v0.x, v0.y, aZ ) );
    }
}

static std::unique_ptr<OPENGL_RENDER_LIST> makePlate( SCENE3D_CTX& aCtx, float aHalf, float aZBot,
                                                      float aZTop )
{
    const std::vector<SFVEC2F> contour = MakeSquareContour( aHalf );

    TRIANGLE_DISPLAY_LIST tdl( 64 );
    addFan( tdl.m_layer_top_triangles, contour, aZTop, true );
    addFan( tdl.m_layer_bot_triangles, contour, aZBot, false );
    tdl.AddToMiddleContours( contour, aZBot, aZTop, true );

    return std::unique_ptr<OPENGL_RENDER_LIST>( aCtx.MakeRenderList( tdl, aZBot, aZTop ) );
}

// 42: setupMaterials + setLayerMaterial — material swatch plates for the
// technical layers (silk, mask, paste, copper).
void Scenario_LayerMaterials( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );

    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    R3D_SetupMaterials( *rig );

    const PCB_LAYER_ID layers[4] = { F_SilkS, F_Mask, F_Paste, B_Cu };

    // Mask is translucent — same blend state the transparent passes use.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for( int i = 0; i < 4; i++ )
    {
        R3D_SetLayerMaterial( *rig, layers[i] );

        glPushMatrix();
        glTranslatef( ( i - 1.5f ) * 3.4f, 0.0f, 0.0f );

        makePlate( aCtx, 1.5f, -0.4f, 0.4f )->DrawAll();

        glPopMatrix();
    }

    glDisable( GL_BLEND );
}

// 43: setArrowMaterial + glColor axes — the Redraw() show_axis block.
void Scenario_ArrowMaterial( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );

    aCtx.SetIsoView();
    aCtx.BeginFrame();
    aCtx.SetupLights();

    R3D_SetArrowMaterial( *rig );

    const float arrow_size = SCENE3D_RANGE_SCALE_3D * 0.30f;

    glColor3f( 0.9f, 0.0f, 0.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( arrow_size, 0.0f, 0.0f ), 0.275f );

    glColor3f( 0.0f, 0.9f, 0.0f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, arrow_size, 0.0f ), 0.275f );

    glColor3f( 0.0f, 0.0f, 0.9f );
    DrawRoundArrow( SFVEC3F( 0.0f ), SFVEC3F( 0.0f, 0.0f, arrow_size ), 0.275f );
}

// 44: createBoard — the real board-outline-to-display-list path (SHAPE_POLY_SET
// triangulation + middle contours), drawn like renderBoardBody().
void Scenario_CreateBoard( SCENE3D_CTX& aCtx )
{
    TIER2_RIG rig( aCtx );
    beginTier2Scene( aCtx );

    std::unique_ptr<OPENGL_RENDER_LIST> board(
            R3D_CreateBoard( *rig, rig.m_adapter.GetBoardPoly(), &rig.m_adapter.GetTH_ODs() ) );

    // renderBoardBody() material + transform (render_3d_opengl.cpp:468-501).
    SMATERIAL epoxy;
    epoxy.m_Ambient = SFVEC3F( 0.1f, 0.1f, 0.12f );
    epoxy.m_Diffuse = SFVEC3F( 0.4f, 0.4f, 0.5f );
    epoxy.m_Emissive = SFVEC3F( 0.0f, 0.0f, 0.0f );
    epoxy.m_Specular = SFVEC3F( 0.2f, 0.2f, 0.2f );
    epoxy.m_Shininess = 0.3f;
    epoxy.m_Transparency = 0.1f;

    OglSetMaterial( epoxy, 1.0f );

    board->ApplyScalePosition( -rig.m_adapter.GetBoardBodyThickness() / 2.0f,
                               rig.m_adapter.GetBoardBodyThickness() );
    board->SetItIsTransparent( true );
    board->DrawAll();
}
