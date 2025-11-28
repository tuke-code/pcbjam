// Minimal wxWidgets WASM Test Application
// Purpose: Verify wxWidgets WASM port is working correctly

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

class TestApp : public wxApp
{
public:
    virtual bool OnInit() wxOVERRIDE;
};

class TestFrame : public wxFrame
{
public:
    TestFrame(const wxString& title);
};

wxIMPLEMENT_APP(TestApp);

bool TestApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    TestFrame *frame = new TestFrame("wxWidgets WASM Test");
    frame->Show(true);

    return true;
}

TestFrame::TestFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(640, 480))
{
#if wxUSE_STATUSBAR
    CreateStatusBar();
    SetStatusText("wxWidgets WASM is working!");
#endif
}
