/*
 * WASM implementation of kiplatform/environment.h
 * Provides environment and path functions for browser environment
 */

#include <kiplatform/environment.h>
#include <wx/string.h>
#include <wx/window.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace KIPLATFORM
{
namespace ENV
{

void Init()
{
    // No special initialization needed for WASM
}

bool MoveToTrash( const wxString& aPath, wxString& aError )
{
    // No trash/recycle bin in browser - just delete
    aError = wxT( "Trash not available in browser environment" );
    return false;
}

bool IsNetworkPath( const wxString& aPath )
{
    // All paths in WASM virtual filesystem are local
    return false;
}

wxString GetDocumentsPath()
{
    // Use virtual filesystem path
    return wxT( "/home/kicad/documents" );
}

wxString GetUserConfigPath()
{
    // Use virtual filesystem path for config
    return wxT( "/home/kicad/.config/kicad" );
}

wxString GetUserDataPath()
{
    // Use virtual filesystem path for data
    return wxT( "/home/kicad/.local/share/kicad" );
}

wxString GetUserLocalDataPath()
{
    // Same as data path in WASM
    return wxT( "/home/kicad/.local/share/kicad" );
}

wxString GetUserCachePath()
{
    // Use virtual filesystem path for cache
    return wxT( "/home/kicad/.cache/kicad" );
}

bool GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg )
{
    // No proxy configuration in browser - browser handles networking
    return false;
}

bool VerifyFileSignature( const wxString& aPath )
{
    // No code signing verification in WASM
    return true;
}

wxString GetAppUserModelId()
{
    // Windows-specific, return empty
    return wxEmptyString;
}

void SetAppDetailsForWindow( wxWindow* aWindow, const wxString& aRelaunchCommand,
                             const wxString& aRelaunchDisplayName )
{
    // Windows-specific, no-op
}

wxString GetCommandLineStr()
{
    // No command line in browser
    return wxEmptyString;
}

void AddToRecentDocs( const wxString& aPath )
{
    // Could implement via localStorage if needed
#ifdef __EMSCRIPTEN__
    EM_ASM({
        try {
            var recent = JSON.parse(localStorage.getItem('kicad_recent_docs') || '[]');
            var path = UTF8ToString($0);
            // Remove if already exists
            recent = recent.filter(function(p) { return p !== path; });
            // Add to front
            recent.unshift(path);
            // Keep only last 10
            recent = recent.slice(0, 10);
            localStorage.setItem('kicad_recent_docs', JSON.stringify(recent));
        } catch(e) {
            console.warn('Failed to save recent docs:', e);
        }
    }, aPath.utf8_str().data());
#endif
}

} // namespace ENV
} // namespace KIPLATFORM
