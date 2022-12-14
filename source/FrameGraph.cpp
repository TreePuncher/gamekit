#include "graphics.h"
#include "FrameGraph.h"
#include "Logging.h"
#include <fmt/core.h>

namespace FlexKit
{	/************************************************************************************************/

	FrameGraphNode::FrameGraphNode(FN_NodeAction IN_action, void* IN_nodeData, iAllocator* IN_allocator) :
			InputObjects	{ IN_allocator	},
			OutputObjects	{ IN_allocator	},
			Sources			{ IN_allocator	},
			barriers		{ IN_allocator	},
			NodeAction		{ IN_action		},
			nodeData		{ IN_nodeData	},
			Executed		{ false			},
			subNodeTracking { IN_allocator	},

			createdObjects	{ IN_allocator	},
			retiredObjects	{ IN_allocator	},
			acquiredObjects	{ IN_allocator	} {}


	FrameGraphNode::FrameGraphNode(FrameGraphNode&& RHS) :
			Sources			{ std::move(RHS.Sources)			},
			InputObjects	{ std::move(RHS.InputObjects)		},
			OutputObjects	{ std::move(RHS.OutputObjects)		},
			barriers		{ std::move(RHS.barriers)			},
			NodeAction		{ std::move(RHS.NodeAction)			},
			nodeData		{ std::move(RHS.nodeData)			},
			retiredObjects	{ std::move(RHS.retiredObjects)		},
			subNodeTracking	{ std::move(RHS.subNodeTracking)	},
			Executed		{ std::move(RHS.Executed)			},
			createdObjects	{ std::move(RHS.createdObjects)		},
			acquiredObjects	{ std::move(RHS.acquiredObjects)	} {}


	/************************************************************************************************/


	void FrameGraphNode::HandleBarriers(FrameResources& Resources, Context& Ctx)
	{
		//for (const auto& virtualResource : acquiredObjects)
		//	Ctx.AddAliasingBarrier(virtualResource.overlap, virtualResource.resource);

		Ctx.AddBarriers(barriers);
	}


	/************************************************************************************************/


