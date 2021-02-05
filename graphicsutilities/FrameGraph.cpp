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
		case OT_Resource:
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
            case DRS_STREAMOUT:
            case DRS_STREAMOUTCLEAR:
            case DRS_UAV:
				ctx->AddResourceBarrier(
                    resourceObject.shaderResource,
					BeforeState,
					AfterState);
				break;
			case DRS_Retired:
				ctx->AddAliasingBarrier(resourceObject.shaderResource, InvalidHandle_t);
				break;
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


    FrameGraphNode::FrameGraphNode(FN_NodeAction IN_action, void* IN_nodeData, iAllocator* IN_allocator) :
			InputObjects	{ IN_allocator	},
			OutputObjects	{ IN_allocator	},
			Sources			{ IN_allocator	},
			Transitions		{ IN_allocator	},
			NodeAction		{ IN_action     },
			nodeData        { IN_nodeData   },
			Executed		{ false		    },
			subNodeTracking { IN_allocator  },

            createdObjects  { IN_allocator  },
			retiredObjects  { IN_allocator  },
            acquiredObjects { IN_allocator  } {}


    FrameGraphNode::FrameGraphNode(const FrameGraphNode& RHS) :
			Sources			{ RHS.Sources		            },
			InputObjects	{ RHS.InputObjects	            },
			OutputObjects	{ RHS.OutputObjects	            },
			Transitions		{ RHS.Transitions	            },
			NodeAction		{ std::move(RHS.NodeAction)	    },
			nodeData        { RHS.nodeData                  },
			retiredObjects  { RHS.retiredObjects            },
			subNodeTracking { RHS.subNodeTracking           },
			Executed		{ false				            },
            createdObjects  { std::move(RHS.createdObjects) },
            acquiredObjects { RHS.acquiredObjects           } {}



    /************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context& Ctx)
	{
        for (const auto& virtualResource : createdObjects)
            Ctx.AddAliasingBarrier(InvalidHandle_t, Resources.GetTexture(virtualResource.handle));

		for (const auto& T : Transitions) 
			T.ProcessTransition(Resources, &Ctx);
	}


	/************************************************************************************************/


	void FrameGraphNode::AddTransition(const ResourceTransition& transition)
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
			case FrameObjectResourceType::OT_DepthBuffer:
			case FrameObjectResourceType::OT_BackBuffer:
			case FrameObjectResourceType::OT_RenderTarget:
			case FrameObjectResourceType::OT_Resource:
				ctx->AddResourceBarrier(resource.shaderResource, currentState, nodeState);
				break;
			case FrameObjectResourceType::OT_ConstantBuffer:
			case FrameObjectResourceType::OT_IndirectArguments:
			case FrameObjectResourceType::OT_PVS:
			case FrameObjectResourceType::OT_VertexBuffer:
				FK_ASSERT(0, "UN-IMPLEMENTED BLOCK!");
			}
        }
  

        for (auto& retired : retiredObjects)
        {
            auto resource = resources.Resources[retired.handle];

            if (resource.virtualState == VirtualResourceState::Virtual_Temporary ||
                resource.virtualState == VirtualResourceState::Virtual_Released)
            {
                if( resource.State == DRS_DEPTHBUFFERWRITE ||
                    resource.State == DRS_RenderTarget)
                ctx->DiscardResource(resource.shaderResource);
                ctx->AddAliasingBarrier(resource.shaderResource, InvalidHandle_t);
            }
        }
	}


	/************************************************************************************************/


	void FrameGraph::AddRenderTarget(ResourceHandle handle)
	{
		Resources.AddResource(handle, true);
	}


    /************************************************************************************************/


    FrameResourceHandle FrameGraph::AddResource(ResourceHandle resource)
    {
        return Resources.AddResource(resource, true);
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

        for (auto& temporary : temporaryObjects)
        {
            auto resource = Resources->Resources[temporary.handle];
            resource.pool->Release(resource.shaderResource, false);
        }

        temporaryObjects.clear();
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

        if (resourceHandle == InvalidHandle_t)
        {
            Resources->AddResource(RenderTarget, true);
            return WriteRenderTarget(RenderTarget);
        }

		auto resource               = Resources->GetAssetObject(resourceHandle);

		if (!resource)
			return InvalidHandle_t;

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
		auto res = AddWriteableResource(handle, DeviceResourceState::DRS_RenderTarget);
        if (res == InvalidHandle_t) {
            Resources->AddResource(handle);
            return WriteBackBuffer(handle);
        }

        return res;
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


	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc desc, DeviceResourceState initialState, bool temp)
	{
        auto NeededFlags = 0;
        NeededFlags |= !desc.unordered && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::RenderTarget : 0;
        NeededFlags |= !desc.unordered && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::RenderTarget : 0;
        NeededFlags |= !desc.unordered && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::RenderTarget : 0;
        NeededFlags |= !desc.unordered && desc.Dimensions == TextureDimension::Texture2DArray ? DeviceHeapFlags::RenderTarget : 0;

        NeededFlags |= desc.unordered && desc.Dimensions == TextureDimension::Buffer    ? DeviceHeapFlags::UAVBuffer : 0;
        NeededFlags |= desc.unordered && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::UAVTextures : 0;
        NeededFlags |= desc.unordered && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::UAVTextures : 0;
        NeededFlags |= desc.unordered && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::UAVTextures : 0;
        NeededFlags |= desc.renderTarget ? DeviceHeapFlags::RenderTarget : 0;

        auto FindResourcePool =
            [&]() -> PoolAllocatorInterface*
            {
                for (auto pool : Resources->memoryPools)
                {
                    if ((pool->Flags() & NeededFlags) == NeededFlags)
                        return pool;
                }

                return nullptr;
            };

        for(auto memoryPool = FindResourcePool(); memoryPool; memoryPool = FindResourcePool())
        {
            auto virtualResource            = memoryPool->Aquire(desc, temp);
		    FrameObject virtualObject       = FrameObject::VirtualObject();
		    virtualObject.shaderResource    = virtualResource;
		    virtualObject.State             = initialState;
            virtualObject.virtualState      = VirtualResourceState::Virtual_Temporary;
            virtualObject.pool              = memoryPool;

            Resources->renderSystem.SetDebugName(virtualResource, "Un-named virtual resources");

		    auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
		    Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
		    Resources->virtualResources.push_back(virtualResourceHandle);

		    FrameObjectLink outputObject;
			    outputObject.neededState    = Resources->renderSystem.GetObjectState(virtualResource);
			    outputObject.Source         = &Node;
			    outputObject.handle         = virtualResourceHandle;

            FK_LOG_9("Allocated Resource: %u", Resources->GetObjectResource(virtualResource));
            Resources->renderSystem.SetDebugName(virtualResource, "Virtual Resource");


		    Context.AddWriteable(outputObject);
		    Node.OutputObjects.push_back(outputObject);

            if (initialState != outputObject.neededState)
                Node.AddTransition(ResourceTransition{ virtualResourceHandle, outputObject.neededState, initialState });

            if (temp) {
                Node.retiredObjects.push_back(outputObject);
                temporaryObjects.push_back(outputObject);
            }

		    return  virtualResourceHandle;
        }

        return InvalidHandle_t;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ReleaseVirtualResource(FrameResourceHandle handle)
	{
        auto& resource              = Resources->Resources[handle];

        FrameObjectLink freeObject;
		freeObject.neededState      = DeviceResourceState::DRS_Retired;
		freeObject.Source           = &Node;
		freeObject.handle           = handle;

        resource.virtualState       = VirtualResourceState::Virtual_Released;

		Node.retiredObjects.push_back(freeObject);
        resource.pool->Release(resource.shaderResource, false);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadWriteUAV(ResourceHandle handle, DeviceResourceState state)
	{
        if (auto frameResource = AddWriteableResource(handle, state); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

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

		static const size_t workerCount = 2;
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
			case OT_StreamOut:
			{
				auto SOBuffer	= I.SOBuffer;
				auto state		= I.State;

				Resources.renderSystem.SetObjectState(SOBuffer, state);
			}	break;
			case OT_BackBuffer:
			case OT_DepthBuffer:
			case OT_RenderTarget:
			case OT_Resource:
			{
				auto shaderResource = I.shaderResource;
				auto state			= I.State;

				Resources.renderSystem.SetObjectState(shaderResource, state);
			}	break;
			case OT_Virtual:
			{
                Resources.renderSystem.SetObjectState(I.shaderResource, I.State);

                if (I.virtualState == VirtualResourceState::Virtual_Released || I.virtualState == VirtualResourceState::Virtual_Temporary) {
                    Resources.renderSystem.ReleaseResource(I.shaderResource);
                }
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
					{ Resources.GetResource(Data.BackBuffer) },
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
					{ Resources.GetResource(Data.DepthBuffer) },
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
