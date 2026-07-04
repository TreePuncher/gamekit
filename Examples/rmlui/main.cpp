#include <Application.hpp>
#include <RenderSystemInterface.hpp>
#include <FrameGraph.hpp>

#include <Win32Graphics.hpp>
#include <dxBackend.hpp>
#include <imgui.h>
#include <RMLRenderer.hpp>
#include <RmlUi/Core.h>
#include <print>

using namespace FlexKit;

struct Button0PressHandler : Rml::EventListener
{
	Button0PressHandler() = default;
	Button0PressHandler(TypeErasedCallable<void(Rml::Event&)> fn) : callback{ fn } {}

	void ProcessEvent(Rml::Event& event) override
	{
		if (callback)
		    callback(event);
	}

	TypeErasedCallable<void (Rml::Event&)> callback;
};

struct rmluiExampleState : FrameworkState
{
	rmluiExampleState(GameFramework& IN_framework) : 
		FrameworkState		{ IN_framework },
		rml					{ GetRenderSystem(), GetAllocator() }
	{
		Win32RenderWindowDesc windowDesc = DefaultWindowDesc({ 1600, 1200 }, DeviceFormat::R16G16B16A16_FLOAT);
		renderWindow = CreateWin32RenderWindow(GetRenderSystem(), windowDesc);

		rmlCtx = rml.GetMainContext();
		rmlCtx->SetDensityIndependentPixelRatio(1.4f);
		rmlCtx->SetDimensions({ 1600, 1200 });
		document = rmlCtx->LoadDocumentFromMemory(R"(
<rml>
	<head>
	</head>
	<body style="font-family:'Creato Display'; font-size:14dp">
        <br></br>
        <br></br>
		This is an example
        <br></br>
        <button id="button0"> click me! </button><br></br>
        <label><input type="checkbox" value="pizza"/>Pizza1</label><br></br>
        <label><input type="checkbox" value="pizza"/>Pizza2</label><br></br>
        <label><input type="checkbox" value="pizza"/>Pizza3</label><br></br>
        <label><input type="checkbox" value="pizza"/>Pizza4</label><br></br>
       <img id="img0" src="assets/pluto.png" width=400></img> 
<textarea cols=10, rows=10>a b c d e f g h i j k l m n o p q r s t u v</textarea>
        
	</body>
</rml>)");	


		auto button0 = document->GetElementById("button0");
		auto img0 = document->GetElementById("img0");

		eventHandler.callback =
			[button0, &handler = eventHandler](Rml::Event& event)
			{
				auto element = event.GetCurrentElement();
				auto id = element->GetId();

				int x = 0;
			};

		button0->AddEventListener(Rml::EventId::Mouseover, &eventHandler);
		img0->AddEventListener(Rml::EventId::Mouseover, &eventHandler);

		document->UpdateDocument();
		document->Show();

		EventNotifier<>::Subscriber sub;
		sub.Notify	= &EventsWrapper;
		sub._ptr	= &framework;
		Subscribe(renderWindow, sub);

		vBuffer = GetRenderSystem().CreateVertexBuffer(512 * KILOBYTE, false);
		cBuffer = GetRenderSystem().CreateConstantBuffer(512 * KILOBYTE, false);
	}

	virtual ~rmluiExampleState()
	{
		document->Close();
		renderWindow->Release();
		GetRenderSystem().ReleaseVB(vBuffer);
		GetRenderSystem().ReleaseCB(cBuffer);
	}

	UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt)
	{
		t += dt;

		Win32UpdateInput();
		framework.UpdateDebugUI(*renderWindow, core, dispatcher, dt);

		document->UpdateDocument();

		ImGui::NewFrame();
		/*
		if (ImGui::Begin("Hello"))
		{
			ImGui::SetWindowSize({ 500, 500 });
			ImGui::Text("Hello world!");
		}	ImGui::End();
        */
		ImGui::EndFrame();
		ImGui::Render();

		auto update = rml.Update(core, dispatcher, dt);

		return update;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph)
	{
		auto renderTarget = renderWindow->GetBackBuffer();
		frameGraph.AddOutput(renderTarget);
		frameGraph.AddConstantBuffer(cBuffer);
		frameGraph.AddVertexBuffer(vBuffer);

		ClearBackBuffer(frameGraph, renderTarget);

		framework.DrawDebugUI(dt, dispatcher, frameGraph, renderTarget);

	    rml.Draw(update, core, RmlPassData{ .renderTarget = renderTarget }, dt, frameGraph);

		PresentBackBuffer(frameGraph, *renderWindow);
		return nullptr;
	}


	bool EventHandler(Event evt) override
	{
		if (evt.InputSource == Event::E_SystemEvent && evt.mType == Event::EventType::Internal && evt.Action == Event::InputAction::Exit)
		{
			framework.quit = true;
			return true;
		}
		rml.HandleEvent(evt);
		return framework.HandleDebugInput(evt);
	}

	void PostDrawUpdate(EngineCore& core, double dT) override
	{
		renderWindow->Present();
		core.RenderSystem->ResetConstantBuffer(cBuffer);
		core.RenderSystem->ResetVertexBuffer(vBuffer);
	}

	double					t = 0.0;
	size_t					vertexCount		= 0;
	IRenderWindow*			renderWindow	= nullptr;
	VertexBufferHandle		vBuffer			= InvalidHandle;
	ConstantBufferHandle	cBuffer			= InvalidHandle;
	TriMeshHandle			shape			= InvalidHandle;
	UniqueResourceHandle	testTexture		= InvalidHandle;

	RmlIntegrator			rml;
	Rml::Context*			rmlCtx		= nullptr;
	Rml::ElementDocument*	document	= nullptr;

	Button0PressHandler eventHandler;
};

int main()
{
	try
	{
		{
			auto memoryPools = CreateEngineMemory();

			auto app = std::make_unique<FKApplication>(
				&memoryPools,
				CoreOptions{
					.GPUdebugMode		= false,
					.GPUValidation		= false,
					.GPUSyncQueues		= false,
					.CreateRenderSystem = CreateDX,
				},
				FrameworkOptions{
					.integrateIMGUI = true
				});

			app->PushState<rmluiExampleState>();
			app->GetCore().FPSLimit = 144;
			app->GetCore().FrameLock = true;
			app->GetCore().vSync = true;
			app->Run();
		}
		int x = 0;
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

Copyright (c) 2015 - 2026 Robert May

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
