#include <BuildSettings.hpp>
#include <Handle.hpp>
#include <RenderSystemInterface.hpp>

#include "vkCopyContext.hpp"
#include "vkDirectContext.hpp"
#include "vkRenderSystem.hpp"

#include "vkPipelineBuilder.hpp"
#include "vkDescriptorSet.hpp"
#include "vkVertexBufferSet.hpp"
#include "vkRenderDocDebug.hpp"

#include <vulkan/vulkan.hpp>
#include <ShaderPreprocessor.hpp>

#include <filesystem>
#include <fmt/format.h>
#include <print>


#ifdef WIN32
#include "Unknwnbase.h"
#endif

#ifdef ANDROID
#else
#include <directx-dxc/dxcapi.h>
#endif

#include <iostream>
#include <scn/xchar.h>
#include <scn/scan.h>
#include <scn/regex.h>
#include <regex>

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

	void LabelQueue(VkQueue queue, VkDevice device, const char* label)
	{
#ifdef _DEBUG
		if (vkSetDebugUtilsObjectName)
		{
			static int n = 0;

			VkDebugUtilsObjectNameInfoEXT nameInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext			= nullptr,
				.objectType		= VkObjectType::VK_OBJECT_TYPE_QUEUE,
				.objectHandle	= (uint64_t)queue,
				.pObjectName	= label
			};

			vkSetDebugUtilsObjectName(device, &nameInfo);
		}
#endif
	}


	void LabelBuffer(VkBuffer buffer, VkDevice device, const char* label)
	{
#ifdef _DEBUG
		if (vkSetDebugUtilsObjectName)
		{
			static int n = 0;

			VkDebugUtilsObjectNameInfoEXT nameInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext			= nullptr,
				.objectType		= VkObjectType::VK_OBJECT_TYPE_BUFFER,
				.objectHandle	= (uint64_t)buffer,
				.pObjectName	= label
			};

			vkSetDebugUtilsObjectName(device, &nameInfo);
		}
#endif
	}

	void LabelImage(VkImage image, VkDevice device, const char* label)
	{
#ifdef _DEBUG
		if (vkSetDebugUtilsObjectName)
		{
			static int n = 0;

			VkDebugUtilsObjectNameInfoEXT nameInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext			= nullptr,
				.objectType		= VkObjectType::VK_OBJECT_TYPE_IMAGE,
				.objectHandle	= (uint64_t)image,
				.pObjectName	= label
			};

			vkSetDebugUtilsObjectName(device, &nameInfo);
		}
#endif
	}

	void LabelSemaphore(VkSemaphore semaphore, VkDevice device, const char* label)
	{
#ifdef _DEBUG
		if (vkSetDebugUtilsObjectName)
		{
			static int n = 0;

			VkDebugUtilsObjectNameInfoEXT nameInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext			= nullptr,
				.objectType		= VkObjectType::VK_OBJECT_TYPE_SEMAPHORE,
				.objectHandle	= (uint64_t)semaphore,
				.pObjectName	= label
			};

			vkSetDebugUtilsObjectName(device, &nameInfo);
		}
#endif
	}

	void LabelFence(VkFence* fence, VkDevice device, const char* label)
	{
#ifdef _DEBUG
		if (vkSetDebugUtilsObjectName)
		{
			static int n = 0;

			VkDebugUtilsObjectNameInfoEXT nameInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.pNext			= nullptr,
				.objectType		= VkObjectType::VK_OBJECT_TYPE_FENCE,
				.objectHandle	= (uint64_t)fence,
				.pObjectName	= label
			};

			vkSetDebugUtilsObjectName(device, &nameInfo);
		}
