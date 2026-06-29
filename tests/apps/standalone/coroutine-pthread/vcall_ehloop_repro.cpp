// vcall_ehloop_repro.cpp — step 3, the DECISIVE vii experiment (task #54).
//
// Round-2 disassembly showed the parking SetWidth call_indirect differs from the passing repro in
// exactly one way: it lives inside doApply's deeply nested native-EH try/catch_all (48 try / 47
// catch_all — throwing json accesses with RAII cleanup), inside a LOOP, with an asyncify suspend
// nested in those hoisted catch scopes. The plain repro has none of that. This adds all of it:
//   - a for-LOOP (the changed-items loop),
//   - each iteration in a try{}/catch_all with a NON-trivial RAII destructor (the cleanup pad
//     HoistCppCatches rewrites) and a volatile-guarded throw (so the try isn't optimized away),
//   - an emscripten_sleep() INSIDE the try (suspend nested in the catch_all scope),
//   - then the four virtual calls (ii / viii / sret-vii / vii) inside the try, after the suspend.
//
// If the vii call now PARKS while viii/ii pass, the bug is the asyncify x wasm-EH-hoist interaction
// (NOT the signature). If all four still pass, the cause is the commit.Push futex/currData (FIX D) and
// the vii correlation is a confound.

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
    virtual int  getInt() { return 7; }
    virtual void setVii( int w ) { m_w = w; }
    virtual void setViii( int a, int b ) { m_w = a + b; }
    virtual Vec2 getVec() { return Vec2{ m_w, m_w + 1 }; }
    virtual ~Base() {}
};
struct Derived : Base
{
    int  getInt() override { return 13; }
    void setVii( int w ) override { m_w = w + 1; }
    void setViii( int a, int b ) override { m_w = a + b + 1; }
    Vec2 getVec() override { return Vec2{ m_w + 1, m_w + 2 }; }
};

// Non-trivial destructor => a real catch_all cleanup landing pad (what HoistCppCatches rewrites).
static volatile int g_sink = 0;
struct Raii
{
    int tag;
    ~Raii() { g_sink += tag; }
};

static Base* makeObj( int seed ) __attribute__( ( noinline ) );
static Base* makeObj( int seed ) { return ( seed & 1 ) ? static_cast<Base*>( new Derived() ) : new Base(); }
static Base*        g_obj = nullptr;
static volatile int g_throwAt = 99;  // volatile => the throw path can't be proven dead => try kept
static void         plog( const char* s ) { EM_ASM( { console.log( UTF8ToString( $0 ) ); }, s ); }

int main()
{
    g_obj = makeObj( 1 );
    plog( "[VCALL] start (eh-loop)" );

    TestCoroutine co( []( TestCoroutine& self ) {
        char b[96];
        for( int iter = 0; iter < 2; ++iter )
        {
            try
            {
                Raii guard{ iter + 1 };  // RAII cleanup pad live across everything below

                emscripten_sleep( 1 );   // suspend NESTED inside the try/catch_all scope
                plog( "[VCALL] eh-fiber: slept inside try; before getInt (ii)" );
                int i = g_obj->getInt();
                std::snprintf( b, sizeof b, "[VCALL] eh-fiber: getInt=%d (ii OK)", i ); plog( b );

                plog( "[VCALL] eh-fiber: before setViii (viii)" );
                g_obj->setViii( 10, 20 );
                std::snprintf( b, sizeof b, "[VCALL] eh-fiber: after setViii m_w=%d (viii OK)", g_obj->m_w ); plog( b );

                plog( "[VCALL] eh-fiber: before getVec (sret -> vii)" );
                Vec2 v = g_obj->getVec();
                std::snprintf( b, sizeof b, "[VCALL] eh-fiber: after getVec=(%d,%d) (sret-vii OK)", v.x, v.y ); plog( b );

                plog( "[VCALL] eh-fiber: before setVii (vii)" );
                g_obj->setVii( 200000 );
                std::snprintf( b, sizeof b, "[VCALL] eh-fiber: after setVii m_w=%d (vii OK)", g_obj->m_w ); plog( b );

                if( iter == g_throwAt )
                    throw 1;  // volatile-guarded throwing path keeps the try real
            }
            catch( ... )
            {
                plog( "[VCALL] eh-fiber: caught" );
            }
        }
        self.Yield( 42 );
    } );
    co.Call( 1 );

    plog( "[VCALL] DONE" );
    return 0;
}
