#include "vkDirectContext.hpp"
#include "vkRenderSystem.hpp"
#include <VkBootstrapDispatch.h>
#include <VkBootstrap.h>

namespace VK_internal
{
	using namespace FlexKit;


	vkDirectContext::vkDirectContext()
	{
		auto& renderSystem = (vkRenderSystem&)vkRenderSystem::GetInstance();
		pendingBarriers	= Vector<Barrier>{ renderSystem.allocator };
		resourcesUsed	= Vector<ResourceHandle>{ renderSystem.allocator };
		waits			= Vector<VkSemaphore>{ renderSystem.allocator };
		signals			= Vector<VkSemaphore>{ renderSystem.allocator };

		VkCommandPoolCreateInfo createPoolDesc{
				.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext				= nullptr,
				.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex	= 0
		};

		if (auto res = vkCreateCommandPool(renderSystem.device, &createPoolDesc, nullptr, &commandPool); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to create command pool!"};

        VkCommandBufferAllocateInfo createCommandBuffer{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext					= nullptr,
			.commandPool			= commandPool,
			.level					= VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount		= 1,
		};

		if (auto res = vkAllocateCommandBuffers(renderSystem.device, &createCommandBuffer, &commandBuffer); res != VK_SUCCESS)
			throw std::runtime_error{ "Failed to create command buffer" };
	}


	void vkDirectContext::SetDebugName(const char* debugStr) noexcept
	{
	}


	void vkDirectContext::FlushBarriers() noexcept
	{
		auto& RS = RenderSystem();

		Vector<VkMemoryBarrier2, 16, uint8_t>		memoryBarriers	{ pendingBarriers.Allocator };
		Vector<VkBufferMemoryBarrier2, 16, uint8_t>	bufferBarriers	{ pendingBarriers.Allocator };
		Vector<VkImageMemoryBarrier2, 16, uint8_t>	imageBarriers	{ pendingBarriers.Allocator };

		for (const auto& barrier : pendingBarriers)
		{
			switch (barrier.type)
			{
			case BarrierType::Global:
			{
				VkMemoryBarrier2 memoryBarrier{
					.sType			= VkStructureType::VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
					.pNext			= nullptr,
					.srcStageMask	= SyncPointToVK(barrier.src),
					.srcAccessMask	= AccessToVK(barrier.accessBefore),
					.dstStageMask	= SyncPointToVK(barrier.dst),
					.dstAccessMask	= AccessToVK(barrier.accessAfter)
				};

				memoryBarriers.push_back(memoryBarrier);
			}	break;
			case BarrierType::Texture:
			{
				VkImageMemoryBarrier2 textureBarrier{
					.sType			= VkStructureType::VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext			= nullptr,
                    .srcStageMask	= SyncPointToVK(barrier.src),
                    .srcAccessMask	= AccessToVK(barrier.accessBefore),
                    .dstStageMask	= SyncPointToVK(barrier.dst),
                    .dstAccessMask	= AccessToVK(barrier.accessAfter),
                    .oldLayout		= (VkImageLayout)LayoutToVK(barrier.texture.layoutBefore),
                    .newLayout		= (VkImageLayout)LayoutToVK(barrier.texture.layoutAfter),
                    .srcQueueFamilyIndex	= RS.device.get_queue_index(vkb::QueueType::graphics).value(),
                    .dstQueueFamilyIndex	= RS.device.get_queue_index(vkb::QueueType::graphics).value(),
                    .image					= RS.GetDeviceResource(barrier.resource).As<VkImage_T>(),
                    .subresourceRange		= VkImageSubresourceRange {
						    .aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel	= 0,
							.levelCount		= 1,
							.baseArrayLayer	= 0,
							.layerCount		= 1,
                    }
				};

				imageBarriers.push_back(textureBarrier);
			}	break;
			case BarrierType::Buffer:
			{
				DebugBreak();
				FK_ASSERT(false);
				VkBufferMemoryBarrier2 bufferBarrier{};

				bufferBarriers.push_back(bufferBarrier);
			}	break;
			}
		}
		
		VkDependencyInfo dependencyInfo{
		    .sType						= VkStructureType::VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext						= nullptr,
			.dependencyFlags			= 0,
			.memoryBarrierCount			= (uint32_t)memoryBarriers.size(),
			.pMemoryBarriers			= memoryBarriers.data(),
			.bufferMemoryBarrierCount	= (uint32_t)bufferBarriers.size(),
		    .pBufferMemoryBarriers		= bufferBarriers.data(),
			.imageMemoryBarrierCount	= (uint32_t)imageBarriers.size(),
			.pImageMemoryBarriers		= imageBarriers.data(),
		};

		pendingBarriers.clear();

		vkCmdPipelineBarrier2(
			commandBuffer,
			&dependencyInfo);
	}

	void vkDirectContext::CreateAS(const AccelerationStructureDesc&, const TriMesh&)
    {}

	void vkDirectContext::BuildBLAS(struct IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace)
    {}

	void vkDirectContext::DiscardResource(ResourceHandle resource)
	{
		DebugBreak();
	}

	void vkDirectContext::AddAliasingBarrier(ResourceHandle before, ResourceHandle after)
	{
		DebugBreak();
	}

    void vkDirectContext::AddUAVBarrier(ResourceHandle Handle, uint32_t subresource, DeviceLayout layout, DeviceSyncPoint src, DeviceSyncPoint dst)
	{
		DebugBreak();
	}

	void vkDirectContext::AddPresentBarrier(ResourceHandle Handle, DeviceAccessState Before)
	{
		DebugBreak();
	}

	void vkDirectContext::AddStreamOutBarrier(SOResourceHandle, DeviceAccessState Before, DeviceAccessState State)
	{
		DebugBreak();
	}

	void vkDirectContext::AddCopyResourceBarrier(ResourceHandle Handle, DeviceAccessState Before, DeviceAccessState State)
	{
		DebugBreak();
	}

	void vkDirectContext::AddGlobalBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
	{
		DebugBreak();
	}

	void vkDirectContext::AddTextureBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceLayout, DeviceLayout, DeviceSyncPoint, DeviceSyncPoint, BarrierSubResourceRange range)
	{
		DebugBreak();
	}

