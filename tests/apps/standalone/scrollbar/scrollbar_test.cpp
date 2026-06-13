// Scrollbar Test - draggable scrollbars in the WASM DOM port. Exercises BOTH
// the standalone wxScrollBar control AND a wxScrolledWindow's built-in gutters
// (the kind KiCad's scrolled panels use).

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/scrolbar.h"
#include "wx/scrolwin.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class ScrollApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

class ScrollFrame : public wxFrame
{
public:
    ScrollFrame();

private:
    wxScrollBar* m_vbar = nullptr;
    wxScrollBar* m_hbar = nullptr;
    wxScrolledWindow* m_scrolled = nullptr;
    wxStaticText* m_status = nullptr;

    void Log(const wxString& msg);
    void OnBarScroll(wxScrollEvent& evt);
};

wxIMPLEMENT_APP(ScrollApp);

bool ScrollApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    (new ScrollFrame())->Show(true);
    return true;
}

ScrollFrame::ScrollFrame()
    : wxFrame(nullptr, wxID_ANY, "Scrollbar WASM Test",
              wxDefaultPosition, wxSize(700, 560))
{
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    m_status = new wxStaticText(this, wxID_ANY, "Drag a scrollbar");
    root->Add(m_status, 0, wxALL, 8);

    // --- Standalone wxScrollBar controls (one of each orientation) ---
    wxBoxSizer* bars = new wxBoxSizer(wxHORIZONTAL);

    m_vbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition,
                             wxSize(20, 140), wxSB_VERTICAL);
    m_vbar->SetScrollbar(0, 20, 100, 20);
    bars->Add(m_vbar, 0, wxALL, 8);

    m_hbar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition,
                             wxSize(240, 20), wxSB_HORIZONTAL);
    m_hbar->SetScrollbar(0, 20, 100, 20);
    bars->Add(m_hbar, 0, wxALL | wxALIGN_CENTRE_VERTICAL, 8);

    root->Add(bars, 0, wxEXPAND);

    m_vbar->Bind(wxEVT_SCROLL_THUMBTRACK, &ScrollFrame::OnBarScroll, this);
    m_vbar->Bind(wxEVT_SCROLL_CHANGED, &ScrollFrame::OnBarScroll, this);
    m_hbar->Bind(wxEVT_SCROLL_THUMBTRACK, &ScrollFrame::OnBarScroll, this);
    m_hbar->Bind(wxEVT_SCROLL_CHANGED, &ScrollFrame::OnBarScroll, this);

    // --- wxScrolledWindow with overflowing content (built-in gutters) ---
    m_scrolled = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize,
                                      wxVSCROLL | wxHSCROLL | wxBORDER_SUNKEN);
    m_scrolled->SetScrollRate(10, 10);

    wxPanel* content = new wxPanel(m_scrolled, wxID_ANY);
    content->SetBackgroundColour(wxColour(230, 235, 250));
    content->SetMinSize(wxSize(1400, 1400)); // far larger than the viewport
    wxBoxSizer* cs = new wxBoxSizer(wxVERTICAL);
    cs->Add(new wxStaticText(content, wxID_ANY,
            "Scrolled content (drag the gutter on the right/bottom edge)"),
            0, wxALL, 12);
    content->SetSizer(cs);

    wxBoxSizer* inner = new wxBoxSizer(wxVERTICAL);
    inner->Add(content, 1, wxEXPAND);
    m_scrolled->SetSizer(inner);
    m_scrolled->FitInside();

    root->Add(m_scrolled, 1, wxEXPAND | wxALL, 8);

    SetSizer(root);

    Log("app started");
}

void ScrollFrame::Log(const wxString& msg)
{
    if (m_status)
        m_status->SetLabel(msg);

#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('[SCROLLBAR_EVENT] ' + UTF8ToString($0));
    }, (const char*)msg.utf8_str());
#endif
}

void ScrollFrame::OnBarScroll(wxScrollEvent& evt)
{
    Log(wxString::Format("scrollbar pos %d", evt.GetPosition()));
    evt.Skip();
}
