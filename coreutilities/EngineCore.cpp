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
	bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen = false);


	/************************************************************************************************/


	void ReleaseCore(EngineCore* Engine)
	{

	}


	/************************************************************************************************/



	EngineMemory* CreateEngineMemory()
	{
		bool ThrowAway;
		return CreateEngineMemory(ThrowAway);
	}


	EngineMemory* CreateEngineMemory(bool& Sucess)
	{
		auto* Memory = (EngineMemory*)_aligned_malloc(PRE_ALLOC_SIZE, 0x40);
		FK_ASSERT(Memory, "Memory Allocation Error!");

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

		return Memory;
	}


	void ReleaseEngineMemory(EngineMemory* Memory)
	{
		DEBUGBLOCK(PrintBlockStatus(&Memory->GetBlockMemory()));
		_aligned_free(Memory);
	}


	/************************************************************************************************/


	bool EngineCore::Initate(EngineMemory* Memory, uint2 WH)
	{
		bool Sucess						= false;
		FlexKit::Graphics_Desc	desc	= { 0 };
		desc.Memory						= GetBlockMemory();
		desc.TempMemory					= GetTempMemory();
		uint32_t width					= WH[0];
		uint32_t height					= WH[1];
		bool InvertDepth				= true;

		if (!RenderSystem.Initiate(&desc))
			return false;

		Window.Close = false;
		Sucess = CreateRenderWindow(this, height, width, false);

		if (!Sucess)
		{
			RenderSystem.Release();
			cout << "Failed to Create Render Window!\n";
			return false;
		}


		SetInputWIndow			(&Window);
		InitiatePhysics			(&Physics, gCORECOUNT, GetBlockMemory());
		InitiateResourceTable	(GetBlockMemory());

		FK_ASSERT(Sucess, "FAILED TO INITIATE ENGINE!");

		return Sucess;
	}



	/************************************************************************************************/


	void EngineCore::Release()
	{
		RenderSystem.ShutDownUploadQueues();


	#if USING(PHYSX)
			ReleasePhysics(&Physics);
	#endif
		FlexKit::Release(&Window);

		for (auto Arg : CmdArguments)
			GetBlockMemory().free((void*)Arg);

		CmdArguments.Release();
		RenderSystem.Release();

		Threads.Release();

		Memory = nullptr;
	}


	/************************************************************************************************/


	void UpdateCoreComponents(EngineCore* Core, double dt)
	{
		//Core->Cameras.Update(dt);
	}


	/************************************************************************************************/


	bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen)
	{
		// Initiate Render Window
		RenderWindowDesc	WinDesc = { 0 };
		WinDesc.POS_X		= 0;
		WinDesc.POS_Y		= 0;
		WinDesc.height		= height;
		WinDesc.width		= width;
		WinDesc.fullscreen	= fullscreen;

		if (CreateRenderWindow(Game->RenderSystem, &WinDesc, &Game->Window))
			FK_ASSERT(false, "RENDER WINDOW FAILED TO INITIALIZE!");

		return true;
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
			ShowCursor(true);
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

			State->NormalizedPos[0]			= max(0.0f, min((float)State->Position[0] / (float)Window->WH[0], 1));
			State->NormalizedPos[1]			= max(0.0f, min((float)State->Position[1] / (float)Window->WH[1], 1));
			State->NormalizedScreenCord		= Position2SS(State->NormalizedPos);

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