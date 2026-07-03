/*
 * gl1_state — the shim's GL 1.x state singleton and capability routing.
 */

#include "gl1_shim.h"

namespace gl1
{

State& S()
{
    static State s;
    return s;
}


bool* ffpCapSlot( GLenum cap )
{
    State& s = S();

    switch( cap )
    {
    case GL_LIGHTING:       return &s.lighting;
    case GL_COLOR_MATERIAL: return &s.colorMaterial;
    case GL_TEXTURE_2D:     return &s.texture2D;
    case GL_NORMALIZE:      return &s.normalizeNormals;
    case GL_ALPHA_TEST:     return &s.alphaTest;
    // Tracked-but-inert: WebGL2 has no equivalent caps and would raise
    // INVALID_ENUM; the suite's goldens are single-sample/aliased anyway.
    case GL_LINE_SMOOTH:    return &s.lineSmooth;
    case GL_POINT_SMOOTH:   return &s.pointSmooth;
    case GL_MULTISAMPLE:    return &s.multisample;
    default:
        if( cap >= GL_LIGHT0 && cap <= GL_LIGHT7 )
            return &s.lightEnabled[cap - GL_LIGHT0];

        return nullptr; // WebGL-native cap: forward
    }
}


void onCapChanged( GLenum cap )
{
    State& s = S();

    switch( cap )
    {
    case GL_LIGHTING:
    case GL_COLOR_MATERIAL:
        s.lightingDirty = true;
        s.miscDirty = true;
        break;

    case GL_TEXTURE_2D:
    case GL_ALPHA_TEST:
        s.miscDirty = true;
        break;

    default:
        if( cap >= GL_LIGHT0 && cap <= GL_LIGHT7 )
            s.lightingDirty = true;
        break;
    }
}

} // namespace gl1
