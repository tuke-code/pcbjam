/**
 * Offscreen FBO for deterministic fixed-size capture, mirroring
 * EDA_3D_CANVAS::RenderToFrameBuffer (eda_3d_canvas.cpp:736-772):
 * GL_RGBA8 color texture + GL_DEPTH24_STENCIL8 renderbuffer on
 * GL_DEPTH_STENCIL_ATTACHMENT (the stencil is required by
 * OPENGL_RENDER_LIST::DrawCulled hole cutting).
 *
 * Rendering into an FBO makes the output independent of window size and
 * Retina scaling — pixels are exactly aWidth x aHeight.
 */

#ifndef FBO_CAPTURE_H
#define FBO_CAPTURE_H

#include <kicad_gl/kiglad.h>

#include <cstdint>
#include <vector>

class FBO_CAPTURE
{
public:
    /// Create the FBO; returns false if the framebuffer is incomplete.
    /// Falls back to the EXT entry points if the core ARB ones didn't load
    /// (Apple GL 2.1 exposes both; glad loads by symbol name).
    bool Create( int aWidth, int aHeight );

    void Bind();

    /// glFinish + glReadPixels(GL_RGBA); aOut is resized to w*h*4.
    bool ReadPixels( std::vector<uint8_t>& aOut );

    void Destroy();

    int Width() const { return m_width; }
    int Height() const { return m_height; }

private:
    int    m_width = 0;
    int    m_height = 0;
    GLuint m_fbo = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthStencil = 0;
};

#endif // FBO_CAPTURE_H
