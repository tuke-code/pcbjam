/*
 * mallinfo() stub for the mimalloc-allocator build.
 *
 * We link with -sMALLOC=mimalloc (per-thread heaps — needed so the parallel
 * s-expr library parse doesn't serialize on dlmalloc's single global lock; see
 * docs/features/libs/0013). Unlike emscripten's dlmalloc/emmalloc, mimalloc does
 * NOT export the glibc memory-report API mallinfo()/mallinfo2(). OpenCASCADE's
 * OSD_MemInfo.cxx (in libTKernel, linked only by pcbnew for 3D/STEP) references
 * mallinfo(), so the pcbnew link fails with `undefined symbol: mallinfo` without
 * this. (eeschema doesn't link OpenCASCADE, so it never needed it.)
 *
 * mallinfo() is used purely for optional memory reporting, so a zeroed result is
 * harmless. Provided unconditionally (mirrors the nanosleep_yield / gl_ffp stub
 * pattern); it's a no-op for apps that never reference it.
 */

#include <malloc.h>

struct mallinfo mallinfo( void )
{
    struct mallinfo info = { 0 };
    return info;
}
