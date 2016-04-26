#ifndef RESOURCES_H
#define RESOURCES_H

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

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\Resources.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\AnimationUtilities.h"

using FlexKit::BlockAllocator;
using FlexKit::StackAllocator;
using FlexKit::static_vector;
using FlexKit::fixed_vector;

struct Scene;
struct EngineMemory;

using FlexKit::EResourceType;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::GUID_t;
using FlexKit::ID_LENGTH;
using FlexKit::Entity;
using FlexKit::NodeHandle;
using FlexKit::Pair;
using FlexKit::PointLight;
using FlexKit::PointLightBuffer;
using FlexKit::Resource;
using FlexKit::RenderSystem;
using FlexKit::SceneNodes;
using FlexKit::TriMesh;

struct FileDir
{	
	char str[256]; 
	bool Valid	= false;
};

/************************************************************************************************/


typedef FlexKit::Handle_t<16> SceneHandle;
using FlexKit::ShaderSetHandle;

struct Scene_Desc
{
	size_t			MaxTriMeshCount		= 0;
	size_t			MaxEntityCount		= 0;
	size_t			MaxPointLightCount	= 0;
	size_t			MaxSkeletonCount	= 0;
	BlockAllocator*	SceneMemory			= nullptr;
	BlockAllocator*	AssetMemory			= nullptr;
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

// TODO: add Resource System instead of referencing Geometry Directly 
struct Scene
{
	size_t				MaxObjects		= 0;
	size_t				GeometryUsed	= 0;
	TriMesh*			Geometry		= nullptr;
	size_t				MaxEntities		= 0;
	size_t				EntitiesUsed	= 0;
	Entity*				Entities		= nullptr;
	size_t				AnimationCount	= 0;
	char*				EntityIDs		= nullptr;

	PointLightBuffer	PLightBuffer;
	NodeHandle			Root;		

	BlockAllocator*		Alloc;
	// TODO: DEBUG VERSIONS WITH CHECKS
	inline TriMesh*		GetTriMesh	( SceneHandle handle )	{ return Geometry	+ handle.INDEX;		 }
	inline Entity*		GetEntity	( SceneHandle handle )	{ return Entities	+ handle.INDEX;		 }
	inline PointLight*	GetPLight	( SceneHandle handle )	{ return &PLightBuffer[handle.INDEX];	 }
	inline char*		GetEntityID ( SceneHandle handle )  { return EntityIDs	+ handle.INDEX * 64; }

	Pair<bool, SceneHandle>
	GetFreeEntity()
	{
		if (EntitiesUsed < MaxEntities)
			return{ true, SceneHandle(EntitiesUsed++) };
		return{ false };
	}

	Pair<bool,SceneHandle>	
	GetFreeLight()
	{
		size_t Handle = PLightBuffer.size();
		if (PLightBuffer.max() > PLightBuffer.size())
		{
			PLightBuffer.push_back({float3(0, 0, 0), 0, 0});
			return{ true, (SceneHandle)Handle };
		}
		return{ false };
	}
	
	//bool			GetFreeTriMesh(SceneHandle&	out);

};


void InitiateScene	(Scene* out, RenderSystem* RS, BlockAllocator* memory, Scene_Desc* desc );
void UpdateScene	(Scene* In, RenderSystem* RS, SceneNodes* Nodes	);
void CleanUpScene	(Scene* scn, EngineMemory* memory );
//void AddSkeleton	(Scene* scn, Skeleton*, EngineMemory* engine			);

Pair<bool, SceneHandle>	SearchForMesh	(Scene* Scn, size_t TriMeshID );
TriMesh*				SearchForMesh	(Scene* Scene, const char* str);
Pair<bool, SceneHandle>	SearchForEntity (Scene* Scn, char* ID );

/************************************************************************************************/


struct LoadSceneFromFBXFile_DESC
{
	FlexKit::BlockAllocator* BlockMemory;
	FlexKit::BlockAllocator* AssetMemory; 
	FlexKit::StackAllocator* TempMem; 
	FlexKit::StackAllocator* LevelMem; 
	FlexKit::RenderSystem*	 RS; 
	FlexKit::ShaderTable*	 ST;
	FlexKit::NodeHandle		 SceneRoot;
	FlexKit::ShaderSetHandle DefaultMaterial;
	bool					 SUBDIVEnabled;
	bool					 CloseFBX;

	//EngineMemory*			 Engine;
};


struct CompileSceneFromFBXFile_DESC
{
	FlexKit::BlockAllocator* BlockMemory;
	FlexKit::StackAllocator* TempMem; 
	FlexKit::StackAllocator* LevelMem; 
	FlexKit::NodeHandle		 SceneRoot;

	bool					 SUBDIVEnabled;
	bool					 CloseFBX;
	bool					 IncludeShaders;
};


typedef static_vector<Resource*, 256>	ResourceList;


struct LoadGeometry_RES;
typedef LoadGeometry_RES* LoadGeometryRES_ptr;

FileDir SelectFile();

// RunTime Functions
Scene* LoadSceneFromFBXFile(char* AssetLocation, FlexKit::SceneNodes* Nodes, FlexKit::RenderSystem* RS, LoadSceneFromFBXFile_DESC* Desc);


/************************************************************************************************/

// Resource Compiler Functions
struct Meta_data
{
	enum class EMETAINFOTYPE
	{
		EMI_STRING,
		EMI_INT,
		EMI_GUID,
		EMI_FLOAT,
		EMI_DOUBLE,
		EMI_ANIMATION_CLIP
	};

	enum class EMETA_RECIPIENT_TYPE
	{
		EMR_MESH,
		EMR_SKELETON,
		EMR_SKELETALANIMATION,
	};

	size_t					size;
	EMETA_RECIPIENT_TYPE	type;
	char					ID[FlexKit::ID_LENGTH];
};

struct Mesh_Skeleton : public Meta_data
{
	char	SkeletonID[ID_LENGTH];// Specifies a Mesh to use a Specific Skeleton
};

struct Animation_Clip : public Meta_data
{
	char	MeshID[ID_LENGTH];// Mesh Name
	char	ClipID[ID_LENGTH];// Mesh Name
	double	T_Start;
	double	T_End;
};

struct Animation_Event : public Meta_data
{
	char	ClipID[ID_LENGTH];//
	double	EventT;// Location of Event relative to Beginning of Clip
};

// Replaces provided GUID with a specific one
struct Mesh_GUID : public Meta_data
{
	GUID_t	ID;
};

typedef fixed_vector<Meta_data*> MetaData_list;


/************************************************************************************************/

LoadGeometryRES_ptr CompileGeometryFromFBXFile(char* AssetLocation, LoadSceneFromFBXFile_DESC* Desc, MetaData_list* METAINFO = nullptr);

namespace FontUtilities
{
	typedef Pair<size_t, FontAsset*> LoadFontResult;

	LoadFontResult	LoadFontAsset(char* file, char* dir, RenderSystem* RS, StackAllocator* tempMem, StackAllocator* outMem);
	void			FreeFontAsset(FontAsset* asset);
}

#endif
