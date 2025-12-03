// wxGraphicsContext Test - Vector graphics like KiCad's anti-aliased rendering
// Tests: wxGraphicsContext, paths, gradients, transforms, comparison with wxDC

#include "wx/wx.h"
#include "wx/graphics.h"
#include "wx/dcbuffer.h"

class GraphicsPanel : public wxPanel
{
public:
    GraphicsPanel(wxWindow* parent, bool useGraphicsContext)
        : wxPanel(parent, wxID_ANY), m_useGC(useGraphicsContext)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &GraphicsPanel::OnPaint, this);
    }

    void SetDrawMode(int mode) { m_drawMode = mode; Refresh(); }
    void SetUseGC(bool use) { m_useGC = use; Refresh(); }

private:
    void OnPaint(wxPaintEvent& event)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();

        if (m_useGC)
        {
            wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
            if (gc)
            {
                DrawWithGraphicsContext(gc);
                delete gc;
            }
            else
            {
                dc.DrawText("wxGraphicsContext not available!", 10, 10);
            }
        }
        else
        {
            DrawWithDC(dc);
        }
    }

    void DrawWithGraphicsContext(wxGraphicsContext* gc)
    {
        wxSize size = GetClientSize();

        switch (m_drawMode)
        {
            case 0: DrawBasicShapes(gc, size); break;
            case 1: DrawPaths(gc, size); break;
            case 2: DrawGradients(gc, size); break;
            case 3: DrawTransforms(gc, size); break;
            case 4: DrawAntiAliased(gc, size); break;
        }
    }

    void DrawBasicShapes(wxGraphicsContext* gc, const wxSize& size)
    {
        // Title
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
        gc->DrawText("Basic Shapes (wxGraphicsContext)", 10, 10);

        // Rectangle
        gc->SetBrush(gc->CreateBrush(wxBrush(*wxRED)));
        gc->SetPen(gc->CreatePen(wxPen(*wxBLACK, 2)));
        gc->DrawRectangle(30, 50, 80, 60);

        // Ellipse
        gc->SetBrush(gc->CreateBrush(wxBrush(*wxBLUE)));
        gc->DrawEllipse(140, 50, 80, 60);

        // Rounded rectangle
        gc->SetBrush(gc->CreateBrush(wxBrush(*wxGREEN)));
        gc->DrawRoundedRectangle(250, 50, 80, 60, 15);

        // Lines
        gc->SetPen(gc->CreatePen(wxPen(*wxRED, 3)));
        gc->StrokeLine(30, 140, 110, 180);
        gc->SetPen(gc->CreatePen(wxPen(*wxBLUE, 3)));
        gc->StrokeLine(140, 140, 220, 180);
        gc->SetPen(gc->CreatePen(wxPen(*wxGREEN, 3)));
        gc->StrokeLine(250, 140, 330, 180);
    }

    void DrawPaths(wxGraphicsContext* gc, const wxSize& size)
    {
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
        gc->DrawText("Path Drawing (wxGraphicsPath)", 10, 10);

        // Star shape using path
        wxGraphicsPath path = gc->CreatePath();
        double cx = 80, cy = 100;
        double outerR = 40, innerR = 20;
        int points = 5;

        for (int i = 0; i < points * 2; i++)
        {
            double r = (i % 2 == 0) ? outerR : innerR;
            double angle = i * M_PI / points - M_PI / 2;
            double x = cx + r * cos(angle);
            double y = cy + r * sin(angle);

            if (i == 0)
                path.MoveToPoint(x, y);
            else
                path.AddLineToPoint(x, y);
        }
        path.CloseSubpath();

        gc->SetBrush(gc->CreateBrush(wxBrush(*wxYELLOW)));
        gc->SetPen(gc->CreatePen(wxPen(*wxBLACK, 2)));
        gc->FillPath(path);
        gc->StrokePath(path);

        // Bezier curve
        wxGraphicsPath bezier = gc->CreatePath();
        bezier.MoveToPoint(150, 60);
        bezier.AddCurveToPoint(180, 40, 220, 140, 280, 120);

        gc->SetPen(gc->CreatePen(wxPen(*wxRED, 3)));
        gc->StrokePath(bezier);

        // Arc
        wxGraphicsPath arc = gc->CreatePath();
        arc.AddArc(200, 180, 40, 0, M_PI * 1.5, true);

        gc->SetPen(gc->CreatePen(wxPen(*wxBLUE, 3)));
        gc->StrokePath(arc);
    }

    void DrawGradients(wxGraphicsContext* gc, const wxSize& size)
    {
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
        gc->DrawText("Gradient Fills", 10, 10);

        // Linear gradient
        wxGraphicsBrush linGrad = gc->CreateLinearGradientBrush(
            30, 50, 130, 110, *wxRED, *wxBLUE);
        gc->SetBrush(linGrad);
        gc->SetPen(gc->CreatePen(wxPen(*wxBLACK, 1)));
        gc->DrawRectangle(30, 50, 100, 60);

        // Another linear gradient
        wxGraphicsBrush linGrad2 = gc->CreateLinearGradientBrush(
            150, 50, 150, 110, *wxGREEN, *wxYELLOW);
        gc->SetBrush(linGrad2);
        gc->DrawRectangle(150, 50, 100, 60);

        // Radial gradient
        wxGraphicsBrush radGrad = gc->CreateRadialGradientBrush(
            300, 80, 300, 80, 50, *wxWHITE, *wxBLUE);
        gc->SetBrush(radGrad);
        gc->DrawEllipse(250, 50, 100, 60);

        // Gradient with custom stops
        wxGraphicsGradientStops stops;
        stops.Add(*wxRED, 0.0f);
        stops.Add(*wxYELLOW, 0.5f);
        stops.Add(*wxGREEN, 1.0f);

        wxGraphicsBrush multiGrad = gc->CreateLinearGradientBrush(
            30, 140, 350, 140, stops);
        gc->SetBrush(multiGrad);
        gc->DrawRectangle(30, 140, 320, 40);
    }

    void DrawTransforms(wxGraphicsContext* gc, const wxSize& size)
    {
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
        gc->DrawText("Transforms (rotate, scale, translate)", 10, 10);

        gc->SetBrush(gc->CreateBrush(wxBrush(*wxBLUE)));
        gc->SetPen(gc->CreatePen(wxPen(*wxBLACK, 2)));

        // Original rectangle
        gc->PushState();
        gc->DrawRectangle(50, 60, 60, 40);
        gc->PopState();

        // Rotated rectangle
        gc->PushState();
        gc->Translate(180, 80);
        gc->Rotate(M_PI / 6);  // 30 degrees
        gc->SetBrush(gc->CreateBrush(wxBrush(*wxRED)));
        gc->DrawRectangle(-30, -20, 60, 40);
        gc->PopState();

        // Scaled rectangle
        gc->PushState();
        gc->Translate(280, 80);
        gc->Scale(1.5, 0.75);
        gc->SetBrush(gc->CreateBrush(wxBrush(*wxGREEN)));
        gc->DrawRectangle(-30, -20, 60, 40);
        gc->PopState();

        // Multiple transforms
        gc->PushState();
        gc->Translate(120, 160);
        gc->Rotate(M_PI / 4);  // 45 degrees
        gc->Scale(1.2, 1.2);
        gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(255, 128, 0))));
        gc->DrawRectangle(-25, -25, 50, 50);
        gc->PopState();
    }

    void DrawAntiAliased(wxGraphicsContext* gc, const wxSize& size)
    {
        gc->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), *wxBLACK);
        gc->DrawText("Anti-Aliased Rendering (vs wxDC)", 10, 10);

        // Draw thin diagonal lines (shows anti-aliasing)
        for (int i = 0; i < 5; i++)
        {
            gc->SetPen(gc->CreatePen(wxPen(*wxBLACK, 1)));
            gc->StrokeLine(30 + i * 20, 50, 70 + i * 20, 120);
        }

        // Draw circles (shows smooth curves)
        gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(255, 200, 200))));
        gc->SetPen(gc->CreatePen(wxPen(*wxRED, 2)));
        gc->DrawEllipse(150, 50, 70, 70);

        // Text rendering
        gc->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL), *wxBLUE);
        gc->DrawText("Smooth Text", 240, 70);

        // Rotated text
        gc->PushState();
        gc->Translate(100, 180);
        gc->Rotate(-M_PI / 12);
        gc->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), wxColour(0, 128, 0));
        gc->DrawText("Rotated Text", 0, 0);
        gc->PopState();
    }

    void DrawWithDC(wxDC& dc)
    {
        // Simple DC drawing for comparison
        dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dc.DrawText("Basic Shapes (wxDC - no anti-aliasing)", 10, 10);

        dc.SetBrush(*wxRED_BRUSH);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawRectangle(30, 50, 80, 60);

        dc.SetBrush(*wxBLUE_BRUSH);
        dc.DrawEllipse(140, 50, 80, 60);

        dc.SetBrush(*wxGREEN_BRUSH);
        dc.DrawRoundedRectangle(250, 50, 80, 60, 15);
    }

    bool m_useGC = true;
    int m_drawMode = 0;
};

