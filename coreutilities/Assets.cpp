/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

#include "Assets.h"
#include "graphics.h"

namespace FlexKit
{   /************************************************************************************************/


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


	void InitiateAssetTable(iAllocator* Memory)
	{
		Resources.Tables			= Vector<ResourceTable*>(Memory);
		Resources.ResourceFiles		= Vector<ResourceDirectory>(Memory);
		Resources.ResourcesLoaded	= Vector<Resource*>(Memory);
		Resources.ResourceGUIDs		= Vector<GUID_t>(Memory);
		Resources.ResourceMemory	= Memory;
	}

	
	/************************************************************************************************/


	void ReleaseAssetTable()
	{
		for (auto* Table : Resources.Tables)
			Resources.ResourceMemory->free(Table);

		for (auto* Resource : Resources.ResourcesLoaded)
			Resources.ResourceMemory->free(Resource);

		Resources.Tables.Release();
		Resources.ResourceFiles.Release();
		Resources.ResourcesLoaded.Release();
		Resources.ResourceGUIDs.Release();
	}


    /************************************************************************************************/


	void AddAssetFile(const char* FILELOC)
	{
		ResourceDirectory Dir;
		strcpy_s(Dir.str, FILELOC);

		FILE* F = 0;
		int S   = fopen_s(&F, FILELOC, "rb");

        if (S)
            FK_LOG_ERROR("Failed To Read Asset Table");

		size_t TableSize	 = ReadAssetTableSize(F);
		ResourceTable* Table = (ResourceTable*)Resources.ResourceMemory->_aligned_malloc(TableSize);

		if (ReadAssetTable(F, Table, TableSize))
		{
			Resources.ResourceFiles.push_back(Dir);
			Resources.Tables.push_back(Table);
		}
		else
			Resources.ResourceMemory->_aligned_free(Table);
	}

    /************************************************************************************************/


    AssetHandle AddAssetBuffer(Resource* buffer)
    {
        buffer->RefCount = 1;

        return Resources.ResourcesLoaded.push_back((Resource*)buffer);
    }


    /************************************************************************************************/


	Pair<GUID_t, bool>	FindAssetGUID(const char* Str)
	{
		bool Found = false;
		GUID_t Guid = INVALIDHANDLE;

		for (auto T : Resources.Tables)
		{
			auto end = T->ResourceCount;
			for (size_t I = 0; I < end; ++I)
			{
				if (!strncmp(T->Entries[I].ID, Str, 64))
				{
					Found = true;
					Guid = T->Entries[I].GUID;
					return{ Guid, Found };
				}
			}
		}

		return{ Guid, Found };
	}


	/************************************************************************************************/


	Resource* GetAsset(AssetHandle RHandle)
	{
		if (RHandle == INVALIDHANDLE)
			return nullptr;

		Resources.ResourcesLoaded[RHandle]->RefCount++;
		return Resources.ResourcesLoaded[RHandle];
	}


	/************************************************************************************************/


	void FreeAllAssets()
	{
		for (auto R : Resources.ResourcesLoaded)
			if(Resources.ResourceMemory) Resources.ResourceMemory->_aligned_free(R);
	}


	/************************************************************************************************/


	void FreeAllAssetFiles()
	{
		for (auto T : Resources.Tables)
			Resources.ResourceMemory->_aligned_free(T);
	}


	/************************************************************************************************/


	void FreeAsset(AssetHandle RHandle)
	{
		Resources.ResourcesLoaded[RHandle]->RefCount--;
		if (Resources.ResourcesLoaded[RHandle]->RefCount == 0)
		{
			// Evict
			// TODO: Resource Eviction
		}
	}


	/************************************************************************************************/


