#pragma once
#include <RenderSystemInterface.hpp>
#include <directx/d3d12.h>
#include "dxFrameBufferedObjects.hpp"
#include "dxUpload.hpp"

namespace dx_Internal
{
	using namespace FlexKit;
	using FlexKit::ConstantBufferDataSet;

	class dxRenderSystem;

	class dxDirectContext : public IDirectContext
	{
	public:
		dxDirectContext(dxRenderSystem*	renderSystem_IN	= nullptr, iAllocator*	allocator = nullptr);
		dxDirectContext(dxDirectContext&& RHS);

		dxDirectContext& operator = (dxDirectContext&& RHS);
		dxDirectContext				(const dxDirectContext& RHS) = delete;
		dxDirectContext& operator = (const dxDirectContext& RHS) = delete;

		void Release();

		void CreateAS(const AccelerationStructureDesc&, const TriMesh&)  final;
		void BuildBLAS(IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace) final;

		void DiscardResource(ResourceHandle resource) final;

		void AddAliasingBarrier			(ResourceHandle before, ResourceHandle after) final;
		void AddUAVBarrier				(ResourceHandle Handle = InvalidHandle, uint32_t subresource = -1, DeviceLayout layout = DeviceLayout::Unknown, DeviceSyncPoint src = Sync_All, DeviceSyncPoint dst = Sync_All) final;
		void AddPresentBarrier			(ResourceHandle Handle,	DeviceAccessState Before) final;
		void AddCopyResourceBarrier		(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State) final;

		void AddGlobalBarrier			(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter) final;
		void AddTextureBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range = {}) final;
		void AddBufferBarrier			(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint);
		void AddBarriers				(std::span<const Barrier> barriers) final;

		void ClearDepthBuffer		(ResourceHandle resource, float clearDepth = 0.0f, uint32_t stencil = 0x00) final; // Assumes full-screen Clear
		void ClearRenderTarget		(ResourceHandle Texture, float4 ClearColor = float4(0.0f)) final; // Assumes full-screen Clear
		void ClearUAVTextureFloat	(ResourceHandle UAV, float4 clearColor = float4(0, 0, 0, 0)) final;
		void ClearUAVTextureUint	(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) final;
		void ClearUAV				(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) final;
		void ClearUAVBuffer			(ResourceHandle UAV, uint4 clearColor = uint4{ 0, 0, 0, 0 }) final;
		void ClearUAVBufferRange	(ResourceHandle UAV, uint begin, uint end, uint4 clearColor = uint4{ 0, 0, 0, 0 }) final;

		void SetRootSignature			(RootSigHandle) final;
		void SetRootSignature			(const IPipelineInterface*) final;
		void SetComputeRootSignature	(RootSigHandle) final;
		void SetComputeRootSignature	(const IPipelineInterface*) final;
		void SetPipelineState			(const IPipelineState* const PSO) final;
		void SetComputePipelineState	(const PSOHandle, iAllocator& temp) final;
		void SetGraphicsPipelineState	(const PSOHandle, iAllocator& temp) final;
		void SetRTStateObject			(ShaderID program, const IPipelineStateLibrary* const object) final;

		void SetRenderTargets			(const static_vector<ResourceHandle> RTs, bool DepthStecil = false, ResourceHandle DepthStencil = InvalidHandle, const size_t MIPMapOffset = 0) final;
		void SetRenderTargets2			(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV) final;

		void SetViewports				(static_vector<D3D12_VIEWPORT, 16>	VPs);
		void SetViewports				(std::span<const D3D12_VIEWPORT>	VPs);
		void SetScissorRects			(static_vector<D3D12_RECT, 16>		rects);
		void SetScissorRects			(std::span<const D3D12_RECT>		rects);

		void SetViewports				(std::span<const Viewport>	VPs)	final;
		void SetScissorRects			(std::span<const Rect>		rects)	final;

		void SetScissorAndViewports		(static_vector<ResourceHandle, 16>	RenderTargets) final;
		void SetScissorAndViewports2	(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset = 0) final;

		void QueueReadBack				(ReadBackResourceHandle readBack) final;
		void QueueReadBack				(ReadBackResourceHandle readBack, ReadBackEventHandler callback) final;

