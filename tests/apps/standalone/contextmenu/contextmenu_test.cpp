// Context-menu (DoPopupMenu) Test - right-click popup menus in the WASM DOM
// port. KiCad pops these from its tool framework (frame->PopupMenu) on a
// right-click in the canvas; this exercises the same synchronous PopupMenu.

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

enum {
    ID_CTX_CUT = wxID_HIGHEST + 1,
    ID_CTX_PASTE,
    ID_CTX_SNAP,
    ID_CTX_SUB_A,
    ID_CTX_SUB_B
};

class CtxApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

class CtxFrame : public wxFrame
{
public:
    CtxFrame();

private:
    wxStaticText* m_status = nullptr;
    bool m_snap = false;

    void Log(const wxString& msg);
    void PopContextMenu();

    void OnRightUp(wxMouseEvent& evt);
    void OnContextMenu(wxContextMenuEvent& evt);

    void OnCut(wxCommandEvent& evt);
    void OnPaste(wxCommandEvent& evt);
    void OnSnap(wxCommandEvent& evt);
    void OnSubA(wxCommandEvent& evt);
    void OnSubB(wxCommandEvent& evt);
};

wxIMPLEMENT_APP(CtxApp);

bool CtxApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    (new CtxFrame())->Show(true);
    return true;
}

CtxFrame::CtxFrame()
    : wxFrame(nullptr, wxID_ANY, "Context Menu WASM Test",
              wxDefaultPosition, wxSize(640, 480))
{
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    panel->SetBackgroundColour(wxColour(250, 250, 245));

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_status = new wxStaticText(panel, wxID_ANY, "Right-click anywhere");
    sizer->Add(m_status, 0, wxALL, 12);
    wxStaticText* hint = new wxStaticText(panel, wxID_ANY,
        "Right-click this panel to open the context menu.");
    sizer->Add(hint, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    panel->SetSizer(sizer);

    // Right-click anywhere on the panel pops the menu (both the concrete
    // mouse event and the higher-level context-menu event are bound; only
    // one fires per click in practice).
    panel->Bind(wxEVT_RIGHT_UP, &CtxFrame::OnRightUp, this);
    panel->Bind(wxEVT_CONTEXT_MENU, &CtxFrame::OnContextMenu, this);

    Bind(wxEVT_MENU, &CtxFrame::OnCut, this, ID_CTX_CUT);
    Bind(wxEVT_MENU, &CtxFrame::OnPaste, this, ID_CTX_PASTE);
    Bind(wxEVT_MENU, &CtxFrame::OnSnap, this, ID_CTX_SNAP);
    Bind(wxEVT_MENU, &CtxFrame::OnSubA, this, ID_CTX_SUB_A);
    Bind(wxEVT_MENU, &CtxFrame::OnSubB, this, ID_CTX_SUB_B);

    Log("app started");
}

void CtxFrame::Log(const wxString& msg)
{
    if (m_status)
        m_status->SetLabel(msg);

#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('[CTXMENU_EVENT] ' + UTF8ToString($0));
    }, (const char*)msg.utf8_str());
#endif
}

void CtxFrame::PopContextMenu()
{
    wxMenu menu;
    menu.Append(ID_CTX_CUT, "Cut");
    menu.AppendSeparator();
    wxMenuItem* snap = menu.AppendCheckItem(ID_CTX_SNAP, "Snap to grid");
    snap->Check(m_snap);
    wxMenuItem* paste = menu.Append(ID_CTX_PASTE, "Paste");
    paste->Enable(false); // disabled item

    wxMenu* sub = new wxMenu;
    sub->Append(ID_CTX_SUB_A, "Sub A");
    sub->Append(ID_CTX_SUB_B, "Sub B");
    menu.AppendSubMenu(sub, "More");

    // No coords: pop at the mouse (the DOM port uses the last pointer pos).
    PopupMenu(&menu);
}

void CtxFrame::OnRightUp(wxMouseEvent& WXUNUSED(evt))
{
    PopContextMenu();
}

void CtxFrame::OnContextMenu(wxContextMenuEvent& WXUNUSED(evt))
{
    PopContextMenu();
}

void CtxFrame::OnCut(wxCommandEvent& WXUNUSED(evt))    { Log("Cut chosen"); }
void CtxFrame::OnPaste(wxCommandEvent& WXUNUSED(evt))  { Log("Paste chosen"); }
void CtxFrame::OnSnap(wxCommandEvent& evt)
{
    m_snap = evt.IsChecked();
    Log(m_snap ? "Snap to grid ON" : "Snap to grid OFF");
}
void CtxFrame::OnSubA(wxCommandEvent& WXUNUSED(evt))   { Log("Sub A chosen"); }
void CtxFrame::OnSubB(wxCommandEvent& WXUNUSED(evt))   { Log("Sub B chosen"); }
