/**********************************************************************

Copyright (c) 2014-2019 Robert May

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
        EResource_CubeMapTexture,
        EResource_Animation,
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
	}inline Resources;


	/************************************************************************************************/


	FLEXKITAPI void			InitiateAssetTable	(iAllocator* Memory);
	FLEXKITAPI void			ReleaseAssetTable	();

	FLEXKITAPI size_t		ReadAssetTableSize	    (FILE* F);
	FLEXKITAPI size_t		ReadAssetSize		    (FILE* F, ResourceTable* Table, size_t Index);

	FLEXKITAPI void					AddAssetFile	(const char* FILELOC);
	FLEXKITAPI Resource*			GetAsset		(AssetHandle RHandle);
	FLEXKITAPI Pair<GUID_t, bool>	FindAssetGUID	(const char* Str);


	FLEXKITAPI bool			ReadAssetTable	(FILE* F, ResourceTable* Out, size_t TableSize);
	FLEXKITAPI bool			ReadResource		(FILE* F, ResourceTable* Table, size_t Index, Resource* out);

	FLEXKITAPI AssetHandle LoadGameAsset (const char* ID);
	FLEXKITAPI AssetHandle LoadGameAsset (GUID_t GUID);

	FLEXKITAPI void FreeAsset			    (AssetHandle RHandle);
	FLEXKITAPI void FreeAllAssets		();
	FLEXKITAPI void FreeAllAssetFiles	();

	FLEXKITAPI bool isAssetAvailable		(GUID_t ID);
	FLEXKITAPI bool isAssetAvailable		(const char* ID);


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
            size_t  bufferOffset    = 0;
            size_t  subMeshCount    = 0;

            static_vector<Buffer> buffers;
        } descriptor;

        SubMesh subMeshes[];
    };

    struct LODEntry
    {
        size_t size;
        size_t offset;
    };

    struct LODTable
    {
        size_t      LODcount;
        LODEntry    lodOffsets[];
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
            bool	HasAnimation;
            bool	HasIndexBuffer;

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

            size_t LODCount;
        }   header;

		char Memory[];
	};


    bool LoadLOD                (TriMesh* triMesh, uint level, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);
    bool LoadAllLODFromMemory   (TriMesh* triMesh, const char* buffer, const size_t bufferSize, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);


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
		char	Buffer[];
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
		byte Buffer[];
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


	FLEXKITAPI bool				        Asset2TriMesh		( RenderSystem* RS, CopyContextHandle handle, AssetHandle RHandle, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true );
	FLEXKITAPI bool				        Buffer2TriMesh		( RenderSystem* RS, CopyContextHandle handle, const char* buffer, size_t bufferSize, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true );
    FLEXKITAPI Vector<TextureBuffer>    LoadCubeMapAsset    ( GUID_t resourceID, size_t& OUT_MIPCount, uint2& OUT_WH, DeviceFormat& OUT_format, iAllocator* );

	FLEXKITAPI TextureSet*		LoadTextureSet	 ( GUID_t ID, iAllocator* Memory );
	FLEXKITAPI void				LoadTriangleMesh ( GUID_t ID, iAllocator* Memory, TriMesh* out );

	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable ( RenderSystem* RS, CopyContextHandle handle, size_t guid );
	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable ( RenderSystem* RS, CopyContextHandle handle, const char* ID );
	FLEXKITAPI TriMeshHandle	LoadTriMeshIntoTable ( RenderSystem* RS, CopyContextHandle handle, const char* buffer, const size_t bufferSize );

	typedef Pair<size_t, SpriteFontAsset*> LoadFontResult;

	FLEXKITAPI LoadFontResult	LoadFontAsset	(const char* file, const char* dir, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem );
	FLEXKITAPI void				Release			( SpriteFontAsset* asset, RenderSystem* RS);


	/************************************************************************************************/


	inline size_t ReadAssetTableSize(FILE* F)
	{
		byte Buffer[128];

		const int       seek_res = fseek(F, 0, SEEK_SET);
		const size_t    read_res = fread(Buffer, 1, 128, F);

		const ResourceTable* table  = (ResourceTable*)Buffer;
		return table->ResourceCount * sizeof(ResourceEntry) + sizeof(ResourceTable);
	}


	/************************************************************************************************/


	inline bool ReadAssetTable(FILE* F, ResourceTable* Out, size_t TableSize)
	{
        const int seek_res    = fseek(F, 0, SEEK_SET);
        const size_t read_res = fread(Out, 1, TableSize, F);

		return (read_res == TableSize);
	}


	/************************************************************************************************/


	inline size_t ReadAssetSize(FILE* F, ResourceTable* Table, size_t Index)
	{
		byte Buffer[64];

		const int seek_res      = fseek(F, (long)Table->Entries[Index].ResourcePosition, SEEK_SET);
		const size_t read_res   = fread(Buffer, 1, 64, F);

		Resource* resource = (Resource*)Buffer;
		return resource->ResourceSize;
	}


	/************************************************************************************************/


	inline bool ReadResource(FILE* F, ResourceTable* Table, size_t Index, Resource* out)
	{
        FK_LOG_INFO( "Loading Resource: %s : ResourceID: %u", Table->Entries[Index].ID, Table->Entries[Index].GUID);
#if _DEBUG
		std::chrono::system_clock Clock;
		auto Before = Clock.now();
		FINALLY
			auto After = Clock.now();
			auto Duration = std::chrono::duration_cast<std::chrono::microseconds>( After - Before );
            FK_LOG_INFO("Loading Resource: %s took %u microseconds", Table->Entries[Index].ID, Duration.count());
		FINALLYOVER
#endif

        if(!F)
            return false;

        fseek(F, 0, SEEK_END);
        const size_t resourceFileSize = ftell(F) + 1;
        rewind(F);

        const size_t position   = Table->Entries[Index].ResourcePosition;
		int seek_res            = fseek(F, (long)position, SEEK_SET);

        size_t resourceSize = 0;
		size_t read_res     = fread(&resourceSize, 1, 8, F);

        if (!(resourceSize + position < resourceFileSize))
            return false;

		seek_res                = fseek(F, (long)position, SEEK_SET);
		const size_t readSize   = fread(out, 1, resourceSize, F);

		return (readSize == out->ResourceSize);
	}


    /************************************************************************************************/


    enum ReadAsset_RC
    {
        RAC_OK,
        RAC_ERROR,
        RAC_ASSET_NOT_FOUND,
    };

