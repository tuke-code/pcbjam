/*
 * ngspice_service — worker-side entry points wrapping ngspice's sharedspice
 * API for the eeschema simulator (see wasm/ngspice-service/CMakeLists.txt and
 * docs/features/ngspice-split/).
 *
 * RPC surface (embind, called by web/standalone/src/wasm/ngspice-worker.js):
 *   init()                -> int   ngSpice_Init with the service callbacks
 *   circ(lines, files)    -> int   stage .include files, then ngSpice_Circ
 *   command(cmd)          -> int   ngSpice_Command
 *   getVecInfo(name)      -> {found, vname, vtype, flags, length, real, comp}
 *   curPlot()             -> string
 *   allPlots()/allVecs(p) -> string[]
 *   running()             -> bool
 *   cmInputPath(path)     -> void
 *
 * Event stream: sharedspice callbacks fire on ngspice's background pthread
 * during bg_run (and synchronously on this module's main thread during
 * commands). Every callback is forwarded with MAIN_THREAD_ASYNC_EM_ASM to
 * Module.ngspiceEmit — the per-target FIFO of emscripten's proxying queue
 * preserves order, and the main thread is idle while a bg simulation runs, so
 * the queue drains promptly. The worker wrapper batches char/stat lines and
 * postMessages { evt } frames to the editor-side provider.
 *
 * KiCad parity notes: SendData/SendInitData are nullptr exactly like
 * eeschema's NGSPICE::init_dll (vectors are pulled after the run); vector
 * copies happen under ngSpice_LockRealloc so mid-run plot refresh cannot race
 * the growing simulation vectors (this replaces KiCad's client-side lock,
 * which is a no-op across the RPC boundary).
 */

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include <emscripten.h>
#include <emscripten/bind.h>

#include "ngspice/sharedspice.h"

using namespace emscripten;

