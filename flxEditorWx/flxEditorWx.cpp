#define WXUSINGDLL 

#include <wx/wx.h>
#include <iostream>
#include <memory>

#include "Application.h"
#include "allsourcefiles.cpp"
#include "Win32Graphics.h"

class ClearWindowState : public FlexKit::FrameworkState
{
public:
    ClearWindowState(FlexKit::GameFramework& framework, FlexKit::ResourceHandle IN_renderTarget) :
        FrameworkState{ framework },
        target{ IN_renderTarget } {}

    void Draw(FlexKit::EngineCore& Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
    {
        auto a = float(rand() % 256);
        auto b = float(rand() % 256);
        auto c = float(rand() % 256);

        frameGraph.AddRenderTarget(target);
        FlexKit::ClearBackBuffer(frameGraph, target, { 1.0f / a, 1.0f / b, 1.0f / c, 1 });
        FlexKit::PresentBackBuffer(frameGraph, target);
    }

    FlexKit::ResourceHandle target;
};

enum
{
    Button_Pressed = wxID_HIGHEST + 1 // declares an id which will be used to call our button
};


class MainFrame : public wxFrame
{
public:
    MainFrame(FlexKit::FKApplication* IN_application);

    void Callme(wxCommandEvent& event)
    {
        pushMe->SetLabel("Touch Me Harder!");
        //outputDebugString(L"I was pressed\n");
    }

    DECLARE_EVENT_TABLE();

public:
    FlexKit::Win32RenderWindow  renderWindow;
    std::unique_ptr<wxButton>   pushMe;
    FlexKit::FKApplication*     application;
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(Button_Pressed, MainFrame::Callme)
END_EVENT_TABLE()


MainFrame::MainFrame(FlexKit::FKApplication* IN_application) :
    wxFrame     { (wxFrame*)NULL, -1, _T("Super guud editor") },
    application { IN_application }
{
    SetBackgroundColour(wxColour{ 1, 0, 1, 0 });

    auto statusBar = CreateStatusBar();
    //statusBar->SetBackgroundColour(wxColour{ 1, 0, 1, 0 });
    statusBar->SetForegroundColour(wxColour{ 255, 0, 255, 1 });
    statusBar->SetStatusText("Hello!");
    Show(true);

    //pushMe = std::make_unique<wxButton>(this, Button_Pressed, "Touch Me", wxPoint{ 0, 0 }, wxSize{ 50, 50 }, wxBU_EXACTFIT);

    auto hwnd = (HWND)GetHWND();
    renderWindow = FlexKit::CreateWin32RenderWindowFromHWND(application->GetCore().RenderSystem, hwnd).first;

    application->PushState<ClearWindowState>(renderWindow.GetBackBuffer());
    application->DrawOneFrame(1.0f);
}


class WxEditorTest : public wxApp
{
public:
    virtual bool OnInit()
    {
        application = std::make_unique<FlexKit::FKApplication>(FlexKit::CreateEngineMemory());

        frame = new MainFrame(application.get());
        Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(WxEditorTest::onIdle));

        return true;
    }


    void onIdle(wxIdleEvent& evt)
    {
        application->DrawOneFrame(1.0f);
        frame->renderWindow.Present(1, 0);

        evt.RequestMore();
    }

    MainFrame*                              frame;
    std::unique_ptr<FlexKit::FKApplication> application;


};

IMPLEMENT_APP(WxEditorTest);
