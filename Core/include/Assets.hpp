#ifndef RESOURCES_H
#define RESOURCES_H

#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "MemoryUtilities.hpp"
#include "Fonts.hpp"
#include "ResourceHandles.hpp"
#include "TextureUtilities.hpp"
#include "Intersection.hpp"


#include <variant>
#include <iostream>


/************************************************************************************************/


namespace FlexKit
{
	static const size_t ID_LENGTH = 64;

	struct TriMesh;
	struct TriMesh;
	struct TextureSet;

	enum EResourceType : uint64_t
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
		Resource() = default;
		Resource(const Resource& rhs) = default;

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
	};


	/************************************************************************************************/

	
	struct ResourceEntry
	{
		GUID_t					GUID;
		size_t					ResourcePosition;
		const char*				ResouceLOC; // Not Used in File
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

	void		InitiateAssetTable	(iAllocator* Memory);
	void		ReleaseAssetTable	();

	size_t		ReadAssetTableSize	    (FILE* F);
	size_t		ReadAssetSize		    (FILE* F, ResourceTable* Table, size_t Index);

	void						AddAssetFile	(const char* FILELOC);
	AssetHandle					AddAssetBuffer	(Resource*);            // Will increment resource refcount
	Resource*					GetAsset		(AssetHandle RHandle);
	std::optional<GUID_t>		FindAssetGUID	(const char* Str);

	bool ReadAssetTable	(FILE* F, ResourceTable* Out, size_t TableSize);
	bool ReadResource	(FILE* F, ResourceTable* Table, size_t Index, Resource* out);

	AssetHandle LoadGameAsset (const char* ID);  // Asset refcount starts at 1
	AssetHandle LoadGameAsset (GUID_t GUID);     // Asset refcount starts at 1

	void FreeAsset			    (AssetHandle RHandle);
	void FreeAllAssets		();
	void FreeAllAssetFiles	();

	bool isAssetAvailable		(GUID_t ID);
	bool isAssetAvailable		(const char* ID);

	void SetLoadFailureHandler   (AssetFailureHandler handler);


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
		uint8_t		materialIndex;
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
				std::byte	_PAD[12];
			}Info;

			float  BS[4];// Uses Float Array instead of float4, float4 requires alignment 
			float  AABB[6];

		}	header;
	};


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


	Vector<TextureBuffer>		LoadCubeMapAsset	(GUID_t resourceID, size_t& OUT_MIPCount, uint2& OUT_WH, DeviceFormat& OUT_format, iAllocator*);

	typedef Pair<size_t, SpriteFontAsset*> LoadFontResult;

	LoadFontResult	LoadFontAsset	(const char* file, const char* dir, IRenderSystem* RS, iAllocator* tempMem, iAllocator* outMem );
	void			Release			(SpriteFontAsset* asset, IRenderSystem& RS);


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


		struct CustomDeleter
		{
			iAllocator* allocator;

			void operator ()(void* _ptr)
			{
				allocator->free(_ptr);
			}
		};

		using pimpl_ptr = std::unique_ptr<ReadContextInterface, CustomDeleter>;

		GUID_t			guid; // Currently read asset
		pimpl_ptr		pimpl;
		iAllocator*		allocator;
	};


	ReadContext		OpenReadContext(GUID_t guid);
	ReadContext		OpenReadContext(GUID_t guid, iAllocator&);
	ReadAsset_RC	ReadAsset(ReadContext& readContext, GUID_t Asset, void* _ptr, size_t readSize, size_t readOffset = 0);
	ReadAsset_RC	ReadAsset(ReadContext& readContext, GUID_t Asset, void* _ptr, size_t readSize, iAllocator& , size_t readOffset = 0);

	const char*		GetResourceStringID(GUID_t guid);

#if 0
    struct FileContext : public ReadContextInterface
	{
		FileContext() = default;

		FileContext(const char* IN_fileDir, size_t IN_offset)
		{
			WCHAR wFileDir[256];
			memset(wFileDir, 0, sizeof(wFileDir));
			size_t converted = 0;

			mbstowcs_s(&converted, wFileDir, IN_fileDir, strnlen_s(IN_fileDir, sizeof(wFileDir)));

			file = CreateFile2(
				wFileDir,
				GENERIC_READ,
				FILE_SHARE_READ,
				OPEN_EXISTING,
				nullptr);


			if (file == INVALID_HANDLE_VALUE)
			{
				auto err = GetLastError();
				//__debugbreak();
			}

			fileDir = IN_fileDir;
			offset  = IN_offset;
		}

		~FileContext() { Close(); }

		HANDLE          file    = INVALID_HANDLE_VALUE;
		const char*     fileDir = nullptr;
		size_t          offset  = 0;

		// Non-copyable
		FileContext(const FileContext& rhs)                 = delete;
		FileContext& operator = (const FileContext& rhs)    = delete;

		FileContext& operator = (FileContext&& rhs) noexcept
		{
			Close();

			file        = rhs.file;
			fileDir     = rhs.fileDir;
			offset      = rhs.offset;

			rhs.file        = INVALID_HANDLE_VALUE;
			rhs.fileDir     = nullptr;
			rhs.offset      = 0;

			return *this;
		}

		void Close() final
		{
			if(file != INVALID_HANDLE_VALUE)
				CloseHandle(file);
		}

		void Read(void* dst_ptr, size_t readSize, size_t readOffset) final
		{
			if (file != INVALID_HANDLE_VALUE)
			{
				DWORD bytesRead = 0;

				OVERLAPPED overlapped   = { 0 };
				overlapped.Offset       = static_cast<DWORD>(readOffset + offset);

				if (bool res = ReadFile(file, dst_ptr, static_cast<DWORD>(readSize), &bytesRead, &overlapped); res != true)
					throw std::runtime_error("Failed to read");
			}
		}

		void SetOffset(size_t readOffset) final
		{
			offset = readOffset;
		}

		bool IsValid() const noexcept
		{
			return file != INVALID_HANDLE_VALUE;
		}
	};
#else
	struct NullFileContext : public ReadContextInterface, public NoCopy, public NoMove
	{
		NullFileContext() = default;

		NullFileContext(const char* IN_fileDir, size_t IN_offset)
		{}


		void Close() final
		{
			FK_ASSERT(false);
		}

		void Read(void* dst_ptr, size_t readSize, size_t readOffset) final
		{
			FK_ASSERT(false);
		}

		void SetOffset(size_t readOffset) final
		{
			FK_ASSERT(false);
		}

		bool IsValid() const noexcept
		{
			return false;
		}
	};

	using FileContext = NullFileContext;
#endif


	struct BufferContext : public ReadContextInterface
	{
		BufferContext(std::byte* IN_buffer, size_t IN_bufferSize, size_t IN_offset) :
			buffer      { IN_buffer },
			bufferSize  { IN_bufferSize },
			offset      { IN_offset } {}

		void Close() final {}

		void Read(void* dst_ptr, size_t readSize, size_t readOffset) final
		{
			if(readOffset + offset + readSize <= bufferSize)
				memcpy(dst_ptr, buffer + readOffset + offset, readSize);
		}

		void SetOffset(size_t readOffset) final
		{
			offset = readOffset;
		}

		bool IsValid() const noexcept final
		{
			return (buffer != nullptr && bufferSize > 0);
		}

		std::byte*  buffer      = nullptr;
		size_t		bufferSize  = 0;
		size_t		offset      = 0;
	};


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
