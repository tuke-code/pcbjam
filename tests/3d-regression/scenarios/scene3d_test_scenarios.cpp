#include "scene3d_test_scenarios.h"
#include "scene3d_test_ctx.h"

#include <wx/debug.h>

// Tier-1 scenario functions (scenario_tier1_*.cpp)
void Scenario_BgGradient( SCENE3D_CTX& aCtx );
void Scenario_BgGradientAlpha( SCENE3D_CTX& aCtx );
void Scenario_BoundingBox( SCENE3D_CTX& aCtx );
void Scenario_HalfOpenCylinder( SCENE3D_CTX& aCtx );
void Scenario_SegmentSingle( SCENE3D_CTX& aCtx );
void Scenario_SegmentsStar( SCENE3D_CTX& aCtx );
void Scenario_RoundArrow( SCENE3D_CTX& aCtx );
void Scenario_RoundArrowsAxes( SCENE3D_CTX& aCtx );
void Scenario_MaterialCopper( SCENE3D_CTX& aCtx );
void Scenario_MaterialDiffuseOnly( SCENE3D_CTX& aCtx );
void Scenario_MaterialTransparent( SCENE3D_CTX& aCtx );
void Scenario_LightFront( SCENE3D_CTX& aCtx );
void Scenario_LightTop( SCENE3D_CTX& aCtx );
void Scenario_LightBottom( SCENE3D_CTX& aCtx );
void Scenario_TdlDrawTop( SCENE3D_CTX& aCtx );
void Scenario_TdlDrawBot( SCENE3D_CTX& aCtx );
void Scenario_TdlDrawMiddle( SCENE3D_CTX& aCtx );
void Scenario_TdlDrawAll( SCENE3D_CTX& aCtx );
void Scenario_TdlSegEndsTexture( SCENE3D_CTX& aCtx );
void Scenario_TdlCulledStencil( SCENE3D_CTX& aCtx );
void Scenario_TdlZScale( SCENE3D_CTX& aCtx );
void Scenario_TdlTransparent( SCENE3D_CTX& aCtx );
void Scenario_Model3dOpaque( SCENE3D_CTX& aCtx );
void Scenario_Model3dTransparent( SCENE3D_CTX& aCtx );
void Scenario_Model3dMaterialModes( SCENE3D_CTX& aCtx );
void Scenario_Model3dBbox( SCENE3D_CTX& aCtx );
void Scenario_SpheresGizmo( SCENE3D_CTX& aCtx );
void Scenario_CameraPersp( SCENE3D_CTX& aCtx );
void Scenario_CameraOrtho( SCENE3D_CTX& aCtx );
void Scenario_CameraPresetViews( SCENE3D_CTX& aCtx );

// Tier-2 scenario functions (scenario_tier2_generators.cpp)
void Scenario_GenCylinder( SCENE3D_CTX& aCtx );
void Scenario_GenInvCone( SCENE3D_CTX& aCtx );
void Scenario_GenDisk( SCENE3D_CTX& aCtx );
void Scenario_GenDimple( SCENE3D_CTX& aCtx );
void Scenario_AddObjAllShapes( SCENE3D_CTX& aCtx );
void Scenario_PostMachining( SCENE3D_CTX& aCtx );
void Scenario_ViaComposite( SCENE3D_CTX& aCtx );
void Scenario_Grid1mm( SCENE3D_CTX& aCtx );
void Scenario_Grid2p5mm( SCENE3D_CTX& aCtx );
void Scenario_Grid5mm( SCENE3D_CTX& aCtx );
void Scenario_Grid10mm( SCENE3D_CTX& aCtx );
void Scenario_LayerMaterials( SCENE3D_CTX& aCtx );
void Scenario_ArrowMaterial( SCENE3D_CTX& aCtx );
void Scenario_CreateBoard( SCENE3D_CTX& aCtx );

// Tier-3 scenario functions (scenario_tier3_composite.cpp)
void Scenario_RedrawEmpty( SCENE3D_CTX& aCtx );
void Scenario_RedrawMiniBoard( SCENE3D_CTX& aCtx );
void Scenario_RedrawMiniBoardNavigator( SCENE3D_CTX& aCtx );