#endif
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


	std::optional<BufferAPIObject> CreateUploadBuffer(vkRenderSystem& renderSystem, size_t bufferSize)
	{
	     // Create Buffer
		VkBufferCreateInfo createBufferInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,			// VkBufferCreateFlags;
			.size					= bufferSize,	// VkDeviceSize
			.usage					= VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		// const uint32_t*        
		};

		VkMemoryRequirements memoryRequirements{};

		VkBuffer buffer;
		vkCreateBuffer(renderSystem.device, &createBufferInfo, nullptr, &buffer);
		vkGetBufferMemoryRequirements(renderSystem.device, buffer, &memoryRequirements);

		auto allocationRes = renderSystem.memoryAllocator.Allocate(
			0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			memoryRequirements.size,
			memoryRequirements.alignment);

		if (!allocationRes.has_value())
			return {};

		auto&& [offset, memory] = allocationRes.value();


		vkBindBufferMemory(renderSystem.device, buffer, memory, offset);

		return BufferAPIObject
				{
					.buffer = buffer,
					.memory = memory,
					.byteOffset	= offset
				};
	}


	std::optional<TextureAPIObject>	CreateTextureResource(vkRenderSystem& renderSystem, uint2 WH, DeviceFormat format)
	{
		VkImageCreateInfo createInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0u,//VkImageCreateFlags
			.imageType				= VkImageType::VK_IMAGE_TYPE_2D,
			.format					= FormatToVK(format),
			.extent					= VkExtent3D{},
			.mipLevels				= 1,
			.arrayLayers			= 1,
			.samples				= {},
			.tiling					= VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
			.usage					= VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode			= VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,
			.pQueueFamilyIndices	= nullptr,
			.initialLayout			= VkImageLayout::VK_IMAGE_LAYOUT_GENERAL,
		};

	    VkImage image;
		if (auto res = vkCreateImage(renderSystem.device, &createInfo, nullptr, &image); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to create texture resource" };


		VkImageMemoryRequirementsInfo2	requirements;
		VkMemoryRequirements2			memoryRequirments;
		vkGetImageMemoryRequirements2(renderSystem.device, &requirements, &memoryRequirments);

		auto allocationRes =
			renderSystem.memoryAllocator.Allocate(
			    0,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			    memoryRequirments.memoryRequirements.size,
			    memoryRequirments.memoryRequirements.alignment);

		if (!allocationRes.has_value())
			return {};

		auto&& [offset, memory] = allocationRes.value();

		vkBindImageMemory(renderSystem.device, image, memory, offset);

		return 
	        TextureAPIObject{
			    .image	= image,
			    .memory = memory,
			    .offset = offset,
		};
	}


	std::optional<BufferAPIObject> CreateVertexBuffer(vkRenderSystem& renderSystem, size_t bufferSize, bool GPUResident, uint32_t extraFlags)
	{
	    auto allocationRes = renderSystem.memoryAllocator.Allocate(
			0,
			GPUResident ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			bufferSize,
			0x10000
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
			.usage					= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		//const uint32_t*        
		};

		VkBuffer buffer;
		vkCreateBuffer(renderSystem.device, &createBufferInfo, nullptr, &buffer);
		vkBindBufferMemory(renderSystem.device, buffer, memory, offset);

		return BufferAPIObject
		        {
			        .buffer		= buffer,
			        .memory		= memory,
					.byteOffset		= offset
		        };
	}


    std::optional<BufferAPIObject> CreateConstantBuffer(vkRenderSystem& renderSystem, size_t bufferSize, bool GPUResident)
	{
	    auto allocationRes = renderSystem.memoryAllocator.Allocate(
			0,
			GPUResident ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
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
			.usage					= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
			.sharingMode			= VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount	= 0,			// uint32_t               
			.pQueueFamilyIndices	= nullptr		//const uint32_t*        
		};

		VkBuffer buffer;
		vkCreateBuffer(renderSystem.device, &createBufferInfo, nullptr, &buffer);
		vkBindBufferMemory(renderSystem.device, buffer, memory, offset);

		return BufferAPIObject
		        {
			        .buffer		= buffer,
			        .memory		= memory,
					.byteOffset		= offset
		        };
	}

	VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const FlexKit::DescriptorHeapLayout& layout, iAllocator& allocator)
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
		copyContextTable		{ IN_allocator },
		memoryAllocator			{ IN_allocator },
		directFramesInFlight	{ IN_allocator },
		pipelineStates			{ threads, IN_allocator },
	    resources				{ IN_allocator },
        vkAllocators			{ nullptr },
		vertexPushBuffers		{ IN_allocator },
		mappings				{ IN_allocator },
		constantPushBuffers		{ IN_allocator },
		freeFences				{ IN_allocator } {}


	bool vkRenderSystem::Initiate(Graphics_Desc& desc)
	{
		FK_LOG_9("VK: Vulkan SDK VERSION: %i\n", VK_HEADER_VERSION);

		#ifdef ANDROID
		FK_LOG_9("VK: Android Detected!");
		#endif

		allocator = desc.Memory;
		const char* extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,

#ifdef ANDROID
			"VK_KHR_android_surface",
#endif
#ifdef WIN32
			VK_KHR_DISPLAY_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef __linux__
			//VK_KHR_DISPLAY_EXTENSION_NAME,
			//VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
			//VK_KHR_XCB_SURFACE_EXTENSION_NAME,
			//VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
			//VK_KHR_DISPLAY_EXTENSION_NAME,
#endif

			VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
			VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
		};

	    vkb::InstanceBuilder builder;
		auto instReq = builder.set_app_name("FlexKit")
#ifdef ANDROID
		    .require_api_version(1, 4, 0)
#else	
		    .require_api_version(1, 4, VK_HEADER_VERSION)
