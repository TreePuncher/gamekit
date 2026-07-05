#include "TriMeshResource.hpp"


namespace FlexKit
{
    struct IRenderSystem;
    /************************************************************************************************/


    struct _GeometryTable
	{
		HandleUtilities::HandleTable<TriMeshHandle>		Handles;
		Vector<TriMesh>									Geometry;
		Vector<size_t>									ReferenceCounts;
		Vector<GUID_t>									Guids;
		Vector<const char*>								GeometryIDs;
		Vector<TriMeshHandle>							Handle;
		Vector<size_t>									FreeList;
		iAllocator*										allocator;
		IRenderSystem*									renderSystem = nullptr;
	}	GeometryTable;


	/************************************************************************************************/


	void InitiateGeometryTable(IRenderSystem* renderSystem, iAllocator* allocator)
	{
		GeometryTable.Handles.Initiate(allocator);
		GeometryTable.Handle			= Vector<TriMeshHandle>(allocator);
		GeometryTable.Geometry			= Vector<TriMesh>(allocator);
		GeometryTable.ReferenceCounts	= Vector<size_t>(allocator);
		GeometryTable.Guids				= Vector<GUID_t>(allocator);
		GeometryTable.GeometryIDs		= Vector<const char*>(allocator);
		GeometryTable.FreeList			= Vector<size_t>(allocator);
		GeometryTable.allocator			= allocator;
		GeometryTable.renderSystem		= renderSystem;
	}


	/************************************************************************************************/


	void ReleaseGeometryTable()
	{
		for (auto G : GeometryTable.Geometry)
			ReleaseTriMesh(&G);

		GeometryTable.Geometry.Release();
		GeometryTable.ReferenceCounts.Release();
		GeometryTable.Guids.Release();
		GeometryTable.GeometryIDs.Release();
		GeometryTable.Handles.Release();
		GeometryTable.FreeList.Release();
		GeometryTable.Handle.Release();
	}

	/************************************************************************************************/


	void AddRef(TriMeshHandle TMHandle)
	{
		size_t Index = GeometryTable.Handles[TMHandle];

#ifdef _DEBUG
		if (Index != -1)
			GeometryTable.ReferenceCounts[Index]++;
#else
		GeometryTable.ReferenceCounts[Index]++;
#endif
	}


	/************************************************************************************************/


