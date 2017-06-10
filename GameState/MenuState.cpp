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


/************************************************************************************************/


bool OnHostPressed(void* _ptr, size_t GUIElement)
{
	std::cout << "Host Pressed\n";
	auto* Args = (CBArguements*)_ptr;

	Args->State->Framework->ActiveScene->ClearScene();

	PopSubState(Args->State->Framework);
	PushSubState(Args->State->Framework, CreateHostState(Args->Engine, Args->State->Framework));

	//Args->Engine->BlockAllocator.free(_ptr);
	return true;
}


/************************************************************************************************/


bool OnJoinPressed(void* _ptr, size_t GUIElement)
{
	std::cout << "Join Pressed\n";
	if (_ptr) {
		auto* Args = (CBArguements*)_ptr;
		auto ThisState = Args->State;

		//Args->State->Framework->ActiveScene->ClearScene();
		auto Framework = Args->State->Framework;

		PopSubState	(Framework);
		//PushSubState(Args->State->Framework, CreateClientState(Args->Engine, Args->State->Framework));
		PushSubState(Framework, CreateJoinScreenState(Framework, Framework->Engine));
	}
	return true;
}


/************************************************************************************************/


bool OnExitPressed(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	Args->State->Framework->Quit = true;
	return true;
}


/************************************************************************************************/


bool OnExitEntered(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	std::cout << "Button Entered!\n";
	return true;
}


/************************************************************************************************/


bool OnExitLeft(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	std::cout << "Button Left!\n";
	return true;
}


/************************************************************************************************/


bool PreDraw(FrameworkState* StateMemory, EngineMemory* Engine, double DT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	SimpleWindowInput	Input	  = {};

	Input.LeftMouseButtonPressed  = ThisState->Framework->MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->Framework->MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;

	ThisState->BettererWindow.Draw		(Engine->RenderSystem, &ThisState->Framework->Immediate, Engine->TempAllocator, GetPixelSize(Engine));

	if(StateMemory->Framework->DrawDebug)
		ThisState->BettererWindow.Draw_DEBUG(Engine->RenderSystem, &ThisState->Framework->Immediate, Engine->TempAllocator, GetPixelSize(Engine));

	ThisState->BettererWindow.Upload(Engine->RenderSystem, &ThisState->Framework->Immediate);

	DrawMouseCursor(Engine, ThisState->Framework, ThisState->Framework->MouseState.NormalizedPos, { 0.05f, 0.05f });

	//PrintText(&ThisState->Framework->Immediate, "THIS IS A TEST!\nHello World!", ThisState->Framework->DefaultAssets.Font, { 0.0f, 0.0f }, {0.4f, 0.2f}, float4(WHITE, 1), GetPixelSize(Engine));

	return true;
}


/************************************************************************************************/


bool Update			(FrameworkState* StateMemory, EngineMemory* Engine, double dT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	SimpleWindowInput	Input	  = {};
	GameFramework*		Framework	  = ThisState->Framework;

	ThisState->T += dT;

	Input.LeftMouseButtonPressed  = ThisState->Framework->MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->Framework->MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;

	//Yaw(Engine->Nodes, Framework->ActiveCamera->Node, pi / 8 );
	//Pitch(Engine->Nodes, Framework->ActiveCamera->Node, pi / 8 * ThisState->Framework->MouseState.dPos[1] * dT);
	
	ThisState->BettererWindow.Update(dT, Input, GetPixelSize(&Engine->Window), Engine->TempAllocator);

	//UpdateSimpleWindow(&Input, &ThisState->Window);

	/*
	auto Model = ThisState->Model;
	Framework->GScene.SetMaterialParams(
		Model, 
		float4(
			std::abs(std::sin(ThisState->T * 2)), 	0, 0, 
			std::abs(std::sin(ThisState->T/2))), 
		float4(1, 1, 1, 0));
	*/

	//DEBUG_DrawCameraFrustum(Framework->ImmediateRender, Framework->ActiveCamera);

	return true;
}


/************************************************************************************************/


void ReleaseMenu	(FrameworkState* StateMemory)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	CleanUpSimpleWindow(&ThisState->Window, ThisState->Framework->Engine->RenderSystem);
	ThisState->BettererWindow.Release();
	ThisState->Framework->Engine->BlockAllocator.free(ThisState);
}


/************************************************************************************************/


