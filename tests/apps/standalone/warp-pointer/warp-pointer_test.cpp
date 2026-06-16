// wxWindow::WarpPointer must update the cached mouse position (DOM port).
//
// Bug (src/wasm/window.cpp): wxWindowWasm::WarpPointer() is a no-op because the
// browser cannot move the OS pointer. KiCad nudges the cursor with the arrow
// keys by warping the pointer and then reading it back via wxGetMousePosition()
// (WX_VIEW_CONTROLS::SetCursorPosition -> WarpMouseCursor -> WarpPointer, then
// the interactive-move loop reads GetViewControls()->GetMousePosition()). With
// the warp a no-op the cached position never changes, so a selected item never
// follows the arrow keys and snaps to the stale cursor on grab.
//
// The invariant the bug violates: after WarpPointer(x, y), wxGetMousePosition()
// must report the screen-space equivalent of (x, y) — exactly what a real OS
// pointer warp produces on desktop.
//
//   RED  (bug present): wxGetMousePosition() is unchanged by the warp.
//   GREEN (fixed):      wxGetMousePosition() == ClientToScreen({x, y}).

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

static void Report(const char *name, bool pass, const wxString &detail)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({
        var msg = '[REPRO] ' + UTF8ToString($0) + ': ' + ($1 ? 'PASS' : 'FAIL')
                  + ' - ' + UTF8ToString($2);
        if ($1) { console.log(msg); } else { console.error(msg); }
    }, name, pass ? 1 : 0, (const char *)detail.utf8_str());
#endif
}

class ReproFrame : public wxFrame
{
public:
    ReproFrame();

private:
    void RunTest();
};

ReproFrame::ReproFrame()
    : wxFrame(nullptr, wxID_ANY, "wxWindow::WarpPointer repro")
{
    CallAfter(&ReproFrame::RunTest);
}

void ReproFrame::RunTest()
{
    // Two distinct targets so a stale/default cached position cannot accidentally
    // match. Warp to each, then require wxGetMousePosition() to report the
    // screen-space equivalent (the same conversion a real OS warp performs).
    const wxPoint targets[] = { wxPoint(137, 211), wxPoint(56, 92) };

    bool allPass = true;
    wxString detail;

    for (const wxPoint &client : targets)
    {
        WarpPointer(client.x, client.y);

        const wxPoint expected = ClientToScreen(client);
        const wxPoint actual = wxGetMousePosition();
        const bool pass = (actual == expected);
        allPass = allPass && pass;

        detail += wxString::Format("[client=(%d,%d) expected=(%d,%d) actual=(%d,%d) %s] ",
                                   client.x, client.y, expected.x, expected.y,
                                   actual.x, actual.y, pass ? "ok" : "MISMATCH");
    }

    Report("warppointer_updates_position", allPass, detail);
}

class ReproApp : public wxApp
{
public:
    bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        (new ReproFrame())->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(ReproApp);
