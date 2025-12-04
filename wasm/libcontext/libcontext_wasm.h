/*
 * WASM implementation of libcontext using Emscripten Asyncify
 *
 * This header provides WASM-specific definitions for the libcontext API.
 * The implementation uses Emscripten's Asyncify feature to implement
 * fiber-like context switching in WebAssembly.
 *
 * Asyncify works by:
 * 1. Instrumenting the WASM code to save/restore the call stack
 * 2. Allowing execution to suspend at any point
 * 3. Resuming execution from where it was suspended
 *
 * This is used by KiCad's router for coroutine-based routing.
 */

#ifndef LIBCONTEXT_WASM_H
#define LIBCONTEXT_WASM_H

#include <cstddef>
#include <cstdint>

// Define WASM platform
#define LIBCONTEXT_PLATFORM_wasm
#define LIBCONTEXT_CALL_CONVENTION

#ifdef __cplusplus
namespace libcontext {
#endif

typedef void* fcontext_t;

#ifdef __cplusplus
extern "C" {
#endif

void LIBCONTEXT_CALL_CONVENTION release_fcontext( fcontext_t ctx );

intptr_t LIBCONTEXT_CALL_CONVENTION jump_fcontext( fcontext_t* ofc, fcontext_t nfc,
        intptr_t vp, bool preserve_fpu = true );

fcontext_t LIBCONTEXT_CALL_CONVENTION make_fcontext( void* sp, size_t size,
        void (* fn)( intptr_t ) );

#ifdef __cplusplus
}    // extern "C"
#endif

#ifdef __cplusplus
}    // namespace libcontext
#endif

#endif // LIBCONTEXT_WASM_H
