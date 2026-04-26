#include "vkPipelineBuilder.hpp"
#include "vkRenderSystem.hpp"
#include <spirv_cross/spirv_cross.hpp>

#ifdef WIN32
#include "Unknwnbase.h"
#endif

#include <directx-dxc/dxcapi.h>
#include <spirv/unified1/spirv.hpp>

namespace VK_internal
{	/************************************************************************************************/


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


	/************************************************************************************************/


	VkCullModeFlags CullMode_FK2VK(ECullMode mode)
	{
		// Cullmode is reversed since VK has top pointing down on Y flipping windings relative to DX12
		switch (mode)
		{
		case ECullMode::BACK:
			return VkCullModeFlagBits::VK_CULL_MODE_FRONT_BIT;
		case ECullMode::FRONT:
			return VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
		case ECullMode::NONE:
			return VkCullModeFlagBits::VK_CULL_MODE_NONE;
		}
	}


	/************************************************************************************************/


	vkPipelineBuilder::vkPipelineBuilder(vkRenderSystem& system, iAllocator& IN_allocator) :
		allocator	{ IN_allocator },
		stateObjects{ IN_allocator },
		shaderStages{ IN_allocator },
		shaders		{ IN_allocator } {}


	/************************************************************************************************/


	vkPipelineBuilder::~vkPipelineBuilder()
	{

	}


	/************************************************************************************************/


	void vkPipelineBuilder::Release()
	{

	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddRootSignature(const IPipelineInterface* rootSig)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/

	IPipelineBuilderImpl& vkPipelineBuilder::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddWorkGraph(const WorkGraph_Desc& desc)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();
		auto shader = vkRS.LoadShader(entryPoint, "vs_6_6", file, options);
		auto	idx = shaders.push_back({ .entryPoint = entryPoint, .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT, .shader = std::move(shader) });

		AddInputLayout({});
		AddInputTopology({});

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto	shader = vkRS.LoadShader(entryPoint, "ps_6_6", file, options);
		auto	idx = shaders.push_back({ .entryPoint = entryPoint, .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, .shader = std::move(shader) });

		AddRenderTargetState({});
		AddRasterizerState({});

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const Shader& shader)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		const auto idx = shaders.push_back({ .entryPoint = entryPoint, .shader = shader });

