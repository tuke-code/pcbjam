/**
 * Test-harness definitions of BOARD_ADAPTER's out-of-line members.
 *
 * The real board_adapter.cpp / create_layer_items.cpp drag the whole pcbnew
 * board model + settings machinery in — none of which exists in this harness.
 * Instead WE define the declared members: an out-of-line definition of a
 * declared member function has full private access, so this TU is both the
 * stub layer and the synthetic-board-data injection seam (InitSettings).
 *
 * Bodies marked "verbatim" are copied from board_adapter.cpp and must behave
 * identically; bodies marked "test" are simplified board-less variants (any
 * behavioral drift shows up as a baseline change and is reviewed there).
 */

#include "kicad_stubs_3d.h"

#include "3d_canvas/board_adapter.h"
#include "3d_rendering/raytracing/shapes2D/filled_circle_2d.h"
#include "3d_rendering/raytracing/shapes2D/round_segment_2d.h"
#include "3d_viewer/eda_3d_viewer_settings.h"

#include <board_design_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h> // GetArcToSegmentCount

// Same values as board_adapter.cpp:51-57 (defined there as TU-local macros).
#define DEFAULT_BOARD_THICKNESS pcbIUScale.mmToIU( 1.6 )
#define DEFAULT_COPPER_THICKNESS pcbIUScale.mmToIU( 0.035 )
#define DEFAULT_TECH_LAYER_THICKNESS pcbIUScale.mmToIU( 0.025 )
#define SOLDERPASTE_LAYER_THICKNESS pcbIUScale.mmToIU( 0.04 )

#include "../scenarios/test_board_data.h"

// Statics (board_adapter.cpp:60-89, verbatim).
CUSTOM_COLORS_LIST BOARD_ADAPTER::g_SilkColors;
CUSTOM_COLORS_LIST BOARD_ADAPTER::g_MaskColors;
CUSTOM_COLORS_LIST BOARD_ADAPTER::g_PasteColors;
CUSTOM_COLORS_LIST BOARD_ADAPTER::g_FinishColors;
CUSTOM_COLORS_LIST BOARD_ADAPTER::g_BoardColors;

KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBackgroundTop;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBackgroundBot;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSilkscreen;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSolderMask;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSolderPaste;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultSurfaceFinish;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultBoardBody;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultComments;
KIGFX::COLOR4D BOARD_ADAPTER::g_DefaultECOs;

const wxChar* BOARD_ADAPTER::m_logTrace = wxT( "KI_TRACE_EDA_CINFO3D_VISU" );

// The raytracer bevel global lives in board_adapter.cpp too.
float g_BevelThickness3DU = 0.0f;


