#include "vkDirectContext.hpp"

namespace VK_internal
{
	using namespace FlexKit;

	void vkDirectContext::SetDebugName(const char* debugStr) noexcept
	{
	}

	void vkDirectContext::FlushBarriers() noexcept
	{}

	void vkDirectContext::CreateAS(const AccelerationStructureDesc&, const TriMesh&)
    {}

	void vkDirectContext::BuildBLAS(struct IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace)
    {}

	void vkDirectContext::DiscardResource(ResourceHandle resource)
	{}

	void vkDirectContext::AddAliasingBarrier(ResourceHandle before, ResourceHandle after)
    {}

    void vkDirectContext::AddUAVBarrier(ResourceHandle Handle, uint32_t subresource, DeviceLayout layout, DeviceSyncPoint src, DeviceSyncPoint dst)
    {}

	void vkDirectContext::AddPresentBarrier(ResourceHandle Handle, DeviceAccessState Before)
    {}

	void vkDirectContext::AddStreamOutBarrier(SOResourceHandle, DeviceAccessState Before, DeviceAccessState State)
	{}

	void vkDirectContext::AddCopyResourceBarrier(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State)
    {}

	void vkDirectContext::AddGlobalBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
    {}

	void vkDirectContext::AddTextureBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range)
    {}

	void vkDirectContext::AddBufferBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint)
    {}

	void vkDirectContext::AddBarriers(std::span<const Barrier> barriers)
    {}

	void vkDirectContext::ClearDepthBuffer(ResourceHandle Texture, float ClearDepth)
    {}

	void vkDirectContext::ClearRenderTarget(ResourceHandle Texture, float4 ClearColor)
    {}

	void vkDirectContext::ClearUAVTextureFloat(ResourceHandle UAV, float4 clearColor)
    {}

	void vkDirectContext::ClearUAVTextureUint(ResourceHandle UAV, uint4 clearColor)
    {}

	void vkDirectContext::ClearUAV(ResourceHandle UAV, uint4 clearColor)
    {}

	void vkDirectContext::ClearUAVBuffer(ResourceHandle UAV, uint4 clearColor)
    {}

	void vkDirectContext::ClearUAVBufferRange(ResourceHandle UAV, uint begin, uint end, uint4 clearColor)
    {}

	void vkDirectContext::SetRootSignature(RootSigHandle)
    {}

	void vkDirectContext::SetRootSignature(const IRootSignature*)
    {}

	void vkDirectContext::SetComputeRootSignature(RootSigHandle)
    {}

	void vkDirectContext::SetComputeRootSignature(const IRootSignature*)
    {}

	void vkDirectContext::SetPipelineState(const struct IPipelineState* const PSO)
    {}

	void vkDirectContext::SetComputePipelineState(const PSOHandle, iAllocator& temp)
    {}

	void vkDirectContext::SetGraphicsPipelineState(const PSOHandle, iAllocator& temp)
    {}

	void vkDirectContext::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil, const size_t MIPMapOffset)
    {}

	void vkDirectContext::SetRenderTargets2(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV)
    {}

	void vkDirectContext::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
    {}

	void vkDirectContext::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{}

	void vkDirectContext::QueueReadBack(ReadBackResourceHandle readBack)
    {}

	void vkDirectContext::QueueReadBack(ReadBackResourceHandle readBack, ReadBackEventHandler callback)
    {}

	void vkDirectContext::SetDepthStencil(ResourceHandle DS)
	{}

    void vkDirectContext::SetInputPrimitive(EInputPrimitive primitive)
    {}

	void vkDirectContext::SetGraphicsConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
    {}

	void vkDirectContext::NullGraphicsConstantBufferView(size_t idx)
    {}

	void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset)
    {}

	void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, const struct ConstantBufferDataSet& CB)
    {}

    void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, DevicePointer)
    {}

	void vkDirectContext::SetGraphicsDescriptorTable(size_t idx, const struct DescriptorHeap& DH)
	{}

	void vkDirectContext::SetGraphicsDescriptorTable(size_t idx, const DescriptorRange& range)
    {}

	void vkDirectContext::SetGraphicsShaderResourceView(size_t idx, ResourceHandle resource, size_t offset)
    {}

	void vkDirectContext::SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle resource, size_t offset)
    {}

	void vkDirectContext::SetComputeDescriptorTable(size_t idx)
    {}

	void vkDirectContext::SetComputeDescriptorTable(size_t idx, const struct DescriptorHeap& DH)
    {}

	void vkDirectContext::SetComputeDescriptorTable(size_t idx, const DescriptorRange& range)
    {}

	void vkDirectContext::SetComputeConstantBufferView(size_t idx, const ConstantBufferHandle, size_t offset)
    {}

	void vkDirectContext::SetComputeConstantBufferView(size_t idx, const struct ConstantBufferDataSet& CB)
    {}

	void vkDirectContext::SetComputeConstantBufferView(size_t idx, ResourceHandle, size_t offset, size_t bufferSize)
    {}

	void vkDirectContext::SetComputeConstantBufferView(size_t idx, DevicePointer)
    {}

	void vkDirectContext::SetComputeShaderResourceView(size_t idx, ResourceHandle resource, size_t offset)
    {}

	void vkDirectContext::SetComputeUnorderedAccessView(size_t idx, ResourceHandle resource, size_t offset)
    {}

	void vkDirectContext::SetComputeConstantValue(size_t idx, size_t valueCount, const void* data_ptr, size_t offset)
    {}

	void vkDirectContext::BeginQuery(QueryHandle query, size_t idx)
    {}

	void vkDirectContext::EndQuery(QueryHandle query, size_t idx)
    {}

	void vkDirectContext::TimeStamp(QueryHandle query, size_t idx)
    {}

	void vkDirectContext::SetMarker_DEBUG(const char* str)
    {}

	void vkDirectContext::BeginEvent_DEBUG(const char* str)
    {}

	void vkDirectContext::EndEvent_DEBUG()
    {}

	void vkDirectContext::CopyResource(ResourceHandle dest, ResourceHandle src)
    {}

	void vkDirectContext::CopyBufferRegion(
		ResourceHandle	destination,
		ResourceHandle	source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset)
    {}

	void vkDirectContext::CopyBufferRegion(
		ResourceHandle		destination,
		DeviceResource_ptr	source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset)
    {}

	void vkDirectContext::CopyBufferRegion(
		DeviceResource_ptr	destination,
		ResourceHandle		source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset)
    {}

	void vkDirectContext::CopyBufferRegion(
		DeviceResource_ptr	destination,
		DeviceResource_ptr	source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset)
    {}

	void vkDirectContext::CopyTextureRegion(
		ResourceHandle		dest,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				wh)
    {}

	void vkDirectContext::CopyTile(
		ResourceHandle			dest,
		const uint3				destTile,
		const size_t			tileOffset,
		const UploadReservation src)
    {}

	void vkDirectContext::ImmediateWrite(
		static_vector<ResourceHandle>		handles,
		static_vector<size_t>				value,
		static_vector<DeviceAccessState>	currentStates,
		static_vector<DeviceAccessState>	finalStates)
    {}

	void vkDirectContext::AddIndexBuffer(TriMesh* Mesh, uint32_t lod)
    {}

	void vkDirectContext::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
    {}

	void vkDirectContext::SetIndexBuffer(ResourceHandle, DeviceFormat format)
    {}

	void vkDirectContext::AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers)
    {}

	void vkDirectContext::AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers)
    {}

	void vkDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferEntry>& span)
    {}

	void vkDirectContext::SetVertexBuffers(const std::span<const VertexBufferEntry> span)
    {}

	void vkDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferResource>& span)
    {}

	void vkDirectContext::SetVertexBuffers(const std::span<const VertexBufferResource> span)
    {}

	void vkDirectContext::SetVertexBuffers2(const std::span<const VBView> views, uint32_t offset)
    {}

	void vkDirectContext::Draw(const size_t VertexCount, const size_t BaseVertex, const size_t baseIndex)
    {}

	void vkDirectContext::DrawInstanced(const size_t VertexCount, const size_t BaseVertex, const size_t instanceCount, size_t instanceOffset)
    {}

	void vkDirectContext::DrawIndexed(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex)
    {}

	void vkDirectContext::DrawIndexedInstanced(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex, const size_t InstanceCount, const size_t InstanceOffset)
    {}

	void vkDirectContext::Clear()
    {}

	void vkDirectContext::ResolveQuery(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset)
	{}

	void vkDirectContext::ResolveQuery(QueryHandle query, size_t begin, size_t end, DeviceResource_ptr destination, size_t destOffset)
    {}

	void vkDirectContext::ExecuteIndirect(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount)
    {}

	void vkDirectContext::Dispatch(const uint3)
    {}

	void vkDirectContext::Dispatch(const IPipelineState* const PSO, const uint3 xyz)
	{}

	void vkDirectContext::DispatchRays(const uint3, const DispatchDesc desc)
    {}

	void vkDirectContext::DispatchMesh(const uint3)
    {}

	void vkDirectContext::SetPredicate(bool Enable, ResourceHandle Handle, size_t, PredicateOp op)
    {}

	void vkDirectContext::CopyBuffer(const UploadReservation src, const ResourceHandle destination, const size_t destOffset)
    {}

	void vkDirectContext::CopyTexture2D(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize)
    {}

	void vkDirectContext::SetRTRead(ResourceHandle Handle)
    {}

	void vkDirectContext::SetRTWrite(ResourceHandle Handle)
    {}

	void vkDirectContext::SetRTFree(ResourceHandle Handle)
    {}

	void vkDirectContext::Close()
    {}

	void vkDirectContext::SetViewports(std::span<const Viewport> VPs)
    {}

	void vkDirectContext::SetScissorRects(std::span<const Rect>	rects)
	{}

	UploadReservation vkDirectContext::ReserveDirectUploadSpace(size_t size, size_t alignment)
	{
		return {};
	}

	IRenderSystem& vkDirectContext::GetRenderSystem() noexcept
	{
		return IRenderSystem::GetInstance();
	}
}
