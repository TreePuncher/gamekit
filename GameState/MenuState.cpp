/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "MenuState.h"
#include "PlayState.h"
#include "HostState.h"
#include "Client.h"

struct CBArguements
{
	EngineMemory*	Engine;
	MenuState*		State;
};


bool OnHostPressed(void* _ptr, size_t GUIElement)
{
	std::cout << "Host Pressed\n";
	auto* Args = (CBArguements*)_ptr;

	Args->State->Base->GScene.ClearScene();

	PopSubState(Args->State->Base);
	PushSubState(Args->State->Base, CreateHostState(Args->Engine, Args->State->Base));

	//Args->Engine->BlockAllocator.free(_ptr);
	return true;
}

bool OnJoinPressed(void* _ptr, size_t GUIElement)
{
	std::cout << "Join Pressed\n";
	if (_ptr) {
		auto* Args = (CBArguements*)_ptr;

		Args->State->Base->GScene.ClearScene();

		PopSubState(Args->State->Base);
		PushSubState(Args->State->Base, CreatePlayState(Args->Engine, Args->State->Base));
		//PushSubState(Args->State->Base, CreateClientState(Args->Engine, Args->State->Base));

		//Args->Engine->BlockAllocator.free(_ptr);
	}
	return true;
}


bool OnExitPressed(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	Args->State->Base->Quit = true;
	return true;
}


bool OnExitEntered(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	std::cout << "Button Entered!\n";
	return true;
}


bool OnExitLeft(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	std::cout << "Button Left!\n";
	return true;
}


bool PreDraw(SubState* StateMemory, EngineMemory* Engine, double DT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	SimpleWindowInput	Input	  = {};

	Input.LeftMouseButtonPressed  = ThisState->Base->MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->Base->MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;

	DrawSimpleWindow(Input, &ThisState->Window, &StateMemory->Base->GUIRender);
	DrawMouseCursor(Engine, ThisState->Base, ThisState->Base->MouseState.NormalizedPos, {0.05f, 0.05f});

	ThisState->BettererWindow.Draw		(Engine->RenderSystem, &ThisState->Base->GUIRender);
	ThisState->BettererWindow.Draw_DEBUG(Engine->RenderSystem, &ThisState->Base->GUIRender);

	ThisState->BettererWindow.Upload(Engine->RenderSystem, &ThisState->Base->GUIRender);

	return true;
}


bool Update			(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	SimpleWindowInput	Input	  = {};
	GameFramework*		Base	  = ThisState->Base;

	ThisState->T += dT;

	Input.LeftMouseButtonPressed  = ThisState->Base->MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->Base->MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;
	
	//Yaw(Engine->Nodes, Base->ActiveCamera->Node, pi / 8 );
	//Pitch(Engine->Nodes, Base->ActiveCamera->Node, pi / 8 * ThisState->Base->MouseState.dPos[1] * dT);
	
	ThisState->BettererWindow.Update(dT, Input);

	//UpdateSimpleWindow(&Input, &ThisState->Window);

	/*
	auto Model = ThisState->Model;
	Base->GScene.SetMaterialParams(
		Model, 
		float4(
			std::abs(std::sin(ThisState->T * 2)), 	0, 0, 
			std::abs(std::sin(ThisState->T/2))), 
		float4(1, 1, 1, 0));
	*/

	//DEBUG_DrawCameraFrustum(Base->GUIRender, Base->ActiveCamera);

	return true;
}

void ReleaseMenu	(SubState* StateMemory)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	CleanUpSimpleWindow(&ThisState->Window, ThisState->Base->Engine->RenderSystem);
	ThisState->BettererWindow.Release();
	ThisState->Base->Engine->BlockAllocator.free(ThisState);
}

MenuState* CreateMenuState(GameFramework* Base, EngineMemory* Engine)
{
	auto& State                = *(MenuState*)Engine->BlockAllocator.malloc(sizeof(MenuState));
	new(&State) MenuState(Engine->BlockAllocator);

	State.VTable.PreDrawUpdate = PreDraw;
	State.VTable.Update        = Update;
	State.VTable.Release       = ReleaseMenu;
	State.T                    = 0;
	State.Base                 = Base;
	State.CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };
	Base->MouseState.Enabled    = true;

	/*
	if(0)
	{
		auto Grid = State.BettererWindow.CreateGrid();
		Grid.resize(0.5f, 0.5f);
		Grid.SetGridDimensions(3, 4);
		Grid.SetPosition({0.5f, 0.0f});

		auto Btn1 = Grid.CreateButton({ 0, 1 });
		auto Btn2 = Grid.CreateButton({ 0, 2 });

		auto Args		= &Engine->BlockAllocator.allocate_aligned<CBArguements>();
		Args->Engine	= Engine;
		Args->State		= &State;

		Btn1.SetActive(true);
		Btn1._IMPL().Clicked = OnJoinPressed;
		Btn1._IMPL().USR = Args;

		Btn2.SetActive(true);
		Btn2._IMPL().Clicked = OnExitPressed;
		Btn2._IMPL().USR = Args;
	}
	else
	{
		SimpleWindow_Desc Desc(0.26f, 0.36f, 1, 1, (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1]);
	
		auto Window			= &State.Window;
		auto WindowSize		= GetWindowWH(Engine);
		InitiateSimpleWindow(Engine->BlockAllocator, Window, Desc);
		Window->Position	= {0.65f, 0.1f};
		Window->CellBorder  = {0.01f, 0.01f};
		Window->CellColor   = float4(Grey(0.2f), 1.0f);
		Window->PanelColor  = float4(Grey(0.2f), 1.0f);

		GUIList List_Desc;
		List_Desc.Position	= { 0, 0 };
		List_Desc.WH		= { 1, 1 };
		List_Desc.Color		= float4(Grey(0.2f), 1);
	
		auto HorizontalOffset	= SimpleWindowAddHorizonalList	(Window, List_Desc);
		SimpleWindowAddDivider(Window, { 0.02f, 0.00f }, HorizontalOffset);

		auto ButtonStack   = SimpleWindowAddVerticalList	(Window, List_Desc, HorizontalOffset);

		List_Desc.Position = { 0, 1 };
		List_Desc.WH	   = { 1, 1 };

		auto Args		= &Engine->BlockAllocator.allocate_aligned<CBArguements>();
		Args->Engine	= Engine;
		Args->State		= &State;

		SimpleWindowAddDivider(Window, {0.0f, 0.01f}, ButtonStack);

		GUITextButton_Desc TextButton{};
		TextButton.Font			   = Base->DefaultAssets.Font;
		TextButton.Text			   = "Host";
		TextButton.WH			   = float2(0.2f, 0.1f);
		TextButton.WindowSize	   = {(float)WindowSize[0], (float)WindowSize[1]};
		TextButton.CB_Args		   = Args;
		TextButton.OnClicked_CB    = OnHostPressed;

		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(Base->DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, { 0.0f, 0.005f }, ButtonStack);

		TextButton.Text			   = "Join";
		TextButton.OnClicked_CB    = OnJoinPressed;
		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(Base->DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, {0.0f, 0.005f}, ButtonStack);

		TextButton.OnClicked_CB    = OnExitPressed;
		TextButton.OnEntered_CB    = OnExitEntered;
		TextButton.OnExit_CB       = OnExitLeft;
		TextButton.Text            = "Exit Game";
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);
	}
	*/
	return &State;
}