	void vkDirectContext::AddBufferBarrier(ResourceHandle Handle, DeviceAccessState, DeviceAccessState, DeviceSyncPoint, DeviceSyncPoint)
	{
		DebugBreak();
	}

	void vkDirectContext::AddBarriers(std::span<const Barrier> barriers)
	{
		for (auto& b : barriers)
			pendingBarriers.push_back(b);
	}

	void vkDirectContext::ClearDepthBuffer(ResourceHandle Texture, float ClearDepth)
    {}

	void vkDirectContext::ClearRenderTarget(ResourceHandle texture, float4 rgba)
	{
		if (std::find(resourcesUsed.begin(), resourcesUsed.end(), texture) == resourcesUsed.end())
		{
			resourcesUsed.push_back(texture);
			auto flags = RenderSystem().resources.Get<ResourceFieldID::Flags>(texture);

			if ((flags & ResourceFlags::SwapChain) != 0)
			{
				VkSemaphore* semaphore = (VkSemaphore*)RenderSystem().resources.Get<ResourceFieldID::Extra>(texture);
				waits.push_back((VkSemaphore)semaphore[0]);
				signals.push_back((VkSemaphore)semaphore[1]);
			}
		}

		auto apiResource = RenderSystem().GetDeviceResource(texture);
		FlushBarriers();

		VkImageSubresourceRange subresource{
            .aspectMask		= VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,
	        .baseMipLevel	= 0,
	        .levelCount		= 1,
	        .baseArrayLayer	= 0,
	        .layerCount		= 1
		};

		vkCmdClearColorImage(
			commandBuffer,
			apiResource.As<VkImage_T>(),
			VK_IMAGE_LAYOUT_GENERAL,
			(VkClearColorValue*)&rgba, 1, &subresource);
	}

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

	void vkDirectContext::SetRootSignature(const IPipelineInterface*)
    {}

	void vkDirectContext::SetComputeRootSignature(RootSigHandle)
    {}

	void vkDirectContext::SetComputeRootSignature(const IPipelineInterface*)
    {}

	void vkDirectContext::SetPipelineState(const struct IPipelineState* const PSO)
    {}

	void vkDirectContext::SetComputePipelineState(const PSOHandle, iAllocator& temp)
    {}

	void vkDirectContext::SetGraphicsPipelineState(const PSOHandle psoHandle, iAllocator& temp)
	{
		auto pso = RenderSystem().GetPSO(psoHandle, temp);

		vkCmdBindPipeline(commandBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pso->GetDevicePipeState().As<VkPipeline_T>());
	}