		void SetDepthStencil			(ResourceHandle DS) final;
		void SetInputPrimitive			(EInputPrimitive primitive) final;

		void SetGraphicsConstantValue	(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0) final;

		void NullGraphicsConstantBufferView	(size_t idx) final;
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferHandle CB, size_t Offset = 0) final;
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBufferDataSet& CB) final;
		void SetGraphicsConstantBufferView	(size_t idx, const ConstantBuffer& CB);
		void SetGraphicsConstantBufferView	(size_t idx, DevicePointer) final;
		void SetGraphicsDescriptorSet		(size_t idx, const DescriptorSet& DH) final;
		void SetGraphicsDescriptorSet		(size_t idx, const DescriptorRange& range) final;
		void SetGraphicsShaderResourceView	(size_t idx, FrameBufferedResource& Resource, size_t Count, size_t ElementSize);
		void SetGraphicsShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0) final;
		void SetGraphicsUnorderedAccessView (size_t idx, ResourceHandle resource, size_t offset = 0) final;


		void SetComputeDescriptorSet		(size_t idx) final;
		void SetComputeDescriptorSet		(size_t idx, const DescriptorSet& DH) final;
		void SetComputeDescriptorSet		(size_t idx, const DescriptorRange& range) final;

		void SetComputeConstantBufferView	(size_t idx, const ConstantBufferHandle, size_t offset) final;
		void SetComputeConstantBufferView	(size_t idx, const ConstantBufferDataSet& CB) final;
		void SetComputeConstantBufferView	(size_t idx, ResourceHandle, size_t offset = 0, size_t bufferSize = 256) final;
		void SetComputeConstantBufferView	(size_t idx, DevicePointer) final;

		void SetComputeShaderResourceView	(size_t idx, ResourceHandle resource, size_t offset = 0) final;
		void SetComputeUnorderedAccessView	(size_t idx, ResourceHandle resource, size_t offset = 0) final;
		void SetComputeConstantValue		(size_t idx, size_t valueCount, const void* data_ptr, size_t offset = 0) final;


		void BeginQuery	(QueryHandle query, size_t idx) final;
		void EndQuery	(QueryHandle query, size_t idx) final;

		void TimeStamp	(QueryHandle query, size_t idx) final;

		void SetMarker_DEBUG(const char* str) final;

		void BeginEvent_DEBUG(const char* str) final;
		void EndEvent_DEBUG() final;

		void CopyResource(ResourceHandle dest, ResourceHandle src) final;

		void CopyBufferRegion(
			ResourceHandle	destination,
			ResourceHandle	source,
			size_t			size,
			size_t			destinationOffset	= 0,
			size_t			sourceOffset		= 0) final;

		void CopyBufferRegion(
			ResourceHandle		destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset	= 0,
			size_t				sourceOffset		= 0) final;

		void CopyBufferRegion(
			DeviceResource_ptr	destination,
			ResourceHandle		source,
			size_t				size,
			size_t				destinationOffset	= 0,
			size_t				sourceOffset		= 0) final;

		void CopyBufferRegion(
			DeviceResource_ptr	destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset	= 0,
			size_t				sourceOffset		= 0) final;

		void CopyTextureRegion(
			DeviceResource_ptr	destination,
			size_t				subResourceIdx,
			uint3				XYZ,
			UploadReservation	source,
			uint2				WH,
			DeviceFormat		format);

		void CopyTextureRegion(
			ResourceHandle		dest,
			size_t				subResourceIdx,
			uint3				XYZ,
			UploadReservation	source,
			uint2				wh) final;

		void CopyTile(
			DeviceResource_ptr		dest,
			const uint3				destTile,
			const size_t			tileOffset,
			const UploadReservation src);

		void CopyTile(
			ResourceHandle			dest,
			const uint3				destTile,
			const size_t			tileOffset,
			const UploadReservation src) final;

		void ImmediateWrite(
			static_vector<ResourceHandle>		handles,
			static_vector<size_t>				value,
			static_vector<DeviceAccessState>	currentStates,
			static_vector<DeviceAccessState>	finalStates) final;


		void CopyUInt64(
			static_vector<ID3D12Resource*>			source,
			static_vector<DeviceAccessState>		sourceState,
			static_vector<size_t>					sourceoffsets,
			static_vector<ID3D12Resource*>			destination,
			static_vector<DeviceAccessState>		destinationState,
			static_vector<size_t>					destinationoffset);

		void AddIndexBuffer			(TriMesh* Mesh, uint32_t lod = 0) final;
		void SetIndexBuffer			(VertexBufferEntry buffer, DeviceFormat format = DeviceFormat::R32_UINT) final;
		void SetIndexBuffer			(ResourceHandle, DeviceFormat format = DeviceFormat::R32_UINT) final;

		void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers = nullptr) final;
		void AddVertexBuffers		(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers = nullptr) final;
		void SetVertexBuffers		(const std::initializer_list<VertexBufferEntry>&	span) final;
		void SetVertexBuffers		(const std::span<const VertexBufferEntry>			span) final;

		void SetVertexBuffers		(const std::initializer_list<VertexBufferResource>&	span) final;
		void SetVertexBuffers		(const std::span<const VertexBufferResource>		span) final;

		void SetVertexBuffers2		(const std::initializer_list<D3D12_VERTEX_BUFFER_VIEW>&	views);
		void SetVertexBuffers2		(const std::span<const D3D12_VERTEX_BUFFER_VIEW>		views);
		void SetVertexBuffers2		(const std::span<const VBView>							views, uint32_t offset = 0) final;

		void Draw					(const size_t VertexCount, const size_t BaseVertex = 0, const size_t baseIndex = 0) final;
		void DrawInstanced			(const size_t VertexCount, const size_t BaseVertex = 0, const size_t instanceCount = 0, size_t instanceOffset = 0) final;
		virtual void DrawIndexed			(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0) final;
		void DrawIndexedInstanced	(const size_t IndexCount, const size_t IndexOffet = 0, const size_t BaseVertex = 0, const size_t InstanceCount = 1, const size_t InstanceOffset = 0) final;
		void Clear					() final;

		void ResolveQuery			(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset) final;
		void ResolveQuery			(QueryHandle query, size_t begin, size_t end, DeviceResource_ptr destination, size_t destOffset) final;

		void ExecuteIndirect		(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset = 0, size_t executionCount = 1);
		void Dispatch				(const uint3) final;
		void Dispatch				(const IPipelineState* const PSO, const uint3 xyz)  final { SetPipelineState(PSO); Dispatch(xyz); }
		void DispatchRays			(const uint3, const DispatchDesc desc) final;
		void DispatchMesh			(const uint3) final;

		virtual void FlushBarriers() noexcept final;

		void SetPredicate(bool Enable, ResourceHandle Handle = InvalidHandle, size_t = 0, PredicateOp op = PredicateOp::EqualZero) final;

		void CopyBuffer		(const UploadReservation src, const ResourceHandle destination, const size_t destOffset = 0) final;
		void CopyTexture2D	(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize) final;

		void CopyTexture2D(auto des, auto src);

		void SetRTRead	(ResourceHandle Handle) final;
		void SetRTWrite	(ResourceHandle Handle) final;
		void SetRTFree	(ResourceHandle Handle) final;

		void				Close() final;
		dxDirectContext&	Reset(DescriptorRange range, const size_t newDispatchIdx, struct ID3D12DescriptorHeap* heap);

		void SetDebugName(const char* ID) noexcept final;

		UploadReservation		ReserveDirectUploadSpace(size_t size, size_t alignment = 256) noexcept final;


		const struct RootSignature*	CurrentGraphicsRootSig() const		{ return CurrentRootSignature; }
		const struct RootSignature*	CurrentComputeRootSig() const		{ return CurrentComputeRootSignature; }


		// Not Yet Implemented
		void SetUAVRead();
		void SetUAVWrite();
		void SetUAVFree();

		IRenderSystem& GetRenderSystem() noexcept final;

		void							QueueReadBacks();

		std::optional<DescHeapPOS>		ReserveSRV(size_t count);
		DescHeapPOS						ReserveDSV(size_t count);
		DescHeapPOS						ReserveRTV(size_t count);
		DescHeapPOS						ReserveSRVLocal(size_t count);


		void							ResetRTV();
		void							ResetDSV();
		void							ResetSRV();

		uint64_t							GetCounter()		{ return dispatchIdx; }
		struct ID3D12GraphicsCommandList*	GetCommandList()	{ return DeviceContext; }

		dxRenderSystem* renderSystem = nullptr;

		void BeginMarker(const char* str);
		void EndMarker(const char* str);

	//private:

		DescHeapPOS GetDepthDesciptor(ResourceHandle resource);

		void UpdateResourceStates();

		struct ID3D12CommandAllocator*			commandAllocator		= nullptr;
		struct ID3D12GraphicsCommandList10*		DeviceContext			= nullptr;

