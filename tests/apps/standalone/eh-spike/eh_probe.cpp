// eh_probe.cpp — Phase 0 probe for the native-wasm-EH spike.
// See docs/features/wasm-exceptions/06-spike-plan.md.
//
// Purpose: determine whether emscripten 4.0.2's LLVM emits *parseable* legacy
// wasm exception-handling on a tiny program. The parked experiment
// (docs/wasm-exceptions-experiment.md) hit a Binaryen parse failure
// ("popping from empty stack") at wasm-emscripten-finalize when building full
// pcbnew with -fwasm-exceptions -sWASM_LEGACY_EXCEPTIONS=1. This isolates that:
// no Asyncify, no wxWidgets, no OpenCASCADE — just throw/catch.
//
// Built two ways from one source (Makefile.wasm `eh-probe` target):
//   - eh_probe_jseh.js   : -fexceptions                (JS-EH baseline, must work)
//   - eh_probe_wasmeh.js  : -fwasm-exceptions ... =1     (native legacy wasm-EH, the probe)
// Run with node; both must print "[EH_PROBE] PASS".

#include <cstdio>
#include <stdexcept>

// noinline + a runtime-dependent argument so -O2 cannot fold the throw away.
static int __attribute__( ( noinline ) ) might_throw( int x )
{
    if( x > 0 )
        throw std::runtime_error( "boom" );

    return x;
}

int main()
{
    int caught = 0;

    try
    {
        might_throw( 1 );
        printf( "[EH_PROBE] FAIL: throw did not propagate\n" );
    }
    catch( const std::exception& e )
    {
        caught = 1;
        printf( "[EH_PROBE] caught: %s\n", e.what() );
    }

    printf( "[EH_PROBE] %s\n", caught ? "PASS throw/catch works" : "FAIL no catch" );
    return caught ? 0 : 1;
}