// test: same default block as the real ctor (board_adapter.cpp:92-157) minus
// ReloadColorSettings() (our version below is settings-free) and the custom
// stackup color tables (not needed — GetLayerColors below is default-based).
BOARD_ADAPTER::BOARD_ADAPTER() :
        m_Cfg( nullptr ),
        m_IsBoardView( true ),
        m_MousewheelPanning( true ),
        m_IsPreviewer( false ),
        m_board( nullptr ),
        m_3dModelManager( nullptr ),
        m_layerZcoordTop(),
        m_layerZcoordBottom()
{
    m_boardPos = VECTOR2I();
    m_boardSize = VECTOR2I();
    m_boardCenter = SFVEC3F( 0.0f );

    m_boardBoundingBox.Reset();

    m_TH_IDs.Clear();
    m_TH_ODs.Clear();
    m_viaAnnuli.Clear();

    m_copperLayersCount = 2;

    m_biuTo3Dunits = 1.0;
    m_boardBodyThickness3DU = DEFAULT_BOARD_THICKNESS * m_biuTo3Dunits;
    m_frontCopperThickness3DU = DEFAULT_COPPER_THICKNESS * m_biuTo3Dunits;
    m_backCopperThickness3DU = DEFAULT_COPPER_THICKNESS * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_frontMaskThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_backMaskThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_solderPasteLayerThickness3DU = SOLDERPASTE_LAYER_THICKNESS * m_biuTo3Dunits;

    m_trackCount = 0;
    m_viaCount = 0;
    m_averageViaHoleDiameter = 0.0f;
    m_holeCount = 0;
    m_averageHoleDiameter = 0.0f;
    m_averageTrackWidth = 0.0f;

    m_BgColorBot = SFVEC4F( 0.4, 0.4, 0.5, 1.0 );
    m_BgColorTop = SFVEC4F( 0.8, 0.8, 0.9, 1.0 );
    m_BoardBodyColor = SFVEC4F( 0.4, 0.4, 0.5, 0.9 );
    m_SolderMaskColorTop = SFVEC4F( 0.1, 0.2, 0.1, 0.83 );
    m_SolderMaskColorBot = SFVEC4F( 0.1, 0.2, 0.1, 0.83 );
    m_SolderPasteColor = SFVEC4F( 0.4, 0.4, 0.4, 1.0 );
    m_SilkScreenColorTop = SFVEC4F( 0.9, 0.9, 0.9, 1.0 );
    m_SilkScreenColorBot = SFVEC4F( 0.9, 0.9, 0.9, 1.0 );
    m_CopperColor = SFVEC4F( 0.75, 0.61, 0.23, 1.0 );
    m_UserDrawingsColor = SFVEC4F( 0.85, 0.85, 0.85, 1.0 );
    m_UserCommentsColor = SFVEC4F( 0.85, 0.85, 0.85, 1.0 );
    m_ECO1Color = SFVEC4F( 0.70, 0.10, 0.10, 1.0 );
    m_ECO2Color = SFVEC4F( 0.70, 0.10, 0.10, 1.0 );

    for( int ii = 0; ii < 45; ++ii )
        m_UserDefinedLayerColor[ii] = SFVEC4F( 0.70, 0.10, 0.10, 1.0 );

    m_platedPadsFront = nullptr;
    m_platedPadsBack = nullptr;
    m_offboardPadsFront = nullptr;
    m_offboardPadsBack = nullptr;

    m_frontPlatedCopperPolys = nullptr;
    m_backPlatedCopperPolys = nullptr;

    ReloadColorSettings();

    g_DefaultBackgroundTop = COLOR4D( 0.80, 0.80, 0.90, 1.0 );
    g_DefaultBackgroundBot = COLOR4D( 0.40, 0.40, 0.50, 1.0 );
    g_DefaultSilkscreen = COLOR4D( 0.94, 0.94, 0.94, 1.0 );
    g_DefaultSolderMask = COLOR4D( 0.08, 0.20, 0.14, 0.83 );
    g_DefaultSolderPaste = COLOR4D( 0.50, 0.50, 0.50, 1.0 );
    g_DefaultSurfaceFinish = COLOR4D( 0.75, 0.61, 0.23, 1.0 );
    g_DefaultBoardBody = COLOR4D( 0.4, 0.4, 0.5, 0.9 );
    g_DefaultComments = COLOR4D( 0.85, 0.85, 0.85, 1.0 );
    g_DefaultECOs = COLOR4D( 0.70, 0.10, 0.10, 1.0 );
}


BOARD_ADAPTER::~BOARD_ADAPTER()
{
    destroyLayers();
}


