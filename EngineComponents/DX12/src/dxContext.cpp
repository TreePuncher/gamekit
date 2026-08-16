#include "dxContext.hpp"
#include "dxRenderSystem.hpp"
#include "PushBuffers.hpp"
#include "dxDescriptorSet.hpp"
#include "TriMeshResource.hpp"

#include <directx/d3d12.h>

namespace dx_Internal
{
    static bool AddVertexBuffer(VERTEXBUFFER_TYPE type, TriMesh* Mesh, size_t lod, static_vector<D3D12_VERTEX_BUFFER_VIEW>& out)
    {
	    auto res = FindBufferEntry(Mesh, lod, type);

	    if (!res)
	    {
			// Null buffer
			out.emplace_back(
				D3D12_VERTEX_BUFFER_VIEW{
					.BufferLocation = 0,
					.SizeInBytes	= 0,
					.StrideInBytes	= 0,
				});
	    }
		else
		{
			auto&& VB = res.value();
			out.emplace_back(
				D3D12_VERTEX_BUFFER_VIEW{
					.BufferLocation		= VB.resource.As<ID3D12Resource>()->GetGPUVirtualAddress(),
					.SizeInBytes		= VB.byteSize,
					.StrideInBytes		= VB.byteStride,
				});
		}
		return true;
	}

	static DeviceResource_ptr GetBuffer(TriMesh* Mesh, size_t lod, size_t Buffer)
	{
		return Mesh->lods[lod].bufferSet->At(Buffer).resource;
	}


	dxDirectContext::dxDirectContext(
				dxRenderSystem*	renderSystem_IN,
				iAllocator*		allocator) :
			CurrentRootSignature	{ nullptr			},
			pendingBarriers			{ },
			renderSystem			{ renderSystem_IN	},
			Memory					{ allocator			},
			RenderTargetCount		{ 0					},
			DepthStencilEnabled		{ false				}
	{
		HRESULT HR;

		D3D12_DESCRIPTOR_HEAP_DESC cpuDescriptorHeapdesc;
		cpuDescriptorHeapdesc.Flags				= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		cpuDescriptorHeapdesc.NumDescriptors	= 1024;
		cpuDescriptorHeapdesc.NodeMask			= 0;
		cpuDescriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&cpuDescriptorHeapdesc, IID_PPV_ARGS(&descHeapSRVLocal));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapdesc;
		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors	= 128;
		descriptorHeapdesc.NodeMask			= 0;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapRTV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		descriptorHeapdesc.Flags			= D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		descriptorHeapdesc.NumDescriptors	= 128;
		descriptorHeapdesc.NodeMask			= 0;
		descriptorHeapdesc.Type				= D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		HR = renderSystem->pDevice->CreateDescriptorHeap(&descriptorHeapdesc, IID_PPV_ARGS(&descHeapDSV));
		FK_ASSERT(HR, "FAILED TO CREATE DESCRIPTOR HEAP");

		FK_LOG_9("GRAPHICS SRV DESCRIPTOR HEAP CREATED: %u", descHeapSRVLocal);
		FK_LOG_9("GRAPHICS RTV DESCRIPTOR HEAP CREATED: %u", descHeapRTV);
		FK_LOG_9("GRAPHICS DSV DESCRIPTOR HEAP CREATED: %u", descHeapDSV);

		SETDEBUGNAME(descHeapRTV, "GPURESOURCEHEAP");
		SETDEBUGNAME(descHeapDSV, "RENDERTARGETHEAP");

		HR = renderSystem->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));												FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND ALLOCATOR!");
		HR = renderSystem->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&DeviceContext));	FK_ASSERT(FAILED(HR), "FAILED TO CREATE COMMAND LIST!");

#if USING(DEBUGGRAPHICS)
		DeviceContext->QueryInterface(IID_PPV_ARGS(&debugCommandList));
#endif

		SETDEBUGNAME(DeviceContext, "GraphicsContext");
		SETDEBUGNAME(DeviceContext, "GraphicsContextAllocator");

#if USING(AFTERMATH)
		auto res = GFSDK_Aftermath_DX12_CreateContextHandle(DeviceContext, &AFTERMATH_context);
