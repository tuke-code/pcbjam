/*
 * Embind bindings for KiCad symbol_editor WASM.
 *
 * Picked up automatically by scripts/kicad/build-kicad-target.sh when building
 * the symbol_editor app (it compiles wasm/bindings/<app>_embind.cpp if present).
 *
 * The symbol editor is the eeschema kiface launched with
 * TOP_FRAME=FRAME_SCH_SYMBOL_EDITOR, so it links the same
 * eeschema_kiface_objects as eeschema — including files-io.cpp, whose save
 * chokepoint references kicadCollabOnSave. eeschema gets the definition from
 * eeschema_embind.cpp; this TU provides it for the symbol_editor link.
 * eeschema's other bindings are not pulled in here: they assume a
 * SCH_EDIT_FRAME top frame, which this app doesn't have.
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// C++ → JS save notification (standalone-hardening save routing). Called from
// the kicad fork's save chokepoints after a successful write to MEMFS, so the
// web app can route the saved bytes onward (API upload, local-disk write-back,
// download). No-op without a JS listener.
extern "C" void kicadCollabOnSave( const char* aPath )
{
    EM_ASM( {
        if( window.kicadCollab && window.kicadCollab.onSave )
            window.kicadCollab.onSave( UTF8ToString( $0 ) );
    }, aPath );
}

#endif // __EMSCRIPTEN__
