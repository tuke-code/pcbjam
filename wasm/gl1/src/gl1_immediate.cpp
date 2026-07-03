/*
 * gl1_immediate — glBegin/glEnd vertex accumulation and primitive conversion.
 *
 * Also the internal emission path for GLU quadrics and display-list replay:
 * every piece of shim geometry funnels through immBegin/immVertex/immEnd so
 * primitive conversion and the draw pipeline live in exactly one place.
 *
 * WebGL2 has no GL_QUADS / GL_QUAD_STRIP / GL_LINE_LOOP / GL_POLYGON:
 *   QUADS      -> GL_TRIANGLES, each quad (0,1,2)(0,2,3)
 *   QUAD_STRIP -> GL_TRIANGLE_STRIP (same vertex order covers the same area)
 *   LINE_LOOP  -> GL_LINE_STRIP with the first vertex appended
 *   POLYGON    -> unused by the renderer (assert-logged, drawn as a fan)
 */

#include "gl1_shim.h"

namespace gl1
{

static std::vector<ImmVertex> s_verts;
static GLenum                 s_mode = 0;
static bool                   s_active = false;


bool immActive()
{
    return s_active;
}


void immBegin( GLenum mode )
{
    if( s_active )
    {
        GL1_WARN_ONCE( "glBegin inside glBegin/glEnd — call ignored" );
        return;
    }

    s_mode = mode;
    s_active = true;
    s_verts.clear();

    if( mode == GL_POLYGON )
        GL1_WARN_ONCE( "GL_POLYGON is not exercised by the renderer; drawing as a triangle fan" );
}


void immVertex( float x, float y, float z )
{
    if( !s_active )
    {
        GL1_WARN_ONCE( "glVertex outside glBegin/glEnd — ignored" );
        return;
    }

    const State& s = S();

    ImmVertex v;
    v.px = x;
    v.py = y;
    v.pz = z;
    v.nx = s.currentNormal.x;
    v.ny = s.currentNormal.y;
    v.nz = s.currentNormal.z;
    v.r = s.currentColor.r;
    v.g = s.currentColor.g;
    v.b = s.currentColor.b;
    v.a = s.currentColor.a;
    v.u = 0.0f;
    v.v = 0.0f;

    s_verts.push_back( v );
}


// Expands GL_QUADS into GL_TRIANGLES in place ((0,1,2)(0,2,3) per quad —
// preserves winding; any trailing partial quad is dropped, as in GL).
static void expandQuads( std::vector<ImmVertex>& verts )
{
    const size_t quadCount = verts.size() / 4;
    std::vector<ImmVertex> tris;
    tris.reserve( quadCount * 6 );

    for( size_t q = 0; q < quadCount; ++q )
    {
        const ImmVertex* v = &verts[q * 4];

        tris.push_back( v[0] );
        tris.push_back( v[1] );
        tris.push_back( v[2] );

        tris.push_back( v[0] );
        tris.push_back( v[2] );
        tris.push_back( v[3] );
    }

    verts.swap( tris );
}


void immEnd()
{
    if( !s_active )
    {
        GL1_WARN_ONCE( "glEnd without glBegin — ignored" );
        return;
    }

    s_active = false;

    if( s_verts.empty() )
        return;

    GLenum drawMode = s_mode;

    switch( s_mode )
    {
    case GL_QUADS:
        expandQuads( s_verts );
        drawMode = GL_TRIANGLES;
        break;

    case GL_QUAD_STRIP:
        drawMode = GL_TRIANGLE_STRIP;
        break;

    case GL_LINE_LOOP:
        s_verts.push_back( s_verts.front() );
        drawMode = GL_LINE_STRIP;
        break;

    case GL_POLYGON:
        drawMode = GL_TRIANGLE_FAN;
        break;

    case GL_POINTS:
    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
        break;

    default:
        GL1_WARN_ONCE( "glBegin: unsupported primitive 0x%x", s_mode );
        return;
    }

    drawImmVertices( drawMode, s_verts.data(), (GLsizei) s_verts.size() );
}

} // namespace gl1
