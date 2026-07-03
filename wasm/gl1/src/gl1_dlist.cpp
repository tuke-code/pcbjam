/*
 * gl1_dlist — display-list name allocation, recording and replay.
 *
 * Semantics the renderer depends on:
 *   - glGenLists creates EXISTING (empty) lists: KiCad's pattern is
 *     `id = glGenLists(1); if( glIsList(id) ) { glNewList(...); }`
 *     (layer_triangles.cpp, render_3d_opengl.cpp:1327) — glIsList must be
 *     true right after allocation or nothing ever renders.
 *   - Only GL_COMPILE recording exists (COMPILE_AND_EXECUTE is asserted).
 *   - THE invariant (do not weaken): a recorded glDrawArrays dereferences the
 *     client arrays AT COMPILE TIME. The renderer frees them right after
 *     glEndList (layer_triangles.cpp seg-ends uvArray), so every enabled
 *     client-memory array is snapshotted eagerly here. VBO-backed pointers
 *     record the buffer name + offset instead (no copy).
 *
 * Replay is a literal command replay through the same shim state machine and
 * draw pipeline the live calls use — state mutations inside a list correctly
 * leak into post-glCallList state (GL semantics), and materials changed
 * BETWEEN glCallList calls (setLayerMaterial -> DrawAll) are honored because
 * draws always read the live uniform state.
 */

#include "gl1_shim.h"

#include <array>
#include <map>
#include <memory>

namespace gl1
{

// Eager copy of one client array's data for a recorded draw (or a reference
// into a user VBO when the pointer was VBO-backed at record time).
struct SnapArray
{
    bool                 enabled = false;
    GLint                size = 4;
    GLenum               type = GL_FLOAT;
    GLsizei              stride = 0; // effective byte stride
    std::vector<uint8_t> data;       // client-memory snapshot (empty if VBO)
    GLuint               buffer = 0;
    GLintptr             offset = 0;
};

struct Cmd
{
    enum class Kind : uint8_t
    {
        ENABLE,      // e0 = cap, i0 = on/off
        BIND_TEXTURE,// e0 = target, u0 = texture
        BLEND_FUNC,  // e0 = sfactor, e1 = dfactor
        LINE_WIDTH,  // f[0]
        ALPHA_FUNC,  // e0 = func, f[0] = ref
        NORMAL,      // f[0..2]
        COLOR,       // f[0..3]
        BEGIN,       // e0 = mode
        VERTEX,      // f[0..2]
        END,
        DRAW_ARRAYS, // e0 = mode, i0 = count, snap
    };

    Kind    kind;
    GLenum  e0 = 0;
    GLenum  e1 = 0;
    GLuint  u0 = 0;
    GLint   i0 = 0;
    float   f[4] = {};

