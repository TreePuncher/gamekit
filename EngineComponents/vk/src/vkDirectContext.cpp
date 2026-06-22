#include "vkDirectContext.hpp"
#include "vkDescriptorSet.hpp"
#include "vkRenderSystem.hpp"
#include <VkBootstrapDispatch.h>
#include <VkBootstrap.h>
#include <vkPipelineLayout.hpp>
#include <TriMeshResource.hpp>

#include "PushBuffers.hpp"

namespace VK_internal
{
	using namespace FlexKit;


	vkDirectContext::vkDirectContext()
	{
		auto& renderSystem = (vkRenderSystem&)vkRenderSystem::GetInstance();
		pendingBarriers		= Vector<Barrier>{ renderSystem.allocator };
		resourcesUsed		= Vector<ResourceHandle>{ renderSystem.allocator };
		waits				= Vector<SyncPoint>{ renderSystem.allocator };
		signals				= Vector<SyncPoint>{ renderSystem.allocator };
		pendingAttachments	= Vector<VkRenderingAttachmentInfo>{ renderSystem.allocator };
		viewports			= Vector<VkViewport>{ renderSystem.allocator };
		scissors			= Vector<VkRect2D>{ renderSystem.allocator };
		
		VkCommandPoolCreateInfo createPoolDesc{
				.sType				= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext				= nullptr,
				.flags				= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex	= 0
		};

		if (auto res = vkCreateCommandPool(renderSystem.device, &createPoolDesc, nullptr, &cmdPool); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to create command pool!"};

        VkCommandBufferAllocateInfo createCommandBuffer{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext					= nullptr,
			.commandPool			= cmdPool,
			.level					= VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount		= 1,
		};

		if (auto res = vkAllocateCommandBuffers(renderSystem.device, &createCommandBuffer, &cmdBuffer); res != VK_SUCCESS)
			throw std::runtime_error{ "Failed to create command buffer" };
	}


	void vkDirectContext::SetDebugName(const char* debugStr) noexcept
	{
	}


	void vkDirectContext::FlushBarriers() noexcept
	{
		if (pendingBarriers.size() == 0)
			return;

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
			cmdBuffer,
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
	    EndPass();

		for (auto& b : barriers)
			pendingBarriers.push_back(b);
	}


	void vkDirectContext::ClearDepthBuffer(ResourceHandle resource, float clearDepth, uint32_t stencil)
	{
		if (std::find(resourcesUsed.begin(), resourcesUsed.end(), resource) == resourcesUsed.end())
		{
			resourcesUsed.push_back(resource);
			auto flags = RenderSystem().resources.Get<ResourceFieldID::Flags>(resource);

			if ((flags & ResourceFlags::SwapChain) != 0)
			{
				VkSemaphore* semaphore = (VkSemaphore*)RenderSystem().resources.Get<ResourceFieldID::Extra>(resource);

				if(semaphore[0])
					waits.push_back(SyncPoint{ .syncCounter = -1u, .fence = semaphore[0] });

				if(semaphore[1])
				signals.push_back(SyncPoint{ .syncCounter = -1u, .fence = semaphore[1] });
			}
		}

		auto apiResource = RenderSystem().GetDeviceResource(resource);
		FlushBarriers();

		VkImageSubresourceRange subresource{
			.aspectMask		= VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel	= 0,
			.levelCount		= 1,
			.baseArrayLayer = 0,
			.layerCount		= 1
		};

		VkClearDepthStencilValue value{
			.depth		= clearDepth,
			.stencil	= 0
		};

		vkCmdClearDepthStencilImage(
			cmdBuffer,
			apiResource.As<VkImage_T>(),
			VK_IMAGE_LAYOUT_GENERAL,
			&value, 1, &subresource);
	}


	void vkDirectContext::ClearRenderTarget(ResourceHandle texture, float4 rgba)
	{
		if (std::find(resourcesUsed.begin(), resourcesUsed.end(), texture) == resourcesUsed.end())
		{
			resourcesUsed.push_back(texture);
			auto flags = RenderSystem().resources.Get<ResourceFieldID::Flags>(texture);

			if ((flags & ResourceFlags::SwapChain) != 0)
			{
				VkSemaphore* semaphore = (VkSemaphore*)RenderSystem().resources.Get<ResourceFieldID::Extra>(texture);

				if (semaphore && semaphore[0])
					waits.push_back(	SyncPoint{ .syncCounter = -1u, .fence = (VkSemaphore)semaphore[0] });

				if (semaphore && semaphore[1])
					signals.push_back(	SyncPoint{ .syncCounter = -1u, .fence = (VkSemaphore)semaphore[1] });
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
			cmdBuffer,
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
	{
		memset(computeDescriptorBuffer, 0x00, sizeof(computeDescriptorBuffer));
	}


	void vkDirectContext::SetGraphicsPipelineState(const PSOHandle psoHandle, iAllocator& temp)
	{
		auto pso = RenderSystem().GetPSO(psoHandle, temp);

		currentGraphicsLayout = static_cast<const vkPipelineInterface*>(pso->GetInterface());
		vkCmdBindPipeline(cmdBuffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pso->GetDevicePipeState().As<VkPipeline_T>());

		memset(graphicsDescriptorBuffer, 0x00, sizeof(graphicsDescriptorBuffer));
	}


	void vkDirectContext::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool DepthStecil, ResourceHandle DepthStencil, const size_t MIPMapOffset)
	{
		EndPass();
		auto& renderSystem = (vkRenderSystem&)vkRenderSystem::GetInstance();

		pendingAttachments.clear();

		for (auto& rt : RTs)
		{
			auto [clear, view] = RenderSystem().resources.Get<ResourceFieldID::Clear, ResourceFieldID::View>(rt);

			pendingAttachments.push_back(
					VkRenderingAttachmentInfo{
						.sType			= VkStructureType::VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
						.pNext			= nullptr,
						.imageView		= view.imageView,
						.imageLayout	= VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
						.resolveMode	= VkResolveModeFlagBits::VK_RESOLVE_MODE_NONE, 
						.loadOp			= VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_NONE,
						.storeOp		= VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
						.clearValue		= clear
					});
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

		scissors.clear();
		viewports.clear();
		pendingAttachments.clear();

		for (auto& target : RenderTargets)
		{
			bool isRenderTarget = (renderSystem.resources.Get<ResourceFieldID::Flags>(target) & ResourceFlags::RenderTarget);

			if (isRenderTarget)
			{
				auto XY = renderSystem.resources.Get<ResourceFieldID::XYZW>(target);

				renderArea.offset = { .x = 0, .y = 0 };
				renderArea.extent = { .width = XY[0], .height = XY[1] };

				VkViewport viewport{
					.x			= 0,
					.y			= (float)XY[1],
					.width		= (float)XY[0],
					.height		= -(float)XY[1],
					.minDepth	= 0.0f,
					.maxDepth	= 1.0f
				};

				VkRect2D rect{
					.offset = {.x = 0, .y = 0 },
	                .extent = {.width = XY[0], .height = XY[1] }
				};

				scissors.push_back(rect);
				viewports.push_back(viewport);
			}
		}
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
	{
		vkCmdPushConstants(
			cmdBuffer,
			currentGraphicsLayout->layout,
			currentGraphicsLayout->pushConstantFlags,
			(uint32_t)offset, valueCount * 4, data_ptr);
	}

	void vkDirectContext::NullGraphicsConstantBufferView(size_t idx)
    {}


	void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, const ConstantBufferHandle CB, size_t Offset)
	{
	    auto& renderSystem = RenderSystem();
		auto vkBufferObj = renderSystem.constantPushBuffers.GetAPIBuffer(CB);

		if (currentGraphicsLayout->pushLayout)
		{
			size_t offset;
			vkGetDescriptorSetLayoutBindingOffset(renderSystem.device, currentGraphicsLayout->pushLayout, idx, &offset);

			auto buffer = renderSystem.constantPushBuffers.GetAPIBuffer(CB);
			VkBufferDeviceAddressInfo getAddressInfo{
				.sType	= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext	= nullptr,
				.buffer	= buffer
			};

			const auto address = vkGetBufferDeviceAddress(renderSystem.device, &getAddressInfo);

			VkDescriptorAddressInfoEXT bufferInfo{
				.sType		= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext		= nullptr,
                .address	= address + Offset,
                .range		= 1024 * 4,
                .format		= VkFormat::VK_FORMAT_UNDEFINED
			};

			VkDescriptorGetInfoEXT getInfo{
	            .sType	= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
	            .pNext	= nullptr,
	            .type	= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.data {
				    .pUniformBuffer = &bufferInfo
				}
			};

			vkGetDescriptor(renderSystem.device, &getInfo, 8, graphicsDescriptorBuffer + offset);

			pendingGraphicsDescriptorBind = true;
		}
	}


	void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, const struct ConstantBufferDataSet& CB)
	{
		auto& renderSystem = RenderSystem();
		auto vkBufferObj = renderSystem.constantPushBuffers.GetAPIBuffer(CB.Handle());

		if (currentGraphicsLayout->pushLayout)
		{
			size_t offset;
			vkGetDescriptorSetLayoutBindingOffset(renderSystem.device, currentGraphicsLayout->pushLayout, idx, &offset);

			auto buffer = renderSystem.constantPushBuffers.GetAPIBuffer(CB.Handle());
			VkBufferDeviceAddressInfo getAddressInfo{
				.sType	= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext	= nullptr,
				.buffer	= buffer
			};

			const auto address = vkGetBufferDeviceAddress(renderSystem.device, &getAddressInfo);

			VkDescriptorAddressInfoEXT bufferInfo{
				.sType		= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext		= nullptr,
                .address	= address + CB.Offset(),
                .range		= CB.Size(),
                .format		= VkFormat::VK_FORMAT_UNDEFINED
			};

			VkDescriptorGetInfoEXT getInfo{
	            .sType	= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
	            .pNext	= nullptr,
	            .type	= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.data {
				    .pUniformBuffer = &bufferInfo
				}
			};

			vkGetDescriptor(renderSystem.device, &getInfo, 8, graphicsDescriptorBuffer + offset);

			pendingGraphicsDescriptorBind = true;
		}
	}


    void vkDirectContext::SetGraphicsConstantBufferView(size_t idx, DevicePointer)
    {}


	void vkDirectContext::SetGraphicsDescriptorSet(size_t idx, const struct DescriptorSet& DH)
	{
		const auto& implDH = static_cast<const vkDescriptorSet&>(DH.GetImpl());
		uint32_t indices[]	 = { 0 };
		uint64_t offsets[16] = { implDH.bufferOffset };

		vkCmdSetDescriptorBufferOffsets(
			cmdBuffer,
			VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
			currentGraphicsLayout->layout,
			idx, 1,
			indices,
			offsets);
	}


	void vkDirectContext::SetGraphicsDescriptorSet(size_t idx, const DescriptorRange& range)
    {}


	void vkDirectContext::SetGraphicsShaderResourceView(size_t idx, ResourceHandle resource, size_t offset)
    {}


	void vkDirectContext::SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle resource, size_t offset)
    {}


	void vkDirectContext::SetComputeDescriptorSet(size_t idx)
    {}


	void vkDirectContext::SetComputeDescriptorSet(size_t idx, const struct DescriptorSet& DH)
    {}


	void vkDirectContext::SetComputeDescriptorSet(size_t idx, const DescriptorRange& range)
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


	void vkDirectContext::AddIndexBuffer(TriMesh* mesh, uint32_t lod)
    {
		auto buffer = mesh->lods[lod].bufferSet->GetIndexBuffer();

		vkCmdBindIndexBuffer(
			cmdBuffer,
			buffer.resource.As_ptr<VkBuffer>(),
			0,
			VkIndexType::VK_INDEX_TYPE_UINT32);
	}


	void vkDirectContext::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
    {
		const vkVertexPushBuffers::APIObjects& apiOjbects =
			RenderSystem().vertexPushBuffers.fields.Get<vkVertexPushBuffers::PushBufferFields::apiObjects>(buffer.VertexBuffer);

		vkCmdBindIndexBuffer(
			cmdBuffer,
			apiOjbects.buffers[apiOjbects.current],
			0,
			VkIndexType::VK_INDEX_TYPE_UINT32);
	}


	void vkDirectContext::SetIndexBuffer(ResourceHandle handle, DeviceFormat format)
    {
		auto& vkRS		= RenderSystem();
		auto resource	= vkRS.resources.Get<ResourceFieldID::APIHandle>(handle);

		FK_ASSERT(resource.type == vkResourceEntry::Type::Buffer);

		vkCmdBindIndexBuffer(
			cmdBuffer,
			resource.buffer,
			0,
			VkIndexType::VK_INDEX_TYPE_UINT32);
	}


	void vkDirectContext::AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* InstanceBuffers)
    {
		static_vector<VkBuffer, 16> apiBuffers;
		static_vector<VkDeviceSize, 16> offsets;

		for (auto& buff : buffers)
		{
			auto res = Mesh->lods[lod].bufferSet->Find(buff);
			apiBuffers.push_back(res.value().resource.As_ptr<VkBuffer>());
			offsets.push_back(0);
		}

		vkCmdBindVertexBuffers(
			cmdBuffer,
			0,
			buffers.size(),
			apiBuffers.data(),
			offsets.data());
	}


	void vkDirectContext::AddVertexBuffers(TriMesh* Mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* InstanceBuffers)
    {
		static_vector<VkBuffer, 16> apiBuffers;

		for (auto& buff : buffers)
		{
			auto res = Mesh->lods[lod].bufferSet->Find(buff);
			apiBuffers.push_back(res.value().resource.As_ptr<VkBuffer>());
		}

		vkCmdBindVertexBuffers(
			cmdBuffer,
			0,
			buffers.size(),
			apiBuffers.data(),
			nullptr);
	}


	void vkDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferEntry>& list)
	{
		SetVertexBuffers(std::span{ list });
	}
    

