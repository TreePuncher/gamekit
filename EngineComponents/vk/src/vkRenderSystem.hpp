#include <RenderSystemInterface.hpp>

namespace VK_internal
{
    using namespace FlexKit;

    class vkRenderSystem : public IRenderSystem
    {
    public:
        bool													Initiate(Graphics_Desc& desc) = 0;

		void													BuildLibrary			(PSOHandle State, const PipelineStateLibraryDesc) final;
		void													RegisterPSOLoader		(PSOHandle State, LOADSTATE_FN FN) final;
		void													LoadPSOIfRequired		(PSOHandle State) final;
		void													QueuePSOLoad			(PSOHandle State) final;

		const IPipelineState*									GetPSO					(PSOHandle State, iAllocator& temp) final;
		const IRootSignature* const 							GetPSORootSignature		(PSOHandle state) const final;
		std::tuple<IPipelineState*, const IRootSignature*>		GetPSOAndRootSignature	(PSOHandle stateID, iAllocator& temp) const final;

		// Sync functions
		size_t		GetCurrentCounter()						final;
		SyncPoint	GetSubmissionTicket(uint32_t count)		final; 
		void		SyncUploadTo(SyncPoint)					final;
		SyncPoint	SyncUploadPoint()						final;
		SyncPoint	SyncUploadTicket()						final;

		void		SyncDirectTo(SyncPoint)		final;
		SyncPoint	SyncDirectPoint()			final;
		SyncPoint	SyncSubmittedDirectPoint()	final;
		SyncPoint	SyncDirectTicket()			final;

		void			SignalDirect(uint64_t)	final;
		//virtual void			SignalCopy(uint64_t)	= 0;
		//virtual void			SignalCompute(uint64_t)	= 0;

		// Copy Queue
		void					SubmitUploadQueues(CopyContextHandle* handle, size_t count = 1, std::optional<SyncPoint> syncBefore = {}, std::optional<SyncPoint> syncAfter = {}) final;
		CopyContextHandle		OpenUploadQueue()		final;
		CopyContextHandle		GetImmediateCopyQueue() final;

		IDirectContext&	GetDirectCommandList(std::optional<SyncPoint> ticket = {}) final;
		ICopyContext&	GetCopyContext(CopyContextHandle handle = InvalidHandle) final;

		SyncPoint	Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync = {})	final;

		void		EndFrame()																	final;
		void		Signal(SyncPoint)															final;

		void		WaitForGPU()				final;
		void		WaitFor(const uint64_t)		final;
		void		WaitFor(const SyncPoint&)	final;

		// Debug
		void		SetDebugName(ResourceHandle, const char*)		final;
		void		SetDebugName(DeviceHeapHandle, const char*)		final;


		// Objects methods
		void		SetObjectLayout(SOResourceHandle	handle, DeviceLayout state) noexcept final;
		void		SetObjectLayout(ResourceHandle		handle, DeviceLayout state) noexcept final;

		// Info queries
		size_t				GetVertexBufferSize		(const VertexBufferHandle)	const noexcept final;
		BLAS_PreBuildInfo	GetBLASPreBuildInfo		(const IVertexBufferSet&)	const noexcept final;

		size_t				GetTextureFrameGraphIndex(ResourceHandle)			noexcept final;
		void				SetTextureFrameGraphIndex(ResourceHandle, size_t)	noexcept final;

		void				MarkTextureUsed			(ResourceHandle Handle) = 0;

		DevicePointer		GetDevicePointer		(const ResourceHandle)			const noexcept final;

		DeviceAddressRange	GetDeviceRange			(const ResourceHandle)			const noexcept final;
		DeviceAddressRange	GetDeviceRange			(const ConstantBufferHandle)	const noexcept final;

		DeviceLayout		GetObjectLayout			(const QueryHandle		handle) const noexcept final;
		DeviceLayout		GetObjectLayout			(const SOResourceHandle	handle) const noexcept final;
		DeviceLayout		GetObjectLayout			(const ResourceHandle	handle) const noexcept final;

		size_t				GetResourceSize			(ConstantBufferHandle handle)	const noexcept final;
		size_t				GetResourceSize			(ResourceHandle desc)			const noexcept final;

		size_t				GetAllocationSize		(ResourceHandle handle) const noexcept final; // Includes padding and alignment
		size_t				GetAllocationSize		(GPUResourceDesc desc)	const noexcept final; // Includes padding and alignment

		size_t				GetTextureElementSize	(ResourceHandle   Handle) const final;
		uint2				GetTextureWH			(ResourceHandle   Handle) const final;

		DeviceFormat		GetTextureFormat		(ResourceHandle Handle) const final;
		uint8_t				GetTextureMipCount		(ResourceHandle Handle) const final;
		uint2				GetTextureTilingWH		(ResourceHandle Handle, const uint mipLevel)	const final;
		uint2				GetHeapOffset			(ResourceHandle Handle, uint subResourceID = 0) const final;

		TextureDimension	GetTextureDimension		(ResourceHandle handle) const final;
		size_t				GetTextureArraySize		(ResourceHandle handle) const final;

