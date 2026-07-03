/*
 * gl1_entry_wrapped — interceptors for the Emscripten-owned GL names the
 * emulator must observe. The ONLY mechanism-aware TU: both link sites pass
 * -Wl,--wrap=<sym> for every name in ../wrapped_symbols.txt, so references
 * land on __wrap_* here and __real_* resolves back to the WebGL JS library.
 * (Fallback if --wrap ever breaks: macro-remap in gal/opengl/kiglew.h — only
 * this file would change.)
 *
 * Draw routing: a glDrawArrays/glDrawElements call belongs to the FFP
 * pipeline iff GL_VERTEX_ARRAY client state is enabled — only GL1 code uses
 * glEnableClientState, while the raytracer blit and the 2D WebGL GAL drive
 * their own GLSL programs with glVertexAttribPointer and never touch client
 * state. Their draws pass through untouched.
 */

#include "gl1_shim.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

using namespace gl1;

extern "C"
{

void __wrap_glEnable( GLenum cap )
{
    if( dlistRecording() )
    {
        dlistRecordEnable( cap, true );
        return;
    }

    if( bool* slot = ffpCapSlot( cap ) )
    {
        if( !*slot )
        {
            *slot = true;
            onCapChanged( cap );
        }

        return;
    }

    __real_glEnable( cap );
}


void __wrap_glDisable( GLenum cap )
{
    if( dlistRecording() )
    {
        dlistRecordEnable( cap, false );
        return;
    }

    if( bool* slot = ffpCapSlot( cap ) )
    {
        if( *slot )
        {
            *slot = false;
            onCapChanged( cap );
        }

        return;
    }

    __real_glDisable( cap );
}


GLboolean __wrap_glIsEnabled( GLenum cap )
{
    // glGet-class queries execute even during display-list recording.
    if( bool* slot = ffpCapSlot( cap ) )
        return *slot ? GL_TRUE : GL_FALSE;

    return __real_glIsEnabled( cap );
}


void __wrap_glGetFloatv( GLenum pname, GLfloat* params )
{
    if( pname == GL_MODELVIEW_MATRIX )
    {
        std::memcpy( params, glm::value_ptr( S().mv.back() ), 16 * sizeof( GLfloat ) );
        return;
    }

    if( pname == GL_PROJECTION_MATRIX )
    {
        std::memcpy( params, glm::value_ptr( S().proj.back() ), 16 * sizeof( GLfloat ) );
        return;
    }

    __real_glGetFloatv( pname, params );
}


void __wrap_glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
    if( dlistRecording() )
    {
        dlistRecordDrawArrays( mode, first, count );
        return;
    }

    if( !S().clientArrays[CA_VERTEX].enabled )
    {
        // Modern-GL consumer (raytracer blit, 2D GAL, the shim itself never
        // reaches here) — pass through untouched.
        __real_glDrawArrays( mode, first, count );
        return;
    }

    drawClientArrays( mode, first, count );
}


void __wrap_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid* indices )
{
    if( dlistRecording() )
    {
        GL1_WARN_ONCE( "glDrawElements inside glNewList is not supported — dropped" );
        return;
    }

    if( !S().clientArrays[CA_VERTEX].enabled )
    {
        __real_glDrawElements( mode, count, type, indices );
        return;
    }

    drawClientElements( mode, count, type, indices );
}


void __wrap_glBindTexture( GLenum target, GLuint texture )
{
    if( dlistRecording() )
    {
        dlistRecordBindTexture( target, texture );
        return;
    }

    if( target == GL_TEXTURE_2D )
        S().boundTexture2D = texture;

    __real_glBindTexture( target, texture );
}


void __wrap_glBlendFunc( GLenum sfactor, GLenum dfactor )
{
    if( dlistRecording() )
    {
        dlistRecordBlendFunc( sfactor, dfactor );
        return;
    }

    __real_glBlendFunc( sfactor, dfactor );
}


void __wrap_glLineWidth( GLfloat width )
{
    if( dlistRecording() )
    {
        dlistRecordLineWidth( width );
        return;
    }

    S().lineWidth = width;
    __real_glLineWidth( width );
}


void __wrap_glHint( GLenum target, GLenum mode )
{
    // Only the two WebGL2-legal hints pass through; the FFP hints
    // (LINE_SMOOTH_HINT, PERSPECTIVE_CORRECTION_HINT...) would raise
    // INVALID_ENUM and are swallowed.
    if( target == GL_GENERATE_MIPMAP_HINT || target == GL_FRAGMENT_SHADER_DERIVATIVE_HINT )
        __real_glHint( target, mode );
}

} // extern "C"
