#include "vkPipelineBuilder.hpp"
#include "vkRenderSystem.hpp"
#include <spirv_cross/spirv_cross.hpp>

namespace VK_internal
{
	vkPipelineBuilder::vkPipelineBuilder(vkRenderSystem& system, iAllocator& IN_allocator) :
        allocator		{ IN_allocator },
	    stateObjects	{ IN_allocator },
        shaderStages	{ IN_allocator },
        shaders			{ IN_allocator }
	{
		int x = 0;
	}

	vkPipelineBuilder::~vkPipelineBuilder()
	{
	    
	}

	void vkPipelineBuilder::Release()
	{
	    
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddRootSignature(const IPipelineInterface* rootSig)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddWorkGraph(const WorkGraph_Desc& desc)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS		= (vkRenderSystem&)vkRenderSystem::GetInstance();
		auto	idx		= shaders.push_back(vkRS.LoadShader(entryPoint, "vs_6_6", file, options));

		VkShaderModuleCreateInfo moduleCreateInfo{
		    .sType		= VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext		= nullptr,
			.flags		= 0,
			.codeSize	= shaders[idx].bufferSize,
			.pCode		= (uint32_t*)shaders[idx].buffer
		};

		VkShaderModule shaderModule;
		if (auto res = vkCreateShaderModule(vkRS.device, &moduleCreateInfo, nullptr, &shaderModule); res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		VkPipelineShaderStageCreateInfo stage{
			.sType	= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext	= nullptr,
			.flags	= 0,
			.stage	= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
			.module = shaderModule,
			.pName	= entryPoint,
			.pSpecializationInfo = nullptr
		};

		shaderStages.push_back(stage);

		AddInputLayout({});
		AddInputTopology({});

		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS		= (vkRenderSystem&)vkRenderSystem::GetInstance();
		auto	idx		= shaders.push_back(vkRS.LoadShader(entryPoint, "ps_6_6", file, options));

		VkShaderModuleCreateInfo moduleCreateInfo{
		    .sType		= VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext		= nullptr,
			.flags		= 0,
			.codeSize	= shaders[idx].bufferSize,
			.pCode		= (uint32_t*)shaders[idx].buffer
		};

		VkShaderModule shaderModule;
		if (auto res = vkCreateShaderModule(vkRS.device, &moduleCreateInfo, nullptr, &shaderModule); res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		VkPipelineShaderStageCreateInfo stage{
			.sType	= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext	= nullptr,
			.flags	= 0,
			.stage	= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaderModule,
			.pName	= entryPoint,
			.pSpecializationInfo = nullptr
		};

		shaderStages.push_back(stage);

		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const Shader& shader)
    {
		const auto idx = shaders.push_back(shader);

	    VkShaderModuleCreateInfo moduleCreateInfo{
		    .sType		= VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext		= nullptr,
			.flags		= 0,
			.codeSize	= shaders[idx].bufferSize / 4,
			.pCode		= (uint32_t*)shaders[idx].buffer
		};

		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();
		VkShaderModule shaderModule;
		if (auto res = vkCreateShaderModule(vkRS.device, &moduleCreateInfo, nullptr, &shaderModule); res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}

		VkPipelineShaderStageCreateInfo stage{
			.sType	= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext	= nullptr,
			.flags	= 0,
			.stage	= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shaderModule,
			.pName	= entryPoint,
			.pSpecializationInfo = nullptr
		};

		shaderStages.push_back(stage);

		return *this;
	}


	IPipelineBuilderImpl& vkPipelineBuilder::SetDebugName(const char* name)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddInputLayout(const InputLayoutState& state)
	{
		VkPipelineVertexInputStateCreateInfo& inputLayout = [this]() -> VkPipelineVertexInputStateCreateInfo&
		    {
		        auto inputLayout = GetVertexInputState();
				if (inputLayout == nullptr)
				{
					inputLayout										= &allocator.allocate<VkPipelineVertexInputStateCreateInfo>();
					inputLayout->sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					inputLayout->pNext								= nullptr;
					inputLayout->flags								= 0;
					inputLayout->pVertexAttributeDescriptions		= nullptr;
					inputLayout->pVertexBindingDescriptions			= nullptr;
					inputLayout->vertexAttributeDescriptionCount	= 0;
					inputLayout->vertexBindingDescriptionCount		= 0;
				}

				return *inputLayout;
		    }();

		Vector<VkVertexInputAttributeDescription>	inputAttributes	{ allocator };
		Vector<VkVertexInputBindingDescription>		inputBindings	{ allocator };
		Vector<uint32_t>							slotSizes		{ allocator };

		for (auto [idx, input] : enumerate(state.inputs))
		{
			if (input.format == DeviceFormat::UNKNOWN)
				continue;

			inputAttributes.push_back(VkVertexInputAttributeDescription{
			    .location	= (uint32_t)idx,
			    .binding	= input.slot,
			    .format		= FormatToVK(input.format),
			    .offset		= input.alignedByteOffset,
            });

			if (slotSizes.size() < (input.slot + 1))
				slotSizes.resize(input.slot + 1);

			slotSizes[input.slot] += GetFormatElementSize(FormatToVK(input.format));
		}

		for (auto [idx, size] : enumerate(slotSizes))
		{
			if (size > 0)
			    inputBindings.push_back(VkVertexInputBindingDescription{
					    .binding	= (uint32_t)idx,
					    .stride		= size,
					    .inputRate	= VK_VERTEX_INPUT_RATE_VERTEX,
				    });
		}

		inputLayout.vertexBindingDescriptionCount	= inputBindings.size();
		inputLayout.pVertexBindingDescriptions		= inputBindings.data();
		inputLayout.vertexAttributeDescriptionCount = inputAttributes.size();
		inputLayout.pVertexAttributeDescriptions	= inputAttributes.data();

		stateObjects.push_back({
				.type = InfoType::VertexInput,
				._ptr = &inputLayout
			});

	    return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddInputTopology(const ETopology topology)
	{
		VkPipelineInputAssemblyStateCreateInfo& inputLayout = [this]() -> VkPipelineInputAssemblyStateCreateInfo&
		    {
		        auto inputLayout = GetInputAssemblyState();
				if (inputLayout == nullptr)
				{
					inputLayout								= &allocator.allocate<VkPipelineInputAssemblyStateCreateInfo>();
					inputLayout->sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					inputLayout->pNext						= nullptr;
					inputLayout->flags						= 0;
					inputLayout->topology					= VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
					inputLayout->primitiveRestartEnable		= false;
				}

				return *inputLayout;
		    }();

		stateObjects.push_back({
			    .type = InfoType::InputAssembly,
			    ._ptr = &inputLayout
		    });

		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddDepthStencilState(const DepthStencilState& state)
	{
		return *this;
	}

	VkPolygonMode PolygonMode_FK2VK(EFillMode mode)
	{
        switch (mode)
        {
        case EFillMode::SOLID:
			return VkPolygonMode::VK_POLYGON_MODE_FILL;
		case EFillMode::WIREFRAME:
			return VkPolygonMode::VK_POLYGON_MODE_LINE;
        case EFillMode::POINT:
			return VkPolygonMode::VK_POLYGON_MODE_POINT;
        }
		return VkPolygonMode::VK_POLYGON_MODE_MAX_ENUM;
	}

	VkCullModeFlags CullMode_FK2VK(ECullMode mode)
	{
		switch (mode)
		{
		case ECullMode::BACK:
			return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
		case ECullMode::FRONT:
			return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;
		case ECullMode::NONE:
			return VkCullModeFlagBits::VK_CULL_MODE_NONE;
		}
	}


	IPipelineBuilderImpl& vkPipelineBuilder::AddRasterizerState(const RasterizerState& state)
	{
		VkPipelineRasterizationStateCreateInfo& info = allocator.allocate<VkPipelineRasterizationStateCreateInfo>();

		info.sType						= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		info.pNext						= nullptr;
		info.flags						= 0;
		info.depthClampEnable			= state.depthClipEnable;
		info.rasterizerDiscardEnable	= GetViewportState() == nullptr;
		info.polygonMode				= PolygonMode_FK2VK(state.fill);
        info.cullMode					= CullMode_FK2VK(state.CullMode);
	    info.frontFace					= state.frontCounterClockWise ? VkFrontFace::VK_FRONT_FACE_CLOCKWISE : VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
		info.depthBiasEnable			= state.depthBias > 0.0f;
		info.depthBiasConstantFactor	= state.depthBias;
		info.depthBiasClamp				= state.depthBiasClamp;
		info.depthBiasSlopeFactor		= state.slopeScaledDepthBias;
		info.lineWidth					= 1.0f;

	    stateObjects.push_back({
			    .type = InfoType::RasterizationState,
			    ._ptr = &info
		    });

		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddRenderTargetState(const RenderTargetState& state)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddDepthStencilFormat(const DeviceFormat format)
	{
		return *this;
	}

	IPipelineBuilderImpl& vkPipelineBuilder::AddBlendState(const BlendState& state)
	{
		return *this;
	}

	LoadPipelineStateRes vkPipelineBuilder::Build(IRenderSystem& renderSystem, iAllocator& tempAllocator)
	{
		auto& vkRS = (vkRenderSystem&)renderSystem;

		struct UniformBuffer{};
		struct ShaderResource{};
		struct UnorderedAccess{};
		struct PushConstants{};

		struct Descriptor
		{
			VkDescriptorType	type;
			uint16_t			binding;
			uint16_t			set;
		};

		Vector<VkDescriptorSetLayout>	descriptorLayout	{ vkRS.allocator };
		Vector<VkPushConstantRange>		pushConstantRanges	{ tempAllocator };
		Vector<Descriptor>				descriptors			{ tempAllocator };

		uint32_t pushConstantsSize = 0;

		for (auto& shader : shaders)
		{
			spirv_cross::Compiler compiler((uint32_t*)shader.buffer, shader.bufferSize / 4);
			auto shaderResources = compiler.get_shader_resources();

			for (auto input : shaderResources.stage_inputs)
			{
				int x = 0;
			}

			for (auto constant : shaderResources.uniform_buffers)
			{
				auto location	= compiler.get_decoration(constant.id, spv::DecorationLocation);
				auto offset		= compiler.get_decoration(constant.id, spv::DecorationOffset);
				auto binding	= compiler.get_decoration(constant.id, spv::DecorationBinding);
				auto set		= compiler.get_decoration(constant.id, spv::DecorationDescriptorSet);

				auto ranges = compiler.get_active_buffer_ranges(constant.id);

				if (descriptorLayout.size() < (set + 1))
				{
					descriptors.resize(set + 1);
					descriptors[set].type		= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptors[set].binding	= (uint16_t)binding;
					descriptors[set].set		= (uint16_t)set;
				}
			}

			for (auto pushConstants : shaderResources.push_constant_buffers)
			{
				auto ranges = compiler.get_active_buffer_ranges(pushConstants.id);
				for (spirv_cross::BufferRange r : ranges)
				{
					auto i = r.index;
					auto o = r.offset;
					auto s = r.range;

					pushConstantsSize = Max(pushConstantsSize, r.range + r.offset);

					//VkPushConstantRange constantRange{
					//    .stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL,
					//    .offset		= (uint32_t)r.offset,
					//    .size		= (uint32_t)r.range,
				    //};
					//
					//pushConstantRanges.push_back(constantRange);
				}

			}

			for (auto image : shaderResources.sampled_images)
			{
				auto binding	= compiler.get_decoration(image.id, spv::DecorationBinding);
				auto set		= compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
				descriptors.push_back(Descriptor{
					.type		= VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				    .binding	= (uint16_t)binding,
				    .set		= (uint16_t)set,
				});
			}
		}

		std::ranges::sort(descriptors,
			[](Descriptor& lhs, Descriptor& rhs)
			{
				uint64_t lhsSortingField = (uint64_t(lhs.set) << 32) | uint64_t(lhs.binding);
				uint64_t rhsSortingField = (uint64_t(rhs.set) << 32) | uint64_t(rhs.binding);

				return (lhsSortingField < rhsSortingField);
			});

		VkSampler* samplers = nullptr;

		uint32_t setCount = 0;
		for (auto& descriptor : descriptors)
		{
			if (setCount < (descriptor.set + 1))
				setCount++;
		}

		for (size_t i = 0; i < setCount; i++)
		{
			auto begin	= std::lower_bound(descriptors.begin(), descriptors.end(), i, [](const Descriptor& lhs, const auto& v) { return lhs.set < v; });
			auto end	= std::upper_bound(descriptors.begin(), descriptors.end(), i, [](const auto& v, const Descriptor& rhs) { return v < rhs.set; });

			Vector<VkDescriptorSetLayoutBinding> bindings{ allocator };
			auto span = std::span(begin, end);
			for (auto& desc : span)
			{
			    bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding			= desc.set, 
					.descriptorType		= desc.type,
					.descriptorCount	= 1,
					.stageFlags			= 0, //VkShaderStageFlagBits;
					.pImmutableSamplers	= samplers
			    });
		    }

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
			    .flags			= 0,//VkDescriptorSetLayoutCreateFlagBits,
				.bindingCount	= (uint32_t)bindings.size(),
				.pBindings		= bindings.data()
			};

			VkDescriptorSetLayout layout;
			if (auto res = vkCreateDescriptorSetLayout(vkRS.device, &createInfo, nullptr, &layout); res != VK_SUCCESS)
				FK_LOG_ERROR("VK: FAILED TO CREATE DESCRIPTOR LAYOUT!");

			descriptorLayout.push_back(layout);
		}

		if (pushConstantsSize > 0)
		    pushConstantRanges.push_back(VkPushConstantRange{
			    .stageFlags = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
	            .offset		= 0,
	            .size		= pushConstantsSize
		    });

		VkPipelineLayoutCreateInfo layoutCreateInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,
			.setLayoutCount			= (uint32_t)descriptorLayout.size(),
			.pSetLayouts			= descriptorLayout.data(),
			.pushConstantRangeCount	= (uint32_t)pushConstantRanges.size(),
			.pPushConstantRanges	= pushConstantRanges.data()
		};

		VkPipelineLayout pipelineLayout;
		if (auto res = vkCreatePipelineLayout(vkRS.device, &layoutCreateInfo, nullptr, &pipelineLayout); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("VK: Failed to create pipeline layout!");
			return {};
		}

		VkGraphicsPipelineCreateInfo createInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,//VkPipelineCreateFlagBits::,
            .stageCount				= shaderStages.size(),
            .pStages				= shaderStages.data(),
			.pVertexInputState		= GetVertexInputState(),
			.pInputAssemblyState	= GetInputAssemblyState(),
			.pTessellationState		= GetTessellationState(),
			.pViewportState			= GetViewportState(),
		    .pRasterizationState	= GetRasterizationState(),
			.pMultisampleState		= GetMultiSampleState(),
			.pDepthStencilState		= GetDepthStencilState(),
			.pColorBlendState		= GetBlendState(),
			.pDynamicState			= GetDynamicState(),
			.layout					= pipelineLayout,
			.renderPass				= nullptr,
			.subpass				= 0,
			.basePipelineHandle		= nullptr,
			.basePipelineIndex		= 0
		};

		VkPipeline pipeline;
		if (auto res = vkCreateGraphicsPipelines(vkRS.device, nullptr, 1, &createInfo, nullptr, &pipeline); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("VK: Failed to create graphics pipeline state");
			return {};
		}

	    return {};
	}

