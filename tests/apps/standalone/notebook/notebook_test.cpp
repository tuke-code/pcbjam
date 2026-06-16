// wxNotebook page-relayout Test - regression coverage for the WASM DOM-port
// fix in wxwidgets/src/wasm/notebook.cpp (wxNotebook::WasmRelayoutSelectedPage).
//
// Bug (pcbjam#8): KiCad's APPEARANCE_CONTROLS::OnNotebookPageChanged calls
// Fit() on the just-selected page from a wxEVT_NOTEBOOK_PAGE_CHANGED handler.
// That collapses a wxScrolledWindow child's viewport to zero height; because
// DoSetSize() only re-runs layout when the size actually changes, resizing the
// page back was a no-op and the scrolled rows stayed clip-pathed away ("pages
// came back blank" after switching tabs away and back).
//
// This app reproduces the same shape in pure wxWidgets: a wxNotebook whose
// "Scrolled" page wraps a wxScrolledWindow full of labelled rows, plus a
// PAGE_CHANGED handler that calls page->Fit() exactly like KiCad. With the fix,
// OnDomEvent re-asserts the page geometry after SetSelection() so the rows stay
// painted; without it, they collapse. The rows are wxStaticText so the DOM port
// renders them as <span> elements the e2e spec can hit-test.

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/notebook.h"
#include "wx/scrolwin.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

static const int kRowCount = 30;

class NotebookApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

class NotebookFrame : public wxFrame
{
public:
    NotebookFrame();

private:
    wxNotebook* m_notebook = nullptr;

    wxWindow* CreatePlainPage();
    wxWindow* CreateScrolledPage();

    void Log(const wxString& msg);
    void OnPageChanged(wxNotebookEvent& evt);
};

bool NotebookApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    (new NotebookFrame())->Show(true);
    return true;
}

wxIMPLEMENT_APP(NotebookApp);

NotebookFrame::NotebookFrame()
    : wxFrame(nullptr, wxID_ANY, "wxNotebook Page Relayout Test",
              wxDefaultPosition, wxSize(420, 480))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->AddPage(CreatePlainPage(), "Plain", true);
    m_notebook->AddPage(CreateScrolledPage(), "Scrolled", false);

    sizer->Add(m_notebook, 1, wxEXPAND | wxALL, 6);
    SetSizer(sizer);

    // Reproduce KiCad's APPEARANCE_CONTROLS::OnNotebookPageChanged: call Fit()
    // on the selected page from the PAGE_CHANGED handler. This is what collapses
    // the scrolled child; the DOM-port fix repairs it after this handler runs.
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &NotebookFrame::OnPageChanged, this);

    Log("notebook test app started");
}

wxWindow* NotebookFrame::CreatePlainPage()
{
    wxPanel* panel = new wxPanel(m_notebook);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Plain page"), 0, wxALL, 10);
    sizer->Add(new wxStaticText(panel, wxID_ANY,
                                "Switch to the Scrolled page and back."),
               0, wxLEFT | wxBOTTOM, 10);
    panel->SetSizer(sizer);
    return panel;
}

wxWindow* NotebookFrame::CreateScrolledPage()
{
    // panel -> wxScrolledWindow -> many rows. Mirrors KiCad's layer panel:
    // the page is a wxPanel that hosts a scrolled list; Fit() on the panel is
    // what drives the viewport to zero.
    wxPanel* panel = new wxPanel(m_notebook);

    wxScrolledWindow* scrolled =
        new wxScrolledWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxVSCROLL | wxBORDER_SIMPLE);
    scrolled->SetScrollRate(0, 10);

    wxBoxSizer* rows = new wxBoxSizer(wxVERTICAL);
    for ( int i = 0; i < kRowCount; ++i )
        rows->Add(new wxStaticText(scrolled, wxID_ANY,
                                   wxString::Format("Row %d", i)),
                  0, wxALL, 4);
    scrolled->SetSizer(rows);
    scrolled->FitInside();

    wxBoxSizer* pageSizer = new wxBoxSizer(wxVERTICAL);
    pageSizer->Add(scrolled, 1, wxEXPAND | wxALL, 4);
    panel->SetSizer(pageSizer);
    return panel;
}

void NotebookFrame::Log(const wxString& msg)
{
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('[NOTEBOOK_EVENT] ' + UTF8ToString($0));
    }, (const char*)msg.utf8_str());
#else
    wxLogMessage("[NOTEBOOK_EVENT] %s", msg);
#endif
}

void NotebookFrame::OnPageChanged(wxNotebookEvent& evt)
{
    const int sel = evt.GetSelection();
    if ( sel != wxNOT_FOUND )
    {
        if ( wxWindow* const page = m_notebook->GetPage(sel) )
            page->Fit();

        Log(wxString::Format("switched to page %d (%s)",
                             sel, m_notebook->GetPageText(sel)));
    }
    evt.Skip();
}
