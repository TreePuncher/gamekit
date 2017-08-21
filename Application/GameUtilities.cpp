/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "stdafx.h"
#include "GameUtilities.h"
#include "..\graphicsutilities\AnimationUtilities.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\SSReflections.h"
#include <cstdio>

using namespace FlexKit;

void ReleaseCore(EngineCore* Engine)
{

}


/************************************************************************************************/


bool InitiateEngineMemory( EngineMemory*& Memory )
{
	using FlexKit::BlockAllocator_desc;
	using FlexKit::GetNewNode;
	using FlexKit::Graphics_Desc;

	Memory = (EngineMemory*)_aligned_malloc(PRE_ALLOC_SIZE, 0x40);
	memset(Memory, 0, PRE_ALLOC_SIZE);

	bool Out = false;
	BlockAllocator_desc BAdesc;
	BAdesc._ptr			= (byte*)Memory->BlockMem;
	BAdesc.SmallBlock	= MEGABYTE * 64;
	BAdesc.MediumBlock	= MEGABYTE * 64;
	BAdesc.LargeBlock	= MEGABYTE * 512;

	Memory->BlockAllocator.Init ( BAdesc );
	Memory->LevelAllocator.Init ( Memory->LevelMem,	LEVELBUFFERSIZE );
	Memory->TempAllocator. Init ( Memory->TempMem,	TEMPBUFFERSIZE );

	InitDebug(&Memory->Debug);

	return true;
}


/************************************************************************************************/


bool InitEngine(EngineCore*& Core, EngineMemory*& Memory)
{
	InitiateEngineMemory(Memory);

	Core = &Memory->BlockAllocator.allocate<EngineCore>(Memory);
	InitiateCoreSystems(Core);

	return true;
}


/************************************************************************************************/


void UpdateCoreComponents(EngineCore* Core, double dt)
{
	UpdateTransforms(Core->Nodes);
	Core->Cameras.Update(dt);
}


/************************************************************************************************/


bool CreateRenderWindow(EngineCore* Game, uint32_t height, uint32_t width, bool fullscreen = false)
{
	using FlexKit::CreateRenderWindow;
	using FlexKit::RenderWindowDesc;

	// Initiate Render Window
	RenderWindowDesc	WinDesc = { 0 };
	WinDesc.POS_X	   = 0;
	WinDesc.POS_Y	   = 0;
	WinDesc.height	   = height;
	WinDesc.width	   = width;
	WinDesc.fullscreen = fullscreen;

	if (CreateRenderWindow(Game->RenderSystem, &WinDesc, &Game->Window))
		FK_ASSERT(false, "RENDER WINDOW FAILED TO INITIALIZE!");

	return true;
}


/************************************************************************************************/


bool InitiateCoreSystems(EngineCore*& Engine)
{
	using FlexKit::CreateDepthBuffer;
	using FlexKit::DepthBuffer;
	using FlexKit::DepthBuffer_Desc;
	using FlexKit::ForwardRender;
	using FlexKit::ForwardPass_DESC;

	bool Out		 = false;
	uint32_t width	 = 1920;
	uint32_t height	 = 1080;
	bool InvertDepth = true;
	FlexKit::Graphics_Desc	desc = { 0 };
	desc.Memory = Engine->GetBlockMemory();


	Out = InitiateRenderSystem(&desc, Engine->RenderSystem);
	if (!Out)
		return false;

	Engine->Window.Close = false;
	Out = CreateRenderWindow	(  Engine, height, width, false );

	if (!Out)
	{
		Release(Engine->RenderSystem);
		return false;
	}

	CreateDepthBuffer	(  Engine->RenderSystem, { width, height }, DepthBuffer_Desc{3, InvertDepth, InvertDepth}, &Engine->DepthBuffer, GetCurrentCommandList(Engine->RenderSystem) );

	SetInputWIndow		( &Engine->Window );
	InitiatePhysics		( &Engine->Physics, gCORECOUNT, Engine->GetBlockMemory() );

	// Initate Component Systems
	//Engine->Cameras.InitiateSystem	(&Engine->RenderSystem, &Engine->Nodes, Engine->GetBlockMemory() );
	//Engine->Nodes.InitiateSystem	(Engine->Memory->NodeMem, NODEBUFFERSIZE);

	ForwardPass_DESC FP_Desc	{ &Engine->DepthBuffer, &Engine->Window };
	TiledRendering_Desc DP_Desc	{ &Engine->DepthBuffer, &Engine->Window, nullptr };

	InitiateForwardPass			(  Engine->RenderSystem, &FP_Desc, &Engine->ForwardRender);
	InitiateTiledDeferredRender	(  Engine->RenderSystem, &DP_Desc, &Engine->TiledRender);
	InitiateGeometryTable		( &Engine->Geometry,	Engine->GetBlockMemory() );
	InitiateSSReflectionTracer	(  Engine->RenderSystem, GetWindowWH(Engine), &Engine->Reflections);

	auto OcclusionBufferWH = GetWindowWH(Engine)/4;

	Engine->Assets.ResourceMemory = &Engine->GetBlockMemory();
	Engine->Culler                = CreateOcclusionCuller(Engine->RenderSystem, 8096, OcclusionBufferWH);


	return Out;
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
	using FlexKit::int2;

	if (!State->Enabled) {
		State->dPos = { 0, 0 };
		return;
	}

	if (GetForegroundWindow() == Window->hWindow)
	{
		State->dPos		   = GetMousedPos(Window);
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


/************************************************************************************************/