// test: frees exactly what our InitSettings allocates.
void BOARD_ADAPTER::destroyLayers()
{
    for( auto& [layer, container] : m_layerMap )
        delete container;

    m_layerMap.clear();

    for( auto& [layer, container] : m_layerHoleMap )
        delete container;

    m_layerHoleMap.clear();

    for( auto& [layer, poly] : m_layers_poly )
        delete poly;

    m_layers_poly.clear();

    for( auto& [layer, poly] : m_layerHoleOdPolys )
        delete poly;

    m_layerHoleOdPolys.clear();

    for( auto& [layer, poly] : m_layerHoleIdPolys )
        delete poly;

    m_layerHoleIdPolys.clear();

    delete m_platedPadsFront;
    delete m_platedPadsBack;
    delete m_offboardPadsFront;
    delete m_offboardPadsBack;
    m_platedPadsFront = m_platedPadsBack = nullptr;
    m_offboardPadsFront = m_offboardPadsBack = nullptr;

    delete m_frontPlatedCopperPolys;
    delete m_backPlatedCopperPolys;
    m_frontPlatedCopperPolys = nullptr;
    m_backPlatedCopperPolys = nullptr;

    m_TH_ODs.Clear();
    m_TH_IDs.Clear();
    m_viaAnnuli.Clear();
    m_viaTH_ODs.Clear();

    m_board_poly.RemoveAllContours();
    m_TH_ODPolys.RemoveAllContours();
    m_NPTH_ODPolys.RemoveAllContours();
    m_viaTH_ODPolys.RemoveAllContours();
    m_viaAnnuliPolys.RemoveAllContours();
}


// test: settings-free — the board-editor color table gets a neutral default
// (the real one loads COLOR_SETTINGS; scenarios don't use per-PCB-layer colors).
void BOARD_ADAPTER::ReloadColorSettings() noexcept
{
    for( int layer = F_Cu; layer < PCB_LAYER_ID_COUNT; ++layer )
        m_BoardEditorColors[layer] = COLOR4D( 0.75, 0.61, 0.23, 1.0 );
}


// verbatim (board_adapter.cpp:243-288) minus the m_board branches (no board).
bool BOARD_ADAPTER::Is3dLayerEnabled( PCB_LAYER_ID aLayer,
                                      const std::bitset<LAYER_3D_END>& aVisibilityFlags ) const
{
    wxASSERT( aLayer < PCB_LAYER_ID_COUNT );

    switch( aLayer )
    {
    case B_Cu:      return aVisibilityFlags.test( LAYER_3D_COPPER_BOTTOM );
    case F_Cu:      return aVisibilityFlags.test( LAYER_3D_COPPER_TOP );
    case B_Adhes:   return aVisibilityFlags.test( LAYER_3D_ADHESIVE );
    case F_Adhes:   return aVisibilityFlags.test( LAYER_3D_ADHESIVE );
    case B_Paste:   return aVisibilityFlags.test( LAYER_3D_SOLDERPASTE );
    case F_Paste:   return aVisibilityFlags.test( LAYER_3D_SOLDERPASTE );
    case B_SilkS:   return aVisibilityFlags.test( LAYER_3D_SILKSCREEN_BOTTOM );
    case F_SilkS:   return aVisibilityFlags.test( LAYER_3D_SILKSCREEN_TOP );
    case B_Mask:    return aVisibilityFlags.test( LAYER_3D_SOLDERMASK_BOTTOM );
    case F_Mask:    return aVisibilityFlags.test( LAYER_3D_SOLDERMASK_TOP );
    case Dwgs_User: return aVisibilityFlags.test( LAYER_3D_USER_DRAWINGS );
    case Cmts_User: return aVisibilityFlags.test( LAYER_3D_USER_COMMENTS );
    case Eco1_User: return aVisibilityFlags.test( LAYER_3D_USER_ECO1 );
    case Eco2_User: return aVisibilityFlags.test( LAYER_3D_USER_ECO2 );
    default:
        return false; // test: no board -> unmapped layers hidden
    }
}


// test: previews/boards not modeled — footprints always "shown".
bool BOARD_ADAPTER::IsFootprintShown( const FOOTPRINT* aFootprint ) const
{
    return aFootprint != nullptr;
}


// verbatim no-board branch (board_adapter.cpp:315-320).
int BOARD_ADAPTER::GetHolePlatingThickness() const noexcept
{
    return DEFAULT_COPPER_THICKNESS;
}