#endif

		DeviceContext->Close();
	}


	/************************************************************************************************/


	dxDirectContext::dxDirectContext(dxDirectContext&& RHS)
	{
		DeviceContext			= RHS.DeviceContext;
		CurrentRootSignature	= RHS.CurrentRootSignature;
		CurrentPipelineState	= RHS.CurrentPipelineState;
		renderSystem			= RHS.renderSystem;

#if USING(DEBUGGRAPHICS)
		debugCommandList		= RHS.debugCommandList;
#endif

		RTV_CPU = RHS.RTV_CPU;

		shaderResources = RHS.shaderResources;
		heapUsed		= RHS.heapUsed;

		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		pendingBarriers			= RHS.pendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator		= RHS.commandAllocator;

		// Null out old dxDirectContext
		RHS.commandAllocator		= nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

#if USING(DEBUGGRAPHICS)
		RHS.debugCommandList		= nullptr;
#endif

		RHS.RenderTargetCount		= 0;
		RHS.DepthStencilEnabled		= false;
		RHS.Memory					= nullptr;

		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.pendingBarriers.clear();

		RHS.RTV_CPU = { 0 };

		RHS.shaderResources = {};
		RHS.heapUsed		= 0;

		RHS.DSV_CPU = { 0 };
		RHS.descHeapRTV = nullptr;
		RHS.descHeapDSV = nullptr;

#if USING(AFTERMATH)
		AFTERMATH_context       = RHS.AFTERMATH_context;
		RHS.AFTERMATH_context   = nullptr;
#endif
	}


	/************************************************************************************************/


	dxDirectContext& dxDirectContext::operator = (dxDirectContext&& RHS)// Moves only
	{
		DeviceContext			= RHS.DeviceContext;
		CurrentRootSignature	= RHS.CurrentRootSignature;
		CurrentPipelineState	= RHS.CurrentPipelineState;
		renderSystem			= RHS.renderSystem;

#if USING(DEBUGGRAPHICS)
		debugCommandList		= RHS.debugCommandList;
#endif

		
		shaderResources = RHS.shaderResources;
		heapUsed		= RHS.heapUsed;


		RTV_CPU = RHS.RTV_CPU;
		DSV_CPU = RHS.DSV_CPU;

		descHeapRTV = RHS.descHeapRTV;
		descHeapDSV = RHS.descHeapDSV;

		RenderTargetCount		= RHS.RenderTargetCount;
		DepthStencilEnabled		= RHS.DepthStencilEnabled;

		DesciptorHeaps			= RHS.DesciptorHeaps;
		VBViews					= RHS.VBViews;
		pendingBarriers			= RHS.pendingBarriers;
		Memory					= RHS.Memory;
		commandAllocator		= RHS.commandAllocator;

		// Null out old dxDirectContext
		RHS.commandAllocator		= nullptr;
		RHS.DeviceContext			= nullptr;
		RHS.CurrentRootSignature	= nullptr;

#if USING(DEBUGGRAPHICS)
		RHS.debugCommandList		= nullptr;
#endif

		RHS.RenderTargetCount		= 0;
		RHS.DepthStencilEnabled		= false;
		RHS.Memory					= nullptr;

		RHS.DesciptorHeaps.clear();
		RHS.VBViews.clear();
		RHS.pendingBarriers.clear();

		RHS.DSV_CPU			= { 0 };
		RHS.RTV_CPU			= { 0 };
		RHS.shaderResources = {};
		RHS.heapUsed		= 0;

		RHS.descHeapRTV = nullptr;
		RHS.descHeapDSV = nullptr;


#if USING(AFTERMATH)
		AFTERMATH_context       = RHS.AFTERMATH_context;
		RHS.AFTERMATH_context   = nullptr;
#endif

		return *this;
	}


	/************************************************************************************************/


	void dxDirectContext::Release()
	{
#if USING(AFTERMATH)
		GFSDK_Aftermath_ReleaseContextHandle(AFTERMATH_context);
#endif

		if(descHeapRTV)
			descHeapRTV->Release();

		if (descHeapSRVLocal)
			descHeapSRVLocal->Release();

		if (descHeapDSV)
			descHeapDSV->Release();

		if(commandAllocator)
			commandAllocator->Release();

#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->Release();
#endif

		if(DeviceContext)
			DeviceContext->Release();

		shaderResources			= {};
		heapUsed				= 0;
		descHeapDSV				= nullptr;
		descHeapRTV				= nullptr;
		descHeapSRVLocal		= nullptr;
		commandAllocator		= nullptr;
		DeviceContext			= nullptr;
		CurrentPipelineState	= nullptr;

#if USING(DEBUGGRAPHICS)
		debugCommandList	 = nullptr;
#endif
	}


	/************************************************************************************************/


	void dxDirectContext::CreateAS(const AccelerationStructureDesc& asDesc, const TriMesh&)
	{
		FK_ASSERT(0);
	}


	void dxDirectContext::BuildBLAS(IVertexBufferSet& bufferSet, ResourceHandle destination, ResourceHandle scratchSpace)
	{
		auto indexBuffer    = bufferSet[bufferSet.GetIndexBufferIndex()];
		auto positionBuffer = bufferSet.Find(VERTEXBUFFER_TYPE::POSITION);

		D3D12_RAYTRACING_GEOMETRY_DESC desc;
		desc.Type   = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Flags =
			D3D12_RAYTRACING_GEOMETRY_FLAGS::D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		desc.Triangles.Transform3x4 = 0;
		desc.Triangles.IndexFormat  = DXGI_FORMAT_R32_UINT;
		desc.Triangles.IndexBuffer  = GetDevicePointer(indexBuffer);
		desc.Triangles.IndexCount   = (UINT)indexBuffer.Size();

		desc.Triangles.VertexFormat                 = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.VertexBuffer.StartAddress    = GetDevicePointer(positionBuffer.value());
		desc.Triangles.VertexBuffer.StrideInBytes   = positionBuffer.value().byteStride;
		desc.Triangles.VertexCount                  = (UINT)positionBuffer->Size();

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build_desc {
							.DestAccelerationStructureData = renderSystem->GetDeviceResource(destination).As<ID3D12Resource>()->GetGPUVirtualAddress(),
							.Inputs = {
								.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
								.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
								.NumDescs       = 1,
								.DescsLayout    = D3D12_ELEMENTS_LAYOUT::D3D12_ELEMENTS_LAYOUT_ARRAY,
								.pGeometryDescs = &desc,
							},
							.ScratchAccelerationStructureData = renderSystem->GetDeviceResource(scratchSpace).As<ID3D12Resource>()->GetGPUVirtualAddress(),
		};

		UpdateResourceStates();
		DeviceContext->BuildRaytracingAccelerationStructure(&build_desc, 0, nullptr);
	}


	/************************************************************************************************/


	void dxDirectContext::DiscardResource(ResourceHandle resource)
	{
		UpdateResourceStates();

		DeviceContext->DiscardResource(renderSystem->GetDeviceResource(resource).As<ID3D12Resource>(), nullptr);
	}


	/************************************************************************************************/


	void dxDirectContext::AddAliasingBarrier(ResourceHandle before, ResourceHandle after)
	{
		DebugBreak();

		/*
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					(rhs.type            == Barrier::type::Aliasing) &&
					((before != InvalidHandle &&  rhs.aliasedResources[0] == before) ||
					 (after != InvalidHandle &&   rhs.aliasedResources[1] == after));
			});

		if (std::end(PendingBarriers) == res)
		{
			Barrier barrier;
			barrier.type                = Barrier::type::Aliasing;
			barrier.aliasedResources[0] = before;
			barrier.aliasedResources[1] = after;

			PendingBarriers.push_back(barrier);
		}
		else
		{
			Barrier barrier;
			barrier.type                = Barrier::type::Aliasing;
			barrier.aliasedResources[0] = res->aliasedResources[0] == InvalidHandle ? before : res->aliasedResources[0];
			barrier.aliasedResources[1] = res->aliasedResources[1] == InvalidHandle ? after  : res->aliasedResources[1];

			PendingBarriers.push_back(barrier);
		}
		*/
	}


	/************************************************************************************************/


	void dxDirectContext::AddUAVBarrier(ResourceHandle resource, uint32_t subresource, DeviceLayout layout, DeviceSyncPoint src, DeviceSyncPoint dst)
	{
		if(resource != FlexKit::InvalidHandle)
		{
			const auto dimension = renderSystem->GetResourceDimension(resource);
			Barrier barrier;
			barrier.resource		= resource;
			barrier.accessBefore	= DASUAV;
			barrier.accessAfter		= DASUAV;
			barrier.src				= src;
			barrier.dst				= dst;

			switch (dimension)
			{
			case ResourceDimension::Buffer:
				barrier.type = BarrierType::Buffer;
				break;
			case ResourceDimension::Texture2D:
			case ResourceDimension::Texture2DArray:
			{
				barrier.type					= BarrierType::Texture;
				barrier.texture.flags			= 0;
				barrier.texture.layoutAfter		= layout;
				barrier.texture.layoutBefore	= layout;
			}	break;
			case ResourceDimension::Texture1D:
			case ResourceDimension::Texture3D:
			case ResourceDimension::TextureCubeMap:
				DebugBreak();
			}

			pendingBarriers.push_back(barrier);
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= DASUAV;
			barrier.accessAfter		= DASUAV;
			barrier.src				= src;
			barrier.dst				= dst;
			barrier.type			= BarrierType::Global;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void dxDirectContext::AddPresentBarrier(ResourceHandle Handle, DeviceAccessState Before)
	{
		DebugBreak();

		/*
		Barrier NewBarrier;
		NewBarrier.OldState		    = Before;
		NewBarrier.NewState		    = DeviceAccessState::DASPresent;
		NewBarrier.type			    = Barrier::type::Resource;
		NewBarrier.resourceHandle	= Handle;

		PendingBarriers.push_back(NewBarrier);
		*/
	}


	/************************************************************************************************/


	
	void dxDirectContext::AddGlobalBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
	{
		Barrier barrier;
		barrier.accessBefore	= accessBefore;
		barrier.accessAfter		= accessAfter;
		barrier.src				= syncBefore;
		barrier.dst				= syncAfter;
		barrier.resource		= resource;
		barrier.type			= BarrierType::Global;

		auto res = find(pendingBarriers, [&](const auto& i) { return (i.resource == resource); });

		if (res == std::end(pendingBarriers))
			pendingBarriers.push_back(barrier);
		else
			(*res) = barrier;
	}


	/************************************************************************************************/


	void dxDirectContext::AddTextureBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceLayout layoutBefore, DeviceLayout layoutAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter, BarrierSubResourceRange range)
	{
		auto res = find(pendingBarriers,
			[&](Barrier& rhs) -> bool { return (rhs.resource == resource); });

		if (res != pendingBarriers.end())
		{
			res->accessAfter			= accessAfter;
			res->texture.layoutAfter	= layoutAfter;
			res->dst					= syncAfter;
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= accessBefore;
			barrier.accessAfter		= accessAfter;
			barrier.src				= syncBefore;
			barrier.dst				= syncAfter;
			barrier.resource		= resource;
			barrier.type			= BarrierType::Texture;

			barrier.texture.layoutBefore	= layoutBefore;
			barrier.texture.layoutAfter		= layoutAfter;
			barrier.texture.flags			= 0;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void dxDirectContext::AddBufferBarrier(ResourceHandle resource, DeviceAccessState accessBefore, DeviceAccessState accessAfter, DeviceSyncPoint syncBefore, DeviceSyncPoint syncAfter)
	{
		auto res = find(pendingBarriers,
			[&](Barrier& rhs) -> bool { return (rhs.resource == resource); });

		if (res != pendingBarriers.end())
		{
			res->accessAfter		= accessAfter;
		}
		else
		{
			Barrier barrier;
			barrier.accessBefore	= accessBefore;
			barrier.accessAfter		= accessAfter;
			barrier.src				= syncBefore;
			barrier.dst				= syncAfter;
			barrier.resource		= resource;
			barrier.type			= BarrierType::Buffer;

			pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void dxDirectContext::AddBarriers(std::span<const Barrier> barriers)
	{
		for (auto& barrier : barriers)
		{
			if (barrier.resource != InvalidHandle)
				pendingBarriers.push_back(barrier);
		}
	}


	/************************************************************************************************/


	void dxDirectContext::AddCopyResourceBarrier(ResourceHandle resource, DeviceAccessState Before, DeviceAccessState State)
	{
		DebugBreak();

		/*
		auto res = find(PendingBarriers,
			[&](Barrier& rhs) -> bool
			{
				return
					rhs.type            == Barrier::type::Resource &&
					rhs.resourceHandle  == resource;
			});

		if (res != PendingBarriers.end()) {
			res->NewState = State;
		}
		else
		{
			Barrier NewBarrier;
			NewBarrier.OldState         = Before;
			NewBarrier.NewState         = State;
			NewBarrier.type             = Barrier::type::Resource;
			NewBarrier.resourceHandle   = resource;

			PendingBarriers.push_back(NewBarrier);
		}
		*/
	}


	/************************************************************************************************/


	void dxDirectContext::SetRootSignature(RootSigHandle rootSigHandle)
	{
		auto rootSig			= renderSystem->_GetRootSignature(rootSigHandle);
		CurrentRootSignature	= rootSig;
		DeviceContext->SetGraphicsRootSignature(*rootSig);
	}

	void dxDirectContext::SetRootSignature(const IPipelineInterface* rootSig)
	{
		auto dxRootSig = static_cast<const RootSignature*>(rootSig);
		CurrentRootSignature	= dxRootSig;
		DeviceContext->SetGraphicsRootSignature(*dxRootSig);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeRootSignature(RootSigHandle rootSigHandle)
	{
		auto rootSig = renderSystem->_GetRootSignature(rootSigHandle);

		CurrentComputeRootSignature = rootSig;
		DeviceContext->SetComputeRootSignature(*rootSig);
	}


	void dxDirectContext::SetComputeRootSignature(const IPipelineInterface* rootSig)
	{
		auto dxRootSig = static_cast<const RootSignature*>(rootSig);

		CurrentComputeRootSignature = dxRootSig;
		DeviceContext->SetComputeRootSignature(*dxRootSig);
	}


	/************************************************************************************************/


	/*
	void dxDirectContext::SetPipelineState(ID3D12PipelineState* PSO)
	{
		FK_ASSERT(PSO);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentPipelineState == PSO)
			return;

		CurrentPipelineState = PSO;
		DeviceContext->SetPipelineState(PSO);
	}
	*/

	/************************************************************************************************/


	void dxDirectContext::SetPipelineState(const IPipelineState* const PSO)
	{
		FK_ASSERT(PSO);

		const dxPipelineState* pso = static_cast<const dxPipelineState*>(PSO);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentPipelineState == pso->state)
			return;

		auto nextRootSignature = (RootSignature*)pso->rootSignature;
		if (nextRootSignature != CurrentRootSignature)
		{
			CurrentRootSignature = nextRootSignature;
			DeviceContext->SetGraphicsRootSignature(CurrentRootSignature->signature);
		}

		DeviceContext->SetPipelineState(pso->state);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputePipelineState(const PSOHandle stateHandle, iAllocator& temp)
	{
		auto [PSO, rootSignature]	= renderSystem->GetPSOAndRootSignature(stateHandle, temp);
		auto implRootSignature		= static_cast<const RootSignature*>(rootSignature);

		if (PSO == nullptr)
			__debugbreak();

		if (CurrentComputeRootSignature != rootSignature)
		{
			DeviceContext->SetComputeRootSignature(*implRootSignature);
			CurrentComputeRootSignature = implRootSignature;
		}

		if (auto implPSO = PSO->GetDevicePipeState(); implPSO != CurrentPipelineState)
		{
			CurrentPipelineState = implPSO.As<ID3D12PipelineState>();
			DeviceContext->SetPipelineState(implPSO.As<ID3D12PipelineState>());
		}
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsPipelineState(const PSOHandle stateHandle, iAllocator& temp)
	{
		auto [PSO, rootSignature] = renderSystem->GetPSOAndRootSignature(stateHandle, temp);
		auto implRootSignature = static_cast<const RootSignature*>(rootSignature);

		if (PSO == nullptr)
		{
			FK_LOG_ERROR("Failed to load pipeline state! %u", stateHandle.to_uint());

#if _DEBUG
			__debugbreak();
#endif
		}

		if (CurrentRootSignature != implRootSignature)
		{
			DeviceContext->SetGraphicsRootSignature(*implRootSignature);
			CurrentRootSignature = implRootSignature;
		}

		if (auto implPSO = PSO->GetDevicePipeState(); implPSO != CurrentPipelineState)
		{
			CurrentPipelineState = implPSO.As<ID3D12PipelineState>();
			DeviceContext->SetPipelineState(implPSO.As<ID3D12PipelineState>());
		}
	}


	/************************************************************************************************/

	void dxDirectContext::SetRTStateObject(ShaderID program, const IPipelineStateLibrary* const lib)
    {
		D3D12_SET_PROGRAM_DESC desc{
			.Type = D3D12_PROGRAM_TYPE_RAYTRACING_PIPELINE,
		};

		memcpy(&desc.RaytracingPipeline.ProgramIdentifier, &program, sizeof(D3D12_PROGRAM_IDENTIFIER));
		auto* dxLib = (dxPipelineStateLibrary*)lib;

		DeviceContext->SetPipelineState1(dxLib->stateObject);
    }


	/************************************************************************************************/


	void dxDirectContext::SetRenderTargets(const static_vector<ResourceHandle> RTs, bool enableDepthStencil, ResourceHandle depthStencil, const size_t MIPMapOffset)
	{
		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTV_CPU_HANDLES;


		bool WHsAllEqual = true;

		uint2 depthWH;
		uint2 textureWH;

		if (RTs.size() && enableDepthStencil)
		{
			depthWH = renderSystem->GetResourceWH(depthStencil);
			textureWH = renderSystem->GetResourceWH(RTs.front());

			WHsAllEqual = depthWH == textureWH;;
		}


		if (!MIPMapOffset && WHsAllEqual)
		{
			for (auto renderTarget : RTs) {
				auto res = std::find_if(
					renderTargetViews.begin(),
					renderTargetViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == renderTarget;
					});

				if (res != renderTargetViews.end())
				{
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 });
				}
				else
				{
					auto view = ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				auto WH = Min(depthWH, textureWH);

				RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}

		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		if(enableDepthStencil)
		{
			if (auto res = std::find_if(
					depthStencilViews.begin(),
					depthStencilViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == depthStencil;
					});
					res == depthStencilViews.end())
			{
				auto DSV = ReserveDSV(1);
				PushDepthStencil(renderSystem, depthStencil, DSV);

				DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ DSV.V1 };
			}
			else
				DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 };
		}


		DeviceContext->OMSetRenderTargets(
			(UINT)RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			enableDepthStencil,
			enableDepthStencil ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/


	void dxDirectContext::SetRenderTargets2(const static_vector<ResourceHandle> RTs, const size_t MIPMapOffset, const DepthStencilView_Options DSV)
	{
		static_vector<D3D12_CPU_DESCRIPTOR_HANDLE> RTV_CPU_HANDLES;

		if (!MIPMapOffset)
		{
			for (auto renderTarget : RTs) {
				auto res = std::find_if(
					renderTargetViews.begin(),
					renderTargetViews.end(),
					[&](RTV_View& view)
					{
						return view.resource == renderTarget;
					});

				if (res != renderTargetViews.end())
				{
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.V1 });
				}
				else
				{
					auto view = ReserveRTV(1);
					PushRenderTarget(renderSystem, renderTarget, view);
					RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
					renderTargetViews.push_back({ renderTarget, view });
				}
			}
		}
		else
		{
			auto view = ReserveRTV(RTs.size());
			
			for (auto& renderTarget : RTs)
			{
				RTV_CPU_HANDLES.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 });
				view = PushRenderTarget(renderSystem, renderTarget, view, MIPMapOffset);
			}
		}
		
		auto DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{};

		const bool depthEnabled = DSV.depthStencil != InvalidHandle;

		if(depthEnabled)
		{
			auto descriptor = GetDepthDesciptor(DSV.depthStencil);

			PushDepthStencilArray(renderSystem, DSV.depthStencil, DSV.ArraySliceOffset, DSV.MipOffset, descriptor, DSV.arraySize);

			DSV_CPU_HANDLE = D3D12_CPU_DESCRIPTOR_HANDLE{ descriptor.V1 };
		}

		DeviceContext->OMSetRenderTargets(
			(UINT)RTV_CPU_HANDLES.size(),
			RTV_CPU_HANDLES.begin(),
			depthEnabled,
			depthEnabled ? &DSV_CPU_HANDLE : nullptr);
	}


	/************************************************************************************************/

	void dxDirectContext::SetViewports(static_vector<D3D12_VIEWPORT, 16> VPs)
	{
		DeviceContext->RSSetViewports((UINT)VPs.size(), VPs.begin());
	}


	void dxDirectContext::SetViewports(std::span<const D3D12_VIEWPORT>	VPs)
	{
		DeviceContext->RSSetViewports((UINT)VPs.size(), VPs.data());
	}

	void dxDirectContext::SetScissorRects(static_vector<D3D12_RECT, 16>	Rects)
	{
		DeviceContext->RSSetScissorRects((UINT)Rects.size(), Rects.begin());
	}


	void dxDirectContext::SetScissorRects(std::span<const D3D12_RECT> rects)
	{
		DeviceContext->RSSetScissorRects((UINT)rects.size(), rects.data());
	}


	/************************************************************************************************/


	void dxDirectContext::SetViewports(std::span<const Viewport> rects)
	{
		DeviceContext->RSSetViewports((UINT)rects.size(), (const D3D12_VIEWPORT*)rects.data());
	}


	/************************************************************************************************/


	void dxDirectContext::SetScissorRects(std::span<const Rect>	rects)
	{
		DeviceContext->RSSetScissorRects((UINT)rects.size(), (const D3D12_RECT*)rects.data());
	}


	/************************************************************************************************/

	// Assumes setting each to fullscreen
	void dxDirectContext::SetScissorAndViewports(static_vector<ResourceHandle, 16>	RenderTargets)
	{
		static_vector<Viewport, 16>	VPs;
		static_vector<Rect, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = renderSystem->GetResourceWH(RT);
			VPs.emplace_back	(0.0f, 0.0f, (float)WH[0], (float)WH[1], 0.0f, 1.0f);
			Rects.emplace_back	(0u, 0u, WH[0], WH[1]);
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}

	/************************************************************************************************/


	void dxDirectContext::SetScissorAndViewports2(static_vector<ResourceHandle, 16>	RenderTargets, const size_t MIPMapOffset)
	{
		static_vector<Viewport, 16>	VPs;
		static_vector<Rect, 16>		Rects;

		for (auto RT : RenderTargets)
		{
			auto WH = float2{ renderSystem->GetResourceWH(RT) } / std::pow(2.0f, (float)MIPMapOffset);
			VPs.emplace_back	(0.0f, 0.0f,	WH[0], WH[1], 0.0f, 1.0f);
			Rects.emplace_back	(0u, 0u, (uint32_t)WH[0], (uint32_t)WH[1]);
		}

		SetViewports(VPs);
		SetScissorRects(Rects);
	}


	/************************************************************************************************/


	void dxDirectContext::QueueReadBack(ReadBackResourceHandle readBack)
	{
		queuedReadBacks.push_back(readBack);
	}

	void dxDirectContext::QueueReadBack(ReadBackResourceHandle readBack, ReadBackEventHandler callback)
	{
		renderSystem->SetReadBackEvent(readBack, std::move(callback));
		QueueReadBack(readBack);
	}


	/************************************************************************************************/


	void dxDirectContext::SetDepthStencil(ResourceHandle DS)
	{
		if (DS != InvalidHandle)
		{
			auto DSV = ReserveDSV(1);
			PushDepthStencil(renderSystem, DS, DSV);
			DeviceContext->OMSetRenderTargets(
				(UINT)RenderTargetCount,
				RenderTargetCount ? &RTVPOSCPU : nullptr, true,
				DepthStencilEnabled ? &DSVPOSCPU : nullptr);
		}
		else 
			DepthStencilEnabled = false;
	}


	/************************************************************************************************/


	void dxDirectContext::SetInputPrimitive(EInputPrimitive topology)
	{
		DeviceContext->IASetPrimitiveTopology((D3D12_PRIMITIVE_TOPOLOGY)topology);
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsConstantValue(size_t slot, size_t valueCount, const void* data_ptr, size_t offset)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::UINT);
		DeviceContext->SetGraphicsRoot32BitConstants((UINT)idx, (UINT)valueCount, data_ptr, (UINT)offset);
	}


	/************************************************************************************************/


	void dxDirectContext::NullGraphicsConstantBufferView(size_t slot)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsConstantBufferView(size_t slot, const ConstantBufferHandle CB, size_t Offset)
	{
		FK_ASSERT(!(Offset % 256), "Incorrect CB Offset!");

		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB) + (UINT)Offset);
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsConstantBufferView(size_t slot, const ConstantBuffer& CB)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, CB.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsConstantBufferView(size_t slot, const ConstantBufferDataSet& CB)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsDescriptorSet(size_t idx, const DescriptorSet& IDH)
	{
		if (!CurrentGraphicsRootSig())
			return;

		auto& impl = dxDescriptorSet::GetImpl(IDH);
		DeviceContext->SetGraphicsRootDescriptorTable((UINT)idx, impl);
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsConstantBufferView(size_t slot, DevicePointer devicePointer)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, devicePointer);
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsDescriptorSet(size_t slot, const DescriptorRange& range)
	{
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t  idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::DescriptorSet);
		DeviceContext->SetGraphicsRootDescriptorTable(
			(UINT)idx,
			D3D12_GPU_DESCRIPTOR_HANDLE{ range.begin.V2 });
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsShaderResourceView(size_t slot, FrameBufferedResource& Resource, size_t Count, size_t ElementSize)
	{
#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(Resource.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif
		if (!CurrentGraphicsRootSig())
			return;
		
		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::SRV);
		DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, Resource.Get()->GetGPUVirtualAddress());
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsShaderResourceView(size_t slot, ResourceHandle resource, size_t offset)
	{
		auto resource_ptr = renderSystem->GetDeviceResource(resource).As<ID3D12Resource>();
#if USING(DEBUGGRAPHICS)
		if (resource != InvalidHandle && debugCommandList)
			debugCommandList->AssertResourceState(resource_ptr, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif
		if (!CurrentGraphicsRootSig())
			return;

		const uint32_t idx = CurrentGraphicsRootSig()->GetIndex(slot, RootSignature::SlotType::SRV);

		if(resource != InvalidHandle)
			DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, resource_ptr->GetGPUVirtualAddress());
		else
			DeviceContext->SetGraphicsRootShaderResourceView((UINT)idx, { 0 });
	}


	/************************************************************************************************/


	void dxDirectContext::SetGraphicsUnorderedAccessView(size_t idx, ResourceHandle UAVresource, size_t offset)
	{
		auto resource = renderSystem->GetDeviceResource(UAVresource).As<ID3D12Resource>();

#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif

		DeviceContext->SetGraphicsRootUnorderedAccessView((UINT)idx, resource->GetGPUVirtualAddress() + offset);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeDescriptorSet(size_t slot)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::DescriptorSet);
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, D3D12_GPU_DESCRIPTOR_HANDLE{ 0 });
	}


	void dxDirectContext::SetComputeDescriptorSet(size_t slot, const DescriptorSet& IDH)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::DescriptorSet);

		auto& DH = dxDescriptorSet::GetImpl(IDH);
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, DH);
	}


	void dxDirectContext::SetComputeDescriptorSet(size_t slot, const DescriptorRange& range)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::DescriptorSet);
		DeviceContext->SetComputeRootDescriptorTable((UINT)idx, D3D12_GPU_DESCRIPTOR_HANDLE{ range.begin.V2 });
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeConstantBufferView(size_t slot, const ConstantBufferHandle CB, size_t offset)
	{
		if (!CurrentComputeRootSig())
			return;

#if USING(DEBUGGRAPHICS)
		auto resource = renderSystem->GetDeviceResource(CB).As<ID3D12Resource>();

		if (debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetGraphicsRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB) + offset);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeConstantBufferView(size_t slot, const ConstantBufferDataSet& CB)
	{
		if (!CurrentComputeRootSig())
			return;

#if USING(DEBUGGRAPHICS)
		auto resource = renderSystem->GetDeviceResource(CB.Handle()).As<ID3D12Resource>();

		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_GENERIC_READ);
