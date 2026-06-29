/**
 * pool_callafter_test.cpp — REPRO of the native-EH collab-apply thread-pool DEADLOCK (task #54).
 *
 * The deadlock shape (from the 4-agent research): a MAIN-THREAD FUTEX wait for a worker that needs
 * an ON-DEMAND Web Worker to boot. std::future::get -> pthread_cond_wait -> emscripten_futex_wait
 * busy-spins via _emscripten_yield (pumps only the proxy queue, NEVER the JS event loop), so the
 * on-demand Worker 'loaded'->'run' handshake can't run -> permanent spin. nanosleep_yield.c fixes the
 * sleep_for/nanosleep path (see pthread-ondemand), but NOT this futex path — exactly the path KiCad's
 * connectivity recompute uses (CN_CONNECTIVITY_ALGO::searchConnections / updateRatsnest ->
 * std::future::wait_for / multi_future::wait).
 *
 * Setup mirrors pthread-ondemand: GetKiCadThreadPool() consumes ALL pre-warmed Web Workers (like
 * KiCad's board load), so the std::async tasks here must boot on-demand. We then FUTEX-wait for them.
 *
 *   ?defer=0   run the wait in OnInit (before the main loop / board-LOAD context)
 *   ?defer=1   run the wait via CallAfter (inside the per-frame-yield main loop / collab-APPLY context)
 *
 * Expectation: the futex wait for on-demand Workers DEADLOCKS (no [POOL] SUCCESS) — the 'AFTER
 * emscripten_sleep' line proves asyncify itself is fine; only the futex on-demand-boot wait hangs.
 */

#include "wx/wx.h"

#include <thread_pool.h> // kicad/include/thread_pool.h -> GetKiCadThreadPool() + BS::*

#include <chrono>
#include <cmath>
#include <cstdio>
#include <future>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

using clk = std::chrono::steady_clock;

static void plog( const char* s )
{
#ifdef __EMSCRIPTEN__
    EM_ASM( { console.log( UTF8ToString( $0 ) ); }, s );
#else
    printf( "%s\n", s );
#endif
}

static int readDefer()
{
#ifdef __EMSCRIPTEN__
    return EM_ASM_INT( {
        var raw = location.search ? location.search.slice( 1 ) : location.hash.slice( 1 );
        var v = parseInt( new URLSearchParams( raw ).get( 'defer' ), 10 );
        return isNaN( v ) ? 1 : v;
    } );
#else
    return 1;
#endif
}

static double computeBlock( int b )
{
    double acc = 0.0;
    for( long i = 0; i < 1000000; ++i )
        acc += std::sin( (double) ( b * 131 + i ) * 1e-6 ) * std::cos( (double) i * 1e-7 );
    return acc;
}

static volatile double g_sink = 0.0;

static void runPoolWork()
{
    plog( "[POOL] runPoolWork: BEFORE emscripten_sleep" );
    emscripten_sleep( 1 );
    plog( "[POOL] runPoolWork: AFTER emscripten_sleep  (asyncify works in this context)" );

    // std::async(launch::async) spawns a NEW pthread; the pre-warmed pool was consumed in OnInit, so
    // these need ON-DEMAND Web Workers. future.get() FUTEX-waits for them on the main thread.
    plog( "[POOL] runPoolWork: std::async x4 (ON-DEMAND workers) + future.get (FUTEX wait)" );
    auto t0 = clk::now();
    std::vector<std::future<double>> futs;
    for( int i = 0; i < 4; ++i )
        futs.push_back( std::async( std::launch::async, [i] { return computeBlock( i ); } ) );
    double total = 0.0;
    for( auto& f : futs )
        total += f.get(); // <-- main-thread futex wait for an on-demand Worker; deadlocks if it can't boot
    g_sink = total;

    char buf[128];
    long poolMs = (long) std::chrono::duration_cast<std::chrono::milliseconds>( clk::now() - t0 ).count();
    std::snprintf( buf, sizeof( buf ), "[POOL] SUCCESS futexWaitMs=%ld sink=%.3f", poolMs, g_sink );
    plog( buf );
}

class ReproApp : public wxApp
{
public:
    bool OnInit() override
    {
        const int defer = readDefer();
        char buf[96];
        std::snprintf( buf, sizeof( buf ), "[POOL] START defer=%d", defer );
        plog( buf );

        // Consume ALL pre-warmed Web Workers with the REAL pool (exactly like KiCad's board load),
        // so the std::async tasks in runPoolWork() must boot ON-DEMAND Workers.
        thread_pool& tp = GetKiCadThreadPool();
        std::snprintf( buf, sizeof( buf ), "[POOL] pool consumed pre-warmed Workers; poolThreads=%u",
                       (unsigned) tp.get_thread_count() );
        plog( buf );

        ( new wxFrame( nullptr, wxID_ANY, "pool-callafter repro" ) )->Show();

        if( defer )
        {
            plog( "[POOL] scheduling runPoolWork via CallAfter (APPLY path)" );
            CallAfter( [] { runPoolWork(); } );
        }
        else
        {
            plog( "[POOL] running runPoolWork in OnInit (LOAD path / control)" );
            runPoolWork();
        }
        return true;
    }
};

wxIMPLEMENT_APP( ReproApp );
