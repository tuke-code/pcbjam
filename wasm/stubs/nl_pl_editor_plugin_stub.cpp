/*
 * 3Dconnexion SpaceMouse plugin stubs for KiCad pagelayout_editor WASM build.
 * The 3DxWare driver is unavailable in the browser; these stubs satisfy the
 * symbols referenced from pl_editor_frame.cpp without doing anything.
 */

// Minimal definition for NL_PL_EDITOR_PLUGIN_IMPL — required because the
// unique_ptr<NL_PL_EDITOR_PLUGIN_IMPL> destructor needs a complete type.
class NL_PL_EDITOR_PLUGIN_IMPL {};

#include <navlib/nl_pl_editor_plugin.h>

NL_PL_EDITOR_PLUGIN::NL_PL_EDITOR_PLUGIN()
{
}

NL_PL_EDITOR_PLUGIN::~NL_PL_EDITOR_PLUGIN()
{
}

void NL_PL_EDITOR_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aViewport )
{
    (void) aViewport;
}

void NL_PL_EDITOR_PLUGIN::SetFocus( bool aFocus )
{
    (void) aFocus;
}
