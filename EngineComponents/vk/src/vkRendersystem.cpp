#include "vkCopyContext.hpp"
#include "vkDirectContext.hpp"
#include "vkRendersystem.hpp"
#include <Handle.hpp>
#include <RenderSystemInterface.hpp>

namespace VK_internal
{
	using namespace FlexKit;
	
	bool vkRenderSystem::Initiate(Graphics_Desc& desc)
	{
		return false;
	}

	void vkRenderSystem::BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc)
	{
	}

	void vkRenderSystem::RegisterPSOLoader(PSOHandle State, LOADSTATE_FN FN)
	{
	}

	void vkRenderSystem::LoadPSOIfRequired(PSOHandle State)
	{
	}

	void vkRenderSystem::QueuePSOLoad(PSOHandle State)
	{
	}

	const IPipelineState* vkRenderSystem::GetPSO(PSOHandle State, iAllocator& temp)
	{
		return nullptr;
	}

	const IRootSignature* const vkRenderSystem::GetPSORootSignature(PSOHandle state) const
	{
		return nullptr;
	}

	std::tuple<IPipelineState*, const IRootSignature*> vkRenderSystem::GetPSOAndRootSignature(PSOHandle stateID, iAllocator& temp) const
	{
		return {};
	}

	size_t vkRenderSystem::GetCurrentCounter()
	{
		return 0;
	}

	SyncPoint vkRenderSystem::GetSubmissionTicket(uint32_t count)
	{
		return {};
	}

	void vkRenderSystem::SyncUploadTo(SyncPoint)
	{

	}

	SyncPoint vkRenderSystem::SyncUploadPoint()
	{
		return {};
	}

	SyncPoint vkRenderSystem::SyncUploadTicket()
	{
		return {};
	}

	void vkRenderSystem::SyncDirectTo(SyncPoint)
	{

	}

	SyncPoint vkRenderSystem::SyncDirectPoint()
	{
		return {};
	}

	SyncPoint vkRenderSystem::SyncSubmittedDirectPoint()
	{
		return {};
	}

	SyncPoint vkRenderSystem::SyncDirectTicket()
	{
		return {};
	}

	void vkRenderSystem::SignalDirect(uint64_t)
	{

	}

	void vkRenderSystem::SubmitUploadQueues(CopyContextHandle* handle, size_t count, std::optional<SyncPoint> syncBefore, std::optional<SyncPoint> syncAfter)
	{

	}

	CopyContextHandle vkRenderSystem::OpenUploadQueue()
	{
		return FlexKit::InvalidHandle;
	}
	CopyContextHandle vkRenderSystem::GetImmediateCopyQueue()
	{
		return FlexKit::InvalidHandle;
	}

	IDirectContext& vkRenderSystem::GetDirectCommandList(std::optional<SyncPoint> ticket)
	{
		static vkDirectContext ctx;
		return ctx;
	}

	ICopyContext& vkRenderSystem::GetCopyContext(CopyContextHandle handle)
	{
		static vkCopyContext ctx;
		return ctx;
	}

	SyncPoint vkRenderSystem::Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync)
	{
		return {};
	}

	void vkRenderSystem::EndFrame()
	{

	}

	void vkRenderSystem::Signal(SyncPoint)
	{

	}

	void vkRenderSystem::WaitForGPU()
	{

	}

	void vkRenderSystem::WaitFor(const uint64_t)
	{

	}

	void vkRenderSystem::WaitFor(const SyncPoint&)
	{
	}

	void vkRenderSystem::SetDebugName(ResourceHandle, const char*)
	{

	}

	void vkRenderSystem::SetDebugName(DeviceHeapHandle, const char*)
	{

	}

	void vkRenderSystem::SetObjectLayout(SOResourceHandle	handle, DeviceLayout state) noexcept
	{

	}

	void vkRenderSystem::SetObjectLayout(ResourceHandle		handle, DeviceLayout state) noexcept
	{

	}

	size_t vkRenderSystem::GetVertexBufferSize(const VertexBufferHandle) const noexcept
	{
		return 0;
	}

	BLAS_PreBuildInfo vkRenderSystem::GetBLASPreBuildInfo(const IVertexBufferSet&)	const noexcept
	{
		return {};
	}

	size_t vkRenderSystem::GetTextureFrameGraphIndex(ResourceHandle) noexcept
	{
		return 0;
	}

	void vkRenderSystem::SetTextureFrameGraphIndex(ResourceHandle, size_t)	noexcept
	{

	}

	void vkRenderSystem::MarkTextureUsed(ResourceHandle Handle)
	{

	}

	DevicePointer vkRenderSystem::GetDevicePointer(const ResourceHandle) const noexcept
	{
		return {};
	}

	DeviceAddressRange vkRenderSystem::GetDeviceRange(const ResourceHandle) const noexcept
	{
		return {};
	}

	DeviceAddressRange vkRenderSystem::GetDeviceRange(const ConstantBufferHandle) const noexcept
	{
		return {};
	}

	DeviceLayout vkRenderSystem::GetObjectLayout(const QueryHandle handle) const noexcept
	{
		return DeviceLayout::Unknown;
	}

	DeviceLayout vkRenderSystem::GetObjectLayout(const SOResourceHandle	handle) const noexcept
	{
		return DeviceLayout::Unknown;
	}

	DeviceLayout vkRenderSystem::GetObjectLayout(const ResourceHandle handle) const noexcept
	{
		return DeviceLayout::Unknown;
	}

	size_t vkRenderSystem::GetResourceSize(ConstantBufferHandle handle) const noexcept
	{
		return 0;
	}

	size_t vkRenderSystem::GetResourceSize(ResourceHandle desc) const noexcept
	{
		return 0;
	}

	size_t vkRenderSystem::GetAllocationSize(ResourceHandle handle) const noexcept
	{
		return 0;
	}

	size_t vkRenderSystem::GetAllocationSize(GPUResourceDesc desc)	const noexcept
	{
		return 0;
	}

	size_t vkRenderSystem::GetTextureElementSize(ResourceHandle   Handle) const
	{
		return 0;
	}

	uint2 vkRenderSystem::GetTextureWH(ResourceHandle Handle) const
	{
		return {};
	}

	DeviceFormat vkRenderSystem::GetTextureFormat(ResourceHandle Handle) const
	{
		return DeviceFormat::UNKNOWN;
	}

	uint8_t	vkRenderSystem::GetTextureMipCount(ResourceHandle Handle) const
	{
		return 0;
	}

	uint2 vkRenderSystem::GetTextureTilingWH(ResourceHandle Handle, const uint mipLevel) const
	{
		return {};
	}

	uint2 vkRenderSystem::GetHeapOffset(ResourceHandle Handle, uint subResourceID) const
	{
		return {};
	}

	TextureDimension vkRenderSystem::GetTextureDimension(ResourceHandle handle) const
	{
		return TextureDimension::Unknown;
	}

	size_t vkRenderSystem::GetTextureArraySize(ResourceHandle handle) const
	{
		return 0;
	}

	DeviceHeap_ptr vkRenderSystem::GetDeviceResource(const DeviceHeapHandle handle) const
	{
		return nullptr;
	}

	DeviceResource_ptr vkRenderSystem::GetDeviceResource(const ReadBackResourceHandle handle) const
	{
		return nullptr;
	}

	DeviceResource_ptr vkRenderSystem::GetDeviceResource(const ConstantBufferHandle	handle) const
	{
		return nullptr;
	}

	DeviceResource_ptr vkRenderSystem::GetDeviceResource(const ResourceHandle handle) const
	{
		return nullptr;
	}

	DeviceResource_ptr vkRenderSystem::GetDeviceResource(const SOResourceHandle	handle) const
	{
		return nullptr;
	}

	DeviceResource_ptr	vkRenderSystem::GetSOCounterResource(const SOResourceHandle handle)	const
	{
		return nullptr;
	}

	size_t vkRenderSystem::GetStreamOutBufferSize(const SOResourceHandle handle) const
	{
		return 0;
	}


	size_t vkRenderSystem::GetVertexBufferOffset(const VertexBufferHandle Handle) const
	{
		return 0;
	}

	bool vkRenderSystem::VertexBufferPush(VertexBufferHandle, void* _ptr, size_t elementSize)
	{
		return false;
	}

	size_t vkRenderSystem::ConstantBufferAlign(ConstantBufferHandle)
	{
		return 0;
	}

	void vkRenderSystem::BackResource(ResourceHandle handle, const GPUResourceDesc& desc) noexcept
	{

	}

	// Resource upload
	void vkRenderSystem::UploadTexture(ResourceHandle, CopyContextHandle, std::byte* buffer, size_t bufferSize)
	{

	}

	void vkRenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle, struct TextureBuffer* buffer, size_t resourceCount)
	{

	}

	void vkRenderSystem::UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState)
	{

	}

	ResourceHandle vkRenderSystem::LoadTexture(TextureBuffer* Buffer, CopyContextHandle handle, DeviceFormat format, iAllocator* allocator)
	{
		return InvalidHandle;
	}


	void vkRenderSystem::SubmitTileMappings(std::span<ResourceHandle> resources, iAllocator* allocator)
	{

	}


	void vkRenderSystem::UpdateTextureTileMappings(const ResourceHandle Handle, std::span<const TileMapping>, iAllocator& temp)
	{

	}

	const TileMapList& vkRenderSystem::GetTileMappings(const ResourceHandle Handle)
	{
		static TileMapList out;
		return out;
	}

	SubAllocation vkRenderSystem::ReserveConstantBuffer(ConstantBufferHandle CB, size_t reserveSize) noexcept
	{
		return {};
	}

	SubAllocation vkRenderSystem::ReserveVertexBuffer(VertexBufferHandle CB, size_t reserveSize)	noexcept
	{
		return {};
	}

	UploadReservation vkRenderSystem::ReserveDirectUploadSpace(size_t size, size_t alignment)	noexcept
	{
		return {};
	}

	UploadReservation vkRenderSystem::ReserveUploadBuffer(const size_t uploadSize, CopyContextHandle)	noexcept
	{
		return {};
	}

	Shader vkRenderSystem::LoadShader(const char* entryPoint, const char* ShaderType, const char* file, const ShaderOptions& options)
	{
		return {};
	}

	Shader vkRenderSystem::LoadShaderLibrary(const char* file, const ShaderOptions& options)
	{
		return {};
	}

	std::expected<Shader, std::string>	vkRenderSystem::LoadRootSignature(const char* file, const char* entry)
	{
		return {};
	}

	// Creation
	std::optional<DescriptorRange> vkRenderSystem::CreateDescriptorRange(const uint32_t descriptorCount)
	{
		return {};
	}

	DeviceHeapHandle vkRenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return InvalidHandle;
	}

	ConstantBufferHandle vkRenderSystem::CreateConstantBuffer(size_t BufferSize, bool GPUResident)
	{
		return InvalidHandle;
	}

	VertexBufferHandle vkRenderSystem::CreateVertexBuffer(size_t BufferSize, bool GPUResident)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateDepthBuffer(const uint2 WH, const bool UseFloat, size_t bufferCount)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateDepthBufferArray(const uint2 WH, const bool UseFloat, const size_t arraySize, const bool buffered, const ResourceAllocationType)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateGPUResource(const GPUResourceDesc& desc)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateGPUResourceHandle()
	{
		return InvalidHandle;
	}

	QueryHandle	vkRenderSystem::CreateOcclusionBuffer(size_t Size)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateUAVBufferResource(size_t bufferHandle, bool tripleBuffer)
	{
		return InvalidHandle;
	}

	ResourceHandle vkRenderSystem::CreateUAVTextureResource(const uint2 WH, const DeviceFormat, const bool RenderTarget)
	{
		return InvalidHandle;
	}

	SOResourceHandle vkRenderSystem::CreateStreamOutResource(size_t bufferHandle, bool tripleBuffer)
	{
		return InvalidHandle;
	}

	QueryHandle	vkRenderSystem::CreateSOQuery(size_t SOIndex, size_t count)
	{
		return InvalidHandle;
	}

	QueryHandle	vkRenderSystem::CreateTimeStampQuery(size_t count)
	{
		return InvalidHandle;
	}

	IndirectLayout vkRenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IRootSignature* signature)
	{
	    return {};
	}

	ReadBackResourceHandle vkRenderSystem::CreateReadBackBuffer(const size_t bufferSize)
	{
		return InvalidHandle;
	}

	bool vkRenderSystem::CreatePipelineBuilder(std::byte* _ptr, size_t bufferSize)
	{
		return false;
	}

	void vkRenderSystem::CreateTextureView(ResourceHandle, DescHeapPOS)
	{
	    
	}

	const IRootSignature* vkRenderSystem::Library(ROOTLIBRARYSIG ID) const noexcept
	{
		return nullptr;
	}

	ResourceHandle vkRenderSystem::DefaultTexture() const noexcept
	{
	    return InvalidHandle;
	}

	void vkRenderSystem::ResetConstantBuffer(ConstantBufferHandle constant)
    {}

    void vkRenderSystem::ResetVertexBuffer(VertexBufferHandle constant)
    {}

    void vkRenderSystem::ResetQuery(QueryHandle handle)
    {}

	void vkRenderSystem::ReleaseCB(ConstantBufferHandle)
    {}

    void vkRenderSystem::ReleaseVB(VertexBufferHandle)
    {}

    void vkRenderSystem::ReleaseResource(ResourceHandle)
    {}

	void vkRenderSystem::ReleaseReadBack(ReadBackResourceHandle)
    {}

    void vkRenderSystem::ReleaseHeap(DeviceHeapHandle)
    {}

    void vkRenderSystem::ReleaseQuery(QueryHandle)
    {}

    void vkRenderSystem::ReleaseDescriptorRange(DescriptorRange, uint64_t)
    {}

	void vkRenderSystem::Release()
    {}

}
