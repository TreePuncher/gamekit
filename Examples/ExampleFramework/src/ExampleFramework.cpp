#include <Application.hpp>
#include <FrameGraph.hpp>
#include <TriggerComponent.hpp>
#include <imgui.h>

#include <Win32Graphics.hpp>
#include <dxBackend.hpp>

#include "ExampleFramework.hpp"

namespace FlexKit
{
	struct BaseExampleState : FrameworkState
	{
		BaseExampleState(GameFramework& IN_framework, const ExampleDescription& desc);
		virtual ~BaseExampleState();

	    UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt) final;
		UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph) final;

		void PostDrawUpdate(EngineCore& core, double dt) final;
		bool EventHandler(Event evt) final;

		uint2					WH;
		double					t				= 0.0;
		IRenderWindow*			renderWindow	= nullptr;
		VertexBufferHandle		vBuffer			= InvalidHandle;
		ConstantBufferHandle	cBuffer			= InvalidHandle;

		ObjectPool<GameObject>	objectPool;
		TriggerComponent		triggers;

		MouseInputState			mouseState;

		std::unique_ptr<ExampleState>	exampleState;
	};


    BaseExampleState::BaseExampleState(GameFramework& IN_framework, const ExampleDescription& desc) :
	    FrameworkState	{ IN_framework },
	    triggers		{ GetAllocatorMT(), GetAllocatorMT() },
	    objectPool		{ GetAllocatorMT(), 1024 }, 
        WH				{ desc.WH }
    {
	    Win32RenderWindowDesc windowDesc = DefaultWindowDesc(desc.WH, DeviceFormat::R16G16B16A16_FLOAT);
	    renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);
		
		void				ToggleMouseCapture(IRenderWindow*);
		MouseInputState		UpdateCapturedMouseInput(double dT, IRenderWindow*);

		SetWindowTitle(desc.windowName, renderWindow);

	    EventNotifier<>::Subscriber sub;
	    sub.Notify = &EventsWrapper;
	    sub._ptr = &framework;
	    Subscribe(renderWindow, sub);

	    vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
	    cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);
    }


    BaseExampleState::~BaseExampleState()
    {
		exampleState.release();
	    renderWindow->Release();

	    GetRenderSystem().ReleaseVB(vBuffer);
	    GetRenderSystem().ReleaseCB(cBuffer);
    }


    UpdateTask* BaseExampleState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt)
    {
	    t += dt;

	    Win32UpdateInput();
		mouseState = UpdateCapturedMouseInput(dt, renderWindow);

		UpdateTask* out = nullptr;
		if (exampleState)
    		out = exampleState->Update(core, dispatcher, dt);
	
        framework.UpdateDebugUI(*renderWindow, core, dispatcher, dt);

	    ImGui::NewFrame();
		
        if (exampleState)
		    exampleState->DrawUI();
	    
        ImGui::EndFrame();
	    ImGui::Render();

	    return out;
    }


    UpdateTask* BaseExampleState::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph)
    {
	    auto renderTarget = renderWindow->GetBackBuffer();
	    frameGraph.AddOutput(renderTarget);
	    frameGraph.AddConstantBuffer(cBuffer);
	    frameGraph.AddVertexBuffer(vBuffer);

	    ClearBackBuffer(frameGraph, renderTarget);

		if (exampleState)
			exampleState->Draw(core, dispatcher, dt, frameGraph);

	    framework.DrawDebugUI(dt, dispatcher, frameGraph, renderTarget);
	    PresentBackBuffer(frameGraph, *renderWindow);

	    return nullptr;
    }


    void BaseExampleState::PostDrawUpdate(FlexKit::EngineCore& core, double dt)
    {
	    renderWindow->Present();
	    core.RenderSystem->ResetVertexBuffer(vBuffer);
	    core.RenderSystem->ResetConstantBuffer(cBuffer);
    }


    bool BaseExampleState::EventHandler(Event evt)
    {
	    if (evt.InputSource == Event::E_SystemEvent && evt.mType == Event::EventType::Internal && evt.Action == Event::InputAction::Exit)
	    {
		    framework.quit = true;
		    return true;
	    }

		if (framework.HandleDebugInput(evt))
			return true;

		if (exampleState)
		    return exampleState->EventHandler(evt);
	    
        return false;
    }

	struct ExampleApplication
    {
		ExampleApplication(const ExampleDescription& desc) :
			memoryPools{ CreateEngineMemory() },
			fkApp{
				std::make_unique<FKApplication>(
				&memoryPools,
				CoreOptions{
	#ifdef _DEBUG
					.GPUdebugMode	= true,
					.GPUValidation	= true,
					.GPUSyncQueues	= true,
	#endif
					.CreateRenderSystem = CreateDX,
				}, FrameworkOptions{
					.integrateIMGUI = true
				}) }
		{
			fkApp->GetCore().FPSLimit	= 144;
			fkApp->GetCore().FrameLock	= true;
			fkApp->GetCore().vSync		= true;

			exampleState = &fkApp->PushState<BaseExampleState>(desc);
		}

		~ExampleApplication()
		{
			fkApp->PopState();
		}

		EngineMemory					memoryPools;
		std::unique_ptr<FKApplication>	fkApp;
		BaseExampleState*				exampleState = nullptr;
    };

	inline static std::unique_ptr<ExampleApplication> app;

    void InitiateExampleFramework(const ExampleDescription& desc)
	{
		app = std::move(std::make_unique<ExampleApplication>(desc));
	}

	void ReleaseExampleApplication()
	{
		app.release();
	}

	int RunExampleApplication()
	{
		try
		{
		    app->fkApp->Run();
			return 0;
		}
		catch (...)
		{
			return -1;
		}
	}

	void SetExampleState(std::unique_ptr<ExampleState> state)
	{
		app->exampleState->exampleState = std::move(state);
	}

	IRenderSystem& ExampleState::GetRenderSystem()
	{
		return app->exampleState->framework.GetRenderSystem();
	}

	iAllocator& ExampleState::GetAllocator()
	{
		return app->exampleState->framework.core.GetBlockMemory();
	}

	iAllocator& ExampleState::GetAllocatorMT()
	{
		return app->exampleState->framework.core.GetMTBlockMemory();
	}

	iAllocator& ExampleState::GetTempAllocator()
	{
		return app->exampleState->framework.core.GetTempMemory();
	}

	iAllocator& ExampleState::GetTempAllocatorMT()
	{
		return app->exampleState->framework.core.GetTempMemoryMT();
	}

	ThreadManager& ExampleState::GetThreads()
	{
		return app->exampleState->framework.core.Threads;
	}

	IRenderWindow& ExampleState::GetRenderWindow()
	{
		return *app->exampleState->renderWindow;
	}

	uint2 ExampleState::GetWH()
	{
		return *app->exampleState->WH;
	}

	MouseInputState& ExampleState::GetMouseState()
    {
		return app->exampleState->mouseState;
    }

	void ExampleState::ToggleMouse(bool b)
    {
		auto& window = GetRenderWindow();
		SetMouseCapture(b, &window);
    }

	static GameObject& AllocateGameObject()
    {
		return app->exampleState->objectPool.Allocate();
	}

	static void ReleaseGameObject(GameObject& go)
    {
		app->exampleState->objectPool.Release(go);
    }
}