#endif

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, renderSystem->GetConstantBufferAddress(CB.Handle()) + CB.Offset());
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeConstantBufferView(size_t slot, ResourceHandle resource, size_t offset, size_t bufferSize)
	{
		if (!CurrentComputeRootSig())
			return;

		auto deviceResource     = renderSystem->GetDeviceResource(resource).As<ID3D12Resource>();
		auto gpuAddress         = deviceResource->GetGPUVirtualAddress();

#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(deviceResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COMMON);
#endif

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, gpuAddress + offset);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeConstantBufferView(size_t slot, DevicePointer pointer)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		DeviceContext->SetComputeRootConstantBufferView((UINT)idx, pointer);
	}


	/************************************************************************************************/



	void dxDirectContext::SetComputeShaderResourceView(size_t slot, ResourceHandle resource, const size_t offset)
	{
		if (!CurrentComputeRootSig())
			return;

		auto deviceResource = renderSystem->GetDeviceResource(resource).As<ID3D12Resource>();

#if USING(DEBUGGRAPHICS)
		if (debugCommandList)
			debugCommandList->AssertResourceState(deviceResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
#endif

        const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::CBV);
		if(resource != InvalidHandle)
			DeviceContext->SetComputeRootShaderResourceView((UINT)idx, deviceResource->GetGPUVirtualAddress() + offset);
		else
			DeviceContext->SetComputeRootShaderResourceView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeUnorderedAccessView(size_t slot, ResourceHandle UAVresource, size_t offset)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::UAV);
		auto resource = renderSystem->GetDeviceResource(UAVresource).As<ID3D12Resource>();

