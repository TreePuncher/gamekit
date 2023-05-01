#include "graphics.h"
#include "FrameGraph.h"
#include "Logging.h"
#include <fmt/core.h>

namespace FlexKit
{	/************************************************************************************************/


	FrameGraphNode::FrameGraphNode(FrameGraphNodeHandle IN_handle, FN_NodeGetWorkItems IN_action, void* IN_nodeData, iAllocator* IN_allocator) :
			handle			{ IN_handle		},
			inputObjects	{ IN_allocator	},
			outputObjects	{ IN_allocator	},
			sources			{ IN_allocator	},
			barriers		{ IN_allocator	},
			nodeAction		{ IN_action		},
			nodeData		{ IN_nodeData	},
			executed		{ false			},
			subNodeTracking { IN_allocator	},

			createdObjects	{ IN_allocator	},
			retiredObjects	{ IN_allocator	},
			acquiredObjects	{ IN_allocator	},
			dataDependencies{ IN_allocator	} {}


	FrameGraphNode::FrameGraphNode(FrameGraphNode&& RHS) :
			handle			{ RHS.handle						},
			sources			{ std::move(RHS.sources)			},
			inputObjects	{ std::move(RHS.inputObjects)		},
			outputObjects	{ std::move(RHS.outputObjects)		},
			barriers		{ std::move(RHS.barriers)			},
			nodeAction		{ std::move(RHS.nodeAction)			},
			nodeData		{ std::move(RHS.nodeData)			},
			retiredObjects	{ std::move(RHS.retiredObjects)		},
			subNodeTracking	{ std::move(RHS.subNodeTracking)	},
			executed		{ std::move(RHS.executed)			},
			createdObjects	{ std::move(RHS.createdObjects)		},
			acquiredObjects	{ std::move(RHS.acquiredObjects)	},
			dataDependencies{ std::move(RHS.dataDependencies)	} {}


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


	void FrameGraphNode::RestoreResourceStates(Context* ctx, FrameResources& frameResources, LocallyTrackedObjectList& locallyTrackedObjects)
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

			auto& resource = frameResources.objects[localResourceDescriptor.resource];

			switch (resource.type)
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
				const auto object		= frameResources.GetResourceObject(localObject);
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
			auto& object_ref = frameResources.objects[retired.handle];
			auto accessState = object_ref.lastSubmission == submissionID ? retired.neededAccess : DASCommon;