MenuState* CreateMenuState(GameFramework* Framework, EngineMemory* Engine)
{
	FK_ASSERT(Framework != nullptr);
	auto* State	= &Engine->BlockAllocator.allocate_aligned<MenuState>(&Engine->BlockAllocator.AllocatorInterface);

	State->VTable.PreDrawUpdate = PreDraw;
	State->VTable.Update        = Update;
	State->VTable.Release       = ReleaseMenu;
	State->T                    = 0;
	State->Framework            = Framework;
	State->CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };

	Framework->MouseState.Enabled   = true;

	if(1)
	{
		auto Grid = State->BettererWindow.CreateGrid();
		Grid.resize(0.2f, 0.5f);
		Grid.SetGridDimensions(3, 5);
		Grid.SetPosition({0.7f, 0.1f});

		auto Font = Framework->DefaultAssets.Font;

		auto C = Grid.ColumnWidths();
		Grid.ColumnWidths()[0] = 0.05f;
		Grid.ColumnWidths()[1] = 0.90f;
		Grid.ColumnWidths()[2] = 0.05f;

		Grid.RowHeights()[0] = 0.05f;
		Grid.RowHeights()[1] = 0.30f;
		Grid.RowHeights()[2] = 0.30f;
		Grid.RowHeights()[3] = 0.30f;
		Grid.RowHeights()[4] = 0.05f;

		auto Btn1 = Grid.CreateButton({ 1, 1 });
		auto Btn2 = Grid.CreateButton({ 1, 2 });
		auto Btn3 = Grid.CreateButton({ 1, 3 });

		auto Args		= &State->CBArgs;
		Args->Engine	= Engine;
		Args->State		= State;

		std::cout << Args->Engine;
		std::cout << Args->State;
		std::cout << Args->State->Framework;

		Btn1.SetActive(true);
		auto& Btn1_Ref = Btn1._IMPL();
		auto DefaultFont = State->Framework->DefaultAssets.Font;

		Btn1_Ref.Clicked = OnHostPressed;
		Btn1_Ref.USR     = Args;
		Btn1_Ref.Text	 = "Host";
		Btn1_Ref.Font	 = DefaultFont;

		Btn2.SetActive(true);
		Btn2._IMPL().Clicked = OnJoinPressed;
		Btn2._IMPL().USR     = Args;
		Btn2._IMPL().Text    = "Join";
		Btn2._IMPL().Font	 = DefaultFont;

		Btn3.SetActive(true);
		Btn3._IMPL().Clicked = OnExitPressed;
		Btn3._IMPL().USR     = Args;
		Btn3._IMPL().Text	 = "Quit";
		Btn3._IMPL().Font	 = DefaultFont;
	}
	else
	{
		SimpleWindow_Desc Desc(0.26f, 0.36f, 1, 1, (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1]);
	
		auto Window			= &State->Window;
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

		auto Args		= &State->CBArgs;
		Args->Engine	= Engine;
		Args->State		= State;

		SimpleWindowAddDivider(Window, {0.0f, 0.01f}, ButtonStack);

		GUITextButton_Desc TextButton{};
		TextButton.Font			   = Framework->DefaultAssets.Font;
		TextButton.Text			   = "Host";
		TextButton.WH			   = float2(0.2f, 0.1f);
		TextButton.WindowSize	   = {(float)WindowSize[0], (float)WindowSize[1]};
		TextButton.CB_Args		   = Args;
		TextButton.OnClicked_CB    = OnHostPressed;

		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(Framework->DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, { 0.0f, 0.005f }, ButtonStack);

		TextButton.Text			   = "Join";
		TextButton.OnClicked_CB    = OnJoinPressed;
		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(Framework->DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, {0.0f, 0.005f}, ButtonStack);

		TextButton.OnClicked_CB    = OnExitPressed;
		TextButton.OnEntered_CB    = OnExitEntered;
		TextButton.OnExit_CB       = OnExitLeft;
		TextButton.Text            = "Exit Game";
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);
	}

	return State;
}


/************************************************************************************************/


bool JoinScreenUpdate(FrameworkState* StateMemory, EngineMemory* Engine, double dT)
{
	JoinScreen*			ThisState = (JoinScreen*)StateMemory;
	SimpleWindowInput	Input = {};
	GameFramework*		Framework = ThisState->Framework;

	Input.LeftMouseButtonPressed = ThisState->Framework->MouseState.LMB_Pressed;
	Input.MousePosition			 = ThisState->Framework->MouseState.NormalizedPos;
	Input.CursorWH				 = ThisState->CursorSize;

	ThisState->BettererWindow.Update(dT, Input, GetPixelSize(&Engine->Window), Engine->TempAllocator);

	return false;
}


/************************************************************************************************/


bool JoinScreenPreDraw(FrameworkState* StateMemory, EngineMemory* Engine, double DT)
{
	JoinScreen*			ThisState = (JoinScreen*)StateMemory;
	SimpleWindowInput	Input = {};

	ThisState->BettererWindow.Draw_DEBUG(Engine->RenderSystem, ThisState->Framework->Immediate, Engine->TempAllocator, GetPixelSize(Engine));
	ThisState->BettererWindow.Draw(Engine->RenderSystem, ThisState->Framework->Immediate, Engine->TempAllocator, GetPixelSize(Engine));

	return false;
}


/************************************************************************************************/


void ReleaseJoinScreen(FrameworkState* StateMemory)
{
	JoinScreen*	ThisState = (JoinScreen*)StateMemory;
	
	ThisState->BettererWindow.Release();
	ThisState->~JoinScreen();
}


/************************************************************************************************/


JoinScreen* CreateJoinScreenState(GameFramework* Framework, EngineMemory* Engine)
{
	FK_ASSERT(Framework != nullptr);
	FK_ASSERT(Engine != nullptr);

	auto* State	= &Engine->BlockAllocator.allocate_aligned<JoinScreen>(&Engine->BlockAllocator.AllocatorInterface);

	State->VTable.PreDrawUpdate = JoinScreenPreDraw;
	State->VTable.Update        = JoinScreenUpdate;
	State->VTable.Release       = ReleaseJoinScreen;
	State->Framework            = Framework;
	State->CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };

	Framework->MouseState.Enabled   = true;

	auto Grid = State->BettererWindow.CreateGrid();
	Grid.resize				(0.4, 0.3);
	Grid.SetPosition		({ 0.3f, 0.35f});
	Grid.SetGridDimensions	(1, 4);
	Grid.SetActive			(true);

	auto TextLabel1		= Grid.CreateTextBox({0, 0}, "Enter IP", Framework->DefaultAssets.Font);
	TextLabel1.SetActive(true);

	auto TextLabel2		= Grid.CreateTextBox({0, 2}, "Enter Name", Framework->DefaultAssets.Font);
	TextLabel2.SetActive(true);

	auto IPInput		= Grid.CreateTextBox({ 0, 1 }, "", Framework->DefaultAssets.Font);
	IPInput.SetActive(true);

	auto NameInput		= Grid.CreateTextBox({ 0, 3 }, "", Framework->DefaultAssets.Font);
	NameInput.SetActive(true);

	return State;
}


/************************************************************************************************/
