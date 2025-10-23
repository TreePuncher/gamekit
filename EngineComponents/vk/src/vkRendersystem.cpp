#include "vkCopyContext.hpp"
#include "vkDirectContext.hpp"
#include "vkRenderSystem.hpp"
#include <Handle.hpp>
#include <RenderSystemInterface.hpp>

#include "vkPipelineBuilder.hpp"
#include "vkDescriptorHeap.hpp"

#include <vulkan/vulkan.hpp>

#include <filesystem>
#include <fmt/format.h>
#include <print>

#include "../../../core/include/BuildSettings.hpp"

#ifdef WIN32
#include "Unknwnbase.h"
#endif

#include <directx-dxc/dxcapi.h>
#include <iostream>

namespace VK_internal
{
	using namespace FlexKit;

	vkGetDescriptorSetLayoutSizeFNDef				vkGetDescriptorSetLayoutSize	= nullptr;
	vkGetDescriptorFNDef							vkGetDescriptor = nullptr;
	vkCmdBindDescriptorBufferEmbeddedSamplersFNDef	vkCmdBindDescriptorBufferEmbeddedSamplers = nullptr;

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
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		    VK_DESCRIPTOR_TYPE_MUTABLE_EXT
		};

		VkMutableDescriptorTypeListEXT availableTypesList[] {
            {
				.descriptorTypeCount	= 7,
				.pDescriptorTypes		= typesAvailable
            },
            {
				.descriptorTypeCount	= 7,
				.pDescriptorTypes		= typesAvailable
			},
		};

		VkMutableDescriptorTypeCreateInfoEXT ext0{
	        .sType							= VkStructureType::VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
		    .pNext							= nullptr,
	        .mutableDescriptorTypeListCount = 2,
	        .pMutableDescriptorTypeLists	= availableTypesList
		};


		VkDescriptorPoolSize sizes[] =
		{
			{ VkDescriptorType::VK_DESCRIPTOR_TYPE_MUTABLE_EXT, 10000 },
		};

		VkDescriptorPoolCreateInfo descriptorPoolCreateDesc{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext				= &ext0,
			.flags				= VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_EXT,
            .maxSets			= 10000,
            .poolSizeCount		= 1,
            .pPoolSizes			= sizes
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


	vkDescriptorHeap CreateDescriptorBuffer(VkDevice device, size_t bufferSize, const vkAllocation& allocation)
	{
	    // Create Buffer
		VkBufferCreateInfo createBufferInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,			//VkBufferCreateFlags;
			.size					= bufferSize,	//VkDeviceSize
			.usage					= VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		//const uint32_t*        
		};

		VkBuffer buffer;
		vkCreateBuffer(device, &createBufferInfo, nullptr, &buffer);
		vkBindBufferMemory(device, buffer, allocation.memory, allocation.offset);

		vkDescriptorHeap heap{
			.allocation	= allocation,
			.buffer		= buffer,
		};

		return heap;
	}


	std::optional<UploadBufferResults> CreateUploadBuffer(vkRenderSystem& renderSystem, size_t bufferSize)
	{
		auto allocationRes = renderSystem.memoryAllocator.Allocate(
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			bufferSize
		);

		if (!allocationRes.has_value())
			return {};

		auto&& [offset, memory] = allocationRes.value();

	     // Create Buffer
		VkBufferCreateInfo createBufferInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,			//VkBufferCreateFlags;
			.size					= bufferSize,	//VkDeviceSize
			.usage					= VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		//const uint32_t*        
		};

		VkBuffer buffer;
		vkCreateBuffer(renderSystem.device, &createBufferInfo, nullptr, &buffer);
		vkBindBufferMemory(renderSystem.device, buffer, memory, offset);

		return UploadBufferResults
		{
			.buffer = buffer,
			.memory = memory
		};
	}


	VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const FlexKit::DesciptorHeapLayout& layout, iAllocator& allocator)
	{
		Vector<VkDescriptorSetLayoutBinding>	bindings		{ allocator };

		for (auto& entry : layout.entries)
		{
			VkDescriptorType type;
			switch (entry.type)
			{
			case DescHeapEntryType::ConstantBuffer:
				type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case DescHeapEntryType::ShaderResourceBuffer:
				type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				break;
			case DescHeapEntryType::ShaderResourceImage:
				type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				break;
			case DescHeapEntryType::UAVBuffer:
				type = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
			case DescHeapEntryType::UAVImage:
				type = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				break;
			}

			VkDescriptorSetLayoutBinding binding;
			binding = VkDescriptorSetLayoutBinding{
						.binding			= entry.registerIdx,
						.descriptorType		= type,
						.descriptorCount	= entry.count,
						.stageFlags			= VK_SHADER_STAGE_ALL,
						.pImmutableSamplers = nullptr };

			bindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo createLayoutDesc{
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext			= nullptr,
			.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			.bindingCount	= (uint32_t)bindings.size(),
			.pBindings		= bindings.data()
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

	void CreateImageSRV(VkDevice device, VkDescriptorSet descriptorSet, VkImage buffer, const DescriptorLocation& viewLocation = {})
	{
		VkDescriptorImageInfo imageInfo{
	        .sampler		= nullptr, 
            .imageView		= nullptr,
            .imageLayout	= VK_IMAGE_LAYOUT_GENERAL
		};

		VkWriteDescriptorSet write{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext				= nullptr,
            .dstSet				= descriptorSet,
            .dstBinding			= viewLocation.idx,
            .dstArrayElement	= viewLocation.arrayIdx,
            .descriptorCount	= 1,
            .descriptorType		= VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo			= &imageInfo
		};
		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	vkRenderSystem::vkRenderSystem(ThreadManager* threads, iAllocator& IN_allocator) :
	    allocator				{ IN_allocator },
		memoryAllocator			{ IN_allocator },
		pendingDirectContexts	{ IN_allocator },
		pipelineStates			{ threads, IN_allocator },
	    resources				{ IN_allocator },
        vkAllocators			{ nullptr }{}


	bool vkRenderSystem::Initiate(Graphics_Desc& desc)
	{
		allocator = desc.Memory;
		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_DISPLAY_EXTENSION_NAME,
#ifdef WIN32
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
#ifdef __linux__
			//VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
			//VK_KHR_XCB_SURFACE_EXTENSION_NAME,
			//VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
		};

	    vkb::InstanceBuilder builder;
		auto instReq = builder.set_app_name("Hello Vulkan")
		    .require_api_version(1, 4, 304)
			.request_validation_layers()
			.set_headless()
		    .enable_extensions(std::size(extensions), extensions)
			.use_default_debug_messenger()
			.build();

		if (!instReq)
			return false;


		instance = instReq.value();
		vkb::PhysicalDeviceSelector selector{ instance };

		VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeature{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
	        .pNext = nullptr, 
	        .descriptorBuffer					= true,
	        .descriptorBufferCaptureReplay		= false,
	        .descriptorBufferImageLayoutIgnored	= true,
	        .descriptorBufferPushDescriptors	= true,
		};

		VkPhysicalDeviceBufferDeviceAddressFeaturesEXT deviceAddressFeatureInfo{
			.sType								= VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_ADDRESS_FEATURES_EXT,
		    .pNext								= &descriptorBufferFeature,
            .bufferDeviceAddress				= true,
            .bufferDeviceAddressCaptureReplay	= false,
            .bufferDeviceAddressMultiDevice		= false
		};

		auto physRequest = selector
	        .set_minimum_version(1, 4)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			//.add_required_extension("VK_NV_descriptor_pool_overallocation")
			.add_required_extension("VK_KHR_depth_stencil_resolve")
		    .add_required_extension("VK_KHR_dynamic_rendering")
			.add_required_extension("VK_KHR_maintenance3")
            .add_required_extension("VK_KHR_swapchain")
			.add_required_extension("VK_KHR_timeline_semaphore")
			.add_required_extension("VK_KHR_spirv_1_4")
			.add_required_extension("VK_EXT_descriptor_buffer")
		    //.add_required_extension("VK_EXT_present_mode_fifo_latest_ready")
			.set_required_features({
                    .fullDrawIndexUint32	= true,
				    .imageCubeArray			= true,
                    .independentBlend		= true,
                    .geometryShader			= true, 
                    .tessellationShader		= true,
                    .sampleRateShading		= true,
                    .dualSrcBlend			= true,		
			        .depthClamp				= true,
					.depthBiasClamp			= true,
                    .fillModeNonSolid		= true,
                    .depthBounds			= true,
				})
            .set_required_features_12({
					.sType				= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
                    .timelineSemaphore	= true
            })
	        .set_required_features_13({
                    .sType				= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			        .pNext				= &deviceAddressFeatureInfo,
				    .synchronization2	= true,
                    .dynamicRendering	= true,
	        })
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

		vkGetDescriptorSetLayoutSize				= (vkGetDescriptorSetLayoutSizeFNDef)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
		vkGetDescriptor								= (vkGetDescriptorFNDef)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
		vkCmdBindDescriptorBufferEmbeddedSamplers	= (vkCmdBindDescriptorBufferEmbeddedSamplersFNDef)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT");

		

		FK_ASSERT(vkGetDescriptorSetLayoutSize != nullptr, "VK: Failed to get vkGetDescriptorSetLayoutSizeEXT");
		FK_ASSERT(vkGetDescriptor != nullptr, "VK: Failed to get vkGetDescriptorEXT");

		auto queue = queueRequest.value();

		memoryAllocator.Init(*this);

		auto descriptorHeapBufferAllocation = memoryAllocator.Allocate(0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			1000000u * 64);

		if (!descriptorHeapBufferAllocation)
			throw std::runtime_error{ "VK: Failed to allocate descriptor heap buffer!" };

		descriptorPool = CreateDescriptorBuffer(device, 1000000u, descriptorHeapBufferAllocation.value());

		VkBufferDeviceAddressInfoKHR address_info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR };
		address_info.buffer = descriptorPool.buffer;
		auto gpuAddress = vkGetBufferDeviceAddress(device, &address_info);

		VkMemoryMapInfo memoryMapInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_MAP_INFO,
			.pNext = nullptr,
			.flags = 0,
			.memory = descriptorHeapBufferAllocation.value().memory,
			.offset	= 0,
			.size = VK_WHOLE_SIZE
		};

		uint64_t cpuAddress;
		vkMapMemory2(device, &memoryMapInfo, (void**)&cpuAddress);

		HeapAllocatorDescription heapAllocDesc{
			.CPUBegin	= cpuAddress,
			.GPUBegin	= gpuAddress,
			.size		= 1000000u,
			.device		= device, 
			.buffer		= descriptorPool.buffer
		};
		heapAllocator.Initialize(heapAllocDesc, allocator);

		// Create Heap Layout
		DesciptorHeapLayout layout{};
		layout.SetParameterAsSRVImage(0, 0, 1);

		auto vkLayout = CreateDescriptorSetLayout(device, layout, *allocator);

		VkDeviceSize size;
		vkGetDescriptorSetLayoutSize(device, vkLayout, &size);

		//auto descriptorSet = AllocateDescriptorSet(device, vkLayout, descriptorPool);

		//auto constantBuffer = VK_internal::CreateConstantBuffer(device, 1024u);
		//CreateCBV(device, descriptorSet, constantBuffer);

		VkFenceCreateInfo createFenceInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		if (auto res = vkCreateFence(device, &createFenceInfo, nullptr, &directQueueFence); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create fence for direct queue!");

		VkSemaphoreTypeCreateInfo semaphoreType{
		    .sType			= VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
	        .pNext			= 0,
	        .semaphoreType	= VK_SEMAPHORE_TYPE_TIMELINE,
	        .initialValue	= 0u
		};

		VkSemaphoreCreateInfo createTimelineSemaphoreInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreType,
            .flags = 0
		};

		if (auto res = vkCreateSemaphore(device, &createTimelineSemaphoreInfo, nullptr, &vkDirectQueueCounter); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create timeline semaphore queue!");

		return true;
	}


	void vkRenderSystem::BuildLibrary(PSOHandle State, const PipelineStateLibraryDesc)
	{
	}


	void vkRenderSystem::RegisterPSOLoader(PSOHandle state, LOADSTATE_FN FN)
	{
		pipelineStates.RegisterLoader(state, FN);
	}


	void vkRenderSystem::LoadPSOIfRequired(PSOHandle State)
	{
	}


	void vkRenderSystem::QueuePSOLoad(PSOHandle state)
	{
		pipelineStates.QueueLoad(state);
	}


	const IPipelineState* vkRenderSystem::GetPSO(PSOHandle State, iAllocator& temp)
	{
		return pipelineStates.GetPSO(State, temp);
	}


	const IPipelineInterface* const vkRenderSystem::GetPSORootSignature(PSOHandle state) const
	{
		return nullptr;
	}


	std::tuple<IPipelineState*, const IPipelineInterface*> vkRenderSystem::GetPSOAndRootSignature(PSOHandle stateID, iAllocator& temp) const
	{
		return {};
	}


	uint64_t vkRenderSystem::GetCurrentProgress() const
	{
		uint64_t currentProgress;
		vkGetSemaphoreCounterValue(device, vkDirectQueueCounter, &currentProgress);
		return currentProgress;
	}


	size_t vkRenderSystem::GetCurrentCounter()
	{
		return directSubmissionCounter;
	}


	SyncPoint vkRenderSystem::GetSubmissionTicket(uint32_t count)
	{
		auto value = directSubmissionCounter.fetch_add(count) + count;

		return SyncPoint{
		    .syncCounter	= value,
		    .fence			= directQueueFence };
	}


	void vkRenderSystem::SyncUploadTo(SyncPoint)
	{
		DebugBreak();
	}


	SyncPoint vkRenderSystem::SyncUploadPoint()
	{
		DebugBreak();

		return {};
	}


	SyncPoint vkRenderSystem::SyncUploadTicket()
	{
		DebugBreak();

		return {};
	}


	void vkRenderSystem::SyncDirectTo(SyncPoint)
	{
		DebugBreak();
	}


	SyncPoint vkRenderSystem::SyncDirectPoint()
	{
		DebugBreak();
		return {};
	}


	SyncPoint vkRenderSystem::SyncSubmittedDirectPoint()
	{
		DebugBreak();
		return {};
	}


	SyncPoint vkRenderSystem::SyncDirectTicket()
	{
		DebugBreak();
		return {};
	}


	void vkRenderSystem::SignalDirect(uint64_t)
	{
		DebugBreak();
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
		uint64_t current;
		vkGetSemaphoreCounterValue(device, vkDirectQueueCounter, &current);

		vkDirectContext* ctx = nullptr;
		for (auto& pendingCtx : pendingDirectContexts)
		{
		    if (pendingCtx->dispatchValue < current)
		    {
				ctx = pendingCtx;
				pendingCtx->Reset();
				pendingDirectContexts.remove_unstable(&ctx);
				break;
		    }
		}

		if (ctx == nullptr)
		    ctx = &allocator->allocate<vkDirectContext>();

		ctx->Begin(ticket.has_value() ? ticket.value().syncCounter : 0);

		return *ctx;
	}


	ICopyContext& vkRenderSystem::GetCopyContext(CopyContextHandle handle)
	{
		static vkCopyContext ctx;
		return ctx;
	}


	SyncPoint vkRenderSystem::Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync)
	{
		uint64_t submissionValue = 0;

		Vector<VkCommandBufferSubmitInfo, 8>	cmdBufferSubmit{ allocator };
		Vector<VkSemaphore, 8>					waits{ allocator };
		Vector<VkSemaphore, 8>					signals{ allocator };

		for (auto& cl : CLs)
		{
			auto vkCL = static_cast<vkDirectContext*>(cl);
			vkCL->Close();

			VkCommandBufferSubmitInfo info{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.pNext			= nullptr,
				.commandBuffer	= vkCL->commandBuffer,
				.deviceMask		= 0
			};
			cmdBufferSubmit.push_back(info);
			submissionValue = Max(submissionValue, vkCL->dispatchValue);

			for (auto sp : vkCL->waits)
				waits.push_back(sp);

			for (auto sp : vkCL->signals)
				signals.push_back(sp);
		}

		auto waitsEnd = std::unique(waits.begin(), waits.end());
		auto signalsEnd = std::unique(signals.begin(), signals.end());


		Vector<VkSemaphoreSubmitInfo, 8> waitInfos{ allocator };
		for (auto& syncObject : std::span(waits.begin(), waitsEnd))
		{
			VkSemaphoreSubmitInfo signalInfo{
			    .sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
	            .pNext		= nullptr,
	            .semaphore	= syncObject,
		        .stageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR
		    };

			waitInfos.push_back(signalInfo);
		}

		Vector<VkSemaphoreSubmitInfo, 8> signalInfos{ allocator };
		for (auto& syncObject : std::span(signals.begin(), signalsEnd))
		{
			VkSemaphoreSubmitInfo signalInfo{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext = nullptr,
				.semaphore = syncObject,
				.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR
			};

			signalInfos.push_back(signalInfo);
		}

		const VkSubmitInfo2 submit{
			.sType						= VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext						= nullptr,
			.flags						= 0x0,
            .waitSemaphoreInfoCount		= (uint32_t)waitInfos.size(), 
            .pWaitSemaphoreInfos		= waitInfos.data(),
            .commandBufferInfoCount		= (uint32_t)cmdBufferSubmit.size(),
            .pCommandBufferInfos		= cmdBufferSubmit.data(),
            .signalSemaphoreInfoCount	= (uint32_t)signalInfos.size(),
            .pSignalSemaphoreInfos		= signalInfos.data()
		};

		vkResetFences(device, 1, &directQueueFence);
		if (auto res = vkQueueSubmit2(device.get_queue(vkb::QueueType::graphics).value(), 1, &submit, directQueueFence); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to submit to Direct Command Queue!" };

		for (auto cl : CLs)
			pendingDirectContexts.push_back(static_cast<vkDirectContext*>(cl));

		return {
			submissionValue,
			directQueueFence
		};
	}


	void vkRenderSystem::EndFrame()
	{
		int x = 0;
	}


	void vkRenderSystem::Signal(SyncPoint)
	{
		int x = 0;
	}


	void vkRenderSystem::WaitForGPU()
	{
		int x = 0;
	}


	void vkRenderSystem::WaitFor(const uint64_t)
	{
		int x = 0;
	}


	void vkRenderSystem::WaitFor(const SyncPoint&)
	{
		int x = 0;
	}


	void vkRenderSystem::SetDebugName(ResourceHandle, const char*)
	{

	}


	void vkRenderSystem::SetDebugName(DeviceHeapHandle, const char*)
	{

	}


	void vkRenderSystem::SetObjectLayout(SOResourceHandle handle, DeviceLayout state) noexcept
	{

	}


	void vkRenderSystem::SetObjectLayout(ResourceHandle	handle, DeviceLayout state) noexcept
	{
		resources.Set<ResourceFieldID::Layout>(handle, state);
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


	DeviceAddressRange vkRenderSystem::GetDeviceRange(const ResourceHandle handle) const noexcept
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
		DeviceLayout layout = resources.Get<ResourceFieldID::Layout>(handle);

		return layout;
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


	size_t vkRenderSystem::GetTextureElementSize(ResourceHandle handle) const
	{
		auto format = resources.Get<ResourceFieldID::Format>(handle);

		auto vkFormat = FormatToVK(format);
		GetFormatElementSize(vkFormat);

		return 0;
	}


	uint2 vkRenderSystem::GetTextureWH(ResourceHandle handle) const
	{
		uint4 xyzw = resources.Get<ResourceFieldID::XYZW>(handle);

		return xyzw.Slice<0, 2>();
	}


	DeviceFormat vkRenderSystem::GetTextureFormat(ResourceHandle handle) const
	{
		auto format = resources.Get<ResourceFieldID::Format>(handle);

		return format;
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
		auto res = resources.Get<APIHandle>(handle);
		return res._ptr;
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


	struct IncludeHandler : public IDxcIncludeHandler
	{
		HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
		{
			char fileStr[256];
			auto fileLength = wcstombs(fileStr, pFilename, 256);

			std::filesystem::path file{ fileStr };
			auto newFilePath = includePath.string() + R"(\)" + file.string();

			wchar_t fileW[256];
			mbstowcs(fileW, newFilePath.c_str(), 256);


			return handler->LoadSource(fileW, ppIncludeSource);
		}

		HRESULT IUnknown::QueryInterface(const IID&, void**)
		{
			return 0;
		}

		std::filesystem::path   includePath;
		IDxcIncludeHandler*     handler;

		ULONG AddRef() { return 0; }
		ULONG Release() { return 0; }
	};


	Shader vkRenderSystem::LoadShader(const char* entryPoint, const char* profile, const char* file, const ShaderOptions& options)
	{
		IDxcLibrary*		hlslLibrary = nullptr;
		IDxcIncludeHandler* hlslIncludeHandler = nullptr;
		IDxcCompiler3*		hlslCompiler = nullptr;

		if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslLibrary))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Library!" });

		if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Compiler!" });

		if (hlslLibrary) hlslLibrary->CreateIncludeHandler(&hlslIncludeHandler);

		EXITSCOPE({
			if (hlslLibrary) hlslLibrary->Release();
		    if (hlslIncludeHandler) hlslIncludeHandler->Release();
		    if (hlslCompiler) hlslCompiler->Release();
		});


        std::filesystem::path filePath{ file };
		auto parentPath = filePath.parent_path();

		wchar_t entryPointW[64];
		wchar_t fileW[256];
		wchar_t filenameW[256];
		wchar_t profileW[64];

		size_t fileWLength = 0;
		if (entryPoint != nullptr)
			mbstowcs(entryPointW, entryPoint, 64);

		mbstowcs(profileW, profile, 64);
		mbstowcs(fileW, file, 256);
		mbstowcs(filenameW, filePath.filename().string().c_str(), 256);


		IDxcBlobEncoding* blob;
		auto HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);

		EXITSCOPE({ blob->Release(); });

		if (FAILED(HR1))
		{
			LPSTR string = nullptr;

			const auto msgLen = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr,
				HR1,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&string,
				0,
				nullptr);

			auto converted = fmt::format("Shader failed to load: {}", string);

			FK_LOG_ERROR(converted.c_str());

			LocalFree(string);

			return {};
		}

		IncludeHandler includeHandler;
		includeHandler.includePath      = parentPath;
		includeHandler.handler          = hlslIncludeHandler;


		IDxcCompiler2* debugCompiler = nullptr;
		hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

		static_vector<LPCWSTR> arguments;

#if USING(DEBUGSHADERS)
		arguments.push_back(L"-Od");
		arguments.push_back(L"/Zi");
		arguments.push_back(L"-Qembed_debug");
		arguments.push_back(L"-T");
		arguments.push_back(profileW);
		arguments.push_back(L"-spirv");
		arguments.push_back(L"-E");
		arguments.push_back(entryPointW);
#else
		arguments.push_back(L"-O3");
#endif

		if(options.enable16BitTypes)
			arguments.push_back(L"-enable-16bit-types");

		if (options.hlsl2021)
			arguments.push_back(L"-HV 2021");

		IDxcOperationResult* result = nullptr;

		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;
		blob->GetEncoding(&_, &buffer.Encoding);

		HRESULT HR2;
		try
		{
			HR2 = hlslCompiler->Compile(
				&buffer,
				arguments.data(),
				arguments.size(),
                &includeHandler,
				IID_PPV_ARGS(&result));
		}
		catch (...)
		{
			std::print("t\n");
		}

		if (FAILED(HR2))
		{
			if(result)
				result->Release();

			return {};
		}
		else
		{
			IDxcBlob* byteCodeBlob;
			HRESULT status;
			result->GetStatus(&status);

			while (FAILED(status))
			{
				IDxcBlobEncoding* errors;
				result->GetErrorBuffer(&errors);

				auto errorString = (const char*)errors->GetBufferPointer();

				std::string traceMessage = GetCallStackString();
				std::string formattedMessage =
					std::format("{}\nFailed to Compile Shader\nEntryPoint: {}\nFile : {}\nStack Trace : \n {}\nPress Enter to try again\n",
						errorString, entryPoint ? entryPoint : "No Entry Point", file, traceMessage);


				FK_LOG_ERROR(formattedMessage.c_str());

				size_t size;
				std::string line;
				std::getline(std::cin, line);


				errors->Release();
				IDxcResult* compileResult = nullptr;

				HR1 = hlslLibrary->CreateBlobFromFile(fileW, nullptr, &blob);
				HR2 = hlslCompiler->Compile(
					&buffer,
					arguments.data(),
					(UINT)arguments.size(),
					&includeHandler,
					IID_PPV_ARGS(&result));

				result->GetStatus(&status);
			}


			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out{ (char*)byteCodeBlob->GetBufferPointer(), byteCodeBlob->GetBufferSize(), FlexKit::SystemAllocator };
			byteCodeBlob->Release();
			result->Release();

			return out;
		}
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
			    resources.Set<ResourceFieldID::APIHandle, ResourceFieldID::Layout> (
				        resourceHandle,
				        vkResourceEntry{
					        .type		= vkResourceEntry::Type::RenderTarget,
					        .image		= (VkImage)desc._ptr,
							//.imageView	= imageView
				        },
					    desc.initialLayout);



		    }	break;
		case ResourceType::DepthTarget:
		    {
			    FK_ASSERT(false);
		    }	break;
		case ResourceType::UnorderedAccess:
		    {
			    FK_ASSERT(false);
		    }	break;
		case ResourceType::UnorderedAccessRenderTarget:
		    {
			    FK_ASSERT(false);
		    }	break;
		case ResourceType::ShaderResource:
		    {
			    //FK_ASSERT(false);
		    }	break;
		case ResourceType::RayTracingStructure:
		    {
			    FK_ASSERT(false);
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


	IndirectLayout vkRenderSystem::CreateIndirectLayout(static_vector<IndirectDrawDescription> entries, iAllocator* allocator, const IPipelineInterface* signature)
	{
	    return {};
	}


	ReadBackResourceHandle vkRenderSystem::CreateReadBackBuffer(const size_t bufferSize)
	{
		return InvalidHandle;
	}


	bool vkRenderSystem::CreatePipelineBuilder(std::byte* _ptr, size_t bufferSize, iAllocator& tempAllocator)
	{
		std::construct_at((vkPipelineBuilder*)_ptr, *this, tempAllocator);
		return false;
	}


	void vkRenderSystem::CreateTextureView(ResourceHandle, DescHeapPOS)
	{
	    
	}


	const IPipelineInterface* vkRenderSystem::Library(ROOTLIBRARYSIG ID) const noexcept
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

	VkDevice vkRenderSystem::GetDevice()
	{
		return device;
	}

	VkQueue	vkRenderSystem::GetQueue() const
	{
		return device.get_queue(vkb::QueueType::graphics).value();
	}


	uint32_t SyncPointToVK(DeviceSyncPoint pipeline) noexcept
	{
		VkPipelineStageFlags out = 0;

		out |= (pipeline | DeviceSyncPoint::Sync_VertexShader	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_VERTEX_INPUT_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_HullShader		!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_DomainShader	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_GeometryShader	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Mesh			!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT: 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Amplification	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT : 0;
		out |= (pipeline | DeviceSyncPoint::Sync_PixelShader	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 0;
		out |= (pipeline | DeviceSyncPoint::Sync_RenderTarget	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Raytracing		!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR: 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Copy			!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Compute		!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Raytracing		!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR : 0;
		//out |= (pipeline | DeviceSyncPoint::Sync_Predication	!= 0 ) ? VkPipelineStageFlagBits::VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT : 0;

		return out;
	}


	uint32_t AccessToVK(DeviceAccessState access) noexcept
	{
        switch (access)
        {
		case DASReadFlag:
			return VK_ACCESS_2_MEMORY_READ_BIT;
        case DASWriteFlag:
			return VK_ACCESS_2_MEMORY_WRITE_BIT;
        case DASRetired:
			return VK_ACCESS_2_NONE;
		case DASPresent:
			return VK_ACCESS_2_MEMORY_READ_BIT;
		case DASRenderTarget:
			return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		case DASPixelShaderResource:
			return VK_ACCESS_2_SHADER_READ_BIT;
		case DASUAV:
			return VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT;
		case DASSTREAMOUT:
			return VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;
		case DASVERTEXBUFFER:
			return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		case DASDEPTHBUFFER:
			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case DASDEPTHBUFFERREAD:
			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case DASDEPTHBUFFERWRITE:
			return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case DASACCELERATIONSTRUCTURE_WRITE:
			return VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		case DASACCELERATIONSTRUCTURE_READ:
			return VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		case DASPREDICATE:
			return VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
		case DASINDIRECTARGS:
			return VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		case DASNonPixelShaderResource:
			return VK_ACCESS_2_SHADER_READ_BIT;
		case DASCopyDest:
			return VK_ACCESS_2_MEMORY_WRITE_BIT;
		case DASCopySrc:
			return VK_ACCESS_2_MEMORY_READ_BIT;
		case DASINDEXBUFFER:
			return VK_ACCESS_2_INDEX_READ_BIT;
		case DASGenericRead:
			return VK_ACCESS_2_MEMORY_READ_BIT;
		case DASCommon:
			return VK_ACCESS_2_MEMORY_READ_BIT;
		case DASShadingRateSrc:
			return VK_ACCESS_2_MEMORY_READ_BIT;
		case DASShadingRateDst:
			return VK_ACCESS_2_MEMORY_WRITE_BIT;
		case DASDecodeWrite:
			return VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		case DASProcessRead:
			return VK_ACCESS_2_MEMORY_READ_BIT_KHR;
		case DASProcessWrite:
			return VK_ACCESS_2_MEMORY_WRITE_BIT_KHR;
		case DASEncodeRead:
			return VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;
		case DASEncodeWrite:
			return VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR;
		case DASResolveRead:
			return VK_ACCESS_2_MEMORY_READ_BIT;
        case DASResolveWrite:
			return VK_ACCESS_2_MEMORY_WRITE_BIT;
		case DASNOACCESS:
		case DASERROR:
        case DASUNKNOWN:
			return VK_ACCESS_2_NONE;
        }

		std::unreachable();
		return VK_ACCESS_2_NONE;
	}


	uint32_t LayoutToVK(DeviceLayout layout) noexcept
    {
		switch (layout)
	    {
		case DeviceLayout::Common:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::Present:
			return VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		case DeviceLayout::GenericRead:
			return VkImageLayout::VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		case DeviceLayout::RenderTarget:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::UnorderedAccess:
			return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case DeviceLayout::DepthStencilWrite:
			return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case DeviceLayout::DepthStencilRead:
			return VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
		case DeviceLayout::ShaderResource:
			return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case DeviceLayout::CopySrc:
		case DeviceLayout::CopyDst:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::ResolveSrc:
		case DeviceLayout::ResolveDst:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::ShadingRateSrc:
			return VkImageLayout::VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		case DeviceLayout::VideoDecodeRead:
			return VkImageLayout::VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR;
		case DeviceLayout::DecodeWrite:
			return VkImageLayout::VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR;
		case DeviceLayout::ProcessRead:
		case DeviceLayout::ProcessWrite:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::EncodeRead:
			return VkImageLayout::VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR;
		case DeviceLayout::EncodeWrite:
			return VkImageLayout::VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR;
		case DeviceLayout::DirectQueueCommon:
		case DeviceLayout::DirectQueueGenericRead:
			return VkImageLayout::VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		case DeviceLayout::DirectQueueUnorderedAccess:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::DirectQueueShaderResource:
			return VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case DeviceLayout::DirectQueueCopySrc:
		case DeviceLayout::DirectQueueCopyDst:
		case DeviceLayout::ComputeQueueCommon:
		case DeviceLayout::ComputeQueueGenericRead:
		case DeviceLayout::ComputeQueueUnorderedAccess:
		case DeviceLayout::ComputeQueueShaderResource:
		case DeviceLayout::ComputeQueueCopySrc:
		case DeviceLayout::ComputeQueueCopyDst:
			return VkImageLayout::VK_IMAGE_LAYOUT_GENERAL;
		case DeviceLayout::Undefined:
		case DeviceLayout::Unknown:
			return VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
	    };

		std::unreachable();
    }


    uint32_t GetFormatElementSize(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SINT:
			return sizeof(int32_t) * 4;
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_UINT:
			return sizeof(int32_t) * 3;
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_UINT:
			return sizeof(int32_t) * 2;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
			return sizeof(uint16_t[4]);
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_USCALED:
			return sizeof(uint16_t[2]);
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
			return 64;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
			return 4;
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_USCALED:
			return 4;
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_USCALED:
			return 4;
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC3_UNORM_BLOCK:
			return 16;
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_UINT:
			return 4;
		case VK_FORMAT_UNDEFINED:
			return 1;
		default:
			FK_LOG_ERROR("UN-IMPLEMENTED FORMAT!");
			throw std::runtime_error{ "UN-IMPLEMENTED FORMAT!" };
			return -1;
		}

		std::unreachable();
	}


	VkFormat FormatToVK(DeviceFormat format)
		{
		switch (format)
		{
		case DeviceFormat::R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case DeviceFormat::R32G32B32A32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DeviceFormat::R32G32B32_FLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case DeviceFormat::R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case DeviceFormat::R32G32B32_INT:
			return VK_FORMAT_R32G32B32_SINT;
		case DeviceFormat::R32G32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case DeviceFormat::R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case DeviceFormat::R32G32_INT:
			return VK_FORMAT_R32G32_SINT;
		case DeviceFormat::R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case DeviceFormat::R32_UINT:
			return VK_FORMAT_R32_UINT;
		case DeviceFormat::R32_INT:
			return VK_FORMAT_R32_SINT;
		case DeviceFormat::R16G16B16A16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case DeviceFormat::R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case DeviceFormat::R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case DeviceFormat::R16G16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case DeviceFormat::R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case DeviceFormat::R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case DeviceFormat::R16_UINT:
			return VK_FORMAT_R16_UINT;
		case DeviceFormat::R16_SINT:
			return VK_FORMAT_R16_SINT;
		case DeviceFormat::R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case DeviceFormat::R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case DeviceFormat::R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case DeviceFormat::R8G8B8A8_UNORM_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case DeviceFormat::R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case DeviceFormat::R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		default:
			FK_LOG_ERROR("UN-IMPLEMENTED FORMAT!");
			throw std::runtime_error{ "UN-IMPLEMENTED FORMAT!" };
			return VkFormat::VK_FORMAT_UNDEFINED;
		}

		std::unreachable();
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
