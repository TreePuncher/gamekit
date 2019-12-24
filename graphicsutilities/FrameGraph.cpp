#include "graphics.h"
#include "FrameGraph.h"
#include "..\coreutilities\Logging.h"

namespace FlexKit
{	/************************************************************************************************/


	ResourceTransition::ResourceTransition(FrameObjectDependency& Dep) :
		Object		{ Dep.FO },
		BeforeState	{ Dep.ExpectedState },
		AfterState	{ Dep.State}{}


	/************************************************************************************************/


	void ResourceTransition::ProcessTransition(FrameResources& Resources, Context* ctx) const
	{
		switch (Object->Type)
		{
		case OT_StreamOut:
		{
			switch (AfterState)
			{
			case DRS_VERTEXBUFFER:
			case DRS_STREAMOUT: {
				ctx->AddStreamOutBarrier(
					Object->SOBuffer,
					BeforeState,
					AfterState);
			}	break;
			}
		}	break;

		case OT_ShaderResource:
		{
			switch (AfterState)
			{
			case DRS_Write:
            case DRS_ShaderResource:
            case DRS_RenderTarget:
				ctx->AddShaderResourceBarrier(
					handle_cast<ResourceHandle>(Object->ShaderResource.handle),
					BeforeState,
					AfterState);
				break;
			default:
				FK_ASSERT(0);
			}
		}	break;
		case OT_UAVBuffer:
		{
			switch (AfterState)
			{
			case DRS_Write:
			case DRS_ShaderResource:
			case DRS_UAV:
			case DRS_STREAMOUT:
			case DRS_STREAMOUTCLEAR:
			{
				ctx->AddUAVBarrier(
					Object->UAVBuffer,
					BeforeState,
					AfterState);
			}	break;
			default:
				FK_ASSERT(0);
			}
		}	break;
		case OT_UAVTexture:
		{
			switch (AfterState)
			{
			case DRS_Write:
			case DRS_ShaderResource:
            case DRS_UAV:
            case DRS_RenderTarget:
			{
				ctx->AddUAVBarrier(
					Object->UAVTexture,
					BeforeState,
					AfterState);
			}	break;
			default:
				FK_ASSERT(0);
			}
		}	break;
		default:
			switch (AfterState)
			{
			case DRS_Present:
			{
				ctx->AddPresentBarrier(
					Resources.GetRenderTarget(Object->Handle),
					BeforeState);
			}	break;
			case DRS_DEPTHBUFFERWRITE:
			case DRS_RenderTarget:
				ctx->AddRenderTargetBarrier(
					Resources.GetRenderTarget(Object->Handle),
					BeforeState,
					AfterState);
				break;
			case DRS_STREAMOUT:
			case DRS_CONSTANTBUFFER:
			case DRS_PREDICATE:
			case DRS_INDIRECTARGS:
			case DRS_UNKNOWN:
			default:
				FK_ASSERT(0); // Unimplemented
			}

		}


	}


	/************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context* Ctx)
	{
		for (const auto& T : Transitions) {
			T.ProcessTransition(Resources, Ctx);

			auto stateObject	= *T.Object;
			stateObject.State	= T.AfterState;
			auto idx			= Resources.SubNodeTracking.push_back(stateObject);
		}
	}


	/************************************************************************************************/


	void FrameGraphNode::AddTransition(FrameObjectDependency& Dep)
	{
		Transitions.push_back(Dep);
	}


	/************************************************************************************************/


	bool FrameGraphNode::DependsOn(uint32_t Tag)
	{
		auto Pred = [&](const auto& RHS) -> bool {	return RHS.FO->Tag == Tag;	};

		return (find(InputObjects, Pred) != InputObjects.end());
	}


	/************************************************************************************************/


	bool FrameGraphNode::Outputs(uint32_t Tag)
	{
		auto Pred = [&](const auto& RHS) -> bool {	return RHS.FO->Tag == Tag;	};

		return (find(OutputObjects, Pred) != OutputObjects.end());
	}


	/************************************************************************************************/


