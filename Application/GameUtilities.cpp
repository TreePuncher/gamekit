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


Pair<bool, FlexKit::Shader>
LoadVShader( RenderSystem* RS, char* loc, FlexKit::ShaderDesc& desc )
{
	FlexKit::Shader	S;
	bool res = FlexKit::LoadVertexShaderFromFile(RS, loc, &desc, &S);
	if (!res)
	{
		std::cout << "Failed to Load Shader\nPress Enter to try again\n";
		char str[100];
		std::cin >> str;
	}
	return{ res, S };
}

Pair<bool, FlexKit::Shader>
LoadGShader( RenderSystem* RS, char* loc, FlexKit::ShaderDesc& desc )
{
	FlexKit::Shader	S;
	bool res = FlexKit::LoadGeometryShaderFromFile(RS, loc, &desc, &S);
	if (!res)
	{
		std::cout << "Failed to Load Shader\nPress Enter to try again\n";
		char str[100];
		std::cin >> str;
	}
	return{ res, S };
}

Pair<bool, FlexKit::Shader>
LoadPShader( RenderSystem* RS, char* loc, FlexKit::ShaderDesc& desc )
{
	FlexKit::Shader	S;
	bool res = FlexKit::LoadPixelShaderFromFile(RS, loc, &desc, &S);
	if (!res)
	{
		std::cout << "Failed to Load Shader\nPress Enter to try again\n";
		char str[100];
		std::cin >> str;
	}
	return{ res, S };
}

void LoadShaders(EngineMemory* Game)
{
		FlexKit::ShaderDesc desc;
		// TODO: use the Direct Load from File function instead so NSight can intercept and show the shader in the debugger
	#if USING( DEBUGGRAPHICS )
		const size_t BufferSize = 1024 * 1024;
		char* TEMP = (char*)malloc(BufferSize);

		bool Success = true;
		do
		{
			strcpy_s(desc.entry, 16, "VMain");
			strcpy_s(desc.shaderVersion, 16, "vs_5_0");
			memset(TEMP, 0, BufferSize);
			Success = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->VShader);

			if (!Success)
			{
				char temp[128];
				std::cout << "Press Enter To Reload\n";
				std::cin >> temp;
			}
		} while (!Success);
		
		Success = true;
		do
		{
			strcpy_s(desc.entry, 16, "V2Main");
			Success = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->V2Shader);
			if (!Success)
			{
				FlexKit::Destroy(&Game->VShader);
				char temp[128];
				std::cout << "Press Enter To Reload\n";
				std::cin >> temp;
			}

		} while (!Success);

		Success = true;
		do
		{
			strcpy_s(desc.entry, 16, "VMain");
			strcpy_s(desc.shaderVersion, 16, "vs_5_0");
			Success = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\StaticMeshBatcher.hlsl", &desc, &Game->VPShader);

			if (!Success)
			{
				char temp[128];
				std::cout << "Press Enter To Reload\n";
				std::cin >> temp;
			}
		} while (!Success);

		Success = true;
		do
		{
			strcpy_s(desc.entry, 16, "PMain");
			strcpy_s(desc.shaderVersion, 16, "ps_5_0");
			Success = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->PShader);

			if (!Success)
			{
				char temp[128];
				std::cout << "Press Enter To Reload: PMain pshader.hlsl \n";
				std::cin >> temp;
			}
		} while (!Success);

		Success = true;
		do
		{
			strcpy_s(desc.entry, 128, "PMainNormalMapped");
			strcpy_s(desc.shaderVersion, 16, "ps_5_0");
			Success = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->NormalMappedPShader);

			if (!Success)
			{
				char temp[128];
				std::cout << "Press Enter To Reload: PMainNormalMapped pshader.hlsl \n";
				std::cin >> temp;
			}
		} while (!Success);


		Success = true;
		do
		{
			strcpy_s(desc.entry, 16, "GSMain");
			strcpy_s(desc.shaderVersion, 16, "gs_5_0");
			Success = FlexKit::LoadGeometryShaderFromFile(Game->RenderSystem, "assets\\gshader.hlsl", &desc, &Game->GShader);

			if (!Success)
			{
				char temp[128];
				std::cout << "Press Enter To Reload: GSMain gshader.hlsl\n";
				std::cin >> temp;
			}
		} while (!Success);
		free(TEMP);

		bool res;
