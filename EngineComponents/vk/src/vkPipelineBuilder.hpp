#include <RenderSystemInterface.hpp>

#include "Serialization.hpp"
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

	struct vkRenderSystem;

    struct vkPipelineBuilder : IPipelineBuilderImpl
    {
		vkPipelineBuilder(vkRenderSystem&, iAllocator&);
		~vkPipelineBuilder() override;

		void Release() override;

		IPipelineBuilderImpl& AddRootSignature		(const IPipelineInterface* rootSig) override;

		IPipelineBuilderImpl& AddShaderLibrary		(const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddComputeShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddWorkGraph			(const WorkGraph_Desc& desc = {}) override;

		IPipelineBuilderImpl& AddVertexShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddDomainShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddHullShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddGeometryShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;

		IPipelineBuilderImpl& AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilderImpl& AddMeshShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;

		IPipelineBuilderImpl& AddPixelShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
        IPipelineBuilderImpl& AddPixelShader		(const char* entryPoint, const Shader&) override;

		IPipelineBuilderImpl& SetDebugName			(const char* name) override;

		IPipelineBuilderImpl& AddInputLayout		(const InputLayoutState&	state = {}) override;
		IPipelineBuilderImpl& AddInputTopology		(const ETopology			topology) override;
		IPipelineBuilderImpl& AddDepthStencilState	(const DepthStencilState&	state = {})override;
		IPipelineBuilderImpl& AddRasterizerState	(const RasterizerState&		state = {})override;
		IPipelineBuilderImpl& AddRenderTargetState	(const RenderTargetState&	state = {})override;
		IPipelineBuilderImpl& AddDepthStencilFormat	(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT) override;
		IPipelineBuilderImpl& AddBlendState			(const BlendState&			state = {}) override;

		LoadPipelineStateRes Build					(IRenderSystem& renderSystem, iAllocator& tempAllocator) override;
		LoadPipelineStateRes BuildStream			(IRenderSystem& renderSystem, void* buffer, const size_t size) override;

		VkPipelineVertexInputStateCreateInfo*	GetVertexInputState() const;
		VkPipelineInputAssemblyStateCreateInfo* GetInputAssemblyState() const;
		VkPipelineTessellationStateCreateInfo*	GetTessellationState() const;
		VkPipelineViewportStateCreateInfo*		GetViewportState() const;
		VkPipelineRasterizationStateCreateInfo* GetRasterizationState() const;
		VkPipelineMultisampleStateCreateInfo*	GetMultiSampleState() const;
		VkPipelineDepthStencilStateCreateInfo*	GetDepthStencilState() const;
		VkPipelineColorBlendStateCreateInfo*	GetBlendState() const;

        enum class InfoType
		{
		    VertexInput,
			InputAssembly,
			Tessellation,
			Viewport,
			RasterizationState,
			MultiSample,
			DepthStencil,
			ColorBlend,
			Dynamic
		};

		struct StateObject
		{
			InfoType type;
			void* _ptr;
		};

		Vector<StateObject, 0, uint8_t>							stateObjects;
		Vector<VkPipelineShaderStageCreateInfo, 0, uint8_t>		shaderStages;
		Vector<Shader, 0, uint8_t>								shaders;
		iAllocator&												allocator;
    };
}