// verbatim (board_adapter.cpp:322-328).
unsigned int BOARD_ADAPTER::GetCircleSegmentCount( float aDiameter3DU ) const
{
    wxASSERT( aDiameter3DU > 0.0f );

    return GetCircleSegmentCount( (int) ( aDiameter3DU / m_biuTo3Dunits ) );
}


// test: like board_adapter.cpp:330-336 with the BOARD_DESIGN_SETTINGS default
// max error (ARC_HIGH_DEF) instead of a live board's setting.
unsigned int BOARD_ADAPTER::GetCircleSegmentCount( int aDiameterBIU ) const
{
    wxASSERT( aDiameterBIU > 0 );

    return GetArcToSegmentCount( aDiameterBIU / 2, ARC_HIGH_DEF, FULL_CIRCLE );
}


// verbatim non-previewer path (board_adapter.cpp:808-905) minus FOLLOW_PCB
// (needs a board).
std::bitset<LAYER_3D_END> BOARD_ADAPTER::GetVisibleLayers() const
{
    std::bitset<LAYER_3D_END> ret;

    ret.set( LAYER_3D_BOARD, m_Cfg->m_Render.show_board_body );
    ret.set( LAYER_3D_PLATED_BARRELS, m_Cfg->m_Render.show_plated_barrels );
    ret.set( LAYER_3D_COPPER_TOP, m_Cfg->m_Render.show_copper_top );
    ret.set( LAYER_3D_COPPER_BOTTOM, m_Cfg->m_Render.show_copper_bottom );
    ret.set( LAYER_3D_SILKSCREEN_TOP, m_Cfg->m_Render.show_silkscreen_top );
    ret.set( LAYER_3D_SILKSCREEN_BOTTOM, m_Cfg->m_Render.show_silkscreen_bottom );
    ret.set( LAYER_3D_SOLDERMASK_TOP, m_Cfg->m_Render.show_soldermask_top );
    ret.set( LAYER_3D_SOLDERMASK_BOTTOM, m_Cfg->m_Render.show_soldermask_bottom );
    ret.set( LAYER_3D_SOLDERPASTE, m_Cfg->m_Render.show_solderpaste );
    ret.set( LAYER_3D_ADHESIVE, m_Cfg->m_Render.show_adhesive );
    ret.set( LAYER_3D_USER_COMMENTS, m_Cfg->m_Render.show_comments );
    ret.set( LAYER_3D_USER_DRAWINGS, m_Cfg->m_Render.show_drawings );
    ret.set( LAYER_3D_USER_ECO1, m_Cfg->m_Render.show_eco1 );
    ret.set( LAYER_3D_USER_ECO2, m_Cfg->m_Render.show_eco2 );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        ret.set( layer, m_Cfg->m_Render.show_user[layer - LAYER_3D_USER_1] );

    ret.set( LAYER_FP_REFERENCES, m_Cfg->m_Render.show_fp_references );
    ret.set( LAYER_FP_VALUES, m_Cfg->m_Render.show_fp_values );
    ret.set( LAYER_FP_TEXT, m_Cfg->m_Render.show_fp_text );

    ret.set( LAYER_3D_TH_MODELS, m_Cfg->m_Render.show_footprints_normal );
    ret.set( LAYER_3D_SMD_MODELS, m_Cfg->m_Render.show_footprints_insert );
    ret.set( LAYER_3D_VIRTUAL_MODELS, m_Cfg->m_Render.show_footprints_virtual );
    ret.set( LAYER_3D_MODELS_NOT_IN_POS, m_Cfg->m_Render.show_footprints_not_in_posfile );
    ret.set( LAYER_3D_MODELS_MARKED_DNP, m_Cfg->m_Render.show_footprints_dnp );

    ret.set( LAYER_3D_BOUNDING_BOXES, m_Cfg->m_Render.show_model_bbox );
    ret.set( LAYER_3D_OFF_BOARD_SILK, m_Cfg->m_Render.show_off_board_silk );
    ret.set( LAYER_3D_NAVIGATOR, m_Cfg->m_Render.show_navigator );

    return ret;
}


