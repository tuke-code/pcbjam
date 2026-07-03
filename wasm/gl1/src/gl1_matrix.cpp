/*
 * gl1_matrix — GL_MODELVIEW / GL_PROJECTION matrix stacks.
 *
 * Only the operations the KiCad 3D renderer uses exist (no glOrtho/glFrustum/
 * glMultMatrix — projection and view arrive prebuilt via glLoadMatrixf, and
 * gluPerspective covers the gizmo). glGetFloatv(GL_MODELVIEW_MATRIX/
 * GL_PROJECTION_MATRIX) readback is served from these stacks by the
 * __wrap_glGetFloatv interceptor.
 */

#include "gl1_shim.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace gl1
{

// GL 1.5 minimums; KiCad never goes deeper than a few levels.
static constexpr size_t MV_STACK_MAX = 64;
static constexpr size_t PROJ_STACK_MAX = 8;

void matrixLoadIdentity()
{
    S().currentTop() = glm::mat4( 1.0f );
    S().matricesDirty = true;
}


void matrixLoadf( const GLfloat* m )
{
    S().currentTop() = glm::make_mat4( m );
    S().matricesDirty = true;
}


void matrixPush()
{
    State& s = S();
    auto&  stack = s.currentStack();

    const size_t maxDepth = ( s.matrixMode == GL_PROJECTION ) ? PROJ_STACK_MAX : MV_STACK_MAX;

    if( stack.size() >= maxDepth )
    {
        GL1_WARN_ONCE( "glPushMatrix: stack overflow (mode 0x%x)", s.matrixMode );
        return;
    }

    stack.push_back( stack.back() );
}


void matrixPop()
{
    State& s = S();
    auto&  stack = s.currentStack();

    if( stack.size() <= 1 )
    {
        GL1_WARN_ONCE( "glPopMatrix: stack underflow (mode 0x%x)", s.matrixMode );
        return;
    }

    stack.pop_back();
    s.matricesDirty = true;
}


void matrixTranslate( float x, float y, float z )
{
    glm::mat4& top = S().currentTop();
    top = glm::translate( top, glm::vec3( x, y, z ) );
    S().matricesDirty = true;
}


void matrixRotate( float angleDeg, float x, float y, float z )
{
    glm::mat4& top = S().currentTop();
    top = glm::rotate( top, glm::radians( angleDeg ), glm::vec3( x, y, z ) );
    S().matricesDirty = true;
}


void matrixScale( float x, float y, float z )
{
    glm::mat4& top = S().currentTop();
    top = glm::scale( top, glm::vec3( x, y, z ) );
    S().matricesDirty = true;
}


void matrixPerspective( double fovyDeg, double aspect, double zNear, double zFar )
{
    // gluPerspective multiplies onto the current matrix (KiCad calls it right
    // after glLoadIdentity on GL_PROJECTION, but multiply is the GLU semantic).
    glm::mat4 p = glm::perspective( glm::radians( (float) fovyDeg ), (float) aspect,
                                    (float) zNear, (float) zFar );

    glm::mat4& top = S().currentTop();
    top = top * p;
    S().matricesDirty = true;
}

} // namespace gl1
