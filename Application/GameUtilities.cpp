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
	CleanupPhysics( &Engine->Physics );
#endif
	
	ReleaseForwardPass	( &Engine->ForwardRender  );
	ReleaseDeferredPass	( &Engine->DeferredRender );

	Release( &Engine->DepthBuffer );
	Release( &Engine->Window );
	CleanUp( &Engine->RenderSystem );


	ReleaseGeometryTable( &Engine->Geometry );

	DEBUGBLOCK(PrintBlockStatus(&Engine->BlockAllocator));
}


/************************************************************************************************/


void InitiateEngineMemory( EngineMemory* Game )
{
	using FlexKit::BlockAllocator_desc;
	using FlexKit::GetNewNode;
	using FlexKit::Graphics_Desc;

	BlockAllocator_desc BAdesc;
	BAdesc._ptr			= (byte*)Game->BlockMem;
	BAdesc.SmallBlock	= MEGABYTE * 64;
	BAdesc.MediumBlock	= MEGABYTE * 64;
	BAdesc.LargeBlock	= MEGABYTE * 256;

	Game->BlockAllocator.Init ( BAdesc );
	Game->LevelAllocator.Init ( Game->LevelMem,	LEVELBUFFERSIZE );
	Game->TempAllocator. Init ( Game->TempMem,	TEMPBUFFERSIZE );

	FlexKit::Graphics_Desc	desc	= { 0 };
	desc.Memory = Game->BlockAllocator;
	InitiateRenderSystem ( &desc, Game->RenderSystem );

	// Initate SceneGraph
	InitiateSceneNodeBuffer		( &Game->Nodes, Game->NodeMem, NODEBUFFERSIZE );
	Game->RootSN = GetNewNode	( &Game->Nodes );
	ZeroNode					( &Game->Nodes, Game->RootSN );
}


/************************************************************************************************/


void CreateRenderWindow(EngineMemory* Game, uint32_t height, uint32_t width, bool fullscreen = false)
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

	FK_ASSERT( CreateRenderWindow( Game->RenderSystem, &WinDesc, &Game->Window ), "RENDER WINDOW FAILED TO INITIALIZE!" );
}


/************************************************************************************************/


void InitiateCoreSystems(EngineMemory* Engine)
{
	using FlexKit::CreateDepthBuffer;
	using FlexKit::DepthBuffer;
	using FlexKit::DepthBuffer_Desc;
	using FlexKit::ForwardPass;
	using FlexKit::ForwardPass_DESC;

	uint32_t width	 = 1000;
	uint32_t height	 = 800;
	bool InvertDepth = true;

	Engine->Window.Close = false;
	CreateRenderWindow	(  Engine, height, width, false );
	CreateDepthBuffer	(  Engine->RenderSystem, { width, height }, DepthBuffer_Desc{3, InvertDepth, InvertDepth}, &Engine->DepthBuffer, GetCurrentCommandList(Engine->RenderSystem) );

	SetInputWIndow		( &Engine->Window );
	InitiatePhysics		( &Engine->Physics, gCORECOUNT, Engine->BlockAllocator );

	ForwardPass_DESC FP_Desc{ &Engine->DepthBuffer, &Engine->Window };
	DeferredPassDesc DP_Desc{ &Engine->DepthBuffer, &Engine->Window, nullptr };

	InitiateForwardPass(Engine->RenderSystem,	&FP_Desc, &Engine->ForwardRender);
	InitiateDeferredPass(Engine->RenderSystem,	&DP_Desc, &Engine->DeferredRender);

	InitiateGeometryTable	( &Engine->Geometry, Engine->BlockAllocator );
	Engine->Assets.ResourceMemory = &Engine->BlockAllocator;
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


void InitEngine( EngineMemory* Engine )
{
	memset( Engine, 0, PRE_ALLOC_SIZE );

	InitiateEngineMemory( Engine );
	InitiateCoreSystems ( Engine );
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