// test: no board -> board-editor copper colors never apply.
bool BOARD_ADAPTER::GetUseBoardEditorCopperLayerColors() const
{
    return false;
}


// verbatim (board_adapter.cpp:1039-1053).
float BOARD_ADAPTER::GetFootprintZPos( bool aIsFlipped ) const
{
    if( aIsFlipped )
    {
        if( auto it = m_layerZcoordBottom.find( B_Paste ); it != m_layerZcoordBottom.end() )
            return it->second;
    }
    else
    {
        if( auto it = m_layerZcoordTop.find( F_Paste ); it != m_layerZcoordTop.end() )
            return it->second;
    }

    return 0.0;
}


// verbatim (board_adapter.cpp:1056-1070); user-layer remap dropped (unused here).
SFVEC4F BOARD_ADAPTER::GetLayerColor( int aLayerId ) const
{
    wxASSERT( aLayerId < PCB_LAYER_ID_COUNT );

    return GetColor( m_BoardEditorColors.at( aLayerId ) );
}


SFVEC4F BOARD_ADAPTER::GetColor( const COLOR4D& aColor ) const
{
    return SFVEC4F( aColor.r, aColor.g, aColor.b, aColor.a );
}


// test: default-color scheme only (no COLOR_SETTINGS machinery).
std::map<int, COLOR4D> BOARD_ADAPTER::GetDefaultColors() const
{
    std::map<int, COLOR4D> colors;

    colors[LAYER_3D_BACKGROUND_TOP] = g_DefaultBackgroundTop;
    colors[LAYER_3D_BACKGROUND_BOTTOM] = g_DefaultBackgroundBot;
    colors[LAYER_3D_BOARD] = g_DefaultBoardBody;
    colors[LAYER_3D_COPPER_TOP] = g_DefaultSurfaceFinish;
    colors[LAYER_3D_COPPER_BOTTOM] = g_DefaultSurfaceFinish;
    colors[LAYER_3D_SILKSCREEN_TOP] = g_DefaultSilkscreen;
    colors[LAYER_3D_SILKSCREEN_BOTTOM] = g_DefaultSilkscreen;
    colors[LAYER_3D_SOLDERMASK_TOP] = g_DefaultSolderMask;
    colors[LAYER_3D_SOLDERMASK_BOTTOM] = g_DefaultSolderMask;
    colors[LAYER_3D_SOLDERPASTE] = g_DefaultSolderPaste;
    colors[LAYER_3D_USER_DRAWINGS] = g_DefaultComments;
    colors[LAYER_3D_USER_COMMENTS] = g_DefaultComments;
    colors[LAYER_3D_USER_ECO1] = g_DefaultECOs;
    colors[LAYER_3D_USER_ECO2] = g_DefaultECOs;

    return colors;
}


std::map<int, COLOR4D> BOARD_ADAPTER::GetLayerColors() const
{
    return GetDefaultColors();
}


