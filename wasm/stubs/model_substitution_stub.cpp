/*
 * MODEL_SUBSTITUTION stubs for the KiCad WASM build.
 *
 * When KICAD_BUILD_3D_VIEWER_WASM=OFF the 3D-viewer library is not linked, but
 * it is where the .wrl→STEP model-substitution helpers
 * (3d-viewer/3d_cache/model_substitution_helpers.cpp) live. pcbnew's
 * dialog_migrate_3d_models.cpp still references them, so the final pcbnew/
 * footprint_editor link fails with undefined MODEL_SUBSTITUTION symbols.
 *
 * These no-op stubs satisfy the linker. There is no 3D viewer in the browser
 * build, so model migration simply finds no substitutions (the dialog reports
 * nothing to migrate) — which is the correct behavior for a WASM build with no
 * local 3D model filesystem.
 */

#include <3d_cache/model_substitution_helpers.h>

namespace MODEL_SUBSTITUTION
{

bool IsWrlExtension( const wxString& )
{
    return false;
}


void STEP_CATALOG::Build( const wxString&, const FILENAME_RESOLVER* )
{
}


wxString STEP_CATALOG::FindMatchFor( const wxString& ) const
{
    return wxEmptyString;
}

}  // namespace MODEL_SUBSTITUTION
