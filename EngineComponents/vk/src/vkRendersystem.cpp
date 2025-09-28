#include "vkCopyContext.hpp"
#include "vkDirectContext.hpp"
#include "vkRendersystem.hpp"
#include <Handle.hpp>
#include <RenderSystemInterface.hpp>

#include "vkDescriptorHeap.hpp"
#include <vulkan/vulkan.hpp>
#include <print>

#ifdef WIN32
#include "vkWin32Surface.hpp"
#endif

namespace VK_internal
{
	using namespace FlexKit;

	VkBool32 VKErrorCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT          messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                 messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT*		pCallbackData,
		void*											pUserData)
	{
		std::print("ERROR: {}", pCallbackData->pMessage);
		return true;
	}


	VkDescriptorPool CreateDescriptorHeap(VkDevice device, size_t numDescriptors)
	{
	    VkDescriptorType typesAvailable[] = {
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		VkMutableDescriptorTypeListEXT availableTypesList[] {
            {
				.descriptorTypeCount	= 6,
				.pDescriptorTypes		= typesAvailable
            },
            {
				.descriptorTypeCount	= 6,
				.pDescriptorTypes		= typesAvailable
			},
		};

		VkMutableDescriptorTypeCreateInfoEXT ext0{
	        .sType							= VkStructureType::VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
		    .pNext							= nullptr,
	        .mutableDescriptorTypeListCount = 2,
	        .pMutableDescriptorTypeLists	= availableTypesList
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateDesc{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext				= &ext0,
			.flags				= VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_SETS_BIT_NV | VK_DESCRIPTOR_POOL_CREATE_ALLOW_OVERALLOCATION_POOLS_BIT_NV,
            .maxSets			= 10000,
            .poolSizeCount		= 0,
            .pPoolSizes			= nullptr
		};

		// Allocate Descriptor pool
		VkDescriptorPool descriptorPool = nullptr;
		if (auto res = vkCreateDescriptorPool(device, &descriptorPoolCreateDesc, nullptr, &descriptorPool); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("Failed to create descriptor set");
			return nullptr;
		}

		return descriptorPool;
	}

	VkBuffer CreateConstantBuffer(VkDevice device, size_t bufferSize)
	{
	    // Create Buffer
		VkBufferCreateInfo createBufferInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,			//VkBufferCreateFlags;
			.size					= bufferSize,	//VkDeviceSize
			.usage					= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		//const uint32_t*        
		};

		VkBuffer buffer;
		vkCreateBuffer(device, &createBufferInfo, nullptr, &buffer);
		return buffer;
	}

	VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const FlexKit::DesciptorHeapLayout& layout, iAllocator& allocator)
	{
		static constexpr VkDescriptorType typesAvailable[] = {
		   VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		   VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		   VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
		   VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
		   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};

		Vector<VkDescriptorSetLayoutBinding>	bindings		{ allocator };
		Vector<VkMutableDescriptorTypeListEXT>	validMutations	{ allocator };

		for (auto& entry : layout.entries)
		{
			VkDescriptorSetLayoutBinding binding;
			binding = VkDescriptorSetLayoutBinding{
						.binding			= entry.registerIdx,
						.descriptorType		= VkDescriptorType::VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
						.descriptorCount	= entry.count,
						.stageFlags			= VK_SHADER_STAGE_ALL,
						.pImmutableSamplers = nullptr };

			bindings.push_back(binding);
			validMutations.push_back(
				VkMutableDescriptorTypeListEXT{
					.descriptorTypeCount	= 6,
				    .pDescriptorTypes		= typesAvailable
				});
		}

		VkMutableDescriptorTypeCreateInfoEXT ext0{
			.sType							= VkStructureType::VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
			.pNext							= nullptr,
			.mutableDescriptorTypeListCount	= (uint32_t)validMutations.size(),
			.pMutableDescriptorTypeLists	= validMutations.data()
		};

		VkDescriptorSetLayoutCreateInfo createLayoutDesc{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &ext0,
			.flags = VkDescriptorSetLayoutCreateFlagBits::VK_DESCRIPTOR_SET_LAYOUT_CREATE_PER_STAGE_BIT_NV,
			.bindingCount = (uint32_t)bindings.size(),
			.pBindings = bindings.data()
		};

		VkDescriptorSetLayout vkLayout = nullptr;
		if (auto res = vkCreateDescriptorSetLayout(device, &createLayoutDesc, nullptr, &vkLayout); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("Failed to create descriptor heap layout!");
			return nullptr;
		}
		else
			return vkLayout;
	}

	VkDescriptorSet AllocateDescriptorSet(VkDevice device, VkDescriptorSetLayout vkLayout, VkDescriptorPool pool)
	{
	    // Allocate descriptor set
		VkDescriptorSetAllocateInfo allocDSDesc{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext				= nullptr,
			.descriptorPool		= pool,
			.descriptorSetCount	= 1,
			.pSetLayouts		= &vkLayout
		};

		VkDescriptorSet descriptorSet;
		if (auto res = vkAllocateDescriptorSets(device, &allocDSDesc, &descriptorSet); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("Failed to allocate descriptor set!");
			return nullptr;
		}
		else
            return descriptorSet;
	}

	struct DescriptorLocation
	{
		uint32_t idx		= 0;
		uint32_t arrayIdx	= 0;
	};

	void CreateCBV(VkDevice device, VkDescriptorSet descriptorSet, VkBuffer buffer, const DescriptorLocation& viewLocation = {})
	{
		VkDescriptorBufferInfo bufferInfo{
			.buffer = buffer,	// VkBuffer	
	        .offset	= 0,		// VkDeviceSize    
	        .range	= 1024		// VkDeviceSize
		};

		VkWriteDescriptorSet write{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext				= nullptr,
            .dstSet				= descriptorSet,
            .dstBinding			= viewLocation.idx,
            .dstArrayElement	= viewLocation.arrayIdx,
            .descriptorCount	= 1,
            .descriptorType		= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo		= &bufferInfo
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	vkRenderSystem::vkRenderSystem(iAllocator& IN_allocator) :
	    resources	{ IN_allocator },
	    allocator	{ IN_allocator }
	{
	    
	}


	bool vkRenderSystem::Initiate(Graphics_Desc& desc)
	{
		allocator = desc.Memory;
		const char* extensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };

	    vkb::InstanceBuilder builder;
		auto instReq = builder.set_app_name("Hello Vulkan")
			.request_validation_layers()
			.set_headless()
		    .enable_extensions(2, extensions)
			//.enable_extension("VK_KHR_win32_surface")
			//.use_default_debug_messenger()
			.set_debug_callback(VKErrorCallback)
			.build();

		if (!instReq)
		{
			return false;
		}


		instance = instReq.value();
		vkb::PhysicalDeviceSelector selector{ instance };

		auto physRequest = selector.set_minimum_version(1, 3)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.add_required_extension("VK_EXT_mutable_descriptor_type")
		    .add_required_extension("VK_KHR_dynamic_rendering")
            .add_required_extension("VK_KHR_swapchain")
			.select_devices();

		if (!physRequest)
			return false;

		auto res = physRequest.value();
		vkb::DeviceBuilder deviceBuilder{ res.back() };
		auto devRequest = deviceBuilder.build();

	    device = devRequest.value();
		auto queueRequest = device.get_queue(vkb::QueueType::graphics);
		if (!queueRequest.has_value())
		{
			return false;
		}

		auto queue = queueRequest.value();
		VkCommandPoolCreateInfo createPoolDesc{
				.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	            .pNext = nullptr,
	            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	            .queueFamilyIndex = 0
		};

		VkCommandPool commandPool;
		if (auto res = vkCreateCommandPool(device, &createPoolDesc, nullptr, &commandPool); res != VK_SUCCESS)
			return false;

		VkCommandBufferAllocateInfo createCommandBuffer{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext				= nullptr,
            .commandPool		= commandPool,
			.level				= VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 32,
		};

		VkCommandBuffer commandBuffer[32];
		vkAllocateCommandBuffers(device, &createCommandBuffer, commandBuffer);

		
		VkDescriptorPool descriptorPool = CreateDescriptorHeap(device, 1000);

		// Create Heap Layout
		DesciptorHeapLayout layout{};
		layout.SetParameterAsCBV(0, 0, 1);
		layout.SetParameterAsSRV(1, 1, 1);

		auto vkLayout = CreateDescriptorSetLayout(device, layout, *allocator);

		auto descriptorSet = AllocateDescriptorSet(device, vkLayout, descriptorPool);

		auto constantBuffer = VK_internal::CreateConstantBuffer(device, 1024u);
		CreateCBV(device, descriptorSet, constantBuffer);

		return true;
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

	void vkRenderSystem::UploadTexture(ResourceHandle, CopyContextHandle, std::byte* buffer, size_t bufferSize)
	{

	}

	void vkRenderSystem::UploadTexture(ResourceHandle handle, CopyContextHandle, struct TextureBuffer* buffer, size_t resourceCount)
	{

	}

	void vkRenderSystem::UpdateResourceByUploadQueue(DeviceResource_ptr Dest, CopyContextHandle, const void* Data, size_t Size, size_t ByteSize, DeviceAccessState EndState)
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
		auto resourceHandle = CreateGPUResourceHandle();


		switch (desc.type)
		{
		case ResourceType::RenderTarget:
		    {
			    resources.Set<ResourceFieldID::APIHandle, ResourceFieldID::Layout>(
				    resourceHandle,
				    vkResourceEntry{
					    .type	= vkResourceEntry::Type::RenderTarget,
					    .image	= (VkImage)desc._ptr
				    },
					DeviceLayout::Common);
		    }	break;
		case ResourceType::DepthTarget:
		    {
		        
		    }	break;
		case ResourceType::UnorderedAccess:
		    {
		        
		    }	break;
		case ResourceType::UnorderedAccessRenderTarget:
		    {

		    }	break;
		case ResourceType::ShaderResource:
		    {

		    }	break;
		case ResourceType::RayTracingStructure:
		    {

		    }	break;
		default:
			throw std::runtime_error("Invalid arguments");
		}

	    return resourceHandle;
	}

	ResourceHandle vkRenderSystem::CreateGPUResourceHandle()
	{
		return resources.AddResource();
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
	{
		vkb::destroy_device(device);
		vkb::destroy_instance(instance);
	}

}


/**********************************************************************

Copyright (c) 2025 Robert May

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
