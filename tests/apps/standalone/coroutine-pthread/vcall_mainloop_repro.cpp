// vcall_mainloop_repro.cpp — step 2 of the vii root hunt (task #54).
//
// The plain-main fiber repro (vcall_fiber_repro) passes all 4 signatures even with interleaved
// asyncify suspends — so the vii hang is NOT signature-intrinsic. The real doApply runs from inside
// the per-frame-yield main loop's rAF callback (rAF -> callUserCallback -> iterFunc -> dynCall_v ->
// wasm refresh -> the apply COROUTINE), which is the JS->wasm boundary KiCad actually uses (see
// mainloop_repro.cpp). This repro adds exactly that: activate the COROUTINE + the four virtual calls
// from inside emscripten_set_main_loop, with an in-coroutine emscripten_sleep mirroring commit.Push's
// futex suspend. If a vii call now parks while viii/ii pass, the rAF/dynCall_v context is the trigger.

#include "kicad_coroutine_harness.h"

#include <emscripten.h>

#include <cstdio>

using coroutine_test::TestCoroutine;

struct Vec2
{
    int x;
    int y;
};
struct Base
{
    int          m_w = 0;
    virtual int  getInt() { return 7; }                    // ii
    virtual void setVii( int w ) { m_w = w; }              // vii
    virtual void setViii( int a, int b ) { m_w = a + b; }  // viii
    virtual Vec2 getVec() { return Vec2{ m_w, m_w + 1 }; } // sret -> vii
    virtual ~Base() {}
};
struct Derived : Base
{
    int  getInt() override { return 13; }
    void setVii( int w ) override { m_w = w + 1; }
    void setViii( int a, int b ) override { m_w = a + b + 1; }
    Vec2 getVec() override { return Vec2{ m_w + 1, m_w + 2 }; }
};

static int   readSeed()
{
    return EM_ASM_INT( {
        var raw = location.search ? location.search.slice( 1 ) : '';
        var v = parseInt( new URLSearchParams( raw ).get( 'seed' ), 10 );
        return isNaN( v ) ? 1 : v;
    } );
}
static Base* makeObj( int seed ) __attribute__( ( noinline ) );
static Base* makeObj( int seed ) { return ( seed & 1 ) ? static_cast<Base*>( new Derived() ) : new Base(); }
static Base* g_obj  = nullptr;
static int   g_frame = 0;
static void  plog( const char* s ) { EM_ASM( { console.log( UTF8ToString( $0 ) ); }, s ); }

static void doApplyLike()
{
    // Mirror doApply: run the virtual calls INSIDE the libcontext COROUTINE, with a suspend first
    // (commit.Push's futex). The calls run after a fiber suspend, in the rAF/dynCall_v stack.
    TestCoroutine co( []( TestCoroutine& self ) {
        char b[96];
        emscripten_sleep( 1 );  // mirror commit.Push's connectivity futex suspend
        plog( "[VCALL] fiber: slept; before getInt (ii)" );
        int i = g_obj->getInt();
        std::snprintf( b, sizeof b, "[VCALL] fiber: getInt=%d (ii OK)", i ); plog( b );

        plog( "[VCALL] fiber: before setViii (viii)" );
        g_obj->setViii( 10, 20 );
        std::snprintf( b, sizeof b, "[VCALL] fiber: after setViii m_w=%d (viii OK)", g_obj->m_w ); plog( b );

        plog( "[VCALL] fiber: before getVec (sret -> vii)" );
        Vec2 v = g_obj->getVec();
        std::snprintf( b, sizeof b, "[VCALL] fiber: after getVec=(%d,%d) (sret-vii OK)", v.x, v.y ); plog( b );

        plog( "[VCALL] fiber: before setVii (vii)" );
        g_obj->setVii( 200000 );
        std::snprintf( b, sizeof b, "[VCALL] fiber: after setVii m_w=%d (vii OK)", g_obj->m_w ); plog( b );

        self.Yield( 42 );
    } );
    co.Call( 1 );
    plog( "[VCALL] doApplyLike: cor.Call returned" );
}

static void mainLoopIter()
{
    ++g_frame;
    if( g_frame == 3 )
    {
        plog( "[VCALL] frame 3: activating COROUTINE apply from rAF (dynCall_v boundary)" );
        doApplyLike();
        plog( "[VCALL] DONE" );
        emscripten_cancel_main_loop();
    }
}

int main()
{
    g_obj = makeObj( readSeed() );  // runtime seed => genuine call_indirect
    plog( "[VCALL] start; emscripten_set_main_loop (rAF)" );
    emscripten_set_main_loop( mainLoopIter, 0, 0 );  // main returns; rAF drives mainLoopIter
    return 0;
}