class GraphicsCtxFrame : public wxFrame
{
public:
    GraphicsCtxFrame() : wxFrame(nullptr, wxID_ANY, "wxGraphicsContext Test",
                                  wxDefaultPosition, wxSize(800, 600))
    {
        wxPanel* mainPanel = new wxPanel(this);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Description
        wxStaticText* desc = new wxStaticText(mainPanel, wxID_ANY,
            "KiCad uses wxGraphicsContext for anti-aliased vector graphics.\n"
            "Tests: Paths, gradients, transforms, anti-aliasing comparison.");
        mainSizer->Add(desc, 0, wxALL, 5);

        // Controls
        wxBoxSizer* ctrlSizer = new wxBoxSizer(wxHORIZONTAL);

        ctrlSizer->Add(new wxStaticText(mainPanel, wxID_ANY, "Draw Mode:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

        wxChoice* modeChoice = new wxChoice(mainPanel, wxID_ANY);
        modeChoice->Append("Basic Shapes");
        modeChoice->Append("Paths (star, bezier, arc)");
        modeChoice->Append("Gradients");
        modeChoice->Append("Transforms");
        modeChoice->Append("Anti-Aliasing Demo");
        modeChoice->SetSelection(0);
        modeChoice->Bind(wxEVT_CHOICE, &GraphicsCtxFrame::OnModeChanged, this);
        ctrlSizer->Add(modeChoice, 0, wxRIGHT, 20);

        m_gcCheck = new wxCheckBox(mainPanel, wxID_ANY, "Use wxGraphicsContext");
        m_gcCheck->SetValue(true);
        m_gcCheck->Bind(wxEVT_CHECKBOX, &GraphicsCtxFrame::OnGCToggle, this);
        ctrlSizer->Add(m_gcCheck, 0, wxALIGN_CENTER_VERTICAL);

        mainSizer->Add(ctrlSizer, 0, wxALL, 5);

        // Graphics panel
        m_graphicsPanel = new GraphicsPanel(mainPanel, true);
        m_graphicsPanel->SetMinSize(wxSize(-1, 250));
        mainSizer->Add(m_graphicsPanel, 1, wxEXPAND | wxALL, 5);

        // Event log
        mainSizer->Add(new wxStaticText(mainPanel, wxID_ANY, "Event Log"), 0, wxLEFT | wxTOP, 5);
        m_log = new wxTextCtrl(mainPanel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 100),
                               wxTE_MULTILINE | wxTE_READONLY);
        mainSizer->Add(m_log, 0, wxEXPAND | wxALL, 5);

        mainPanel->SetSizer(mainSizer);

        CreateStatusBar();
        SetStatusText("Graphics context test app started");
        Log("Graphics context test app started");
        Log("wxGraphicsContext provides anti-aliased vector graphics");
    }

private:
    void OnModeChanged(wxCommandEvent& event)
    {
        wxChoice* choice = dynamic_cast<wxChoice*>(event.GetEventObject());
        int mode = choice->GetSelection();
        m_graphicsPanel->SetDrawMode(mode);
        Log(wxString::Format("Draw mode changed to: %s", choice->GetStringSelection()));
    }

    void OnGCToggle(wxCommandEvent& event)
    {
        bool useGC = m_gcCheck->IsChecked();
        m_graphicsPanel->SetUseGC(useGC);
        Log(wxString::Format("Using %s", useGC ? "wxGraphicsContext" : "wxDC"));
    }

    void Log(const wxString& msg)
    {
        m_log->AppendText(msg + "\n");
    }

    GraphicsPanel* m_graphicsPanel;
    wxCheckBox* m_gcCheck;
    wxTextCtrl* m_log;
};

class GraphicsCtxApp : public wxApp
{
public:
    virtual bool OnInit() override
    {
        GraphicsCtxFrame* frame = new GraphicsCtxFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(GraphicsCtxApp);
