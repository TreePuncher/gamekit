/**********************************************************************

Copyright (c) 2018 Robert May

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


#include "MainMenu.h"


/************************************************************************************************/


MainMenu::MainMenu(
	GameFramework&		IN_framework,
	BaseState&			IN_base) :
		FlexKit::FrameworkState{ IN_framework },
		constantBuffer		{ IN_base.constantBuffer },
		vertexBuffer		{ IN_base.vertexBuffer },
		textBuffer			{ IN_base.textBuffer },
		gui					{{}, IN_framework.core.GetBlockMemory()}
{
	menuGrid = &gui.CreateGrid();
	menuGrid->SetGridDimensions({ 1, 3 });
	menuGrid->XY = { 0.4f, 0.2f };
	menuGrid->WH = { 0.2f, 0.5f };

	menuJoinButton = &gui.CreateButton(menuGrid, { 0, 0 });
	menuJoinButton->Text	= "Join";
	menuJoinButton->WH		= {1, 1};
	menuJoinButton->Font	= IN_framework.DefaultAssets.Font;

	menuQuitButton = &gui.CreateButton(menuGrid, { 0, 2 });
	menuQuitButton->Text	= "Quit";
	menuQuitButton->Font	= IN_framework.DefaultAssets.Font;
	menuQuitButton->Clicked = [&]() { 
		framework.quit = true; 
		return false; 
	};
}


/************************************************************************************************/


void MainMenu::Update(
	EngineCore& core,
	UpdateDispatcher& dispatcher, 
	double dT)
{
	FlexKit::WindowInput windowInput;
	windowInput.CursorWH				= { 0.05f, 0.05f };
	windowInput.MousePosition			= framework.MouseState.NormalizedScreenCord;
	windowInput.LeftMouseButtonPressed	= framework.MouseState.LMB_Pressed;

	gui.Update(dT, windowInput, GetPixelSize(core.Window), core.GetTempMemory());
}


/************************************************************************************************/


void MainMenu::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	auto currentRenderTarget = core.Window.backBuffer;

	ClearVertexBuffer	(frameGraph, vertexBuffer);
	ClearVertexBuffer	(frameGraph, textBuffer);
	ClearBackBuffer		(frameGraph, currentRenderTarget, { 0.0f, 0.0f, 0.0f, 0.0f });

	FlexKit::DrawUI_Desc drawDesc = 
	{
		&frameGraph,
		currentRenderTarget,
		vertexBuffer,
		textBuffer,
		constantBuffer
	};
	gui.Draw(drawDesc, core.GetTempMemory());

	FlexKit::DrawMouseCursor(
		framework.MouseState.NormalizedScreenCord,
		{0.05f, 0.05f}, 
		vertexBuffer, 
		constantBuffer,
		currentRenderTarget,
		core.GetTempMemory(),
		&frameGraph);
}

/************************************************************************************************/


void MainMenu::PostDrawUpdate(EngineCore& core, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph)
{
	if (framework.drawDebugStats)
		framework.DrawDebugHUD(dT, textBuffer, Graph);

	PresentBackBuffer(Graph, &core.Window);
}


/************************************************************************************************/