	void FrameGraphNode::AddBarrier(const Barrier& transition)
	{
		barriers.push_back(transition);
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
			const auto currentAccess	= localResourceDescriptor.access;
			const auto currentLayout	= localResourceDescriptor.layout;

			const auto finalAccess	= localResourceDescriptor.finalAccess;
			const auto finalLayout	= localResourceDescriptor.finalLayout;

			if (currentAccess == finalAccess && currentLayout == finalLayout)
				continue;

			auto& resource = resources.Resources[localResourceDescriptor.resource];

			switch (resource.Type)
			{
			case FrameObjectResourceType::OT_StreamOut:
				DebugBreak();
				FK_ASSERT(0, "UN-IMPLEMENTED BLOCK!");
				//ctx->AddStreamOutBarrier(resource.SOBuffer, currentState, nodeState);
				break;
			case FrameObjectResourceType::OT_Virtual:
			case FrameObjectResourceType::OT_DepthBuffer:
			case FrameObjectResourceType::OT_BackBuffer:
			case FrameObjectResourceType::OT_RenderTarget:
			case FrameObjectResourceType::OT_Resource:
			{
				const auto localObject	= localResourceDescriptor.resource;
				const auto object		= resources.GetResourceObject(localObject);
				const auto resource		= object->shaderResource;

				switch (object->dimensions)
				{
					case TextureDimension::Buffer:
						ctx->AddBufferBarrier(resource, currentAccess, finalAccess, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_All);
						break;
					case TextureDimension::Texture1D:
					case TextureDimension::Texture2D:
					case TextureDimension::Texture3D:
					case TextureDimension::TextureCubeMap:
					case TextureDimension::Texture2DArray:
					{
						uint subResourceRange;

						ctx->AddTextureBarrier(resource, currentAccess, finalAccess, currentLayout, finalLayout, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_All);
					}	break;
				default:
					break;
				}
				object->dimensions;

			}	break;
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
				ctx->AddGlobalBarrier(resource.shaderResource, retired.neededAccess, DASNOACCESS, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_None);
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
			if (object.source != nullptr || object.source != &Node)
				Node.Sources.push_back(object.source);

		for (auto& object : Node.OutputObjects)
			if(object.source != nullptr || object.source != &Node)
				Node.Sources.push_back(object.source);

		if (Node.Sources.size())
		{
			std::sort(Node.Sources.begin(), Node.Sources.end());
			auto res = std::unique(Node.Sources.begin(), Node.Sources.end());
			Node.Sources.erase(res, Node.Sources.end());
		}

		Node.barriers			+= barriers;
		Node.acquiredObjects	+= acquiredResources;
		Context.Retirees		+= RetiredObjects;

		for (auto& temporary : temporaryObjects)
		{
			auto resource = Resources->Resources[temporary.handle];
			resource.pool->Release(resource.shaderResource, false);
		}

		temporaryObjects.clear();
		barriers.clear();
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::AddDataDependency(UpdateTask& task)
	{
		DataDependencies.push_back(&task);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AccelerationStructureRead(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASACCELERATIONSTRUCTURE_READ, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		DebugBreak();
		//Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

		return AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_READ, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AccelerationStructureWrite(ResourceHandle handle)
	{
		if (auto frameResource = AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_WRITE, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		DebugBreak();
		//Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

		return AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_WRITE, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::PixelShaderResource(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASPixelShaderResource, DeviceLayout_ShaderResource); frameResource != InvalidHandle)
			return frameResource;

		Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectLayout(handle));

		return AddReadableResource(handle, DASPixelShaderResource, DeviceLayout_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::NonPixelShaderResource(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASNonPixelShaderResource, DeviceLayout_ShaderResource); frameResource != InvalidHandle)
			return frameResource;

		Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectLayout(handle));

		return AddReadableResource(handle, DASNonPixelShaderResource, DeviceLayout_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::NonPixelShaderResource(FrameResourceHandle handle)
	{
		return AddReadableResource(handle, DASNonPixelShaderResource, DeviceLayout_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::CopySource(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASCopySrc, DeviceLayout_DecodeWrite); frameResource != InvalidHandle)
			return frameResource;

		DebugBreak();
		//Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

		return AddReadableResource(handle, DASCopySrc, DeviceLayout_DecodeWrite);
	}

	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::CopyDest(ResourceHandle  handle)
	{
		if (auto frameResource = AddWriteableResource(handle, DASCopyDest, DeviceLayout_CopyDst); frameResource != InvalidHandle)
			return frameResource;

		DebugBreak();
		//Context.resources.AddResource(handle, Context.resources.renderSystem.GetObjectState(handle));

		return AddWriteableResource(handle, DASCopyDest, DeviceLayout_CopyDst);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::RenderTarget(ResourceHandle target)
	{
		Barrier barrier;
		barrier.type					= BarrierType::Texture;
		barrier.src						= Sync_All;
		barrier.dst						= Sync_All;
		barrier.accessAfter				= DASRenderTarget;
		barrier.texture.layoutAfter		= DeviceLayout_RenderTarget;

		const auto resourceHandle = AddWriteableResource(target, DASRenderTarget, DeviceLayout_RenderTarget);

		if (resourceHandle == InvalidHandle)
		{
			Resources->AddResource(target, true);
			return RenderTarget(target);
		}

		if (auto resource = Resources->GetResourceObject(resourceHandle); !resource)
			return InvalidHandle;

		return resourceHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::RenderTarget(FrameResourceHandle target)
	{
		return WriteTransition(target, DASRenderTarget);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::Present(ResourceHandle renderTarget)
	{
		auto resourceHandle = AddReadableResource(renderTarget, DeviceAccessState::DASPresent, DeviceLayout_Present);

		if (resourceHandle == InvalidHandle)
		{
			Resources->AddResource(renderTarget, true);
			return Present(renderTarget);
		}

		auto resource = Resources->GetResource(resourceHandle);

		if (!resource)
			return InvalidHandle;

		return resourceHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::DepthRead(ResourceHandle handle)
	{
		if (auto frameObject = AddReadableResource(handle, DeviceAccessState::DASDEPTHBUFFERREAD, DeviceLayout_DepthStencilRead); frameObject == InvalidHandle)
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
		if (auto frameObject = AddWriteableResource(handle, DeviceAccessState::DASDEPTHBUFFERWRITE, DeviceLayout_DepthStencilWrite); frameObject == InvalidHandle)
		{
			Resources->AddResource(handle, true);
			return DepthTarget(handle);
		}
		else
			return frameObject;
	}


	/************************************************************************************************/


	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc& desc, DeviceAccessState access, bool temp)
	{
		ProfileFunction();

		auto NeededFlags = 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2DArray ? DeviceHeapFlags::RenderTarget : 0;

		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Buffer    ? DeviceHeapFlags::UAVBuffer : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::RenderTarget) ? DeviceHeapFlags::RenderTarget : 0;

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

			ResourceHandle virtualResource	= InvalidHandle;
			ResourceHandle overlap			= InvalidHandle;

			auto [reuseableResource, found] = Node.FindReuseableResource(*memoryPool, allocationSize, NeededFlags, *Resources);

			if (found || reuseableResource != InvalidHandle)
			{
				auto reusedResource = Resources->Resources[reuseableResource].shaderResource;
				auto deviceResource = Resources->renderSystem.GetHeapOffset(reusedResource);
			}
			else
			{
				auto [resource, _overlap, offset, heap] = memoryPool->AcquireDeferred(desc, temp);
				Context.AddDeferredCreation(resource, offset, heap, desc);

				virtualResource	= resource;
				overlap			= _overlap;
			}

			if (virtualResource == InvalidHandle)
				return InvalidHandle;

			FrameObject virtualObject		= FrameObject::VirtualObject();
			virtualObject.shaderResource	= virtualResource;
			virtualObject.dimensions		= desc.Dimensions;
			virtualObject.layout			= desc.initialLayout;
			virtualObject.access			= temp ? DASNOACCESS : access;
			virtualObject.virtualState		= VirtualResourceState::Virtual_Temporary;
			virtualObject.pool				= memoryPool;


			auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
			Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
			Resources->virtualResources.push_back(virtualResourceHandle);

			FrameObjectLink outputObject;
			outputObject.neededAccess	= access;
			outputObject.neededLayout	= desc.initialLayout;
			outputObject.source			= &Node;
			outputObject.handle			= virtualResourceHandle;

			Context.AddWriteable(outputObject);
			Node.OutputObjects.push_back(outputObject);
			Node.createdObjects.push_back(outputObject);

			Barrier barrier;
			barrier.resource		= virtualResource;
			barrier.accessBefore	= DASNOACCESS;
			barrier.accessAfter		= access;
			barrier.src				= DeviceSyncPoint::Sync_None;
			barrier.dst				= DeviceSyncPoint::Sync_All;

			switch (desc.Dimensions)
			{
			case TextureDimension::Buffer:
			{
				barrier.type = BarrierType::Buffer;

			}	break;
			case TextureDimension::Texture1D:
			case TextureDimension::Texture2D:
			case TextureDimension::Texture2DArray:
			case TextureDimension::Texture3D:
			case TextureDimension::TextureCubeMap:
			{
				barrier.type					= BarrierType::Texture;
				barrier.texture.layoutBefore	= DeviceLayout_Unknown;
				barrier.texture.layoutAfter		= desc.initialLayout;
				barrier.texture.flags			= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
			}	break;
			}

			Node.AddBarrier(barrier);

			if (temp) {
				Node.retiredObjects.push_back(outputObject);
				temporaryObjects.push_back(outputObject);
			}

			/*
			struct LocallyTrackedResource
			{
				FrameResourceHandle resource;
				DeviceAccessState	access;
				DeviceLayout		layout;

				DeviceAccessState	finalAccess;
				DeviceLayout		finalLayout;
			};
			*/

			acquiredResources.push_back({ virtualResource, overlap });
			Node.subNodeTracking.push_back({
				.resource		= virtualResourceHandle,
				.access			= access,
				.layout			= desc.initialLayout,
				.finalAccess	= access,
				.finalLayout	= desc.initialLayout });

			return virtualResourceHandle;
		}

		return InvalidHandle;
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
					auto resourceObject = handler.GetResourceObject(retiredResource.handle);
					resourceObject->resourceFlags;

					DebugBreak();
					/*
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
					else */continue;
				}
			}
			else
			{
				const auto [resource, success] = sourceNode->FindReuseableResource(allocator, allocationSize, flags, handler);

				if (success)
					return { resource, success };
			}
		}

		return { InvalidHandle, false };
	}


	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(PoolAllocatorInterface& allocator, const GPUResourceDesc& desc, DeviceAccessState access, bool temp)
	{
		auto NeededFlags = 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2DArray ? DeviceHeapFlags::RenderTarget : 0;

		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Buffer    ? DeviceHeapFlags::UAVBuffer : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::RenderTarget) ? DeviceHeapFlags::RenderTarget : 0;

		if (!((allocator.Flags() & NeededFlags) == NeededFlags))
		{
			FK_LOG_ERROR("Allocation with incompatible heap was detected!");
			return InvalidHandle;
		}


#if 1
		auto [virtualResource, overlap] = allocator.Acquire(desc, temp);

		if (virtualResource == InvalidHandle)
			return InvalidHandle;
#else
		auto allocationSize = Resources->renderSystem.GetAllocationSize(desc);


		ResourceHandle virtualResource  = InvalidHandle;
		ResourceHandle overlap          = InvalidHandle;

		auto [reuseableResource, found] = Node.FindReuseableResource(allocator, allocationSize, NeededFlags, *Resources);

		if (found || reuseableResource != InvalidHandle)
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

		DebugBreak();

		FrameObject virtualObject		= FrameObject::VirtualObject();
		virtualObject.shaderResource	= virtualResource;
		virtualObject.virtualState		= VirtualResourceState::Virtual_Temporary;
		virtualObject.pool				= &allocator;
		virtualObject.resourceFlags		= NeededFlags;

		Resources->renderSystem.SetDebugName(virtualResource, "Un-named virtual resources");

		auto virtualResourceHandle = FrameResourceHandle{ Resources->Resources.emplace_back(virtualObject) };
		Resources->Resources[virtualResourceHandle].Handle = virtualResourceHandle;
		Resources->virtualResources.push_back(virtualResourceHandle);

		FrameObjectLink outputObject;
			outputObject.neededAccess	= access;
			outputObject.neededLayout	= desc.initialLayout;
			outputObject.source			= &Node;
			outputObject.handle			= virtualResourceHandle;

		//FK_LOG_9("Allocated Resource: %u", Resources->GetObjectResource(virtualResource));
		Resources->renderSystem.SetDebugName(virtualResource, "Virtual Resource");


		Context.AddWriteable(outputObject);
		Node.OutputObjects.push_back(outputObject);
		Node.createdObjects.push_back(outputObject);

		
		if (!CheckCompatibleAccessState(desc.initialLayout, access) || desc.initialLayout != outputObject.neededLayout)
		{
			DebugBreak();
			Barrier barrier;
			Node.AddBarrier(barrier);
		}

		if (temp) {
			Node.retiredObjects.push_back(outputObject);
			temporaryObjects.push_back(outputObject);
		}

		acquiredResources.push_back({ virtualResource, overlap });
		//Node.subNodeTracking.push_back({ virtualResourceHandle, initialState, initialState });

		return  virtualResourceHandle;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ReleaseVirtualResource(FrameResourceHandle handle)
	{
		auto& resource				= Resources->Resources[handle];

		FrameObjectLink freeObject;
		freeObject.neededAccess		= DeviceAccessState::DASNOACCESS;
		freeObject.source			= &Node;
		freeObject.handle			= handle;

		resource.virtualState		= VirtualResourceState::Virtual_Released;

		Node.retiredObjects.push_back(freeObject);
		resource.pool->Release(resource.shaderResource, false);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::UnorderedAccess(ResourceHandle handle, DeviceAccessState state)
	{
		if (auto frameResource = AddWriteableResource(handle, state, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		Context.resources.AddResource(handle);

		return AddWriteableResource(handle, state, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::VertexBuffer(SOResourceHandle handle)
	{
		return AddReadableResource(handle, DeviceAccessState::DASVERTEXBUFFER, DeviceLayout_Common);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::StreamOut(SOResourceHandle handle)
	{
		return AddWriteableResource(handle, DeviceAccessState::DASSTREAMOUT, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadTransition(FrameResourceHandle handle, DeviceAccessState access, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints)
	{
		auto object = Resources->GetResourceObject(handle);

		return AddReadableResource(handle, access, object->layout);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteTransition(FrameResourceHandle handle, DeviceAccessState access, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints)
	{
		auto object = Resources->GetResourceObject(handle);

		return AddWriteableResource(handle, access, object->layout);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::SetDebugName(FrameResourceHandle handle, const char* debugName)
	{
		if (auto res = Resources->GetResource(handle); res != InvalidHandle)
			Resources->renderSystem.SetDebugName(res, debugName);
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

		if (!taskList.size())
			return;

		auto& renderSystem = Resources.renderSystem;
		Vector<Context*> contexts{ &threadLocalAllocator };

		struct SubmissionWorkRange
		{
			Vector<FrameGraphNodeWork>::Iterator begin;
			Vector<FrameGraphNodeWork>::Iterator end;
		};


		const size_t workerCount	= 4;//Min(taskList.size(), renderSystem.threads.GetThreadCount());
		const size_t blockSize		= (taskList.size() / workerCount) + (taskList.size() % workerCount == 0 ? 0 : 1);

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
				iWork	{ &persistentAllocator },
				work	{ IN_work },
				ctx		{ IN_ctx } {}

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

		ResourceContext.WaitFor();

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
				DebugBreak();

				auto SOBuffer	= I.SOBuffer;

				//auto state		= I.State;
				//Resources.renderSystem.SetObjectAccessState(SOBuffer, state);
			}	break;
			case OT_BackBuffer:
			case OT_DepthBuffer:
			case OT_RenderTarget:
			case OT_Resource:
			{
				auto shaderResource	= I.shaderResource;
				auto layout			= I.layout;

				Resources.renderSystem.SetObjectLayout(shaderResource, layout);
			}	break;
			case OT_Virtual:
			{
				auto shaderResource = I.shaderResource;
				auto layout			= I.layout;

				Resources.renderSystem.SetObjectLayout(shaderResource, layout);

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
				InvalidHandle,
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
		struct ClearDepthBuffer
		{
			FrameResourceHandle DepthBuffer;
			float				ClearDepth;
		};

		auto& Pass = Graph.AddNode<ClearDepthBuffer>(
			ClearDepthBuffer{
				InvalidHandle,
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
