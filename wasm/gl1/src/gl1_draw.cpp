/*
 * gl1_draw — draw execution: streaming immediate-mode geometry and routed
 * client-array / user-VBO draws, all through the FFP uber-program.
 *
 * VAO policy: default VAO only, all four attributes respecified per draw. The
 * renderer binds its own VBO/IBO on the default VAO (3d_model.cpp), so a shim
 * VAO would hide the user's GL_ELEMENT_ARRAY_BUFFER binding. The shim never
 * binds GL_ELEMENT_ARRAY_BUFFER itself.
 *
 * GL_ARRAY_BUFFER discipline: GL1 code assumes the binding it last set — the
 * shim saves and restores it around its own streaming uploads so a later
 * gl*Pointer call captures the app's binding, not the shim's scratch VBO.
 */

#include "gl1_shim.h"

namespace gl1
{

static GLuint s_streamVBO = 0;

enum
{
    ATTR_POSITION = 0,
    ATTR_NORMAL = 1,
    ATTR_COLOR = 2,
    ATTR_TEXCOORD = 3,
};


void drawImmVertices( GLenum mode, const ImmVertex* verts, GLsizei count )
{
    if( !programSync() )
        return;

    if( !s_streamVBO )
        glGenBuffers( 1, &s_streamVBO );

    GLint prevArrayBuffer = 0;
    glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer );

    glBindBuffer( GL_ARRAY_BUFFER, s_streamVBO );
    glBufferData( GL_ARRAY_BUFFER, (GLsizeiptr) ( count * sizeof( ImmVertex ) ), verts,
                  GL_STREAM_DRAW );

    const GLsizei stride = (GLsizei) sizeof( ImmVertex );

    glEnableVertexAttribArray( ATTR_POSITION );
    glVertexAttribPointer( ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, stride, (const void*) 0 );

    glEnableVertexAttribArray( ATTR_NORMAL );
    glVertexAttribPointer( ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, stride,
                           (const void*) ( 3 * sizeof( float ) ) );

    glEnableVertexAttribArray( ATTR_COLOR );
    glVertexAttribPointer( ATTR_COLOR, 4, GL_FLOAT, GL_FALSE, stride,
                           (const void*) ( 6 * sizeof( float ) ) );

    glDisableVertexAttribArray( ATTR_TEXCOORD );
    glVertexAttrib2f( ATTR_TEXCOORD, 0.0f, 0.0f );

    __real_glDrawArrays( mode, 0, count );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint) prevArrayBuffer );
}


// Routed glDrawArrays over FFP client-array state (M3).
void drawClientArrays( GLenum mode, GLint first, GLsizei count )
{
    (void) mode;
    (void) first;
    (void) count;
    GL1_WARN_ONCE( "client-array glDrawArrays not implemented yet (M3) — draw dropped" );
}


// Routed glDrawElements over user VBO/IBO state (M5).
void drawClientElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices )
{
    (void) mode;
    (void) count;
    (void) type;
    (void) indices;
    GL1_WARN_ONCE( "client-array glDrawElements not implemented yet (M5) — draw dropped" );
}

} // namespace gl1