#else
		bool res;
		strcpy_s(desc.entry, 128, "VMain");
		strcpy_s(desc.shaderVersion, 16, "vs_5_0");
#ifdef _DEBUG
		res = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->VShader);
		strcpy_s(desc.entry, 128, "V2Main");
		res = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->V2Shader);
#else
		res = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->VShader);

		strcpy_s(desc.entry, 128, "V2Main");
		res = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\vshader.hlsl", &desc, &Game->V2Shader);
#endif

		strcpy_s(desc.entry, 128, "PMain");
		strcpy_s(desc.shaderVersion, 16, "ps_5_0");
#ifdef _DEBUG
		res = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->PShader);
#else
		res = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->PShader);
#endif

		do
		{
			strcpy_s(desc.entry, 128, "PMainNormalMapped");
			strcpy_s(desc.shaderVersion, 16, "ps_5_0");
			res = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->NormalMappedPShader);
			if (!res)
			{
				std::cout << "Failed to Load Shader\nPress Enter to try again\n";
				char str[100];
				std::cin >> str;
			}
		} while (!res);

		do
		{
			strcpy_s(desc.entry, 128, "DebugPaint");
			strcpy_s(desc.shaderVersion, 16, "ps_5_0");
			res = FlexKit::LoadPixelShaderFromFile(Game->RenderSystem, "assets\\pshader.hlsl", &desc, &Game->PDShader);
			if (!res)
			{
				std::cout << "Failed to Load Shader\nPress Enter to try again\n";
				char str[100];
				std::cin >>  str;
			}
		} while (!res);

		strcpy_s(desc.entry, 128, "GSMain");
		strcpy_s(desc.shaderVersion, 16, "gs_5_0");

#ifdef _DEBUG
		res = FlexKit::LoadGeometryShaderFromFile(Game->RenderSystem, "assets\\gshader.hlsl", &desc, &Game->GShader);
#else
		res = FlexKit::LoadGeometryShaderFromFile(Game->RenderSystem, "assets\\gshader.hlsl", &desc, &Game->GShader);
#endif
		if(!res)
		{
			std::cout << "Failed to Load Shader\nPress Enter to try again\n";
			char str[100];
			std::cin >> str;
		}

		do
		{
			strcpy_s(desc.entry, 128, "VMain");
			strcpy_s(desc.shaderVersion, 16, "vs_5_0");
			res = FlexKit::LoadVertexShaderFromFile(Game->RenderSystem, "assets\\StaticMeshBatcher.hlsl", &desc, &Game->VPShader);
			if (!res)
			{
				std::cout << "Failed to Load Shader\nPress Enter to try again\n";
				char str[100];
				std::cin >>  str;
			}
		} while (!res);