			if (object_ref.virtualState == VirtualResourceState::Virtual_Released)
			{
				switch (retired.neededLayout)
				{
					// Layout Transition required
				case DeviceLayout_DepthStencilRead:
				case DeviceLayout_DepthStencilWrite:
						ctx->AddTextureBarrier(object_ref.shaderResource, accessState, DASNOACCESS, retired.neededLayout, DeviceLayout_Undefined, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_None);
					break;
				default:
						ctx->AddGlobalBarrier(object_ref.shaderResource, accessState, DASNOACCESS, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_None);
					break;
				}
			}
		}
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraph::AddResource(ResourceHandle resource)
	{
		return resources.AddResource(resource);
	}


	FrameResourceHandle FrameGraph::AddOutput(ResourceHandle resource)
	{
		auto temp = resources.AddResource(resource);
		resources.outputObjects.push_back(temp);

		return temp;
	}

	/************************************************************************************************/


	void FrameGraph::AddMemoryPool(PoolAllocatorInterface* allocator)
	{
		resources.AddMemoryPool(allocator);
	}


	/************************************************************************************************/


	void FrameGraph::AddTaskDependency(UpdateTask& task)
	{
		globalDependencies.push_back(&task);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::BuildNode(FrameGraph* FrameGraph)
	{	// Builds Nodes Linkages, Transitions
		// Process Transitions
		for (auto acquire : acquiredResources)
			FrameGraph->acquiredResources.push_back(acquire.resource);

		node.barriers			+= barriers;
		node.acquiredObjects	+= acquiredResources;
		node.sources			+= inputNodes;
		context.retirees		+= retiredObjects;

		if (node.sources.size())
		{
			std::sort(node.sources.begin(), node.sources.end());
			auto res = std::unique(node.sources.begin(), node.sources.end());
			node.sources.erase(res, node.sources.end());
		}

		std::ranges::sort(node.sources);
		auto res = std::ranges::unique(node.sources);
		node.sources.erase(res.begin(), res.end());

		for (auto& temporary : temporaryObjects)
		{
			auto& object_ref = resources->objects[temporary.handle];
			object_ref.pool->Release(object_ref.shaderResource, FrameGraph->GetRenderSystem().directSubmissionCounter, false);
			object_ref.virtualState = VirtualResourceState::Virtual_Released;
		}

		temporaryObjects.clear();
		barriers.clear();
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::BuildPass(FrameGraph* FrameGraph, FrameGraphNode& endNode)
	{
		for (auto acquire : acquiredResources)
			FrameGraph->acquiredResources.push_back(acquire.resource);

		for (auto& object : node.inputObjects)
			if (object.source != node.handle)
				node.sources.push_back(object.source);

		for (auto& object : node.outputObjects)
			if (object.source != node.handle)
				node.sources.push_back(object.source);

		node.barriers += barriers;
		node.acquiredObjects += acquiredResources;
		context.retirees += retiredObjects;

		for (auto& nodeResource : node.subNodeTracking)
		{
			auto resourceObject = context.frameResources.GetResourceObject(nodeResource.resource);

			for (auto& user : resourceObject->lastUsers)
				if (user == node.handle)
					user = endNode.handle;
		}

		for (auto& temporary : temporaryObjects)
		{
			auto& object_ref = resources->objects[temporary.handle];
			object_ref.pool->Release(object_ref.shaderResource, false);
		}

		endNode.subNodeTracking = node.subNodeTracking;
		endNode.outputObjects	= std::move(node.outputObjects);
		endNode.sources			= std::move(node.sources);

		for (auto& outputObject : node.outputObjects)
		{
			outputObject.source = endNode.handle;
		}

		temporaryObjects.clear();
		barriers.clear();
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::AddDataDependency(UpdateTask& task)
	{
		node.dataDependencies.push_back(&task);
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::AddNodeDependency(FrameGraphNodeHandle dependency)
	{
		node.sources.push_back(dependency);
	}


	/************************************************************************************************/


	FrameGraphNodeHandle FrameGraphNodeBuilder::GetNodeHandle() const
	{
		return node.handle;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::CreateConstantBuffer()
	{
		FrameResourceHandle resource = context.frameResources.AddConstantBuffer();

		auto& object_ref = resources->objects[resource];
		object_ref.lastUsers.push_back(node.handle);

		FrameObjectLink dependency{ resource, node.handle };
		context.AddReadable(dependency);

		node.subNodeTracking.push_back({ resource });

		return resource;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::ReadConstantBuffer(FrameResourceHandle constantBuffer)
	{
		auto& object_ref = resources->objects[constantBuffer];

		for(auto& source : object_ref.lastUsers)
			node.sources.push_back(source);

		object_ref.lastUsers.push_back(node.handle);

		FrameObjectLink dependency{ constantBuffer, node.handle };
		context.AddReadable(dependency);

		node.subNodeTracking.push_back({ constantBuffer });

		return constantBuffer;
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AccelerationStructureRead(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASACCELERATIONSTRUCTURE_READ, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

		return AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_READ, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::AccelerationStructureWrite(ResourceHandle handle)
	{
		if (auto frameResource = AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_WRITE, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

		return AddWriteableResource(handle, DASACCELERATIONSTRUCTURE_WRITE, DeviceLayout_UnorderedAccess);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::Common(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASCommon, DeviceLayout_Common); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

		return AddReadableResource(handle, DASCommon, DeviceLayout_Common);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::PixelShaderResource(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASPixelShaderResource, DeviceLayout_ShaderResource); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

		return AddReadableResource(handle, DASPixelShaderResource, DeviceLayout_ShaderResource);
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::NonPixelShaderResource(ResourceHandle handle)
	{
		if (auto frameResource = AddReadableResource(handle, DASNonPixelShaderResource, DeviceLayout_ShaderResource); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

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

		context.frameResources.AddResource(handle);

		return AddReadableResource(handle, DASCopySrc, DeviceLayout_DecodeWrite);
	}

	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::CopyDest(ResourceHandle  handle)
	{
		if (auto frameResource = AddWriteableResource(handle, DASCopyDest, DeviceLayout_CopyDst); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

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
			resources->AddResource(target);
			return RenderTarget(target);
		}

		if (auto resource = resources->GetResourceObject(resourceHandle); !resource)
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
			resources->AddResource(renderTarget);
			return Present(renderTarget);
		}

		auto resource = resources->GetResource(resourceHandle);

		if (!resource)
			return InvalidHandle;

		return resourceHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::DepthRead(ResourceHandle handle)
	{
		if (auto frameObject = AddReadableResource(handle, DeviceAccessState::DASDEPTHBUFFERREAD, DeviceLayout_DepthStencilRead); frameObject == InvalidHandle)
		{
			resources->AddResource(handle);
			return DepthRead(handle);
		}
		else
			return frameObject;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::DepthTarget(ResourceHandle handle, DeviceAccessState finalAccessState)
	{
		if (auto frameObject = AddWriteableResource(handle, DeviceAccessState::DASDEPTHBUFFERWRITE, DeviceLayout_DepthStencilWrite,
													{ { finalAccessState, GuessLayoutFromAccess(finalAccessState), } });
			frameObject == InvalidHandle)
		{
			resources->AddResource(handle);
			return DepthTarget(handle, finalAccessState);
		}
		else
			return frameObject;
	}


	/************************************************************************************************/


	DeviceLayout GuessLayoutFromAccess(DeviceAccessState access)
	{
		switch (access)
		{
		case DASRetired:
			return DeviceLayout_Unknown;
		case DASPresent:
			return DeviceLayout_Present;
		case DASRenderTarget:
			return DeviceLayout_RenderTarget;
		case DASPixelShaderResource:
			return DeviceLayout_ShaderResource;
		case DASUAV:
			return DeviceLayout_UnorderedAccess;
		case DASSTREAMOUT:
			return DeviceLayout_UnorderedAccess;
		case DASVERTEXBUFFER:
			return DeviceLayout_GenericRead;
		case DASDEPTHBUFFER:
			return DeviceLayout_DepthStencilWrite;
		case DASDEPTHBUFFERREAD:
			return DeviceLayout_DepthStencilRead;
		case DASDEPTHBUFFERWRITE:
			return DeviceLayout_DepthStencilWrite;
		case DASACCELERATIONSTRUCTURE_WRITE:
		case DASACCELERATIONSTRUCTURE_READ:
			return DeviceLayout_UnorderedAccess;
		case DASPREDICATE:
			return DeviceLayout_GenericRead;
		case DASINDIRECTARGS:
			return DeviceLayout_GenericRead;
		case DASNonPixelShaderResource:
			return DeviceLayout_ShaderResource;
		case DASCopyDest:
			return DeviceLayout_CopyDst;
		case DASCopySrc:
			return DeviceLayout_CopySrc;
		case DASINDEXBUFFER:
			return DeviceLayout_Common;
		case DASGenericRead:
			return DeviceLayout_GenericRead;
		case DASCommon:
			return DeviceLayout_Common;
		case DASShadingRateSrc:
			return DeviceLayout_ShadingRateSrc;
		case DASShadingRateDst:
			return DeviceLayout_UnorderedAccess;
		case DASDecodeWrite:
			return DeviceLayout_DecodeWrite;
		case DASProcessRead:
			return DeviceLayout_ProcessRead;
		case DASProcessWrite:
			return DeviceLayout_ProcessWrite;
		case DASEncodeRead:
			return DeviceLayout_EncodeRead;
		case DASEncodeWrite:
			return DeviceLayout_EncodeWrite;
		case DASResolveRead:
			return DeviceLayout_ResolveSrc;
		case DASResolveWrite:
			return DeviceLayout_ResolveDst;
		case DASNOACCESS:
		case DASERROR:
		case DASUNKNOWN:
			return DeviceLayout_Unknown;
		}

		std::unreachable();
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::AcquireResourceHandle(DeviceAccessState access, DeviceLayout layout, PoolAllocatorInterface* pool)
	{
		FrameObject virtualObject		= FrameObject::VirtualObject(*allocator);
		virtualObject.shaderResource	= InvalidHandle;
		virtualObject.dimensions		= TextureDimension::Buffer;
		virtualObject.layout			= layout;
		virtualObject.access			= access;
		virtualObject.virtualState		= VirtualResourceState::Virtual_Null;
		virtualObject.pool				= pool;

		auto virtualResourceHandle	= FrameResourceHandle{ resources->objects.emplace_back(virtualObject) };
		auto& object_ref			= resources->objects[virtualResourceHandle];
		object_ref.handle			= virtualResourceHandle;
		object_ref.lastUsers.push_back(node.handle);

		return virtualResourceHandle;
	}


	/************************************************************************************************/


	uint32_t	GetNeededFlags(const GPUResourceDesc& desc)
	{
		uint32_t NeededFlags = 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::RenderTarget : 0;
		NeededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2DArray ? DeviceHeapFlags::RenderTarget : 0;

		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Buffer    ? DeviceHeapFlags::UAVBuffer : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::UAVTextures : 0;
		NeededFlags |= (desc.type == ResourceType::RenderTarget) ? DeviceHeapFlags::RenderTarget : 0;

		return NeededFlags;
	}


	/************************************************************************************************/



	FrameResourceHandle	FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc& desc, DeviceAccessState access, VirtualResourceScope lifeSpan)
	{
		ProfileFunction();

		const uint32_t neededFlags = GetNeededFlags(desc);

		for(auto memoryPool = resources->FindMemoryPool(neededFlags); memoryPool; memoryPool = resources->FindMemoryPool(neededFlags))
			return AcquireVirtualResource(desc, access, memoryPool, lifeSpan);

		return InvalidHandle;
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::AcquireVirtualResource(const GPUResourceDesc& desc, DeviceAccessState access, PoolAllocatorInterface* memoryPool, VirtualResourceScope lifeSpan)
	{
		ProfileFunction();

		const uint32_t neededFlags = GetNeededFlags(desc);

		auto allocationSize = resources->renderSystem.GetAllocationSize(desc);

		ResourceHandle virtualResource	= InvalidHandle;
		ResourceHandle overlap			= InvalidHandle;

		auto [reuseableResource, found] = node.FindReuseableResource(*memoryPool, allocationSize, neededFlags, *resources, nodeTable);

		if (found || reuseableResource != InvalidHandle)
		{
			auto reusedResource = resources->objects[reuseableResource].shaderResource;
			auto deviceResource = resources->renderSystem.GetHeapOffset(reusedResource);
		}
		else
		{
			auto [resource, _overlap, offset, heap] = memoryPool->AcquireDeferred(desc, VirtualResourceScope::Temporary == lifeSpan);
			context.AddDeferredCreation(resource, offset, heap, desc);

			virtualResource	= resource;
			overlap			= _overlap;
		}

		if (virtualResource == InvalidHandle)
			return InvalidHandle;

		const auto layout = GuessLayoutFromAccess(access);

		FrameObject virtualObject		= FrameObject::VirtualObject(*allocator);
		virtualObject.shaderResource	= virtualResource;
		virtualObject.dimensions		= desc.Dimensions;
		virtualObject.layout			= layout;
		virtualObject.access			= (VirtualResourceScope::Temporary == lifeSpan) ? DASNOACCESS : access;
		virtualObject.virtualState		= VirtualResourceState::Virtual_Created;
		virtualObject.pool				= memoryPool;

		auto virtualResourceHandle = FrameResourceHandle{ resources->objects.emplace_back(virtualObject) };
		auto& object_ref	= resources->objects[virtualResourceHandle];
		object_ref.handle	= virtualResourceHandle;
		object_ref.lastUsers.push_back(node.handle);

		resources->virtualResources.push_back(virtualResourceHandle);

		FrameObjectLink outputObject;
		outputObject.neededAccess	= access;
		outputObject.neededLayout	= desc.initialLayout;
		outputObject.source			= node.handle;
		outputObject.handle			= virtualResourceHandle;

		context.AddWriteable(outputObject);
		node.outputObjects.push_back(outputObject);
		node.createdObjects.push_back(outputObject);

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
			barrier.texture.layoutBefore	= DeviceLayout_Undefined;
			barrier.texture.layoutAfter		= layout;
			barrier.texture.flags			= D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
		}	break;
		}

		node.AddBarrier(barrier);

		if (VirtualResourceScope::Temporary == lifeSpan) {
			node.retiredObjects.push_back(outputObject);
			temporaryObjects.push_back(outputObject);
		}

		acquiredResources.push_back({ virtualResource, overlap });
		node.subNodeTracking.push_back({
			.resource		= virtualResourceHandle,
			.access			= access,
			.layout			= layout,
			.finalAccess	= access,
			.finalLayout	= layout });

		resources->virtualResourceCount++;

		return virtualResourceHandle;
	}


	/************************************************************************************************/


	ResusableResourceQuery FrameGraphNode::FindReuseableResource(PoolAllocatorInterface& allocator, const size_t allocationSize, const uint32_t flags, FrameResources& handler, std::span<FrameGraphNode> nodes)
	{
		for (auto sourceNodeHandle : sources)
		{
			auto& sourceNode = nodes[sourceNodeHandle];

			if (sourceNode.retiredObjects.size())
			{
				for (auto& retiredResource : sourceNode.retiredObjects)
				{
					auto resourceObject = handler.GetResourceObject(retiredResource.handle);
					resourceObject->resourceFlags;

					//DebugBreak();
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

			const auto [resource, success] = sourceNode.FindReuseableResource(allocator, allocationSize, flags, handler, nodes);

			if (success)
				return { resource, success };
		}

		return { InvalidHandle, false };
	}

	std::optional<InputObject> FrameGraphNode::GetInputObject(FrameResourceHandle handle)
	{
		for (auto& inputObject : inputObjects)
		{
			if (inputObject.handle == handle)
				return inputObject;
		}

		return {};
	}

	FrameResourceHandle  FrameGraphNodeBuilder::AcquireVirtualResource(PoolAllocatorInterface& poolAllocator, const GPUResourceDesc& desc, DeviceAccessState access, VirtualResourceScope lifeSpan)
	{
		auto neededFlags = 0;
		neededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::RenderTarget : 0;
		neededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::RenderTarget : 0;
		neededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::RenderTarget : 0;
		neededFlags |= !(desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2DArray ? DeviceHeapFlags::RenderTarget : 0;

		neededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Buffer    ? DeviceHeapFlags::UAVBuffer : 0;
		neededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture1D ? DeviceHeapFlags::UAVTextures : 0;
		neededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture2D ? DeviceHeapFlags::UAVTextures : 0;
		neededFlags |= (desc.type == ResourceType::UnorderedAccess) && desc.Dimensions == TextureDimension::Texture3D ? DeviceHeapFlags::UAVTextures : 0;
		neededFlags |= (desc.type == ResourceType::RenderTarget) ? DeviceHeapFlags::RenderTarget : 0;

		if (!((poolAllocator.Flags() & neededFlags) == neededFlags))
		{
			FK_LOG_ERROR("Allocation with incompatible heap was detected!");
			return InvalidHandle;
		}


#if 1
		auto [virtualResource, overlap] = poolAllocator.Acquire(desc, lifeSpan == VirtualResourceScope::Temporary);

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

		FrameObject virtualObject		= FrameObject::VirtualObject(*allocator);
		virtualObject.shaderResource	= virtualResource;
		virtualObject.virtualState		= VirtualResourceState::Virtual_Created;
		virtualObject.pool				= &poolAllocator;
		virtualObject.resourceFlags		= neededFlags;

		resources->renderSystem.SetDebugName(virtualResource, "Un-named virtual resources");

		auto virtualResourceHandle = FrameResourceHandle{ resources->objects.emplace_back(virtualObject) };
		resources->objects[virtualResourceHandle].handle = virtualResourceHandle;
		resources->virtualResources.push_back(virtualResourceHandle);

		FrameObjectLink outputObject;
			outputObject.neededAccess	= access;
			outputObject.neededLayout	= desc.initialLayout;
			outputObject.source			= node.handle;
			outputObject.handle			= virtualResourceHandle;

		//FK_LOG_9("Allocated Resource: %u", Resources->GetObjectResource(virtualResource));
		resources->renderSystem.SetDebugName(virtualResource, "Virtual Resource");


		context.AddWriteable(outputObject);
		node.outputObjects.push_back(outputObject);
		node.createdObjects.push_back(outputObject);

		
		if (!CheckCompatibleAccessState(desc.initialLayout, access) || desc.initialLayout != outputObject.neededLayout)
		{
			DebugBreak();
			Barrier barrier;
			node.AddBarrier(barrier);
		}

		if (lifeSpan == VirtualResourceScope::Temporary) {
			node.retiredObjects.push_back(outputObject);
			temporaryObjects.push_back(outputObject);
		}

		acquiredResources.push_back({ virtualResource, overlap });
		//Node.subNodeTracking.push_back({ virtualResourceHandle, initialState, initialState });

		resources->virtualResourceCount++;

		return  virtualResourceHandle;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::ReleaseVirtualResource(FrameResourceHandle handle)
	{
		auto& resource				= resources->objects[handle];

		FrameObjectLink freeObject;
		freeObject.neededAccess		= resource.access;
		freeObject.source			= node.handle;
		freeObject.handle			= handle;

		resource.virtualState		= VirtualResourceState::Virtual_Released;

		node.retiredObjects.push_back(freeObject);
		resource.pool->Release(resource.shaderResource, false);
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::UnorderedAccess(ResourceHandle handle, DeviceAccessState state)
	{
		if (auto frameResource = AddWriteableResource(handle, state, DeviceLayout_UnorderedAccess); frameResource != InvalidHandle)
			return frameResource;

		context.frameResources.AddResource(handle);

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
		auto object = resources->GetResourceObject(handle);

		return AddReadableResource(handle, access, GuessLayoutFromAccess(access));
	}


	/************************************************************************************************/


	FrameResourceHandle FrameGraphNodeBuilder::WriteTransition(FrameResourceHandle handle, DeviceAccessState access, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints)
	{
		auto object = resources->GetResourceObject(handle);

		return AddWriteableResource(handle, access, GuessLayoutFromAccess(access));
	}


	/************************************************************************************************/


	FrameResourceHandle	FrameGraphNodeBuilder::ReadBack(ReadBackResourceHandle readBackBuffer)
	{
		FrameResourceHandle resource = context.frameResources.AddReadBackBuffer(readBackBuffer);

		auto& object_ref = resources->objects[resource];
		object_ref.lastUsers.push_back(node.handle);

		FrameObjectLink dependency{ resource, node.handle };
		context.AddWriteable(dependency);

		node.subNodeTracking.push_back({ resource });

		return resource;
	}


	/************************************************************************************************/


	void FrameGraphNodeBuilder::SetDebugName(FrameResourceHandle handle, const char* debugName)
	{
		if (auto res = resources->GetResource(handle); res != InvalidHandle)
			resources->renderSystem.SetDebugName(res, debugName);
	}


	/************************************************************************************************/


	size_t FrameGraphNodeBuilder::GetDescriptorTableSize(PSOHandle State, size_t idx) const
	{
		auto rootSig	= resources->renderSystem.GetPSORootSignature(State);
		auto tableSize	= rootSig->GetDesciptorTableSize(idx);

		return tableSize;
	}


	const DesciptorHeapLayout<16>&	FrameGraphNodeBuilder::GetDescriptorTableLayout(PSOHandle State, size_t idx) const
	{
		auto rootSig = resources->renderSystem.GetPSORootSignature(State);
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


	void FrameGraph::_FinalSubmit(iAllocator& threadLocalAllocator)
	{
		ProfileFunction();

		Vector<FrameGraphNode*> orderedWorkList{ threadLocalAllocator };

		auto visitNode =
			[&](this auto self, FrameGraphNode& node) -> void
			{
				if (node.executed)
					return;

				node.executed = true;

				for (auto& handle : node.sources)
					self(nodes[handle]);

				orderedWorkList.push_back(&node);
			};

		for (auto outputObject : resources.outputObjects)
		{
			auto& object_ref = resources.objects[outputObject];

			for (auto handle : object_ref.lastUsers)
				visitNode(nodes[handle]);
		}

		FlexKit::WorkBarrier barrier{ threads, &threadLocalAllocator };
		Vector<FrameGraphNodeWorkItem> workList{ threadLocalAllocator };

		for (auto node : orderedWorkList)
			node->nodeAction(*node, workList, barrier, resources, threadLocalAllocator);

		barrier.AddOnCompletionEvent([&] { ReleaseVirtualObjects(orderedWorkList); });

		std::ranges::stable_sort(
			workList,
			[](const FrameGraphNodeWorkItem& lhs, const FrameGraphNodeWorkItem& rhs) -> bool { return lhs.submissionID < rhs.submissionID; });

		auto& renderSystem = resources.renderSystem;

		Vector<FrameGraphNodeWorkItem*> submissionNodes{ threadLocalAllocator };
		WorkBarrier*	previousBarrier = nullptr;
		SyncPoint		prevTicket;

		struct SubmissionContext
		{
			static_vector<Context*, 16> contexts;
			SyncPoint					prev;
			SyncPoint					sync;
		};

		Vector<SubmissionContext>		queuedSubmissions{ threadLocalAllocator };
		static_vector<WorkBarrier*, 16> barriers;

		const auto tickets = renderSystem.GetSubmissionTicket((uint32_t)submissions.size());

		for(const auto&& [pass, idx] : zip(submissions, iota(0)))
		{
			switch (pass.queue)
			{
			case Submission::Queue::Direct:
			{
				static_vector<Context*, 16> contexts{};

				struct SubmissionWorkRange
				{
					Vector<FrameGraphNodeWorkItem> workList;
				};

				static_vector<SubmissionWorkRange, 64> workerTaskList;

				for (auto& workItem : workList)
				{
					if (workItem.submissionID == idx)
						submissionNodes.push_back(&workItem);
				}

				size_t workBlockSize = 0;
				Vector<FrameGraphNodeWorkItem> workList{ threadLocalAllocator };
				workList.reserve(16);

				for (auto itr = submissionNodes.begin(); itr < submissionNodes.end(); itr++)
				{
					auto& workItem = *itr;

					if (workBlockSize >= 1000)
					{
						workerTaskList.emplace_back(std::move(workList));
						workBlockSize	= 0;
					}

					workList.push_back(*workItem);
					workBlockSize += workItem->workWeight;
				}

				if(workBlockSize < 1000)
					workerTaskList.emplace_back(std::move(workList));

				std::atomic_uint workerCount = 0;

				class RenderWorker : public iWork
				{
				public:
					RenderWorker(SubmissionWorkRange IN_work, Context& IN_ctx, FrameResources& IN_resources, std::atomic_uint& IN_count) :
						iWork		{ nullptr },
						work		{ IN_work },
						resources	{ IN_resources },
						workerCount	{ IN_count },
						ctx			{ IN_ctx } {}

					void Run(iAllocator& tempAllocator) override
					{
						ProfileFunction();
						workerCount++;

						for(auto& item : work.workList)
						{
							ProfileFunction();

							item(ctx, resources, tempAllocator);
						}

						ctx.FlushBarriers();
					}

					void Release() {}

					std::atomic_uint&	workerCount;
					SubmissionWorkRange work;
					Context&			ctx;
					FrameResources&		resources;
				};

				static_vector<RenderWorker*, 64> workers;

				auto& submissionBarrier = threadLocalAllocator.allocate<WorkBarrier>(threads, &threadLocalAllocator);
				barriers.push_back(&submissionBarrier);

				if (previousBarrier)
					submissionBarrier.AddWork(*previousBarrier);

				SyncPoint submissionTicket {
					.syncCounter	= tickets.syncCounter + idx + 1,
					.fence			= tickets.fence };

				for (auto& workerTask : workerTaskList)
				{
					auto& context = renderSystem.GetCommandList(submissionTicket);
					contexts.push_back(&context);

#if USING(DEBUGGRAPHICS)
					auto ID = fmt::format("CommandList{}", contexts.size() - 1);
					context.SetDebugName(ID.c_str());
#endif

					auto& worker = threadLocalAllocator.allocate<RenderWorker>(workerTask, context, resources, workerCount);
					workers.push_back(&worker);
					submissionBarrier.AddWork(worker);
				}

				queuedSubmissions.emplace_back(contexts, prevTicket, submissionTicket);
				prevTicket = submissionTicket;

				if(previousBarrier != nullptr)
					previousBarrier->AddOnCompletionEvent(
						[&queuedSubmissions, idx = queuedSubmissions.size() - 2, &renderSystem, submissionTicket, &submissionBarrier]() mutable
						{
							auto& contexts	= queuedSubmissions[idx].contexts;
							auto prev		= queuedSubmissions[idx].prev;

							//if (prev)
							//	renderSystem.SyncDirectTo(prev);
							//
							//if (contexts.size())
								renderSystem.Submit(contexts);
							//else
							//	renderSystem.Signal(submissionTicket);

							submissionBarrier.JoinLocal();
						});

				previousBarrier = &submissionBarrier;

				directStateContext.WaitFor();
				computeStateContext.WaitFor();
				barrier.JoinLocal();

				for (auto worker : workers)
					threads.AddWork(worker);

				submissionNodes.clear();
			}	break;
			case Submission::Queue::Compute:
			{
				submissionNodes.clear();
				DebugBreak();
			}	break;
			}
		}

		if (previousBarrier)
		{
			previousBarrier->AddOnCompletionEvent(
				[&, &a = queuedSubmissions.back()]() mutable
				{
					if (a.contexts.size())
					{
						FK_LOG_9("Submitting %u", a.sync.syncCounter);

						renderSystem.Submit(a.contexts);
						renderSystem.SignalDirect(tickets.syncCounter);
					}
				});
		}

		if(barriers.size())
			barriers.front()->JoinLocal();

		renderSystem.EndFrame();
		UpdateResourceFinalState();
	}


	/************************************************************************************************/


	uint32_t FrameGraph::SubmitDirect(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator* persistentAllocator)
	{
		if (pendingDirectNodes.empty())
			return -1;

		const auto submissionID = (uint32_t)submissions.size();
		for (auto node : pendingDirectNodes)
			node->submissionID = submissionID;

		directStateContext.SetLastUsed(submissionID);
		directStateContext.usedResources.clear();

		submissions.emplace_back(
			Submission{
				.nodes = std::move(pendingDirectNodes),
				.queue = Submission::Queue::Direct,
				.syncs = directSyncs });

		return (uint32_t)(submissions.size() - 1);
	}


	/************************************************************************************************/


	uint32_t FrameGraph::SubmitCompute(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator* persistentAllocator)
	{
		if (pendingComputeNodes.empty())
			return -1;

		for (auto node : pendingComputeNodes)
			node->submissionID = (uint32_t)submissions.size();

		submissions.emplace_back(
			Submission{
				.nodes = std::move(pendingComputeNodes),
				.queue = Submission::Queue::Compute,
				.syncs = computeSyncs });

		computeSyncs.clear();
		return (uint32_t)(submissions.size() - 1);
	}


	/************************************************************************************************/


	void FrameGraph::SyncDirectTo(uint32_t id)
	{
		directSyncs.push_back(id);
	}


	/************************************************************************************************/


	UpdateTask& FrameGraph::Finish(UpdateDispatcher& dispatcher, RenderSystem* renderSystem, iAllocator* persistentAllocator)
	{
		SubmitDirect(dispatcher, renderSystem, persistentAllocator);
		SubmitCompute(dispatcher, renderSystem, persistentAllocator);

		struct SubmitData
		{
			FrameGraph*		frameGraph;
			RenderSystem*	renderSystem;
			RenderWindow*	renderWindow;
		};

		auto framegraph = this;

		for (auto& node : nodes)
		{
			for (auto& d : node.dataDependencies)
				globalDependencies.push_back(d);
		}

		std::ranges::sort(globalDependencies);
		auto endCap = std::ranges::unique(globalDependencies);

		globalDependencies.resize(globalDependencies.size() - endCap.size());

		auto& node = dispatcher.Add<SubmitData>(
			[&](auto& builder, SubmitData& data)
			{
				FK_LOG_9("Frame Graph Single-Thread Section Begin");
				builder.SetDebugString("Frame Graph Task");

				data.frameGraph		= framegraph;
				data.renderSystem	= renderSystem;

				for (auto dependency : globalDependencies)
					builder.AddInput(*dependency);

				FK_LOG_9("Frame Graph Single-Thread Section End");
			},
			[=](SubmitData& data, iAllocator& threadAllocator)
			{
				ProfileFunction();
				data.frameGraph->_FinalSubmit(threadAllocator);
			});

		globalDependencies.clear();

		return node;
	}


	/************************************************************************************************/


	void FrameGraph::ReleaseVirtualObjects(std::span<FrameGraphNode*> workList)
	{
		const auto r_begin	= workList.end();
		const auto r_end	= workList.begin();

		for (auto& resource : resources.objects)
		{
			if (resource.type			== OT_Virtual &&
				resource.virtualState	== VirtualResourceState::Virtual_Created &&
				resource.pool			!= nullptr)
			{
				auto& users = resource.lastUsers;

				if (r_begin != r_end)
				{
					auto r_itr	= r_begin - 1;

					for (; r_end <= r_itr; r_itr--)
					{
						const auto handle = (*r_itr)->handle;

						if (std::find(users.begin(), users.end(), handle) != users.end())
							break;
					}

					if (r_itr >= workList.begin())
					{
						FrameObjectLink outputObject;
						outputObject.neededAccess	= resource.access;
						outputObject.neededLayout	= resource.layout;
						outputObject.source			= InvalidHandle;
						outputObject.handle			= resource.handle;

						(*r_itr)->retiredObjects.push_back(outputObject);
					}
				}

				resource.virtualState = VirtualResourceState::Virtual_Released;
				resource.pool->Release(resource.shaderResource, GetRenderSystem().directSubmissionCounter, false);
			}
		}
	}


	/************************************************************************************************/


	void FrameGraph::UpdateResourceFinalState()
	{
		ProfileFunction();

		auto& Objects = resources.objects;

		size_t resourcesFreed = 0;

		for (auto& I : Objects)
		{
			switch (I.type)
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

				if (I.lastUsers.size() == 0)
					continue;

				auto nodeIdx = I.lastUsers.back();
				if(nodes[nodeIdx].executed)
					resources.renderSystem.SetObjectLayout(shaderResource, layout);
				else
				{
					auto res = nodes[nodeIdx].GetInputObject(I.handle);
					if (res)
						resources.renderSystem.SetObjectLayout(shaderResource, res.value().initialLayout);
					//else
					//	DebugBreak();
				}
			}	break;
			case OT_Virtual:
			{
				auto shaderResource = I.shaderResource;
				auto layout			= I.layout;

				if (I.virtualState == VirtualResourceState::Virtual_Released)
				{
					resources.renderSystem.ReleaseResource(I.shaderResource);
					resourcesFreed++;
				}
			}	break;
			default:
				FK_ASSERT(false, "UN-IMPLEMENTED BLOCK!");
			}
		}

		if (resources.virtualResourceCount != resourcesFreed)
		{
			DebugBreak();
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
		FG.resources.renderSystem.VertexBuffers.Reset(PushBuffer);
	}


}	/************************************************************************************************/
