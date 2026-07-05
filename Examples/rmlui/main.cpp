#include <ExampleFramework.hpp>
#include <Events.hpp>
#include <RMLRenderer.hpp>
#include <RmlUi/Core.h>
#include <FrameGraph.hpp>

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

struct rmluiExampleState : ExampleState
{
	rmluiExampleState() : 
        rml{ GetRenderSystem(), GetAllocator() }
	{
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
	}

	virtual ~rmluiExampleState()
	{
		document->Close();
	}

	UpdateTask* Update(EngineCore& core, UpdateDispatcher& dispatcher, double dt) override
	{
		document->UpdateDocument();
		auto update = rml.Update(core, dispatcher, dt);

		return update;
	}


	UpdateTask* Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph) override
	{
		auto renderTarget = GetRenderWindow().GetBackBuffer();
	    rml.Draw(update, core, RmlPassData{ .renderTarget = renderTarget }, dt, frameGraph);
		return nullptr;
	}


	bool EventHandler(Event& evt) override
	{
		rml.HandleEvent(evt);
		return true;
	}

	RmlIntegrator			rml;
	Rml::Context*			rmlCtx		= nullptr;
	Rml::ElementDocument*	document	= nullptr;
	Button0PressHandler		eventHandler;
};

int main()
{
	return FlexKit::RunExample<rmluiExampleState>();
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
