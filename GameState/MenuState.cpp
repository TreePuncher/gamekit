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

struct CBArguements
{
	EngineMemory*	Engine;
	MenuState*		State;
};


bool OnPlayPressed(void* _ptr, size_t GUIElement)
{
	auto* Args = (CBArguements*)_ptr;
	PopSubState(Args->State->Base);
	PushSubState(Args->State->Base, CreatePlayState(Args->Engine, Args->State->Base));

	//Args->Engine->BlockAllocator.free(_ptr);
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
	DrawMouseCursor(Engine, ThisState->Base);

	ThisState->BettererWindow.Draw		(Engine->RenderSystem, &ThisState->Base->GUIRender);
	ThisState->BettererWindow.Draw_DEBUG(Engine->RenderSystem, &ThisState->Base->GUIRender);

	ThisState->BettererWindow.Upload(Engine->RenderSystem, &ThisState->Base->GUIRender);

	return true;
}

bool Update			(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	SimpleWindowInput	Input	  = {};
	BaseState*			Base	  = ThisState->Base;

	ThisState->T += dT;

	Input.LeftMouseButtonPressed  = ThisState->Base->MouseState.LMB_Pressed;
	Input.MousePosition			  = ThisState->Base->MouseState.NormalizedPos;
	Input.CursorWH				  = ThisState->CursorSize;

	ThisState->BettererWindow.Update(dT, Input);

	UpdateSimpleWindow(&Input, &ThisState->Window);

	auto Model = ThisState->Model;
	Base->GScene.SetMaterialParams(Model, 
		float4(
			std::abs(std::sin(ThisState->T * 2)), 	0, 0, std::abs(std::sin(ThisState->T/2))), 
		float4(1, 1, 1, 0));




	return true;
}

void ReleaseMenu	(SubState* StateMemory)
{
	MenuState*			ThisState = (MenuState*)StateMemory;
	CleanUpSimpleWindow(&ThisState->Window, ThisState->Base->Engine->RenderSystem);
}

MenuState* CreateMenuState(BaseState* Base, EngineMemory* Engine)
{
	auto* State                 = &Engine->BlockAllocator.allocate_aligned<MenuState>(Engine->BlockAllocator);
	State->VTable.PreDrawUpdate = PreDraw;
	State->VTable.Update        = Update;
	State->VTable.Release       = ReleaseMenu;
	State->T                    = 0;
	State->Base                 = Base;
	State->CursorSize           = float2{ 0.03f / GetWindowAspectRatio(Engine), 0.03f };
	Base->MouseState.Enabled    = true;

	auto Model = Base->GScene.CreateDrawableAndSetMesh("Flower");
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100,100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);
	Base->GScene.AddPointLight(float3(1, 1, 1), float3{ 0, 1000, 10 }, 100, 100);

	Base->GScene.SetMaterialParams(Model, float4(1, 0, 0, 1), float4(0, 0, 1, 1));
	Base->GScene.TranslateEntity_WT(Model, {-3, -4, -10});

	FlexKit::SetPositionW(Engine->Nodes, Base->ActiveCamera->Node, float3{ 0,10, -25.921f });
	FlexKit::Yaw(Engine->Nodes, Base->ActiveCamera->Node, pi);

	AddResourceFile("assets\\ShaderBallTestScene.gameres", &Engine->Assets);
	FK_ASSERT(FlexKit::LoadScene(Engine->RenderSystem, Base->Nodes, &Engine->Assets, &Engine->Geometry, 201, &Base->GScene, Engine->TempAllocator ), "FAILED TO LOAD!\n");

	State->Model = Model;

	{
		auto Grid = State->BettererWindow.CreateGrid();
		Grid.resize(0.5f, 0.5f);
		Grid.SetGridDimensions(5, 5);

		auto Btn1 = Grid.CreateButton({ 3, 4 });
		auto Btn2 = Grid.CreateButton({ 4, 4 });
	}

	{
		SimpleWindow_Desc Desc(0.26f, 0.26f, 1, 1, (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1]);
	
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

		auto Args		= &Engine->BlockAllocator.allocate_aligned<CBArguements>();
		Args->Engine	= Engine;
		Args->State		= State;

		SimpleWindowAddDivider(Window, {0.0f, 0.01f}, ButtonStack);

		GUITextButton_Desc TextButton{};
		TextButton.Font			   = Base->DefaultAssets.Font;
		TextButton.Text			   = "Play Game";
		TextButton.WH			   = float2(0.2f, 0.1f);
		TextButton.WindowSize	   = {(float)WindowSize[0], (float)WindowSize[1]};
		TextButton.CB_Args		   = Args;
		TextButton.OnClicked_CB    = OnPlayPressed;

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
	return State;
}
