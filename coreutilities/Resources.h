/**********************************************************************

Copyright (c) 2014-2017 Robert May

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

#ifndef Resources_H
#define Resources_H

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\Fonts.h"
#include "..\graphicsutilities\AnimationUtilities.h"
#include "..\coreutilities\ResourceHandles.h"

#include <iostream>


/************************************************************************************************/


namespace FlexKit
{
	static const size_t ID_LENGTH = 64;

	class RenderSystem;
	struct TriMesh;
	struct TriMesh;
	struct TextureSet;

	enum EResourceType : size_t
	{
		EResource_Collider,
		EResource_Font,
		EResource_GameDB,
		EResource_Skeleton,
		EResource_SkeletalAnimation,
		EResource_Shader,
		EResource_Scene,
		EResource_TriMesh,
		EResource_TerrainCollider,
		EResource_Texture,
		EResource_TextureSet,
	};

	struct Resource
	{
		size_t			ResourceSize;
		EResourceType	Type;

	#pragma warning(disable:4200)

		GUID_t	GUID;
		enum	ResourceState : uint32_t
		{
			EResourceState_UNLOADED,
			EResourceState_LOADING,
			EResourceState_LOADED,
			EResourceState_EVICTED,
		};

		ResourceState	State;
		uint32_t		RefCount;

		char	ID[ID_LENGTH];
		char	Memory[];

	private:
		Resource();
	};

	/************************************************************************************************/


	struct ResourceEntry
	{
		GUID_t					GUID;
		size_t					ResourcePosition;
		char*					ResouceLOC; // Not Used in File
		EResourceType			Type;
		char					ID[ID_LENGTH];
	};

	struct ResourceTable
	{
		size_t			MagicNumber;
		size_t			Version;
		size_t			ResourceCount;
		ResourceEntry	Entries[];
	};

	/************************************************************************************************/



	struct ResourceDirectory
	{
		char str[256];
	};
	
	struct GlobalResourceTable
	{
		~GlobalResourceTable()
		{
			Tables.A			= nullptr;
			ResourceFiles.A		= nullptr;
			ResourcesLoaded.A	= nullptr;
			ResourceGUIDs.A		= nullptr;

			Tables.Allocator			= nullptr;
			ResourceFiles.Allocator		= nullptr;
			ResourcesLoaded.Allocator	= nullptr;
			ResourceGUIDs.Allocator		= nullptr;
		}

		Vector<ResourceTable*>		Tables;
		Vector<ResourceDirectory>	ResourceFiles;
		Vector<Resource*>			ResourcesLoaded;
		Vector<GUID_t>				ResourceGUIDs;
		iAllocator*					ResourceMemory;
	}Resources;


	/************************************************************************************************/


	FLEXKITAPI void			InitiateResourceTable	(iAllocator* Memory);
	FLEXKITAPI void			ReleaseResourceTable	();

	FLEXKITAPI size_t		ReadResourceTableSize	(FILE* F);
	FLEXKITAPI size_t		ReadResourceSize		(FILE* F, ResourceTable* Table, size_t Index);

	FLEXKITAPI void					AddResourceFile		(char* FILELOC);
	FLEXKITAPI Resource*			GetResource			(ResourceHandle RHandle);
	FLEXKITAPI Pair<GUID_t, bool>	FindResourceGUID	(char* Str);


	FLEXKITAPI bool			ReadResourceTable	(FILE* F, ResourceTable* Out, size_t TableSize);
	FLEXKITAPI bool			ReadResource		(FILE* F, ResourceTable* Table, size_t Index, Resource* out);

	FLEXKITAPI ResourceHandle LoadGameResource (const char* ID);
	FLEXKITAPI ResourceHandle LoadGameResource (GUID_t GUID);

	FLEXKITAPI void FreeResource			(ResourceHandle RHandle);
	FLEXKITAPI void FreeAllResources		();
	FLEXKITAPI void FreeAllResourceFiles	();

	FLEXKITAPI bool isResourceAvailable		(GUID_t ID);
	FLEXKITAPI bool isResourceAvailable		(const char* ID);


	/************************************************************************************************/


	const size_t GUIDMASK		= 0x00000000FFFFFFFF;


	/************************************************************************************************/


	struct TriMeshResourceBlob
	{
		#pragma warning(disable:4200)

		size_t			ResourceSize;
		EResourceType	Type;
		GUID_t			GUID;
		size_t			Pad;

