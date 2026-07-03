/*
 * gl1_glu — GLU quadrics and gluPerspective.
 *
 * M4 ports the SGI GLU reference tessellation (quad.c) for
 * gluCylinder/gluDisk/gluSphere — the native goldens were rendered with
 * Apple's SGI-derived GLU, so vertex placement and emission order must match.
 * The quadrics emit through the shim's internal immediate-mode path
 * (immBegin/immVertex/immEnd) so they also record into display lists.
 *
 * Note: the GLU *tesselator* (gluNewTess & co, declared in the same
 * wasm/stubs/GL/glu.h) is a separate concern implemented in
 * kicad/libs/kimath/glu_tess/ — not part of this shim.
 */

#include "gl1_shim.h"

extern "C"
{

// Real (heap-allocated) quadric state; only the modes the renderer uses are
// honored — GLU_FILL draw style, GLU_SMOOTH normals, GLU_OUTSIDE orientation.
struct GLUquadric
{
    GLenum drawStyle;
    GLenum normals;
};


GLUquadric* gluNewQuadric( void )
{
    GLUquadric* q = new GLUquadric;
    q->drawStyle = GLU_FILL;
    q->normals = GLU_SMOOTH;
    return q;
}


void gluDeleteQuadric( GLUquadric* q )
{
    delete q;
}


void gluQuadricDrawStyle( GLUquadric* q, GLenum style )
{
    if( !q )
        return;

    if( style != GLU_FILL )
        GL1_WARN_ONCE( "gluQuadricDrawStyle: only GLU_FILL is supported (got 0x%x)", style );

    q->drawStyle = style;
}


void gluQuadricNormals( GLUquadric* q, GLenum normals )
{
    if( !q )
        return;

    if( normals != GLU_SMOOTH )
        GL1_WARN_ONCE( "gluQuadricNormals: only GLU_SMOOTH is supported (got 0x%x)", normals );

    q->normals = normals;
}


void gluCylinder( GLUquadric* q, double base, double top, double height, int slices, int stacks )
{
    (void) q;
    (void) base;
    (void) top;
    (void) height;
    (void) slices;
    (void) stacks;
    GL1_WARN_ONCE( "gluCylinder not implemented yet (M4) — geometry dropped" );
}


void gluDisk( GLUquadric* q, double inner, double outer, int slices, int loops )
{
    (void) q;
    (void) inner;
    (void) outer;
    (void) slices;
    (void) loops;
    GL1_WARN_ONCE( "gluDisk not implemented yet (M4) — geometry dropped" );
}


void gluSphere( GLUquadric* q, double radius, int slices, int stacks )
{
    (void) q;
    (void) radius;
    (void) slices;
    (void) stacks;
    GL1_WARN_ONCE( "gluSphere not implemented yet (M4) — geometry dropped" );
}


void gluPerspective( double fovy, double aspect, double zNear, double zFar )
{
    gl1::matrixPerspective( fovy, aspect, zNear, zFar );
}

} // extern "C"