#if USING(DEBUGGRAPHICS)
		if(debugCommandList)
			debugCommandList->AssertResourceState(resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#endif
		if (UAVresource != InvalidHandle)
			DeviceContext->SetComputeRootUnorderedAccessView((UINT)idx, resource->GetGPUVirtualAddress() + offset);
		else
			DeviceContext->SetComputeRootUnorderedAccessView((UINT)idx, 0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetComputeConstantValue(size_t slot, size_t valueCount, const void* data_ptr, size_t offset)
	{
		if (!CurrentComputeRootSig())
			return;

		const uint32_t  idx = CurrentComputeRootSig()->GetIndex(slot, RootSignature::SlotType::UINT);
		DeviceContext->SetComputeRoot32BitConstants((UINT)idx, (UINT)valueCount, data_ptr, (UINT)offset);
	}


	/************************************************************************************************/


	void dxDirectContext::BeginQuery(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource	= renderSystem->Queries.GetDeviceObject(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->BeginQuery(resource, queryType, (UINT)idx);
	}


	void dxDirectContext::EndQuery(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource	= renderSystem->Queries.GetDeviceObject(query);
		auto queryType	= renderSystem->Queries.GetType(query);

		DeviceContext->EndQuery(resource, queryType, (UINT)idx);
	}


	void dxDirectContext::TimeStamp(QueryHandle query, size_t idx)
	{
		if (query == InvalidHandle)
			return;

		auto resource   = renderSystem->Queries.GetDeviceObject(query);
		auto queryType  = renderSystem->Queries.GetType(query);

		DeviceContext->EndQuery(resource, queryType, (UINT)idx);
	}


	/************************************************************************************************/


	void dxDirectContext::SetMarker_DEBUG(const char* str)
	{
#if USING(AFTERMATH)
		GFSDK_Aftermath_GetShaderHash;
		AFTERMATH_context;
#endif
	}


	/************************************************************************************************/


	void dxDirectContext::BeginEvent_DEBUG(const char* str)
	{
#if USING(PIX)
		wchar_t temp[64];
		mbstowcs(temp, str, 64);

		PIXBeginEvent(DeviceContext, PIX_COLOR_INDEX(rand() % 255), temp);
#endif
	}


	/************************************************************************************************/


	void dxDirectContext::EndEvent_DEBUG()
	{
#if USING(PIX)
		PIXEndEvent(DeviceContext);
#endif
	}


	/************************************************************************************************/


	void dxDirectContext::CopyResource(ResourceHandle dest, ResourceHandle src)
	{
		FlushBarriers();

		DeviceContext->CopyResource(
			renderSystem->GetDeviceResource(dest).As<ID3D12Resource>(),
			renderSystem->GetDeviceResource(src).As<ID3D12Resource>());
	}


	/************************************************************************************************/


	void dxDirectContext::CopyTextureRegion(
		DeviceResource_ptr	destination,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				WH,
		DeviceFormat		format)
	{
		FlushBarriers();

		const auto		deviceFormat	= TextureFormat2DXGIFormat(format);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(format);
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination.As<ID3D12Resource>(), (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource.As<ID3D12Resource>(), SubRegion);

		DeviceContext->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}

	/************************************************************************************************/

	void dxDirectContext::CopyTextureRegion(
		ResourceHandle		dest,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				WH)
	{
		FlushBarriers();

		auto destination				= renderSystem->GetDeviceResource(dest).As<ID3D12Resource>();
		const auto		deviceFormat	= renderSystem->GetResourceDeviceFormat(dest);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(renderSystem->GetTextureFormat(dest));
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination, (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource.As<ID3D12Resource>(), SubRegion);

		DeviceContext->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}


	/************************************************************************************************/


	void dxDirectContext::CopyTile(DeviceResource_ptr dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{
		FlushBarriers();

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		DeviceContext->CopyTiles(
			dest.As<ID3D12Resource>(),
			&coordinate,
			&regionSize,
			src.resource.As<ID3D12Resource>(),
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	void dxDirectContext::CopyTile(
		ResourceHandle			dest,
		const uint3				destTile,
		const size_t			tileOffset,
		const UploadReservation src)
	{
		FlushBarriers();

		auto resource_ptr	= renderSystem->GetDeviceResource(dest);

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		DeviceContext->CopyTiles(
			resource_ptr.As<ID3D12Resource>(),
			&coordinate,
			&regionSize,
			src.resource.As<ID3D12Resource>(),
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	void dxDirectContext::CopyBufferRegion(
		ResourceHandle	destination,
		ResourceHandle	source,
		size_t			size,
		size_t			destinationOffset,
		size_t			sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			renderSystem->GetDeviceResource(destination).As<ID3D12Resource>(),
			destinationOffset,
			renderSystem->GetDeviceResource(source).As<ID3D12Resource>(),
			sourceOffset,
			size);
	}

	void dxDirectContext::CopyBufferRegion(
		ResourceHandle		destination,
		DeviceResource_ptr	source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset
	)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			renderSystem->GetDeviceResource(destination).As<ID3D12Resource>(),
			destinationOffset,
			source.As<ID3D12Resource>(),
			sourceOffset,
			size);
	}

	void dxDirectContext::CopyBufferRegion(
		DeviceResource_ptr	destination,
		ResourceHandle		source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			destination.As<ID3D12Resource>(),
			destinationOffset,
			renderSystem->GetDeviceResource(source).As<ID3D12Resource>(),
			sourceOffset,
			size);
	}

	void dxDirectContext::CopyBufferRegion(
		DeviceResource_ptr	destination,
		DeviceResource_ptr	source,
		size_t				size,
		size_t				destinationOffset,
		size_t				sourceOffset)
	{
		FlushBarriers();

		DeviceContext->CopyBufferRegion(
			destination.As<ID3D12Resource>(),
			destinationOffset,
			source.As<ID3D12Resource>(),
			sourceOffset,
			size);
	}


	/************************************************************************************************/


	void dxDirectContext::ImmediateWrite(
		static_vector<ResourceHandle>		    handles,
		static_vector<size_t>					value,
		static_vector<DeviceAccessState>		currentStates,
		static_vector<DeviceAccessState>		finalStates)
	{
		DebugBreak();

		FK_ASSERT(handles.size() == currentStates.size(), "Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/
		/*
		DeviceAccessState prevState		= DeviceAccessState::DASERROR;
		ID3D12Resource*		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if(prevResource != resource && prevState != state)
				_AddBarrier(resource, state, DeviceAccessState::DASCopyDest);

			prevResource	= resource;
			prevState		= state;
		}

		FlushBarriers();

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource = renderSystem->GetDeviceResource(handles[itr]);

			D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[] = {
				{resource->GetGPUVirtualAddress() + 0, 0u },
			};

			D3D12_WRITEBUFFERIMMEDIATE_MODE modes[] = {
				D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT
			};

			DeviceContext->WriteBufferImmediate(1, params, nullptr);
		}

		prevState		= DeviceAccessState::DASERROR;
		prevResource	= nullptr;

		for (size_t itr = 0; itr < handles.size(); ++itr)
		{
			auto resource	= renderSystem->GetDeviceResource(handles[itr]);
			auto state		= currentStates[itr];

			if (prevResource != resource && prevState != state)
				_AddBarrier(resource, DeviceAccessState::DASCopyDest, state);

			prevResource	= resource;
			prevState		= prevState;
		}
		*/
	}


	/************************************************************************************************/


	void dxDirectContext::CopyUInt64(
		static_vector<ID3D12Resource*>			sources,
		static_vector<DeviceAccessState>		sourceState,
		static_vector<size_t>					sourceOffsets,
		static_vector<ID3D12Resource*>			destinations,
		static_vector<DeviceAccessState>		destinationState,
		static_vector<size_t>					destinationOffset)
	{
		DebugBreak();

		FK_ASSERT(sources.size()		== sourceState.size(),			"Invalid argument!");
		FK_ASSERT(sources.size()		== sourceOffsets.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationState.size(),		"Invalid argument!");
		FK_ASSERT(destinations.size()	== destinationOffset.size(),	"Invalid argument!");
		FK_ASSERT(sources.size()		== destinations.size(),			"Invalid argument!");

		/*
		typedef struct D3D12_WRITEBUFFERIMMEDIATE_PARAMETER
		{
		D3D12_GPU_VIRTUAL_ADDRESS Dest;
		UINT32 Value;
		} 	D3D12_WRITEBUFFERIMMEDIATE_PARAMETER;
		*/
		/*

		// transition source resources
		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, state, DeviceAccessState::DASCopySrc);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, state, DeviceAccessState::DASCopyDest);
		}

		FlushBarriers();

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			DeviceContext->AtomicCopyBufferUINT64(
				destinations[itr],
				destinationOffset[itr], 
				sources[itr], 
				sourceOffsets[itr], 
				0, 
				nullptr, 
				nullptr);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr) 
		{
			auto resource	= sources[itr];
			auto state		= sourceState[itr];
			_AddBarrier(resource, DeviceAccessState::DASCopySrc, state);
		}

		for (size_t itr = 0; itr < sources.size(); ++itr)
		{
			auto resource	= destinations[itr];
			auto state		= destinationState[itr];
			_AddBarrier(resource, DeviceAccessState::DASCopyDest, state);
		}
		*/
	}


	/************************************************************************************************/


	void dxDirectContext::AddIndexBuffer(TriMesh* Mesh, uint32_t lod)
	{
		const size_t	IBIndex		= Mesh->lods[lod].GetIndexBufferIndex();
		const size_t	IndexCount	= Mesh->lods[lod].GetIndexCount();

		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation	= GetBuffer(Mesh, lod, IBIndex).As<ID3D12Resource>()->GetGPUVirtualAddress();
		IndexView.Format			= DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
		IndexView.SizeInBytes		= (UINT)IndexCount * 4;

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void dxDirectContext::SetIndexBuffer(VertexBufferEntry buffer, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetVertexBufferAddress(buffer.VertexBuffer) + buffer.Offset;
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = (UINT)(renderSystem->GetVertexBufferSize(buffer.VertexBuffer) - buffer.Offset);

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void dxDirectContext::SetIndexBuffer(ResourceHandle resource, DeviceFormat format)
	{
		D3D12_INDEX_BUFFER_VIEW		IndexView;
		IndexView.BufferLocation    = renderSystem->GetDeviceResource(resource).As<ID3D12Resource>()->GetGPUVirtualAddress();
		IndexView.Format            = TextureFormat2DXGIFormat(format);
		IndexView.SizeInBytes       = (UINT)(renderSystem->GetResourceSize(resource));

		DeviceContext->IASetIndexBuffer(&IndexView);
	}


	/************************************************************************************************/


	void dxDirectContext::AddVertexBuffers(TriMesh* mesh, uint32_t lod, const std::initializer_list<VERTEXBUFFER_TYPE>& buffers, VertexBufferList* instanceBuffers)
	{
		AddVertexBuffers(mesh, lod, std::span{ buffers.begin(), buffers.size() }, instanceBuffers);
	}


	/************************************************************************************************/


	void dxDirectContext::AddVertexBuffers(TriMesh* mesh, uint32_t lod, const std::span<const VERTEXBUFFER_TYPE> buffers, VertexBufferList* instanceBuffers)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;

		for (auto& I : buffers)
			FK_ASSERT(AddVertexBuffer(I, mesh, lod, VBViews));

		if (instanceBuffers)
		{
			for (auto& IB : *instanceBuffers)
			{
				VBViews.push_back({
					renderSystem->GetVertexBufferAddress(IB.VertexBuffer) + IB.Offset,
					(UINT)renderSystem->GetVertexBufferSize(IB.VertexBuffer) - IB.Offset,
					IB.Stride });
			}
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferEntry>& list)
	{
		SetVertexBuffers(std::span{ list.begin(), list.end() });
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers(const std::span<const VertexBufferEntry> list)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : list)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetVertexBufferAddress(VB.VertexBuffer) + VB.Offset,
				(UINT)renderSystem->GetVertexBufferSize(VB.VertexBuffer) - +VB.Offset,
				VB.Stride});
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers(const std::initializer_list<VertexBufferResource>& span)
	{
		SetVertexBuffers(std::span{ span.begin(), span.end() });
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers(const std::span<const VertexBufferResource> list)
	{
		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		for (auto& VB : list)
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			VBViews.push_back({
				renderSystem->GetDeviceResource(VB.resource).As<ID3D12Resource>()->GetGPUVirtualAddress() + VB.offset,
				(UINT)renderSystem->GetResourceSize(VB.resource) - VB.offset,
				VB.stride});
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)VBViews.size(), VBViews.begin());
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers2(const std::initializer_list<D3D12_VERTEX_BUFFER_VIEW>& list)
	{
		SetVertexBuffers2(std::span(list.begin(), list.end()));
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers2(const std::span<const D3D12_VERTEX_BUFFER_VIEW> list)
	{
		DeviceContext->IASetVertexBuffers(0, (UINT)list.size(), list.data());
	}


	/************************************************************************************************/


	void dxDirectContext::SetVertexBuffers2(const std::span<const VBView>	views, uint32_t offset)
	{
		D3D12_VERTEX_BUFFER_VIEW apiViews[16];
		for (auto&& [idx, view] : enumerate(views))
		{
			apiViews[idx] = D3D12_VERTEX_BUFFER_VIEW{
				.BufferLocation	= view.buffer,
				.SizeInBytes	= view.size,
				.StrideInBytes	= view.stride,
			};
		}

		DeviceContext->IASetVertexBuffers(0, (UINT)views.size(), apiViews);
	}


	/************************************************************************************************/


	void dxDirectContext::ClearDepthBuffer(ResourceHandle resource, float clearDepth, uint32_t stencil)
	{
		UpdateResourceStates();

		auto descriptor = GetDepthDesciptor(resource);
		PushDepthStencilArray(renderSystem, resource, 0, 0, descriptor);

		DeviceContext->ClearDepthStencilView(
			D3D12_CPU_DESCRIPTOR_HANDLE{ descriptor.V1 },
			D3D12_CLEAR_FLAG_DEPTH, clearDepth, 0, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(resource);
	}


	/************************************************************************************************/


	void dxDirectContext::ClearRenderTarget(ResourceHandle renderTarget, float4 clearColor)
	{
		UpdateResourceStates();

		D3D12_CPU_DESCRIPTOR_HANDLE RTV_CPU_HANDLES{ 0 };

		auto res = std::find_if(
			renderTargetViews.begin(),
			renderTargetViews.end(),
			[&](RTV_View& view)
			{
				return view.resource == renderTarget;
			});

		if (res != renderTargetViews.end())
		{
			RTV_CPU_HANDLES = D3D12_CPU_DESCRIPTOR_HANDLE{ res->descriptor.GetByType<CPUDescriptorHandle>() };
		}
		else
		{
			auto view = ReserveRTV(1);
			PushRenderTarget(renderSystem, renderTarget, view);
			RTV_CPU_HANDLES = D3D12_CPU_DESCRIPTOR_HANDLE{ view.V1 };
			renderTargetViews.push_back({ renderTarget, view });
		}

		DeviceContext->ClearRenderTargetView(RTV_CPU_HANDLES, clearColor, 0, nullptr);

		renderSystem->Textures.MarkRTUsed(renderTarget);
	}


	/************************************************************************************************/


	void dxDirectContext::ClearUAVTextureFloat(ResourceHandle UAV, float4 clearColor)
	{
		auto viewCPU    = ReserveSRVLocal(1);
		auto viewGPU    = ReserveSRV(1).value();
		auto resource   = renderSystem->GetDeviceResource(UAV);

		Texture2D tex{
			renderSystem->GetDeviceResource(UAV).As<ID3D12Resource>(),
			renderSystem->GetResourceWH(UAV),
			renderSystem->GetTextureMipCount(UAV),
			renderSystem->GetResourceDeviceFormat(UAV),
		};

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			viewCPU);

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			viewGPU);

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewFloat(
			D3D12_GPU_DESCRIPTOR_HANDLE{ viewGPU.V2 },
			D3D12_CPU_DESCRIPTOR_HANDLE{ viewCPU.V1 },
			resource.As<ID3D12Resource>(), clearColor, 0, nullptr);
	}

	/************************************************************************************************/


	void dxDirectContext::ClearUAVTextureUint(ResourceHandle UAV, uint4 clearColor)
	{
		auto CPUview	= ReserveSRVLocal(1);
		auto GPUview	= ReserveSRV(1);
		auto resource	= renderSystem->GetDeviceResource(UAV);

		FK_ASSERT(GPUview.has_value() != false, "Failed to allocated descriptor");

		Texture2D tex{
			renderSystem->GetDeviceResource(UAV).As<ID3D12Resource>(),
			renderSystem->GetResourceWH(UAV),
			renderSystem->GetTextureMipCount(UAV),
			renderSystem->GetResourceDeviceFormat(UAV),
		};

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			CPUview);

		PushUAV2DToDescHeap(
			renderSystem,
			tex,
			GPUview.value());

		const auto CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ CPUview.Get<0>() };
		const auto GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ GPUview.value().Get<1>() };

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(
			            GPUHandle, CPUHandle,
			            resource.As<ID3D12Resource>(),
			            (UINT*)&clearColor, 0, nullptr);
	}


	/************************************************************************************************/


	void dxDirectContext::ClearUAV(ResourceHandle resource, uint4 clearColor)
	{
		const auto view				= ReserveSRVLocal(1);
		const auto deviceResource	= renderSystem->GetDeviceResource(resource).As<ID3D12Resource>();
		const auto deviceFormat		= renderSystem->GetResourceDeviceFormat(resource);

		PushUAV1DToDescHeap(renderSystem, deviceResource, deviceFormat, 0, view);

		const auto CPUHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ view.Get<0>() };
		const auto GPUHandle = D3D12_GPU_DESCRIPTOR_HANDLE{ view.Get<1>() };

		FlushBarriers();

		DeviceContext->ClearUnorderedAccessViewUint(GPUHandle, CPUHandle, deviceResource, clearColor, 0, 0);
	}


	/************************************************************************************************/


	void dxDirectContext::ClearUAVBuffer(ResourceHandle UAV, uint4 clearColor)
	{
		BeginEvent_DEBUG("ClearUAVBuffer");

		UpdateResourceStates();

		static auto PSO = static_cast<const dxPipelineState*>(renderSystem->GetPSO(CLEARBUFFERPSO, *renderSystem->allocator));
		DeviceContext->SetPipelineState(PSO->state);
		DeviceContext->SetComputeRoot32BitConstants(0, 4, &clearColor, 0);
		DeviceContext->SetComputeRootUnorderedAccessView(1, renderSystem->GetDeviceResource(UAV).As<ID3D12Resource>()->GetGPUVirtualAddress());

		auto resourceSize = renderSystem->GetResourceSize(UAV);

		uint2 range{ 0, resourceSize / 16 };
		DeviceContext->SetComputeRoot32BitConstants(0, 2, &range, 4);

		DeviceContext->Dispatch(UINT(ceil(resourceSize / 1024.0f)), 1, 1);

		if(CurrentComputeRootSignature)
			DeviceContext->SetComputeRootSignature(*CurrentComputeRootSignature);

		if(CurrentPipelineState)
			DeviceContext->SetPipelineState(CurrentPipelineState);

		EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void dxDirectContext::ClearUAVBufferRange(ResourceHandle UAV, uint begin, uint end, uint4 clearColor)
	{
		FK_ASSERT(begin % 16 == 0, "Begin must be 16-byte aligned");
		FK_ASSERT(end % 16 == 0, "End must be 16-byte aligned");

		BeginEvent_DEBUG("ClearUAVBuffer");

		UpdateResourceStates();

		end = Min((uint32_t)renderSystem->GetResourceSize(UAV), end);
		uint2 range{ begin / 16, end / 16};

		auto PSO = static_cast<const dxPipelineState*>(renderSystem->GetPSO(CLEARBUFFERPSO, *renderSystem->allocator));
		auto pipelineinterface = PSO->rootSignature;

		DeviceContext->SetComputeRootSignature(pipelineinterface->GetAPIObject().As<ID3D12RootSignature>());
		DeviceContext->SetPipelineState(PSO->state);
		DeviceContext->SetComputeRoot32BitConstants(0, 4, &clearColor, 0);
		DeviceContext->SetComputeRoot32BitConstants(0, 2, &range, 4);
		DeviceContext->SetComputeRootUnorderedAccessView(1, renderSystem->GetDeviceResource(UAV).As<ID3D12Resource>()->GetGPUVirtualAddress());

		auto resourceSize = renderSystem->GetResourceSize(UAV);
		DeviceContext->Dispatch(UINT(ceil(Min(resourceSize, end - begin) / 1024.0f)), 1, 1);

		if(CurrentComputeRootSignature)
			DeviceContext->SetComputeRootSignature(*CurrentComputeRootSignature);

		if(CurrentPipelineState)
			DeviceContext->SetPipelineState(CurrentPipelineState);

		EndEvent_DEBUG();
	}


	/************************************************************************************************/


	void dxDirectContext::ResolveQuery(QueryHandle query, size_t begin, size_t end, ResourceHandle destination, size_t destOffset)
	{
		if (query == InvalidHandle)
			return;

		auto res			= renderSystem->GetDeviceResource(destination).As<ID3D12Resource>();
		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetDeviceObject(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, (UINT)begin, (UINT)(end - begin), res, (UINT)destOffset);
	}


	/************************************************************************************************/


	void dxDirectContext::ResolveQuery(QueryHandle query, size_t begin, size_t end, DeviceResource_ptr destination, size_t destOffset)
	{
		if (query == InvalidHandle)
			return;

		auto type			= renderSystem->Queries.GetType(query);
		auto queryResource	= renderSystem->Queries.GetDeviceObject(query);

		UpdateResourceStates();

		DeviceContext->ResolveQueryData(queryResource, type, (UINT)begin, (UINT)(end - begin), destination.As<ID3D12Resource>(), (UINT)destOffset);
	}


	/************************************************************************************************/


	void dxDirectContext::ExecuteIndirect(ResourceHandle args, const IndirectLayout& layout, size_t argumentBufferOffset, size_t executionCount)
	{
		UpdateResourceStates();
		auto& impl = dxIndirectLayout::GetImpl(layout);

		DeviceContext->ExecuteIndirect(
			impl.signature,
			(UINT)Min(impl.entries.size(), executionCount),
			renderSystem->GetDeviceResource(args).As<ID3D12Resource>(),
			(UINT)argumentBufferOffset,
			nullptr, 
			0);
	}


	void dxDirectContext::Draw(const size_t VertexCount, const size_t BaseVertex, const size_t baseIndex)
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced((UINT)VertexCount, 1, (UINT)BaseVertex, (UINT)baseIndex);
	}


	void dxDirectContext::DrawInstanced(const size_t vertexCount, const size_t baseVertex, const size_t instanceCount, const size_t instanceOffset )
	{
		UpdateResourceStates();
		DeviceContext->DrawInstanced((UINT)vertexCount, (UINT)instanceCount, (UINT)baseVertex, (UINT)instanceOffset);
	}


	void dxDirectContext::DrawIndexed(const size_t IndexCount, const size_t IndexOffet, const size_t BaseVertex)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced((UINT)IndexCount, 1, (UINT)IndexOffet, (UINT)BaseVertex, 0);
	}


	void dxDirectContext::DrawIndexedInstanced(
		const size_t IndexCount, const size_t IndexOffet, 
		const size_t BaseVertex, const size_t InstanceCount, 
		const size_t InstanceOffset)
	{
		UpdateResourceStates();
		DeviceContext->DrawIndexedInstanced(
			(UINT)IndexCount, (UINT)InstanceCount,
			(UINT)IndexOffet, (UINT)BaseVertex,
			(UINT)InstanceOffset);
	}


	/************************************************************************************************/


	void dxDirectContext::Dispatch(const uint3 xyz)
	{
		UpdateResourceStates();
		DeviceContext->Dispatch((UINT)xyz[0], (UINT)xyz[1], (UINT)xyz[2]);
	}


	/************************************************************************************************/


	void dxDirectContext::DispatchRays(const uint3 WHD, const DispatchDesc desc)
	{
		UpdateResourceStates();

		D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
		dispatchDesc.Width  = WHD[0];
		dispatchDesc.Height = WHD[1];
		dispatchDesc.Depth  = WHD[2];

		dispatchDesc.CallableShaderTable		= DeviceAddressRangeStrideToDX(desc.callableShaderTable);
		dispatchDesc.HitGroupTable				= DeviceAddressRangeStrideToDX(desc.hitGroupTable);
		dispatchDesc.MissShaderTable			= DeviceAddressRangeStrideToDX(desc.missTable);
		dispatchDesc.RayGenerationShaderRecord	= DeviceAddressRangeToDX(desc.rayGenerationRecord);

		DeviceContext->DispatchRays(&dispatchDesc);
	}


	/************************************************************************************************/


	void dxDirectContext::DispatchMesh(const uint3 xyz)
	{
		UpdateResourceStates();
		DeviceContext->DispatchMesh(xyz[0], xyz[1], xyz[2]);
	}


	/************************************************************************************************/


	void dxDirectContext::FlushBarriers() noexcept
	{
		UpdateResourceStates();
	}

	/************************************************************************************************/


	void dxDirectContext::SetPredicate(bool Enabled, ResourceHandle handle, size_t Offset, PredicateOp op)
	{
		if (Enabled)
			DeviceContext->SetPredication(
				renderSystem->GetDeviceResource(handle).As<ID3D12Resource>(),
				Offset * 8, 
				op == PredicateOp::NotEqualZero ? D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_NOT_EQUAL_ZERO : D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
		else
			DeviceContext->SetPredication(nullptr, 0, D3D12_PREDICATION_OP::D3D12_PREDICATION_OP_EQUAL_ZERO);
	}


	/************************************************************************************************/


	void dxDirectContext::CopyBuffer(const UploadReservation src, const ResourceHandle destination, const size_t destOffset)
	{
		const auto destinationResource	= renderSystem->GetDeviceResource(destination).As<ID3D12Resource>();
		const auto sourceResource       = src.resource.As<ID3D12Resource>();

		UpdateResourceStates();

		DeviceContext->CopyBufferRegion(destinationResource, destOffset, sourceResource, src.offset, src.size);
	}


	/************************************************************************************************/


	void dxDirectContext::CopyTexture2D(const UploadReservation src, const ResourceHandle destination, const uint2 BufferSize)
	{
		const auto destinationResource		= renderSystem->GetDeviceResource(destination).As<ID3D12Resource>();
		const auto WH						= renderSystem->GetResourceWH(destination);
		const auto format					= renderSystem->GetResourceDeviceFormat(destination);
		const auto texelSize				= renderSystem->GetResourceElementSize(destination);

		D3D12_TEXTURE_COPY_LOCATION destLocation{};
		destLocation.pResource			= destinationResource;
		destLocation.SubresourceIndex	= 0;
		destLocation.Type				= D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;


		D3D12_TEXTURE_COPY_LOCATION srcLocation{};
		srcLocation.pResource							= src.resource.As<ID3D12Resource>();
		srcLocation.Type								= D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint.Offset				= src.offset;
		srcLocation.PlacedFootprint.Footprint.Depth		= 1;
		srcLocation.PlacedFootprint.Footprint.Format	= format;
		srcLocation.PlacedFootprint.Footprint.Height	= WH[1];
		srcLocation.PlacedFootprint.Footprint.Width		= WH[0];
		srcLocation.PlacedFootprint.Footprint.RowPitch	= (UINT)(BufferSize[0] * texelSize);

		UpdateResourceStates();
		DeviceContext->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}


	/************************************************************************************************/


    void dxDirectContext::CopyTexture2D(auto des, auto src)
	{
		FlushBarriers();

		DeviceContext->CopyResource(
			renderSystem->GetDeviceResource(des),
			renderSystem->GetDeviceResource(src));
	}


	/************************************************************************************************/


	void dxDirectContext::Clear()
	{
		pendingBarriers.clear();
		RenderTargets.clear();
		DesciptorHeaps.clear();
		VBViews.clear();

		CurrentPipelineState = nullptr;

		DeviceContext->ClearState(nullptr);

		depthStencilViews.clear();
		renderTargetViews.clear();
	}


	/************************************************************************************************/


	void dxDirectContext::SetRTRead(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetRTWrite(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetRTFree(ResourceHandle Handle)
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void dxDirectContext::Close()
	{
		renderSystem->ReleaseDescriptorRange(shaderResources, dispatchIdx);
		shaderResources = {};

		if (auto HR = DeviceContext->Close(); FAILED(HR)) {
			FK_LOG_ERROR("Failed to close graphics context!");
			renderSystem->_OnCrash();
		}
	}


	/************************************************************************************************/


	dxDirectContext& dxDirectContext::Reset(DescriptorRange range, const size_t newDispatchIdx, ID3D12DescriptorHeap* heap)
	{
		shaderResources = range;

		CurrentPipelineState		= nullptr;
		CurrentRootSignature		= nullptr;
		CurrentComputeRootSignature = nullptr;

		if (FAILED(commandAllocator->Reset()))
		{
			DebugBreak();
			FK_LOG_ERROR("Failed to reset command allocator");
		}

		if (FAILED(DeviceContext->Reset(commandAllocator, nullptr)))
		{
			DebugBreak();
			FK_LOG_ERROR("Failed to reset device context");
		}

		ResetDSV();
		ResetRTV();
		ResetSRV();

		pendingBarriers.clear();
		queuedBarriers.clear();
		renderTargetViews.clear();
		depthStencilViews.clear();
		queuedReadBacks.clear();

		heapUsed	= 0;
		dispatchIdx	= newDispatchIdx;

		DeviceContext->SetDescriptorHeaps(1, &heap);

		return *this;
	}


	/************************************************************************************************/


	void dxDirectContext::SetDebugName(const char* ID) noexcept
	{
		SETDEBUGNAME(DeviceContext, ID);
	}


	/************************************************************************************************/


	UploadReservation dxDirectContext::ReserveDirectUploadSpace(size_t size, size_t alignment) noexcept
	{
		return renderSystem->ReserveDirectUploadSpace(size, alignment);
	}


	/************************************************************************************************/


	void dxDirectContext::SetUAVRead() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetUAVWrite() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	void dxDirectContext::SetUAVFree() 
	{
		FK_ASSERT(0);
	}


	/************************************************************************************************/


	IRenderSystem& dxDirectContext::GetRenderSystem() noexcept
	{
		return dxRenderSystem::_GetInstance();
	}


	/************************************************************************************************/


	void dxDirectContext::BeginMarker(const char* str)
	{
	}


	/************************************************************************************************/


	void dxDirectContext::EndMarker(const char* str)
	{

	}


	/************************************************************************************************/


	DescHeapPOS dxDirectContext::GetDepthDesciptor(ResourceHandle depthBuffer)
	{
		auto DSV_CPU_HANDLE = DescHeapPOS{};

		if (auto res = std::find_if(
			depthStencilViews.begin(),
			depthStencilViews.end(),
			[&](RTV_View& view)
			{
				return view.resource == depthBuffer;
			});
			res == depthStencilViews.end())
		{
			if (!depthStencilViews.full()) {
				auto DSV        = ReserveDSV(1);
				DSV_CPU_HANDLE  = DSV;
				depthStencilViews.push_back({ depthBuffer, DSV });
			}
			else
				DSV_CPU_HANDLE = depthStencilViews[rand() % depthStencilViews.size()].descriptor;
		}
		else
			DSV_CPU_HANDLE = res->descriptor;

		return DSV_CPU_HANDLE;
	}


	/************************************************************************************************/


	void dxDirectContext::UpdateResourceStates()
	{
		if (!pendingBarriers.size())
			return;

		static_vector<D3D12_GLOBAL_BARRIER, 32>		globalBarriers;
		static_vector<D3D12_TEXTURE_BARRIER, 32>	textureBarriers;
		static_vector<D3D12_BUFFER_BARRIER, 32>		bufferBarriers;

		for (const Barrier& barrier : pendingBarriers)
		{
			switch(barrier.type)
			{
			case BarrierType::Global:
			{
				D3D12_GLOBAL_BARRIER globalBarrier;
				globalBarrier.AccessBefore	= DAS2AccessState(barrier.accessBefore);
				globalBarrier.AccessAfter	= DAS2AccessState(barrier.accessAfter);
				globalBarrier.SyncBefore	= SyncPoint2DX_Forward(barrier.src);
				globalBarrier.SyncAfter		= SyncPoint2DX_Backward(barrier.dst);

				globalBarriers.push_back(globalBarrier);
			}	break;
			case BarrierType::Buffer:
			{
#ifdef USING(DEBUGGRAPHICS)
				FK_ASSERT(
					renderSystem->GetResourceDimension(barrier.resource) == ResourceDimension::Buffer ||
					renderSystem->GetResourceDimension(barrier.resource) == ResourceDimension::AccelerationStructure);
#endif

				D3D12_BUFFER_BARRIER bufferBarrier;
				bufferBarrier.pResource		= renderSystem->GetDeviceResource(barrier.resource).As<ID3D12Resource>();
				bufferBarrier.AccessBefore	= DAS2AccessState(barrier.accessBefore);
				bufferBarrier.AccessAfter	= DAS2AccessState(barrier.accessAfter);
				bufferBarrier.SyncBefore	= SyncPoint2DX_Forward(barrier.src);
				bufferBarrier.SyncAfter		= SyncPoint2DX_Backward(barrier.dst);
				bufferBarrier.Offset		= 0;// barrier.buffer.rangeBegin;
				bufferBarrier.Size			= UINT64_MAX;//barrier.buffer.rangeEnd - barrier.buffer.rangeBegin;

				bufferBarriers.push_back(bufferBarrier);
			}	break;
			case BarrierType::Texture:
			{
#ifdef USING(DEBUGGRAPHICS)
				auto dimension = renderSystem->GetResourceDimension(barrier.resource);
				FK_ASSERT(
					dimension == ResourceDimension::Texture1D ||
					dimension == ResourceDimension::Texture2D ||
					dimension == ResourceDimension::Texture2DArray ||
					dimension == ResourceDimension::Texture3D ||
					dimension == ResourceDimension::TextureCubeMap);
#endif

				D3D12_TEXTURE_BARRIER textureBarrier;
				textureBarrier.AccessBefore		= DAS2AccessState(barrier.accessBefore);
				textureBarrier.AccessAfter		= DAS2AccessState(barrier.accessAfter);
				textureBarrier.LayoutBefore		= DeviceLayout2DX(barrier.texture.layoutBefore);
				textureBarrier.LayoutAfter		= DeviceLayout2DX(barrier.texture.layoutAfter);
				textureBarrier.Flags			= (D3D12_TEXTURE_BARRIER_FLAGS)barrier.texture.flags;
				textureBarrier.pResource		= renderSystem->GetDeviceResource(barrier.resource).As<ID3D12Resource>();
				textureBarrier.SyncBefore		= SyncPoint2DX_Backward(barrier.src);
				textureBarrier.SyncAfter		= SyncPoint2DX_Forward(barrier.dst);

				textureBarrier.Subresources		= D3D12_BARRIER_SUBRESOURCE_RANGE{
					.IndexOrFirstMipLevel	= 0,
					.NumMipLevels			= renderSystem->GetTextureMipCount(barrier.resource),
					.FirstArraySlice		= 0,
					.NumArraySlices			= (uint32_t)renderSystem->GetTextureArraySize(barrier.resource),
					.FirstPlane				= 0,
					.NumPlanes				= 1,
				};

				textureBarriers.push_back(textureBarrier);
			}	break;
			default:
				FK_ASSERT(0);
			};
		}

		static_vector<D3D12_BARRIER_GROUP>		groups;
		if (globalBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_GLOBAL, .NumBarriers = (uint32_t)globalBarriers.size(), .pGlobalBarriers = globalBarriers.data()});
		if (textureBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_TEXTURE, .NumBarriers = (uint32_t)textureBarriers.size(), .pTextureBarriers = textureBarriers.data() });
		if (bufferBarriers.size())	groups.emplace_back(D3D12_BARRIER_GROUP{ .Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_BUFFER, .NumBarriers =  (uint32_t)bufferBarriers.size(), .pBufferBarriers = bufferBarriers.data() });

		if(groups.size())
			DeviceContext->Barrier((uint32_t)groups.size(), groups);

		pendingBarriers.clear();
	}


	/************************************************************************************************/


	void dxDirectContext::QueueReadBacks()
	{
		for (const auto readBackHandle : queuedReadBacks)
		{
			auto& readBack	= renderSystem->ReadBackTable[readBackHandle];
			auto fence		= renderSystem->directFence;

			auto HR	= fence->SetEventOnCompletion(dispatchIdx, readBack.event); FK_ASSERT(SUCCEEDED(HR));
			renderSystem->GraphicsQueue->Signal(fence, dispatchIdx);

			readBack.queueUntil	= dispatchIdx;
			readBack.queued		= true;
		}

		queuedReadBacks.clear();
	}


	/************************************************************************************************/


	DescHeapPOS dxDirectContext::ReserveDSV(size_t count)
	{
		auto currentCPU = CPUDescriptorHandle{ DSV_CPU.ptr };
		DSV_CPU.ptr = DSV_CPU.ptr + renderSystem->DescriptorDSVSize * count;

		return { currentCPU, InvalidHandle };
	}


	/************************************************************************************************/


	std::optional<DescHeapPOS> dxDirectContext::ReserveSRV(size_t count)
	{
		if (shaderResources.size > heapUsed + count)
		{
			auto out = shaderResources[heapUsed];

			heapUsed += count;
			return { out };
		}
		else
		{
			DebugBreak();
			return {};
		}
	}



	/************************************************************************************************/


	DescHeapPOS dxDirectContext::ReserveSRVLocal(size_t count)
	{
		auto currentCPU = SRV_LOCAL_CPU;
		SRV_LOCAL_CPU.ptr = SRV_LOCAL_CPU.ptr + renderSystem->DescriptorCBVSRVUAVSize * count;

		return { CPUDescriptorHandle{ currentCPU.ptr }, InvalidHandle };
	}


	/************************************************************************************************/


	DescHeapPOS dxDirectContext::ReserveRTV(size_t count)
	{
		auto currentCPU = RTV_CPU;
		RTV_CPU.ptr = RTV_CPU.ptr + renderSystem->DescriptorRTVSize * count;

		return { CPUDescriptorHandle{ currentCPU.ptr }, InvalidHandle };
	}


	/************************************************************************************************/


	void dxDirectContext::ResetRTV()
	{
		RTV_CPU = descHeapRTV->GetCPUDescriptorHandleForHeapStart();

		renderTargetViews.clear();
	}


	/************************************************************************************************/


	void dxDirectContext::ResetDSV()
	{
		DSV_CPU = descHeapDSV->GetCPUDescriptorHandleForHeapStart();

		depthStencilViews.clear();
	}


	/************************************************************************************************/


	void dxDirectContext::ResetSRV()
	{
		heapUsed = 0;

		SRV_LOCAL_CPU = descHeapSRVLocal->GetCPUDescriptorHandleForHeapStart();
	}


    /************************************************************************************************/


	void CopyContext::Barrier(ID3D12Resource* resource, const DeviceAccessState before, const DeviceAccessState after)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			DRS2D3DState(before),
			DRS2D3DState(after));


		if (pendingBarriers.full())
			flushPendingBarriers();

		pendingBarriers.push_back(barrier);
	}


	/************************************************************************************************/


	void CopyContext::Barrier(ResourceHandle destination, DeviceAccessState before, DeviceAccessState after)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			GetRenderSystem().GetDeviceResource(destination).As<ID3D12Resource>(),
			DRS2D3DState(before),
			DRS2D3DState(after));


		if (pendingBarriers.full())
			flushPendingBarriers();

		pendingBarriers.push_back(barrier);
	}


	/************************************************************************************************/


	void CopyContext::flushPendingBarriers()
	{
		if (pendingBarriers.empty())
			return;

		commandList->ResourceBarrier((UINT)pendingBarriers.size(), pendingBarriers.data());
		pendingBarriers.clear();
	}


	/************************************************************************************************/


	IRenderSystem& CopyContext::GetRenderSystem() noexcept
	{
		return dxRenderSystem::_GetInstance();
	}



	/************************************************************************************************/


	UploadReservation CopyContext::Reserve(const size_t reserveSize, const uint32_t reserveAlignement)
	{
		// Not enough remaining space in Buffer GOTO Beginning if space in front of upload buffer is available
		if	(uploadBuffer.position + reserveSize > uploadBuffer.size && uploadBuffer.last != 0)
			uploadBuffer.position = 0;

		auto GetOffset = [&]() {
			auto offset = reserveAlignement - (uploadBuffer.position & (reserveAlignement - 1));
			return (offset == reserveAlignement) ? 0 : offset;
		};

		auto ResizeBuffer = [&] {
			const auto newSize = (size_t )std::pow(2, std::floor(std::log2(reserveSize)) + 1);
			freeResources.push_back(uploadBuffer.Resize(newSize));
		};

		// Buffer too small
		auto temp = reserveSize + GetOffset();
		if (uploadBuffer.position + reserveSize + GetOffset() > uploadBuffer.size)
			ResizeBuffer();

		if (uploadBuffer.last > uploadBuffer.position)
		{	// Potential Overlap condition
			if (uploadBuffer.position + reserveSize + GetOffset() >= uploadBuffer.last)
				ResizeBuffer();  // Resize Buffer and then upload

			const auto alignmentOffset  = GetOffset();
			char*           buffer      = uploadBuffer.buffer + uploadBuffer.position + alignmentOffset;
			const size_t    offset      = uploadBuffer.position + alignmentOffset;

			uploadBuffer.position += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= uploadBuffer.deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer
			};
		}

		if(uploadBuffer.last <= uploadBuffer.position)
		{	// Safe, Do Upload
			const auto alignmentOffset = GetOffset();

			char* buffer            = uploadBuffer.buffer + uploadBuffer.position + alignmentOffset;
			size_t offset           = uploadBuffer.position + alignmentOffset;
			uploadBuffer.position  += reserveSize + alignmentOffset;

			return UploadReservation{
				.resource	= uploadBuffer.deviceBuffer,
				.size		= reserveSize,
				.offset		= offset,
				.buffer		= buffer
			};
		}

		return {};
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(GPURange destRange, void* source_ptr, uint64_t size)
	{
		auto uploadSize		= Min(destRange.size, size);
		auto uploadSpace	= Reserve(uploadSize);

		ID3D12Resource* apiResource = nullptr;
		if (destRange.resource != InvalidHandle)
			apiResource = (ID3D12Resource*)dxRenderSystem::globalInstance->GetDevicePointer(destRange.resource);
		else
			apiResource = (ID3D12Resource*)destRange.devicePtr;

		memcpy(uploadSpace.buffer, source_ptr, size);

		commandList->CopyBufferRegion(
			apiResource,
			destRange.offset,
			uploadSpace.resource.As<ID3D12Resource>(),
			uploadSpace.offset,
			uploadSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ResourceHandle handle, const size_t destOffset, UploadReservation source)
	{
		flushPendingBarriers();

		auto dest = dxRenderSystem::globalInstance->GetDeviceResource(handle);

		commandList->CopyBufferRegion(
			dest.As<ID3D12Resource>(),
			destOffset,
			source.resource.As<ID3D12Resource>(),
			source.offset,
			source.size);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, UploadReservation source)
	{
		flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source.resource.As<ID3D12Resource>(),
			source.offset,
			source.size);
	}


	/************************************************************************************************/


	void CopyContext::CopyBuffer(ID3D12Resource* destination, const size_t destinationOffset, ID3D12Resource* source, const size_t sourceOffset, const size_t sourceSize)
	{
		flushPendingBarriers();

		commandList->CopyBufferRegion(
			destination,
			destinationOffset,
			source,
			sourceOffset,
			sourceSize);
	}


	/************************************************************************************************/


	void CopyContext::CopyTextureRegion(
		ID3D12Resource*		destination,
		size_t				subResourceIdx,
		uint3				XYZ,
		UploadReservation	source,
		uint2				WH,
		DeviceFormat		format)
	{
		flushPendingBarriers();

		const auto		deviceFormat	= TextureFormat2DXGIFormat(format);
		const size_t	formatSize		= GetFormatElementSize(deviceFormat);
		const bool		BCformat		= IsDDS(format);
		const size_t	rowPitch		= AlignedSize(BCformat ? formatSize * WH[0] / 4 : formatSize * WH[0]);
		//size_t alignmentOffset    = rowPitch & 0x01ff;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT SubRegion;
		SubRegion.Footprint.Depth		= 1;
		SubRegion.Footprint.Format		= deviceFormat;
		SubRegion.Footprint.RowPitch	= (UINT)rowPitch;
		SubRegion.Footprint.Width		= WH[0];
		SubRegion.Footprint.Height		= WH[1];
		SubRegion.Offset				= source.offset;

		auto destinationLocation	= CD3DX12_TEXTURE_COPY_LOCATION(destination, (UINT)subResourceIdx);
		auto sourceLocation			= CD3DX12_TEXTURE_COPY_LOCATION(source.resource.As<ID3D12Resource>(), SubRegion);

		commandList->CopyTextureRegion(
			&destinationLocation,
			XYZ[0], XYZ[1], XYZ[2],
			&sourceLocation,
			nullptr);
	}


	/************************************************************************************************/


	void CopyContext::CopyTile(ID3D12Resource* dest, const uint3 destTile, const size_t tileOffset, const UploadReservation src)
	{
		flushPendingBarriers();

		auto desc = dest->GetDesc();

		D3D12_TILED_RESOURCE_COORDINATE coordinate;
		coordinate.X			= (UINT)destTile[0];
		coordinate.Y			= (UINT)destTile[1];
		coordinate.Z			= (UINT)0;
		coordinate.Subresource	= (UINT)destTile[2];
		
		D3D12_TILE_REGION_SIZE regionSize;
		regionSize.NumTiles		= 1;
		regionSize.UseBox		= false;
		regionSize.Width		= 1;
		regionSize.Height		= 1;
		regionSize.Depth		= 1;

		commandList->CopyTiles(
			dest,
			&coordinate,
			&regionSize,
			src.resource.As<ID3D12Resource>(),
			src.offset,
			D3D12_TILE_COPY_FLAG_LINEAR_BUFFER_TO_SWIZZLED_TILED_RESOURCE);
	}


	/************************************************************************************************/


	bool CopyContext::IsSubResourceTiled(ID3D12Resource* resource, const size_t level) const
	{
		ID3D12Device* device = nullptr;
		commandList->GetDevice(IID_PPV_ARGS(&device));

		UINT						TileCount = 0;
		D3D12_PACKED_MIP_INFO		packedMipInfo;
		D3D12_TILE_SHAPE			TileShape;
		UINT						subResourceTilingCount = 1;
		D3D12_SUBRESOURCE_TILING	subResourceTiling_Packed;

		device->GetResourceTiling(resource, &TileCount, &packedMipInfo, &TileShape, &subResourceTilingCount, (UINT)level, &subResourceTiling_Packed);

		return (subResourceTiling_Packed.HeightInTiles * subResourceTiling_Packed.WidthInTiles) != 0;
	}


}	/************************************************************************************************/
