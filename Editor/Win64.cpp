#include "buildsettings.h"
#include "Logging.h"
#include "Application.h"
#include "AnimationComponents.h"
#include "Logging.cpp"

#include "EditorPanels.cpp"
#include "EditorBase.cpp"

#include "..\FlexKitResourceCompiler\Animation.cpp"
#include "..\FlexKitResourceCompiler\MetaData.cpp"
#include "..\FlexKitResourceCompiler\MeshProcessing.cpp"
#include "..\FlexKitResourceCompiler\ResourceUtilities.cpp"
#include "..\FlexKitResourceCompiler\SceneResource.cpp"

#define WXUSINGDLL

#include <wx/wxprec.h>
#include <wx/splitter.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/artprov.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "resource.h"

class ViewPort : public wxWindow
{
public:
    ViewPort(wxWindow* parent) :
        wxWindow{ parent, wxID_ANY, wxDefaultPosition, parent->GetSize() },
        app     {{ (uint32_t)parent->GetSize().x, (uint32_t)parent->GetSize().y },
                    memory,
                    Max(std::thread::hardware_concurrency(), 1u) - 1, GetHandle() }
    {
        Show(true);
        auto& editor = app.PushState<EditorBase>();

        CreateDefaultLayout(editor);
    }

    ~ViewPort()
    {
        app.Release();
    }

    void DrawFrame()
    {
        app.DrawOneFrame(1.0f / 60.0f);
    }

    void Resize(wxSize size)
    {
        
        app.GetFramework().ActiveWindow->Resize(
            { (uint32_t)size.x, (uint32_t)size.y - GetPosition().y },
            app.GetCore().RenderSystem);

        wxWindow::SetSize(size);
    }

    wxDECLARE_EVENT_TABLE();

    FlexKit::EngineMemory* memory = FlexKit::CreateEngineMemory();
    FlexKit::FKApplication app;// { WH, memory, threadCount };
};

wxBEGIN_EVENT_TABLE(ViewPort, wxWindow)
wxEND_EVENT_TABLE()

class ViewPortRenderTimer : public wxTimer
{
public:
    ViewPortRenderTimer(ViewPort* IN_VP) :
        VP{ IN_VP }
    {

    }

    void Notify()
    {
        VP->DrawFrame();
    }

    void start()
    {
        wxTimer::Start(33);
    }

    ViewPort* VP;
};



class EditorFrame : public wxFrame
{
public:
    EditorFrame(const wxPoint& pos, const wxSize& size) :
        wxFrame     { nullptr, wxID_ANY, std::string{"FkED"}, pos, size, wxDEFAULT_FRAME_STYLE }
        //splitter    { this, -1, wxPoint(0, 300), wxSize(size.x / 1.5 - 60, size.y - 500), wxSP_3D },
        //viewPort    { &splitter },
        //vpTimer     { &viewPort }
    {
        auto ribbonBar = new wxRibbonBar(
            this, -1, wxDefaultPosition, wxDefaultSize,
            wxRIBBON_BAR_FLOW_HORIZONTAL |
            wxRIBBON_BAR_SHOW_PAGE_LABELS |
            wxRIBBON_BAR_SHOW_PANEL_EXT_BUTTONS |
            wxRIBBON_BAR_SHOW_TOGGLE_BUTTON);

        ribbonBar->SetForegroundColour({ 1, 0, 1, 0 });

        auto ribbonPage = new wxRibbonPage{ ribbonBar, wxID_ANY, "Hello", wxNullBitmap };
        auto ribbonPage2 = new wxRibbonPage{ ribbonBar, wxID_ANY, "World", wxNullBitmap };
        auto ribbonPanel = new wxRibbonPanel{ ribbonPage, wxID_ANY, "Hello.Page.1" };
        auto ribbonPanel2 = new wxRibbonPanel{ ribbonPage2, wxID_ANY, "World.Page.1" };

        auto ribbonButtonBar = new wxRibbonButtonBar{ ribbonPanel };
        auto ribbonButtonBar2 = new wxRibbonButtonBar{ ribbonPanel2 };

        ribbonButtonBar->AddButton(wxID_ANY, "Button1", wxArtProvider::GetBitmap(wxART_ADD_BOOKMARK, wxART_TOOLBAR, wxSize(16, 15)));

        ribbonBar->AddPageHighlight(ribbonBar->GetPageCount() - 1);
        ribbonBar->Realize();
        ribbonBar->DismissExpandedPanel();
        ribbonBar->SetArtProvider(new wxRibbonMSWArtProvider);

        wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(ribbonBar, 0, wxEXPAND);
        SetSizer(sizer);
        Centre();
        //vpTimer.start();

        Show(true);
    }

    void OnExit(wxCommandEvent& event)
    {
        Close(true);
    }

    void Resize(wxSizeEvent& event)
    {
        //auto newSize = wxSize(event.GetSize().x / 1.5 - 60, event.GetSize().y - 100);
        //splitter.SetSize(newSize);
        //viewPort.Resize(newSize);
    }

    //wxSplitterWindow    splitter;
    //ViewPort            viewPort;
    //ViewPortRenderTimer vpTimer;

    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(EditorFrame, wxFrame)
    EVT_MENU(wxID_EXIT, EditorFrame::OnExit)
    //EVT_SIZE(EditorFrame::Resize)
wxEND_EVENT_TABLE()


class TestApp : public wxApp
{
public:
    bool OnInit() override
    {
        frame = new EditorFrame(wxPoint(40, 40), wxSize(1920, 1080));

        return true;
    }

    EditorFrame* frame;
};


wxIMPLEMENT_APP(TestApp);

/*
{
    FlexKit::InitLog(argc, argv);
    FlexKit::SetShellVerbocity(FlexKit::Verbosity_1);
    FlexKit::AddLogFile("Editor.log", FlexKit::Verbosity_INFO);


    const auto        WH          = FlexKit::uint2{ 1920, 1080 };
    const uint32_t    threadCount = Max(std::thread::hardware_concurrency(), 1u) - 1;

    //auto* memory = FlexKit::CreateEngineMemory();
    //EXITSCOPE(FlexKit::ReleaseEngineMemory(memory));

    /*
    FlexKit::FKApplication app{ WH, memory, threadCount };

    auto& editor = app.PushState<EditorBase>();
    CreateDefaultLayout(editor);

    app.Run();
    app.Release();
    */

    //return 0;
//}*/



/**********************************************************************

Copyright (c) 2020 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
