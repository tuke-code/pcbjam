/**
 * Native 3D-renderer test application — golden-baseline generator.
 *
 * Renders the shared scenarios (tests/3d-regression/scenarios/) through the
 * REAL KiCad 3D-viewer OpenGL code on a desktop GL 2.1 compatibility context,
 * captures each into a fixed-size offscreen FBO and writes PNGs. These PNGs
 * are the committed goldens the WebGL port will be compared against with the
 * pixelmatch engine (tests/tools/screenshots/compare-dirs.ts).
 *
 * Modeled on tests/gal-regression/native/gal_native_test.cpp.
 */

#include "kicad_stubs_3d.h" // kiglad before wx

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "fbo_capture.h"
#include "scene3d_test_ctx.h"
#include "scene3d_test_scenarios.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Fixed capture size — must match the WebGL harness canvas (wasm/3d_webgl_test.html)
// and manifest.json. FBO capture makes this independent of window size / Retina.
static const int CAPTURE_WIDTH = 800;
static const int CAPTURE_HEIGHT = 600;

static std::string g_outputDir;
static std::string g_manifestPath;
static std::string g_filter;
static bool        g_showWindow = false;


static bool SavePng( const std::string& aPath, std::vector<uint8_t>& aPixels, int aWidth,
                     int aHeight )
{
    // Force alpha opaque: the browser canvas the WebGL side screenshots is
    // composited opaque, while the FBO keeps partial alpha (same rationale as
    // gal_native_test.cpp SaveScreenshot).
    for( size_t i = 3; i < aPixels.size(); i += 4 )
        aPixels[i] = 255;

    // OpenGL rows are bottom-up.
    stbi_flip_vertically_on_write( 1 );

    return stbi_write_png( aPath.c_str(), aWidth, aHeight, 4, aPixels.data(), aWidth * 4 ) != 0;
}


static bool WriteManifest( const std::string& aPath )
{
    std::ofstream out( aPath );

    if( !out )
    {
        std::cerr << "Failed to write manifest: " << aPath << "\n";
        return false;
    }

    out << "{\n  \"width\": " << CAPTURE_WIDTH << ",\n  \"height\": " << CAPTURE_HEIGHT
        << ",\n  \"scenarios\": [\n";

    for( int i = 0; i < Scene3DTest::GetScenarioCount(); i++ )
    {
        out << "    \"" << Scene3DTest::GetScenarioName( i ) << "\""
            << ( i + 1 < Scene3DTest::GetScenarioCount() ? "," : "" ) << "\n";
    }

    out << "  ]\n}\n";
    return true;
}


class TEST_GL_CANVAS : public wxGLCanvas
{
public:
    explicit TEST_GL_CANVAS( wxWindow* aParent, const wxGLAttributes& aAttrs ) :
            wxGLCanvas( aParent, aAttrs, wxID_ANY, wxDefaultPosition,
                        wxSize( CAPTURE_WIDTH, CAPTURE_HEIGHT ) )
    {
        wxGLContextAttrs ctxAttrs; // default: legacy compatibility profile —
        ctxAttrs.PlatformDefaults().EndList(); // required for immediate mode + display lists
        m_context = new wxGLContext( this, nullptr, &ctxAttrs );
    }

    ~TEST_GL_CANVAS() override { delete m_context; }

    bool MakeCurrent() { return SetCurrent( *m_context ); }

private:
    wxGLContext* m_context;
};


class SCENE3D_TEST_FRAME : public wxFrame
{
public:
    SCENE3D_TEST_FRAME() :
            wxFrame( nullptr, wxID_ANY, "3D Renderer Native Test", wxDefaultPosition,
                     wxSize( CAPTURE_WIDTH, CAPTURE_HEIGHT ) )
    {
        wxGLAttributes attrs;
        attrs.PlatformDefaults().RGBA().DoubleBuffer().Depth( 24 ).Stencil( 8 ).EndList();

        m_canvas = new TEST_GL_CANVAS( this, attrs );

        CallAfter( &SCENE3D_TEST_FRAME::RunScenarios );
    }

