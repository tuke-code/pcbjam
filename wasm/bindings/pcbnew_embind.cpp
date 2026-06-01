/*
 * Embind bindings for KiCad WASM
 * Exposes core PCBnew objects to JavaScript
 *
 * This provides a foundation for future Pyodide integration.
 *
 * Note: GetBoard() is not available when KICAD_SCRIPTING=OFF.
 * These bindings expose the classes for use when a board reference
 * is obtained through other means (e.g., from the UI).
 */

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <kiway_player.h>
#include <kiway.h>
#include <vector>
#include <wx/app.h>
#include <wx/string.h>
#include <wx/window.h>

using namespace emscripten;

// Programmatically open a project file (board/schematic) in the running editor
// frame, without UI automation. Mirrors single_top.cpp's MacOpenFile path:
// the editor frame is the app's top window and is a KIWAY_PLAYER. Returns the
// result of OpenProjectFiles, or false if no frame is available — letting the
// JS caller fall back to driving File→Open.
bool kicadOpenFile( std::string path )
{
    KIWAY_PLAYER* frame =
            wxTheApp ? static_cast<KIWAY_PLAYER*>( wxTheApp->GetTopWindow() ) : nullptr;

    if( !frame )
        return false;

    if( wxWindow* blocking = frame->Kiway().GetBlockingDialog() )
        blocking->Close( true );

    return frame->OpenProjectFiles(
            std::vector<wxString>( 1, wxString::FromUTF8( path.c_str() ) ) );
}

// Wrapper to return footprints as vector for JS iteration
std::vector<FOOTPRINT*> Board_GetFootprints(BOARD* board) {
    if (!board) return {};
    std::vector<FOOTPRINT*> result;
    for (FOOTPRINT* fp : board->Footprints()) {
        result.push_back(fp);
    }
    return result;
}

// Wrapper to return pads as vector
std::vector<PAD*> Footprint_GetPads(FOOTPRINT* fp) {
    if (!fp) return {};
    std::vector<PAD*> result;
    for (PAD* pad : fp->Pads()) {
        result.push_back(pad);
    }
    return result;
}

// Wrapper for GetFileName since it returns wxString
std::string Board_GetFileName(BOARD* board) {
    if (!board) return "";
    return board->GetFileName().ToStdString();
}

// Wrapper for footprint reference
std::string Footprint_GetReference(FOOTPRINT* fp) {
    if (!fp) return "";
    return fp->GetReference().ToStdString();
}

// Wrapper for footprint value
std::string Footprint_GetValue(FOOTPRINT* fp) {
    if (!fp) return "";
    return fp->GetValue().ToStdString();
}

// Wrapper for pad number
std::string Pad_GetNumber(PAD* pad) {
    if (!pad) return "";
    return pad->GetNumber().ToStdString();
}

// Wrapper for pad pin function
std::string Pad_GetPinFunction(PAD* pad) {
    if (!pad) return "";
    return pad->GetPinFunction().ToStdString();
}

EMSCRIPTEN_BINDINGS(pcbnew) {
    // Register vector types for iteration
    register_vector<FOOTPRINT*>("FootprintVector");
    register_vector<PAD*>("PadVector");

    // Helper functions that operate on pointers
    // Note: GetBoard() not available - will be added when Pyodide integration is done
    function("Board_GetFootprints", &Board_GetFootprints, allow_raw_pointers());
    function("Board_GetFileName", &Board_GetFileName, allow_raw_pointers());
    function("Footprint_GetPads", &Footprint_GetPads, allow_raw_pointers());
    function("Footprint_GetReference", &Footprint_GetReference, allow_raw_pointers());
    function("Footprint_GetValue", &Footprint_GetValue, allow_raw_pointers());
    function("Pad_GetNumber", &Pad_GetNumber, allow_raw_pointers());
    function("Pad_GetPinFunction", &Pad_GetPinFunction, allow_raw_pointers());

    // Programmatic file open (preferred over UI automation from the web app).
    function("kicadOpenFile", &kicadOpenFile);
}
#endif
