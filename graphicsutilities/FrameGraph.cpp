#include "graphics.h"
#include "FrameGraph.h"
#include "Logging.h"

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

		case OT_Virtual:
		case OT_ShaderResource:
		case OT_BackBuffer:
		case OT_DepthBuffer:
		{
			switch (AfterState)
			{
			case DRS_DEPTHBUFFERWRITE:
			case DRS_DEPTHBUFFERREAD:
			case DRS_Write:
			case DRS_ShaderResource:
			case DRS_RenderTarget:
			case DRS_Present:
				ctx->AddShaderResourceBarrier(
					Object->ShaderResource.handle,
					BeforeState,
					AfterState);
				break;
			case DRS_Retired:
				ctx->AddAliasingBarrier(Object->ShaderResource.handle, BeforeState, AfterState);
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


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context& Ctx)
	{
		for (const auto& T : Transitions) {
			T.ProcessTransition(Resources, &Ctx);

			auto stateObject	= *T.Object;
			stateObject.State	= T.AfterState;
			auto idx			= SubNodeTracking.push_back(stateObject);
		}
	}


	/************************************************************************************************/


	void FrameGraphNode::AddTransition(FrameObjectDependency& Dep)
	{
		Transitions.push_back(Dep);
	}


	/************************************************************************************************/


	void FrameGraphNode::AcquireResources(FrameResources& resources, Context& ctx)
	{
	}


	/************************************************************************************************/


	void FrameGraphNode::ReleaseResources(FrameResources& resources, Context& ctx)
	{
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
				case FrameObjectResourceType::OT_RenderTarget:
				case FrameObjectResourceType::OT_ShaderResource:
					ctx->AddShaderResourceBarrier(resNode.Texture, resNode.State, res->State);
					break;
				case FrameObjectResourceType::OT_ConstantBuffer:
				case FrameObjectResourceType::OT_DepthBuffer:
				case FrameObjectResourceType::OT_IndirectArguments:
				case FrameObjectResourceType::OT_PVS:
				case FrameObjectResourceType::OT_VertexBuffer:
					FK_ASSERT(0, "UN-IMPLEMENTED BLOCK!");
				}
			}
		}

		locallyTrackedObjects.clear();

		return;
	}


	/************************************************************************************************/


	void FrameGraph::AddRenderTarget(ResourceHandle handle)
	{
		Resources.AddShaderResource(handle, true);
	}


	/************************************************************************************************/


	void FrameGraph::AddMemoryPool(MemoryPoolAllocator* allocator)
	{
		Resources.AddMemoryPool(allocator);
	}


	/************************************************************************************************/

	
	void FrameGraph::UpdateFrameGraph(RenderSystem* RS, iAllocator* Temp)
	{
		RS->BeginSubmission();
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

			
			if(Object.ExpectedState != Object.State)
				Node.AddTransition(Object);
		}

		for (auto& SourceNode : LocalInputs)
			CheckNodeSource(Node, SourceNode);

		for (auto& retired : RetiredObjects)
			Context.Retire(retired);

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


	FrameResourceHandle	FrameGraphNodeBuilder::WriteDepthBuffer(ResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERWRITE, FrameObjectResourceType::OT_DepthBuffer);
	}


	/************************************************************************************************/


	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc desc, DeviceResourceState initialState)
	{
		const auto& memoryPool      = Resources->memoryPools.back();
		auto virtualResource        = memoryPool->Aquire(desc);

		FrameObject virtualObject = FrameObject::VirtualObject();
		virtualObject.RenderTarget.Texture  = virtualResource;
		virtualObject.State                 = DeviceResourceState::DRS_DEPTHBUFFERWRITE;

		auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
		Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
		Resources->virtualResources.push_back(virtualResourceHandle);

		FrameObjectDependency outputObject;
			outputObject.ExpectedState      = Resources->renderSystem.GetObjectState(virtualResource);
			outputObject.State              = Resources->renderSystem.GetObjectState(virtualResource);
			outputObject.ShaderResource     = virtualResource;
			outputObject.Source             = &Node;
			outputObject.FO                 = &Resources->Resources[virtualResourceHandle];

		Context.AddWriteable(outputObject);
		Node.OutputObjects.push_back(outputObject);

		return  virtualResourceHandle;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ReleaseVirtualResource(FrameResourceHandle handle)
	{
		FrameResourceHandle resource    = Resources->FindFrameResource(handle);
		auto freedResource              = Resources->GetTexture(resource);

		FrameObjectDependency freeObject;
		freeObject.ExpectedState    = Resources->GetAssetObjectState(resource);
		freeObject.State            = DeviceResourceState::DRS_Retired;
		freeObject.ShaderResource   = freedResource;
		freeObject.Source           = &Node;
		freeObject.FO               = &Resources->Resources[handle];

		Node.RetiredObjects.push_back(freeObject);

		const auto& memoryPool  = Resources->memoryPools.back();
		memoryPool->Release(freedResource, false);
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


	FrameResourceHandle FrameGraphNodeBuilder::ReadResource(FrameResourceHandle handle, DeviceResourceState state)
	{
		return AddReadableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteResource(FrameResourceHandle handle, DeviceResourceState state)
	{
		return AddWriteableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(ResourceHandle handle, DeviceResourceState state, FrameObjectResourceType type)
	{
		bool TrackedReadable = Context.IsTrackedReadable(handle, type);
		bool TrackedWritable = Context.IsTrackedWriteable(handle, type);

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

		return InvalidHandle_t;
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


	FrameGraphNodeBuilder::CheckStateRes FrameGraphNodeBuilder::CheckResourceSituation(
		Vector<FrameObjectDependency>&  Set1,
		Vector<FrameObjectDependency>&  Set2,
		FrameObjectDependency&			Object)
	{
		auto Pred = [&](auto& lhs){	return (lhs.FO == Object.FO);	};

		if (find(Set1, Pred) != Set1.end())
			return CheckStateRes::TransitionNeeded;
		else if (find(Set2, Pred) == Set2.end())
			return CheckStateRes::ResourceNotTracked;
		else
			return CheckStateRes::CorrectState;
	}


	/************************************************************************************************/


	void FrameGraph::ProcessNode(FrameGraphNode* node, FrameResources& resources, Vector<FrameGraphNodeWork>& taskList, iAllocator& allocator)
	{
		if (!node->Executed)
		{
			node->Executed = true;

			for (auto Source : node->Sources)
				ProcessNode(node, resources, taskList, allocator);

            taskList.push_back(
                {
                    node->NodeAction,
                    node,
                    &resources
                });
		}
	}


	/************************************************************************************************/


	void FrameGraph::_SubmitFrameGraph(iAllocator& allocator)
	{
        FlexKit::WorkBarrier barrier{ threads, &allocator };
        
        Vector<FrameGraphNodeWork> taskList{ &SystemAllocator };

		for (auto& N : Nodes)
			ProcessNode(&N, Resources, taskList, SystemAllocator);

        auto& renderSystem = Resources.renderSystem;
        Vector<Context*> contexts{ &SystemAllocator };

        struct SubmissionWorkRange
        {
            Vector<FrameGraphNodeWork>::Iterator begin;
            Vector<FrameGraphNodeWork>::Iterator end;
        };

        static const size_t workerCount = 4;
        static_vector<SubmissionWorkRange> workList;
        for (size_t I = 0; I < workerCount; I++)
        {
            workList.push_back(
                {
                    taskList.begin() + I * taskList.size() / workerCount,
                    taskList.begin() + (I  + 1) * taskList.size() / workerCount
                });
        }

        class RenderWorker : public iWork
        {
        public:
            RenderWorker(SubmissionWorkRange IN_work, Context* IN_ctx, iAllocator& allocator) :
                iWork   { &allocator    },
                work    { IN_work       },
                ctx     { IN_ctx        } {}

            void Run(iAllocator& allocator) override
            {
                std::for_each(work.begin, work.end,
                    [&](auto& item)
                    {
                        item(ctx, SystemAllocator);
                    });

                ctx->FlushBarriers();
            }

            void Release() {}

            SubmissionWorkRange work;
            Context*            ctx;
        };

        static_vector<RenderWorker*> workerList;
        for (auto& work : workList)
        {
            contexts.push_back(&renderSystem.GetCommandList());
            workerList.push_back(&SystemAllocator.allocate<RenderWorker>(work, contexts.back(), SystemAllocator));
            threads.AddWork(*workerList.back());
            barrier.AddWork(*workerList.back());
        }

        barrier.Wait();

        renderSystem.Submit(contexts);

        UpdateResourceFinalState();
	}


	void FrameGraph::SubmitFrameGraph(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator& allocator)
	{
		struct SubmitData
		{
			FrameGraph*		    frameGraph;
			RenderSystem*	    renderSystem;
			RenderWindow*	    renderWindow;
		};

		auto framegraph = this;

		std::sort(dataDependencies.begin(), dataDependencies.end());
		dataDependencies.erase(std::unique(dataDependencies.begin(), dataDependencies.end()), dataDependencies.end());


		dispatcher.Add<SubmitData>(
			[&](auto& builder, SubmitData& data)
			{
				FK_LOG_9("Frame Graph Single-Thread Section Begin");
				builder.SetDebugString("Frame Graph Task");

				data.frameGraph		= framegraph;
				data.renderSystem	= renderSystem;

				for (auto dependency : dataDependencies)
					builder.AddInput(*dependency);

				FK_LOG_9("Frame Graph Single-Thread Section End");
			},
			[=, &allocator](SubmitData& data, iAllocator& threadAllocator)
			{
				data.frameGraph->_SubmitFrameGraph(allocator);

				for (auto resource : Resources.virtualResources)
					data.renderSystem->ReleaseTexture(Resources.GetRenderTarget(resource));
			});
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
			case OT_Virtual:
			{
				if(I.State == DRS_Retired)
					Resources.renderSystem.ReleaseTexture(I.ShaderResource);
			}   break;
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
			[=](const PassData& Data, const ResourceHandler& Resources, Context& Ctx, iAllocator& allocator)
			{	// do clear here
				Ctx.ClearRenderTarget(
					{ Resources.GetTexture(Data.BackBuffer) },
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
			[=](const ClearDepthBuffer& Data, const ResourceHandler& Resources, Context& Ctx, iAllocator& allocator)
			{	// do clear here
				Ctx.ClearDepthBuffer(
					{ Resources.GetTexture(Data.DepthBuffer) },
					Data.ClearDepth);
			});
	}


	/************************************************************************************************/


	void PresentBackBuffer(FrameGraph& frameGraph, IRenderWindow& window)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& Pass = frameGraph.AddNode<PassData>(
			PassData{},
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.PresentBackBuffer(window.GetBackBuffer());
			},
			[](const PassData& Data, const ResourceHandler& Resources, Context& ctx, iAllocator&)
			{
			});
	}

	
	/************************************************************************************************/


	void ClearVertexBuffer(FrameGraph& FG, VertexBufferHandle PushBuffer)
	{
		FG.Resources.renderSystem.VertexBuffers.Reset(PushBuffer);
	}


	/************************************************************************************************/


	MemoryPoolAllocator::MemoryPoolAllocator(RenderSystem& IN_renderSystem, DeviceHeapHandle IN_heap, size_t IN_blockCount, size_t IN_blockSize, iAllocator* IN_allocator) :
		renderSystem    { IN_renderSystem },
		blockCount      { IN_blockCount },
		blockSize       { IN_blockSize },
		allocator       { IN_allocator },
		allocations     { IN_allocator },
		heap            { IN_heap }
	{
		rootNode = _Link{
			.leaf       = { 0, blockCount, ResourceHandle{ InvalidHandle_t } },
			.isLeaf     = true };
	}


}	/************************************************************************************************/
