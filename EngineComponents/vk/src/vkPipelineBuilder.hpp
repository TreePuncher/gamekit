#include <RenderSystemInterface.hpp>

#include "Serialization.hpp"
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

	struct vkRenderSystem;

	struct VertexStateObject
	{
		Vector<VkVertexInputAttributeDescription>	inputAttributes;
		Vector<VkVertexInputBindingDescription>		inputBindings;
		VkPipelineVertexInputStateCreateInfo		info;
	};

    struct vkPipelineBuilder : IPipelineBuilder
    {
		vkPipelineBuilder(vkRenderSystem&, iAllocator&);
		~vkPipelineBuilder() override;

		void Release() override;

		IPipelineBuilder& AddRootSignature		(const IPipelineInterface* rootSig) override;

		IPipelineBuilder& AddShaderLibrary		(const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddShaderLibrary		(GUID_t) override;

		IPipelineBuilder& AddComputeShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddComputeShader		(GUID_t) override;
		IPipelineBuilder& AddComputeShader		(const char* assetID) override;

		IPipelineBuilder& AddWorkGraph			(const WorkGraph_Desc& desc = {}) override;

		IPipelineBuilder& AddVertexShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddVertexShader		(GUID_t) override;
		IPipelineBuilder& AddVertexShader		(const char* assetID) override;
		IPipelineBuilder& AddDomainShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddDomainShader		(GUID_t) override;
		IPipelineBuilder& AddDomainShader		(const char* assetID) override;
		IPipelineBuilder& AddHullShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddHullShader			(GUID_t) override;
		IPipelineBuilder& AddHullShader			(const char* assetID) override;
		IPipelineBuilder& AddGeometryShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddGeometryShader		(GUID_t) override;
		IPipelineBuilder& AddGeometryShader		(const char* assetID) override;

		IPipelineBuilder& AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddAmplificationShader(GUID_t) override;
		IPipelineBuilder& AddAmplificationShader(const char* assetID) override;
		IPipelineBuilder& AddMeshShader			(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddMeshShader			(GUID_t) override;
		IPipelineBuilder& AddMeshShader			(const char* assetID) override;

		IPipelineBuilder& AddPixelShader		(const char* entryPoint, const char* file, const ShaderOptions& options = {}) override;
		IPipelineBuilder& AddPixelShader		(GUID_t) override;
        IPipelineBuilder& AddPixelShader		(const char* entryPoint, const Shader&) override;
		IPipelineBuilder& AddPixelShader		(const char* assetID) override;

		void LoadShaderAsset(GUID_t, VkShaderStageFlagBits);
		void LoadShaderAsset(const char* assetID, VkShaderStageFlagBits);

		IPipelineBuilder& SetDebugName			(const char* name) override;

		IPipelineBuilder& AddInputLayout		(const InputLayoutState&	state = {}) override;
		IPipelineBuilder& AddInputTopology		(const ETopology			topology) override;
		IPipelineBuilder& AddDepthStencilState	(const DepthStencilState&	state = {})override;
		IPipelineBuilder& AddRasterizerState	(const RasterizerState&		state = {})override;
		IPipelineBuilder& AddRenderTargetState	(const RenderTargetState&	state = {})override;
		IPipelineBuilder& AddDepthStencilFormat	(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT) override;
		IPipelineBuilder& AddBlendState			(const BlendState&			state = {}) override;

		LoadPipelineStateRes Build					(IRenderSystem& renderSystem, iAllocator& tempAllocator) override;
		LoadPipelineStateRes BuildStream			(IRenderSystem& renderSystem, void* buffer, const size_t size) override;


		VkPipelineViewportStateCreateInfo*		CreateViewportState();

		struct VertexStateObject*				GetVertexInputState() const;
		VkPipelineInputAssemblyStateCreateInfo* GetInputAssemblyState() const;
		VkPipelineTessellationStateCreateInfo*	GetTessellationState() const;
		VkPipelineRenderingCreateInfoKHR*		GetRenderTargetState() const;
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
			RenderTarget,
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

		struct LoadedShader
		{
			const char*				entryPoint;
			VkShaderStageFlagBits	stage;
			Shader					shader;
		};

		Vector<StateObject, 0, uint8_t>							stateObjects;
		Vector<VkPipelineShaderStageCreateInfo, 0, uint8_t>		shaderStages;
		Vector<LoadedShader, 0, uint8_t>						shaders;
		Vector<VkViewport, 0, uint8_t>							viewports;
		iAllocator&												allocator;
    };
}