// ---------------------------------------------------------------------------
// THE SEAM: synthetic test-board data instead of a real BOARD.
//
// A 40 x 30 mm two-layer board. Layer Z stacking follows the real
// InitSettings maths (board body centered on Z=0, copper plated on top/bottom,
// tech layers above copper). All values in BIU (nm) scaled by m_biuTo3Dunits.
// ---------------------------------------------------------------------------
void BOARD_ADAPTER::InitSettings( REPORTER* aStatusReporter, REPORTER* aWarningReporter )
{
    (void) aStatusReporter;
    (void) aWarningReporter;

    destroyLayers();

    const int boardW = pcbIUScale.mmToIU( 40 );
    const int boardH = pcbIUScale.mmToIU( 30 );

    m_boardSize = VECTOR2I( boardW, boardH );
    m_boardPos = VECTOR2I( 0, 0 );
    m_copperLayersCount = 2;

    // Same scale maths as the real InitSettings (board_adapter.cpp:382-387):
    // no BOARD -> the "footprint holder" zoom hack applies.
    m_biuTo3Dunits = RANGE_SCALE_3D / std::max( m_boardSize.x, m_boardSize.y );
    m_biuTo3Dunits *= 1.6f;

    m_boardBodyThickness3DU = DEFAULT_BOARD_THICKNESS * m_biuTo3Dunits;
    m_frontCopperThickness3DU = DEFAULT_COPPER_THICKNESS * m_biuTo3Dunits;
    m_backCopperThickness3DU = DEFAULT_COPPER_THICKNESS * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_frontMaskThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_backMaskThickness3DU = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_solderPasteLayerThickness3DU = SOLDERPASTE_LAYER_THICKNESS * m_biuTo3Dunits;

    // Layer Z coordinates (board body spans -body/2 .. +body/2).
    const float bodyTop = m_boardBodyThickness3DU / 2.0f;
    const float bodyBot = -m_boardBodyThickness3DU / 2.0f;

    m_layerZcoordBottom[F_Cu] = bodyTop;
    m_layerZcoordTop[F_Cu] = bodyTop + m_frontCopperThickness3DU;
    m_layerZcoordBottom[B_Cu] = bodyBot;
    m_layerZcoordTop[B_Cu] = bodyBot - m_backCopperThickness3DU;

    m_layerZcoordBottom[F_Mask] = m_layerZcoordTop[F_Cu];
    m_layerZcoordTop[F_Mask] = m_layerZcoordBottom[F_Mask] + m_frontMaskThickness3DU;
    m_layerZcoordBottom[B_Mask] = m_layerZcoordTop[B_Cu];
    m_layerZcoordTop[B_Mask] = m_layerZcoordBottom[B_Mask] - m_backMaskThickness3DU;

    m_layerZcoordBottom[F_SilkS] = m_layerZcoordTop[F_Mask];
    m_layerZcoordTop[F_SilkS] = m_layerZcoordBottom[F_SilkS] + m_nonCopperLayerThickness3DU;
    m_layerZcoordBottom[B_SilkS] = m_layerZcoordTop[B_Mask];
    m_layerZcoordTop[B_SilkS] = m_layerZcoordBottom[B_SilkS] - m_nonCopperLayerThickness3DU;

    m_layerZcoordBottom[F_Paste] = m_layerZcoordTop[F_Cu];
    m_layerZcoordTop[F_Paste] = m_layerZcoordBottom[F_Paste] + m_solderPasteLayerThickness3DU;
    m_layerZcoordBottom[B_Paste] = m_layerZcoordTop[B_Cu];
    m_layerZcoordTop[B_Paste] = m_layerZcoordBottom[B_Paste] - m_solderPasteLayerThickness3DU;

    m_boardCenter = SFVEC3F( 0.0f, 0.0f, 0.0f );

    m_boardBoundingBox.Set( SFVEC3F( -boardW / 2 * m_biuTo3Dunits, -boardH / 2 * m_biuTo3Dunits,
                                     bodyBot ),
                            SFVEC3F( boardW / 2 * m_biuTo3Dunits, boardH / 2 * m_biuTo3Dunits,
                                     bodyTop ) );

    // Board outline: plain rectangle.
    m_board_poly.RemoveAllContours();
    m_board_poly.NewOutline();
    m_board_poly.Append( -boardW / 2, -boardH / 2 );
    m_board_poly.Append( boardW / 2, -boardH / 2 );
    m_board_poly.Append( boardW / 2, boardH / 2 );
    m_board_poly.Append( -boardW / 2, boardH / 2 );
    m_board_poly.Outline( 0 ).SetClosed( true );

    // Copper: a few tracks + round pads per side; silkscreen: a frame; the
    // BVH containers own the objects (they delete them).
    auto addTrack = [&]( BVH_CONTAINER_2D* aDst, double aX1mm, double aY1mm, double aX2mm,
                         double aY2mm, double aWidthMm )
    {
        aDst->Add( new ROUND_SEGMENT_2D(
                SFVEC2F( pcbIUScale.mmToIU( aX1mm ) * m_biuTo3Dunits,
                         pcbIUScale.mmToIU( aY1mm ) * m_biuTo3Dunits ),
                SFVEC2F( pcbIUScale.mmToIU( aX2mm ) * m_biuTo3Dunits,
                         pcbIUScale.mmToIU( aY2mm ) * m_biuTo3Dunits ),
                pcbIUScale.mmToIU( aWidthMm ) * m_biuTo3Dunits, DummyBoardItem() ) );
    };

    auto addCircle = [&]( BVH_CONTAINER_2D* aDst, double aXmm, double aYmm, double aRmm )
    {
        aDst->Add( new FILLED_CIRCLE_2D( SFVEC2F( pcbIUScale.mmToIU( aXmm ) * m_biuTo3Dunits,
                                                  pcbIUScale.mmToIU( aYmm ) * m_biuTo3Dunits ),
                                         pcbIUScale.mmToIU( aRmm ) * m_biuTo3Dunits,
                                         DummyBoardItem() ) );
    };

    BVH_CONTAINER_2D* frontCu = new BVH_CONTAINER_2D;
    addTrack( frontCu, -15, -10, 15, -10, 1.0 );
    addTrack( frontCu, -15, -10, -15, 10, 1.0 );
    addTrack( frontCu, -15, 10, 0, 10, 0.6 );
    addTrack( frontCu, 0, 10, 8, 2, 0.6 );
    addCircle( frontCu, -15, -10, 1.5 );
    addCircle( frontCu, 15, -10, 1.5 );
    addCircle( frontCu, 8, 2, 1.2 );
    frontCu->BuildBVH();
    m_layerMap[F_Cu] = frontCu;

    BVH_CONTAINER_2D* backCu = new BVH_CONTAINER_2D;
    addTrack( backCu, 15, -10, 15, 10, 1.2 );
    addTrack( backCu, 15, 10, -8, 10, 1.2 );
    addCircle( backCu, -15, -10, 1.5 );
    addCircle( backCu, 15, -10, 1.5 );
    backCu->BuildBVH();
    m_layerMap[B_Cu] = backCu;

    BVH_CONTAINER_2D* frontSilk = new BVH_CONTAINER_2D;
    addTrack( frontSilk, -17, -13, 17, -13, 0.3 );
    addTrack( frontSilk, 17, -13, 17, 13, 0.3 );
    addTrack( frontSilk, 17, 13, -17, 13, 0.3 );
    addTrack( frontSilk, -17, 13, -17, -13, 0.3 );
    addCircle( frontSilk, -12, 6, 0.8 );
    frontSilk->BuildBVH();
    m_layerMap[F_SilkS] = frontSilk;

    // Through holes: the two big pads are plated through.
    auto addHolePoly = [&]( SHAPE_POLY_SET& aPolys, double aXmm, double aYmm, double aRmm )
    {
        TransformCircleToPolygon( aPolys,
                                  VECTOR2I( pcbIUScale.mmToIU( aXmm ), pcbIUScale.mmToIU( aYmm ) ),
                                  pcbIUScale.mmToIU( aRmm ), ARC_HIGH_DEF, ERROR_INSIDE );
    };

    addCircle( &m_TH_ODs, -15, -10, 0.8 );
    addCircle( &m_TH_ODs, 15, -10, 0.8 );
    m_TH_ODs.BuildBVH();

    addCircle( &m_TH_IDs, -15, -10, 0.65 );
    addCircle( &m_TH_IDs, 15, -10, 0.65 );
    m_TH_IDs.BuildBVH();

    addHolePoly( m_TH_ODPolys, -15, -10, 0.8 );
    addHolePoly( m_TH_ODPolys, 15, -10, 0.8 );

    m_TH_ODPolys.Simplify();
}
