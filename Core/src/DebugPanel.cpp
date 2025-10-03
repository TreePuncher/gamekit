
#include "DebugPanel.hpp"
#include "DebugUI.hpp"
#include <imgui.h>

//#include "Win32Graphics.hpp"

namespace FlexKit
{	/************************************************************************************************/


	DebugPanel::DebugPanel(GameFramework& framework, IRenderWindow& IN_renderWindow, FrameworkState& IN_topState) :
		FrameworkState	{ framework         },
		topState        { IN_topState       },
		core            { framework.core    },
		console         { framework.console },
		renderWindow	{ &IN_renderWindow	}
	{
		pauseBackgroundLogic = false;
	}


	/************************************************************************************************/


	DebugPanel::~DebugPanel()
	{
		framework.consoleActive = false;
	}


	/************************************************************************************************/


	UpdateTask* DebugPanel::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
	{
		UpdateTask* res = nullptr;
		if (!pauseBackgroundLogic)
			res = topState.Update(core, dispatcher, dT);
		//else
		//	Win32UpdateInput();

		if (framework.ImGuiAvailable())
		{
			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("Console"))
				{
					console.Draw(core.GetTempMemory());
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Profiler"))
				{
					profiler.DrawProfiler(core.GetTempMemory());
					ImGui::EndTabItem();
				}

			}
			ImGui::EndTabBar();
		}

		return res;
	}


	/************************************************************************************************/


	UpdateTask* DebugPanel::Draw(UpdateTask* update, EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& graph)
	{
		auto temp = topState.Draw(update, core, dispatcher, dT, graph);

		return temp;
	}


	/************************************************************************************************/


	void DebugPanel::PostDrawUpdate(EngineCore& core, double dT)
	{
		topState.PostDrawUpdate(core, dT);
	}


	/************************************************************************************************/


	bool DebugPanel::EventHandler(Event evt)
	{
		if (evt.InputSource == Event::Keyboard)
		{
			switch (evt.Action)
			{
			case Event::Pressed:
			{
				switch (evt.mData1.mKC[0])
				{
				case KC_TILDA: {
					PopSubState(framework);
				}	break;
				case KC_BACKSPACE:
					framework.console.BackSpace();
					break;
				case KC_ARROWUP:
				{
					if(console.commandHistory.size()){
						auto line	  = console.commandHistory[recallIndex].Str;
						auto LineSize = strlen(line);

						FK_ASSERT(false);
						//strcpy_s(console.inputBuffer, console.commandHistory[recallIndex]);

						console.inputBufferSize = LineSize;
						IncrementRecallIndex();
					}
				}	break;
				case KC_ARROWDOWN:
				{
					if (console.commandHistory.size()) {
						auto line		= console.commandHistory[recallIndex].Str;
						auto lineSize	= strlen(line);

						FK_ASSERT(false);
						//strcpy_s(console.inputBuffer, console.commandHistory[recallIndex]);

						console.inputBufferSize = lineSize;
						DecrementRecallIndex();
					}
				}	break;
				}
			}	break;
			}
		}

		return framework.debugUI->HandleInput(evt);
	}


	/************************************************************************************************/


	void DebugPanel::IncrementRecallIndex()
	{
		recallIndex = (recallIndex + 1) % console.commandHistory.size();
	}


	/************************************************************************************************/


	void DebugPanel::DecrementRecallIndex()
	{
		recallIndex = (console.commandHistory.size() + recallIndex - 1) % console.commandHistory.size();
	}


	/************************************************************************************************/

} // namespace FlexKit;


/**********************************************************************

Copyright (c) 2019 Robert May

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

