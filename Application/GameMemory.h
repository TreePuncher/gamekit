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

#pragma once

#include "..\buildsettings.h"

#include "..\coreutilities\timeutilities.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Resources.h"

#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\MeshUtils.h"

#include "..\PhysicsUtilities\physicsutilities.h"

/************************************************************************************************/

using namespace FlexKit;


/************************************************************************************************/


struct sKeyState
{
	sKeyState()
	{
		W = false;
		A = false;
		S = false;
		D = false;
		Q = false;
		E = false;
		P = false;
		R = false;
		F = false;
		SpaceBar = false;

		FrameID = 0;
	}

	char B[4];
	bool W;
	bool A;
	bool S;
	bool D;
	bool Q;
	bool E;
	bool P;
	bool R;
	bool F;
	bool SpaceBar;
	size_t FrameID;
};


/************************************************************************************************/


struct EngineMemory;

static const size_t MAX_CLIENTS = 10;
static const size_t SERVER_PORT = 60000;

static const size_t PRE_ALLOC_SIZE  = GIGABYTE * 1;
static const size_t LEVELBUFFERSIZE = MEGABYTE * 64;
static const size_t NODEBUFFERSIZE  = MEGABYTE * 256;
static const size_t TEMPBUFFERSIZE  = MEGABYTE * 128;
static const size_t BLOCKALLOCSIZE	= MEGABYTE * 512;


/************************************************************************************************/


struct EngineMemory
{
	BlockAllocator	BlockAllocator;
	StackAllocator	TempAllocator;
	StackAllocator	LevelAllocator;

	bool			End;
	RenderSystem	RenderSystem;

	RenderWindow	Window;
	DepthBuffer		DepthBuffer;
	Time			Time;
	Resources		Assets;
	GeometryTable	Geometry;

	PhysicsSystem	Physics;
	PScene*			CurrentScene;

	NodeHandle		RootSN;

	FlexKit::PVS			PVS_;
	FlexKit::ShaderTable	Materials;

	struct
	{
		FlexKit::ShaderSetHandle			DefaultMaterial;
		FlexKit::ShaderSetHandle			GenerateNormal;
	}BuiltInMaterials;

	// Move this to a resource Manager
	struct
	{
		FlexKit::ShaderHandle VertexShaderNoNormal;
		FlexKit::ShaderHandle VertexShaderPassthrough;
		FlexKit::ShaderHandle VertexPulling;
		FlexKit::ShaderHandle CalcNormalGshader;
		FlexKit::ShaderHandle OutputToGBuffer;
		FlexKit::ShaderHandle NormalMappedOutputToGbuffer;
		FlexKit::ShaderHandle PixelShaderDebug;
		FlexKit::ShaderHandle VertexPaletteSkinning;
		FlexKit::ShaderHandle VTextRendering;
		FlexKit::ShaderHandle GTextRendering;
		FlexKit::ShaderHandle PTextRendering;
	}ShaderHandles;
	static_vector<FlexKit::Event>	NetworkEvents;
					
	Shader	GShader;
	Shader	PShader;
	Shader	PDShader;				// Outputs white
	Shader	NormalMappedPShader;
	Shader	VShader;				// Passes Normals Through
	Shader	V2Shader;				// Passes BiTangents with Normals Through
	Shader	VPShader;				// 
	Shader	VertexPalletSkinning;	// No Bi-Tangent/Bi-Normal Passthrough 
	Shader	VTextRendering;			// Simple Text Rendering
	Shader	GTextRendering;			// Simple Text Rendering
	Shader	PTextRendering;			// Simple Text Rendering
	
	FlexKit::SceneNodes			Nodes;
	byte						NodeMem	[NODEBUFFERSIZE];
	byte						BlockMem[BLOCKALLOCSIZE];
	byte						LevelMem[LEVELBUFFERSIZE];
	byte						TempMem	[TEMPBUFFERSIZE];
};


/************************************************************************************************/


typedef void (*MouseEventHandler)		(EngineMemory* Engine, const FlexKit::Event& in);
typedef void*(*InitiateGameStateFN)		(EngineMemory* Engine);
typedef void (*InitiateEngineFN)		(EngineMemory* Engine);
typedef void (*OnHotReloadFN)			(EngineMemory*, double dt, void* _ptr);

typedef void (*UpdateFN)				(EngineMemory* Engine, void* _ptr, double dt);
typedef void (*FixedUpdateFN)			(EngineMemory*, double dt, void* _ptr);

typedef void (*UpdateAnimationsFN)		(RenderSystem* RS,		iAllocator* TempMemory, double dt, void* _ptr);
typedef	void (*UpdatePreDrawFN)			(EngineMemory* Engine,	iAllocator* TempMemory, double dt, void* _ptr); 
typedef	void (*DrawFN)					(RenderSystem* RS,		iAllocator* TempMemory, FlexKit::ShaderTable* M, void* _ptr); 
typedef void (*PostDrawFN)				(EngineMemory* Engine,	double dt,					void* _ptr);
typedef void (*CleanUpFN)				(EngineMemory* Engine,	void* _ptr);

typedef void (*PostPhysicsUpdateFN)	(void*);
typedef void (*PrePhysicsUpdateFN)	(void*);



/************************************************************************************************/


struct CodeTable
{
	InitiateGameStateFN	Init;
	InitiateEngineFN	InitEngine;
	UpdateFN			Update;
	FixedUpdateFN		UpdateFixed;
	UpdateAnimationsFN	UpdateAnimations;
	UpdatePreDrawFN		UpdatePreDraw;
	DrawFN				Draw;
	PostDrawFN			PostDraw;
	CleanUpFN			CleanUp;
};

typedef void (*GetStateTableFN)(CodeTable* out);


/************************************************************************************************/