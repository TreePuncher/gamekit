#include "BuildSettings.hpp"
#include "dxPipelineBuilder.hpp"

namespace dx_Internal
{	/************************************************************************************************/


	PipelineBuilderImpl::PipelineBuilderImpl(iAllocator& IN_allocator) :
		allocator{ IN_allocator },
		blob{ IN_allocator },
		shaders{ IN_allocator }
	{
		blob.buffer.reserve(1024);
	}


	/************************************************************************************************/


	PipelineBuilderImpl::~PipelineBuilderImpl()
	{
		Release();
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddRootSignature(const IPipelineInterface* pipelineInterface)
	{
		auto IN_rootSig = static_cast<const RootSignature*>(pipelineInterface);
		rootSig = IN_rootSig;

		blob += CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE{ rootSig->Get_ptr() };

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddShaderLibrary(const char* file, const ShaderOptions& options)
	{
		/*
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(nullptr, "lib_6_8", file, options));
		hash = FNVa62(shaders.back().buffer, shaders.back().bufferSize, hash);

		struct
		{
			D3D12_STATE_SUBOBJECT_TYPE type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;

		} DXILObject;
		blob += DXILObject;

		//D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH
		*/

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddShaderLibrary(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddComputeShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "cs_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_CS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddComputeShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddComputeShader(const char*)
	{
		return*this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddWorkGraph(const WorkGraph_Desc& work_desc)
	{
		struct {
			D3D12_STATE_SUBOBJECT_TYPE type = D3D12_STATE_SUBOBJECT_TYPE::D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH;
			D3D12_WORK_GRAPH_DESC workGraph;
		} subObject = {
			.workGraph {
				.ProgramName = nullptr,
				.Flags = (D3D12_WORK_GRAPH_FLAGS)work_desc.flags,
				.NumEntrypoints = 0,
				.pEntrypoints = nullptr,
				.NumExplicitlyDefinedNodes = work_desc.nodeCount,
				.pExplicitlyDefinedNodes = nullptr,
			}
		};

		blob += subObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddVertexShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "vs_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_VS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddVertexShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddVertexShader(const char*)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddDomainShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "ds_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_DS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddDomainShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddDomainShader(const char*)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddHullShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "hs_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_HS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddHullShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddHullShader(const char*)
	{
		return *this;

	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddGeometryShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "gs_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_GS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddGeometryShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddGeometryShader(const char*)
	{
		return *this;

	}


	/************************************************************************************************/



	IPipelineBuilder& PipelineBuilderImpl::AddAmplificationShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "as_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_AS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddAmplificationShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddAmplificationShader(const char*)
	{
		return *this;

	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddMeshShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "ms_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_MS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddMeshShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddMeshShader(const char*)
	{
		return *this;

	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddPixelShader(const char* entryPoint, const char* file, const ShaderOptions& options)
	{
		shaders.emplace_back(dxRenderSystem::_GetInstance().LoadShader(entryPoint, "ps_6_7", file, options));
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_PS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	IPipelineBuilder& PipelineBuilderImpl::AddPixelShader(const char* entryPoint, const Shader& shader)
	{
		shaders.emplace_back(shader);
		hash = FNVa62((const char*)shaders.back().buffer, shaders.back().bufferSize, hash);

		CD3DX12_PIPELINE_STATE_STREAM_PS streamObject = D3D12_SHADER_BYTECODE{ (D3D12_SHADER_BYTECODE)Shader2ByteCode(shaders.back()) };
		blob += streamObject;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddPixelShader(GUID_t)
	{
		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddPixelShader(const char*)
	{
		return *this;

	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddInputLayout(const InputLayoutState& state)
	{
		if (inputElements)
			return *this;

		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT layout;
		memset(std::addressof(layout), 0, sizeof(layout));

		inputElements = (D3D12_INPUT_ELEMENT_DESC*)allocator->malloc(state.count * sizeof(D3D12_INPUT_ELEMENT_DESC));

		for (auto&& [idx, input] : zip(iota(0u, state.count), state.inputs))
		{
			inputElements[idx] =
				D3D12_INPUT_ELEMENT_DESC{
					.SemanticName			= input.name,
					.SemanticIndex			= input.index,
					.Format					= TextureFormat2DXGIFormat(input.format),
					.InputSlot				= input.slot,
					.AlignedByteOffset		= input.alignedByteOffset,
					.InputSlotClass			= ToDX(input.inputSlotClass),
					.InstanceDataStepRate	= input.instanceStepRate
			};
		}

		layout = D3D12_INPUT_LAYOUT_DESC{
			.pInputElementDescs = inputElements,
			.NumElements		= state.count,
		};

		hash = FNVa62((const char*)inputElements, state.count * sizeof(D3D12_INPUT_ELEMENT_DESC), hash);
		blob += layout;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddInputTopology(const ETopology topology)
	{
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY dxTopolgy{ (D3D12_PRIMITIVE_TOPOLOGY_TYPE)topology };
		hash = FNVa62((const char*)&dxTopolgy, sizeof(dxTopolgy), hash);
		blob += dxTopolgy;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddRasterizerState(const RasterizerState& state)
	{
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER2 rasterizerState;
		memset(&rasterizerState, 0, sizeof(CD3DX12_RASTERIZER_DESC2));

		CD3DX12_RASTERIZER_DESC2& desc = rasterizerState;
		desc = CD3DX12_RASTERIZER_DESC2{ D3D12_DEFAULT };

		desc.ConservativeRaster		= state.conservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		desc.CullMode				= (D3D12_CULL_MODE)state.CullMode;
		desc.DepthBias				= state.depthBias;
		desc.DepthBiasClamp			= state.depthBiasClamp;
		desc.FillMode				= (D3D12_FILL_MODE)state.fill;
		desc.ForcedSampleCount		= state.forcedSampleCount;
		desc.FrontCounterClockwise	= state.frontCounterClockWise;
		desc.LineRasterizationMode	= (D3D12_LINE_RASTERIZATION_MODE)state.antialiasedLineMode;

		hash = FNVa62((const char*)&rasterizerState, sizeof(rasterizerState), hash);
		blob += rasterizerState;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddDepthStencilState(const DepthStencilState& inputState)
	{
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencil{};
		CD3DX12_DEPTH_STENCIL_DESC& state = depthStencil;

		state.BackFace.StencilDepthFailOp	= (D3D12_STENCIL_OP)inputState.backFace.stencilDepthFailOp;
		state.BackFace.StencilFailOp		= (D3D12_STENCIL_OP)inputState.backFace.stencilFailOp;
		state.BackFace.StencilPassOp		= (D3D12_STENCIL_OP)inputState.backFace.stencilPassOp;
		state.BackFace.StencilFunc			= (D3D12_COMPARISON_FUNC)inputState.backFace.stencilFunc;

		state.FrontFace.StencilDepthFailOp	= (D3D12_STENCIL_OP)inputState.frontFace.stencilDepthFailOp;
		state.FrontFace.StencilFailOp		= (D3D12_STENCIL_OP)inputState.frontFace.stencilFailOp;
		state.FrontFace.StencilPassOp		= (D3D12_STENCIL_OP)inputState.frontFace.stencilPassOp;
		state.FrontFace.StencilFunc			= (D3D12_COMPARISON_FUNC)inputState.frontFace.stencilFunc;

		state.DepthEnable		= (uint32_t)inputState.depthEnable;
		state.DepthFunc			= (D3D12_COMPARISON_FUNC)inputState.depthFunc;
		state.DepthWriteMask	= (D3D12_DEPTH_WRITE_MASK)inputState.depthWriteMask;
		state.StencilEnable		= inputState.stencilEnable;
		state.StencilReadMask	= inputState.stencilReadMask;
		state.StencilWriteMask	= inputState.stencilWriteMask;

		hash = FNVa62((const char*)&depthStencil, sizeof(depthStencil), hash);
		blob += depthStencil;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddBlendState(const BlendState& state)
	{
		CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blendState{};
		CD3DX12_BLEND_DESC& desc = blendState;

		desc.AlphaToCoverageEnable = state.alphaToCoverageEnable;
		desc.IndependentBlendEnable = state.independentBlendEnable;
		memcpy(&desc.RenderTarget, &state.renderTarget, sizeof(state));

		blob += blendState;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddRenderTargetState(const RenderTargetState& state)
	{
		CD3DX12_RT_FORMAT_ARRAY formats{};
		memset(&formats, 0, sizeof(formats));
		formats.NumRenderTargets = state.targetCount;

		for (auto [idx, format] : zip(iota(0u, state.targetCount), state.targetFormats))
			formats.RTFormats[idx] = TextureFormat2DXGIFormat(format);

		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS dxFormats{ formats };
		hash = FNVa62((const char*)&dxFormats, sizeof(dxFormats), hash);
		blob += dxFormats;

		return *this;
	}


	/************************************************************************************************/


	IPipelineBuilder& PipelineBuilderImpl::AddDepthStencilFormat(const DeviceFormat format)
	{
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dxDepthFormat{ TextureFormat2DXGIFormat(format) };
		hash = FNVa62((const char*)&dxDepthFormat, sizeof(dxDepthFormat), hash);
		blob += dxDepthFormat;

		return *this;
	}


	/************************************************************************************************/


	FlexKit::LoadPipelineStateRes PipelineBuilderImpl::Build(IRenderSystem& irs, iAllocator& tempAllocator)
	{
		auto& renderSystem = static_cast<dxRenderSystem&>(irs);

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
			.SizeInBytes = blob.size(),
			.pPipelineStateSubobjectStream = blob.data()
		};

		ID3D12PipelineState* pso = nullptr;
		auto HR = renderSystem.pDevice14->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));

		if (SUCCEEDED(HR))
		{
			if (!rootSig)
			{
				for (auto& shader : shaders)
				{
					ID3D12RootSignature* dxRootSig = nullptr;
					HR = renderSystem.pDevice14->CreateRootSignature(0, shader.buffer, shader.bufferSize, IID_PPV_ARGS(&dxRootSig));

					if (SUCCEEDED(HR))
					{
						rootSig = renderSystem._GetRootSignature((uint64_t)dxRootSig);

						if (!rootSig)
						{
							RootSignatureBuilder builder{ *renderSystem.allocator };
							rootSig = (RootSignature*)builder.LoadSignatureFromBlob(shader.buffer, shader.bufferSize, *renderSystem.allocator);
						}
						break;
					}
				}
			}

			if (!rootSig)
			{
				// TODO: use shader reflection!
				FK_LOG_ERROR("Failed to acquire root signature!");
				return { nullptr, nullptr };
			}

			if (pso && debugName)
			{
				SETDEBUGNAME(pso, debugName);
				rootSig->SetDebugStr(debugName);
			}

			allocator->free(inputElements);
			inputElements = nullptr;
			blob.Clear();

			return { pso, rootSig };
		}
		else
			return { nullptr, nullptr };
	}


	/************************************************************************************************/


	LoadPipelineStateRes PipelineBuilderImpl::BuildStream(IRenderSystem& irs, void* buffer, const size_t size)
	{
		auto& renderSystem = static_cast<dxRenderSystem&>(irs);

		const D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{
			.SizeInBytes = size,
			.pPipelineStateSubobjectStream = buffer
		};

		ID3D12PipelineState* pso = nullptr;
		auto HR = renderSystem.pDevice14->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pso));

		return { pso, nullptr };
	}


	/************************************************************************************************/


	void PipelineBuilderImpl::Release()
	{
		if (allocator)
		{
			blob.Release();
			shaders.Release();
			allocator->free(inputElements);
			allocator = nullptr;
		}
	}

	
}	/************************************************************************************************/
