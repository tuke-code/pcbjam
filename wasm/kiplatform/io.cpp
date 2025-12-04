/*
 * WASM implementation of kiplatform/io.h
 * Provides file I/O functions for WASM virtual filesystem
 */

#include <kiplatform/io.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <stdio.h>

namespace KIPLATFORM
{
namespace IO
{

FILE* SeqFOpen( const wxString& aPath, const wxString& mode )
{
    // WASM doesn't have special sequential read hints
    // Just use standard fopen
    return fopen( aPath.utf8_str(), mode.utf8_str() );
}

bool DuplicatePermissions( const wxString& aSrc, const wxString& aDest )
{
    // WASM virtual filesystem doesn't have detailed permissions
    return true;
}

bool MakeWriteable( const wxString& aFilePath )
{
    // All files in WASM virtual filesystem are writeable
    return true;
}

bool IsFileHidden( const wxString& aFileName )
{
    // Check for Unix-style hidden files (starting with .)
    wxFileName fn( aFileName );
    wxString name = fn.GetFullName();
    return !name.IsEmpty() && name[0] == '.';
}

void LongPathAdjustment( wxFileName& aFilename )
{
    // No-op on non-Windows platforms
}

} // namespace IO
} // namespace KIPLATFORM