		AddRenderTargetState({});
		AddRasterizerState({});

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::SetDebugName(const char* name)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddInputLayout(const InputLayoutState& state)
	{
		VertexStateObject& inputLayout = [this]() -> VertexStateObject&
			{
				VertexStateObject* inputLayout = GetVertexInputState();
				if (inputLayout == nullptr)
				{
					inputLayout = &allocator.allocate<VertexStateObject>();
					inputLayout->info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					inputLayout->info.pNext = nullptr;
					inputLayout->info.flags = 0;
					inputLayout->info.pVertexAttributeDescriptions = nullptr;
					inputLayout->info.pVertexBindingDescriptions = nullptr;
					inputLayout->info.vertexAttributeDescriptionCount = 0;
					inputLayout->info.vertexBindingDescriptionCount = 0;
					inputLayout->inputBindings.Allocator = allocator;
					inputLayout->inputAttributes.Allocator = allocator;

					stateObjects.push_back({
						.type = InfoType::VertexInput,
						._ptr = inputLayout
						});
				}

				return *inputLayout;
			}();

		Vector<uint32_t> slotSizes{ allocator };

		for (auto [idx, input] : enumerate(state.inputs))
		{
			if (input.format == DeviceFormat::UNKNOWN)
				continue;

			inputLayout.inputAttributes.push_back(VkVertexInputAttributeDescription{
				.location = (uint32_t)idx,
				.binding = input.slot,
				.format = FormatToVK(input.format),
				.offset = input.alignedByteOffset,
				});

			if (slotSizes.size() < (input.slot + 1))
				slotSizes.resize(input.slot + 1);

			slotSizes[input.slot] += GetFormatElementSize(FormatToVK(input.format));
		}

		for (auto [idx, size] : enumerate(slotSizes))
		{
			if (size > 0)
				inputLayout.inputBindings.push_back(VkVertexInputBindingDescription{
						.binding = (uint32_t)idx,
						.stride = size,
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
					});
		}

		inputLayout.info.vertexBindingDescriptionCount = inputLayout.inputBindings.size();
		inputLayout.info.pVertexBindingDescriptions = inputLayout.inputBindings.data();
		inputLayout.info.vertexAttributeDescriptionCount = inputLayout.inputAttributes.size();
		inputLayout.info.pVertexAttributeDescriptions = inputLayout.inputAttributes.data();

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddInputTopology(const ETopology topology)
	{
		VkPipelineInputAssemblyStateCreateInfo& inputLayout = [this]() -> VkPipelineInputAssemblyStateCreateInfo&
			{
				auto inputLayout = GetInputAssemblyState();
				if (inputLayout == nullptr)
				{
					inputLayout = &allocator.allocate<VkPipelineInputAssemblyStateCreateInfo>();
					inputLayout->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
					inputLayout->pNext = nullptr;
					inputLayout->flags = 0;
					inputLayout->topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
					inputLayout->primitiveRestartEnable = false;

					stateObjects.push_back({
						.type = InfoType::InputAssembly,
						._ptr = inputLayout
						});
				}

				return *inputLayout;
			}();

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddDepthStencilState(const DepthStencilState& state)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddRasterizerState(const RasterizerState& state)
	{
		VkPipelineRasterizationStateCreateInfo* info = [this]()
			{
				auto info = GetRasterizationState();
				if (info == nullptr)
				{
					info = &allocator.allocate<VkPipelineRasterizationStateCreateInfo>();
					info->sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
					info->pNext = nullptr;
					info->flags = 0;

					stateObjects.push_back({
						.type = InfoType::RasterizationState,
						._ptr = info
						});
				}

				return info;
			}();

		VkPipelineMultisampleStateCreateInfo* multiSampleStateInfo = [this]()
			{
				auto info = GetMultiSampleState();
				if (info == nullptr)
				{
					info = &allocator.allocate<VkPipelineMultisampleStateCreateInfo>();
					info->sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					info->pNext = nullptr;
					info->flags = 0;

					stateObjects.push_back({
							.type = InfoType::MultiSample,
							._ptr = info
						});
				}

				return info;
			}();

		info->depthClampEnable			= state.depthClipEnable;
		info->rasterizerDiscardEnable	= GetViewportState() == nullptr;
		info->polygonMode				= PolygonMode_FK2VK(state.fill);
		info->cullMode					= CullMode_FK2VK(state.CullMode);
		info->frontFace					= state.frontCounterClockWise ? VkFrontFace::VK_FRONT_FACE_CLOCKWISE : VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE;
		info->depthBiasEnable			= state.depthBias > 0.0f;
		info->depthBiasConstantFactor	= state.depthBias;
		info->depthBiasClamp			= state.depthBiasClamp;
		info->depthBiasSlopeFactor		= state.slopeScaledDepthBias;
		info->lineWidth					= 1.0f;

		multiSampleStateInfo->rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
		multiSampleStateInfo->alphaToCoverageEnable = false;
		multiSampleStateInfo->alphaToOneEnable = false;
		multiSampleStateInfo->sampleShadingEnable = false;
		multiSampleStateInfo->minSampleShading = 0.0f;
		multiSampleStateInfo->pSampleMask = nullptr;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddRenderTargetState(const RenderTargetState& state)
	{
		VkPipelineViewportStateCreateInfo& viewport = [this]() -> VkPipelineViewportStateCreateInfo&
			{
				auto viewport = GetViewportState();
				if (viewport == nullptr)
				{
					viewport = &allocator.allocate<VkPipelineViewportStateCreateInfo>();
					viewport->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					viewport->pNext = nullptr;
					viewport->flags = 0;
					viewport->viewportCount = 0;
					viewport->pViewports = nullptr;
					viewport->scissorCount = 0;
					viewport->pScissors = nullptr;

					stateObjects.push_back({
						.type = InfoType::Viewport,
						._ptr = viewport
						});
				}

				return *viewport;
			}();



		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddDepthStencilFormat(const DeviceFormat format)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilderImpl& vkPipelineBuilder::AddBlendState(const BlendState& state)
	{
		return *this;
	}


	/************************************************************************************************/


	struct DxilContainerRootDescriptor1 {
		uint32_t ShaderRegister;
		uint32_t RegisterSpace;
		uint32_t Flags;
	};

	struct DxilContainerDescriptorRange {
		uint32_t RangeType;
		uint32_t NumDescriptors;
		uint32_t BaseShaderRegister;
		uint32_t RegisterSpace;
		uint32_t OffsetInDescriptorsFromTableStart;
	};

	struct DxilContainerDescriptorRange1 {
		uint32_t RangeType;
		uint32_t NumDescriptors;
		uint32_t BaseShaderRegister;
		uint32_t RegisterSpace;
		uint32_t Flags;
		uint32_t OffsetInDescriptorsFromTableStart;
	};

	struct DxilContainerRootDescriptorTable {
		uint32_t NumDescriptorRanges;
		uint32_t DescriptorRangesOffset;
	};

	struct DxilContainerRootParameter {
		uint32_t ParameterType;
		uint32_t ShaderVisibility;
		uint32_t PayloadOffset;
	};

	struct DxilContainerRootSignatureDesc {
		uint32_t Version;
		uint32_t NumParameters;
		uint32_t RootParametersOffset;
		uint32_t NumStaticSamplers;
		uint32_t StaticSamplersOffset;
		uint32_t Flags;
	};


	/************************************************************************************************/


	LoadPipelineStateRes vkPipelineBuilder::Build(IRenderSystem& renderSystem, iAllocator& tempAllocator)
	{
		auto& vkRS = (vkRenderSystem&)renderSystem;

		struct UniformBuffer {};
		struct ShaderResource {};
		struct UnorderedAccess {};
		struct PushConstants {};

		struct Descriptor
		{
			VkDescriptorType	type;
			uint16_t			binding;
			uint16_t			set;
			uint32_t			stages = 0;
		};

		struct ShaderRecord
		{
			spirv_cross::ID id;
			uint32_t		shader;
		};

		struct PushDescriptor
		{
			VkDescriptorType	type;
			uint16_t			binding;
			uint16_t			set;
			uint32_t			stages = 0;

			Vector<ShaderRecord, 3>		shaderRecords;
		};

		Vector<VkDescriptorSetLayout>	descriptorLayouts	{ vkRS.allocator };
		Vector<PushDescriptor>			pushDescriptorLayout{ vkRS.allocator };
		Vector<VkPushConstantRange>		pushConstantRanges	{ tempAllocator };
		Vector<Descriptor>				descriptors			{ tempAllocator };

		uint32_t pushConstantFlags = 0;
		uint32_t pushConstantsSize = 0;

		for (auto&& [idx, shaderRec] : enumerate(shaders))
		{
			spirv_cross::Compiler compiler((uint32_t*)shaderRec.shader.buffer, shaderRec.shader.bufferSize / 4);
			auto shaderResources = compiler.get_shader_resources();

			shaderRec.shader.ForEachDescriptorSetAttribute(
				[&](const ShaderAttributeDescriptorTable& descriptorTable)
			    {
					uint32_t bindingCounter = 0;
					for (const DescriptorTableEntry& entry : descriptorTable.entries)
					{
						std::span spanDescriptors{ descriptors.begin(), descriptors.end() };

						for (size_t i = 0; i < entry.num; i++)
						{
							if (auto res = std::find_if(
								spanDescriptors.rbegin(),
								spanDescriptors.rend(),
								[&](const Descriptor& desc) -> bool
								{
									return (desc.set == descriptorTable.set && desc.binding == bindingCounter);
								}); res != std::rend(spanDescriptors))
							{
								res->stages |= shaderRec.stage;
							}
							else
							{
								const auto getType = [&]() -> VkDescriptorType
								{
									switch (entry.type)
									{
									case DescriptorType::SRVBuffer:
										return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
									case DescriptorType::SRVTexture:
										return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
									case DescriptorType::CBV:
										return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
									case DescriptorType::UAVBuffer:
										return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
									case DescriptorType::UAVTexture:
										return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
									}
								};

								
								descriptors.push_back(
									Descriptor
									{
										.type		= getType(),
										.binding	= (uint16_t)bindingCounter,
										.set		= (uint16_t)descriptorTable.set,
										.stages		= (uint32_t)shaderRec.stage
									});
							}

							bindingCounter++;
						}
						
					}
			    });

			for (auto constant : shaderResources.uniform_buffers)
			{
				auto binding = compiler.get_decoration(constant.id, spv::DecorationBinding);
				auto set = compiler.get_decoration(constant.id, spv::DecorationDescriptorSet);
				std::string_view id{ constant.name.data() + 5, constant.name.size() - 5 };

				auto ranges = compiler.get_active_buffer_ranges(constant.id);
				if (auto res = shaderRec.shader.FindCBVAttribute(id); res)
				{
					const ShaderAttributeCBV& attribute = res.value();

					auto descriptor =
						std::find_if(
							descriptors.begin(), descriptors.end(),
							[&](const Descriptor& desc)
							{
								return (desc.set == attribute.set && desc.binding == attribute.binding);
							});

					if (descriptor == descriptors.end())
						descriptors.push_back(
							Descriptor
							{
								.type		= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.binding	= (uint16_t)attribute.binding,
								.set		= (uint16_t)attribute.set,
								.stages		= attribute.pipelineStage | shaderRec.stage
							});
					else
						descriptor->stages |= shaderRec.stage;
				}
				else if (auto res = shaderRec.shader.FindCBVPushAttribute(id); res)
				{
					const ShaderAttributePushCBV& attribute = res.value();

					if (auto res = std::ranges::find_if(
						pushDescriptorLayout,
						[&](PushDescriptor& desc) -> bool
						{
							return (attribute.binding == binding);
						}); res == std::end(pushDescriptorLayout))
					{
						pushDescriptorLayout.push_back(PushDescriptor{
								.type			= VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.binding		= (uint16_t)binding,
								.set			= (uint16_t)set,
								.stages			= (uint32_t)shaders[idx].stage,
								.shaderRecords	= { vkRS.allocator }
							});

						pushDescriptorLayout.back().shaderRecords.push_back(ShaderRecord{ .id = constant.id, .shader = (uint32_t)idx });
					}
					else
						res->stages |= shaders[idx].stage;
				}
				else
				{
					if (auto res = std::ranges::find_if(
						descriptors,
						[&](Descriptor& desc) -> bool
						{
							return (desc.set == set && desc.binding == binding);
						}); res == std::end(descriptors))
					{
						descriptors.push_back({
							.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							.binding = (uint16_t)binding,
							.set = (uint16_t)set,
							.stages = (uint32_t)shaders[idx].stage,
							});
					}
					else
						res->stages |= shaders[idx].stage;
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
					pushConstantFlags |= shaders[idx].stage;
				}
			}

			for (auto image : shaderResources.separate_images)
			{
				auto binding = compiler.get_decoration(image.id, spv::DecorationBinding);
				auto set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);


				if (auto res = std::ranges::find_if(
					descriptors,
					[&](Descriptor& desc) -> bool
					{
						return (desc.set == set && desc.binding == binding);
					}); res == std::end(descriptors))
				{
					descriptors.push_back({
						.type		= VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
						.binding	= (uint16_t)binding,
						.set		= (uint16_t)set,
						.stages		= (uint32_t)shaders[idx].stage,
						});
				}
				else
					res->stages |= shaders[idx].stage;
			}

			for (auto image : shaderResources.separate_samplers)
			{
				auto binding = compiler.get_decoration(image.id, spv::DecorationBinding);
				auto set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);

				if (auto res = std::ranges::find_if(
					descriptors,
					[&](Descriptor& desc) -> bool
					{
						return (desc.set == set && desc.binding == binding);
					}); res == std::end(descriptors))
				{
					descriptors.push_back({
						.type		= VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER,
						.binding	= (uint16_t)binding,
						.set		= (uint16_t)set,
						.stages		= (uint32_t)shaders[idx].stage,
						});
				}
				else
					res->stages |= shaders[idx].stage;
			}
		}

		std::ranges::sort(descriptors,
			[](Descriptor& lhs, Descriptor& rhs)
			{
				uint64_t lhsSortingField = (uint64_t(lhs.set) << 32) | uint64_t(lhs.binding);
				uint64_t rhsSortingField = (uint64_t(rhs.set) << 32) | uint64_t(rhs.binding);

				return (lhsSortingField < rhsSortingField);
			});

		std::ranges::sort(pushDescriptorLayout,
			[](PushDescriptor& lhs, PushDescriptor& rhs)
			{
				uint64_t lhsSortingField = (uint64_t(lhs.set) << 32) | uint64_t(lhs.binding);
				uint64_t rhsSortingField = (uint64_t(rhs.set) << 32) | uint64_t(rhs.binding);

				return (lhsSortingField < rhsSortingField);
			});


		VkSampler* samplers = nullptr;
		Vector<DescriptorHeapLayout> heapLayouts{ allocator };

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
			bindings.reserve(span.size());
			for (auto& desc : span)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding			= desc.binding,
					.descriptorType		= desc.type,
					.descriptorCount	= 1,
					.stageFlags			= desc.stages,
					.pImmutableSamplers = samplers
					});
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
				.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				.bindingCount	= (uint32_t)bindings.size(),
				.pBindings		= bindings.data()
			};