namespace
{

enum EvtKind
{
    EVT_CHAR = 0, // SendChar console line
    EVT_STAT = 1, // SendStat status line
    EVT_BG = 2,   // BGThreadRunning: a = 1 when finished/not-running
    EVT_EXIT = 3, // ControlledExit: a = status, b = immediate|quit<<1
};

// Safe from any thread. The string is strdup'd and freed by the proxied JS
// after conversion (the pointer must outlive the async hop).
void emitEvt( int aKind, const char* aText, int aA, int aB )
{
    char* copy = aText ? strdup( aText ) : nullptr;

    MAIN_THREAD_ASYNC_EM_ASM(
        {
            var s = $1 ? UTF8ToString( $1 ) : null;
            if( $1 )
                _free( $1 );
            if( Module.ngspiceEmit )
                Module.ngspiceEmit( $0, s, $2, $3 );
        },
        aKind, copy, aA, aB );
}

int cbSendChar( char* aWhat, int, void* )
{
    emitEvt( EVT_CHAR, aWhat, 0, 0 );
    return 0;
}

int cbSendStat( char* aWhat, int, void* )
{
    emitEvt( EVT_STAT, aWhat, 0, 0 );
    return 0;
}

int cbControlledExit( int aStatus, NG_BOOL aImmediate, NG_BOOL aQuit, int, void* )
{
    emitEvt( EVT_EXIT, nullptr, aStatus, ( aImmediate ? 1 : 0 ) | ( aQuit ? 2 : 0 ) );
    return 0;
}

int cbBGThreadRunning( NG_BOOL aNotRunning, int, void* )
{
    emitEvt( EVT_BG, nullptr, aNotRunning ? 1 : 0, 0 );
    return 0;
}

std::atomic<bool> s_inited{ false };

int svcInit()
{
    if( s_inited.load() )
        return 0;

    int ret = ngSpice_Init( cbSendChar, cbSendStat, cbControlledExit,
                            nullptr, nullptr, cbBGThreadRunning, nullptr );

    if( ret == 0 )
        s_inited.store( true );

    return ret;
}

void mkdirRecursive( const std::string& aDir )
{
    std::string partial;

    for( size_t i = 0; i < aDir.size(); i++ )
    {
        partial += aDir[i];

        if( aDir[i] == '/' && partial.size() > 1 )
            ::mkdir( partial.c_str(), 0777 );
    }

    if( !partial.empty() )
        ::mkdir( partial.c_str(), 0777 );
}

// lines: string[]; files: [{ path, text }] — netlist-referenced .include /
// .lib files read out of the editor's MEMFS, staged here at identical
// absolute paths so ngspice's own file access resolves them.
int svcCirc( val aLines, val aFiles )
{
    const unsigned nFiles = aFiles["length"].as<unsigned>();

    for( unsigned i = 0; i < nFiles; i++ )
    {
        val f = aFiles[i];
        std::string path = f["path"].as<std::string>();
        std::string text = f["text"].as<std::string>();

        size_t slash = path.find_last_of( '/' );

        if( slash != std::string::npos && slash > 0 )
            mkdirRecursive( path.substr( 0, slash ) );

        FILE* fp = fopen( path.c_str(), "w" );

        if( !fp )
        {
            fprintf( stderr, "[ngspice_service] cannot stage %s: %s\n",
                     path.c_str(), strerror( errno ) );
            return 1;
        }

        fwrite( text.data(), 1, text.size(), fp );
        fclose( fp );
    }

    const unsigned n = aLines["length"].as<unsigned>();
    std::vector<std::string> strs;
    strs.reserve( n );

    for( unsigned i = 0; i < n; i++ )
        strs.push_back( aLines[i].as<std::string>() );

    // ngSpice_Circ wants a NULL-terminated char* array; it copies the lines.
    std::vector<char*> arr( n + 1 );

    for( unsigned i = 0; i < n; i++ )
        arr[i] = const_cast<char*>( strs[i].c_str() );

    arr[n] = nullptr;

    return ngSpice_Circ( arr.data() );
}

int svcCommand( std::string aCmd )
{
    return ngSpice_Command( const_cast<char*>( aCmd.c_str() ) );
}

val svcGetVecInfo( std::string aName )
{
    val out = val::object();

    // Persistent copy buffers: the returned typed_memory_views stay valid
    // until the next call; the worker structured-clones them into fresh
    // arrays before posting (a SAB-backed view cannot be transferred).
    static std::vector<double> s_realBuf;
    static std::vector<double> s_compBuf;

    ngSpice_LockRealloc();

    pvector_info vi = ngGet_Vec_Info( const_cast<char*>( aName.c_str() ) );

    if( !vi )
    {
        ngSpice_UnlockRealloc();
        out.set( "found", false );
        return out;
    }

    out.set( "found", true );
    out.set( "vname", std::string( vi->v_name ? vi->v_name : "" ) );
    out.set( "vtype", vi->v_type );
    out.set( "flags", (int) vi->v_flags );
    out.set( "length", vi->v_length );

    if( vi->v_realdata && vi->v_length > 0 )
    {
        s_realBuf.assign( vi->v_realdata, vi->v_realdata + vi->v_length );
        out.set( "real", val( typed_memory_view( s_realBuf.size(), s_realBuf.data() ) ) );
    }
    else
    {
        out.set( "real", val::null() );
    }

    if( vi->v_compdata && vi->v_length > 0 )
    {
        s_compBuf.resize( (size_t) vi->v_length * 2 );

        for( int i = 0; i < vi->v_length; i++ )
        {
            s_compBuf[(size_t) i * 2] = vi->v_compdata[i].cx_real;
            s_compBuf[(size_t) i * 2 + 1] = vi->v_compdata[i].cx_imag;
        }

        out.set( "comp", val( typed_memory_view( s_compBuf.size(), s_compBuf.data() ) ) );
    }
    else
    {
        out.set( "comp", val::null() );
    }

    ngSpice_UnlockRealloc();
    return out;
}

std::string svcCurPlot()
{
    char* name = ngSpice_CurPlot();
    return name ? name : "";
}

val svcAllPlots()
{
    val out = val::array();
    char** names = ngSpice_AllPlots();

    for( int i = 0; names && names[i]; i++ )
        out.call<void>( "push", std::string( names[i] ) );

    return out;
}

val svcAllVecs( std::string aPlot )
{
    val out = val::array();
    char** names = ngSpice_AllVecs( const_cast<char*>( aPlot.c_str() ) );

    for( int i = 0; names && names[i]; i++ )
        out.call<void>( "push", std::string( names[i] ) );

    return out;
}

bool svcRunning()
{
    return ngSpice_running();
}

void svcCmInputPath( std::string aPath )
{
    ngCM_Input_Path( aPath.empty() ? nullptr : aPath.c_str() );
}

} // namespace

EMSCRIPTEN_BINDINGS( ngspice_service )
{
    function( "init", &svcInit );
    function( "circ", &svcCirc );
    function( "command", &svcCommand );
    function( "getVecInfo", &svcGetVecInfo );
    function( "curPlot", &svcCurPlot );
    function( "allPlots", &svcAllPlots );
    function( "allVecs", &svcAllVecs );
    function( "running", &svcRunning );
    function( "cmInputPath", &svcCmInputPath );
}

int main()
{
    // ngspice resolves spinit via $SPICE_LIB_DIR/scripts/spinit before the
    // compiled-in NGSPICEDATADIR (whose baked host path is meaningless in
    // MEMFS); the CMake link embeds spinit at this fixed location.
    setenv( "SPICE_LIB_DIR", "/ngspice", 1 );

    fprintf( stderr, "[ngspice_service] ready\n" );

    // EXIT_RUNTIME=0: the module stays alive for embind calls from onmessage.
    return 0;
}