		DeviceHeap_ptr		GetDeviceResource(const DeviceHeapHandle        handle) const final;
		DeviceResource_ptr	GetDeviceResource(const ReadBackResourceHandle	handle) const final;
		DeviceResource_ptr	GetDeviceResource(const ConstantBufferHandle	handle) const final;
		DeviceResource_ptr	GetDeviceResource(const ResourceHandle		    handle) const final;
		DeviceResource_ptr	GetDeviceResource(const SOResourceHandle		handle) const final;

		DeviceResource_ptr	GetSOCounterResource(const SOResourceHandle handle)		const final;
		size_t				GetStreamOutBufferSize(const SOResourceHandle handle)	const final;
		size_t				GetVertexBufferOffset(const VertexBufferHandle Handle)	const final;

		bool				VertexBufferPush		(VertexBufferHandle, void* _ptr, size_t elementSize) final;
		size_t				ConstantBufferAlign		(ConstantBufferHandle) final;

		void				BackResource(ResourceHandle handle, const GPUResourceDesc& desc) noexcept final;

		// Resource upload
		void				UploadTexture(ResourceHandle, CopyContextHandle, std::byte* buffer, size_t bufferSize) final; // Uses Upload Queue
		void				UploadTexture(ResourceHandle handle, CopyContextHandle, struct TextureBuffer* buffer, size_t resourceCount) final; // Uses Upload Queue
		void				UpdateResourceByUploadQueue(ID3D12Resource* Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState) final;

		ResourceHandle		LoadTexture(TextureBuffer* Buffer, CopyContextHandle handle, DeviceFormat format, iAllocator* allocator) final;

		void				SubmitTileMappings			(std::span<ResourceHandle> resources, iAllocator* allocator) final;
		void				UpdateTextureTileMappings	(const ResourceHandle Handle, std::span<const TileMapping>, iAllocator& temp) final;
		const TileMapList&	GetTileMappings				(const ResourceHandle Handle) final;

		SubAllocation		ReserveConstantBuffer	(ConstantBufferHandle CB, size_t reserveSize)	noexcept final;
		SubAllocation		ReserveVertexBuffer		(VertexBufferHandle CB, size_t reserveSize)		noexcept final;
		UploadReservation	ReserveDirectUploadSpace(size_t size, size_t alignment)					noexcept final;
		UploadReservation	ReserveUploadBuffer(const size_t uploadSize, CopyContextHandle)			noexcept final;

		// Shader
		Shader								LoadShader(const char* entryPoint, const char* ShaderType, const char* file, const ShaderOptions& options = {}) final;
		Shader								LoadShaderLibrary(const char* file, const ShaderOptions& options = {}) final;
		std::expected<Shader, std::string>	LoadRootSignature(const char* file, const char* entry) final;

		// Creation
		std::optional<DescriptorRange>	CreateDescriptorRange(const uint32_t descriptorCount = 1) final;
		DeviceHeapHandle				CreateHeap(const size_t heapSize, const uint32_t flags) final;
		ConstantBufferHandle			CreateConstantBuffer(size_t BufferSize, bool GPUResident = true) final;
		VertexBufferHandle				CreateVertexBuffer(size_t BufferSize, bool GPUResident = true) final;
		ResourceHandle					CreateDepthBuffer(const uint2 WH, const bool UseFloat = false, size_t bufferCount = 3) final;
		ResourceHandle					CreateDepthBufferArray(const uint2 WH, const bool UseFloat = false, const size_t arraySize = 1, const bool buffered = true, const ResourceAllocationType = ResourceAllocationType::Committed) final;
		ResourceHandle					CreateGPUResource(const GPUResourceDesc& desc) final;
		ResourceHandle					CreateGPUResourceHandle() final;
		QueryHandle						CreateOcclusionBuffer(size_t Size) final;
		ResourceHandle					CreateUAVBufferResource(size_t bufferHandle, bool tripleBuffer = true) final;
		ResourceHandle					CreateUAVTextureResource(const uint2 WH, const DeviceFormat, const bool RenderTarget = false) final;
		SOResourceHandle				CreateStreamOutResource(size_t bufferHandle, bool tripleBuffer = true) final;
		QueryHandle						CreateSOQuery(size_t SOIndex, size_t count) final;
		QueryHandle						CreateTimeStampQuery(size_t count) final;
		IndirectLayout					CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IRootSignature* signature = nullptr);
		ReadBackResourceHandle			CreateReadBackBuffer(const size_t bufferSize) final;
		bool							CreatePipelineBuilder(std::byte* _ptr, size_t bufferSize) final;
	    void							CreateTextureView(ResourceHandle, DescHeapPOS) final;


		const IRootSignature*	Library(ROOTLIBRARYSIG ID) const noexcept final;
		ResourceHandle			DefaultTexture() const noexcept;

		// Resetable resources
		void ResetConstantBuffer(ConstantBufferHandle constant) final;
		void ResetVertexBuffer(VertexBufferHandle constant) final;
		void ResetQuery(QueryHandle handle) final;

		// Release
		void ReleaseCB(ConstantBufferHandle) final;
		void ReleaseVB(VertexBufferHandle) final;
		void ReleaseResource(ResourceHandle) final;
		void ReleaseReadBack(ReadBackResourceHandle) final;
		void ReleaseHeap(DeviceHeapHandle) final;
		void ReleaseQuery(QueryHandle) final;
		void ReleaseDescriptorRange(DescriptorRange, uint64_t) final;
		void Release() final;
    };
}