	LoadPipelineStateRes vkPipelineBuilder::BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size)
	{
		return {};
	}

	VkPipelineVertexInputStateCreateInfo* vkPipelineBuilder::GetVertexInputState() const
	{
	    for (auto& obj : stateObjects)
	    {
			if (obj.type == InfoType::VertexInput)
				return reinterpret_cast<VkPipelineVertexInputStateCreateInfo*>(obj._ptr);
	    }

		return nullptr;
	}

	VkPipelineInputAssemblyStateCreateInfo* vkPipelineBuilder::GetInputAssemblyState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::InputAssembly)
				return reinterpret_cast<VkPipelineInputAssemblyStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineTessellationStateCreateInfo* vkPipelineBuilder::GetTessellationState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::Tessellation)
				return reinterpret_cast<VkPipelineTessellationStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineViewportStateCreateInfo* vkPipelineBuilder::GetViewportState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::Viewport)
				return reinterpret_cast<VkPipelineViewportStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineRasterizationStateCreateInfo* vkPipelineBuilder::GetRasterizationState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::RasterizationState)
				return reinterpret_cast<VkPipelineRasterizationStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineMultisampleStateCreateInfo* vkPipelineBuilder::GetMultiSampleState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::MultiSample)
				return reinterpret_cast<VkPipelineMultisampleStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineDepthStencilStateCreateInfo* vkPipelineBuilder::GetDepthStencilState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::DepthStencil)
				return reinterpret_cast<VkPipelineDepthStencilStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineColorBlendStateCreateInfo* vkPipelineBuilder::GetBlendState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::ColorBlend)
				return reinterpret_cast<VkPipelineColorBlendStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}

	VkPipelineDynamicStateCreateInfo* vkPipelineBuilder::GetDynamicState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::Dynamic)
				return reinterpret_cast<VkPipelineDynamicStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}
}