	void FrameGraphNode::RestoreResourceStates(Context* ctx, PassObjectList& locallyTrackedObjects)
	{
		for (auto& resNode : locallyTrackedObjects)
		{
			auto res = find(OutputObjects,
				[&](FrameObjectDependency& rhs) -> bool
				{
					return rhs.FO->Handle == resNode.Handle;
				});

			if (res!= OutputObjects.end() && res->State != resNode.State)
			{
				switch (resNode.Type)
				{
				case FrameObjectResourceType::OT_StreamOut:
					ctx->AddStreamOutBarrier(resNode.SOBuffer, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_UAVTexture:
					ctx->AddUAVBarrier(resNode.UAVTexture, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_UAVBuffer:
					ctx->AddUAVBarrier(resNode.UAVBuffer, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_BackBuffer:
				case FrameObjectResourceType::OT_ConstantBuffer:
				case FrameObjectResourceType::OT_DepthBuffer:
				case FrameObjectResourceType::OT_IndirectArguments:
				case FrameObjectResourceType::OT_PVS:
				case FrameObjectResourceType::OT_RenderTarget:
				case FrameObjectResourceType::OT_ShaderResource:
				case FrameObjectResourceType::OT_VertexBuffer:
					FK_ASSERT(0, "UN-IMPLEMENTED BLOCK!");
				}
			}
		}

		locallyTrackedObjects.clear();

		return;
	}


	/************************************************************************************************/


	void FrameGraph::AddRenderTarget(ResourceHandle Texture)
	{

	}


	/************************************************************************************************/

	
	void FrameGraph::UpdateFrameGraph(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp)
	{
		RS->SubmitUploadQueues();
		RS->BeginSubmission(Window);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::BuildNode(FrameGraph* FrameGraph)
	{	// Builds Nodes Linkages, Transitions
		auto CheckNodeSource = [](FrameGraphNode& Node, FrameObjectDependency& Resource)
		{
			if (//Input.SourceObject != &Node &&
				Resource.Source&&
				!IsXInSet(Resource.Source, Node.Sources))
			{
				Node.Sources.push_back(Resource.Source);
			}
		};

		// Process Transitions
		for (auto& Object : Transitions)
		{
			CheckNodeSource(Node, Object);
			Object.Source = &Node;

			if(Object.State == DRS_Free)
				Context.Retire(Object);
			else
				if(Object.ExpectedState != Object.State)
					Node.AddTransition(Object);
		}

		for (auto& SourceNode : LocalInputs)
			CheckNodeSource(Node, SourceNode);

		Node.InputObjects	= std::move(LocalInputs);
		Node.OutputObjects	= std::move(LocalOutputs);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::AddDataDependency(UpdateTask& task)
	{
		DataDependencies.push_back(&task);
	}


	FrameResourceHandle FrameGraphNodeBuilder::ReadShaderResource(ResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceResourceState::DRS_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteShaderResource(ResourceHandle  handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_Write);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadRenderTarget(ResourceHandle RenderTarget)
	{
		return AddReadableResource(RenderTarget, DRS_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(ResourceHandle RenderTarget)
	{
		const auto resourceHandle   = AddWriteableResource(RenderTarget, DeviceResourceState::DRS_RenderTarget);
        auto resource               = Resources->GetAssetObject(resourceHandle);

        if (!resource)
            return InvalidHandle_t;

        if (resource->Type == FrameObjectResourceType::OT_ShaderResource)
            resource->ShaderResource.renderTargetUse = true;

        return resourceHandle;
	}


    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(UAVTextureHandle handle)
    {
        const auto resourceHandle   = AddWriteableResource(handle, DeviceResourceState::DRS_RenderTarget);
        auto resource               = Resources->GetAssetObject(resourceHandle);

        FK_ASSERT(resource != nullptr);

        resource->UAVTexture.renderTargetUse = true;

        return resourceHandle;
    }


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::PresentBackBuffer(ResourceHandle renderTarget)
	{
		return renderTarget != InvalidHandle_t ? AddReadableResource(renderTarget, DeviceResourceState::DRS_Present) : InvalidHandle_t;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteBackBuffer(ResourceHandle handle)
	{
		FK_ASSERT(handle != InvalidHandle_t);
		return AddWriteableResource(handle, DeviceResourceState::DRS_RenderTarget);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadBackBuffer(ResourceHandle handle)
	{
		FrameResourceHandle Resource = Resources->FindFrameResource(handle);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetAssetObject(Resource),
			nullptr,
			Resources->GetAssetObjectState(Resource),
			DeviceResourceState::DRS_ShaderResource });

		return Resource;
	}


	/************************************************************************************************/

	/*
	FrameResourceHandle	FrameGraphNodeBuilder::ReadDepthBuffer(ResourceHandle Handle)
	{
		auto Resource = Resources->FindRenderTarGetAsset(Handle);
		LocalInputs.push_back(FrameObjectDependency{
			Resources->GetAssetObject(Resource),
			nullptr,
			Resources->GetAssetObjectState(Resource),
			DeviceResourceState::DRS_DEPTHBUFFERWRITE });

		return FrameResourceHandle(INVALIDHANDLE);
	}
	*/

	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteDepthBuffer(ResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERWRITE, FrameObjectResourceType::OT_DepthBuffer);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadWriteUAV(UAVResourceHandle handle, DeviceResourceState state)
	{
		return AddWriteableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadWriteUAV(UAVTextureHandle handle, DeviceResourceState state)
	{
		return AddWriteableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadSOBuffer(SOResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceResourceState::DRS_VERTEXBUFFER);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteSOBuffer(SOResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_STREAMOUT);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(ResourceHandle handle, DeviceResourceState state, FrameObjectResourceType type)
	{
		bool TrackedReadable = Context.IsTrackedReadable	(handle, type);
		bool TrackedWritable = Context.IsTrackedWriteable	(handle, type);

		if (!TrackedReadable && !TrackedWritable)
		{
			FrameResourceHandle Resource = Resources->FindFrameResource(handle);

			LocalOutputs.push_back(
				FrameObjectDependency{
					Resources->GetAssetObject(Resource),
					nullptr,
					Resources->GetAssetObjectState(Resource),
					state });

			Context.AddWriteable(LocalOutputs.back());
			Transitions.push_back(LocalOutputs.back());

			return Resource;
		}
		else
		{
			FK_ASSERT(0);
			/*
			auto Object = TrackedReadable ?
				Context.GetReadable(Tag) : Context.GetWriteable(Tag);
			Object.ExpectedState = Object.State;
			Object.State = State;
			LocalOutputs.push_back(Object);
			if (TrackedReadable) {
				Context.RemoveReadable(Tag);
				Context.AddWriteable(Object);
				Transitions.push_back(Object);
			}
			return Object.FO->Handle;
			*/
		}

		return FrameResourceHandle(0);
	}


	/************************************************************************************************/


	size_t FrameGraphNodeBuilder::GetDescriptorTableSize(PSOHandle State, size_t idx) const
	{
		auto rootSig	= Resources->renderSystem.GetPSORootSignature(State);
		auto tableSize	= rootSig->GetDesciptorTableSize(idx);

		return tableSize;
	}


	const DesciptorHeapLayout<16>&	FrameGraphNodeBuilder::GetDescriptorTableLayout(PSOHandle State, size_t idx) const
	{
		auto rootSig = Resources->renderSystem.GetPSORootSignature(State);
		return rootSig->GetDescHeap(idx);
	}


	/************************************************************************************************/


	DescriptorHeap	FrameGraphNodeBuilder::ReserveDescriptorTableSpaces(const DesciptorHeapLayout<16>& IN_layout, size_t requiredTables, iAllocator* tempMemory)
	{
		DescriptorHeap newHeap;
		newHeap.Init(
			Resources->renderSystem,
			IN_layout,
			requiredTables,
			tempMemory);


		return newHeap;
	}


	/************************************************************************************************/


	FrameGraphNodeBuilder::CheckStateRes FrameGraphNodeBuilder::CheckResourceSituation(
		Vector<FrameObjectDependency>&  Set1,
		Vector<FrameObjectDependency>&  Set2,
		FrameObjectDependency&			Object)
	{
		auto Pred = [&](auto& lhs)
		{	return (lhs.Tag == Object.Tag);	};

		{
			auto Res = find(Set1, Pred);
			if (Res != Set1.end())
			{
				return CheckStateRes::TransitionNeeded;
			}
		}
		{
			auto Res = find(Set2, Pred);
			if (Res == Set2.end())
			{
				return CheckStateRes::ResourceNotTracked;
			}
			return CheckStateRes::CorrectState;
		}

		return CheckStateRes::Error;
	}


	/************************************************************************************************/


	void FrameGraph::ProcessNode(FrameGraphNode* N, FrameResources& Resources, Context& Context)
	{
		if (!N->Executed)
		{
			N->Executed = true;

			for (auto Source : N->Sources)
				ProcessNode(Source, Resources, Context);

			N->NodeAction(*N, Resources, &Context);
		}
	}


	/************************************************************************************************/

	 void FrameGraph::_SubmitFrameGraph(Vector<Context>& contexts)
	{
		for (auto& N : Nodes)
			ProcessNode(&N, Resources, contexts.back());
	}


	void FrameGraph::SubmitFrameGraph(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, RenderWindow* renderWindow)
	{

		struct SubmitData
		{
			Vector<Context>				contexts;
			FrameGraph*					frameGraph;
			RenderSystem*				renderSystem;
			RenderWindow*				renderWindow;
		};

		auto framegraph = this;

		dispatcher.Add<SubmitData>(
			[&](auto& builder, SubmitData& data)
			{
				FK_LOG_9("Frame Graph Single-Thread Section Begin");
				builder.DebugID		= "Frame Graph Task";

				data.contexts		= Vector<Context>{ Memory };
				data.contexts.emplace_back(renderSystem->_GetCurrentCommandList(), renderSystem, Memory);

				data.frameGraph		= framegraph;
				data.renderSystem	= renderSystem;
				data.renderWindow	= renderWindow;

				std::sort(dataDependencies.begin(), dataDependencies.end());
				dataDependencies.erase(std::unique(dataDependencies.begin(), dataDependencies.end()), dataDependencies.end());

				for (auto dependency : dataDependencies)
					builder.AddInput(*dependency);

				ReadyResources();

				FK_LOG_9("Frame Graph Single-Thread Section End");
			},
			[=](SubmitData& data)
			{
				FK_LOG_9("Frame Graph Multi-Thread Section Begin");

				data.frameGraph->_SubmitFrameGraph(data.contexts);
				data.contexts.back().FlushBarriers();

				static_vector<ID3D12CommandList*> CLs = { data.contexts.back().GetCommandList() };

				Close({ data.contexts.back().GetCommandList() });
				data.renderSystem->Submit(CLs);

				UpdateResourceFinalState();

				FK_LOG_9("Frame Graph Multi-Thread Section End");
			});
	}


	/************************************************************************************************/


	void FrameGraph::ReadyResources()
	{
		Vector<FrameObject*> RenderTargets(Memory);
		RenderTargets.reserve(64);

		Vector<FrameObject*> DepthBuffers(Memory);
		DepthBuffers.reserve(64);


		for (auto& Resource : Resources.Resources)
		{
			if (Resource.Type == OT_RenderTarget ||
				Resource.Type == OT_BackBuffer)
			{
				RenderTargets.push_back(&Resource);
			}
			else if(Resource.Type == OT_DepthBuffer)
				DepthBuffers.push_back(&Resource);
		}

		auto& renderSystem	= Resources.renderSystem;

		{	// Push RenderTargets
			auto Table		= renderSystem._ReserveRTVHeap(RenderTargets.size());
			auto TablePOS	= Table;

			for (auto RT : RenderTargets)
			{
				const auto Handle			= RT->RenderTarget.Texture;

				RT->RenderTarget.HeapPOS	= TablePOS;
				TablePOS					= PushRenderTarget(renderSystem, Handle, TablePOS);

                renderSystem.MarkTextureUsed(Handle);
			}
		}

        for (auto& resource : Resources.Resources)
        {
            if (resource.Type == OT_UAVTexture && resource.UAVTexture.renderTargetUse)
            {
                const auto Texture          = renderSystem.GetUAV2DTexture(resource.UAVTexture.handle);
                const auto TablePOS         = renderSystem._ReserveRTVHeap(1);
                resource.UAVTexture.HeapPOS = TablePOS;

                PushRenderTarget(renderSystem, Texture, TablePOS);
            }

            if (resource.Type == OT_ShaderResource && resource.ShaderResource.renderTargetUse)
            {
                const auto TablePOS             = renderSystem._ReserveRTVHeap(1);
                resource.ShaderResource.HeapPOS = TablePOS;

                PushRenderTarget2(renderSystem, resource.ShaderResource, TablePOS);
            }
        }

		{	// Push Depth Stencils
			auto TableDepth = renderSystem._ReserveDSVHeap(DepthBuffers.size());
			auto TableDepthPOS = TableDepth;

			for (auto RT : DepthBuffers)
			{
				auto Handle		            = RT->RenderTarget.Texture;
				RT->RenderTarget.HeapPOS	= TableDepthPOS;
				TableDepthPOS				= PushDepthStencil(renderSystem, Handle, TableDepthPOS);

                renderSystem.MarkTextureUsed(Handle);
			}
		}
	}


	/************************************************************************************************/


	void FrameGraph::UpdateResourceFinalState()
	{
		auto Objects = ResourceContext.GetFinalStates();

		for (auto& I : Objects)
		{
			switch (I.FO->Type)
			{
			case OT_UAVBuffer:
			{
				auto UAV	= I.FO->UAVBuffer;
				auto state	= I.State;
				Resources.renderSystem.SetObjectState(UAV, state);
			}	break;
			case OT_UAVTexture:
			{
				auto UAV	= I.FO->UAVTexture;
				auto state	= I.State;
				Resources.renderSystem.SetObjectState(UAV, state);
			}	break;
			case OT_StreamOut:
			{
				auto SOBuffer	= I.FO->SOBuffer;
				auto state		= I.State;

				Resources.renderSystem.SetObjectState(SOBuffer, state);
			}	break;
            case OT_BackBuffer:
            case OT_DepthBuffer:
            case OT_RenderTarget:
			case OT_ShaderResource:
			{
				auto shaderResource = I.FO->ShaderResource.handle;
				auto state			= I.State;

				Resources.renderSystem.SetObjectState(shaderResource, state);
			}	break;

			default:
				FK_ASSERT(false, "UN-IMPLEMENTED BLOCK!");
			}
		}
	}


	/************************************************************************************************/


	void ClearBackBuffer(FrameGraph& Graph, ResourceHandle backBuffer, float4 Color)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
			float4				ClearColor;
		};

		auto& Pass = Graph.AddNode<PassData>(
			PassData
			{
				InvalidHandle_t,
				Color
			},
			[=](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.WriteBackBuffer(backBuffer);
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	// do clear here
				Ctx->ClearRenderTarget(
					Resources.GetRenderTargetObject(Data.BackBuffer), 
					Data.ClearColor);
			});
	}


	/************************************************************************************************/


	void ClearDepthBuffer(FrameGraph& Graph, ResourceHandle Handle, float clearDepth)
	{
		FK_VLOG(Verbosity_9, "Clearing depth buffer.");

		struct ClearDepthBuffer
		{
			FrameResourceHandle DepthBuffer;
			float				ClearDepth;
		};

		auto& Pass = Graph.AddNode<ClearDepthBuffer>(
			ClearDepthBuffer{
				InvalidHandle_t,
				clearDepth
			},
			[=](FrameGraphNodeBuilder& Builder, ClearDepthBuffer& Data)
			{
				Data.DepthBuffer = Builder.WriteDepthBuffer(Handle);
			},
			[=](const ClearDepthBuffer& Data, const FrameResources& Resources, Context* Ctx)
			{	// do clear here
				Ctx->ClearDepthBuffer(
					Resources.GetRenderTargetObject(Data.DepthBuffer),
					Data.ClearDepth);
			});
	}


	/************************************************************************************************/


	void PresentBackBuffer(FrameGraph& Graph, RenderWindow* Window)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& Pass = Graph.AddNode<PassData>(
			PassData{},
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.PresentBackBuffer(Window->backBuffer);
			},
			[=](const PassData& Data, const FrameResources& Resources, Context* Ctx)
			{	
			});
	}

	
	/************************************************************************************************/

	/*
	void SetRenderTargets(
		Context* Ctx, 
		static_vector<FrameResourceHandle> RenderTargets, 
		FrameResources& Resources)
	{
		static_vector<DescHeapPOS> Targets;
		for (auto RenderTarget : RenderTargets)
			auto HeapPOS = Resources.GetRenderTargetDescHeapEntry(RenderTarget);

		Ctx->SetRenderTargets(Targets, false, DescHeapPOS());
	}
	*/

	/************************************************************************************************/


	void ClearVertexBuffer(FrameGraph& FG, VertexBufferHandle PushBuffer)
	{
		FG.Resources.renderSystem.VertexBuffers.Reset(PushBuffer);
	}


}	/************************************************************************************************/
