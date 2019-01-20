#ifndef RESOURCES_H
#define RESOURCES_H

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

#include "Common.h"
#include "MetaData.h"

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Resources.h"
#include "..\graphicsutilities\AnimationUtilities.h"

#include <PhysX_sdk/physx/include/PxPhysicsAPI.h>

#include <random>
#include <limits>

using FlexKit::iAllocator;
using FlexKit::BlockAllocator;
using FlexKit::StackAllocator;
using FlexKit::static_vector;

struct Scene;
struct EngineMemory;

using FlexKit::Vector;
using FlexKit::EResourceType;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::ID_LENGTH;
using FlexKit::Drawable;
using FlexKit::Quaternion;
using FlexKit::Pair;
using FlexKit::Resource;
using FlexKit::RenderSystem;
using FlexKit::TriMesh;

struct FileDir
{	
	char str[256]; 
	bool Valid	= false;
};


/************************************************************************************************/


size_t CreateRandomID()
{
	std::random_device generator;
	std::uniform_int_distribution<size_t> distribution(
		std::numeric_limits<size_t>::min(),
		std::numeric_limits<size_t>::max());

	return distribution(generator);
}


/************************************************************************************************/


typedef FlexKit::Handle_t<16> SceneHandle;
typedef FlexKit::Handle_t<16> NodeHandle;
typedef FlexKit::Handle_t<16> ShaderSetHandle;

struct Scene_Desc
{
	size_t			MaxTriMeshCount		= 0;
	size_t			MaxEntityCount		= 0;
	size_t			MaxPointLightCount	= 0;
	size_t			MaxSkeletonCount	= 0;
	iAllocator*		SceneMemory			= nullptr;
	iAllocator*		AssetMemory			= nullptr;
	NodeHandle		Root;
	ShaderSetHandle DefaultMaterial;
};

struct SceneStats
{
	size_t EntityCount	= 0;
	size_t LightCount	= 0;

	SceneStats& operator += (const SceneStats& in)
	{
		EntityCount += in.EntityCount;
		LightCount	+= in.LightCount;
		return *this;
	}
};



/************************************************************************************************/


struct CompileSceneFromFBXFile_DESC
{
	FlexKit::iAllocator*	BlockMemory;
	FlexKit::iAllocator*	TempMem;
	FlexKit::iAllocator*	LevelMem;
	NodeHandle				SceneRoot;

	physx::PxCooking*		Cooker		= nullptr;
	physx::PxFoundation*	Foundation	= nullptr;

	bool					 SUBDIVEnabled;
	bool					 CloseFBX;
	bool					 IncludeShaders;
	bool					 CookingEnabled;
};

struct	LoadGeometry_RES;
typedef LoadGeometry_RES*				LoadGeometryRES_ptr;
typedef static_vector<Resource*, 256>	ResourceList;

FileDir SelectFile();


/************************************************************************************************/

#endif
