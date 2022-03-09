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
            case DRS_PixelShaderResource:
            case DRS_NonPixelShaderResource:
			case DRS_RenderTarget:
			case DRS_Present:
            case DRS_STREAMOUT:
            case DRS_STREAMOUTCLEAR:
            case DRS_UAV:
            case DRS_CopyDest:
            case DRS_CopySrc:
            case DRS_ACCELERATIONSTRUCTURE:
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


    FrameGraphNode::FrameGraphNode(FrameGraphNode&& RHS) :
			Sources			{ std::move(RHS.Sources)	        },
			InputObjects	{ std::move(RHS.InputObjects)	    },
			OutputObjects	{ std::move(RHS.OutputObjects)      },
			Transitions		{ std::move(RHS.Transitions)        },
			NodeAction		{ std::move(RHS.NodeAction)	        },
			nodeData        { std::move(RHS.nodeData)           },
			retiredObjects  { std::move(RHS.retiredObjects)     },
			subNodeTracking { std::move(RHS.subNodeTracking)    },
			Executed		{ std::move(RHS.Executed)           },
            createdObjects  { std::move(RHS.createdObjects)     },
            acquiredObjects { std::move(RHS.acquiredObjects)    } {}


    /************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context& Ctx)
	{
        for (const auto& virtualResource : acquiredObjects)
            Ctx.AddAliasingBarrier(virtualResource.overlap, virtualResource.resource);

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
        ProfileFunction();

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
            case FrameObjectResourceType::OT_Virtual:
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

        for (auto acquire : acquiredResources)
            FrameGraph->acquiredResources.push_back(acquire.resource);


        for (auto& object : Node.InputObjects)
            if (object.Source != nullptr || object.Source != &Node)
                Node.Sources.push_back(object.Source);

        for (auto& object : Node.OutputObjects)
            if(object.Source != nullptr || object.Source != &Node)
                Node.Sources.push_back(object.Source);

        if (Node.Sources.size())
        {
            std::sort(Node.Sources.begin(), Node.Sources.end());
            auto res = std::unique(Node.Sources.begin(), Node.Sources.end());
            Node.Sources.erase(res, Node.Sources.end());
        }

        Node.Transitions        += Transitions;
        Node.acquiredObjects    += acquiredResources;
        Context.Retirees        += RetiredObjects;

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


    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::AccelerationStructure(ResourceHandle handle)
    {
        if (auto frameResource = AddWriteableResource(handle, DeviceResourceState::DRS_ACCELERATIONSTRUCTURE); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddWriteableResource(handle, DeviceResourceState::DRS_ACCELERATIONSTRUCTURE);
    }


	FrameResourceHandle FrameGraphNodeBuilder::PixelShaderResource(ResourceHandle handle)
	{
        if (auto frameResource = AddReadableResource(handle, DeviceResourceState::DRS_PixelShaderResource); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddReadableResource(handle, DeviceResourceState::DRS_PixelShaderResource);
	}


    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::NonPixelShaderResource(ResourceHandle handle)
    {
        if (auto frameResource = AddReadableResource(handle, DeviceResourceState::DRS_NonPixelShaderResource); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddReadableResource(handle, DeviceResourceState::DRS_NonPixelShaderResource);
    }


    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::NonPixelShaderResource(FrameResourceHandle handle)
    {
        return AddReadableResource(handle, DeviceResourceState::DRS_NonPixelShaderResource);
    }


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::CopySource(ResourceHandle handle)
	{
        if (auto frameResource = AddReadableResource(handle, DeviceResourceState::DRS_CopySrc); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddReadableResource(handle, DeviceResourceState::DRS_CopySrc);
	}

    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::CopyDest(ResourceHandle  handle)
    {
        if (auto frameResource = AddWriteableResource(handle, DeviceResourceState::DRS_CopyDest); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddWriteableResource(handle, DeviceResourceState::DRS_CopyDest);
    }


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::RenderTarget(ResourceHandle target)
	{
		const auto resourceHandle = AddWriteableResource(target, DeviceResourceState::DRS_RenderTarget);

        if (resourceHandle == InvalidHandle_t)
        {
            Resources->AddResource(target, true);
            return RenderTarget(target);
        }

		auto resource = Resources->GetAssetObject(resourceHandle);

		if (!resource)
			return InvalidHandle_t;

		return resourceHandle;
	}


    /************************************************************************************************/


    FrameResourceHandle FrameGraphNodeBuilder::RenderTarget(FrameResourceHandle target)
    {
        return WriteTransition(target, DRS_RenderTarget);
    }


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::Present(ResourceHandle renderTarget)
	{
		return AddReadableResource(renderTarget, DeviceResourceState::DRS_Present);
	}


	/************************************************************************************************/


    FrameResourceHandle	FrameGraphNodeBuilder::DepthRead(ResourceHandle handle)
    {
        if (auto frameObject = AddReadableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERREAD); frameObject == InvalidHandle_t)
        {
            Resources->AddResource(handle, true);
            return DepthRead(handle);
        }
        else
            return frameObject;
    }


    /************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::DepthTarget(ResourceHandle handle)
	{
        if (auto frameObject = AddWriteableResource(handle, DeviceResourceState::DRS_DEPTHBUFFERWRITE); frameObject == InvalidHandle_t)
        {
            Resources->AddResource(handle, true);
            return DepthTarget(handle);
        }
        else
            return frameObject;
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
            auto allocationSize = Resources->renderSystem.GetAllocationSize(desc);

#if 1
            ResourceHandle virtualResource  = InvalidHandle_t;
            ResourceHandle overlap          = InvalidHandle_t;

            auto [reuseableResource, found] = Node.FindReuseableResource(*memoryPool, allocationSize, NeededFlags, *Resources);

            if (found || reuseableResource != InvalidHandle_t)
            {
                auto reusedResource = Resources->Resources[reuseableResource].shaderResource;
                auto deviceResource = Resources->renderSystem.GetHeapOffset(reusedResource);
            }
            else
            {
                auto [resource, _overlap] = memoryPool->Acquire(desc, temp);

                virtualResource = resource;
                overlap         = _overlap;
            }
#else
            auto [virtualResource, overlap] = memoryPool->Acquire(desc, temp);
#endif
            if (virtualResource == InvalidHandle_t)
                return InvalidHandle_t;

		    FrameObject virtualObject       = FrameObject::VirtualObject();
		    virtualObject.shaderResource    = virtualResource;
		    virtualObject.State             = initialState;
            virtualObject.virtualState      = VirtualResourceState::Virtual_Temporary;
            virtualObject.pool              = memoryPool;

            Resources->renderSystem.SetDebugName(virtualResource, "Un-named virtual resources");

		    auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
		    Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
		    Resources->virtualResources.push_back(virtualResourceHandle);

            auto defaultState = Resources->renderSystem.GetObjectState(virtualResource);

		    FrameObjectLink outputObject;
			    outputObject.neededState    = initialState;
			    outputObject.Source         = &Node;
			    outputObject.handle         = virtualResourceHandle;

            FK_LOG_9("Allocated Resource: %u", Resources->GetObjectResource(virtualResource));
            Resources->renderSystem.SetDebugName(virtualResource, "Virtual Resource");


		    Context.AddWriteable(outputObject);
		    Node.OutputObjects.push_back(outputObject);
            Node.createdObjects.push_back(outputObject);

            if (initialState != defaultState)
                Node.AddTransition(ResourceTransition{ virtualResourceHandle, defaultState, initialState });

            if (temp) {
                Node.retiredObjects.push_back(outputObject);
                temporaryObjects.push_back(outputObject);
            }

            acquiredResources.push_back({ virtualResource, overlap });
            Node.subNodeTracking.push_back({ virtualResourceHandle, initialState, initialState });

            return  virtualResourceHandle;
        }

        return InvalidHandle_t;
	}


    /************************************************************************************************/


    ResusableResourceQuery FrameGraphNode::FindReuseableResource(PoolAllocatorInterface& allocator, const size_t allocationSize, const uint32_t flags, FrameResources& handler)
    {
        for (auto sourceNode : Sources)
        {
            if (sourceNode->retiredObjects.size())
            {
                for (auto& retiredResource : sourceNode->retiredObjects)
                {
                    auto resourceObject = handler.GetAssetObject(retiredResource.handle);
                    resourceObject->resourceFlags;

                    if  (resourceObject->virtualState == VirtualResourceState::Virtual_Released &&
                        (resourceObject->resourceFlags & flags) == flags)
                    {   // Reusable candidate found!
                        auto resourceSize = handler.renderSystem.GetResourceSize(resourceObject->shaderResource);

                        if (resourceSize > allocationSize)
                        {
                            resourceObject->virtualState = VirtualResourceState::Virtual_Reused;
                            return { retiredResource.handle, true };
                        }
                    }
                    else continue;
                }
            }
            else
            {
                const auto [resource, success] = sourceNode->FindReuseableResource(allocator, allocationSize, flags, handler);

                if (success)
                    return { resource, success };
            }
        }

        return { InvalidHandle_t, false };
    }


    FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(PoolAllocatorInterface& allocator, const GPUResourceDesc desc, DeviceResourceState initialState, bool temp)
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

        if (!((allocator.Flags() & NeededFlags) == NeededFlags))
        {
            FK_LOG_ERROR("Allocation with incompatible heap was detected!");
            return InvalidHandle_t;
        }


