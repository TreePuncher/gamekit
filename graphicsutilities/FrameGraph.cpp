#include "graphics.h"
#include "FrameGraph.h"
#include "Logging.h"
#include <fmt/core.h>

namespace FlexKit
{	/************************************************************************************************/


	void ResourceTransition::ProcessTransition(FrameResources& Resources, Context* ctx) const
	{
        auto& resourceObject = Resources.Resources[Object];

		switch (resourceObject.Type)
		{
		case OT_StreamOut:
		{
			switch (AfterState)
			{
			case DRS_VERTEXBUFFER:
			case DRS_STREAMOUT: {
				ctx->AddStreamOutBarrier(
                    resourceObject.SOBuffer,
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
                    resourceObject.shaderResource,
					BeforeState,
					AfterState);
				break;
			case DRS_Retired:
				ctx->AddAliasingBarrier(resourceObject.shaderResource, BeforeState, AfterState);
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
                    resourceObject.UAVBuffer,
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
                    resourceObject.UAVTexture,
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
					Resources.GetRenderTarget(resourceObject.Handle),
					BeforeState);
			}	break;
			case DRS_DEPTHBUFFERWRITE:
			case DRS_RenderTarget:
				ctx->AddRenderTargetBarrier(
					Resources.GetRenderTarget(resourceObject.Handle),
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
		for (const auto& T : Transitions) 
			T.ProcessTransition(Resources, &Ctx);
	}


	/************************************************************************************************/


	void FrameGraphNode::AddTransition(ResourceTransition& transition)
	{
		Transitions.push_back(transition);
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


	void FrameGraphNode::RestoreResourceStates(Context* ctx, FrameResources& resources, LocallyTrackedObjectList& locallyTrackedObjects)
	{
		for (auto& localResourceDescriptor : locallyTrackedObjects)
		{
            auto currentState   = localResourceDescriptor.currentState;
            auto nodeState      = localResourceDescriptor.nodeState;

            if (currentState == nodeState)
                continue;

            auto& resource = resources.Resources[localResourceDescriptor.resource];

			switch (resource.Type)
			{
			case FrameObjectResourceType::OT_StreamOut:
				ctx->AddStreamOutBarrier(resource.SOBuffer, currentState, nodeState);
				break;
			case FrameObjectResourceType::OT_UAVTexture:
				ctx->AddUAVBarrier(resource.UAVTexture, currentState, nodeState);
				break;
			case FrameObjectResourceType::OT_UAVBuffer:
				ctx->AddUAVBarrier(resource.UAVBuffer, currentState, nodeState);
				break;
			case FrameObjectResourceType::OT_BackBuffer:
			case FrameObjectResourceType::OT_RenderTarget:
			case FrameObjectResourceType::OT_ShaderResource:
				ctx->AddShaderResourceBarrier(resource.shaderResource, currentState, nodeState);
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


	/************************************************************************************************/


	void FrameGraph::AddRenderTarget(ResourceHandle handle)
	{
		Resources.AddShaderResource(handle, true);
	}


	/************************************************************************************************/


	void FrameGraph::AddMemoryPool(PoolAllocatorInterface* allocator)
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
		// Process Transitions

        Node.Transitions = std::move(Transitions);
        Context.Retirees += RetiredObjects;

        Transitions.clear();
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

		return resourceHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteRenderTarget(UAVTextureHandle handle)
	{
		const auto resourceHandle   = AddWriteableResource(handle, DeviceResourceState::DRS_RenderTarget);
		auto resource               = Resources->GetAssetObject(resourceHandle);

		FK_ASSERT(resource != nullptr);

		return resourceHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::PresentBackBuffer(ResourceHandle renderTarget)
	{
		return AddReadableResource(renderTarget, DeviceResourceState::DRS_Present);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteBackBuffer(ResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_RenderTarget);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadBackBuffer(ResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceResourceState::DRS_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::WriteDepthBuffer(ResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERWRITE);
	}


	/************************************************************************************************/


	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc desc, DeviceResourceState initialState)
	{
		const auto& memoryPool      = Resources->memoryPools.back();
		auto virtualResource        = memoryPool->Aquire(desc);

		FrameObject virtualObject       = FrameObject::VirtualObject();
		virtualObject.shaderResource    = virtualResource;
		virtualObject.State             = DeviceResourceState::DRS_DEPTHBUFFERWRITE;

		auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
		Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
		Resources->virtualResources.push_back(virtualResourceHandle);

		FrameObjectLink outputObject;
			outputObject.neededState    = Resources->renderSystem.GetObjectState(virtualResource);
			outputObject.Source         = &Node;
			outputObject.handle         = virtualResourceHandle;

		Context.AddWriteable(outputObject);
		Node.OutputObjects.push_back(outputObject);

		return  virtualResourceHandle;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ReleaseVirtualResource(FrameResourceHandle handle)
	{
		FrameResourceHandle resource    = Resources->FindFrameResource(handle);
		auto freedResource              = Resources->GetTexture(resource);

        FrameObjectLink freeObject;
		freeObject.neededState      = DeviceResourceState::DRS_Retired;
		freeObject.Source           = &Node;
		freeObject.handle           = handle;

        Resources->Resources[handle].State = DRS_Retired;

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
		Vector<FrameObjectLink>&    Set1,
		Vector<FrameObjectLink>&    Set2,
        FrameObjectLink&			Object)
	{
		auto Pred = [&](auto& lhs){	return (lhs.handle == Object.handle);	};

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


	void FrameGraph::_SubmitFrameGraph(iAllocator& persistentAllocator)
	{        
		Vector<FrameGraphNodeWork> taskList{ &persistentAllocator };

		for (auto& N : Nodes)
			ProcessNode(&N, Resources, taskList, persistentAllocator);

		auto& renderSystem = Resources.renderSystem;
		Vector<Context*> contexts{ &persistentAllocator };

		struct SubmissionWorkRange
		{
			Vector<FrameGraphNodeWork>::Iterator begin;
			Vector<FrameGraphNodeWork>::Iterator end;
		};

		static const size_t workerCount = 10;
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
			RenderWorker(SubmissionWorkRange IN_work, Context* IN_ctx, iAllocator& persistentAllocator) :
				iWork   { &persistentAllocator  },
				work    { IN_work               },
				ctx     { IN_ctx                } {}

			void Run(iAllocator& tempAllocator) override
			{
				std::for_each(work.begin, work.end,
					[&](auto& item)
					{
						item(ctx, tempAllocator);
					});

				ctx->FlushBarriers();
			}

			void Release() {}

			SubmissionWorkRange work;
			Context*            ctx;
		};

		FlexKit::WorkBarrier barrier{ threads, &persistentAllocator };
		static_vector<RenderWorker*> workers;

		for (auto& work : workList)
		{
			auto& context = renderSystem.GetCommandList();
			contexts.push_back(&context);

#if USING(DEBUGGRAPHICS)
			auto ID = fmt::format("CommandList{}", contexts.size() - 1);
			context.SetDebugName(ID.c_str());
#endif

			auto& worker = SystemAllocator.allocate<RenderWorker>(work, &context, persistentAllocator);
			workers.push_back(&worker);
			barrier.AddWork(worker);
		}

		for (auto& worker : workers)
			threads.AddWork(*worker);

		barrier.JoinLocal();

		UpdateResourceFinalState();

		renderSystem.Submit(contexts);
	}


	void FrameGraph::SubmitFrameGraph(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator* persistentAllocator)
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
			[=](SubmitData& data, iAllocator& threadAllocator)
			{
				data.frameGraph->_SubmitFrameGraph(*persistentAllocator);
			});
	}


	/************************************************************************************************/


	void FrameGraph::UpdateResourceFinalState()
	{
		auto Objects = Resources.Resources;

		for (auto& I : Objects)
		{
			switch (I.Type)
			{
			case OT_UAVBuffer:
			{
				auto UAV	= I.UAVBuffer;
				auto state	= I.State;
				Resources.renderSystem.SetObjectState(UAV, state);
			}	break;
			case OT_UAVTexture:
			{
				auto UAV	= I.UAVTexture;
				auto state	= I.State;
				Resources.renderSystem.SetObjectState(UAV, state);
			}	break;
			case OT_StreamOut:
			{
				auto SOBuffer	= I.SOBuffer;
				auto state		= I.State;

				Resources.renderSystem.SetObjectState(SOBuffer, state);
			}	break;
			case OT_BackBuffer:
			case OT_DepthBuffer:
			case OT_RenderTarget:
			case OT_ShaderResource:
			{
				auto shaderResource = I.shaderResource;
				auto state			= I.State;

				Resources.renderSystem.SetObjectState(shaderResource, state);
			}	break;
			case OT_Virtual:
			{
				if(I.State == DRS_Retired)
					Resources.renderSystem.ReleaseTexture(I.shaderResource);
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


}	/************************************************************************************************/