    void RunScenarios()
    {
        std::vector<int> toRun;

        for( int i = 0; i < Scene3DTest::GetScenarioCount(); i++ )
        {
            const std::string name = Scene3DTest::GetScenarioName( i );

            if( g_filter.empty() || name.find( g_filter ) != std::string::npos )
                toRun.push_back( i );
        }

        m_total = static_cast<int>( toRun.size() );

        if( !m_canvas->MakeCurrent() )
        {
            std::cerr << "Failed to make GL context current\n";
            Close();
            return;
        }

        const int gladVersion = gladLoaderLoadGL();

        if( !gladVersion )
        {
            std::cerr << "gladLoaderLoadGL failed\n";
            Close();
            return;
        }

        std::cout << "GL_VERSION:  " << (const char*) glGetString( GL_VERSION ) << "\n";
        std::cout << "GL_RENDERER: " << (const char*) glGetString( GL_RENDERER ) << "\n";

        // The FFP renderer needs a compatibility context: display lists must exist.
        if( !glad_glGenLists )
        {
            std::cerr << "glGenLists did not load — not a compatibility context?\n";
            Close();
            return;
        }

        FBO_CAPTURE fbo;

        if( !fbo.Create( CAPTURE_WIDTH, CAPTURE_HEIGHT ) )
        {
            std::cerr << "FBO creation failed\n";
            Close();
            return;
        }

        fs::create_directories( g_outputDir );

        SCENE3D_CTX ctx( CAPTURE_WIDTH, CAPTURE_HEIGHT );
        ctx.InitOnce(); // initializeOpenGL()-equivalent state + circle texture

        std::vector<uint8_t> pixels;

        for( int i : toRun )
        {
            const std::string name = Scene3DTest::GetScenarioName( i );

            std::cout << "Scenario " << i << ": " << name << "... " << std::flush;

            fbo.Bind();
            Scene3DTest::RenderScenario( ctx, i );

            const std::string path = g_outputDir + "/3d-" + name + ".png";

            if( fbo.ReadPixels( pixels ) && SavePng( path, pixels, fbo.Width(), fbo.Height() ) )
            {
                std::cout << "OK\n";
                m_passed++;
            }
            else
            {
                std::cout << "FAILED\n";
            }
        }

        fbo.Destroy();

        if( !g_manifestPath.empty() )
        {
            if( WriteManifest( g_manifestPath ) )
                std::cout << "Manifest: " << g_manifestPath << "\n";
            else
                m_passed = -1;
        }

        std::cout << "\nResults: " << m_passed << "/" << m_total << " scenarios saved\n";

        if( !g_showWindow )
            Close();
    }

    int GetPassed() const { return m_passed; }
    int GetTotal() const { return m_total; }

private:
    TEST_GL_CANVAS* m_canvas;
    int             m_passed = 0;
    int             m_total = 0;
};


class SCENE3D_TEST_APP : public wxApp
{
public:
    bool OnInit() override
    {
        m_frame = new SCENE3D_TEST_FRAME();
        m_frame->Show( true );
        return true;
    }

    int OnExit() override
    {
        if( m_frame )
            return ( m_frame->GetPassed() == m_frame->GetTotal() ) ? 0 : 1;

        return 1;
    }

private:
    SCENE3D_TEST_FRAME* m_frame = nullptr;
};

wxIMPLEMENT_APP_NO_MAIN( SCENE3D_TEST_APP );


int main( int argc, char** argv )
{
    for( int i = 1; i < argc; i++ )
    {
        const std::string arg = argv[i];

        if( arg == "--output" && i + 1 < argc )
        {
            g_outputDir = argv[++i];
        }
        else if( arg == "--manifest" && i + 1 < argc )
        {
            g_manifestPath = argv[++i];
        }
        else if( arg == "--filter" && i + 1 < argc )
        {
            g_filter = argv[++i];
        }
        else if( arg == "--show" )
        {
            g_showWindow = true;
        }
        else if( arg == "--list" )
        {
            for( int s = 0; s < Scene3DTest::GetScenarioCount(); s++ )
                std::cout << s << ": " << Scene3DTest::GetScenarioName( s ) << "\n";

            return 0;
        }
        else
        {
            std::cout << "Usage: scene3d_native_test --output <dir> [options]\n"
                         "  --output <dir>    Output directory for 3d-<name>.png\n"
                         "  --manifest <file> Write the scenario manifest JSON\n"
                         "  --filter <substr> Only run scenarios whose name contains <substr>\n"
                         "  --list            Print scenario names and exit\n"
                         "  --show            Keep the window open after rendering\n";
            return arg == "--help" ? 0 : 2;
        }
    }

    if( g_outputDir.empty() )
    {
        std::cerr << "--output is required (or use --list)\n";
        return 2;
    }

    std::cout << "3D Renderer Native Test - RENDER_3D_OPENGL Baseline Generator\n";
    std::cout << "==============================================================\n\n";

    return wxEntry( argc, argv );
}
