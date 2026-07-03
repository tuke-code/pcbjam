#include "fbo_capture.h"

#include <cstdio>

// Core-vs-EXT selection: Apple's 2.1 context exports the ARB entry points on
// all modern renderers, but keep the EXT fallback the compositor path proves
// works (see plan §7 risk 2). Enum values are shared between core and EXT.
static PFNGLGENFRAMEBUFFERSPROC         s_glGenFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC         s_glBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC    s_glFramebufferTexture2D = nullptr;
static PFNGLGENRENDERBUFFERSPROC        s_glGenRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC        s_glBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC     s_glRenderbufferStorage = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC s_glFramebufferRenderbuffer = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC  s_glCheckFramebufferStatus = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC      s_glDeleteFramebuffers = nullptr;
static PFNGLDELETERENDERBUFFERSPROC     s_glDeleteRenderbuffers = nullptr;

static bool resolveFboEntryPoints()
{
    if( s_glGenFramebuffers )
        return true;

    if( glad_glGenFramebuffers )
    {
        s_glGenFramebuffers = glad_glGenFramebuffers;
        s_glBindFramebuffer = glad_glBindFramebuffer;
        s_glFramebufferTexture2D = glad_glFramebufferTexture2D;
        s_glGenRenderbuffers = glad_glGenRenderbuffers;
        s_glBindRenderbuffer = glad_glBindRenderbuffer;
        s_glRenderbufferStorage = glad_glRenderbufferStorage;
        s_glFramebufferRenderbuffer = glad_glFramebufferRenderbuffer;
        s_glCheckFramebufferStatus = glad_glCheckFramebufferStatus;
        s_glDeleteFramebuffers = glad_glDeleteFramebuffers;
        s_glDeleteRenderbuffers = glad_glDeleteRenderbuffers;
        return true;
    }

    if( glad_glGenFramebuffersEXT )
    {
        std::fprintf( stderr, "[fbo] core FBO entry points missing; using EXT fallback\n" );
        s_glGenFramebuffers = glad_glGenFramebuffersEXT;
        s_glBindFramebuffer = glad_glBindFramebufferEXT;
        s_glFramebufferTexture2D = glad_glFramebufferTexture2DEXT;
        s_glGenRenderbuffers = glad_glGenRenderbuffersEXT;
        s_glBindRenderbuffer = glad_glBindRenderbufferEXT;
        s_glRenderbufferStorage = glad_glRenderbufferStorageEXT;
        s_glFramebufferRenderbuffer = glad_glFramebufferRenderbufferEXT;
        s_glCheckFramebufferStatus = glad_glCheckFramebufferStatusEXT;
        s_glDeleteFramebuffers = glad_glDeleteFramebuffersEXT;
        s_glDeleteRenderbuffers = glad_glDeleteRenderbuffersEXT;
        return true;
    }

    std::fprintf( stderr, "[fbo] no framebuffer object support in this context\n" );
    return false;
}


bool FBO_CAPTURE::Create( int aWidth, int aHeight )
{
    if( !resolveFboEntryPoints() )
        return false;

    m_width = aWidth;
    m_height = aHeight;

    // Same sequence as EDA_3D_CANVAS::RenderToFrameBuffer (eda_3d_canvas.cpp:736-772).
    s_glGenFramebuffers( 1, &m_fbo );
    s_glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );

    glGenTextures( 1, &m_colorTexture );
    glBindTexture( GL_TEXTURE_2D, m_colorTexture );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, aWidth, aHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                  nullptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    s_glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                              m_colorTexture, 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    s_glGenRenderbuffers( 1, &m_depthStencil );
    s_glBindRenderbuffer( GL_RENDERBUFFER, m_depthStencil );
    s_glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, aWidth, aHeight );
    s_glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                 m_depthStencil );

    const GLenum status = s_glCheckFramebufferStatus( GL_FRAMEBUFFER );

    if( status != GL_FRAMEBUFFER_COMPLETE )
    {
        std::fprintf( stderr, "[fbo] framebuffer incomplete: 0x%04X\n", status );
        Destroy();
        return false;
    }

    return true;
}


void FBO_CAPTURE::Bind()
{
    s_glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
    glViewport( 0, 0, m_width, m_height );
}


bool FBO_CAPTURE::ReadPixels( std::vector<uint8_t>& aOut )
{
    if( !m_fbo )
        return false;

    s_glBindFramebuffer( GL_FRAMEBUFFER, m_fbo );
    glFinish();

    aOut.resize( static_cast<size_t>( m_width ) * m_height * 4 );
    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glReadPixels( 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, aOut.data() );

    const GLenum err = glGetError();

    if( err != GL_NO_ERROR )
    {
        std::fprintf( stderr, "[fbo] glReadPixels error: 0x%04X\n", err );
        return false;
    }

    return true;
}


void FBO_CAPTURE::Destroy()
{
    if( m_fbo )
        s_glDeleteFramebuffers( 1, &m_fbo );

    if( m_colorTexture )
        glDeleteTextures( 1, &m_colorTexture );

    if( m_depthStencil )
        s_glDeleteRenderbuffers( 1, &m_depthStencil );

    m_fbo = m_colorTexture = m_depthStencil = 0;
}