    ReadContext OpenReadContext(GUID_t guid)
    {
        AssetHandle RHandle = INVALIDHANDLE;

        for (auto resource : Resources.ResourcesLoaded)
        {
            if (resource->GUID == guid)
            {
                auto& bufferCtx = Resources.ResourceMemory->allocate<BufferContext>((byte*)resource, resource->ResourceSize, 0);
                return ReadContext{ guid, &bufferCtx, Resources.ResourceMemory };
            }
        }

		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
                if (t->Entries[I].GUID == guid)
                {
                    auto& fileCtx = Resources.ResourceMemory->allocate<FileContext>(Resources.ResourceFiles[TI].str, t->Entries[I].ResourcePosition);
                    return ReadContext{ guid, &fileCtx, Resources.ResourceMemory };
                }
			}
		}

        return {};
    }


    /************************************************************************************************/


    ReadAsset_RC ReadAsset(ReadContext& readContext, GUID_t guid, void* _ptr, size_t readSize, size_t readOffset)
    {
        for (auto resource : Resources.ResourcesLoaded)
        {
            if (resource->GUID == guid)
            {
                auto& bufferCtx = Resources.ResourceMemory->allocate<BufferContext>((byte*)resource, resource->ResourceSize, 0);
                readContext = ReadContext{ guid, &bufferCtx, Resources.ResourceMemory };

                readContext.Read(_ptr, readSize, readOffset);

                return RAC_OK;
            }
        }


        AssetHandle RHandle = INVALIDHANDLE;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == guid)
				{
                    size_t resourceOffset = t->Entries[I].ResourcePosition;

                    //if (readContext.fileDir != Resources.ResourceFiles[TI].str)
                    if (readContext.guid != guid)
                        readContext = OpenReadContext(guid);// ReadContext{ Resources.ResourceFiles[TI].str, resourceOffset };

                    readContext.SetOffset(resourceOffset);
                    readContext.Read(_ptr, readSize, readOffset);

                    return RAC_OK;
				}
			}
		}

        return RAC_ERROR;
    }


    /************************************************************************************************/


    const char* GetResourceStringID(GUID_t guid)
    {
        AssetHandle RHandle = INVALIDHANDLE;
        for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
        {
            auto& t = Resources.Tables[TI];
            for (size_t I = 0; I < t->ResourceCount; ++I)
            {
                if (t->Entries[I].GUID == guid)
                    return t->Entries[I].ID;
            }
        }

        return nullptr;
    }



    /************************************************************************************************/


	AssetHandle LoadGameAsset(GUID_t guid)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (Resources.ResourceGUIDs[I] == guid)
				return I;

        AssetHandle RHandle = INVALIDHANDLE;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == guid)
				{

					FILE* F             = 0;
					int S               = fopen_s(&F, Resources.ResourceFiles[TI].str, "rb");
					size_t ResourceSize = ReadAssetSize(F, t, I);

					Resource* NewResource = (Resource*)Resources.ResourceMemory->_aligned_malloc(ResourceSize);
					if (!NewResource)
					{
						// Memory Full
						// Evict A Unused Resource
						// TODO: Handle running out of memory
						FK_ASSERT(false, "OUT OF MEMORY!");
					}

					if (!ReadResource(F, t, I, NewResource))
					{
						Resources.ResourceMemory->_aligned_free(NewResource);

						FK_ASSERT(false, "FAILED TO LOAD RESOURCE!");
					}
					else
					{
						NewResource->State		= Resource::EResourceState_LOADED;
						NewResource->RefCount	= 0;
						RHandle					= Resources.ResourcesLoaded.size();
						Resources.ResourcesLoaded.push_back(NewResource);
						Resources.ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


	/************************************************************************************************/


    AssetHandle LoadGameAsset(const char* ID)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (!strcmp(Resources.ResourcesLoaded[I]->ID, ID))
				return I;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
				{
					FILE* F             = 0;
					int S               = fopen_s(&F, Resources.ResourceFiles[TI].str, "rb");
					size_t ResourceSize = FlexKit::ReadAssetSize(F, t, I);

					Resource* NewResource = (Resource*)Resources.ResourceMemory->_aligned_malloc(ResourceSize);
					if (!NewResource)
					{
						// Memory Full
						// Evict A Unused Resource
						// TODO: THIS
						FK_ASSERT(false, "OUT OF MEMORY!");
					}

					if (!FlexKit::ReadResource(F, t, I, NewResource))
					{
						FK_ASSERT(false, "FAILED TO LOAD RESOURCE!");
					}
					else
					{
						NewResource->State		= Resource::EResourceState_LOADED;
						NewResource->RefCount	= 0;
						RHandle					= Resources.ResourcesLoaded.size();
						Resources.ResourcesLoaded.push_back(NewResource);
						Resources.ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


    /************************************************************************************************/


    size_t ReadAssetTableSize(FILE* F)
	{
		byte Buffer[128];

		const int       seek_res = fseek(F, 0, SEEK_SET);
		const size_t    read_res = fread(Buffer, 1, 128, F);

		const ResourceTable* table  = (ResourceTable*)Buffer;
		return table->ResourceCount * sizeof(ResourceEntry) + sizeof(ResourceTable);
	}


	/************************************************************************************************/


	bool ReadAssetTable(FILE* F, ResourceTable* Out, size_t TableSize)
	{
        const int seek_res    = fseek(F, 0, SEEK_SET);
        const size_t read_res = fread(Out, 1, TableSize, F);

		return (read_res == TableSize);
	}


	/************************************************************************************************/


	size_t ReadAssetSize(FILE* F, ResourceTable* Table, size_t Index)
	{
		byte Buffer[64];

		const int seek_res      = fseek(F, (long)Table->Entries[Index].ResourcePosition, SEEK_SET);
		const size_t read_res   = fread(Buffer, 1, 64, F);

		Resource* resource = (Resource*)Buffer;
		return resource->ResourceSize;
	}


    /************************************************************************************************/


	bool ReadResource(FILE* F, ResourceTable* Table, size_t Index, Resource* out)
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


	bool isAssetAvailable(GUID_t ID)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (Resources.ResourcesLoaded[I]->GUID == ID)
				return true;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == ID)
					return true;
			}
		}

		return false;
	}


	bool isAssetAvailable(const char* ID)
	{
			for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (!strcmp(Resources.ResourcesLoaded[I]->ID, ID))
				return true;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
					return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


    bool LoadLOD(TriMesh* triMesh, uint level, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory)
    {
        auto readCtx = OpenReadContext(triMesh->assetHandle);

        if (!readCtx)
            return false;

        auto& lod       = triMesh->lods[level];

        TriMesh::LOD_Runtime::LOD_State lodState = lod.state.load();

        if (lodState != TriMesh::LOD_Runtime::LOD_State::Unloaded)
            return false;

        if (!lod.state.compare_exchange_strong(lodState, TriMesh::LOD_Runtime::LOD_State::Loading))
            return false;

        auto buffer     = (char*)memory.malloc(lod.lodSize);
        auto lodHeader  = reinterpret_cast<FlexKit::LODlevel*>(buffer);

        readCtx.Read(lodHeader, lod.lodSize, lod.lodFileOffset);

		for (auto& vertexBuffer : lodHeader->descriptor.buffers)
		{
			if (vertexBuffer.size)
			{
                float* temp = (float*)(buffer + vertexBuffer.Begin);
				auto View = new (memory._aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)((char*)buffer + vertexBuffer.Begin), vertexBuffer.size);
				View->SetTypeFormatSize((VERTEXBUFFER_TYPE)vertexBuffer.Type, (VERTEXBUFFER_FORMAT)vertexBuffer.Format, vertexBuffer.size / vertexBuffer.Format );
				lod.buffers.push_back(View);
			}
		}

        CreateVertexBuffer(renderSystem, copyCtx, lod.buffers, lod.buffers.size(), lod.vertexBuffer);

        const size_t subMeshCount = lodHeader->descriptor.subMeshCount;
        for (size_t I = 0; I < subMeshCount; I++)
        {
            SubMesh subMesh;
            memcpy(&subMesh, buffer + lod.lodFileOffset + sizeof(LODlevel) + sizeof(SubMesh) * I, sizeof(SubMesh));
            
            lod.subMeshes.push_back(subMesh);
        }

        lod.state = TriMesh::LOD_Runtime::LOD_State::Loaded;

        memory.free(buffer);

        return true;
    }


    /************************************************************************************************/


    bool LoadAllLODFromMemory(TriMesh* triMesh, const char* buffer, const size_t bufferSize, RenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory)
    {
        const size_t lodCount = triMesh->lods.size();
        for (auto& lod : triMesh->lods)
        {
            TriMesh::LOD_Runtime::LOD_State lodState = lod.state.load();

            if (lodState != TriMesh::LOD_Runtime::LOD_State::Unloaded)
                return false;

            if (!lod.state.compare_exchange_strong(lodState, TriMesh::LOD_Runtime::LOD_State::Loading))
                return false;

            auto lodBuffer  = (char*)memory.malloc(lod.lodSize);

            FlexKit::LODlevel lodHeader;
            memcpy(&lodHeader, buffer + lod.lodFileOffset, sizeof(LODlevel));

            auto buffer2 = (char*)memory.malloc(lod.lodSize);
            memcpy(buffer2, buffer + lod.lodFileOffset, lod.lodSize);


		    for (auto& vertexBuffer : lodHeader.descriptor.buffers)
		    {
			    if (vertexBuffer.size)
			    {
                    float* temp = (float*)(buffer2 + vertexBuffer.Begin);
				    auto View   = new (memory._aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)((char*)buffer2 + vertexBuffer.Begin), vertexBuffer.size);
				    View->SetTypeFormatSize((VERTEXBUFFER_TYPE)vertexBuffer.Type, (VERTEXBUFFER_FORMAT)vertexBuffer.Format, vertexBuffer.size / vertexBuffer.Format );
				    lod.buffers.push_back(View);
			    }
		    }

            CreateVertexBuffer(renderSystem, copyCtx, lod.buffers, lod.buffers.size(), lod.vertexBuffer);

            const size_t subMeshCount = lodHeader.descriptor.subMeshCount;
            for (size_t I = 0; I < subMeshCount; I++)
            {
                SubMesh subMesh;
                memcpy(&subMesh, buffer + lod.lodFileOffset + sizeof(LODlevel) + sizeof(SubMesh) * I, sizeof(SubMesh));

                lod.subMeshes.push_back(subMesh);
            }

            lod.state = TriMesh::LOD_Runtime::LOD_State::Loaded;
        }

        return true;
    }


    /************************************************************************************************/


    bool Buffer2TriMesh(RenderSystem* RS, CopyContextHandle copyCtx, const char* buffer, size_t bufferSize, iAllocator* Memory, TriMesh* triMesh, bool ClearBuffers)
    {
		TriMeshAssetBlob* Blob  = (TriMeshAssetBlob*)buffer;
		size_t BufferCount      = 0;

		triMesh->SkinTable	    = nullptr;
		triMesh->SkeletonGUID   = Blob->header.SkeletonGuid;
		triMesh->Skeleton	    = nullptr;
		triMesh->Info.Min.x     = Blob->header.Info.maxx;
		triMesh->Info.Min.y     = Blob->header.Info.maxy;
		triMesh->Info.Min.z     = Blob->header.Info.maxz;
		triMesh->Info.Max.x     = Blob->header.Info.minx;
		triMesh->Info.Max.y     = Blob->header.Info.miny;
		triMesh->Info.Max.z     = Blob->header.Info.minz;
		triMesh->Info.r		    = Blob->header.Info.r;
		triMesh->Memory		    = Memory;
        triMesh->assetHandle    = Blob->header.GUID;
        triMesh->TriMeshID      = Blob->header.GUID;

        triMesh->BS     = { { Blob->header.BS[0], Blob->header.BS[1], Blob->header.BS[2] }, Blob->header.BS[3] };
        triMesh->AABB   =
        { 
			{ Blob->header.AABB[0], Blob->header.AABB[1], Blob->header.AABB[2] },
			{ Blob->header.AABB[3], Blob->header.AABB[4], Blob->header.AABB[5] }
		};

            
		if (strlen(Blob->header.ID))
		{
            triMesh->ID = (char*)Memory->malloc(64);
			strcpy_s((char*)triMesh->ID, 64, Blob->header.ID);
		} else
            triMesh->ID = nullptr;


        auto lodCount = Blob->header.LODCount;

        const size_t tableSize = lodCount * sizeof(LODEntry);
        LODEntry* lodTable = reinterpret_cast<LODEntry*>(Memory->malloc(tableSize));
        memcpy(lodTable, Blob->Memory, tableSize);

        for (size_t I = 0; I < lodCount; I++)
        {
            TriMesh::LOD_Runtime lod;
            lod.lodFileOffset   = lodTable[I].offset;
            lod.lodSize         = lodTable[I].size;
            lod.state           = TriMesh::LOD_Runtime::LOD_State::Unloaded;

            triMesh->lods.push_back(lod);
        }

        // Load lowest detail lod level
        if (!LoadLOD(triMesh, uint(lodCount - 1), *RS, copyCtx, *Memory))
            LoadAllLODFromMemory(triMesh, buffer, bufferSize, *RS, copyCtx, *Memory);// not able to stream lods, load all now

        Memory->free(lodTable);

        return true;
    }


    /************************************************************************************************/


	bool Asset2TriMesh(RenderSystem* RS, CopyContextHandle copyCtx, AssetHandle RHandle, iAllocator* Memory, TriMesh* triMesh, bool ClearBuffers)
	{
		Resource* R = GetAsset(RHandle);
		if (R->State == Resource::EResourceState_LOADED && R->Type == EResource_TriMesh)
		{
			TriMeshAssetBlob* Blob  = (TriMeshAssetBlob*)R;
            return Buffer2TriMesh(RS, copyCtx, (const char*)R, R->ResourceSize, Memory, triMesh, ClearBuffers);
		}
        else
		    return false;
	}


	/************************************************************************************************/


    Vector<TextureBuffer> LoadCubeMapAsset(GUID_t resourceID, size_t& OUT_MIPCount, uint2& OUT_WH, DeviceFormat& OUT_format, iAllocator* allocator)
    {
        Vector<TextureBuffer> textureArray{ allocator };

        auto assetHandle            = LoadGameAsset(resourceID);
        CubeMapAssetBlob* resource  = reinterpret_cast<CubeMapAssetBlob*>(FlexKit::GetAsset(assetHandle));

        OUT_MIPCount    = resource->GetFace(0)->MipCount;
        OUT_WH          = { (uint32_t)resource->Width, (uint32_t)resource->Height };
        OUT_format      = (DeviceFormat)resource->Format;

        for (size_t I = 0; I < 6; I++)
        {
            for (size_t MIPLevel = 0; MIPLevel < resource->MipCount; MIPLevel++)
            {
                float* buffer = (float*)resource->GetFace(I)->GetMip(MIPLevel);
                const size_t bufferSize = resource->GetFace(I)->GetMipSize(MIPLevel);

                textureArray.emplace_back(
                    TextureBuffer{
                        uint2{(uint32_t)resource->Width >> MIPLevel, (uint32_t)resource->Height >> MIPLevel},
                        (byte*)buffer,
                        bufferSize,
                        sizeof(float4),
                        nullptr });
            }
        }

        return textureArray;
    }


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(CopyContextHandle handle, size_t GUID)
	{	// Make this atomic
        auto Available = isAssetAvailable(GUID);
        if (!Available)
            return InvalidHandle_t;

		TriMeshHandle Handle;

		for (size_t idx = 0;
			 idx < GeometryTable.Geometry.size();
			 ++idx)
		{
			if (GeometryTable.Guids[idx] == GUID)
				return GeometryTable.Handle[idx];
		}

		if(!GeometryTable.FreeList.size())
		{


			auto Index	= GeometryTable.Geometry.size();
			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back(TriMesh());
			GeometryTable.GeometryIDs.push_back(nullptr);
			GeometryTable.Guids.push_back(0);
			GeometryTable.ReferenceCounts.push_back	(0);
			GeometryTable.Handle.push_back(Handle);

			auto RHandle = LoadGameAsset(GUID);
			auto GameRes = GetAsset(RHandle);
			if( Asset2TriMesh(GeometryTable.renderSystem, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle = GeometryTable.Handles.GetNewHandle();

			auto Available = isAssetAvailable(GUID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(GUID);
			auto GameRes = GetAsset(RHandle);
			
			if(Asset2TriMesh(GeometryTable.renderSystem, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles			[Handle]	= Index;
				GeometryTable.GeometryIDs		[Index]		= GameRes->ID;
				GeometryTable.Guids				[Index]		= GUID;
				GeometryTable.ReferenceCounts	[Index]		= 1;
				GeometryTable.Handle			[Index]		= Handle;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}

		return Handle;
	}


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(CopyContextHandle handle, const char* ID)
	{	// Make this atomic
		TriMeshHandle Handle;

		if(!GeometryTable.FreeList.size())
		{
			auto Index	= GeometryTable.Geometry.size();
			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back		(TriMesh());
			GeometryTable.GeometryIDs.push_back		(nullptr);
			GeometryTable.Guids.push_back			(0);
			GeometryTable.ReferenceCounts.push_back	(0);

			auto Available = isAssetAvailable(ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(ID);
			auto GameRes = GetAsset(RHandle);
			
			if(Asset2TriMesh(GeometryTable.renderSystem, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle		= GeometryTable.Handles.GetNewHandle();

			auto Available = isAssetAvailable(ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(ID);
			auto GameRes = GetAsset(RHandle);

			if(Asset2TriMesh(GeometryTable.renderSystem, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}

		return Handle;
	}


    /************************************************************************************************/


    TriMeshHandle LoadTriMeshIntoTable(CopyContextHandle handle, const char* buffer, const size_t bufferSize)
    {
        TriMeshHandle Handle;
        TriMeshAssetBlob* Blob = (TriMeshAssetBlob*)buffer;

		if(!GeometryTable.FreeList.size())
		{
			auto Index	= GeometryTable.Geometry.size();
			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back		(TriMesh());
			GeometryTable.GeometryIDs.push_back		(nullptr);
			GeometryTable.Guids.push_back			(0);
			GeometryTable.ReferenceCounts.push_back	(0);


			if(Buffer2TriMesh(GeometryTable.renderSystem, handle, buffer, bufferSize, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= Blob->header.ID;
				GeometryTable.Guids[Index]				= Blob->header.GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
				Handle = InvalidHandle_t;
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle		= GeometryTable.Handles.GetNewHandle();

			if(Buffer2TriMesh(GeometryTable.renderSystem, handle, buffer, bufferSize, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				GeometryTable.Handles[Handle]			= Index;
				GeometryTable.GeometryIDs[Index]		= Blob->header.ID;
				GeometryTable.Guids[Index]				= Blob->header.GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
				Handle = InvalidHandle_t;
		}

		return Handle;
    }


	/************************************************************************************************/


	struct INFOBLOCK
	{
		int16_t		FontSize;
		uint8_t		BitField;
		uint8_t		CharSet;
		uint16_t	stretchH;
		uint8_t		AA;
		uint8_t		PaddingUp;
		uint8_t		PaddingRight;
		uint8_t		PaddingDown;
		uint8_t		PaddingLeft;
		uint8_t		SpacingHoriz;
		uint8_t		SpacingVert;
		uint8_t		Outline;
		char		fontName;
	};


	/************************************************************************************************/


	struct COMMONBLOCK
	{
		uint16_t	lineHeight;
		uint16_t	framework;
		uint16_t	ScaleW;
		uint16_t	ScaleH;
		uint16_t	Pages;
		uint8_t		BitField;
		uint8_t		alphaChannel;
		uint8_t		RedChannel;
		uint8_t		GreenChannel;
		uint8_t		BlueChannel;
	};


	/************************************************************************************************/


	struct CHARARRAYBLOCK
	{
#pragma pack(push, 1)
		struct CHAR
		{
			uint32_t	id;
			uint16_t	x;
			uint16_t	y;
			uint16_t	width;
			uint16_t	height;
			uint16_t	xoffset;
			uint16_t	yoffset;
			uint16_t	xadvance;
			uint8_t		page;
			uint8_t		chnl;
		}First;
#pragma pack(pop)
	};


	/************************************************************************************************/


	struct KERNINGENTRY
	{
		uint32_t FirstChar;
		uint32_t SecondChar;
		uint16_t Amount;
	};

	struct KERNINGARRAYBLOCK
	{
		KERNINGENTRY First;
	};


	/************************************************************************************************/


	LoadFontResult LoadFontAsset(const char* dir, const char* file, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem)
	{
		char TEMP[256];
		strcpy_s(TEMP, dir);
		strcat_s(TEMP, file);

		size_t Size = 1 + GetFileSize(TEMP);
		byte* mem = (byte*)tempMem->malloc(Size);
        if (!LoadFileIntoBuffer(TEMP, mem, Size, false))
            return { 0, nullptr };

		char*	FontPath   = nullptr;
		size_t  PathLength = strlen(dir);

		INFOBLOCK	Info;
		COMMONBLOCK	CB;

		SpriteFontAsset*	Fonts      = nullptr;
		size_t				FontCount  = 0;

		KERNINGENTRY*	KBlocks    = nullptr;
		size_t			KBlockUsed = 0;

		uint4	Padding = 0;// Ordering { Top, Left, Bottom, Right }

		char ID[128];
#pragma pack(push, 1)
		struct BlockDescriptor
		{
			char	 ID;
			uint32_t BlockSize;
		};
#pragma pack(pop)

		if (mem[0] == 'B' &&
			mem[1] == 'M' &&
			mem[2] == 'F' &&
			mem[3] == 0x03)
		{
			size_t Position = 4;
			while (Position < Size)
			{
				BlockDescriptor* Desc = (BlockDescriptor*)(mem + Position);
				switch (Desc->ID)
				{
				case 0x01:
				{
					auto test = sizeof(BlockDescriptor);
					INFOBLOCK* IB = (INFOBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					char* FontName = &IB->fontName;
					strcpy_s(ID, FontName);

					Padding[0] = IB->PaddingUp;
					Padding[1] = IB->PaddingLeft;
					Padding[2] = IB->PaddingDown;
					Padding[3] = IB->PaddingRight;

					Info = *IB;
				}break;
				case 0x02:
				{
					COMMONBLOCK* pCB = (COMMONBLOCK*)(mem + Position + sizeof(BlockDescriptor));

					FontCount = pCB->Pages;
					Fonts = (SpriteFontAsset*)outMem->malloc(sizeof(SpriteFontAsset) * FontCount);

					for (size_t I = 0; I < FontCount; ++I) {
                        memset(Fonts + I, 0, sizeof(SpriteFontAsset));

						Fonts[I].Padding = Padding;
						Fonts[I].Memory  = outMem;

						strcpy_s(Fonts[I].FontName, ID);
					}

					CB = *pCB;
				}break;
				case 0x03:
				{
					int32_t BlockSize = Desc->BlockSize;
					char*	FONTPATH = (char*)(mem + Position + sizeof(BlockDescriptor));
					size_t	FontPathLen = strnlen_s(FONTPATH, BlockSize);

					for (size_t I = 0; I < FontCount; ++I) {
						size_t BufferSize = FontPathLen + PathLength + 1;
						Fonts[I].FontDir  = (char*)outMem->malloc(BufferSize);

						strcpy_s(Fonts[I].FontDir, BufferSize, dir);
						strcat_s(Fonts[I].FontDir, BufferSize, FONTPATH + I * FontPathLen);

						auto Texture					= LoadDDSTextureFromFile(Fonts[I].FontDir, RS, RS->GetImmediateUploadQueue(), outMem);
						Fonts[I].Texture				= Texture;
						Fonts[I].TextSheetDimensions	= { CB.ScaleW, CB.ScaleH };
					}
				}break;
				case 0x04:
				{
					CHARARRAYBLOCK* pCAB = (CHARARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					size_t End = Desc->BlockSize / sizeof(CHARARRAYBLOCK::CHAR);
					auto Begin = &pCAB->First;

					for (size_t I = 0; I < End; ++I)
					{
						auto pCB                                      = Begin + I;
						Fonts[pCB->page].GlyphTable[pCB->id].Channel  = pCB->chnl;
						Fonts[pCB->page].GlyphTable[pCB->id].Offsets  = { float(pCB->xoffset),	float(pCB->yoffset) };
						Fonts[pCB->page].GlyphTable[pCB->id].WH       = { float(pCB->width),	float(pCB->height) };
						Fonts[pCB->page].GlyphTable[pCB->id].XY       = { float(pCB->x),		float(pCB->y) };
						Fonts[pCB->page].GlyphTable[pCB->id].Xadvance = pCB->xadvance;
						Fonts[pCB->page].FontSize[0]				  = Max(Fonts[pCB->page].FontSize[0], pCB->width);
						Fonts[pCB->page].FontSize[1]				  = Max(Fonts[pCB->page].FontSize[1], pCB->height);
					}
				}break;
				case 0x05:
				{
					KERNINGARRAYBLOCK* pKAB = (KERNINGARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
				}break;
				default:
					break;
				}
				Position = Position + sizeof(BlockDescriptor) + Desc->BlockSize;

			}
		}

		return{ FontCount, Fonts };
	}


	/************************************************************************************************/


	void Release(SpriteFontAsset* asset, RenderSystem* RS)
	{
        if (!asset)
            return;

		RS->ReleaseResource(asset->Texture);
		asset->Memory->free(asset->FontDir);
		asset->Memory->free(asset);
	}


	/************************************************************************************************/
}
