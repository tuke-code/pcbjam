/*
 * sym_convert — standalone CLI for legacy (.lib) -> S-expression (.kicad_sym)
 * symbol-library conversion, built as a WebAssembly module.
 *
 * It drives SCH_IO_MGR::ConvertLibrary directly from main(): no GUI, no
 * renderer, no embind bindings, no JS host logic. Wired into
 * eeschema/CMakeLists.txt behind the KICAD_SYM_CONVERTER_WASM option
 * (see scripts/kicad/build-sym-convert-wasm.sh).
 *
 * Targets:
 *   - Node:     built with -sENVIRONMENT=node -sNODERAWFS=1 (the default here);
 *               run with `node sym_convert.js in.lib out.kicad_sym`.
 *   - wasmtime: a follow-up build (no pthreads, -sSTANDALONE_WASM) lets the
 *               same code run host-less under `wasmtime sym_convert.wasm ...`.
 *
 * GPL note: this is GPL KiCad code. The artifact is meant to be invoked as a
 * separate process from the closed ingester, never linked into closed code.
 */

#include <cstdio>

#include <wx/init.h>
#include <wx/string.h>

#include <sch_io/sch_io_mgr.h>

int main( int argc, char** argv )
{
    if( argc < 3 )
    {
        std::fprintf( stderr, "usage: sym_convert <input.lib> <output.kicad_sym>\n" );
        return 2;
    }

    // Bring up wxBase (no GUI): wxString / wxFileName / wxFFile rely on the
    // library being initialised. If the conversion path turns out to need more
    // global state (PGM_BASE, settings), the run will surface it here.
    wxInitializer initializer( argc, argv );

    if( !initializer.IsOk() )
    {
        std::fprintf( stderr, "sym_convert: wxWidgets initialisation failed\n" );
        return 3;
    }

    const wxString inPath  = wxString::FromUTF8( argv[1] );
    const wxString outPath = wxString::FromUTF8( argv[2] );

    // aOldFileProps = nullptr: no library-table properties; ConvertLibrary
    // guesses the source format from the path and writes the SCH_KICAD format.
    const bool ok = SCH_IO_MGR::ConvertLibrary( nullptr, inPath, outPath );

    if( ok )
    {
        std::fprintf( stderr, "sym_convert: OK  %s -> %s\n", argv[1], argv[2] );
        return 0;
    }

    std::fprintf( stderr, "sym_convert: FAILED to convert %s\n", argv[1] );
    return 1;
}
