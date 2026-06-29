/*
 * !!! NOT CURRENTLY COMPILED OR LINKED (task #54, 2026-06-29) !!!
 * Kept as a documented, ready-to-use fix but intentionally excluded from the build. See
 * scripts/kicad/build-kicad-target.sh (the "AVAILABLE BUT NOT COMPILED" block) for why and the exact
 * re-enable steps. Summary: the collab apply hung on a vtable-slot skew, not on this futex; the -DDEBUG
 * embind fix resolves it and collab passes WITHOUT this shim (connectivity recompute is bounded by the
 * pre-warmed pthread pool, so no on-demand Worker is needed). Re-enable ONLY if an on-demand-Worker
 * futex deadlock surfaces — symptom: an edit/collab apply hangs at commit.Push's RecalculateRatsnest on
 * a heavy board or a cold pool. The mechanism + fix below are correct and validated (pool-callafter repro).
 *
 * futex_yield.c — make a MAIN-THREAD emscripten_futex_wait() Asyncify-YIELD to the JS event loop
 * instead of busy-spinning, so an ON-DEMAND pthread-Worker can boot during the wait. Companion to
 * wasm/shims/nanosleep_yield.c: that covers the sleep_for/nanosleep join path; THIS covers the FUTEX
 * path (pthread_cond_wait / std::future::wait_for -> emscripten_futex_wait) that KiCad's thread-pool
 * connectivity recompute uses (CN_CONNECTIVITY_ALGO::searchConnections / CONNECTIVITY_DATA::
 * updateRatsnest). Collab apply parks there because the pool needs an on-demand Worker.
 *
 * THE PROBLEM: on the main browser thread Atomics.wait is forbidden, so upstream
 * emscripten_futex_wait busy-spins in futex_wait_main_browser_thread() calling _emscripten_yield(),
 * which services the proxying queue but NEVER returns to the JS event loop — so a NEW Worker's
 * 'loaded'->'run' handshake never runs and the wait spins forever (deadlock). nanosleep_yield only
 * patches nanosleep; this is the uncovered futex path.
 *
 * THE FIX: a strong emscripten_futex_wait definition that SHADOWS emscripten's archive member. On the
 * main thread it polls the futex word and, between polls, yields via an EM_ASYNC_JS setTimeout await
 * (= an Asyncify unwind/rewind, like emscripten_sleep) so the event loop runs and the on-demand
 * Worker boots. On a worker it keeps the real __builtin_wasm_memory_atomic_wait32 blocking wait.
 * (-Wl,--wrap is not used — it SIGSEGVs wasm-ld, same as nanosleep_yield.c notes.)
 */
#include <emscripten/emscripten.h>
#include <emscripten/threading.h>

#include <errno.h>
#include <math.h>
#include <stdint.h>

/* EM_ASYNC_JS integrates with Asyncify automatically (binaryen instruments every caller). */
EM_ASYNC_JS( void, __futex_main_thread_yield_ms, ( double ms ), {
    await new Promise( function( resolve ) { setTimeout( resolve, ms ); } );
} );

int emscripten_futex_wait( volatile void* addr, uint32_t val, double max_wait_ms )
{
    if( ( ( (intptr_t) addr ) & 3 ) != 0 )
        return -EINVAL;

    if( !emscripten_is_main_runtime_thread() )
    {
        /* worker thread: the real blocking atomic wait is allowed off the main browser thread. */
        int64_t ns = ( max_wait_ms == INFINITY ) ? -1 : (int64_t) ( max_wait_ms * 1000.0 * 1000.0 );
        int     r = __builtin_wasm_memory_atomic_wait32( (int*) addr, val, ns );
        if( r == 1 )
            return -EWOULDBLOCK;
        if( r == 2 )
            return -ETIMEDOUT;
        return 0;
    }

    /* main browser thread: poll + Asyncify-yield so the JS event loop runs between checks (on-demand
     * Workers boot, proxied work pumps), instead of the proxy-only busy-spin that deadlocks. */
    double end = emscripten_get_now() + max_wait_ms;
    while( __atomic_load_n( (volatile uint32_t*) addr, __ATOMIC_SEQ_CST ) == val )
    {
        if( max_wait_ms != INFINITY && emscripten_get_now() >= end )
            return -ETIMEDOUT;
        __futex_main_thread_yield_ms( 1 );
    }
    return -EWOULDBLOCK;
}