	bool IsMeshLoaded(GUID_t guid)
	{
		bool res = false;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid) {
				res = true;
				break;
			}
		}

		return res;
	}

	/************************************************************************************************/


	TriMeshHandle CreateMesh(GUID_t GUID)
	{
		auto Available = isAssetAvailable(GUID);
		if (Available)
			return InvalidHandle;

		TriMeshHandle Handle;

		if (!GeometryTable.FreeList.size())
		{
			auto Index = GeometryTable.Geometry.size();
			Handle = GeometryTable.Handles.GetNewHandle();
			GeometryTable.Handles[Handle] = (index_t)Index;

			GeometryTable.Geometry.push_back(TriMesh{});
			GeometryTable.GeometryIDs.push_back(nullptr);
			GeometryTable.Guids.push_back(GUID);
			GeometryTable.ReferenceCounts.push_back(1);
			GeometryTable.Handle.push_back(Handle);

			GeometryTable.Geometry.back().allocator = GeometryTable.allocator;
		}
		else
		{
			auto Index = GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle = GeometryTable.Handles.GetNewHandle();

			GeometryTable.Handles[Handle] = (FlexKit::index_t)Index;
			GeometryTable.GeometryIDs[Index] = nullptr;
			GeometryTable.Guids[Index] = GUID;
			GeometryTable.ReferenceCounts[Index] = 1;
			GeometryTable.Handle[Index] = Handle;
		}

		return Handle;
	}


	/************************************************************************************************/


	void ReleaseMesh(TriMeshHandle TMHandle)
	{
		if (TMHandle == InvalidHandle)
			return;
		// TODO: MAKE ATOMIC
		if (GeometryTable.Handles[TMHandle] == -1)
			return;// Already Released

		size_t Index = GeometryTable.Handles[TMHandle];
		auto Count = --GeometryTable.ReferenceCounts[Index];

		if (Count == 0)
		{
			auto G = GetMeshResource(TMHandle);

			DelayedReleaseTriMesh(GeometryTable.renderSystem, G);

			GeometryTable.FreeList.push_back(Index);
			GeometryTable.Geometry[Index] = TriMesh();
			GeometryTable.Handles[TMHandle] = -1;
			GeometryTable.Handles.RemoveHandle(TMHandle);
		}
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(GUID_t guid, CopyContextHandle copyCtx )
	{
		if (IsMeshLoaded(guid))
		{
			auto mesh = FindMesh(guid);

			if(mesh)
				return mesh.value();
		}

		TriMeshHandle triMesh = LoadTriMeshIntoTable(
			copyCtx == InvalidHandle ? GeometryTable.renderSystem->GetImmediateCopyQueue() : copyCtx, guid);

		return triMesh;
	}


	/************************************************************************************************/


	TriMeshHandle GetMesh(const char* meshID, CopyContextHandle copyCtx )
	{
		auto mesh = FindMesh(meshID);

		if(mesh)
			return mesh.value();

		return LoadTriMeshIntoTable(copyCtx == InvalidHandle ? GeometryTable.renderSystem->GetImmediateCopyQueue() : copyCtx, meshID);
	}


	/************************************************************************************************/


	std::expected<TriMeshHandle, FINDMESH_RES> FindMesh(GUID_t guid)
	{
		size_t location = 0;
		size_t HandleIndex = 0;
		for (auto Entry : GeometryTable.Guids)
		{
			if (Entry == guid) {
				for (auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location)
						return TriMeshHandle{ HandleIndex };

					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return std::unexpected{ FINDMESH_RES::NotFound };
	}


	/************************************************************************************************/


	std::expected<TriMeshHandle, FINDMESH_RES>	FindMesh(const char* ID)
	{
		TriMeshHandle HandleOut = InvalidHandle;
		size_t location = 0;
		size_t HandleIndex = 0;

		for (auto Entry : GeometryTable.GeometryIDs)
		{
			if (!strncmp(Entry, ID, 64)) {
				for (auto index : GeometryTable.Handles.Indexes)
				{
					if (index == location)
						return TriMeshHandle{ HandleIndex };

					++HandleIndex;
				}
				break;
			}
			++location;
		}

		return std::unexpected{ FINDMESH_RES::NotFound };
	}


	/************************************************************************************************/


	TriMesh* GetMeshResource(TriMeshHandle TMHandle)
	{
		FK_ASSERT(TMHandle != InvalidHandle);

#if USING(DEBUGGRAPHICS)
		if (GeometryTable.Handles[TMHandle] == -1)
		{
			DebugBreak();
			return nullptr;

		}
#endif
		return &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
	}


	/************************************************************************************************/


	BoundingSphere GetMeshBoundingSphere(TriMeshHandle TMHandle)
	{
		auto Mesh = &GeometryTable.Geometry[GeometryTable.Handles[TMHandle]];
		return float4{ float3{0}, Mesh->info.r };
	}



	/************************************************************************************************/


	uint32_t FindBufferIdx(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE type)
	{
		return Mesh->lods[lod].bufferSet->FindIdx(type).value_or(-1);
	}


	/************************************************************************************************/



	std::optional<VertexBuffer> FindBufferEntry(TriMesh* mesh, size_t lod, VERTEXBUFFER_TYPE type)
	{
		return mesh->lods[lod].bufferSet->Find(type);
	}


	/************************************************************************************************/


	void Release(IVertexBufferSet* vertexBufferSet)
	{
		FK_ASSERT(0);
#if 0
		for (auto buffer : vertexBufferSet->buffers) {
			if (buffer.apiResource)
				buffer.apiResource->Release();

			buffer.apiResource = nullptr;
			buffer.bufferSizeInBytes = 0;
		}
#endif
	}


	/************************************************************************************************/


	void DelayedRelease(IRenderSystem* RS, IVertexBufferSet* vertexBufferSet)
	{
		FK_ASSERT(0);
#if 0
		for (auto& buffer : vertexBufferSet->buffers) {
			if (buffer.apiResource)
				Push_DelayedRelease(RS, buffer.apiResource);

			buffer.apiResource = nullptr;
			buffer.bufferSizeInBytes = 0;
		}
#endif
	}


	/************************************************************************************************/


	void DelayedReleaseTriMesh(IRenderSystem* RS, TriMesh* T)
	{
		for (auto& detailLevel : T->lods)
		{
			detailLevel.bufferSet->Release();

			for (auto& B : detailLevel.views)
			{
				if (B)
					T->allocator->free(B);

				B = nullptr;
			}

			detailLevel.bufferSet = nullptr;
		}

		T->allocator->free((void*)T->ID);
	}


	/************************************************************************************************/


	void ReleaseTriMesh(TriMesh* T)
	{
		if (T->allocator)
		{
			for (auto& details : T->lods)
			{
				details.bufferSet->Release();

				for (auto& view : details.views)
				{
					if (view)
						T->allocator->free(view);

					view = nullptr;
				}
			}
			T->allocator->free((void*)T->ID);
		}
	}


	/************************************************************************************************/

	TriMesh::LOD_Runtime::LOD_Runtime(const LOD_Runtime& rhs) :
		views			{ rhs.views			},
		lodFileOffset   { rhs.lodFileOffset },
		lodSize         { rhs.lodSize       },
		state           { rhs.state.load()  },
		subMeshes       { rhs.subMeshes     },
		bufferSet		{ rhs.bufferSet		} {}


	/************************************************************************************************/


	TriMesh::LOD_Runtime& TriMesh::LOD_Runtime::operator =(const LOD_Runtime& rhs)
	{
		views			= rhs.views;

		lodFileOffset   = rhs.lodFileOffset;
		lodSize         = rhs.lodSize;

		state           = rhs.state.load();
		subMeshes       = rhs.subMeshes;

		bufferSet		= rhs.bufferSet;

		return *this;
	}


	/************************************************************************************************/


	bool TriMesh::LOD_Runtime::HasTangents() const
	{
		for (auto view : views)
		{
			if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::TANGENT)
				return true;
		}

		return false;
	}


	/************************************************************************************************/


	bool TriMesh::LOD_Runtime::HasNormals() const
	{
		for (auto view : views)
		{
			if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::NORMAL)
				return true;
		}

		return false;
	}


	/************************************************************************************************/


	VertexBufferView* TriMesh::LOD_Runtime::GetNormals()
	{
		for (auto view : views)
		{
			if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::NORMAL)
				return view;
		}

		return nullptr;
	}


	/************************************************************************************************/


	VertexBufferView* TriMesh::LOD_Runtime::GetIndices()
	{
		for (auto view : views)
		{
			if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::INDEX)
				return view;
		}

		return nullptr;
	}


	/************************************************************************************************/


	VertexBufferView* TriMesh::LOD_Runtime::GetPoints()
	{
		for (auto view : views)
		{
			if (view && view->GetBufferType() == VERTEXBUFFER_TYPE::POSITION)
				return view;
		}

		return nullptr;
	}


	/************************************************************************************************/


	size_t TriMesh::LOD_Runtime::GetIndexBufferIndex() const
	{
		return bufferSet->GetIndexBufferIndex();
	}


	/************************************************************************************************/


	size_t TriMesh::LOD_Runtime::GetIndexCount() const
	{
		return bufferSet->At(bufferSet->GetIndexBufferIndex()).Size();
	}


	/************************************************************************************************/


    const uint32_t TriMesh::GetHighestLoadedLodIdx() const
    {
	    for (uint32_t I = 0; I < lods.size(); I++)
	    {
		    if (lods[I].state == LOD_Runtime::LOD_State::Loaded)
			    return I;
	    }

	    return -1;
    }


	/************************************************************************************************/


    const TriMesh::LOD_Runtime& TriMesh::GetHighestLoadedLod() const
    {
	    for (auto& lod : lods)
	    {
		    if (lod.state == LOD_Runtime::LOD_State::Loaded)
			    return lod;
	    }

	    return lods.back();
    }


	/************************************************************************************************/


	TriMesh::LOD_Runtime& TriMesh::GetHighestLoadedLod()
    {
	    for (auto& lod : lods)
	    {
		    if (lod.state == LOD_Runtime::LOD_State::Loaded)
			    return lod;
	    }
		return lods.back();
	}
			

	/************************************************************************************************/


	const	uint32_t				TriMesh::GetLowestLodIdx()		const	noexcept { return uint32_t(lods.size() - 1); }
	const	TriMesh::LOD_Runtime&	TriMesh::GetLowestLoadedLod()	const	noexcept { return lods.back(); }
	TriMesh::LOD_Runtime&			TriMesh::GetLowestLoadedLod()			noexcept { return lods.back(); }


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(CopyContextHandle handle, size_t GUID)
	{	// Make this atomic
		auto Available = isAssetAvailable(GUID);
		if (!Available)
			return InvalidHandle;

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
			if(Asset2TriMesh(*GeometryTable.renderSystem, handle, RHandle, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle;
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
			
			if(Asset2TriMesh(*GeometryTable.renderSystem, handle, RHandle, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles			[Handle]	= (FlexKit::index_t)Index;
				GeometryTable.GeometryIDs		[Index]		= GameRes->ID;
				GeometryTable.Guids				[Index]		= GUID;
				GeometryTable.ReferenceCounts	[Index]		= 1;
				GeometryTable.Handle			[Index]		= Handle;
			}
			else
			{
				Handle = InvalidHandle;
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
			GeometryTable.Handle.push_back			(Handle);

			auto Available = isAssetAvailable(ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(ID);
			auto GameRes = GetAsset(RHandle);
			
			if(Asset2TriMesh(*GeometryTable.renderSystem, handle, RHandle, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle;
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

			if(Asset2TriMesh(*GeometryTable.renderSystem, handle, RHandle, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (FlexKit::index_t)Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle;
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
			GeometryTable.Handle.push_back			(Handle);


			if(Buffer2TriMesh(*GeometryTable.renderSystem, handle, buffer, bufferSize, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= Blob->header.ID;
				GeometryTable.Guids[Index]				= Blob->header.GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
				Handle = InvalidHandle;
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle		= GeometryTable.Handles.GetNewHandle();

			if(Buffer2TriMesh(*GeometryTable.renderSystem, handle, buffer, bufferSize, GeometryTable.allocator, &GeometryTable.Geometry[Index]))
			{
				GeometryTable.Handles[Handle]			= (FlexKit::index_t)Index;
				GeometryTable.GeometryIDs[Index]		= Blob->header.ID;
				GeometryTable.Guids[Index]				= Blob->header.GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
				Handle = InvalidHandle;
		}

		return Handle;
	}



}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2014-2025 Robert May

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
