#pragma once

#include <Geometry.hpp>

namespace FlexKit
{
	struct VertexBufferView;
	struct IVertexBufferSet;

	struct TriMesh
	{
		TriMesh() = default;
		TriMesh(const TriMesh& rhs) = default;

		struct LOD_Runtime
		{
			LOD_Runtime() = default;

			LOD_Runtime(const LOD_Runtime& rhs);

			LOD_Runtime& operator =(const LOD_Runtime& rhs);

			bool HasTangents() const;
			bool HasNormals() const;

			VertexBufferView* GetNormals();
			VertexBufferView* GetIndices();
			VertexBufferView* GetPoints();

			size_t GetIndexBufferIndex() const;
			size_t GetIndexCount() const;

			size_t lodFileOffset;
			size_t lodSize;

			enum class LOD_State
			{
				Unloaded,
				Loaded,
				Loading,
				GPUResourceEvicted
			};

			static_vector<VertexBufferView*>	views;
			IVertexBufferSet*					bufferSet;
			ResourceHandle						blAS = InvalidHandle; // TODO(Wrap this type)

			std::atomic<LOD_State>		state   = LOD_State::Unloaded;
			static_vector<SubMesh, 32>	subMeshes;
		};


		struct MorphTarget
		{
			ResourceHandle gpuResource;
		};


		struct MorphTargetAsset
		{
			char name[32];
			size_t offset;
			size_t size;
		};


		const uint32_t		GetHighestLoadedLodIdx() const;
		const LOD_Runtime&	GetHighestLoadedLod() const;
		LOD_Runtime&		GetHighestLoadedLod();


		const	uint32_t		GetLowestLodIdx()		const	noexcept;
		const	LOD_Runtime&	GetLowestLoadedLod()	const	noexcept;
				LOD_Runtime&	GetLowestLoadedLod()			noexcept;


		size_t animationData	 = EAD_None;
		size_t triMeshID		= INVALIDHANDLE;

		struct SubDivInfo
		{
			size_t  numVertices;
			size_t  numFaces;
			int* numVertsPerFace;
			int* IndicesPerFace;
		}*subDiv = nullptr;

		const char*		ID			= nullptr;
		SkinDeformer*	skinTable	= nullptr;

		GUID_t							assetHandle = INVALIDHANDLE;
		static_vector<LOD_Runtime>		lods;


		struct RInfo
		{
			float3 Offset;
			float3 Min, Max;
			float  r;
		}info;

		// Visibility Information
		AABB			aabb;
		BoundingSphere	bs;

		size_t			skeletonGUID	= INVALIDHANDLE;
		iAllocator*		allocator		= nullptr;

		static_vector<MorphTarget>		morphTargets;
		static_vector<MorphTargetAsset>	morphTargetAssets;
	};


	/************************************************************************************************/


	void						InitiateGeometryTable(IRenderSystem* renderSystem, iAllocator* memory = nullptr);
	void						ReleaseGeometryTable();

	void						AddRef(TriMeshHandle  TMHandle);
	void						ReleaseMesh(TriMeshHandle  TMHandle);
	void						DelayedRelease(IRenderSystem* RS, IVertexBufferSet* vertexBufferSet);
	void						DelayedReleaseTriMesh(IRenderSystem* RS, TriMesh* T);


	TriMeshHandle				CreateMesh(GUID_t ID);
	TriMeshHandle				LoadMesh(GUID_t TMHandle);

	TriMeshHandle				GetMesh(GUID_t, CopyContextHandle copyCtx = InvalidHandle);
	TriMeshHandle				GetMesh(const char* meshID, CopyContextHandle = InvalidHandle);


	TriMesh*					GetMeshResource(TriMeshHandle  TMHandle);
	BoundingSphere				GetMeshBoundingSphere(TriMeshHandle  TMHandle);

	Skeleton*					GetSkeleton(TriMeshHandle  TMHandle);
	size_t						GetSkeletonGUID(TriMeshHandle  TMHandle);
	void						SetSkeleton(TriMeshHandle  TMHandle, Skeleton* S);

	uint32_t FindBufferIdx	(TriMesh* Mesh, size_t lod, VERTEXBUFFER_TYPE type);

	enum class FINDMESH_RES
	{
		NotFound,
		FailedToCreate
	};

	std::expected<TriMeshHandle, FINDMESH_RES>	FindMesh(GUID_t			guid);
	std::expected<TriMeshHandle, FINDMESH_RES >	FindMesh(const char* ID);

	bool						IsMeshLoaded(GUID_t		guid);
	bool						IsSkeletonLoaded(TriMeshHandle	guid);
	bool						HasAnimationData(TriMeshHandle	guid);

	std::optional<VertexBuffer> FindBufferEntry(TriMesh* mesh, size_t lod, VERTEXBUFFER_TYPE type);
	//bool AddVertexBuffer(VERTEXBUFFER_TYPE type, TriMesh* Mesh, size_t lod, static_vector<D3D12_VERTEX_BUFFER_VIEW>& out);


	bool LoadLOD				(TriMesh* triMesh, uint level, IRenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);
	bool LoadAllLODFromMemory	(TriMesh* triMesh, const char* buffer, const size_t bufferSize, IRenderSystem& renderSystem, CopyContextHandle copyCtx, iAllocator& memory);

	bool						Asset2TriMesh(IRenderSystem& RS, CopyContextHandle handle, AssetHandle RHandle, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true);
	bool						Buffer2TriMesh(IRenderSystem& RS, CopyContextHandle handle, const char* buffer, size_t bufferSize, iAllocator* Memory, TriMesh* Out, bool ClearBuffers = true);

	void						LoadTriangleMesh(GUID_t ID, iAllocator* Memory, TriMesh* out);
	TriMeshHandle				LoadTriMeshIntoTable(CopyContextHandle handle, size_t guid);
	TriMeshHandle				LoadTriMeshIntoTable(CopyContextHandle handle, const char* ID);
	TriMeshHandle				LoadTriMeshIntoTable(CopyContextHandle handle, const char* buffer, const size_t bufferSize);

	void ReleaseTriMesh(TriMesh* p);
	void DelayedReleaseTriMesh(IRenderSystem* RS, TriMesh* T);


	TriMeshHandle	CreateCube(IRenderSystem* RS, iAllocator* Memory, float R, GUID_t MeshID);


}	/************************************************************************************************/
