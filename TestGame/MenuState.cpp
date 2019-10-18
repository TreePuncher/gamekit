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

#include "MenuState.h"
#include "PlayState.h"
#include "HostState.h"
#include "Client.h"


/************************************************************************************************/


bool OnHostPressed(void* _ptr, size_t GUIElement)
{
	return false;
	/*
	std::cout << "Host Pressed\n";
	auto* Args = (CBArguements*)_ptr;

	Args->State->framework.ActiveScene->ClearScene();

	PopSubState(Args->State->framework);
	PushSubState(
		Args->State->framework, 
		&Args->State->framework.core.GetBlockMemory().allocate_aligned<HostState>(
			Args->Engine, 
			Args->State->framework));

	//Args->Engine->GetBlockMemory().free(_ptr);
	return true;
	*/
}


/************************************************************************************************/


bool OnJoinPressed(void* _ptr, size_t GUIElement)
{
	return false;

	/*
	std::cout << "Join Pressed\n";
	if (_ptr) {
		auto* Args = (CBArguements*)_ptr;
		auto ThisState = Args->State;

		//Args->State->framework.ActiveScene->ClearScene();
		auto framework = Args->State->framework;

		PopSubState	(framework);
		//PushSubState(Args->State->framework, CreateClientState(Args->Engine, Args->State->framework));
		PushSubState(framework, CreateJoinScreenState(framework, framework.Core));
	}
	return true;
	*/
}


/************************************************************************************************/


