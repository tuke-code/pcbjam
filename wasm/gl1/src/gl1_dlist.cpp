/*
 * gl1_dlist — display-list name allocation, recording and replay.
 *
 * Semantics the renderer depends on:
 *   - glGenLists creates EXISTING (empty) lists: KiCad's pattern is
 *     `id = glGenLists(1); if( glIsList(id) ) { glNewList(...); }`
 *     (layer_triangles.cpp, render_3d_opengl.cpp:1327) — glIsList must be
 *     true right after allocation or nothing ever renders.
 *   - Only GL_COMPILE recording exists (COMPILE_AND_EXECUTE is asserted).
 *   - M3 adds the command recorder (state + geometry + glDrawArrays with
 *     client-array snapshots); until then recorded commands are dropped,
 *     which keeps the pre-M3 red scenarios blank instead of crashing.
 */

#include "gl1_shim.h"

#include <map>

namespace gl1
{

struct DList
{
    // M3: recorded command stream + baked static VBO.
    bool empty = true;
};

static std::map<GLuint, DList> s_lists;
static GLuint                  s_nextId = 1;
static bool                    s_recording = false;
static GLuint                  s_recordingId = 0;


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
        GL1_WARN_ONCE( "glNewList while already recording — previous list discarded" );
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

    // M3: replay the recorded command stream.
}


void dlistDeleteLists( GLuint list, GLsizei range )
{
    for( GLsizei i = 0; i < range; ++i )
        s_lists.erase( list + (GLuint) i );
}


// --- recording hooks (M3 replaces these drops with the command recorder) ---

void dlistRecordEnable( GLenum cap, bool enable )
{
    (void) cap;
    (void) enable;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordBindTexture( GLenum target, GLuint texture )
{
    (void) target;
    (void) texture;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordBlendFunc( GLenum sfactor, GLenum dfactor )
{
    (void) sfactor;
    (void) dfactor;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordLineWidth( GLfloat width )
{
    (void) width;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordAlphaFunc( GLenum func, GLclampf ref )
{
    (void) func;
    (void) ref;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordNormal( float nx, float ny, float nz )
{
    (void) nx;
    (void) ny;
    (void) nz;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordColor( float r, float g, float b, float a )
{
    (void) r;
    (void) g;
    (void) b;
    (void) a;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordBegin( GLenum mode )
{
    (void) mode;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}


void dlistRecordVertex( float x, float y, float z )
{
    (void) x;
    (void) y;
    (void) z;
}


void dlistRecordEnd()
{
}


void dlistRecordDrawArrays( GLenum mode, GLint first, GLsizei count )
{
    (void) mode;
    (void) first;
    (void) count;
    GL1_WARN_ONCE( "display-list recorder not implemented yet (M3) — commands dropped" );
}

} // namespace gl1