#if 0 // Use std file io
    struct ReadContext
    {
        ReadContext() = default;

        ReadContext(const char* IN_fileDir, size_t IN_offset)
        {
            FILE* F = 0;
            if (auto res = fopen_s(&F, IN_fileDir, "rb"); res != 0)
                return;

            file    = F;
            fileDir = IN_fileDir;
            offset  = IN_offset;
        }

        ~ReadContext() { Close(); }

        FILE*           file    = nullptr;
        const char*     fileDir = nullptr;
        size_t          offset  = 0;

        // Non-copyable
        ReadContext(const ReadContext& rhs) = delete;
        ReadContext& operator = (const ReadContext& rhs) = delete;

        ReadContext& operator = (ReadContext&& rhs) noexcept
        {
            Close();

            file    = rhs.file;
            fileDir = rhs.fileDir;
            offset  = rhs.offset;

            rhs.file    = nullptr;
            rhs.fileDir = nullptr;
            rhs.offset  = 0;

            return *this;
        }

        void Close()
        {
            if(file)
                ::fclose(file);

            file    = nullptr;
            fileDir = nullptr;
        }

        void Read(void* dst_ptr, size_t readSize, size_t readOffset)
        {
            if (auto res = fseek(file, offset + readOffset, SEEK_SET); res != 0)
                __debugbreak();

            if (auto res = fread_s(dst_ptr, readSize, 1, readSize, file); res != readSize)
                __debugbreak();
        }

        operator bool() { return file != nullptr; }
    };

#else // Win32 IO

    struct ReadContext
    {
        ReadContext() = default;

        ReadContext(const char* IN_fileDir, size_t IN_offset)
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

        ~ReadContext() { Close(); }

        HANDLE          file    = INVALID_HANDLE_VALUE;
        const char*     fileDir = nullptr;
        size_t          offset  = 0;

        // Non-copyable
        ReadContext(const ReadContext& rhs) = delete;
        ReadContext& operator = (const ReadContext& rhs) = delete;

        ReadContext& operator = (ReadContext&& rhs) noexcept
        {
            Close();

            file = rhs.file;
            fileDir = rhs.fileDir;
            offset = rhs.offset;

            rhs.file = INVALID_HANDLE_VALUE;
            rhs.fileDir = nullptr;
            rhs.offset = 0;

            return *this;
        }

        void Close()
        {
            if(file != INVALID_HANDLE_VALUE)
                CloseHandle(file);
        }

        void Read(void* dst_ptr, size_t readSize, size_t readOffset)
        {
            if (file != INVALID_HANDLE_VALUE)
            {
                DWORD bytesRead = 0;

                OVERLAPPED overlapped   = { 0 };
                overlapped.Offset       = static_cast<DWORD>(readOffset + offset);

                if (bool res = ReadFile(file, dst_ptr, static_cast<DWORD>(readSize), &bytesRead, &overlapped); res != true)
                {
                    auto error = GetLastError();
                    //__debugbreak();
                }
            }

        }


        operator bool() { return file != INVALID_HANDLE_VALUE; }
    };

#endif

    ReadContext         OpenReadContext(GUID_t guid);
    ReadAsset_RC        ReadAsset(ReadContext& readContext, GUID_t Asset, void* _ptr, size_t readSize, size_t readOffset = 0);


    const char*         GetResourceStringID(GUID_t guid);


}	/************************************************************************************************/

#endif // include guard
