#include <chrono>
#include <cstdint>
#include <android/log.h>
#include <Events.hpp>
#include <jni.h>
#include <Application.hpp>
#include <memory>
#include <FrameGraph.hpp>
#include <thread>

using namespace FlexKit;
using namespace std::chrono_literals;

struct TestState final : public FrameworkState
{
    TestState(GameFramework& IN_framework, IRenderWindow* IN_renderWindow) : 
        FrameworkState  { IN_framework      },
        renderWindow    { IN_renderWindow   } {}


    UpdateTask* Update(EngineCore&, UpdateDispatcher&, double dT) final 
    { 
        std::this_thread::sleep_for(1ms);

        return nullptr; 
    }


    UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher&, double dT, FrameGraph& frameGraph) final
    { 
        static bool toggle = false;
        frameGraph.AddOutput(renderWindow->GetBackBuffer());
        
        ClearBackBuffer(
            frameGraph, renderWindow->GetBackBuffer(), 
            float4{ 
                (float)sinf(t) / 2.0f + 0.5f, 
                (float)sinf(t * 3.0f) / 2.0f + 0.5f, 
                (float)sinf(t * 7.0f) / 2.0f + 0.5f, 
                0.0f });

        toggle = !toggle;

        return nullptr;
    }


    void PostDrawUpdate(EngineCore&, double dT) final 
    {
        renderWindow->Present(1, 0);
        t += dT;
    }


    bool EventHandler(Event evt) final 
    { 
        if(evt.mType == Event::System && evt.InputSource == Event::E_SystemEvent && evt.Action == Event::Exit)
        {
            FK_LOG_0("Quit Event Received");

            framework.PopState();    
        }
        return true;	
    }

    IRenderWindow*  renderWindow = nullptr;
    double          t = 0.0;
};

void SetupApplication(FKApplication& app, IRenderWindow* renderWindow)
{
    __android_log_write(ANDROID_LOG_VERBOSE, "FlexKit", "SetupApplication(): Created Application!");

    app.PushState<TestState>(renderWindow);
}