namespace Scene3DTest
{

struct SCENARIO_ENTRY
{
    const char* name; // becomes 3d-<name>.png — append-only, never rename
    void ( *render )( SCENE3D_CTX& );
};

static const SCENARIO_ENTRY SCENARIOS[] = {
    { "bg-gradient", Scenario_BgGradient },
    { "bg-gradient-alpha", Scenario_BgGradientAlpha },
    { "bounding-box", Scenario_BoundingBox },
    { "half-open-cylinder", Scenario_HalfOpenCylinder },
    { "segment-single", Scenario_SegmentSingle },
    { "segments-star", Scenario_SegmentsStar },
    { "round-arrow", Scenario_RoundArrow },
    { "round-arrows-axes", Scenario_RoundArrowsAxes },
    { "material-copper", Scenario_MaterialCopper },
    { "material-diffuse-only", Scenario_MaterialDiffuseOnly },
    { "material-transparent", Scenario_MaterialTransparent },
    { "light-front", Scenario_LightFront },
    { "light-top", Scenario_LightTop },
    { "light-bottom", Scenario_LightBottom },
    { "tdl-draw-top", Scenario_TdlDrawTop },
    { "tdl-draw-bot", Scenario_TdlDrawBot },
    { "tdl-draw-middle", Scenario_TdlDrawMiddle },
    { "tdl-draw-all", Scenario_TdlDrawAll },
    { "tdl-seg-ends-texture", Scenario_TdlSegEndsTexture },
    { "tdl-culled-stencil", Scenario_TdlCulledStencil },
    { "tdl-zscale", Scenario_TdlZScale },
    { "tdl-transparent", Scenario_TdlTransparent },
    { "model3d-opaque", Scenario_Model3dOpaque },
    { "model3d-transparent", Scenario_Model3dTransparent },
    { "model3d-material-modes", Scenario_Model3dMaterialModes },
    { "model3d-bbox", Scenario_Model3dBbox },
    { "spheres-gizmo", Scenario_SpheresGizmo },
    { "camera-persp", Scenario_CameraPersp },
    { "camera-ortho", Scenario_CameraOrtho },
    { "camera-preset-views", Scenario_CameraPresetViews },
    { "gen-cylinder", Scenario_GenCylinder },
    { "gen-invcone", Scenario_GenInvCone },
    { "gen-disk", Scenario_GenDisk },
    { "gen-dimple", Scenario_GenDimple },
    { "addobj-all-shapes", Scenario_AddObjAllShapes },
    { "post-machining", Scenario_PostMachining },
    { "via-composite", Scenario_ViaComposite },
    { "grid-1mm", Scenario_Grid1mm },
    { "grid-2p5mm", Scenario_Grid2p5mm },
    { "grid-5mm", Scenario_Grid5mm },
    { "grid-10mm", Scenario_Grid10mm },
    { "layer-materials", Scenario_LayerMaterials },
    { "arrow-material", Scenario_ArrowMaterial },
    { "create-board", Scenario_CreateBoard },
    { "redraw-empty", Scenario_RedrawEmpty },
    { "redraw-mini-board", Scenario_RedrawMiniBoard },
    { "redraw-mini-board-navigator", Scenario_RedrawMiniBoardNavigator },
};

static const int SCENARIO_COUNT = sizeof( SCENARIOS ) / sizeof( SCENARIOS[0] );


int GetScenarioCount()
{
    return SCENARIO_COUNT;
}


const char* GetScenarioName( int aIndex )
{
    if( aIndex < 0 || aIndex >= SCENARIO_COUNT )
        return nullptr;

    return SCENARIOS[aIndex].name;
}


void RenderScenario( SCENE3D_CTX& aCtx, int aIndex )
{
    wxASSERT( aIndex >= 0 && aIndex < SCENARIO_COUNT );

    if( aIndex < 0 || aIndex >= SCENARIO_COUNT )
        return;

    SCENARIOS[aIndex].render( aCtx );
}

} // namespace Scene3DTest
