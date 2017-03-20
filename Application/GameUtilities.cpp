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

#include "stdafx.h"
#include "GameUtilities.h"
#include "..\graphicsutilities\AnimationUtilities.h"
#include "..\graphicsutilities\graphics.h"
#include <cstdio>

using namespace FlexKit;

void ReleaseEngine(EngineMemory* Engine)
{
	using FlexKit::Release;
	using FlexKit::PrintBlockStatus;

#if USING(PHYSX)
	ReleasePhysics( &Engine->Physics );
#endif
	
	ReleaseForwardPass	( &Engine->ForwardRender  );
	ReleaseTiledRender	( &Engine->TiledRender );

	Release( &Engine->DepthBuffer );
	Release( &Engine->Window );
	CleanUp( &Engine->RenderSystem );


	ReleaseGeometryTable( &Engine->Geometry );

	Engine->Culler.Release();

	DEBUGBLOCK(PrintBlockStatus(&Engine->BlockAllocator));
}


/************************************************************************************************/


void InitiateEngineMemory( EngineMemory* Engine )
{
	using FlexKit::BlockAllocator_desc;
	using FlexKit::GetNewNode;
	using FlexKit::Graphics_Desc;

	bool Out = false;
	BlockAllocator_desc BAdesc;
	BAdesc._ptr			= (byte*)Engine->BlockMem;
	BAdesc.SmallBlock	= MEGABYTE * 64;
	BAdesc.MediumBlock	= MEGABYTE * 64;
	BAdesc.LargeBlock	= MEGABYTE * 256;

	Engine->BlockAllocator.Init ( BAdesc );
	Engine->LevelAllocator.Init ( Engine->LevelMem,	LEVELBUFFERSIZE );
	Engine->TempAllocator. Init ( Engine->TempMem,	TEMPBUFFERSIZE );

	// Initate SceneGraph
	InitiateSceneNodeBuffer		( &Engine->Nodes, Engine->NodeMem, NODEBUFFERSIZE );
	Engine->RootSN = GetNewNode	( &Engine->Nodes );
	ZeroNode					( &Engine->Nodes, Engine->RootSN );

	Engine->CmdArguments.Allocator = Engine->BlockAllocator;
}


/************************************************************************************************/


bool CreateRenderWindow(EngineMemory* Game, uint32_t height, uint32_t width, bool fullscreen = false)
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


bool InitiateCoreSystems(EngineMemory* Engine)
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
	desc.Memory = Engine->BlockAllocator;


	Out = InitiateRenderSystem(&desc, Engine->RenderSystem);
	if (!Out)
		return false;

	Engine->Window.Close = false;
	Out = CreateRenderWindow	(  Engine, height, width, false );
	if (!Out)
	{
		CleanUp(Engine->RenderSystem);
		return false;
	}

	CreateDepthBuffer	(  Engine->RenderSystem, { width, height }, DepthBuffer_Desc{3, InvertDepth, InvertDepth}, &Engine->DepthBuffer, GetCurrentCommandList(Engine->RenderSystem) );

	SetInputWIndow		( &Engine->Window );
	InitiatePhysics		( &Engine->Physics, gCORECOUNT, Engine->BlockAllocator );

	ForwardPass_DESC FP_Desc{ &Engine->DepthBuffer, &Engine->Window };
	TiledRendering_Desc DP_Desc{ &Engine->DepthBuffer, &Engine->Window, nullptr };

	InitiateForwardPass(Engine->RenderSystem,	&FP_Desc, &Engine->ForwardRender);
	InitiateTiledDeferredRender(Engine->RenderSystem, &DP_Desc, &Engine->TiledRender);
	InitiateGeometryTable	( &Engine->Geometry, Engine->BlockAllocator );

	auto OcclusionBufferWH = GetWindowWH(Engine)/4;

	Engine->Assets.ResourceMemory = &Engine->BlockAllocator;
	Engine->Culler                = CreateOcclusionCuller(Engine->RenderSystem, 8096, OcclusionBufferWH);

	return Out;
}


/************************************************************************************************/


float GetWindowAspectRatio(EngineMemory* Engine)
{
	FK_ASSERT(Engine);

	float out = (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1];

	return out;
}


/************************************************************************************************/


uint2 GetWindowWH(EngineMemory* Engine)
{
	FK_ASSERT(Engine);
	
	return Engine->Window.WH;
}


/************************************************************************************************/


bool InitEngine( EngineMemory* Engine )
{
	memset( Engine, 0, PRE_ALLOC_SIZE );

	InitiateEngineMemory( Engine );
	return InitiateCoreSystems ( Engine );
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

		State->dPos = GetMousedPos(Window);
		State->Position.x -= State->dPos[0];
		State->Position.y += State->dPos[1];

		State->Position[0] = max(0, min(State->Position[0], Window->WH[0]));
		State->Position[1] = max(0, min(State->Position[1], Window->WH[1]));

		State->NormalizedPos[0] = max(0, min(State->Position[0] / Window->WH[0], 1));
		State->NormalizedPos[1] = max(0, min(State->Position[1] / Window->WH[1], 1));

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

void PushCmdArg(EngineMemory* Engine, const char* str)
{
	Engine->CmdArguments.push_back(str);
}