		char	ID[FlexKit::ID_LENGTH];
		bool	HasAnimation;
		bool	HasIndexBuffer;
		size_t	BufferCount;
		size_t	IndexCount;
		size_t	SkeletonGuid;
		GUID_t	ColliderGuid;

		struct RInfo
		{
			float minx; 
			float miny; 
			float minz;
			float maxx;
			float maxy;
			float maxz;
			float r;
			byte   _PAD[12];
		}Info;

		float  BS[4];// Uses Float Array instead of float4, float4 requires alignment 
		float  AABB[6];

		struct Buffer
		{
			uint16_t Format;
			uint16_t Type;
			size_t	 Begin;
			size_t	 size;
		}Buffers[16];
		char Memory[];
	};


	/************************************************************************************************/


	struct SkeletonResourceBlob
	{
		#pragma warning(disable:4200)

		size_t			ResourceSize;
		EResourceType	Type;
		size_t			GUID;
		size_t			Pad;

		char ID[FlexKit::ID_LENGTH];

		struct JointEntry
		{
			FlexKit::float4x4		IPose;
			FlexKit::JointPose		Pose;
			FlexKit::JointHandle	Parent;
			uint16_t				Pad;
			char					ID[64];
		};

		size_t JointCount;
		JointEntry Joints[];
	};


	/************************************************************************************************/

	struct FontResourceBlob
	{
#pragma warning(disable:4200)

		size_t			ResourceSize;
		EResourceType	Type;
		GUID_t			GUID;
		size_t			Pad;

		char	ID[FlexKit::ID_LENGTH];
	};

	/************************************************************************************************/


	struct AnimationResourceBlob
	{
		size_t			ResourceSize;
		EResourceType	Type;

	#pragma warning(disable:4200)

		GUID_t					GUID;
		Resource::ResourceState	State;
		uint32_t				RefCount;

		char   ID[FlexKit::ID_LENGTH];

		GUID_t Skeleton;
		size_t FrameCount;
		size_t FPS;
		bool   IsLooping;

		struct FrameEntry
		{
			size_t JointCount;
			size_t PoseCount;
			size_t JointStarts;
			size_t PoseStarts;
		};

		char	Buffer[];
	};

	struct TextureSet_Locations
	{
		GUID_t TextureID[16];
		struct {
			char Directory[64];
		}TextureLocation[16];
	};

	struct TextureSetBlob
	{
		TextureSetBlob()
		{
			ResourceSize	= sizeof(TextureSetBlob);
			Type			= EResourceType::EResource_TextureSet;
			State			= Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
		}

	#pragma warning(disable:4200)

		size_t					ResourceSize;
		EResourceType			Type;
		GUID_t					GUID;
		Resource::ResourceState	State;		// Runtime Member
		uint32_t				RefCount;	// Runtime Member

		char	ID[ID_LENGTH];
		struct TextureEntry
		{
			GUID_t	guid;
			char	Directory[64];
		}Textures[16];
	};


	/************************************************************************************************/


	struct CompiledScene
	{
		struct SceneNode
		{
			SceneNode& operator = (const SceneNode& RHS)
			{
				Q = RHS.Q;
				TS = RHS.TS;
				Parent = RHS.Parent;
				return *this;
			}

			Quaternion	Q;
			float4		TS;
			size_t		Parent;
			size_t		pad;
		};

		struct PointLight
		{
			float	I, R;
			float3	K;
			size_t	Node;
		};

		struct Entity
		{
			GUID_t		MeshGuid;
			GUID_t		TextureSet;
			size_t		Node;
			GUID_t		Collider;
			size_t		idlength;
			//float4		albedo;
			//float4		specular;
			const char*	id;
		};

		struct SceneGeometryTable
		{
			size_t	FBXTriMeshID;
			GUID_t	Guid;
			char*	ID;
		};

		Vector<SceneNode>				Nodes;
		Vector<PointLight>			SceneLights;
		Vector<SceneGeometryTable>	SceneGeometry;
		Vector<Entity>				SceneEntities;
		Vector<Entity>				SceneStatics;
		GUID_t							Guid;
		char							ID[64];
		size_t							IDSize;
	};

	typedef Vector<CompiledScene*> SceneList;


	/************************************************************************************************/