			VkDescriptorSetLayout layout;
			if (auto res = vkCreateDescriptorSetLayout(vkRS.device, &createInfo, nullptr, &layout); res != VK_SUCCESS)
				FK_LOG_ERROR("VK: FAILED TO CREATE DESCRIPTOR LAYOUT!");

			heapLayouts.emplace_back(allocator);
			heapLayouts.back().deviceLayout = layout;

			descriptorLayouts.push_back(layout);
		}

		uint32_t pushDescriptorSet = setCount;
		Vector<VkDescriptorType>	pushLayoutTypes{ allocator };
		VkDescriptorSetLayout		pushLayout = nullptr;
		uint32_t					pushCount = 0;

		if (pushDescriptorLayout.size())
		{
			setCount++;
			Vector<VkDescriptorSetLayoutBinding> bindings{ allocator };
			bindings.reserve(pushDescriptorLayout.size());

			for (auto& pushDesc : pushDescriptorLayout)
			{
				bindings.push_back(
					VkDescriptorSetLayoutBinding{
					    .binding			= pushDesc.binding,
					    .descriptorType		= pushDesc.type,
					    .descriptorCount	= 1,
					    .stageFlags			= pushDesc.stages,
					    .pImmutableSamplers = nullptr
					});
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
				.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				.bindingCount	= (uint32_t)bindings.size(),
				.pBindings		= bindings.data()
			};

			if (auto res = vkCreateDescriptorSetLayout(vkRS.device, &createInfo, nullptr, &pushLayout); res != VK_SUCCESS)
				FK_LOG_ERROR("VK: FAILED TO CREATE DESCRIPTOR LAYOUT!");

			descriptorLayouts.push_back(pushLayout);

			for (auto& desc : pushDescriptorLayout)
			{
				for (auto& sr : desc.shaderRecords)
				{
					auto& shaderRec = shaders[sr.shader];
					spirv_cross::Compiler compiler((uint32_t*)shaderRec.shader.buffer, shaderRec.shader.bufferSize / 4);

					compiler.set_decoration(sr.id, spv::DecorationBinding, pushCount);
					compiler.set_decoration(sr.id, spv::DecorationDescriptorSet, pushDescriptorSet);
					pushCount++;
				}
			}
		}

		if (pushConstantsSize > 0)
			pushConstantRanges.push_back(
			    VkPushConstantRange{
				    .stageFlags = pushConstantFlags,
				    .offset		= 0,
				    .size		= pushConstantsSize
				});


		for (auto& shaderRec : shaders)
		{
			VkShaderModuleCreateInfo moduleCreateInfo{
				.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shaderRec.shader.bufferSize,
				.pCode = (uint32_t*)shaderRec.shader.buffer
			};

			VkShaderModule shaderModule;
			if (auto res = vkCreateShaderModule(vkRS.device, &moduleCreateInfo, nullptr, &shaderModule); res != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader module");
			}

			VkPipelineShaderStageCreateInfo stage{
				.sType		= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext		= nullptr,
				.flags		= 0,
				.stage		= shaderRec.stage,
				.module		= shaderModule,
				.pName		= shaderRec.entryPoint,
				.pSpecializationInfo = nullptr
			};

			shaderStages.push_back(stage);
		}

		VkPipelineLayoutCreateInfo layoutCreateInfo{
			.sType					= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,
			.setLayoutCount			= (uint32_t)descriptorLayouts.size(),
			.pSetLayouts			= descriptorLayouts.data(),
			.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(),
			.pPushConstantRanges	= pushConstantRanges.data()
		};

		VkPipelineLayout pipelineLayout;
		if (auto res = vkCreatePipelineLayout(vkRS.device, &layoutCreateInfo, nullptr, &pipelineLayout); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("VK: Failed to create pipeline layout!");
			return {};
		}


		static const VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
			VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
			VK_DYNAMIC_STATE_RASTERIZATION_SAMPLES_EXT,
			VK_DYNAMIC_STATE_SAMPLE_MASK_EXT,
			VK_DYNAMIC_STATE_ALPHA_TO_COVERAGE_ENABLE_EXT
		};

		static const VkPipelineDynamicStateCreateInfo dynamicInfo{
			.sType				= VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext				= nullptr,
			.flags				= 0,
			.dynamicStateCount	= 2,
			.pDynamicStates		= dynamicStates,
		};

		VkGraphicsPipelineCreateInfo createInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,//VkPipelineCreateFlagBits::,
			.stageCount = shaderStages.size(),
			.pStages = shaderStages.data(),
			.pVertexInputState = [&] { auto res = GetVertexInputState(); return res ? &res->info : nullptr; }(),
			.pInputAssemblyState = GetInputAssemblyState(),
			.pTessellationState = GetTessellationState(),
			.pViewportState = GetViewportState(),
			.pRasterizationState = GetRasterizationState(),
			.pMultisampleState = GetMultiSampleState(),
			.pDepthStencilState = GetDepthStencilState(),
			.pColorBlendState = GetBlendState(),
			.pDynamicState = &dynamicInfo,
			.layout = pipelineLayout,
			.renderPass = nullptr,
			.subpass = 0,
			.basePipelineHandle = nullptr,
			.basePipelineIndex = 0
		};

		VkPipeline pipeline;
		if (auto res = vkCreateGraphicsPipelines(vkRS.device, nullptr, 1, &createInfo, nullptr, &pipeline); res != VK_SUCCESS)
		{
			FK_LOG_ERROR("VK: Failed to create graphics pipeline state");
			return {};
		}

		auto& pipelineInterface = allocator.allocate<vkPipelineInterface>(allocator);
		pipelineInterface.layout				= pipelineLayout;
		pipelineInterface.pushLayout			= pushLayout;
		pipelineInterface.pushSet				= pushDescriptorSet;
		pipelineInterface.pushConstantFlags		= pushConstantRanges.size() ? pushConstantRanges.front().stageFlags : 0;
		pipelineInterface.pushCount				= pushCount;
		pipelineInterface.vkLayouts				= std::move(descriptorLayouts);
		pipelineInterface.heapLayouts			= std::move(heapLayouts);

		return { pipeline, &pipelineInterface };
	}


	/************************************************************************************************/


	LoadPipelineStateRes vkPipelineBuilder::BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size)
	{
		return {};
	}


	/************************************************************************************************/


	VertexStateObject* vkPipelineBuilder::GetVertexInputState() const
	{
	    for (auto& obj : stateObjects)
	    {
			if (obj.type == InfoType::VertexInput)
				return reinterpret_cast<VertexStateObject*>(obj._ptr);
	    }

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineInputAssemblyStateCreateInfo* vkPipelineBuilder::GetInputAssemblyState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::InputAssembly)
				return reinterpret_cast<VkPipelineInputAssemblyStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineTessellationStateCreateInfo* vkPipelineBuilder::GetTessellationState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::Tessellation)
				return reinterpret_cast<VkPipelineTessellationStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineViewportStateCreateInfo* vkPipelineBuilder::GetViewportState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::Viewport)
				return reinterpret_cast<VkPipelineViewportStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


    VkPipelineRasterizationStateCreateInfo* vkPipelineBuilder::GetRasterizationState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::RasterizationState)
				return reinterpret_cast<VkPipelineRasterizationStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineMultisampleStateCreateInfo* vkPipelineBuilder::GetMultiSampleState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::MultiSample)
				return reinterpret_cast<VkPipelineMultisampleStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineDepthStencilStateCreateInfo* vkPipelineBuilder::GetDepthStencilState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::DepthStencil)
				return reinterpret_cast<VkPipelineDepthStencilStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}


	/************************************************************************************************/


	VkPipelineColorBlendStateCreateInfo* vkPipelineBuilder::GetBlendState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::ColorBlend)
				return reinterpret_cast<VkPipelineColorBlendStateCreateInfo*>(obj._ptr);
		}

		return nullptr;
	}
}
