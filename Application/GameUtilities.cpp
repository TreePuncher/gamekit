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

void CleanUpEngine(EngineMemory* Game)
{
#if USING(PHYSX)
	CleanupPhysics(&Game->Physics);
#endif
	
	FlexKit::Destroy(&Game->DepthBuffer);
	FlexKit::Destroy(&Game->Window);
	FlexKit::Destroy(&Game->GShader);
	FlexKit::Destroy(&Game->PShader);
	FlexKit::Destroy(&Game->NormalMappedPShader);
	FlexKit::Destroy(&Game->VShader);
	FlexKit::Destroy(&Game->V2Shader);
	FlexKit::Destroy(&Game->VPShader);
	FlexKit::Destroy(&Game->VertexPalletSkinning);
	FlexKit::Destroy(&Game->GTextRendering);
	FlexKit::Destroy(&Game->PTextRendering);
	FlexKit::Destroy(&Game->VTextRendering);
	FlexKit::CleanUp(&Game->RenderSystem);

	FreeGeometryTable(&Game->Geometry);

#ifdef _DEBUG
	FlexKit::PrintBlockStatus(&Game->BlockAllocator);
#endif
}


/************************************************************************************************/


void InitiateEngineMemory( EngineMemory* Game )
{
	FlexKit::BlockAllocator_desc BAdesc;
	BAdesc._ptr			= (byte*)Game->BlockMem;
	BAdesc.SmallBlock	= MEGABYTE * 64;
	BAdesc.MediumBlock	= MEGABYTE * 64;
	BAdesc.LargeBlock	= MEGABYTE * 256;

	Game->BlockAllocator.Init(BAdesc);
	Game->LevelAllocator.Init(Game->LevelMem,	LEVELBUFFERSIZE);
	Game->TempAllocator. Init(Game->TempMem,	TEMPBUFFERSIZE);

	FlexKit::Graphics_Desc	desc	= { 0 };
	desc.Memory = Game->BlockAllocator;
	InitiateRenderSystem(&desc, Game->RenderSystem);

	// Initate SceneGraph
	InitiateSceneNodeBuffer(&Game->Nodes, Game->NodeMem, NODEBUFFERSIZE);
	Game->RootSN = FlexKit::GetNewNode(&Game->Nodes);
	ZeroNode(&Game->Nodes, Game->RootSN);
}


/************************************************************************************************/


void CreateRenderWindow(EngineMemory* Game, uint32_t height, uint32_t width, bool fullscreen = false)
{
	// Initiate Render Window
	FlexKit::RenderWindowDesc	WinDesc = { 0 };
	WinDesc.POS_X	   = 400;
	WinDesc.POS_Y	   = 100;
	WinDesc.height	   = height;
	WinDesc.width	   = width;
	WinDesc.fullscreen = fullscreen;
	FK_ASSERT( FlexKit::CreateRenderWindow(Game->RenderSystem, &WinDesc, &Game->Window), "RENDER WINDOW FAILED TO INITIALIZE!");
}


/************************************************************************************************/


void LoadShaders(EngineMemory* Game)
{
	// TODO: use the Direct Load from File function instead so NSight can intercept and show the shader in the debugger
			
	Game->VShader				= LoadShader("VMain",				"VMain",				"vs_5_0", "assets\\vshader.hlsl");
	Game->V2Shader				= LoadShader("V2Main",				"V2Main",				"vs_5_0", "assets\\vshader.hlsl");
	Game->VPShader				= LoadShader("VMain",				"StaticMeshBatcher",	"vs_5_0", "assets\\StaticMeshBatcher.hlsl");
	Game->GShader				= LoadShader("VMainVertexPallet",	"VMainVertexPallet",	"vs_5_0", "assets\\vshader.hlsl");
	Game->PShader				= LoadShader("PMain",				"PMain",				"ps_5_0", "assets\\pshader.hlsl");
	Game->NormalMappedPShader	= LoadShader("PMainNormalMapped",	"PMainNormalMapped",	"ps_5_0", "assets\\pshader.hlsl");
	Game->GShader				= LoadShader("GSMain",				"GSMain",				"gs_5_0", "assets\\gshader.hlsl");

	// -----------------------------------------------------------------------------------
}


/************************************************************************************************/


void InitiateCoreSystems(EngineMemory* Engine)
{
	using FlexKit::DepthBuffer;
	using FlexKit::DepthBuffer_Desc;
	using FlexKit::ForwardPass;
	using FlexKit::ForwardPass_DESC;
	using FlexKit::CreateDepthBuffer;

	uint32_t width	 = 1600;
	uint32_t height	 = 1000;
	bool InvertDepth = true;

	Engine->Window.Close = false;
	CreateRenderWindow(Engine, height, width, false);
	CreateDepthBuffer(Engine->RenderSystem, { width, height }, DepthBuffer_Desc{3, InvertDepth, InvertDepth}, &Engine->DepthBuffer);
	SetInputWIndow(&Engine->Window);
	InitiatePhysics(&Engine->Physics, gCORECOUNT, Engine->BlockAllocator);

	ForwardPass_DESC fd;
	fd.OutputTarget = &Engine->Window;
	fd.DepthBuffer  = &Engine->DepthBuffer;

	InitiateGeometryTable(&Engine->Geometry, Engine->BlockAllocator);
	Engine->Assets.ResourceMemory = &Engine->BlockAllocator;
}


/************************************************************************************************/


void InitEngine( EngineMemory* Engine )
{
	memset(Engine, 0, PRE_ALLOC_SIZE );

	InitiateEngineMemory(Engine);
	InitiateCoreSystems (Engine);
	LoadShaders			(Engine);
}