#if 1
        auto [virtualResource, overlap] = allocator.Acquire(desc, temp);

        if (virtualResource == InvalidHandle_t)
            return InvalidHandle_t;
#else
        auto allocationSize = Resources->renderSystem.GetAllocationSize(desc);


        ResourceHandle virtualResource  = InvalidHandle_t;
        ResourceHandle overlap          = InvalidHandle_t;

        auto [reuseableResource, found] = Node.FindReuseableResource(allocator, allocationSize, NeededFlags, *Resources);

        if (found || reuseableResource != InvalidHandle_t)
        {
            auto reusedResource = Resources->Resources[reuseableResource].shaderResource;
            auto deviceResource = Resources->renderSystem.GetHeapOffset(reusedResource);
        }
        else
        {
            auto [resource, _overlap] = allocator.Acquire(desc, temp);

            virtualResource = resource;
            overlap         = _overlap;
        }
#endif

		FrameObject virtualObject       = FrameObject::VirtualObject();
		virtualObject.shaderResource    = virtualResource;
		virtualObject.State             = initialState;
        virtualObject.virtualState      = VirtualResourceState::Virtual_Temporary;
        virtualObject.pool              = &allocator;
        virtualObject.resourceFlags     = NeededFlags;

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
        Node.createdObjects.push_back(outputObject);

        if (initialState != outputObject.neededState)
            Node.AddTransition(ResourceTransition{ virtualResourceHandle, outputObject.neededState, initialState });

        if (temp) {
            Node.retiredObjects.push_back(outputObject);
            temporaryObjects.push_back(outputObject);
        }

        acquiredResources.push_back({ virtualResource, overlap });
        Node.subNodeTracking.push_back({ virtualResourceHandle, initialState, initialState });

		return  virtualResourceHandle;
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


	FrameResourceHandle	FrameGraphNodeBuilder::UnorderedAccess(ResourceHandle handle, DeviceResourceState state)
	{
        if (auto frameResource = AddWriteableResource(handle, state); frameResource != InvalidHandle_t)
            return frameResource;

        Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

        return AddWriteableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::VertexBuffer(SOResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceResourceState::DRS_VERTEXBUFFER);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::StreamOut(SOResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceResourceState::DRS_STREAMOUT);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadTransition(FrameResourceHandle handle, DeviceResourceState state)
	{
		return AddReadableResource(handle, state);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteTransition(FrameResourceHandle handle, DeviceResourceState state)
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


    void FrameGraph::_SubmitFrameGraph(iAllocator& threadLocalAllocator)
    {
        ProfileFunction();

        Vector<FrameGraphNodeWork> taskList{ &threadLocalAllocator };

        for (auto& N : Nodes)
            ProcessNode(&N, Resources, taskList, threadLocalAllocator);

        auto& renderSystem = Resources.renderSystem;
        Vector<Context*> contexts{ &threadLocalAllocator };

        struct SubmissionWorkRange
        {
            Vector<FrameGraphNodeWork>::Iterator begin;
            Vector<FrameGraphNodeWork>::Iterator end;
        };


        const size_t workerCount    = Min(taskList.size(), renderSystem.threads.GetThreadCount());
        const size_t blockSize      = (taskList.size() / workerCount) + (taskList.size() % workerCount == 0 ? 0 : 1);

        static_vector<SubmissionWorkRange, 64> workList;
        for (size_t I = 0; I < workerCount; I++)
        {
            workList.push_back(
                {
                    taskList.begin() + Min((I + 0) * blockSize, taskList.size()),
                    taskList.begin() + Min((I + 1) * blockSize, taskList.size())
                });
        }

        class RenderWorker : public iWork
        {
        public:
            RenderWorker(SubmissionWorkRange IN_work, Context* IN_ctx, iAllocator& persistentAllocator) :
                iWork{ &persistentAllocator },
                work{ IN_work },
                ctx{ IN_ctx } {}

            void Run(iAllocator& tempAllocator) override
            {
                ProfileFunction();

                std::for_each(work.begin, work.end,
                    [&](auto& item)
                    {
                        ProfileFunction();

                        item(ctx, tempAllocator);
                    });
                ctx->FlushBarriers();
            }

            void Release() {}

            SubmissionWorkRange work;
            Context* ctx;
        };

        FlexKit::WorkBarrier barrier{ threads, &threadLocalAllocator };
        static_vector<RenderWorker*, 64> workers;

        for (auto& work : workList)
        {
            auto& context = renderSystem.GetCommandList();
            contexts.push_back(&context);

#if USING(DEBUGGRAPHICS)
            auto ID = fmt::format("CommandList{}", contexts.size() - 1);
            context.SetDebugName(ID.c_str());
#endif

            auto& worker = threadLocalAllocator.allocate<RenderWorker>(work, &context, threadLocalAllocator);
            workers.push_back(&worker);
            barrier.AddWork(worker);
        }

        if (!contexts.size())
            return;

        auto& ctx = *contexts.front();
        for (auto acquired : acquiredResources)
            ctx.AddAliasingBarrier(acquired, InvalidHandle_t);

		for (auto& worker : workers)
			threads.AddWork(*worker);

		barrier.Join();

		UpdateResourceFinalState();

		renderSystem.Submit(contexts);
	}


	UpdateTask& FrameGraph::SubmitFrameGraph(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator* persistentAllocator)
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

		return dispatcher.Add<SubmitData>(
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
                ProfileFunction();
				data.frameGraph->_SubmitFrameGraph(threadAllocator);
			});
	}


	/************************************************************************************************/


	void FrameGraph::UpdateResourceFinalState()
	{
        ProfileFunction();

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
				Data.BackBuffer = Builder.RenderTarget(backBuffer);
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
				Data.DepthBuffer = Builder.DepthTarget(Handle);
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
				Data.BackBuffer = Builder.Present(window.GetBackBuffer());
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
