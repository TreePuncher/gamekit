#include "vkPipelineBuilder.hpp"
#include "vkRenderSystem.hpp"
#include <Assets.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <format>

#ifdef WIN32
#include "Unknwnbase.h"
#endif

#ifndef ANDROID
#include <directx-dxc/dxcapi.h>
#endif

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


	IPipelineBuilder& vkPipelineBuilder::AddRootSignature(const IPipelineInterface* rootSig)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/

	
	IPipelineBuilder& vkPipelineBuilder::AddShaderLibrary(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto shader = vkRS.LoadShader(entryPoint, "cs_6_6", file, options);
		shaders.push_back({ .entryPoint = entryPoint, .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT, .shader = std::move(shader) });

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddComputeShader(GUID_t guid)
	{
		LoadShaderAsset(guid, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddComputeShader(const char* assetID)
	{
		LoadShaderAsset(assetID, VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT);

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddWorkGraph(const WorkGraph_Desc& desc)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto shader = vkRS.LoadShader(entryPoint, "vs_6_6", file, options);

		AddInputLayout({});
		AddInputTopology({});

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddVertexShader(GUID_t guid)
	{
		LoadShaderAsset(guid, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);

		AddInputLayout({});
		AddInputTopology({});

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddVertexShader(const char* assetID)
	{
		LoadShaderAsset(assetID, VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);

		AddInputLayout({});
		AddInputTopology({});

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddDomainShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddDomainShader(const char* assetID)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddHullShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddHullShader(const char* assetID)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddGeometryShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddGeometryShader(const char* assetID)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddAmplificationShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddAmplificationShader(const char* assetID)
	{
		return *this;
	}

	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddMeshShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddMeshShader(const char* assetID)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto shader = vkRS.LoadShader(entryPoint, "ps_6_6", file, options);
		shaders.push_back({ .entryPoint = entryPoint, .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, .shader = std::move(shader) });

		AddRenderTargetState({});
		AddBlendState({});
		AddRasterizerState({});

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddPixelShader(GUID_t guid)
	{
		LoadShaderAsset(guid, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

		AddRenderTargetState({});
		AddBlendState({});
		AddRasterizerState({});
		CreateViewportState();

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddPixelShader(const char* entryPoint, const Shader& shader)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		shaders.push_back({ .entryPoint = entryPoint, .stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT, .shader = shader });

		AddRenderTargetState({});
		AddBlendState({});
		AddRasterizerState({});
		CreateViewportState();

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddPixelShader(const char* assetID)
	{
		LoadShaderAsset(assetID, VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);

		AddRenderTargetState({});
		AddBlendState({});
		AddRasterizerState({});
		CreateViewportState();

		auto rasterizerState = GetRasterizationState();
		rasterizerState->rasterizerDiscardEnable = false;
		return *this;
	}


	/************************************************************************************************/

	

	void vkPipelineBuilder::LoadShaderAsset(GUID_t assetUID, VkShaderStageFlagBits shaderStage)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto shaderAsset = FlexKit::LoadGameAsset(assetUID);
		ShaderResourceBlob* asset_ptr = (ShaderResourceBlob*)FlexKit::GetAsset(shaderAsset);

		if (asset_ptr->header.Type != EResourceType::EResource_Shader)
			FK_LOG_ERROR("VK: Failed to load shader resource!: assetUID: {}", assetUID);

		auto spirv = asset_ptr->GetByteCode();
		auto spirvSize = asset_ptr->GetByteCodeSize();

		Shader shader{ spirv, spirvSize, vkRS.allocator };
		shaders.push_back({ .entryPoint = nullptr, .stage = shaderStage, .shader = std::move(shader) });
	}


	void vkPipelineBuilder::LoadShaderAsset(const char* assetID, VkShaderStageFlagBits shaderStage)
	{
		auto& vkRS = (vkRenderSystem&)vkRenderSystem::GetInstance();

		auto shaderAsset = FlexKit::LoadGameAsset(assetID);
		ShaderResourceBlob* asset_ptr = (ShaderResourceBlob*)FlexKit::GetAsset(shaderAsset);

		if (asset_ptr->header.Type != EResourceType::EResource_Shader)
			FK_LOG_ERROR("VK: Failed to load shader resource!: assetID: {}", assetID);

		auto spirv = asset_ptr->GetByteCode();
		auto spirvSize = asset_ptr->GetByteCodeSize();

		Shader shader{ spirv, spirvSize, vkRS.allocator };
		shaders.push_back({ .entryPoint = nullptr, .stage = shaderStage, .shader = std::move(shader) });
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::SetDebugName(const char* name)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddInputLayout(const InputLayoutState& state)
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


	IPipelineBuilder& vkPipelineBuilder::AddInputTopology(const ETopology topology)
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


	IPipelineBuilder& vkPipelineBuilder::AddDepthStencilState(const DepthStencilState& state)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddRasterizerState(const RasterizerState& state)
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


	IPipelineBuilder& vkPipelineBuilder::AddRenderTargetState(const RenderTargetState& state)
	{
		VkPipelineRenderingCreateInfoKHR& renderTarget =
			[this]() -> VkPipelineRenderingCreateInfoKHR&
			{
				VkPipelineRenderingCreateInfoKHR* renderTarget = GetRenderTargetState();
				if (renderTarget == nullptr)
				{
					renderTarget = &allocator.allocate<VkPipelineRenderingCreateInfoKHR>(VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR);
					renderTarget->pColorAttachmentFormats	= (VkFormat*)allocator.malloc(sizeof(VkFormat[8]));
					renderTarget->pNext						= nullptr;
					renderTarget->colorAttachmentCount		= 0;
					renderTarget->depthAttachmentFormat		= VK_FORMAT_UNDEFINED;
					renderTarget->stencilAttachmentFormat	= VK_FORMAT_UNDEFINED;
					renderTarget->viewMask					= 0;

					stateObjects.push_back({
						.type = InfoType::RenderTarget,
						._ptr = renderTarget
						});
				}

				return *renderTarget;
			}();

		VkFormat* formats = (VkFormat*)renderTarget.pColorAttachmentFormats;

		for(size_t i = 0; i < state.targetCount; i++)
			formats[i] = FormatToVK(state.targetFormats[i]);

		renderTarget.colorAttachmentCount = state.targetCount;

		auto blendState = GetBlendState();
		if (!blendState)
		{
			AddBlendState();
			blendState = GetBlendState();
		}
		blendState->attachmentCount = state.targetCount;

		auto rasterizerState = GetRasterizationState();
		if (!rasterizerState)
		{
			AddRasterizerState();
			rasterizerState = GetRasterizationState();
		}

		rasterizerState->rasterizerDiscardEnable = false;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddDepthStencilFormat(const DeviceFormat format)
	{
		auto renderTargetState = GetRenderTargetState();
		if (!renderTargetState)
		{
			AddRenderTargetState();
			renderTargetState = GetRenderTargetState();
		}

		renderTargetState->depthAttachmentFormat = FormatToVK(format);
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& vkPipelineBuilder::AddBlendState(const BlendState& state)
	{
		VkPipelineColorBlendStateCreateInfo& blendState = [&, this]() -> VkPipelineColorBlendStateCreateInfo&
			{
				auto blendState = GetBlendState();
				if (blendState == nullptr)
				{
					VkPipelineColorBlendAttachmentState* attachmentBlendStates = (VkPipelineColorBlendAttachmentState*)allocator.malloc(sizeof(VkPipelineColorBlendAttachmentState[8]));
					blendState = &allocator.allocate<VkPipelineColorBlendStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
					blendState->pNext			= nullptr;
					blendState->flags			= 0; //VkPipelineColorBlendStateCreateFlagBits;
					blendState->logicOpEnable	= false;
					blendState->attachmentCount = 0;
					blendState->pAttachments	= attachmentBlendStates;

					blendState->blendConstants[0] = 1;
					blendState->blendConstants[1] = 1;
					blendState->blendConstants[2] = 1;
					blendState->blendConstants[3] = 1;


					stateObjects.push_back({
						.type = InfoType::ColorBlend,
						._ptr = blendState
						});
				}

				return *blendState;
			}();

		auto attachmentBlendStates = (VkPipelineColorBlendAttachmentState*)blendState.pAttachments;
		for (size_t i = 0; i < 8; i++)
		{
			attachmentBlendStates[i].blendEnable			= state.renderTarget[i].blendEnable;
			attachmentBlendStates[i].srcColorBlendFactor	= BlendFactorToVk(state.renderTarget[i].srcBlend);
			attachmentBlendStates[i].dstColorBlendFactor	= BlendFactorToVk(state.renderTarget[i].dstBlend);
			attachmentBlendStates[i].colorBlendOp			= BlendOpToVk(state.renderTarget[i].blendOp);
			attachmentBlendStates[i].srcAlphaBlendFactor	= BlendFactorToVk(state.renderTarget[i].srcBlendAlpha);
			attachmentBlendStates[i].dstAlphaBlendFactor	= BlendFactorToVk(state.renderTarget[i].dstBlendAlpha);
			attachmentBlendStates[i].alphaBlendOp			= BlendOpToVk(state.renderTarget[i].blendOpAlpha);
			attachmentBlendStates[i].colorWriteMask			= (VkColorComponentFlags)state.renderTarget[i].renderTargetWriteMask;
		}


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

		Vector<VkDescriptorSetLayout>	descriptorLayouts{ vkRS.allocator };
		Vector<PushDescriptor>			pushDescriptorLayout{ vkRS.allocator };
		Vector<VkPushConstantRange>		pushConstantRanges{ tempAllocator };
		Vector<Descriptor>				descriptors{ tempAllocator };

		uint32_t pushConstantFlags = 0;
		uint32_t pushConstantsSize = 0;

		for (auto&& [idx, shaderRec] : enumerate(shaders))
		{
			spirv_cross::Compiler compiler((uint32_t*)shaderRec.shader.buffer, shaderRec.shader.bufferSize / 4);
			auto shaderResources = compiler.get_shader_resources();

			if (shaderRec.entryPoint == nullptr)
			{
				auto entryPoints = compiler.get_entry_points_and_stages();
				const std::string name = entryPoints[0].name;

				char* nameStr = (char*)tempAllocator.malloc(name.size() + 1);
				strncpy(nameStr, name.data(), name.size() + 1);

				shaderRec.entryPoint = nameStr;
			}

			shaderRec.shader.ForEachDescriptorSetAttribute(
				[&](const ShaderAttributeDescriptorTable& descriptorTable)
				{
					uint32_t bindingCounter = 0;
					for (const ShaderAttributeDescriptorTableEntry& entry : descriptorTable.entries)
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
										case ShaderResourceType::SRVBuffer:
											return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
										case ShaderResourceType::SRVTexture:
											return VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
										case ShaderResourceType::CBV:
											return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
										case ShaderResourceType::UAVBuffer:
											return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
										case ShaderResourceType::UAVTexture:
											return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
										}
									};


								descriptors.push_back(
									Descriptor
									{
										.type = getType(),
										.binding = (uint16_t)bindingCounter,
										.set = (uint16_t)descriptorTable.set,
										.stages = (uint32_t)shaderRec.stage
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
					const ShaderAttributeResource& attribute = res.value();

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
								.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.binding = (uint16_t)attribute.binding,
								.set = (uint16_t)attribute.set,
								.stages = attribute.pipelineStage | shaderRec.stage
							});
					else
						descriptor->stages |= shaderRec.stage;
				}
				else if (auto res = shaderRec.shader.FindCBVPushAttribute(id); res)
				{
					const ShaderAttributeConstantValues& attribute = res.value();

					if (auto res = std::ranges::find_if(
						pushDescriptorLayout,
						[&](PushDescriptor& desc) -> bool
						{
							return (attribute.binding == binding);
						}); res == std::end(pushDescriptorLayout))
					{
						pushDescriptorLayout.push_back(PushDescriptor{
								.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.binding = (uint16_t)binding,
								.set = (uint16_t)set,
								.stages = (uint32_t)shaders[idx].stage,
								.shaderRecords = { vkRS.allocator }
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
						.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
						.binding = (uint16_t)binding,
						.set = (uint16_t)set,
						.stages = (uint32_t)shaders[idx].stage,
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
						.type = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLER,
						.binding = (uint16_t)binding,
						.set = (uint16_t)set,
						.stages = (uint32_t)shaders[idx].stage,
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
			auto begin = std::lower_bound(descriptors.begin(), descriptors.end(), i, [](const Descriptor& lhs, const auto& v) { return lhs.set < v; });
			auto end = std::upper_bound(descriptors.begin(), descriptors.end(), i, [](const auto& v, const Descriptor& rhs) { return v < rhs.set; });

			Vector<VkDescriptorSetLayoutBinding> bindings{ allocator };
			auto span = std::span(begin, end);
			bindings.reserve(span.size());
			for (auto& desc : span)
			{
				bindings.push_back(VkDescriptorSetLayoutBinding{
					.binding = desc.binding,
					.descriptorType = desc.type,
					.descriptorCount = 1,
					.stageFlags = desc.stages,
					.pImmutableSamplers = samplers
					});
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				.bindingCount = (uint32_t)bindings.size(),
				.pBindings = bindings.data()
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
						.binding = pushDesc.binding,
						.descriptorType = pushDesc.type,
						.descriptorCount = 1,
						.stageFlags = pushDesc.stages,
						.pImmutableSamplers = nullptr
					});
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
				.bindingCount = (uint32_t)bindings.size(),
				.pBindings = bindings.data()
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
					.offset = 0,
					.size = pushConstantsSize
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
				.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = shaderRec.stage,
				.module = shaderModule,
				.pName = shaderRec.entryPoint,
				.pSpecializationInfo = nullptr
			};

			shaderStages.push_back(stage);
		}

		VkPipelineLayoutCreateInfo layoutCreateInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = (uint32_t)descriptorLayouts.size(),
			.pSetLayouts = descriptorLayouts.data(),
			.pushConstantRangeCount = (uint32_t)pushConstantRanges.size(),
			.pPushConstantRanges = pushConstantRanges.data()
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
			.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates,
		};

		VkGraphicsPipelineCreateInfo createInfo{
			.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = GetRenderTargetState(),
			.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
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


	VkPipelineViewportStateCreateInfo* vkPipelineBuilder::CreateViewportState()
	{
		auto viewportState = GetViewportState();

		if (viewportState)
			return viewportState;

		viewportState =
			[this]() -> VkPipelineViewportStateCreateInfo*
			{
				auto info = &allocator.allocate<VkPipelineViewportStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
				info->pNext			= nullptr;
				info->flags			= 0;
				info->viewportCount	= 0;
				info->pViewports	= 0;
				info->scissorCount	= 0;
				info->pScissors		= 0;

				stateObjects.push_back({
						.type = InfoType::Viewport,
						._ptr = info
					});

				return info;
			}();

		return viewportState;
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


	VkPipelineRenderingCreateInfoKHR* vkPipelineBuilder::GetRenderTargetState() const
	{
		for (auto& obj : stateObjects)
		{
			if (obj.type == InfoType::RenderTarget)
				return reinterpret_cast<VkPipelineRenderingCreateInfoKHR*>(obj._ptr);
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