#if USING(DEBUGGRAPHICS)
		struct ID3D12DebugCommandList*			debugCommandList		= nullptr;
#endif

		const RootSignature*			CurrentRootSignature		= nullptr;
		const RootSignature*			CurrentComputeRootSignature	= nullptr;
		ID3D12PipelineState*			CurrentPipelineState		= nullptr;

		ID3D12DescriptorHeap*			descHeapRTV				= nullptr;
		ID3D12DescriptorHeap*			descHeapSRVLocal		= nullptr; // CPU visable only
		ID3D12DescriptorHeap*			descHeapDSV				= nullptr;


		DescriptorRange				shaderResources;
		size_t						heapUsed	= 0;
		uint64_t					dispatchIdx = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSV_CPU;

		D3D12_CPU_DESCRIPTOR_HANDLE RTVPOSCPU;
		D3D12_CPU_DESCRIPTOR_HANDLE DSVPOSCPU;

		D3D12_CPU_DESCRIPTOR_HANDLE SRV_LOCAL_CPU;

		size_t	RenderTargetCount;
		bool	DepthStencilEnabled;

		
		static_vector<ResourceHandle, 16>		RenderTargets;
		static_vector<DescriptorSet*>			DesciptorHeaps;
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		struct RTV_View {
			ResourceHandle	resource;
			DescHeapPOS		descriptor;
		};

		static_vector<Barrier, 128>					pendingBarriers; // Barriers potentially needed
		static_vector<Barrier, 128>					queuedBarriers; // Barriers required
		static_vector<RTV_View, 128>				renderTargetViews;
		static_vector<RTV_View, 128>				depthStencilViews;
		static_vector<ReadBackResourceHandle, 128>	queuedReadBacks;

		iAllocator*									Memory;

