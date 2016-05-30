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

using FlexKit::iAllocator;
using FlexKit::BlockAllocator;
using FlexKit::StackAllocator;
using FlexKit::static_vector;
using FlexKit::fixed_vector;

struct Scene;
struct EngineMemory;

using FlexKit::DynArray;
using FlexKit::EResourceType;
using FlexKit::float2;
using FlexKit::float3;
using FlexKit::GUID_t;
using FlexKit::ID_LENGTH;
using FlexKit::Drawable;
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

// TODO: add Resource System instead of referencing Geometry Directly 
struct ResourceScene
{
	size_t				MaxObjects		= 0;
	size_t				GeometryUsed	= 0;
	TriMesh*			Geometry		= nullptr;
	size_t				MaxDrawables	= 0;
	size_t				DrawablesUsed	= 0;
	Drawable*			Drawables		= nullptr;
	size_t				AnimationCount	= 0;
	char*				EntityIDs		= nullptr;

	PointLightBuffer	PLightBuffer;
	NodeHandle			Root;		

	iAllocator*		Alloc;
	// TODO: DEBUG VERSIONS WITH CHECKS
	inline TriMesh*		GetTriMesh	( SceneHandle handle )	{ return Geometry	+ handle.INDEX;		 }
	inline Drawable*	GetEntity	( SceneHandle handle )	{ return Drawables	+ handle.INDEX;		 }
	inline PointLight*	GetPLight	( SceneHandle handle )	{ return &PLightBuffer[handle.INDEX];	 }
	inline char*		GetEntityID ( SceneHandle handle )  { return EntityIDs	+ handle.INDEX * 64; }

	Pair<bool, SceneHandle>
	GetFreeEntity()
	{
		if (DrawablesUsed < MaxDrawables)
			return{ true, SceneHandle(DrawablesUsed++) };
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


void InitiateScene	(ResourceScene* out, RenderSystem* RS, iAllocator* memory, Scene_Desc* desc );
void UpdateScene	(ResourceScene* In, RenderSystem* RS, SceneNodes* Nodes	);
void CleanUpScene	(ResourceScene* scn, EngineMemory* memory );
//void AddSkeleton	(Scene* scn, Skeleton*, EngineMemory* engine			);

Pair<bool, SceneHandle>	SearchForMesh	(ResourceScene* Scn, size_t TriMeshID );
TriMesh*				SearchForMesh	(ResourceScene* Scene, const char* str);
Pair<bool, SceneHandle>	SearchForEntity (ResourceScene* Scn, char* ID );

/************************************************************************************************/


struct LoadSceneFromFBXFile_DESC
{
	FlexKit::iAllocator*	 BlockMemory;
	FlexKit::iAllocator*	 AssetMemory; 
	FlexKit::iAllocator*	 TempMem; 
	FlexKit::iAllocator*	 LevelMem; 
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
ResourceScene* LoadSceneFromFBXFile(char* AssetLocation, FlexKit::SceneNodes* Nodes, FlexKit::RenderSystem* RS, LoadSceneFromFBXFile_DESC* Desc);


/************************************************************************************************/

// Resource Compiler Functions
struct MetaData
{
	enum class EMETAINFOTYPE
	{
		EMI_STRING,
		EMI_INT,
		EMI_GUID,
		EMI_FLOAT,
		EMI_DOUBLE,
		EMI_MESH,
		EMI_SKELETAL,
		EMI_SKELETALANIMATION,
		EMI_ANIMATIONCLIP,
		EMI_ANIMATIONEVENT,
	};

	enum class EMETA_RECIPIENT_TYPE
	{
		EMR_MESH,
		EMR_SKELETON,
		EMR_SKELETALANIMATION,
	};

	void SetID(char* Str, size_t StrSize)
	{
		memset(ID, 0x00, ID_LENGTH);

		
		strncpy(ID, Str, StrSize);

		for (auto I = StrSize; I > 0; --I)
		{
			if (ID[I] == ' ')
			{
				ID[I] = '\0';
				StrSize--;
			}
			else if (ID[I] == '\n')
			{
				ID[I] = '\0';
				StrSize--;
			}
		}
		ID[StrSize] = '\0';
		size = StrSize;
	}

	size_t					size;
	EMETA_RECIPIENT_TYPE	UserType;
	EMETAINFOTYPE			type;
	char					ID[ID_LENGTH];	// Specifies the Asset that uses the meta data
};

typedef FlexKit::DynArray<MetaData*> MD_Vector;

struct Skeleton_MetaData : public MetaData
{
	Skeleton_MetaData(){
		UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON;
		type		= MetaData::EMETAINFOTYPE::EMI_SKELETAL;
		size		= 0;
	}

	char	SkeletonID[ID_LENGTH];
	GUID_t	SkeletonGUID;			
};

struct AnimationClip_MetaData : public MetaData
{
	AnimationClip_MetaData() {
		UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
		type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP;
		size	 = 0;
	}

	char	ClipID[ID_LENGTH];// Mesh Name
	double	T_Start;
	double	T_End;
	GUID_t	guid;
};

struct AnimationEvent_MetaData : public MetaData
{
	AnimationEvent_MetaData() {
		UserType = MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETALANIMATION;
		type	 = MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT;
		size	 = 0;
	}

	char		ClipID[ID_LENGTH];//
	uint32_t	EventID;//
	double		EventT;// Location of Event relative to Beginning of Clip
};

// Replaces provided GUID with a specific one
struct Mesh_MetaData : public MetaData
{
	Mesh_MetaData(){
		UserType	= MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH;
		type		= MetaData::EMETAINFOTYPE::EMI_MESH;
		size		= 0;
	}

	char	MeshID[ID_LENGTH];//
	GUID_t	guid;
};

typedef DynArray<size_t> RelatedMetaData;

bool			ReadMetaData				(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MD_Vector& MD_Out);
RelatedMetaData	FindRelatedGeometryMetaData	(MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem);


/************************************************************************************************/


LoadGeometryRES_ptr CompileGeometryFromFBXFile(char* AssetLocation, LoadSceneFromFBXFile_DESC* Desc, MD_Vector* METAINFO = nullptr);

namespace TextUtilities
{
	typedef Pair<size_t, FontAsset*> LoadFontResult;

	LoadFontResult	LoadFontAsset(char* file, char* dir, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem);
	void			FreeFontAsset(FontAsset* asset);
}

#endif
