/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "EngineCore.h"
#include "../coreutilities/Transforms.h"

namespace FlexKit
{
	/************************************************************************************************/


	void ReleaseCore(EngineCore* Engine)
	{

	}


	/************************************************************************************************/


	bool InitiateEngineMemory(EngineMemory*& Memory)
	{
		Memory = (EngineMemory*)_aligned_malloc(PRE_ALLOC_SIZE, 0x40);
		memset(Memory, 0, PRE_ALLOC_SIZE);

		bool Out = false;
		BlockAllocator_desc BAdesc;
		BAdesc._ptr			= (FlexKit::byte*)Memory->BlockMem;
		BAdesc.SmallBlock	= MEGABYTE * 64;
		BAdesc.MediumBlock	= MEGABYTE * 64;
		BAdesc.LargeBlock	= MEGABYTE * 512;

		Memory->BlockAllocator.Init(BAdesc);
		Memory->LevelAllocator.Init(Memory->LevelMem, LEVELBUFFERSIZE);
		Memory->TempAllocator.Init(Memory->TempMem, TEMPBUFFERSIZE);

		InitDebug(&Memory->Debug);

		return true;
	}


	/************************************************************************************************/


	bool InitEngine(EngineCore*& Core, EngineMemory*& Memory, uint2 WH)
	{
		InitiateEngineMemory(Memory);

		Core = &Memory->BlockAllocator.allocate<EngineCore>(Memory);
		auto RES = InitiateCoreSystems(WH, Core);

		FK_ASSERT(RES, "FAILED TO INITIATE ENGINE!");

		return RES;
	}


	/************************************************************************************************/


	void UpdateCoreComponents(EngineCore* Core, double dt)
	{
		//Core->Cameras.Update(dt);
	}


	/************************************************************************************************/


	bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen = false)
	{
		using FlexKit::CreateRenderWindow;
		using FlexKit::RenderWindowDesc;

		// Initiate Render Window
		RenderWindowDesc	WinDesc = { 0 };
		WinDesc.POS_X = 0;
		WinDesc.POS_Y = 0;
		WinDesc.height = height;
		WinDesc.width = width;
		WinDesc.fullscreen = fullscreen;

		if (CreateRenderWindow(Game->RenderSystem, &WinDesc, &Game->Window))
			FK_ASSERT(false, "RENDER WINDOW FAILED TO INITIALIZE!");

		return true;
	}


	/************************************************************************************************/


	bool InitiateCoreSystems(uint2 WH, EngineCore*& Engine)
	{
		bool Sucess						= false;
		uint32_t width					= WH[0];
		uint32_t height					= WH[1];
		bool InvertDepth				= true;
		FlexKit::Graphics_Desc	desc	= { 0 };
		desc.Memory						= Engine->GetBlockMemory();


		if (!Engine->RenderSystem.Initiate(&desc))
			return false;

		Engine->Window.Close = false;
		Sucess = CreateRenderWindow(Engine, height, width, false);

		if (!Sucess)
		{
			Engine->RenderSystem.Release();
			cout << "Failed to Create Render Window!\n";
			return false;
		}


		SetInputWIndow			(&Engine->Window);
		InitiatePhysics			(&Engine->Physics, gCORECOUNT, Engine->GetBlockMemory());
		InitiateResourceTable	(Engine->GetBlockMemory());

		return Sucess;
	}


	/************************************************************************************************/


	float GetWindowAspectRatio(EngineCore* Engine)
	{
		FK_ASSERT(Engine);

		float out = (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1];

		return out;
	}


	/************************************************************************************************/


	uint2 GetWindowWH(EngineCore* Engine)
	{
		FK_ASSERT(Engine);

		return Engine->Window.WH;
	}


	/************************************************************************************************/


	float2 GetPixelSize(EngineCore* Engine)
	{
		return GetPixelSize(&Engine->Window);
	}



	/************************************************************************************************/


	void ClearMouseButtonStates(MouseInputState* State)
	{
		State->LMB_Duration = 0;
		State->RMB_Duration = 0;

		State->LMB_Pressed = false;
		State->RMB_Pressed = false;
	}



	/************************************************************************************************/


	void UpdateMouseInput(MouseInputState* State, RenderWindow* Window)
	{
		if (!State->Enabled) {
			State->dPos = { 0, 0 };
			return;
		}

		if (GetForegroundWindow() == Window->hWindow)
		{
			State->dPos = GetMousedPos(Window);
			State->Position.x -= State->dPos[0] * 0.5f;
			State->Position.y += State->dPos[1] * 0.5f;

			State->Position[0] = max(0.0f, min((float)State->Position[0], (float)Window->WH[0]));
			State->Position[1] = max(0.0f, min((float)State->Position[1], (float)Window->WH[1]));

			State->NormalizedPos[0] = max(0.0f, min((float)State->Position[0] / (float)Window->WH[0], 1));
			State->NormalizedPos[1] = max(0.0f, min((float)State->Position[1] / (float)Window->WH[1], 1));

			SetSystemCursorToWindowCenter(Window);
			ShowCursor(false);
		}
		else
		{
			ShowCursor(true);
			State->Enabled = false;
		}
	}


	/************************************************************************************************/


	void PushCmdArg(EngineCore* Engine, const char* str)
	{
		Engine->CmdArguments.push_back(str);
	}

}	/************************************************************************************************/