// eh_inspect.cpp — design aid for the catch-arm-hoisting pass (not built into the toy).
// A single isolated function that reproduces the suspend-inside-catch pattern, so its
// legacy-wasm-EH IR (try / catch $cpp_tag / pop / rethrow / catch_all) is readable at -O1.
// eh_sleep_ms is a plain extern import here (structure is identical to the EM_ASYNC_JS one).
#include <stdexcept>
#include <emscripten.h>

extern "C" int eh_sleep_ms( int ms );

extern "C" EMSCRIPTEN_KEEPALIVE __attribute__( ( noinline ) ) int suspend_in_catch_demo()
{
    int slept = -1;

    try
    {
        throw std::runtime_error( "boom" );
    }
    catch( const std::exception& e )
    {
        slept = eh_sleep_ms( 50 );
    }

    return slept;
}

// A local with a non-trivial destructor in the try body — forces an LLVM cleanup
// (catch_all) landing pad alongside the suspending C++ catch.
struct InspectGuard
{
    int* p;
    ~InspectGuard();
};

__attribute__( ( noinline ) ) InspectGuard::~InspectGuard()
{
    if( p )
        *p = 1;
}

extern "C" EMSCRIPTEN_KEEPALIVE __attribute__( ( noinline ) ) int cleanup_catch_demo()
{
    int dtor = 0;
    int slept = -1;

    try
    {
        InspectGuard g{ &dtor };
        throw std::runtime_error( "boom" );
    }
    catch( const std::exception& )
    {
        slept = eh_sleep_ms( 50 );
    }

    return slept + dtor;
}

int main()
{
    return 0;
}