#if USING(AFTERMATH)
public:
		GFSDK_Aftermath_ContextHandle   AFTERMATH_context;
private:
#endif
	};

	

	class CopyContext : public ICopyContext
	{
	public:

		void                Barrier(ID3D12Resource* destination, DeviceAccessState before, DeviceAccessState after);
		void                Barrier(ResourceHandle destination, DeviceAccessState before, DeviceAccessState after) final;

		UploadReservation   Reserve(const size_t reserveSize, const size_t reserveAignement = 256);

		void                CopyBuffer(GPURange dest, void* source_ptr, uint64_t size);
		void                CopyBuffer(ResourceHandle , const size_t destinationOffset, UploadReservation);
		void                CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, UploadReservation);
		void                CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, ID3D12Resource* source, const size_t sourceOffset, const size_t copySize);
		void                CopyTextureRegion(ID3D12Resource*, size_t subResourceIdx, uint3 XYZ, UploadReservation source, uint2 WH, DeviceFormat format);
		void                CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src);

		bool                    IsSubResourceTiled(ID3D12Resource* Resource, const size_t level) const;

		void                    flushPendingBarriers();

		virtual struct IRenderSystem& GetRenderSystem() noexcept final;

		ID3D12GraphicsCommandList* GetAPIObject() { return commandList; }

		ID3D12CommandAllocator*		commandAllocator    = nullptr;
		ID3D12GraphicsCommandList*	commandList         = nullptr;
		size_t                      counter             = 0;
		HANDLE                      eventHandle;

		dxUploadBuffer                uploadBuffer;

		Vector<ID3D12Resource*>                 freeResources;
		static_vector<D3D12_RESOURCE_BARRIER>   pendingBarriers;
	};
}
