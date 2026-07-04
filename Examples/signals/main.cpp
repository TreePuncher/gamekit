#include <Application.hpp>
#include <RenderSystemInterface.hpp>
#include <FrameGraph.hpp>
#include <filesystem>
#include <ModifiableShape.hpp>
#include <MeshUtilities.hpp>
#include <TriMeshResource.hpp>
#include <Type.hpp>
#include <TriggerComponent.hpp>
#include <imgui.h>

#include <Win32Graphics.hpp>
#include <dxBackend.hpp>

#include <print>


using namespace FlexKit;

struct TestInput
{
	double t;
};

constexpr uint32_t SlotID		= GetCRC32("Slot0");
constexpr uint32_t TriggerID	= GetCRC32("Trigger0");
constexpr uint32_t InputTypeID	= GetTypeGUID(TestInput);

struct SignalExampleState : FrameworkState
{
	SignalExampleState(GameFramework& IN_framework) : 
		FrameworkState	{ IN_framework },
		triggers		{ GetAllocatorMT(), GetAllocatorMT() },
		objectPool		{ GetAllocatorMT(), 1024 }
	{
		Win32RenderWindowDesc windowDesc = DefaultWindowDesc({ 800, 600 }, DeviceFormat::R16G16B16A16_FLOAT);
		renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);

		EventNotifier<>::Subscriber sub;
		sub.Notify	= &EventsWrapper;
		sub._ptr	= &framework;
		Subscribe(renderWindow, sub);

		vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
		cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);

		object0	= &objectPool.Allocate();
		auto& triggerView	= object0->AddView<TriggerView>();

		triggerView->CreateTrigger(TriggerID);
		triggerView->CreateSlot(
			SlotID,
			[](void* _ptr, uint32_t typeID)
			{
				if (_ptr != nullptr && typeID == InputTypeID)
				{
					ImGui::Text("Signal was triggered!");
				    ImGui::Text("Signaled at t= %lf!", static_cast<TestInput*>(_ptr)->t);
				}
			});

		triggerView->Connect(TriggerID, SlotID);
	}

	~SignalExampleState()
	{
		renderWindow->Release();
		objectPool.Release(*object0);

		GetRenderSystem().ReleaseVB(vBuffer);
		GetRenderSystem().ReleaseCB(cBuffer);
	}

	UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt)
	{
		t += dt;

		Win32UpdateInput();
		framework.UpdateDebugUI(*renderWindow, core, dispatcher, dt);

		ImGui::NewFrame();
		if (ImGui::Begin("Hello"))
		{
			ImGui::SetWindowPos({ 0, 0 });
			ImGui::SetWindowSize({ 600, 500 });
			ImGui::Text("Hello world!");

			Trigger(*object0, TriggerID, TestInput{ .t = t }, InputTypeID);
		}	ImGui::End();
		ImGui::EndFrame();
		ImGui::Render();

	    return nullptr;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);
		frameGraph.AddConstantBuffer(cBuffer);
		frameGraph.AddVertexBuffer(vBuffer);

		ClearBackBuffer(frameGraph, renderTarget, { sinf(t) * 0.5f + 0.5f, cosf(t * 5.0f) * 0.5f + 0.5f, tanf(t * 10.0f) * 0.5f + 0.5f, 1 });
		framework.DrawDebugUI(dt, dispatcher, frameGraph, renderTarget);
		PresentBackBuffer(frameGraph, *renderWindow);

	    return nullptr;
	}

	void PostDrawUpdate(FlexKit::EngineCore& core, double dt) override
	{
		renderWindow->Present();
		core.RenderSystem->ResetVertexBuffer(vBuffer);
		core.RenderSystem->ResetConstantBuffer(cBuffer);
	}

	bool EventHandler(Event evt) override
	{
		if (evt.InputSource == Event::E_SystemEvent && evt.mType == Event::EventType::Internal && evt.Action == Event::InputAction::Exit)
		{
			framework.quit = true;
			return true;
		}
		return framework.HandleDebugInput(evt);
	}

	double					t				= 0.0;
	size_t					vertexCount		= 0;
	IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;

	ObjectPool<GameObject>	objectPool;
	TriggerComponent		triggers;

    GameObject*				object0 = nullptr;
};

int main()
{
	try
	{
		auto memoryPools = CreateEngineMemory();

		auto app = std::make_unique<FKApplication>(
			&memoryPools,
			CoreOptions{
				.GPUdebugMode		= true,
				.GPUValidation		= true,
				.GPUSyncQueues		= true,
				.CreateRenderSystem = CreateDX,
			}, FrameworkOptions{
				.integrateIMGUI		= true
			});

		app->PushState<SignalExampleState>();
		app->GetCore().FPSLimit		= 144;
		app->GetCore().FrameLock	= true;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (std::runtime_error runtimeError)
	{
		FK_LOG_ERROR("Exception Caught!\n%s", runtimeError.what());
	}
	catch (...)
	{
		FK_LOG_ERROR("Exception Caught!");
		return -1;
	}
	return 0;
}


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