#endif
			.request_validation_layers()
			.set_headless()
		    .enable_extensions(std::size(extensions), extensions)
			.set_debug_callback (
				[] (VkDebugUtilsMessageSeverityFlagBitsEXT 		messageSeverity,
					VkDebugUtilsMessageTypeFlagsEXT 			messageType,
					const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
					void*										pUserData)
					-> VkBool32 
					{
						auto severity	= vkb::to_string_message_severity(messageSeverity);
						auto type		= vkb::to_string_message_type(messageType);

						auto message = std::format("VK: Validation Error! Severity: {}, Type: {}, Message: {}", severity, type, pCallbackData->pMessage);
						FK_LOG_INFO(message.c_str());
						return false;
					})

			.build();

		if (!instReq)
		{
			FK_LOG_ERROR("VK:vkRenderSystem::Initiate(...): Missing Instance Extension!");
			return false;
		}
		else 
			FK_LOG_9("VK:vkRenderSystem::Initiate(...): Instance Created!");


		instance = instReq.value();
		vkb::PhysicalDeviceSelector selector{ instance };


		VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1FeatureEnable{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT,
			.pNext					= nullptr,
			.swapchainMaintenance1	= true
		};

		VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeature{
			.sType								= VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
	        .pNext								= &swapchainMaintenance1FeatureEnable,
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

		FK_LOG_9("VK: Selecting Device!");

		auto physRequest = selector
#ifdef ANDROID
	        .set_minimum_version(1, 3)
#else
	        .set_minimum_version(1, 4)
#endif
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.add_required_extension("VK_KHR_depth_stencil_resolve")
		    .add_required_extension("VK_KHR_dynamic_rendering")
			.add_required_extension("VK_KHR_maintenance3")
            .add_required_extension("VK_KHR_swapchain")
			.add_required_extension("VK_KHR_timeline_semaphore")
			.add_required_extension("VK_KHR_spirv_1_4")
			.add_required_extension("VK_EXT_descriptor_buffer")
			.add_required_extension("VK_EXT_swapchain_maintenance1")
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

		FK_LOG_9("VK: Getting Queues!");

	    device = devRequest.value();
		auto graphicsQueueRequest = device.get_queue(vkb::QueueType::graphics);
		auto transferQueueRequest = device.get_queue(vkb::QueueType::transfer);
		auto computeQueueRequest  = device.get_queue(vkb::QueueType::compute);

		if (!graphicsQueueRequest.has_value() || !transferQueueRequest.has_value() || !computeQueueRequest.has_value())
		{
			FK_LOG_ERROR("VK: Failed Getting Queues!");
			return false;
		}

		graphicsQueue	= graphicsQueueRequest.value();
		transferQueue	= transferQueueRequest.value();
		computeQueue	= computeQueueRequest.value();

		FK_LOG_9("VK: Getting Extension functions!");
		vkGetDescriptorSetLayoutSize				= (vkGetDescriptorSetLayoutSizeFNDef)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutSizeEXT");
		vkGetDescriptor								= (vkGetDescriptorFNDef)vkGetDeviceProcAddr(device, "vkGetDescriptorEXT");
		vkCmdBindDescriptorBufferEmbeddedSamplers	= (vkCmdBindDescriptorBufferEmbeddedSamplersFNDef)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT");
		vkCmdBindDescriptorBuffers					= (vkCmdBindDescriptorBuffersFNDef)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorBuffersEXT");
		vkGetDescriptorSetLayoutBindingOffset		= (vkGetDescriptorSetLayoutBindingOffsetFNDef)vkGetDeviceProcAddr(device, "vkGetDescriptorSetLayoutBindingOffsetEXT");
		vkCmdSetDescriptorBufferOffsets				= (vkCmdSetDescriptorBufferOffsetsFNDef)vkGetDeviceProcAddr(device, "vkCmdSetDescriptorBufferOffsetsEXT");

		FK_ASSERT(vkGetDescriptorSetLayoutSize != nullptr, "VK: Failed to get vkGetDescriptorSetLayoutSizeEXT");
		FK_ASSERT(vkGetDescriptor != nullptr, "VK: Failed to get vkGetDescriptorEXT");
		FK_ASSERT(vkCmdBindDescriptorBufferEmbeddedSamplers != nullptr, "VK: Failed to get vkCmdBindDescriptorBufferEmbeddedSamplersEXT");
		FK_ASSERT(vkCmdBindDescriptorBuffers != nullptr, "VK: Failed to get vkCmdBindDescriptorBuffersEXT");
		FK_ASSERT(vkGetDescriptorSetLayoutBindingOffset != nullptr, "VK: Failed to get vkCmdBindDescriptorBuffersEXT");
		FK_ASSERT(vkCmdSetDescriptorBufferOffsets != nullptr, "VK: Failed to get vkCmdSetDescriptorBufferOffsetsEXT");


#ifdef _DEBUG
		vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");

		if(vkSetDebugUtilsObjectName == nullptr)
			FK_LOG_WARNING("VK: Failed to get vkSetDebugUtilsObjectNameEXT");
#endif

		FK_LOG_9("VK: Initializing Memory Allocator!");
		descriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
		descriptorBufferProperties.pNext = nullptr;

	    VkPhysicalDeviceProperties2KHR deviceProperties{};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
		deviceProperties.pNext = &descriptorBufferProperties;
		vkGetPhysicalDeviceProperties2(device.physical_device, &deviceProperties);
		
		memoryAllocator.Init(*this);

		auto descriptorHeapBufferAllocation = memoryAllocator.Allocate(0,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			64 * KILOBYTE);
		
		if (!descriptorHeapBufferAllocation)
			throw std::runtime_error{ "VK: Failed to allocate descriptor heap buffer!" };


		FK_LOG_9("VK: Creating descriptor buffer!");
		descriptorPool = CreateDescriptorBuffer(device, 1000000u, descriptorHeapBufferAllocation.value());

		auto [offset, memory] = descriptorHeapBufferAllocation.value();

		uint64_t cpuAddress = (uint64_t)MapDeviceAddress(memory, offset);

		VkBufferDeviceAddressInfoKHR address_info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR };
		address_info.buffer = descriptorPool.buffer;
		auto gpuAddress = vkGetBufferDeviceAddress(device, &address_info);

		HeapAllocatorDescription heapAllocDesc{
			.CPUBegin	= cpuAddress,
			.GPUBegin	= gpuAddress,
			.size		= 1000000u,
			.device		= device, 
			.buffer		= descriptorPool.buffer
		};

		FK_LOG_9("VK: Allocating Descriptor Heap!");
	    heapAllocator.Initialize(heapAllocDesc, allocator);


		FK_LOG_9("VK: Creating Fences!");
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


		FK_LOG_9("VK: Creating Semaphores!");
		if (auto res = vkCreateSemaphore(device, &createTimelineSemaphoreInfo, nullptr, &vkDirectQueueCounter); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create timeline semaphore queue!");

		if (auto res = vkCreateFence(device, &createFenceInfo, nullptr, &transferQueueFence); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create fence for transfer queue!");

		if (auto res = vkCreateSemaphore(device, &createTimelineSemaphoreInfo, nullptr, &vkTransferQueueCounter); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create transfer timeline semaphore queue!");

		if (auto res = vkCreateSemaphore(device, &createTimelineSemaphoreInfo, nullptr, &vkComputeQueueCounter); res != VK_SUCCESS)
			throw std::runtime_error("Failed to create compute timeline semaphore queue!");

		RenderDocDebugUtils::Connect();

		FK_LOG_9("VK: Labeling Objects!");
		LabelQueue(graphicsQueue, device, "Graphics Queue");
		LabelQueue(transferQueue, device, "Transfer Queue");
		LabelQueue(computeQueue, device, "Compute Queue");
		LabelSemaphore(vkDirectQueueCounter, device, "Direct Semaphore");
		LabelSemaphore(vkTransferQueueCounter, device, "Transfer Semaphore");
		LabelSemaphore(vkComputeQueueCounter, device, "Compute Semaphore");
		
		FK_LOG_9("VK: Feature Querying!");
		availableFeatures.RT_Level 			= AvailableFeatures::RT_FeatureLevel_NOTAVAILABLE;
		availableFeatures.conservativeRast 	= AvailableFeatures::ConservativeRast_NOTAVAILABLE;
		
		#ifdef ANDROID
		availableFeatures.Compiler = AvailableFeatures::HLSL_CompilerDisabled;
		#else
		availableFeatures.Compiler = AvailableFeatures::HLSL_CompilerEnabled;
		#endif

		availableFeatures.workGraph 		= AvailableFeatures::WorkGraphs_NOTAVAILABLE;
		availableFeatures.indirectLevel 	= AvailableFeatures::IndirectLevel_1;
		availableFeatures.resourceHeapTier	= ResourceHeapTier::HeapTier1;

		FK_LOG_9("VK: Initialization Success!");

		VkSemaphore	vkComputeQueueCounter = nullptr;

		return true;
	}


	FlexKit::AvailableFeatures vkRenderSystem::GetFeatures() const noexcept
	{
		return availableFeatures;
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
		    .fence			= vkDirectQueueCounter };
	}


	void vkRenderSystem::SyncUploadTo(SyncPoint)
	{
		DebugBreak();
	}


	SyncPoint vkRenderSystem::SyncUploadPoint()
	{
		const uint64_t counter = vkTransferQueueProgress;
		return { counter, vkTransferQueueCounter };
	}


	SyncPoint vkRenderSystem::SyncUploadTicket()
	{
		const uint64_t counter = ++vkTransferQueueProgress;
		return { counter, vkTransferQueueCounter };
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


	void vkRenderSystem::SubmitUploadQueues(CopyContextHandle* handles, size_t count, std::optional<SyncPoint> syncBefore, std::optional<SyncPoint> syncAfter)
	{
		uint64_t submissionValue = 0;

		Vector<VkCommandBufferSubmitInfo, 8>	cmdBufferSubmit	{ allocator };
		Vector<SyncPoint, 8>					waits			{ allocator };
		Vector<SyncPoint, 8>					signals			{ allocator };

		if(syncBefore)
			waits.push_back(syncBefore.value());

		if (syncAfter)
			signals.push_back(syncAfter.value());

		for(size_t i = 0; i < count; i++)
		{
			const CopyContextHandle handle = handles[i];

			auto res = copyContextTable.find(handle);
			if (res != nullptr)
			{
				auto vkCL = static_cast<vkCopyContext*>(*res);
				vkCL->Close();

				VkCommandBufferSubmitInfo info{
					.sType			= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
					.pNext			= nullptr,
					.commandBuffer	= vkCL->cmdBuffer,
					.deviceMask		= 0
				};

				cmdBufferSubmit.push_back(info);
			}
		}

		Vector<VkSemaphoreSubmitInfo, 8> waitInfos		{ allocator };
		for (auto waits : waits)
		{
			auto& [value, syncObject] = waits;

			VkSemaphoreSubmitInfo infos{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext			= nullptr,
				.semaphore		= syncObject.As_ptr<VkSemaphore>(),
				.value			= value,
				.stageMask		= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
				.deviceIndex	= 0
			};

			waitInfos.push_back(infos);
		}

		Vector<VkSemaphoreSubmitInfo, 8> signalInfos{ allocator };
		for(auto signal : signals)
		{
			auto& [value, syncObject] = signal;

			VkSemaphoreSubmitInfo infos{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext			= nullptr,
				.semaphore		= syncObject.As_ptr<VkSemaphore>(),
				.value			= value,
				.stageMask		= VkPipelineStageFlagBits::VK_PIPELINE_STAGE_TRANSFER_BIT,
				.deviceIndex	= 0
			};

			signalInfos.push_back(infos);
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

		vkResetFences(device, 1, &transferQueueFence);

		if (auto res = vkQueueSubmit2(transferQueue, 1, &submit, transferQueueFence); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to submit to Transfer Queue!" };
	}


	CopyContextHandle vkRenderSystem::OpenUploadQueue()
	{
		auto copyContextHandle{ rand() };
		auto copyContext_ptr = &allocator->allocate<vkCopyContext>();
		copyContextTable.insert(copyContextHandle, copyContext_ptr);

		return copyContextHandle;
	}


	CopyContextHandle vkRenderSystem::GetImmediateCopyQueue()
	{
		if (immediateUploadQueue == InvalidHandle)
			immediateUploadQueue = OpenUploadQueue();

		return immediateUploadQueue;
	}


	IDirectContext& vkRenderSystem::GetDirectCommandList(std::optional<SyncPoint> ticket)
	{
		uint64_t current;
		vkGetSemaphoreCounterValue(device, vkDirectQueueCounter, &current);

		vkDirectContext* ctx = nullptr;

		if (freeDirectContexts.size())
		{
			ctx = freeDirectContexts.pop_back();
			ctx->Reset();
		}
		else
		{
			for (auto& pendingFrame : directFramesInFlight)
			{
				auto status = vkGetFenceStatus(device, pendingFrame.fence);

				if (status == VK_SUCCESS)
				{
					ctx = pendingFrame.contexts.pop_back();
					ctx->Reset();

					while (pendingFrame.contexts.size())
						freeDirectContexts.push_back(pendingFrame.contexts.pop_back());

					freeFences.push_back(pendingFrame.fence);

					directFramesInFlight.remove_unstable(&pendingFrame);
					break;
				}
			}
		}

		if (ctx == nullptr)
		    ctx = &allocator->allocate<vkDirectContext>();

		ctx->Begin(ticket.has_value() ? ticket.value().syncCounter : 0);

		return *ctx;
	}


	ICopyContext& vkRenderSystem::GetCopyContext(CopyContextHandle handle)
	{
		auto res = copyContextTable.find(handle);
		FK_ASSERT(res != nullptr);

		return **res;
	}


	SyncPoint vkRenderSystem::Submit(std::span<IDirectContext*> CLs, std::optional<SyncPoint> sync)
	{
		uint64_t submissionValue = 0;

		Vector<VkCommandBufferSubmitInfo, 8>	cmdBufferSubmit	{ allocator };
		Vector<SyncPoint, 8>					waits			{ allocator };
		Vector<SyncPoint, 8>					signals			{ allocator };

		if (immediateUploadQueue != InvalidHandle)
		{
			auto transferCtx = std::exchange(immediateUploadQueue, InvalidHandle);
			auto transferSyncPoint = SyncUploadTicket();

			SubmitUploadQueues(&transferCtx, 1, {}, { transferSyncPoint });
			waits.push_back(transferSyncPoint);
		}

		for (auto& cl : CLs)
		{
			auto vkCL = static_cast<vkDirectContext*>(cl);
			vkCL->Close();

			VkCommandBufferSubmitInfo info{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
				.pNext			= nullptr,
				.commandBuffer	= vkCL->cmdBuffer,
				.deviceMask		= 0
			};

			cmdBufferSubmit.push_back(info);
			submissionValue = Max(submissionValue, vkCL->dispatchValue);

			for (auto sp : vkCL->waits)
				waits.push_back(sp);

			for (auto sp : vkCL->signals)
				signals.push_back(sp);
		}

		auto waitsEnd	= std::unique(waits.begin(), waits.end());
		auto signalsEnd = std::unique(signals.begin(), signals.end());


		Vector<VkSemaphoreSubmitInfo, 8> waitInfos{ allocator };
		for (auto& syncObject : std::span(waits.begin(), waitsEnd))
		{
			VkSemaphoreSubmitInfo signalInfo{
			    .sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
	            .pNext		= nullptr,
	            .semaphore	= syncObject.fence.As_ptr<VkSemaphore>(),
				.value		= syncObject.syncCounter,
		        .stageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
		    };

			waitInfos.push_back(signalInfo);
		}

		if (sync)
		{
			VkSemaphoreSubmitInfo signalInfo{
				.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext		= nullptr,
				.semaphore	= sync.value().fence.As_ptr<VkSemaphore>(),
				.stageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR
			};

			waitInfos.push_back(signalInfo);
		}

		Vector<VkSemaphoreSubmitInfo, 8> signalInfos{ allocator };
		for (auto& syncObject : std::span(signals.begin(), signalsEnd))
		{
			VkSemaphoreSubmitInfo signalInfo{
				.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext		= nullptr,
				.semaphore	= syncObject.fence.As_ptr<VkSemaphore>(),
				.value		= syncObject.syncCounter,
				.stageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR
			};

			signalInfos.push_back(signalInfo);
		}

		VkSemaphoreSubmitInfo timelineSignalInfo{
				.sType		= VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
				.pNext		= nullptr,
				.semaphore	= vkDirectQueueCounter,
                .value		= submissionValue,
				.stageMask	= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
		};

		signalInfos.push_back(timelineSignalInfo);

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


		auto fence = [&]{
			if (freeFences.size())
			{
				auto fence = freeFences.pop_back();
				vkResetFences(device, 1, &fence);
				return fence;
			}
			else
			{
				VkFenceCreateInfo createFenceInfo{
					.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0
				};

				VkFence newFence;
				if (auto res = vkCreateFence(device, &createFenceInfo, nullptr, &newFence); res != VK_SUCCESS)
					throw std::runtime_error("Failed to create fence for direct queue!");

				return newFence;
			}
			}();

		if (auto res = vkQueueSubmit2(graphicsQueue, 1, &submit, fence); res != VK_SUCCESS)
			throw std::runtime_error{ "VK: Failed to submit to Direct Command Queue!" };

		Frame newFrame{ .contexts{ allocator } };

		for (auto cl : CLs)
		{
			auto vkCL		= static_cast<vkDirectContext*>(cl);
			newFrame.fence	= fence;
			newFrame.contexts.push_back(vkCL);
		}

		directFramesInFlight.push_back(newFrame);

		return {
			submissionValue,
			vkDirectQueueCounter
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


	size_t vkRenderSystem::GetVertexBufferOffset(const VertexBufferHandle handle) const
	{
		return vertexPushBuffers.GetOffset(handle);
	}


	bool vkRenderSystem::VertexBufferPush(VertexBufferHandle handle, void* _ptr, size_t elementSize)
	{
		return vertexPushBuffers.Push(handle, _ptr, elementSize);
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


	SubAllocation vkRenderSystem::ReserveConstantBuffer(ConstantBufferHandle cb, size_t size) noexcept
	{
		return constantPushBuffers.Reserve(cb, size);
	}

	SubAllocation vkRenderSystem::ReserveVertexBuffer(VertexBufferHandle vb, size_t size)	noexcept
	{
		return vertexPushBuffers.Reserve(vb, size);
	}


	UploadReservation vkRenderSystem::ReserveDirectUploadSpace(size_t size, size_t alignment)	noexcept
	{
		return {};
	}


	UploadReservation vkRenderSystem::ReserveUploadBuffer(const size_t uploadSize, CopyContextHandle)	noexcept
	{
		return {};
	}

#ifndef ANDROID
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

		HRESULT QueryInterface(const IID&, void**) override
		{
			return 0;
		}

		std::filesystem::path   includePath;
		IDxcIncludeHandler*     handler;

		ULONG AddRef() { return 0; }
		ULONG Release() { return 0; }
	};
#endif

	Shader vkRenderSystem::LoadShader(const char* entryPoint, const char* profile, const char* file, const ShaderOptions& options)
	{
#ifndef ANDROID

		IDxcUtils* hlslUtils = nullptr;
		IDxcIncludeHandler* hlslIncludeHandler = nullptr;
		IDxcCompiler3* hlslCompiler = nullptr;

		if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslUtils))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Library!" });

		if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
			throw(std::runtime_error{ "Unable to create HLSL 6.x Compiler!" });

		if (hlslUtils) hlslUtils->CreateDefaultIncludeHandler(&hlslIncludeHandler);

		EXITSCOPE({
			if (hlslUtils) hlslUtils->Release();
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

		auto type = [](const char* profile)
			{
				uint16_t code = *(uint16_t*)profile;

				switch (code)
				{
				case 0x7370:
					return SHADER_TYPE::Pixel;
				case 0x7376:
					return SHADER_TYPE::Vertex;
				case 0x7361:
					return SHADER_TYPE::Amplification;
				case 0x736d:
					return SHADER_TYPE::Mesh;
				case 0x7363:
					return SHADER_TYPE::Compute;
				case 0x7364:
					return SHADER_TYPE::Domain;
				case 0x7368:
					return SHADER_TYPE::Hull;
				default:
					return SHADER_TYPE::Unknown;
				}
			}(profile);

		auto size = FlexKit::GetFileSize(filePath.string().c_str());
		std::string shaderStr;
		shaderStr.resize(size);
		LoadFileIntoBuffer(filePath.string().c_str(), (std::byte*)shaderStr.data(), size);

		auto&& [attributes, CBVcount, tableCount] = VKShaderProprocessor(shaderStr, type, *allocator);

		IDxcBlobEncoding* blob;
		auto HR1 = hlslUtils->CreateBlobFromPinned(shaderStr.data(), shaderStr.size(), DXC_CP_ACP, &blob);

		EXITSCOPE({ blob->Release(); });

		if (FAILED(HR1))
		{
#if WIN32
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
#endif
			return {};
		}

		IncludeHandler includeHandler;
		includeHandler.includePath = parentPath;
		includeHandler.handler = hlslIncludeHandler;


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

		//arguments.push_back(L"/T rootsig_1_1");
#else
		arguments.push_back(L"-O3");
#endif

		if (options.enable16BitTypes)
			arguments.push_back(L"-enable-16bit-types");

		if (options.hlsl2021)
			arguments.push_back(L"-HV 2021");

		IDxcOperationResult* result = nullptr;

		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;

#if WIN32
		blob->GetEncoding(&_, &buffer.Encoding);
#endif

		HRESULT HR2;
		try
		{
#if WIN32
			HR2 = hlslCompiler->Compile(
				&buffer,
				arguments.data(),
				arguments.size(),
				&includeHandler,
				IID_PPV_ARGS(&result));
#endif
		}
		catch (...)
		{
			std::print("t\n");
		}

		if (FAILED(HR2))
		{
			if (result)
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

#if WIN32
				HR2 = hlslCompiler->Compile(
					&buffer,
					arguments.data(),
					(UINT)arguments.size(),
					&includeHandler,
					IID_PPV_ARGS(&result));

				result->GetStatus(&status);
#endif
			}


			auto HR = result->GetResult(&byteCodeBlob);

			wchar_t* text = (wchar_t*)byteCodeBlob->GetBufferPointer();

			Shader out{ (char*)byteCodeBlob->GetBufferPointer(), byteCodeBlob->GetBufferSize(), FlexKit::SystemAllocator };
			byteCodeBlob->Release();

			if (attributes.size())
				out.GetExtra().attributes = std::move(attributes);

			out.type = type;
			result->Release();

			return out;
		}

		return {};
		#else
		FK_LOG_ERROR("NO SHADER COMPILATION ON ANDROID!");
		#endif

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
		auto allocation =
			heapAllocator.Alloc2Temp(
				descriptorCount, GetCurrentProgress(), 0xffffffffffffffff,
				descriptorBufferProperties.descriptorBufferOffsetAlignment);

		if (!allocation)
		{
			FK_LOG_ERROR("VK: allocation failed : Failed to bind inline descriptor set!");
			return {};
		}

		auto& [range, offset] = allocation.value();

		return range;
	}


	DeviceHeapHandle vkRenderSystem::CreateHeap(const size_t heapSize, const uint32_t flags)
	{
		return InvalidHandle;
	}


	ConstantBufferHandle vkRenderSystem::CreateConstantBuffer(size_t size, bool gpuResident)
	{
		return constantPushBuffers.CreateBuffer(size, gpuResident);
	}


	VertexBufferHandle vkRenderSystem::CreateVertexBuffer(size_t size, bool gpuResident)
	{
		return vertexPushBuffers.CreateBuffer(size, gpuResident);
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

			    FK_ASSERT(false);
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
		FK_ASSERT(bufferSize >= sizeof(vkPipelineBuilder));
		std::construct_at((vkPipelineBuilder*)_ptr, *this, tempAllocator);
		return false;
	}


	void vkRenderSystem::CreateDescriptorSet(std::byte* buffer, size_t size)
	{
		FK_ASSERT(size >= sizeof(vkDescriptorSet));
		std::construct_at<vkDescriptorSet>((vkDescriptorSet*)buffer, *this);
	}


	void vkRenderSystem::CreateTextureView(ResourceHandle, DescHeapPOS)
	{
	    
	}


	IVertexBufferSet& vkRenderSystem::CreateVertexBufferSet()
	{
		return VK_internal::CreateVertexBufferSet(allocator);
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
	{
		constantPushBuffers.Reset(constant, GetCurrentCounter());
	}


    void vkRenderSystem::ResetVertexBuffer(VertexBufferHandle handle)
	{
		vertexPushBuffers.Reset(handle, GetCurrentCounter());
	}


    void vkRenderSystem::ResetQuery(QueryHandle handle)
    {}


	void vkRenderSystem::ReleaseCB(ConstantBufferHandle handle)
	{
	    constantPushBuffers.ReleaseBuffer(handle);
	}


    void vkRenderSystem::ReleaseVB(VertexBufferHandle handle)
	{
		vertexPushBuffers.ReleaseBuffer(handle);
	}


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

	VkSemaphore	vkRenderSystem::GetSemaphore()
	{
		if (freeSemaphores.size())
			return freeSemaphores.pop_back();

		VkSemaphoreCreateInfo createInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		VkSemaphore newSemaphore = nullptr;
		if (auto res = vkCreateSemaphore(device, &createInfo, nullptr, &newSemaphore); res != VK_SUCCESS)
			FK_LOG_INFO("VK: Failed to create semaphore!");

		return newSemaphore;
	}

	std::byte* vkRenderSystem::MapDeviceAddress(VkDeviceMemory memory, uint32_t offset)
	{
		auto res = mappings.find(memory);

		if (!res)
		{
			std::byte* mappedAddress = nullptr;
			if (auto res = vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, (void**)&mappedAddress); res != VK_SUCCESS)
			{
				FK_LOG_ERROR("VK: Failed to map memory!");
				return nullptr;
			}

			mappings.insert(
				memory,
				DeviceMemoryMapping{
				    .refCount	= 1,
				    .mapping	= mappedAddress
				});

		    return mappedAddress + offset;
		}
		else
		{
			res->refCount++;
			return res->mapping + offset;
		}
	}

	void vkRenderSystem::UnMapDeviceAddress(VkDeviceMemory memory)
	{
		auto res = mappings.find(memory);
		if (res)
		{
			if (res->refCount-- == 0)
			{
				vkUnmapMemory(device, memory);

				mappings.remove(memory);
			}
		}
	}

	uint64_t SyncPointToVK(DeviceSyncPoint pipeline) noexcept
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


	uint64_t AccessToVK(DeviceAccessState access) noexcept
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


	uint64_t LayoutToVK(DeviceLayout layout) noexcept
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
			return sizeof(uint16_t[3]);
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


	VkBlendFactor BlendFactorToVk(FlexKit::EBlend blend)
	{
		switch (blend)
		{
		case FlexKit::EBlend::ALPHA_FACTOR:
			return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case FlexKit::EBlend::BLEND_FACTOR:
			return VkBlendFactor::VK_BLEND_FACTOR_CONSTANT_COLOR;
		case FlexKit::EBlend::DEST_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_DST_ALPHA;
		case FlexKit::EBlend::DEST_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_DST_COLOR;
		case FlexKit::EBlend::INV_ALPHA_FACTOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case FlexKit::EBlend::INV_BLEND_FACTOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case FlexKit::EBlend::INV_DEST_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case FlexKit::EBlend::INV_DEST_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case FlexKit::EBlend::INV_SRC_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case FlexKit::EBlend::INV_SRC1_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		case FlexKit::EBlend::INV_SRC_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case FlexKit::EBlend::INV_SRC1_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case FlexKit::EBlend::ONE:
			return VkBlendFactor::VK_BLEND_FACTOR_ONE;
		case FlexKit::EBlend::SRC_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA;
		case FlexKit::EBlend::SRC1_ALPHA:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC1_ALPHA;
		case FlexKit::EBlend::SRC_ALPHA_SAT:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case FlexKit::EBlend::SRC_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC_COLOR;
		case FlexKit::EBlend::SRC1_COLOR:
			return VkBlendFactor::VK_BLEND_FACTOR_SRC1_COLOR;
		case FlexKit::EBlend::ZERO:
			return VkBlendFactor::VK_BLEND_FACTOR_ZERO;
		}
	}

	VkBlendOp BlendOpToVk(FlexKit::EBlendOP blendOp)
	{
		switch (blendOp)
		{
			case FlexKit::EBlendOP::ADD:
				return VkBlendOp::VK_BLEND_OP_ADD;
			case FlexKit::EBlendOP::SUBTRACT:
				return VkBlendOp::VK_BLEND_OP_SUBTRACT;
			case FlexKit::EBlendOP::REV_SUBTRACT:
				return VkBlendOp::VK_BLEND_OP_REVERSE_SUBTRACT;
			case FlexKit::EBlendOP::MIN:
				return VkBlendOp::VK_BLEND_OP_MIN;
			case FlexKit::EBlendOP::MAX:
				return VkBlendOp::VK_BLEND_OP_MAX;
		}
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