    std::shared_ptr<std::array<SnapArray, CA_COUNT>> snap;
};

struct DList
{
    std::vector<Cmd> cmds;
};

static std::map<GLuint, DList> s_lists;
static GLuint                  s_nextId = 1;
static bool                    s_recording = false;
static GLuint                  s_recordingId = 0;


static std::vector<Cmd>* recCmds()
{
    auto it = s_lists.find( s_recordingId );
    return it != s_lists.end() ? &it->second.cmds : nullptr;
}


bool dlistRecording()
{
    return s_recording;
}


GLuint dlistGenLists( GLsizei range )
{
    if( range <= 0 )
        return 0;

    const GLuint first = s_nextId;

    for( GLsizei i = 0; i < range; ++i )
        s_lists[s_nextId++] = DList{};

    return first;
}


GLboolean dlistIsList( GLuint list )
{
    return s_lists.count( list ) ? GL_TRUE : GL_FALSE;
}


void dlistNewList( GLuint list, GLenum mode )
{
    if( list == 0 )
    {
        GL1_WARN_ONCE( "glNewList(0) is invalid — ignored" );
        return;
    }

    if( mode != GL_COMPILE )
        GL1_WARN_ONCE( "glNewList: only GL_COMPILE is supported (got 0x%x)", mode );

    if( s_recording )
    {
        GL1_WARN_ONCE( "glNewList while already recording — previous list kept as-is" );
        dlistEndList();
    }

    s_lists[list] = DList{}; // re-recording replaces the old content
    s_recording = true;
    s_recordingId = list;
}


void dlistEndList()
{
    if( !s_recording )
    {
        GL1_WARN_ONCE( "glEndList without glNewList — ignored" );
        return;
    }

    s_recording = false;
    s_recordingId = 0;
}


void dlistCallList( GLuint list )
{
    auto it = s_lists.find( list );

    if( it == s_lists.end() )
        return; // calling a nonexistent list is a silent no-op in GL

    State& s = S();

    for( const Cmd& c : it->second.cmds )
    {
        switch( c.kind )
        {
        case Cmd::Kind::ENABLE:
            stateEnable( c.e0, c.i0 != 0 );
            break;

        case Cmd::Kind::BIND_TEXTURE:
            stateBindTexture( c.e0, c.u0 );
            break;

        case Cmd::Kind::BLEND_FUNC:
            stateBlendFunc( c.e0, c.e1 );
            break;

        case Cmd::Kind::LINE_WIDTH:
            stateLineWidth( c.f[0] );
            break;

        case Cmd::Kind::ALPHA_FUNC:
            stateAlphaFunc( c.e0, c.f[0] );
            break;

        case Cmd::Kind::NORMAL:
            s.currentNormal = glm::vec3( c.f[0], c.f[1], c.f[2] );
            break;

        case Cmd::Kind::COLOR:
            s.currentColor = glm::vec4( c.f[0], c.f[1], c.f[2], c.f[3] );
            break;

        case Cmd::Kind::BEGIN:
            immBegin( c.e0 );
            break;

        case Cmd::Kind::VERTEX:
            immVertex( c.f[0], c.f[1], c.f[2] );
            break;

        case Cmd::Kind::END:
            immEnd();
            break;

        case Cmd::Kind::DRAW_ARRAYS:
        {
            AttribSource src[CA_COUNT];

            for( int i = 0; i < CA_COUNT; ++i )
            {
                const SnapArray& sa = ( *c.snap )[i];
                src[i].enabled = sa.enabled;

                if( !sa.enabled )
                    continue;

                src[i].size = sa.size;
                src[i].type = sa.type;
                src[i].normalized = attribNormalized( i, sa.type );
                src[i].stride = sa.stride;

                if( !sa.data.empty() )
                {
                    src[i].cpuData = sa.data.data();
                }
                else
                {
                    src[i].buffer = sa.buffer;
                    src[i].offset = sa.offset;
                }
            }

            drawArraysWithSources( c.e0, c.i0, src );
            break;
        }
        }
    }
}


void dlistDeleteLists( GLuint list, GLsizei range )
{
    for( GLsizei i = 0; i < range; ++i )
        s_lists.erase( list + (GLuint) i );
}


// --- recording hooks ---

static void push( Cmd&& c )
{
    if( std::vector<Cmd>* cmds = recCmds() )
        cmds->push_back( std::move( c ) );
}


void dlistRecordEnable( GLenum cap, bool enable )
{
    Cmd c{ Cmd::Kind::ENABLE };
    c.e0 = cap;
    c.i0 = enable ? 1 : 0;
    push( std::move( c ) );
}


void dlistRecordBindTexture( GLenum target, GLuint texture )
{
    Cmd c{ Cmd::Kind::BIND_TEXTURE };
    c.e0 = target;
    c.u0 = texture;
    push( std::move( c ) );
}


void dlistRecordBlendFunc( GLenum sfactor, GLenum dfactor )
{
    Cmd c{ Cmd::Kind::BLEND_FUNC };
    c.e0 = sfactor;
    c.e1 = dfactor;
    push( std::move( c ) );
}


void dlistRecordLineWidth( GLfloat width )
{
    Cmd c{ Cmd::Kind::LINE_WIDTH };
    c.f[0] = width;
    push( std::move( c ) );
}


void dlistRecordAlphaFunc( GLenum func, GLclampf ref )
{
    Cmd c{ Cmd::Kind::ALPHA_FUNC };
    c.e0 = func;
    c.f[0] = ref;
    push( std::move( c ) );
}


void dlistRecordNormal( float nx, float ny, float nz )
{
    Cmd c{ Cmd::Kind::NORMAL };
    c.f[0] = nx;
    c.f[1] = ny;
    c.f[2] = nz;
    push( std::move( c ) );
}


void dlistRecordColor( float r, float g, float b, float a )
{
    Cmd c{ Cmd::Kind::COLOR };
    c.f[0] = r;
    c.f[1] = g;
    c.f[2] = b;
    c.f[3] = a;
    push( std::move( c ) );
}


void dlistRecordBegin( GLenum mode )
{
    Cmd c{ Cmd::Kind::BEGIN };
    c.e0 = mode;
    push( std::move( c ) );
}


void dlistRecordVertex( float x, float y, float z )
{
    Cmd c{ Cmd::Kind::VERTEX };
    c.f[0] = x;
    c.f[1] = y;
    c.f[2] = z;
    push( std::move( c ) );
}


void dlistRecordEnd()
{
    push( Cmd{ Cmd::Kind::END } );
}


void dlistRecordDrawArrays( GLenum mode, GLint first, GLsizei count )
{
    if( count <= 0 )
        return;

    Cmd c{ Cmd::Kind::DRAW_ARRAYS };
    c.e0 = mode;
    c.i0 = count;
    c.snap = std::make_shared<std::array<SnapArray, CA_COUNT>>();

    const State& s = S();

    for( int i = 0; i < CA_COUNT; ++i )
    {
        const ClientArray& ca = s.clientArrays[i];
        SnapArray&         sa = ( *c.snap )[i];

        sa.enabled = ca.enabled;

        if( !ca.enabled )
            continue;

        sa.size = ca.size;
        sa.type = ca.type;
        sa.stride = attribEffectiveStride( ca.size, ca.type, ca.stride );

        if( ca.boundBuffer == 0 )
        {
            // EAGER copy — the caller may (and does) free this memory right
            // after glEndList. Whole strided block, last vertex tight-sized.
            const GLsizei tight = attribEffectiveStride( ca.size, ca.type, 0 );
            const size_t  bytes = (size_t) ( count - 1 ) * sa.stride + tight;
            const auto*   base = (const uint8_t*) ca.pointer + (size_t) first * sa.stride;

            sa.data.assign( base, base + bytes );
        }
        else
        {
            sa.buffer = ca.boundBuffer;
            sa.offset = (GLintptr) ca.pointer + (GLintptr) first * sa.stride;
        }
    }

    push( std::move( c ) );
}

} // namespace gl1