	void vkDirectContext::SetVertexBuffers(const std::span<const VertexBufferEntry> span)
	{
	    VkBuffer		buffers[16];
		VkDeviceSize	offsets[16];


		auto& vkRS = RenderSystem();
		for (auto&& [idx, entry] : enumerate(span))
		{
			auto& [handle, stride, offset] = entry;
			auto& [bufs, memory, locks, memoryOffset, current] = vkRS.vertexPushBuffers.fields.Get_ref<vkVertexPushBuffers::apiObjects>(handle);

			buffers[idx] = bufs[current];
			offsets[idx] = offset;
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, span.size(), buffers, offsets);
	}


	void vkDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferResource>& list)
    {
		SetVertexBuffers(std::span{ list });
	}


	void vkDirectContext::SetVertexBuffers(const std::span<const VertexBufferResource> span)
    {
		VkBuffer		buffers[16];
		VkDeviceSize	offsets[16];

		auto& vkRS = RenderSystem();
		for (auto&& [idx, entry] : enumerate(span))
		{
			auto& [handle, stride, offset] = entry;
			auto resource = vkRS.resources.Get<ResourceFieldID::APIHandle>(handle);

			FK_ASSERT(resource.type == vkResourceEntry::Type::Buffer);

			buffers[idx] = resource.buffer;
			offsets[idx] = offset;
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, span.size(), buffers, offsets);
	}


	void vkDirectContext::SetVertexBuffers2(const std::span<const VBView> views, uint32_t offset)
    {
		VkBuffer		buffers[16];
		VkDeviceSize	offsets[16];

		auto& vkRS = RenderSystem();
		for (auto&& [idx, entry] : enumerate(views))
		{
			auto& [buffer, stride, offset] = entry;

			buffers[idx] = (VkBuffer)buffer;
			offsets[idx] = offset;
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, views.size(), buffers, offsets);
	}


	void vkDirectContext::Draw(const size_t vertexCount, const size_t baseVertex, const size_t baseIndex)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();
		ApplyPendingRasterizingStates();
		ApplyGraphicsDescriptorSetBindings();

	    vkCmdDraw(cmdBuffer, vertexCount, 1, baseVertex, 0);
		pendingDraws = true;
	}


	void vkDirectContext::DrawInstanced(const size_t vertexCount, const size_t baseVertex, const size_t instanceCount, size_t instanceOffset)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();
		ApplyPendingRasterizingStates();
		ApplyGraphicsDescriptorSetBindings();

	    vkCmdDraw(cmdBuffer, vertexCount, instanceCount, baseVertex, instanceOffset);
		pendingDraws = true;
	}

	void vkDirectContext::DrawIndexed(const size_t indexCount, const size_t indexOffet, const size_t baseVertex)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();
		ApplyPendingRasterizingStates();
		ApplyGraphicsDescriptorSetBindings();

	    vkCmdDrawIndexed(cmdBuffer, indexCount, 1, baseVertex, baseVertex, 0);
		pendingDraws = true;
	}

	void vkDirectContext::DrawIndexedInstanced(const size_t indexCount, const size_t indexOffet, const size_t baseVertex, const size_t instanceCount, const size_t instanceOffset)
	{
		FlushBarriers();
		ApplyRenderTargetSetup();
		ApplyPendingRasterizingStates();
		ApplyGraphicsDescriptorSetBindings();

	    vkCmdDrawIndexed(cmdBuffer, indexCount, instanceCount, baseVertex, baseVertex, instanceOffset);
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
		FlushBarriers();

	    VkCommandBufferSubmitInfo clInfo{
			.sType			= VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	        .pNext			= nullptr,
	        .commandBuffer	= cmdBuffer
		};

		if (auto res = vkEndCommandBuffer(cmdBuffer); res != VK_SUCCESS)
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
		vkBeginCommandBuffer(cmdBuffer, &beginInfo);


		VkBufferDeviceAddressInfo getAddressInfo{
				.sType	= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
				.pNext	= nullptr,
				.buffer = RenderSystem().descriptorPool.buffer,
			};

	    auto address = vkGetBufferDeviceAddress(RenderSystem().device, &getAddressInfo);

		VkDescriptorBufferBindingInfoEXT bindingInfo{
				.sType		= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
				.pNext		= nullptr,
				.address	= address,
				.usage		= VK_BUFFER_USAGE_2_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
		};

		vkCmdBindDescriptorBuffers(cmdBuffer, 1, &bindingInfo);
	}


	void vkDirectContext::Reset()
	{
		if (auto res = vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT); res != VK_SUCCESS)
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

		vkCmdEndRendering(cmdBuffer);

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
			.renderArea				= renderArea,
			.layerCount				= 1,
			.viewMask				= 0,
			.colorAttachmentCount	= (uint32_t)pendingAttachments.size(),
			.pColorAttachments		= pendingAttachments.data(),
			.pDepthAttachment		= depthBufferAttachment.and_then([](auto& val)		{ return std::optional{ &val }; }).value_or(nullptr),
			.pStencilAttachment		= stencilBufferAttachment.and_then([](auto& val)	{ return std::optional{ &val }; }).value_or(nullptr),
		};


		vkCmdSetScissorWithCount(cmdBuffer, scissors.size(), scissors.data());
		vkCmdSetViewportWithCount(cmdBuffer, viewports.size(), viewports.data());

		vkCmdBeginRendering(cmdBuffer, &renderingInfo);

		pendingTargetConfiguration = false;
	}


	void vkDirectContext::ApplyPendingRasterizingStates()
	{
		/*
		VkWriteDescriptorSet writeDescriptors{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = ;
			uint32_t                         dstBinding;
			uint32_t                         dstArrayElement;
			uint32_t                         descriptorCount;
			VkDescriptorType                 descriptorType;
			const VkDescriptorImageInfo* pImageInfo;
			const VkDescriptorBufferInfo* pBufferInfo;
			const VkBufferView* pTexelBufferView;
		};

		vkCmdPushDescriptorSet(
			commandBuffer,
            VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS,
			currentGraphicsLayout->layout,
            0, 1, &writeDescriptors
		);
		*/
	}


	void vkDirectContext::ApplyGraphicsDescriptorSetBindings()
	{
		if (pendingGraphicsDescriptorBind && currentGraphicsLayout->pushLayout)
		{
			auto& vkRS = RenderSystem();

			size_t size;
			vkGetDescriptorSetLayoutSize(vkRS.device, currentGraphicsLayout->pushLayout, &size);

			auto allocation =
				vkRS.heapAllocator.Alloc2Temp(
					size,
					vkRS.GetCurrentProgress(),
					dispatchValue,
					vkRS.descriptorBufferProperties.descriptorBufferOffsetAlignment);

			if (!allocation)
			{
				FK_LOG_ERROR("VK: allocation failed : Failed to bind inline descriptor set!");
				return;
			}
			
			auto& [range, offset] = allocation.value();

			uint32_t indices[]		= { 0 };
			uint64_t offsets[16]	= { offset };

			memcpy((void*)range.begin.V1.to_uint(), graphicsDescriptorBuffer, size);

			vkCmdSetDescriptorBufferOffsets(
				cmdBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				currentGraphicsLayout->layout,
				currentGraphicsLayout->pushSet,
				1,
				indices, offsets);

			pendingGraphicsDescriptorBind = false;
		}
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
