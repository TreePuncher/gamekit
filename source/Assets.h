#ifndef RESOURCES_H
#define RESOURCES_H

#include "buildsettings.h"
#include "containers.h"
#include "memoryutilities.h"
#include "Fonts.h"
#include "ResourceHandles.h"
#include "TextureUtilities.h"
#include "intersection.h"

#define WINDOW_LEAN_AND_MEAN

#include <variant>
#include <iostream>
#include <Windows.h>


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
		EResource_Animation,
		EResource_ByteCode,
		EResource_Collider,
		EResource_CubeMapTexture,
		EResource_Font,
		EResource_GameDB,
		EResource_Prefab,
		EResource_Skeleton,
		EResource_SkeletalAnimation,
		EResource_Shader,
		EResource_Scene,
		EResource_TriMesh,
		EResource_TerrainCollider,
		EResource_Texture,
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

	protected:
		Resource() = default;
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

	using AssetIdentifier		= std::variant<const char*, GUID_t>;
	using AssetFailureHandler	= TypeErasedCallable<AssetHandle (AssetIdentifier)>;
	struct ResourceTable;

	FLEXKITAPI void			InitiateAssetTable	(iAllocator* Memory);
	FLEXKITAPI void			ReleaseAssetTable	();

	FLEXKITAPI size_t		ReadAssetTableSize	    (FILE* F);
	FLEXKITAPI size_t		ReadAssetSize		    (FILE* F, ResourceTable* Table, size_t Index);

	FLEXKITAPI void					    AddAssetFile	(const char* FILELOC);
	FLEXKITAPI AssetHandle			    AddAssetBuffer	(Resource*);            // Will increment resource refcount
	FLEXKITAPI Resource*			    GetAsset		(AssetHandle RHandle);
	FLEXKITAPI std::optional<GUID_t>    FindAssetGUID	(const char* Str);

	FLEXKITAPI bool			ReadAssetTable	(FILE* F, ResourceTable* Out, size_t TableSize);
	FLEXKITAPI bool			ReadResource	(FILE* F, ResourceTable* Table, size_t Index, Resource* out);

	FLEXKITAPI AssetHandle LoadGameAsset (const char* ID);  // Asset refcount starts at 1
	FLEXKITAPI AssetHandle LoadGameAsset (GUID_t GUID);     // Asset refcount starts at 1

	FLEXKITAPI void FreeAsset			    (AssetHandle RHandle);
	FLEXKITAPI void FreeAllAssets		();
	FLEXKITAPI void FreeAllAssetFiles	();

	FLEXKITAPI bool isAssetAvailable		(GUID_t ID);
	FLEXKITAPI bool isAssetAvailable		(const char* ID);

	FLEXKITAPI void SetLoadFailureHandler   (AssetFailureHandler handler);


	/************************************************************************************************/


	const size_t GUIDMASK		= 0x00000000FFFFFFFF;


	/************************************************************************************************/


	struct Face
	{
		size_t MipCount;

		size_t GetOffset(size_t idx)
		{
			return ((size_t*)this)[1 + idx];
		}

		char* GetMip(size_t mipLevel)
		{
			return ((char*)this) + GetOffset(mipLevel);
		}


		size_t GetMipSize(size_t mipLevel)
		{
			return ((size_t*)this)[1 + MipCount + mipLevel];
		}
	};

	struct CubeMapAssetBlob
	{
		size_t			ResourceSize;
		EResourceType	Type;
		GUID_t			GUID;
		size_t			Pad;

		size_t          Width;
		size_t          Height;
		size_t          MipCount;

		size_t          Format;
		size_t          Offset[6];

		Face*    GetFace(size_t faceIdx)
		{
			return reinterpret_cast<Face*>(((char*)this) + Offset[faceIdx]);
		}
	};


	/************************************************************************************************/


	struct SubMesh
	{
		uint32_t    BaseIndex;
		uint32_t    IndexCount;
		AABB        aabb;
	};

	struct LODlevel
	{
		struct Buffer
		{
			uint16_t Format;
			uint16_t Type;
			size_t	 Begin;
			size_t	 size;
		};

		struct LODlevelDesciption
		{
			size_t bufferOffset    = 0;
			size_t subMeshCount    = 0;
			size_t morphTargets    = 0; 
			static_vector<Buffer> buffers;
		} descriptor;

		struct LODMorphTarget
		{
			char    morphTargetName[32];
			size_t  bufferOffset    = 0;
			size_t  buffserSize     = 0;
		};

		//SubMesh subMeshes[];
	};

	struct LODEntry
	{
		size_t size;
		size_t offset;
	};

	struct LODTable
	{
		size_t		LODcount;
		LODEntry	lodOffsets[];
	};


	struct TriMeshAssetBlob
	{
		struct TriMeshAssetHeader
		{
			size_t			ResourceSize;
			EResourceType	Type;
			GUID_t			GUID;
			size_t			Pad;

			char	ID[FlexKit::ID_LENGTH];
			bool	HasIndexBuffer;
			size_t	LODCount;
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
				byte	_PAD[12];
			}Info;

			float  BS[4];// Uses Float Array instead of float4, float4 requires alignment 
			float  AABB[6];

		}	header;
	};


	bool LoadLOD				(TriMesh* triMesh, uint level, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);
	bool LoadAllLODFromMemory	(TriMesh* triMesh, const char* buffer, const size_t bufferSize, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);


	/************************************************************************************************/


	struct FontAssetBlob
	{
		size_t			ResourceSize;
		EResourceType	Type;
		GUID_t			GUID;
		size_t			Pad;

		char	ID[FlexKit::ID_LENGTH];
	};


	/************************************************************************************************/


	struct CompiledScene
	{
		struct SceneNode
		{
			Quaternion	Q			= { 0, 0, 0, 1 };
			float3		position	= { 0, 0, 0 };
			float3		scale		= { 1, 1, 1 };
			size_t		Parent		= INVALIDHANDLE;
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
			float4		albedo;
			float4		specular;
			const char*	id;
		};

		struct SceneGeometryTable
		{
			size_t	FBXTriMeshID;
			GUID_t	Guid;
			char*	ID;
		};

		Vector<SceneNode>				Nodes			= { SystemAllocator };
		Vector<PointLight>				SceneLights		= { SystemAllocator };
		Vector<SceneGeometryTable>		SceneGeometry	= { SystemAllocator };
		Vector<Entity>					SceneEntities	= { SystemAllocator };
		Vector<Entity>					SceneStatics	= { SystemAllocator };
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
			ResourceSize	= sizeof(SceneResourceBlob);
			Type			= EResourceType::EResource_Scene;
			State			= Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
		}

		size_t						ResourceSize;
		EResourceType				Type;
		GUID_t						GUID;
		Resource::ResourceState		State;		// Runtime Member
		uint32_t					RefCount;	// Runtime Member

		char	ID[ID_LENGTH];

		size_t	blockCount;
	};


	/************************************************************************************************/


	struct ColliderResourceBlob
	{
		size_t					ResourceSize;
		EResourceType			Type;
		GUID_t					GUID;
		Resource::ResourceState	State;		// Runtime Member
		uint32_t				RefCount;	// Runtime Member

		char ID[ID_LENGTH];
	};


	/************************************************************************************************/


	struct TextureResourceBlob : public Resource
	{
		TextureResourceBlob()
		{
			ResourceSize    = sizeof(TextureResourceBlob);
			Type            = EResourceType::EResource_Texture;
			State           = Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
		}

		DeviceFormat    format;
		uint2           WH;
		uint32_t        mipLevels;
		uint32_t        mipOffsets[15];

		const char* GetBuffer() const
		{
			return ((const char*)this) + sizeof(TextureResourceBlob);
		}

		size_t GetBufferSize() const
		{
			return ResourceSize - sizeof(TextureResourceBlob);
		}

	};


	/************************************************************************************************/


	struct PrefabResource : public Resource
	{
		PrefabResource(size_t blobSize, uint32_t IN_componentCount) :
			componentCount  { IN_componentCount }
		{
			ResourceSize    = sizeof(PrefabResource) + blobSize;
			Type            = EResourceType::EResource_Prefab;
			State           = Resource::ResourceState::EResourceState_UNLOADED;		// Runtime Member
		}

		uint32_t componentCount;
	};


	/************************************************************************************************/


	FLEXKITAPI bool						Asset2TriMesh		( RenderSystem* RS, CopyContextHandle handle, AssetHandle RHandle, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true );
	FLEXKITAPI bool						Buffer2TriMesh		( RenderSystem* RS, CopyContextHandle handle, const char* buffer, size_t bufferSize, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true );
	FLEXKITAPI Vector<TextureBuffer>	LoadCubeMapAsset	( GUID_t resourceID, size_t& OUT_MIPCount, uint2& OUT_WH, DeviceFormat& OUT_format, iAllocator* );

	FLEXKITAPI TextureSet*		LoadTextureSet	 (GUID_t ID, iAllocator* Memory );
	FLEXKITAPI void				LoadTriangleMesh (GUID_t ID, iAllocator* Memory, TriMesh* out );

	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable(CopyContextHandle handle, size_t guid );
	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable(CopyContextHandle handle, const char* ID );
	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable(CopyContextHandle handle, const char* buffer, const size_t bufferSize );

	typedef Pair<size_t, SpriteFontAsset*> LoadFontResult;

	FLEXKITAPI LoadFontResult	LoadFontAsset	(const char* file, const char* dir, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem );
	FLEXKITAPI void				Release			( SpriteFontAsset* asset, RenderSystem* RS);


	/************************************************************************************************/


	enum ReadAsset_RC
	{
		RAC_OK,
		RAC_ERROR,
		RAC_ASSET_NOT_FOUND,
	};

	struct ReadContextInterface
	{
		ReadContextInterface() = default;

		virtual ~ReadContextInterface() {}

		ReadContextInterface				(const ReadContextInterface& rhs) = delete;
		ReadContextInterface& operator =	(const ReadContextInterface& rhs) = delete;

		virtual void Close()													= 0;
		virtual void Read(void* dst_ptr, size_t readSize, size_t readOffset)	= 0;
		virtual void SetOffset(size_t readOffset)								= 0;
		virtual bool IsValid() const noexcept									= 0;
	};

	struct ReadContext
	{
		ReadContext(GUID_t IN_guid = INVALIDHANDLE, ReadContextInterface* IN_ctx = nullptr, iAllocator* IN_allocator = nullptr);
		~ReadContext();

		ReadContext				(const ReadContext& rhs) = delete;
		ReadContext& operator =	(const ReadContext& rhs) = delete;

		ReadContext& operator = (ReadContext&& rhs) noexcept;

		void Close();
		void Read(void* dst_ptr, size_t readSize, size_t readOffset);

		void SetOffset(size_t offset);

		void Release();

		operator bool() const noexcept;


		GUID_t					guid; // Currently read asset
		ReadContextInterface*	pimpl;
		iAllocator*				allocator;
	};


	ReadContext		OpenReadContext(GUID_t guid);
	ReadAsset_RC	ReadAsset(ReadContext& readContext, GUID_t Asset, void* _ptr, size_t readSize, size_t readOffset = 0);

	const char*		GetResourceStringID(GUID_t guid);


}	/************************************************************************************************/

#endif // include guard


/**********************************************************************

Copyright (c) 2014-2023 Robert May

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