bool OnExitPressed(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	Args->State->framework.Quit = true;
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


bool PreDraw(FrameworkState* StateMemory, EngineCore* Engine, double DT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	WindowInput	Input	  = {};

	Input.LeftMouseButtonPressed  = ThisState->framework.MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->framework.MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;


	return true;
}


/************************************************************************************************/


bool Update			(FrameworkState* StateMemory, EngineCore* Engine, double dT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	WindowInput	Input	  = {};
	GameFramework*		framework	  = ThisState->framework;

	ThisState->T += dT;

	Input.LeftMouseButtonPressed  = ThisState->framework.MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->framework.MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;

	//Yaw(Engine->Nodes, framework.ActiveCamera->Node, pi / 8 );
	//Pitch(Engine->Nodes, framework.ActiveCamera->Node, pi / 8 * ThisState->framework.MouseState.dPos[1] * dT);
	
	ThisState->UISystem.Update(dT, Input, GetPixelSize(&Engine->Window), Engine->GetTempMemory());

	//UpdateSimpleWindow(&Input, &ThisState->Window);

	/*
	auto Model = ThisState->Model;
	framework.GScene.SetMaterialParams(
		Model, 
		float4(
			std::abs(std::sin(ThisState->T * 2)), 	0, 0, 
			std::abs(std::sin(ThisState->T/2))), 
		float4(1, 1, 1, 0));
	*/

	//DEBUG_DrawCameraFrustum(framework.ImmediateRender, framework.ActiveCamera);

	return true;
}


/************************************************************************************************/


void ReleaseMenu	(FrameworkState* StateMemory)
{
	MenuState*			ThisState = (MenuState*)StateMemory;

	ThisState->UISystem.Release();
	ThisState->framework.core.GetBlockMemory().free(ThisState);
}


/************************************************************************************************/


MenuState* CreateMenuState(GameFramework* framework, EngineCore* Engine)
{
	/*
	FK_ASSERT(framework != nullptr);
	auto* State	= &Engine->GetBlockMemory().allocate_aligned<MenuState>(&Engine->GetBlockMemory().AllocatorInterface);

	//State->VTable.PreDrawUpdate = PreDraw;
	//State->VTable.Update        = Update;
	//State->VTable.Release       = ReleaseMenu;
	State->T                    = 0;
	State->framework            = framework;
	State->CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };

	framework.MouseState.Enabled   = true;

	if(1)
	{
		auto Grid = State->BettererWindow.CreateGrid();
		Grid.resize(0.2f, 0.5f);
		Grid.SetGridDimensions(3, 5);
		Grid.SetPosition({0.7f, 0.1f});

		auto Font = framework.DefaultAssets.Font;

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
		std::cout << Args->State->framework;

		Btn1.SetActive(true);
		auto& Btn1_Ref		= Btn1._IMPL();
		auto DefaultFont	= State->framework.DefaultAssets.Font;

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
		InitiateSimpleWindow(Engine->GetBlockMemory(), Window, Desc);
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
		TextButton.Font			   = framework.DefaultAssets.Font;
		TextButton.Text			   = "Host";
		TextButton.WH			   = float2(0.2f, 0.1f);
		TextButton.WindowSize	   = {(float)WindowSize[0], (float)WindowSize[1]};
		TextButton.CB_Args		   = Args;
		TextButton.OnClicked_CB    = OnHostPressed;

		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(framework.DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, { 0.0f, 0.005f }, ButtonStack);

		TextButton.Text			   = "Join";
		TextButton.OnClicked_CB    = OnJoinPressed;
		TextButton.BackGroundColor = float4(Grey(.5), 1);
		TextButton.CharacterScale  = float2(16, 32) / Conversion::Vect2TOfloat2(framework.DefaultAssets.Font->FontSize);
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);

		SimpleWindowAddDivider(Window, {0.0f, 0.005f}, ButtonStack);

		TextButton.OnClicked_CB    = OnExitPressed;
		TextButton.OnEntered_CB    = OnExitEntered;
		TextButton.OnExit_CB       = OnExitLeft;
		TextButton.Text            = "Exit Game";
		SimpleWindowAddTextButton(Window, TextButton, Engine->RenderSystem, ButtonStack);
	}

	return State;
	*/
	return nullptr;
}


/************************************************************************************************/


bool JoinScreenUpdate(FrameworkState* StateMemory, EngineCore* Engine, double dT)
{
	JoinScreen*			ThisState = (JoinScreen*)StateMemory;
	WindowInput	Input = {};
	GameFramework*		framework = ThisState->framework;

	Input.LeftMouseButtonPressed = ThisState->framework.MouseState.LMB_Pressed;
	Input.MousePosition			 = ThisState->framework.MouseState.NormalizedPos;
	Input.CursorWH				 = ThisState->CursorSize;

	ThisState->UISystem.Update(dT, Input, GetPixelSize(&Engine->Window), Engine->GetTempMemory());

	return false;
}


/************************************************************************************************/


bool JoinScreenPreDraw(FrameworkState* StateMemory, EngineCore* Engine, double DT)
{
	JoinScreen*			ThisState = (JoinScreen*)StateMemory;
	WindowInput	Input = {};

	FK_ASSERT(0);
	//ThisState->BettererWindow.Draw(Engine->RenderSystem, ThisState->framework.Immediate, Engine->GetTempMemory(), GetPixelSize(Engine));
	//ThisState->BettererWindow.Draw_DEBUG(Engine->RenderSystem, ThisState->framework.Immediate, Engine->GetTempMemory(), GetPixelSize(Engine));

	DrawMouseCursor(Engine, ThisState->framework, ThisState->framework.MouseState.NormalizedPos, { 0.02f / GetWindowAspectRatio(Engine), 0.02f});

	return false;
}


/************************************************************************************************/


void ReleaseJoinScreen(FrameworkState* StateMemory)
{
	JoinScreen*	ThisState = (JoinScreen*)StateMemory;
	
	ThisState->UISystem.Release();
	ThisState->~JoinScreen();
}


/************************************************************************************************/


bool ServerFieldClicked(void* _ptr, size_t GUIElement)
{
	std::cout << "Server Field Clicked\n";
	return false;
}


/************************************************************************************************/


bool NameFieldClicked(void* _ptr, size_t GUIElement)
{
	std::cout << "Name Field Clicked\n";
	return false;
}


/************************************************************************************************/


JoinScreen* CreateJoinScreenState(GameFramework* framework, EngineCore* Engine)
{
	FK_ASSERT(framework != nullptr);
	FK_ASSERT(Engine != nullptr);

	auto* State	= &Engine->GetBlockMemory().allocate_aligned<JoinScreen>(
													framework, 
													(iAllocator*)Engine->GetBlockMemory());

	framework.MouseState.Enabled = true;

	State->framework            = framework;
	State->CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };

	memset(State->Name, '\0',	sizeof(JoinScreen::Name));
	memset(State->Server, '\0', sizeof(JoinScreen::Server));

	auto&  Grid = State->UISystem.CreateGrid();

	//Grid.resize				(0.3f, 0.2f);
	//Grid.SetPosition		({ 0.3f, 0.35f});
	//Grid.SetGridDimensions	(1, 5);
	//Grid.SetActive			(true);

	Grid.RowHeights[0] = 0.20f;
	Grid.RowHeights[1] = 0.20f;
	Grid.RowHeights[2] = 0.20f;
	Grid.RowHeights[3] = 0.20f;
	Grid.RowHeights[4] = 0.20f;

	Grid.ColumnWidths[0] = 1.0f;

	//auto TextLabel1		= Grid.CreateTextBox({0, 0}, "Enter Server IP", framework.DefaultAssets.Font);
	//TextLabel1.SetActive(true);

	//auto TextLabel2		= Grid.CreateTextBox({0, 2}, "Enter Name", framework.DefaultAssets.Font);
	//TextLabel2.SetActive(true);

	//auto IPInput		= Grid.CreateTextBox({ 0, 1 }, State->Name, framework.DefaultAssets.Font);
	//IPInput.SetActive(true);
	//IPInput._IMPL().Clicked		= [](size_t x) -> bool{return false;};
	//auto NameInput				= Grid.CreateTextBox({ 0, 3 }, State->Server, framework.DefaultAssets.Font);
	//NameInput.SetActive(true);
	//NameInput._IMPL().Clicked	= NameFieldClicked;

	//auto AcceptButton	= Grid.CreateButton({ 0, 4 }, "Accept", framework.DefaultAssets.Font);
	//AcceptButton.SetActive(true);
	//sAcceptButton.SetUSR(State);

	return State;
}


/************************************************************************************************/
