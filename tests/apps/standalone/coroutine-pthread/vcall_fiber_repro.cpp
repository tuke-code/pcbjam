// vcall_fiber_repro.cpp — ISOLATE the native-EH "vii" virtual call_indirect mis-dispatch (task #54).
//
// In real pcbnew, a VIRTUAL method call made from embind code HANGS under native-EH + post-link
// asyncify when its wasm signature is "vii" = (i32,i32)->void — both void setters (SetWidth(int)) and
// struct-returning getters lowered via the sret ABI (GetPosition()->VECTOR2I => (this,sret)->void).
// Signature "ii" (Type/GetWidth, (this)->i32) works; "viii" (view->Update, (this,i32,i32)->void) works.
//
// This calls all four shapes on a non-devirtualizable polymorphic object FROM A libcontext FIBER and
// logs each — to prove the hang is SIGNATURE-SPECIFIC ("vii" parks, the others pass) in isolation,
// with a small wasm we can disassemble. Built native-EH by default (Makefile EH_FLAGS); WX_LEGACY_EH=1
// gives the JS-EH control.
//
// Expectation if it reproduces: getInt(ii) + setViii(viii) print; getVec(sret-vii) and/or setVii(vii)
// do NOT print "after".

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
    virtual int  getInt() { return 7; }                          // "ii"   (this)->i32
    virtual void setVii( int w ) { m_w = w; }                    // "vii"  (this,i32)->void
    virtual void setViii( int a, int b ) { m_w = a + b; }        // "viii" (this,i32,i32)->void
    virtual Vec2 getVec() { return Vec2{ m_w, m_w + 1 }; }       // sret => "vii" (this,sret_ptr)->void
    virtual ~Base() {}
};
struct Derived : Base
{
    int  getInt() override { return 13; }
    void setVii( int w ) override { m_w = w + 1; }
    void setViii( int a, int b ) override { m_w = a + b + 1; }
    Vec2 getVec() override { return Vec2{ m_w + 1, m_w + 2 }; }
};

static Base* makeObj( int seed ) __attribute__( ( noinline ) );
static Base* makeObj( int seed )
{
    return ( seed & 1 ) ? static_cast<Base*>( new Derived() ) : new Base();
}
static Base* g_obj = nullptr;

static void plog( const char* s )
{
    EM_ASM( { console.log( UTF8ToString( $0 ) ); }, s );
}

int main()
{
    g_obj = makeObj( 1 );  // Derived via noinline factory => genuine call_indirect
    plog( "[VCALL] start" );

    TestCoroutine co( []( TestCoroutine& self ) {
        char b[96];

        // DECISIVE TEST (agent's): interleave a real asyncify suspend (emscripten_sleep) BEFORE each
        // virtual call — so each call_indirect runs just AFTER a suspend/rewind IN THE FIBER, mirroring
        // the rebaseline snapshot's vii getters running after commit.Push's futex suspend. If the vii
        // calls park after a sleep but viii/ii don't, the bug is the suspend x vii-dispatch interaction.
        emscripten_sleep( 1 );
        plog( "[VCALL] fiber: slept; before getInt (ii)" );
        int i = g_obj->getInt();
        std::snprintf( b, sizeof b, "[VCALL] fiber: getInt=%d (ii OK)", i );
        plog( b );

        emscripten_sleep( 1 );
        plog( "[VCALL] fiber: slept; before setViii (viii)" );
        g_obj->setViii( 10, 20 );
        std::snprintf( b, sizeof b, "[VCALL] fiber: after setViii m_w=%d (viii OK)", g_obj->m_w );
        plog( b );

        emscripten_sleep( 1 );
        plog( "[VCALL] fiber: slept; before getVec (sret -> vii)" );
        Vec2 v = g_obj->getVec();
        std::snprintf( b, sizeof b, "[VCALL] fiber: after getVec=(%d,%d) (sret-vii OK)", v.x, v.y );
        plog( b );

        emscripten_sleep( 1 );
        plog( "[VCALL] fiber: slept; before setVii (vii)" );
        g_obj->setVii( 200000 );
        std::snprintf( b, sizeof b, "[VCALL] fiber: after setVii m_w=%d (vii OK)", g_obj->m_w );
        plog( b );

        self.Yield( 42 );
    } );
    co.Call( 1 );

    plog( "[VCALL] DONE" );
    return 0;
}
