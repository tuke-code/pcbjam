/*
 * Merged-editor safety-net Kiface() (editor-unification Part 2).
 *
 * In the merged kicad_editor image the pcbnew and eeschema kifaces are compiled with
 * -DKiface=PcbKiface / -DKiface=SchKiface, so every module-owned call site binds
 * statically to its own engine. The handful of shared common/ call sites are patched
 * to resolve their owning frame's kiface exactly (eda_base_frame.cpp,
 * dialog_color_picker.cpp, design_block_tree_model_adapter.cpp) and only FALL BACK to
 * the plain Kiface() — which, in this image, is THIS definition. It also keeps any
 * future upstream common/ caller linking.
 *
 * Resolution is best-effort by design: focused window → top window → KIWAY top frame,
 * walking parents to an EDA_BASE_FRAME whose FRAME_T names its face. With the one-
 * frame-per-page-load model there is exactly one editor frame, so the walk is exact
 * in practice; FACE_PCB is the last-resort default.
 */

#ifdef __EMSCRIPTEN__

#include <wx/app.h>
#include <wx/window.h>

#include <kiway.h>
#include <kiface_base.h>
#include <eda_base_frame.h>

// single_top.cpp's process-global KIWAY (declared in kiway.h).
static KIFACE_BASE* frameKiface( wxWindow* aWindow )
{
    for( wxWindow* w = aWindow; w; w = w->GetParent() )
    {
        if( EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( w ) )
        {
            KIWAY::FACE_T face = KIWAY::KifaceType( frame->GetFrameType() );

            if( face != KIWAY::FACE_T( -1 ) )
            {
                if( KIFACE* kiface = Kiway.KiFACE( face ) )
                    return static_cast<KIFACE_BASE*>( kiface );
            }
        }
    }

    return nullptr;
}


KIFACE_BASE& Kiface()
{
    if( KIFACE_BASE* kiface = frameKiface( wxWindow::FindFocus() ) )
        return *kiface;

    if( wxTheApp )
    {
        if( KIFACE_BASE* kiface = frameKiface( wxTheApp->GetTopWindow() ) )
            return *kiface;
    }

    if( KIFACE_BASE* kiface = frameKiface( Kiway.GetTop() ) )
        return *kiface;

    // Last resort — no frame up yet. FACE_PCB is the merged image's default TOP_FRAME
    // face; both faces are registered in OnPgmInit, so this cannot be reached before
    // registration in any path that previously had a working Kiface().
    return *static_cast<KIFACE_BASE*>( Kiway.KiFACE( KIWAY::FACE_PCB ) );
}

#endif // __EMSCRIPTEN__