#endif

	do
	{
		strcpy_s(desc.entry, 128, "VMainVertexPallet");
		strcpy_s(desc.shaderVersion, 16, "vs_5_0");
		auto result = LoadVShader( Game->RenderSystem, "assets\\vshader.hlsl", desc );
		Game->VertexPalletSkinning = result;
		res = result;

		if (!res)
		{
			std::cout << "Failed to Load Shader\nPress Enter to try again\n";
			char str[100];
			std::cin >>  str;
		}
	} while (!res);

	// -----------------------------------------------------------------------------------


	Shader NullGShader;
	NullGShader.Type = FlexKit::SHADER_TYPE::SHADER_TYPE_Geometry;

	auto GShader						= Game->Materials.AddShader(Game->GShader);
	auto NGShader						= Game->Materials.AddShader(NullGShader);
	auto TOGSHADER						= Game->Materials.AddShader(Game->VShader);
	auto TOPSHADER						= Game->Materials.AddShader(Game->V2Shader);
	auto GBUFFEROUT						= Game->Materials.AddShader(Game->PShader);
	auto NormalMappedOutputToGbuffer	= Game->Materials.AddShader(Game->NormalMappedPShader);
	auto DebugPixelShader				= Game->Materials.AddShader(Game->PDShader);
	auto VertexPullingShader			= Game->Materials.AddShader(Game->VPShader);
	auto VertexPaletteSkinning			= Game->Materials.AddShader(Game->VertexPalletSkinning);
	auto GTextRendering					= Game->Materials.AddShader(Game->GTextRendering);
	auto PTextRendering					= Game->Materials.AddShader(Game->PTextRendering);
	auto VTextRendering					= Game->Materials.AddShader(Game->VTextRendering);

	Game->ShaderHandles.CalcNormalGshader			= GShader;
	Game->ShaderHandles.OutputToGBuffer				= GBUFFEROUT;
	Game->ShaderHandles.VertexShaderPassthrough		= TOPSHADER;
	Game->ShaderHandles.VertexShaderNoNormal		= TOGSHADER;
	Game->ShaderHandles.NormalMappedOutputToGbuffer = NormalMappedOutputToGbuffer;
	Game->ShaderHandles.PixelShaderDebug			= DebugPixelShader;
	Game->ShaderHandles.VertexPulling				= VertexPullingShader;
	Game->ShaderHandles.VertexPaletteSkinning		= VertexPaletteSkinning;
	Game->ShaderHandles.VTextRendering				= VTextRendering;
	Game->ShaderHandles.PTextRendering				= PTextRendering;
	Game->ShaderHandles.GTextRendering				= GTextRendering;

	FlexKit::ShaderTable::ShaderSet SS;
	SS.GShader			= NGShader;
	SS.PShader			= GBUFFEROUT;
	SS.VShader			= TOPSHADER;
	SS.VShader_Animated = VertexPaletteSkinning;
	size_t ShaderSet1	= Game->Materials.RegisterShaderSet(SS);

	SS.GShader			= GShader;
	SS.PShader			= GBUFFEROUT;
	SS.VShader			= TOGSHADER;
	size_t ShaderSet2	= Game->Materials.RegisterShaderSet(SS);

	auto Mat1 = Game->Materials.GetNewShaderSet();
	auto Mat2 = Game->Materials.GetNewShaderSet();

	Game->Materials.SetSSet(Mat1, ShaderSet1 );
	Game->Materials.SetSSet(Mat2, ShaderSet2 );

	Game->Materials.SetDefaultMaterial(Mat1);
	Game->BuiltInMaterials.DefaultMaterial	= Mat1;
	Game->BuiltInMaterials.GenerateNormal	= Mat2;
}


/************************************************************************************************/


void InitiateCoreSystems(EngineMemory* Engine)
{
	using FlexKit::DepthBuffer;
	using FlexKit::DepthBuffer_Desc;
	using FlexKit::ForwardPass;
	using FlexKit::ForwardPass_DESC;
	using FlexKit::CreateDepthBuffer;
	using TextUtilities::InitiateTextRender;

	uint32_t width	 = 1600;
	uint32_t height	 = 1000;
	bool InvertDepth = true;

	Engine->Window.Close = false;
	CreateRenderWindow(Engine, height, width, false);
	CreateDepthBuffer(Engine->RenderSystem, { width, height }, DepthBuffer_Desc{3, InvertDepth, InvertDepth}, &Engine->DepthBuffer);
	SetInputWIndow(&Engine->Window);
	InitiatePhysics(&Engine->Physics, gCORECOUNT);

	ForwardPass_DESC fd;
	fd.OutputTarget = &Engine->Window;
	fd.DepthBuffer  = &Engine->DepthBuffer;

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