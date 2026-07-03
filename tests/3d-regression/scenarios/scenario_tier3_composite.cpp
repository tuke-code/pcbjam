/**
 * Tier-3 scenarios: full RENDER_3D_OPENGL::Redraw() composites over the
 * synthetic mini-board (board_adapter_test_impl.cpp InitSettings). Redraw's
 * reload() re-runs InitSettings itself, so these exercise the complete real
 * pipeline: board body triangulation, layer display lists, stencil hole
 * subtraction, solder mask transparency, grid list and the navigator gizmo.
 */

#include "scene3d_test_ctx.h"
#include "scene3d_test_rig.h"

// The composites drive Redraw() exactly like EDA_3D_CANVAS::DoRePaint does:
// window size, reload request, then Redraw(aIsMoving=false, no reporters).
static void runRedraw( SCENE3D_CTX& aCtx, SCENE3D_TEST_RIG& aRig )
{
    aRig->SetCurWindowSize( wxSize( aCtx.m_width, aCtx.m_height ) );
    aRig->ReloadRequest();
    aRig->Redraw( false, nullptr, nullptr );
}

// 45: Redraw with every layer hidden — background, camera matrices, lights and
// the frame plumbing only (the "empty viewer" reference frame).
void Scenario_RedrawEmpty( SCENE3D_CTX& aCtx )
{
    SCENE3D_TEST_RIG rig( aCtx );

    rig.m_cfg.m_Render.show_board_body = false;
    rig.m_cfg.m_Render.show_copper_top = false;
    rig.m_cfg.m_Render.show_copper_bottom = false;
    rig.m_cfg.m_Render.show_silkscreen_top = false;
    rig.m_cfg.m_Render.show_silkscreen_bottom = false;
    rig.m_cfg.m_Render.show_soldermask_top = false;
    rig.m_cfg.m_Render.show_soldermask_bottom = false;
    rig.m_cfg.m_Render.show_solderpaste = false;
    rig.m_cfg.m_Render.show_adhesive = false;
    rig.m_cfg.m_Render.show_plated_barrels = false;

    aCtx.SetIsoView();
    runRedraw( aCtx, rig );
}

// 46: the full synthetic two-layer mini-board — copper tracks/pads with
// stencil-subtracted through holes, silkscreen frame, translucent solder mask
// and board body.
void Scenario_RedrawMiniBoard( SCENE3D_CTX& aCtx )
{
    SCENE3D_TEST_RIG rig( aCtx );

    aCtx.SetIsoView();
    runRedraw( aCtx, rig );
}

// 47: everything at once — mini-board plus the 5mm grid and the navigator
// spheres gizmo, straight top view. The port-complete gate.
void Scenario_RedrawMiniBoardNavigator( SCENE3D_CTX& aCtx )
{
    SCENE3D_TEST_RIG rig( aCtx );

    rig.m_cfg.m_Render.grid_type = GRID3D_TYPE::GRID_5MM;
    rig.m_cfg.m_Render.show_navigator = true;

    aCtx.ResetCamera();
    aCtx.SetView( VIEW3D_TYPE::VIEW3D_TOP );
    runRedraw( aCtx, rig );
}
