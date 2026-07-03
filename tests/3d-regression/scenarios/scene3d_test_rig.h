/**
 * Shared Tier-2/Tier-3 rig: stub settings + synthetic BOARD_ADAPTER
 * (native/board_adapter_test_impl.cpp InitSettings) + a real RENDER_3D_OPENGL
 * bound to the scenario camera, initialized the way the first real Redraw()
 * would (initializeOpenGL under the fresh identity modelview — init_lights
 * re-runs identically, keeping the directional lights eye-anchored).
 */

#ifndef SCENE3D_TEST_RIG_H
#define SCENE3D_TEST_RIG_H

#include "scene3d_test_ctx.h"

#include "../native/render3d_test_accessor.h"

#include "3d_canvas/board_adapter.h"
#include "3d_rendering/opengl/render_3d_opengl.h"
#include "3d_viewer/eda_3d_viewer_settings.h"

#include <memory>

struct SCENE3D_TEST_RIG
{
    EDA_3D_VIEWER_SETTINGS            m_cfg;
    BOARD_ADAPTER                     m_adapter;
    std::unique_ptr<RENDER_3D_OPENGL> m_renderer;

    explicit SCENE3D_TEST_RIG( SCENE3D_CTX& aCtx )
    {
        m_adapter.m_Cfg = &m_cfg;
        m_adapter.InitSettings( nullptr, nullptr );

        m_renderer = std::make_unique<RENDER_3D_OPENGL>( nullptr, m_adapter, aCtx.m_camera );

        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
        R3D_InitializeOpenGL( *m_renderer );
    }

    RENDER_3D_OPENGL& operator*() { return *m_renderer; }
    RENDER_3D_OPENGL* operator->() { return m_renderer.get(); }
};

#endif // SCENE3D_TEST_RIG_H
