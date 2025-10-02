#include <RenderSystemInterface.hpp>
#include <vulkan/vulkan.h>

namespace VK_internal
{
	using namespace FlexKit;

	struct vkDirectContext : public IDirectContext
	{
		vkDirectContext();

	    void SetDebugName(const char* debugStr) noexcept final;
		void FlushBarriers() noexcept final;

		void CreateAS(const AccelerationStructureDesc&, const TriMesh&) final;
		void BuildBLAS(struct IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace) final;

		void DiscardResource(ResourceHandle resource) final;

		void AddAliasingBarrier(ResourceHandle before, ResourceHandle after) final;
		void AddUAVBarrier(ResourceHandle Handle, uint32_t subresource, DeviceLayout layout, DeviceSyncPoint src, DeviceSyncPoint dst) final;
		void AddPresentBarrier(ResourceHandle Handle, DeviceAccessState Before) final;
		void AddStreamOutBarrier(SOResourceHandle, DeviceAccessState Before, DeviceAccessState State) final;
		void AddCopyResourceBarrier(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State) final;

		void AddGlobalBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter) final;
		void AddTextureBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range) final;
		void AddBufferBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint) final;
		void AddBarriers(std::span<const Barrier> barriers) final;

		void ClearDepthBuffer(ResourceHandle Texture, float ClearDepth);
		void ClearRenderTarget(ResourceHandle Texture, float4 ClearColor);
		void ClearUAVTextureFloat(ResourceHandle UAV, float4 clearColor);
		void ClearUAVTextureUint(ResourceHandle UAV, uint4 clearColor);
		void ClearUAV(ResourceHandle UAV, uint4 clearColor);
		void ClearUAVBuffer(ResourceHandle UAV, uint4 clearColor);
		void ClearUAVBufferRange(ResourceHandle UAV, uint begin, uint end, uint4 clearColor);

		void SetRootSignature(RootSigHandle) final;
		void SetRootSignature(const IRootSignature*) final;
		void SetComputeRootSignature(RootSigHandle) final;
		void SetComputeRootSignature(const IRootSignature*) final;
		void SetPipelineState(const struct IPipelineState* const PSO) final;
		void SetComputePipelineState(const PSOHandle, iAllocator& temp) final;
		void SetGraphicsPipelineState(const PSOHandle, iAllocator& temp) final;

		void SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil, const size_t MIPMapOffset) final;
		void SetRenderTargets2(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV) final;

		void SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets) final;
		void SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset) final;

		void QueueReadBack(ReadBackResourceHandle readBack) final;
		void QueueReadBack(ReadBackResourceHandle readBack, ReadBackEventHandler callback) final;

		void SetDepthStencil(ResourceHandle DS) final;
		void SetInputPrimitive(EInputPrimitive primitive) final;

		void SetGraphicsConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset) final;

		void NullGraphicsConstantBufferView(size_t idx) final;
		void SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset) final;
		void SetGraphicsConstantBufferView(size_t idx, const struct ConstantBufferDataSet& CB) final;
		void SetGraphicsConstantBufferView(size_t idx, DevicePointer) final;
		void SetGraphicsDescriptorTable(size_t idx, const struct DescriptorHeap& DH) final;
		void SetGraphicsDescriptorTable(size_t idx, const DescriptorRange& range) final;
		void SetGraphicsShaderResourceView(size_t idx, ResourceHandle resource, size_t offset) final;
		void SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle resource, size_t offset) final;

		void SetComputeDescriptorTable(size_t idx) final;
		void SetComputeDescriptorTable(size_t idx, const struct DescriptorHeap& DH) final;
		void SetComputeDescriptorTable(size_t idx, const DescriptorRange& range) final;

		void SetComputeConstantBufferView(size_t idx, const ConstantBufferHandle, size_t offset) final;
		void SetComputeConstantBufferView(size_t idx, const struct ConstantBufferDataSet& CB) final;
		void SetComputeConstantBufferView(size_t idx, ResourceHandle, size_t offset, size_t bufferSize) final;
		void SetComputeConstantBufferView(size_t idx, DevicePointer) final;

		void SetComputeShaderResourceView(size_t idx, ResourceHandle resource, size_t offset) final;
		void SetComputeUnorderedAccessView(size_t idx, ResourceHandle resource, size_t offset) final;
		void SetComputeConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset) final;


		void BeginQuery(QueryHandle query, size_t idx) final;
		void EndQuery(QueryHandle query, size_t idx) final;

		void TimeStamp(QueryHandle query, size_t idx) final;

		void SetMarker_DEBUG(const char* str) final;

		void BeginEvent_DEBUG(const char* str) final;
		void EndEvent_DEBUG() final;

		void CopyResource(ResourceHandle dest, ResourceHandle src) final;

		void CopyBufferRegion(
			ResourceHandle	destination,
			ResourceHandle	source,
			size_t			size,
			size_t			destinationOffset,
			size_t			sourceOffset) final;

		void CopyBufferRegion(
			ResourceHandle		destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset,
			size_t				sourceOffset) final;

		void CopyBufferRegion(
			DeviceResource_ptr	destination,
			ResourceHandle		source,
			size_t				size,
			size_t				destinationOffset,
			size_t				sourceOffset) final;

		void CopyBufferRegion(
			DeviceResource_ptr	destination,
			DeviceResource_ptr	source,
			size_t				size,
			size_t				destinationOffset,
			size_t				sourceOffset) final;

		void CopyTextureRegion(
			ResourceHandle		dest,
			size_t				subResourceIdx,
			uint3				XYZ,
			UploadReservation	source,
			uint2				wh) final;

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

		void AddIndexBuffer(TriMesh* Mesh, uint32_t lod) final;
		void SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format) final;
		void SetIndexBuffer(ResourceHandle, DeviceFormat format) final;

		void AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers) final;
		void AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers) final;
		void SetVertexBuffers(const std::initializer_list<VertexBufferEntry>& span) final;
		void SetVertexBuffers(const std::span<const VertexBufferEntry>			span) final;

		void SetVertexBuffers(const std::initializer_list<VertexBufferResource>& span) final;
		void SetVertexBuffers(const std::span<const VertexBufferResource>		span) final;

		void SetVertexBuffers2(const std::span<const VBView> views, uint32_t offset) final;

		void Draw(const size_t VertexCount, const size_t BaseVertex, const size_t baseIndex) final;
		void DrawInstanced(const size_t VertexCount, const size_t BaseVertex, const size_t instanceCount, size_t instanceOffset) final;
		void DrawIndexed(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex) final;
		void DrawIndexedInstanced(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex, const size_t InstanceCount, const size_t InstanceOffset) final;
		void Clear() final;

		void ResolveQuery(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset) final;
		void ResolveQuery(QueryHandle query, size_t begin, size_t end, DeviceResource_ptr destination, size_t destOffset) final;

		void ExecuteIndirect(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount) final;
		void Dispatch(const uint3) final;
		void Dispatch(const IPipelineState* const PSO, const uint3 xyz) final;
		void DispatchRays(const uint3, const DispatchDesc desc) final;
		void DispatchMesh(const uint3) final;

		void SetPredicate(bool Enable, ResourceHandle Handle, size_t, PredicateOp op) final;

		void CopyBuffer(const UploadReservation src, const ResourceHandle destination, const size_t destOffset) final;
		void CopyTexture2D(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize) final;

		void SetRTRead(ResourceHandle Handle) final;
		void SetRTWrite(ResourceHandle Handle) final;
		void SetRTFree(ResourceHandle Handle) final;

		void Close() final;
		void Begin(uint64_t submissionValue);
		void Reset();

		void SetViewports(std::span<const Viewport>		VPs)	final;
		void SetScissorRects(std::span<const Rect>		rects)	final;

		struct vkRenderSystem& RenderSystem() noexcept;

		UploadReservation	ReserveDirectUploadSpace(size_t size, size_t alignment) final;
		IRenderSystem&		GetRenderSystem() noexcept final;

		Vector<Barrier>		pendingBarriers;
		VkCommandPool		commandPool		= nullptr;
		VkCommandBuffer		commandBuffer	= nullptr;
		uint64_t			dispatchValue	= 0;

		Vector<ResourceHandle>	resourcesUsed;
		Vector<VkSemaphore>		waits;
		Vector<VkSemaphore>		signals;
	};
}