	struct SceneResourceBlob
	{
		SceneResourceBlob()
		{
			ResourceSize	= sizeof(TextureSetBlob);
			Type			= EResourceType::EResource_Scene;
			State			= Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
		}

#pragma warning(disable:4200)

		size_t					ResourceSize;
		EResourceType			Type;
		GUID_t					GUID;
		Resource::ResourceState	State;		// Runtime Member
		uint32_t				RefCount;	// Runtime Member

		char	ID[ID_LENGTH];

		struct {
			size_t EntityCount;
			size_t EntityOffset;

			size_t LightCount;
			size_t LightOffset;

			size_t NodeCount;
			size_t NodeOffset;

			size_t StaticsCount;
			size_t StaticsOffset;

			size_t SceneStringCount;
			size_t SceneStringsOffset;
		}SceneTable;
		
		char Buffer[];
	};


	/************************************************************************************************/


	struct ColliderResourceBlob
	{
#pragma warning(disable:4200)
		size_t					ResourceSize;
		EResourceType			Type;
		GUID_t					GUID;
		Resource::ResourceState	State;		// Runtime Member
		uint32_t				RefCount;	// Runtime Member

		char ID[ID_LENGTH];
		byte Buffer[];
	};



	/************************************************************************************************/


	FLEXKITAPI AnimationClip	Resource2AnimationClip	( Resource* R, iAllocator* Memory );
	FLEXKITAPI Skeleton*		Resource2Skeleton		( ResourceHandle RHandle, iAllocator* Memory );
	FLEXKITAPI bool				Resource2TriMesh		( RenderSystem* RS, ResourceHandle RHandle, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true );
	FLEXKITAPI TextureSet*		Resource2TextureSet		( ResourceHandle RHandle, iAllocator* Memory );

	FLEXKITAPI TextureSet*		LoadTextureSet	 ( GUID_t ID, iAllocator* Memory );
	FLEXKITAPI void				LoadTriangleMesh ( GUID_t ID, iAllocator* Memory, TriMesh* out );

	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable ( RenderSystem* RS, size_t guid );
	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable ( RenderSystem* RS, const char* ID );

	typedef Pair<size_t, FlexKit::SpriteFontAsset*> LoadFontResult;

	FLEXKITAPI LoadFontResult	LoadFontAsset	( char* file, char* dir, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem );
	FLEXKITAPI void				Release			( SpriteFontAsset* asset, RenderSystem* RS);


	/************************************************************************************************/


	inline size_t ReadResourceTableSize(FILE* F)
	{
		byte Buffer[128];

		int s = fseek(F, 0, SEEK_SET);
		s = fread(Buffer, 1, 128, F);

		ResourceTable* T = (ResourceTable*)Buffer;
		return T->ResourceCount * sizeof(ResourceEntry) + sizeof(ResourceTable);
	}


	/************************************************************************************************/


	inline bool ReadResourceTable(FILE* F, ResourceTable* Out, size_t TableSize)
	{
		int s = fseek(F, 0, SEEK_SET);
		s = fread(Out, 1, TableSize, F);
		return (s == TableSize);
	}


	/************************************************************************************************/


	inline size_t ReadResourceSize(FILE* F, ResourceTable* Table, size_t Index)
	{
		byte Buffer[8];

		int s = fseek(F, Table->Entries[Index].ResourcePosition, SEEK_SET);
		s = fread(Buffer, 1, 8, F);

		Resource* R = (Resource*)Buffer;
		return R->ResourceSize;
	}


	/************************************************************************************************/


	inline bool ReadResource(FILE* F, ResourceTable* Table, size_t Index, Resource* out)
	{
		size_t ResourceSize = 0;
		size_t Position = Table->Entries[Index].ResourcePosition;

#if _DEBUG
		std::chrono::system_clock Clock;
		auto Before = Clock.now();
		FINALLY
			auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::microseconds>( After - Before );
			std::cout << "Loading Resource: " << Table->Entries[Index].ID << " : ResourceID: "<< Table->Entries[Index].GUID << "\n";
			std::cout << "Resource Load Duration: " << Duration.count() << "microseconds\n";
		FINALLYOVER
#endif

		int s = fseek(F, Position, SEEK_SET);
		s = fread(&ResourceSize, 1, 8, F);

		s = fseek(F, Position, SEEK_SET);
		s = fread(out, 1, ResourceSize, F);

		return (s == out->ResourceSize);
	}

	/************************************************************************************************/
}

#endif