	void vkDirectContext::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil, const size_t MIPMapOffset)
	{
		EndPass();

		for (auto& rt : RTs)
		{
			auto res = RenderSystem().resources.Get<ResourceFieldID::APIHandle>(rt);
			
			VkRenderingAttachmentInfo attachment{
			    .sType					= VkStructureType::VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
				.pNext					= nullptr,
				.imageView				= nullptr,
				.imageLayout			= VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
				.resolveMode			= VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE,
				.resolveImageView		= nullptr,
				.resolveImageLayout		= VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
				.loadOp					= VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp				= VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.clearValue				= VkClearValue{ .color{ .float32{ 0.0f, 0.0f, 0.0f, 0.0f } } }
			};

			pendingAttachments.push_back(attachment);
		}

		pendingTargetConfiguration = true;
	}

	void vkDirectContext::SetRenderTargets2(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV)
	{
		EndPass();
	}

	void vkDirectContext::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
	{
		auto& renderSystem = (vkRenderSystem&)vkRenderSystem::GetInstance();

		Vector<VkRenderingAttachmentInfo> attachments{ renderSystem.allocator };
		/*
	    {
			VkStructureType          sType;
			const void*				 pNext;
			VkImageView              imageView;
			VkImageLayout            imageLayout;
			VkResolveModeFlagBits    resolveMode;
			VkImageView              resolveImageView;
			VkImageLayout            resolveImageLayout;
			VkAttachmentLoadOp       loadOp;
			VkAttachmentStoreOp      storeOp;
			VkClearValue             clearValue;
		} VkRenderingAttachmentInfo;
        */

		
		//VkRenderingInfoKHR renderTargetInfo{
		//	VkStructureType                        sType;
		//	const void* pNext;
		//	VkRenderingFlagsKHR                    flags;
		//	VkRect2D                               renderArea;
		//	uint32_t                               layerCount;
		//	uint32_t                               viewMask;
		//	uint32_t                               colorAttachmentCount;
		//	const VkRenderingAttachmentInfoKHR* pColorAttachments;
		//	const VkRenderingAttachmentInfoKHR* pDepthAttachment;
		//	const VkRenderingAttachmentInfoKHR* pStencilAttachment;
		//};

		//const VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{
		//	.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		//	.colorAttachmentCount = 1,
		//	.pColorAttachmentFormats = &swapchain_image_format_,
		//};
	}

	void vkDirectContext::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{
	    
	}

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

	void vkDirectContext::Draw(const size_t vertexCount, const size_t baseVertex, const size_t baseIndex)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();

	    vkCmdDraw(commandBuffer, vertexCount, 1, baseVertex, 0);
		pendingDraws = true;
	}

	void vkDirectContext::DrawInstanced(const size_t vertexCount, const size_t baseVertex, const size_t instanceCount, size_t instanceOffset)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();

	    vkCmdDraw(commandBuffer, vertexCount, instanceCount, baseVertex, instanceOffset);
		pendingDraws = true;
	}

	void vkDirectContext::DrawIndexed(const size_t indexCount, const size_t indexOffet, const size_t baseVertex)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();

	    vkCmdDrawIndexed(commandBuffer, indexCount, 1, baseVertex, baseVertex, 0);
		pendingDraws = true;
	}

	void vkDirectContext::DrawIndexedInstanced(const size_t indexCount, const size_t indexOffet, const size_t baseVertex, const size_t instanceCount, const size_t instanceOffset)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();

	    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, baseVertex, baseVertex, instanceOffset);
		pendingDraws = true;
	}

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
	{
		EndPass();

	    VkCommandBufferSubmitInfo clInfo{
			.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	        .pNext			= nullptr,
	        .commandBuffer	= commandBuffer
		};

		if (auto res = vkEndCommandBuffer(commandBuffer); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to close command buffer!" };
	}

	void vkDirectContext::Begin(uint64_t submissionValue)
	{
		resourcesUsed.clear();
		waits.clear();
		signals.clear();
		dispatchValue = submissionValue;
		pendingDraws = false;

		VkCommandBufferBeginInfo beginInfo{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext				= nullptr,
			.flags				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo	= nullptr
		};
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	void vkDirectContext::Reset()
	{
		if (auto res = vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to reset commandbuffer!" };
	}

	void vkDirectContext::SetViewports(std::span<const Viewport> VPs)
    {}

	void vkDirectContext::SetScissorRects(std::span<const Rect>	rects)
	{}

	void vkDirectContext::EndPass()
	{
		if (!pendingDraws)
			return;

		vkCmdEndRendering(commandBuffer);

		pendingDraws = false;
		pendingAttachments.clear();
		depthBufferAttachment.reset();
		stencilBufferAttachment.reset();
	}

	void vkDirectContext::ApplyRenderTargetSetup()
	{
		if (!pendingTargetConfiguration)
			return;

		VkRenderingInfo renderingInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext					= nullptr,
			.flags					= 0,
			.renderArea				= {},
			.layerCount				= 0,
			.viewMask				= 0,
			.colorAttachmentCount	= (uint32_t)pendingAttachments.size(),
			.pColorAttachments		= pendingAttachments.data(),
			.pDepthAttachment		= depthBufferAttachment.and_then([](auto& val)		{ return std::optional{ &val }; }).value_or(nullptr),
			.pStencilAttachment		= stencilBufferAttachment.and_then([](auto& val)	{ return std::optional{ &val }; }).value_or(nullptr),
		};

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
	}

	vkRenderSystem& vkDirectContext::RenderSystem() noexcept
	{
		return static_cast<vkRenderSystem&>(vkRenderSystem::GetInstance());
	}

	UploadReservation vkDirectContext::ReserveDirectUploadSpace(size_t size, size_t alignment)
	{
		return {};
	}

	IRenderSystem& vkDirectContext::GetRenderSystem() noexcept
	{
		return IRenderSystem::GetInstance();
	}
}
