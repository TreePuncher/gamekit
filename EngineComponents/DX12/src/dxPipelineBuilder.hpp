#include "dxRenderSystem.hpp"

namespace dx_Internal
{
	struct PipelineBuilderImpl : IPipelineBuilder
	{
		PipelineBuilderImpl(iAllocator& allocator);
		~PipelineBuilderImpl();

		IPipelineBuilder& AddRootSignature(const IPipelineInterface* rootSig) final;

		IPipelineBuilder& AddShaderLibrary(const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddShaderLibrary(GUID_t) final;

		IPipelineBuilder& AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddComputeShader(GUID_t) final;
		IPipelineBuilder& AddComputeShader(const char*) final;
		IPipelineBuilder& AddWorkGraph(const WorkGraph_Desc& desc = {}) final;

		IPipelineBuilder& AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddVertexShader(GUID_t) final;
		IPipelineBuilder& AddVertexShader(const char*) final;

		IPipelineBuilder& AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddDomainShader(GUID_t) final;
		IPipelineBuilder& AddDomainShader(const char*) final;

		IPipelineBuilder& AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddHullShader(GUID_t) final;
		IPipelineBuilder& AddHullShader(const char*) final;

		IPipelineBuilder& AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddGeometryShader(GUID_t) final;
		IPipelineBuilder& AddGeometryShader(const char*) final;

		IPipelineBuilder& AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddAmplificationShader(GUID_t) final;
		IPipelineBuilder& AddAmplificationShader(const char*) final;

		IPipelineBuilder& AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddMeshShader(GUID_t) final;
		IPipelineBuilder& AddMeshShader(const char*) final;

		IPipelineBuilder& AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options = {}) final;
		IPipelineBuilder& AddPixelShader(const char* entryPoint, const Shader&) final;
		IPipelineBuilder& AddPixelShader(GUID_t) final;
		IPipelineBuilder& AddPixelShader(const char*) final;

		IPipelineBuilder& SetDebugName(const char* name) { debugName = name; return *this; }

		IPipelineBuilder& AddInputLayout(const InputLayoutState& state = {});
		IPipelineBuilder& AddInputTopology(const ETopology			topology);
		IPipelineBuilder& AddDepthStencilState(const DepthStencilState& state = {});
		IPipelineBuilder& AddRasterizerState(const RasterizerState& state = {});
		IPipelineBuilder& AddRenderTargetState(const RenderTargetState& state = {});
		IPipelineBuilder& AddDepthStencilFormat(const DeviceFormat			format = DeviceFormat::D24_UNORM_S8_UINT);
		IPipelineBuilder& AddBlendState(const BlendState& state = {});

		LoadPipelineStateRes Build(IRenderSystem& renderSystem, iAllocator& tempAllocator) final;
		LoadPipelineStateRes BuildStream(IRenderSystem& renderSystem, void* buffer, const size_t size) final;

		void Release() final;

		class PipelineBlob
		{
		public:
			PipelineBlob(iAllocator& IN_allocator) :
				buffer{ IN_allocator } {}


			template<typename TY>
			PipelineBlob(const TY& IN_struct)
			{
				//static_assert(std::is_pod_v<TY>, "POD types only!");

				buffer.resize(sizeof(IN_struct));
				memcpy(data(), &IN_struct, sizeof(TY));
			}


			PipelineBlob(const char* IN_buffer, const size_t size)
			{
				buffer.resize(size);
				memcpy(data(), IN_buffer, size);
			}


			template<typename TY>
			PipelineBlob& operator += (const TY& blob)
			{
				const size_t offset = buffer.size();

				buffer.resize(buffer.size() + sizeof(blob));
				memcpy(buffer.data() + offset, &blob, sizeof(blob));

				return *this;
			}

			template<typename TY, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE TYPEID, typename TY_>
			PipelineBlob& operator += (const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>& blob)
			{
				const size_t offset = buffer.size();
				const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>* _ptr = std::addressof(blob);

				buffer.resize(buffer.size() + sizeof(const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>));
				memcpy(buffer.data() + offset, _ptr, sizeof(const CD3DX12_PIPELINE_STATE_STREAM_SUBOBJECT<TY, TYPEID, TY_>));

				return *this;
			}

			size_t size() const
			{
				return buffer.size();
			}


			void resize(size_t newSize)
			{
				buffer.resize(newSize);
			}


			char* data()
			{
				return buffer.data();
			}


			operator const char* () const
			{
				return buffer.data();
			}


			void Clear()
			{
				buffer.clear();
			}

			void Serialize(auto& ar)
			{
				ar& buffer;
			}

			void Release()
			{
				buffer.Release();
			}

			Vector<char> buffer;
		};

		const char*					debugName		= nullptr;
		const RootSignature*		rootSig			= nullptr;
		D3D12_INPUT_ELEMENT_DESC*	inputElements	= nullptr;
		bool						built			= false;
		uint64_t					hash			= 0xcbf29ce484222325;
		iAllocator*					allocator		= nullptr;

		PipelineBlob				blob;
		Vector<Shader>				shaders;
	};
}
