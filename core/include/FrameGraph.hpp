#ifndef RENDERGRAPH_H
#define RENDERGRAPH_H

#include "Containers.hpp"
#include "Components.hpp"
#include "DefaultPipelineStates.hpp"
#include "PushBuffers.hpp"
#include "RenderSystemInterface.hpp"
#include "TriMeshResource.hpp"

#include <type_traits>


/************************************************************************************************/


namespace FlexKit
{
	enum RenderObjectState
	{
		RO_Undefined,
		RO_Read,
		RO_Write,
		RO_Present
	};


	/************************************************************************************************/


	enum RenderTargetFormat
	{
		TRF_INT4,
		TRF_SINT4_RGBA,
		TRF_Int4,
		TRF_UInt4,
		TRF_Float4,
		TRF_Auto,
	};


	/************************************************************************************************/


	enum FrameObjectResourceType
	{
		OT_BackBuffer,
		OT_DepthBuffer,
		OT_RenderTarget,
		OT_ConstantBuffer,
		OT_ByteBuffer,
		OT_PVS,
		OT_Query,
		OT_StreamOut,
		OT_Resource,
		OT_ReadBack,
		OT_VertexBuffer,
		OT_IndirectArguments,
		OT_Virtual,
	};


	/************************************************************************************************/


	class FrameGraphNode;

	enum class VirtualResourceState
	{
		NonVirtual,
		Virtual_Null,
		Virtual_Created,
		Virtual_Released,
		Virtual_Persistent,
		Virtual_Reused,
	};

	struct FrameObject
	{
		FrameObject(iAllocator* allocator = nullptr) :
			handle		{ InvalidHandle },
			lastUsers	{ allocator } {}

		FrameObject(const FrameObject& rhs) = default;

		FrameResourceHandle				handle; // For Fast Search
		FrameObjectResourceType			type;

		uint32_t						lastSubmission	= -1;
		DeviceAccessState				access			= DeviceAccessState::DASCommon;
		DeviceLayout					layout			= DeviceLayout::Unknown;

		TextureDimension				dimensions		= TextureDimension::Unknown;
		VirtualResourceState			virtualState	= VirtualResourceState::NonVirtual;
		Vector<FrameGraphNodeHandle>	lastUsers		= nullptr;
		PoolAllocatorInterface*			pool			= nullptr;
		uint32_t						resourceFlags	= 0;

		union
		{
			ResourceHandle			shaderResource;
			CBPushBuffer*			constantBuffer;
			QueryHandle				query;
			ReadBackResourceHandle	readBackBuffer;
			SOResourceHandle		SOBuffer;
		};


		static FrameObject PixelShaderResourceObject(ResourceHandle resource, TextureDimension dimensions, iAllocator& allocator)
		{
			FrameObject shaderResource;
			shaderResource.layout			= DeviceLayout::ShaderResource;
			shaderResource.type				= OT_RenderTarget;
			shaderResource.shaderResource	= resource;
			shaderResource.dimensions		= dimensions;
			shaderResource.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return shaderResource;
		}


		static FrameObject RenderTargetObject(ResourceHandle resource, iAllocator& allocator)
		{
			FrameObject renderTarget;
			renderTarget.layout			= DeviceLayout::RenderTarget;
			renderTarget.type			= OT_RenderTarget;
			renderTarget.shaderResource = resource;
			renderTarget.dimensions		= TextureDimension::Texture2D;
			renderTarget.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return renderTarget;
		}


		static FrameObject BackBufferObject(ResourceHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::RenderTarget)
		{
			FrameObject renderTarget;
			renderTarget.layout			= initialLayout;
			renderTarget.type			= OT_BackBuffer;
			renderTarget.shaderResource = resource;
			renderTarget.dimensions		= TextureDimension::Texture2D;
			renderTarget.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return renderTarget;
		}

		static FrameObject ConstantBufferObject(FrameResourceHandle handle, iAllocator& allocator)
		{
			FrameObject constantBuffer;
			constantBuffer.handle		= handle;
			constantBuffer.type			= OT_ConstantBuffer;
			constantBuffer.dimensions	= TextureDimension::Buffer;
			constantBuffer.lastUsers	= Vector<FrameGraphNodeHandle>{ allocator };

			return constantBuffer;
		}


		static FrameObject DepthBufferObject(ResourceHandle resource, DeviceLayout initialLayout, iAllocator& allocator)
		{
			FrameObject depthBufferTarget;
			depthBufferTarget.layout			= initialLayout;
			depthBufferTarget.type				= OT_DepthBuffer;
			depthBufferTarget.shaderResource	= resource;
			depthBufferTarget.dimensions		= TextureDimension::Texture2D;
			depthBufferTarget.lastUsers			= Vector<FrameGraphNodeHandle>{ allocator };

			return depthBufferTarget;
		}


		static FrameObject ReadBackBufferObject(FrameResourceHandle handle, ReadBackResourceHandle readback, iAllocator& allocator)
		{
			FrameObject readBackBuffer;
			readBackBuffer.handle			= handle;
			readBackBuffer.type				= OT_ReadBack;
			readBackBuffer.dimensions		= TextureDimension::Buffer;
			readBackBuffer.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };
			readBackBuffer.readBackBuffer	= readback;

			return readBackBuffer;
		}


		static FrameObject TextureObject(ResourceHandle resource, DeviceLayout initialLayout, TextureDimension dimensions, iAllocator& allocator)
		{
			FrameObject shaderResource;
			shaderResource.layout			= initialLayout;
			shaderResource.type				= OT_Resource;
			shaderResource.shaderResource	= resource;
			shaderResource.dimensions		= dimensions;
			shaderResource.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return shaderResource;
		}


		static FrameObject SOBufferObject(SOResourceHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::Common)
		{
			FrameObject Streamout;
			Streamout.layout		= initialLayout;
			Streamout.type			= OT_StreamOut;
			Streamout.SOBuffer		= resource;
			Streamout.dimensions	= TextureDimension::Texture2D;
			Streamout.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return Streamout;
		}


		static FrameObject QueryObject(QueryHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::Common)
		{
			FrameObject query;
			query.layout		= initialLayout;
			query.type			= OT_Query;
			query.query			= resource;
			query.dimensions	= TextureDimension::Texture2D;
			query.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return query;
		}


		// Virtual Objects are objects that are not yet backed by any GPU resource
		// Can be used as placeholder handles for temporaries that are not yet created
		// after creation, they will function as a normal frame object, but at the end of the frame,
		// they are destroyed. All virtual objects well be either placed or tiled resources and be GPU resources.
		static FrameObject VirtualObject(iAllocator& allocator)
		{
			FrameObject virtualObject;
			virtualObject.virtualState		= VirtualResourceState::Virtual_Null;
			virtualObject.access			= DeviceAccessState::DASNOACCESS;
			virtualObject.layout			= DeviceLayout::Common;
			virtualObject.type				= OT_Virtual;
			virtualObject.lastUsers			= Vector<FrameGraphNodeHandle>{ allocator };

			return virtualObject;
		}
	};


	/************************************************************************************************/


	struct LocallyTrackedResource
	{
		FrameResourceHandle resource;
		DeviceAccessState	access;
		DeviceLayout		layout;

		DeviceAccessState	finalAccess;
		DeviceLayout		finalLayout;
	};

	typedef Vector<FrameObject>				PassObjectList;
	typedef Vector<LocallyTrackedResource>	LocallyTrackedObjectList;
	typedef Vector<FrameResourceHandle>		TemporaryPassObjectList;

	class FrameGraph;


	/************************************************************************************************/


	class FrameResources
	{
	public:
		FrameResources(IRenderSystem& IN_renderSystem, iAllocator* IN_allocator) :
			allocator				{ IN_allocator },
			memoryPools				{ IN_allocator },
			outputObjects			{ IN_allocator },
			renderSystem			{ &IN_renderSystem },
			objects					{ IN_allocator },
			virtualResources		{ IN_allocator },
			cbAllocators			{ IN_allocator },
			vbAllocators			{ IN_allocator } {
		}

		PassObjectList					objects;
		Vector<FrameResourceHandle>		outputObjects;
		TemporaryPassObjectList			virtualResources;
		Vector<PoolAllocatorInterface*>	memoryPools;

		Vector<ReserveConstantBufferFunction>	cbAllocators;
		Vector<ReserveVertexBufferFunction>		vbAllocators;

		std::mutex		m;

		IRenderSystem*	renderSystem;
		iAllocator*		allocator;

		uint32_t		virtualResourceCount = 0;

		/************************************************************************************************/


		void MarkResourceObjectAsOutput(FrameResourceHandle resource)
		{
			outputObjects.push_back(resource);
		}

		void AddMemoryPool(PoolAllocatorInterface* heapAllocator)
		{
			memoryPools.push_back(heapAllocator);
		}

		void AddBackBuffer(ResourceHandle Handle)
		{
			AddRenderTarget(
				Handle,
				renderSystem->GetObjectLayout(Handle));
		}


		/************************************************************************************************/


		void AddConstantBuffer(ConstantBufferHandle constantBuffer)
		{
			renderSystem->ResetConstantBuffer(constantBuffer);
			cbAllocators.emplace_back(CreateConstantBufferReserveObject(constantBuffer, *renderSystem, allocator));
		}


		/************************************************************************************************/


		void AddVertexBuffer(VertexBufferHandle vertexBuffer)
		{
			renderSystem->ResetVertexBuffer(vertexBuffer);
			vbAllocators.emplace_back(CreateVertexBufferReserveObject(vertexBuffer, *renderSystem, allocator));
		}


		/************************************************************************************************/


		void AddRenderTarget(ResourceHandle handle, DeviceLayout layout = DeviceLayout::Common)
		{
			objects.push_back(
				FrameObject::BackBufferObject(handle, *allocator, layout));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
			MarkResourceObjectAsOutput(objects.back().handle);
		}


		/************************************************************************************************/


		FrameResourceHandle AddConstantBuffer()
		{
			auto handle = FrameResourceHandle{ (uint32_t)objects.size() };
			objects.push_back(FrameObject::ConstantBufferObject(handle, *allocator));

			return handle;
		}


		/************************************************************************************************/


		FrameResourceHandle AddReadBackBuffer(ReadBackResourceHandle)
		{
			auto handle = FrameResourceHandle{ (uint32_t)objects.size() };
			objects.push_back(FrameObject::ConstantBufferObject(handle, *allocator));

			MarkResourceObjectAsOutput(handle);

			return handle;
		}


		/************************************************************************************************/


		void AddDepthBuffer(ResourceHandle Handle)
		{
			AddDepthBuffer(
				Handle,
				renderSystem->GetObjectLayout(Handle));
		}


		/************************************************************************************************/


		void AddDepthBuffer(ResourceHandle Handle, DeviceLayout InitialState)
		{
			objects.push_back(
				FrameObject::DepthBufferObject(Handle, InitialState, *allocator));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
		}


		/************************************************************************************************/


		void AddSOResource(SOResourceHandle handle)
		{
			DeviceLayout layout = renderSystem->GetObjectLayout(handle);

			objects.push_back(
				FrameObject::SOBufferObject(handle, *allocator, layout));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
		}


		/************************************************************************************************/


		FrameResourceHandle AddResource(ResourceHandle handle)
		{
			const DeviceLayout		layout = renderSystem->GetObjectLayout(handle);
			const TextureDimension	dimensions = renderSystem->GetTextureDimension(handle);

			if (auto res = FindFrameResource(handle); res != InvalidHandle)
				return res;
			else
			{
				const auto resourceHandle =
					FrameResourceHandle{
						objects.push_back(FrameObject::TextureObject(handle, layout, dimensions, *allocator)) };

				objects[resourceHandle].handle = resourceHandle;

				return resourceHandle;
			}
		}


		/************************************************************************************************/


		FrameResourceHandle AddResourceMT(ResourceHandle handle)
		{
			std::scoped_lock lock{};

			const DeviceLayout		layout = renderSystem->GetObjectLayout(handle);
			const TextureDimension	dimensions = renderSystem->GetTextureDimension(handle);

			const auto resourceHandle =
				FrameResourceHandle{
					objects.push_back(FrameObject::TextureObject(handle, layout, dimensions, *allocator)) };

			objects[resourceHandle].handle = resourceHandle;

			return resourceHandle;
		}


		/************************************************************************************************/


		void AddQuery(QueryHandle handle)
		{
			DeviceLayout layout = renderSystem->GetObjectLayout(handle);

			objects.push_back(
				FrameObject::QueryObject(handle, *allocator, layout));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
		}


		/************************************************************************************************/


		template<typename TY>
		auto GetDeviceResource(TY handle) const
		{
			return renderSystem->GetDeviceResource(handle);
		}

		DeviceResource_ptr GetDeviceResource(FrameResourceHandle handle) const
		{
			return renderSystem->GetDeviceResource(GetResource(handle));
		}


		/************************************************************************************************/


		const IPipelineState* GetPipelineState(PSOHandle stateID, iAllocator& temp)	const
		{
			if (auto state = renderSystem->GetPSO(stateID, temp); state != nullptr)
				return state;
			else
				return nullptr;

		}

		const IRootSignature* GetPipelineStateRootSig(PSOHandle state) const
		{
			return renderSystem->GetPSORootSignature(state);
		}


		/************************************************************************************************/


		size_t GetVertexBufferOffset(VertexBufferHandle handle, size_t elementSize)
		{
			return renderSystem->GetVertexBufferOffset(handle) / elementSize;
		}


		/************************************************************************************************/


		size_t GetVertexBufferByteOffset(VertexBufferHandle handle)
		{
			return renderSystem->GetVertexBufferOffset(handle);
		}


		/************************************************************************************************/


		DeviceAccessState GetResourceAccess(FrameResourceHandle Handle)
		{
			return objects[Handle].access;
		}


		/************************************************************************************************/


		DeviceLayout GetResourceLayout(FrameResourceHandle Handle)
		{
			return objects[Handle].layout;
		}


		/************************************************************************************************/


		FrameObject* GetResourceObject(FrameResourceHandle Handle)
		{
			return &objects[Handle];
		}


		/************************************************************************************************/


		ResourceHandle GetResource(FrameResourceHandle Handle) const
		{
			return objects[Handle].shaderResource;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(ResourceHandle Handle)
		{
			auto res = find(objects,
				[&](const FrameObject& LHS)
				{
					auto CorrectType = (
						LHS.type == OT_Resource ||
						LHS.type == OT_RenderTarget ||
						LHS.type == OT_DepthBuffer ||
						LHS.type == OT_BackBuffer);

					return (CorrectType && LHS.shaderResource == Handle);
				});

			if (res != objects.end())
				return res->handle;

			return InvalidHandle;
		}


		/************************************************************************************************/


		PoolAllocatorInterface* FindMemoryPool(uint32_t neededFlags) noexcept
		{
			for (auto pool : memoryPools)
			{
				if ((pool->Flags() & neededFlags) == neededFlags)
					return pool;
			}

			return nullptr;
		}


		/************************************************************************************************/

		FrameResourceHandle	FindFrameResource(ShaderResourceHandle Handle)
		{
			auto res = find(objects,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.type == OT_Resource;

					return (CorrectType && LHS.shaderResource == Handle);
				});

			if (res != objects.end())
				return res->handle;

			return InvalidHandle;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(SOResourceHandle handle)
		{
			auto res = find(objects,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.type == OT_StreamOut;

					return (CorrectType && LHS.SOBuffer == handle);
				});

			if (res != objects.end())
				return res->handle;

			return InvalidHandle;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(QueryHandle handle)
		{
			auto res = find(objects,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.type == FrameObjectResourceType::OT_Query;

					return (CorrectType && LHS.query == handle);
				});

			if (res != objects.end())
				return res->handle;

			return InvalidHandle;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(FrameResourceHandle handle)
		{
			auto res = find(objects, [&](const FrameObject& LHS) { return LHS.handle == handle; });

			if (res != objects.end())
				return res->handle;

			return InvalidHandle;
		}


		operator IRenderSystem& ()
		{
			return *renderSystem;
		}


		uint2 GetTextureWH(FrameResourceHandle handle) const
		{
			if (auto res = GetResource(handle); res != InvalidHandle)
				return renderSystem->GetTextureWH(res);
			else
				return { 0, 0 };
		}


		CBPushBuffer ReserveCB(const size_t size)
		{
			for (auto& allocator : cbAllocators)
				return (allocator)(size);

			FK_LOG_ERROR("ReserveCB Failed, no buffers added!");
			return {};
		}


		VBPushBuffer ReserveVB(const size_t size)
		{
			for (auto& allocator : vbAllocators)
				return (allocator)(size);

			FK_LOG_ERROR("ReserveVB Failed, no buffers added!");
			return {};
		}


	};/************************************************************************************************/


	FLEXKITAPI class ResourceHandler
	{
	public:
		ResourceHandler(FrameResources& IN_globalResource, LocallyTrackedObjectList& IN_SubNodeTracking) :
			globalResources{ IN_globalResource },
			SubNodeTracking{ IN_SubNodeTracking } {}


		/************************************************************************************************/

		template<typename TY>
		DeviceResource_ptr		GetDeviceResource(TY handle) const							{ return globalResources.GetDeviceResource(handle); }
		DeviceResource_ptr		GetDeviceResource(FrameResourceHandle handle) const			{ return globalResources.GetDeviceResource(GetResource(handle)); }

		DevicePointer			GetDevicePointer		(auto handle) const
		{
		    return { globalResources.renderSystem->GetDevicePointer(handle) };
		}

		DevicePointer			GetDevicePointer		(FrameResourceHandle handle) const
		{
		    return { globalResources.renderSystem->GetDevicePointer(GetResource(handle)) };
		}

		DeviceAddressRange		GetDevicePointerRange	(FrameResourceHandle handle) const
		{
		    return globalResources.renderSystem->GetDeviceRange(GetResource(handle));
		}

		DeviceAddressRange		GetDevicePointerRange(ResourceHandle handle) const
		{
			return globalResources.renderSystem->GetDeviceRange(handle);
		}

		DeviceAddressRange		GetDevicePointerRange(const ConstantBufferDataSet& dataSet) const
		{
			const auto range = globalResources.renderSystem->GetDeviceRange(dataSet.Handle());

			return DeviceAddressRange{
				.address	= range.address + dataSet.Offset(),
				.size		= dataSet.Size()
			};
		}


		const IPipelineState*	GetPipelineState(PSOHandle state, iAllocator& temp) const			{ return globalResources.GetPipelineState(state, temp); }

		size_t					GetVertexBufferOffset(VertexBufferHandle handle, size_t vertexSize)	{ return globalResources.GetVertexBufferOffset(handle, vertexSize); }
		size_t					GetVertexBufferOffset(VertexBufferHandle handle)					{ return globalResources.GetVertexBufferByteOffset(handle); }

		ResourceHandle			GetResource(FrameResourceHandle handle) const	{ return globalResources.GetResource(handle); }
		DeviceLayout			GetResourceLayout(FrameResourceHandle handle)	{ return globalResources.GetResourceLayout(handle); }
		DeviceAccessState		GetResourceAccess(FrameResourceHandle handle)	{ return globalResources.GetResourceAccess(handle); }
		FrameObject*			GetResourceObject(FrameResourceHandle handle)	{ return globalResources.GetResourceObject(handle); }


		ResourceHandle			operator[](FrameResourceHandle handle) const
		{
			return GetResource(handle);
		}

		/************************************************************************************************/


		FrameResourceHandle     AddResource(ResourceHandle resource) const
		{
			return globalResources.AddResourceMT(resource);
		}


		/************************************************************************************************/


		SOResourceHandle GetSOResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const auto& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.objects,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.handle == handle;
					});

				FK_ASSERT(res != globalResources.objects.end());
				//SubNodeTracking.push_back({ res->Handle, res->State });

				return res->SOBuffer;
			}
			else
				return globalResources.objects[res->resource].SOBuffer;
		}


#if USING(ENABLEDX12)
		/*
		D3D12_VERTEX_BUFFER_VIEW ReadStreamOut(FrameResourceHandle handle, IDirectContext& ctx, size_t vertexSize) const
		{
			auto& res			= _FindSubNodeResource(handle);
			auto SOHandle		= globalResources.objects[res.resource].SOBuffer;
			auto deviceResource = renderSystem().GetDeviceResource(SOHandle);

			DebugBreak();
			if (res.access != DASVERTEXBUFFER && res.layout != DeviceLayout::GenericRead) 
				ctx.AddStreamOutBarrier(SOHandle, res.access, DASVERTEXBUFFER);

			res.access = DASVERTEXBUFFER;

			D3D12_VERTEX_BUFFER_VIEW view = {
				deviceResource->GetGPUVirtualAddress(),
				static_cast<UINT>(renderSystem().GetStreamOutBufferSize(SOHandle)),
				static_cast<UINT>(vertexSize)
			};

			return view;
		}
        */
#endif

		ResourceHandle Transition(const FrameResourceHandle resource, DeviceAccessState access, DeviceLayout layout, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			auto& currentObject	= _FindSubNodeResource(resource);
			auto currentLayout	= currentObject.layout;
			auto& object_ref	= globalResources.objects[resource];
			auto resourceHandle	= object_ref.shaderResource;

			if (access != currentObject.access || layout != currentObject.layout)
			{
				switch (object_ref.dimensions)
				{
				case TextureDimension::Buffer:
					if (access != currentObject.access)
					{
						FK_ASSERT(renderSystem().GetTextureDimension(object_ref.shaderResource) == TextureDimension::Buffer);

						ctx.AddBufferBarrier(object_ref.shaderResource, currentObject.access, access, before, after);
						currentObject.access = access;
					}
					break;
				case TextureDimension::Texture1D:
				case TextureDimension::Texture2D:
				case TextureDimension::Texture2DArray:
				case TextureDimension::Texture3D:
				case TextureDimension::TextureCubeMap:
					ctx.AddTextureBarrier(object_ref.shaderResource, currentObject.access, access, currentObject.layout, layout, before, after);
					currentObject.access = access;
					currentObject.layout = layout;
					break;
				}
			}

			return resourceHandle;
		}

		ResourceHandle CopyDest(FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASCopyDest, DeviceLayout::DirectQueueCopyDst, ctx, before, after);
		}

		ResourceHandle CopySrc(FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASCopySrc, DeviceLayout::DirectQueueCopySrc, ctx, before, after);
		}

		ResourceHandle UAV(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASUAV, DeviceLayout::UnorderedAccess, ctx, before, after);
		}

		ResourceHandle AccelerationStructure(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASACCELERATIONSTRUCTURE_READ, DeviceLayout::Common, ctx, before, after);
		}

		ResourceHandle RenderTarget(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASRenderTarget, DeviceLayout::RenderTarget, ctx, before, after);
		}

		ResourceHandle PixelShaderResource(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASPixelShaderResource, DeviceLayout::DirectQueueShaderResource, ctx, before, after);
		}

		ResourceHandle DepthTarget(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASDEPTHBUFFERWRITE, DeviceLayout::DepthStencilWrite, ctx, before, after);
		}

		ResourceHandle DepthRead(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASDEPTHBUFFERREAD, DeviceLayout::DepthStencilRead, ctx, before, after);
		}

		ResourceHandle NonPixelShaderResource(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASNonPixelShaderResource, DeviceLayout::ShaderResource, ctx, before, after);
		}

		ResourceHandle IndirectArgs(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASINDIRECTARGS, DeviceLayout::DirectQueueGenericRead, ctx, before, after);
		}

		ResourceHandle VertexBuffer(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::DirectQueueGenericRead, ctx, before, after);
		}

		ResourceHandle ResolveDst(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::ResolveDst, ctx, before, after);
		}

		ResourceHandle ResolveSrc(const FrameResourceHandle resource, IDirectContext& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::ResolveSrc, ctx, before, after);
		}

		DeviceResource_ptr ResolveDst(const ReadBackResourceHandle resource, IDirectContext& ctx) const
		{
			return renderSystem().GetDeviceResource(resource);
			//return Transition(resource, DASVERTEXBUFFER, DeviceLayout::ResolveDst, ctx, before, after);
		}

		std::pair<DeviceAccessState, DeviceLayout> GetObjectStates(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const auto& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.objects,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.handle == handle;
					});

				FK_ASSERT(res != globalResources.objects.end());
				//SubNodeTracking.push_back({ res->Handle, res->State });

				return { res->access, res->layout };
			}
			else
				return { res->access, res->layout };
		}

#if USING(ENABLEDX12)
#if 0
		D3D12_STREAM_OUTPUT_BUFFER_VIEW WriteStreamOut(FrameResourceHandle handle, Context* ctx, size_t inputStride) const
		{
			auto& localResourceObj  = _FindSubNodeResource(handle);
			auto& resource          = globalResources.objects[localResourceObj.resource];

			DebugBreak();
			/*
			if (localResourceObj.currentState != DASSTREAMOUT)
				ctx->AddStreamOutBarrier(resource.SOBuffer, localResourceObj.currentState, DASSTREAMOUT);

			localResourceObj.currentState = DASSTREAMOUT;
			*/
			/*
			typedef struct D3D12_STREAM_OUTPUT_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT64 SizeInBytes;
			D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;
			} 	D3D12_STREAM_OUTPUT_BUFFER_VIEW;
			*/

			auto SOHandle = resource.SOBuffer;

			D3D12_STREAM_OUTPUT_BUFFER_VIEW view =
			{
				renderSystem().GetDeviceResource(SOHandle)->GetGPUVirtualAddress(),
				renderSystem().GetStreamOutBufferSize(SOHandle),
				renderSystem().GetSOCounterResource(SOHandle)->GetGPUVirtualAddress(),
			};

			return view;
		}
#endif
#endif

		IRenderSystem& renderSystem() const { return *globalResources.renderSystem; }


		operator IRenderSystem& ()
		{
			return *globalResources.renderSystem;
		}


		uint2 GetTextureWH(FrameResourceHandle handle) const
		{
			return globalResources.GetTextureWH(handle);
		}


		void SetDebugName(FrameResourceHandle handle, const char* debugName) const
		{
			if (auto res = GetResource(handle); res != InvalidHandle)
				renderSystem().SetDebugName(res, debugName);
		}


		void SetDebugName(ResourceHandle handle, const char* debugName) const
		{
			if (handle != InvalidHandle)
				renderSystem().SetDebugName(handle, debugName);
		}


		auto ReserveCB(const size_t size) const
		{
			return globalResources.ReserveCB(size);
		}


		auto ReserveVB(const size_t size) const
		{
			return globalResources.ReserveVB(size);
		}

		private:

		LocallyTrackedResource& _FindSubNodeResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](auto& res)
				{
					return res.resource == handle;
				});

#if _DEBUG
			if (res == SubNodeTracking.end())
				FK_LOG_ERROR("Failed to find tracking for resource!");
#endif
			return *res;
		}

		LocallyTrackedObjectList&	SubNodeTracking;
		FrameResources&				globalResources;
	};

	/************************************************************************************************/


	class FrameGraphNode;

	struct FrameObjectLink
	{
		FrameObjectLink(
			FrameResourceHandle		IN_handle			= InvalidHandle,
			FrameGraphNodeHandle	IN_sourceObject		= InvalidHandle,
			DeviceAccessState		IN_neededAccess		= DeviceAccessState::DASUNKNOWN,
			DeviceLayout			IN_neededLayout		= DeviceLayout::Unknown) :
				source			{ IN_sourceObject },
				neededAccess	{ IN_neededAccess },
				neededLayout	{ IN_neededLayout },
				handle			{ IN_handle } {}

		FrameGraphNodeHandle	source;
		FrameResourceHandle		handle;
		DeviceAccessState		neededAccess;
		DeviceLayout			neededLayout;
	};

	struct InputObject
	{
		FrameGraphNodeHandle	source;
		FrameResourceHandle		handle;

		DeviceAccessState		initialAccess;
		DeviceLayout			initialLayout;
	};

	inline auto MakePred(FrameResourceHandle handle, const PassObjectList& resources)
	{
		return [handle](FrameObjectLink& lhs)
		{
			return lhs.handle == handle;
		};
	}


	inline auto MakePred(ResourceHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return resources[lhs.handle].shaderResource == handle;
		};
	}


	inline auto MakePred(SOResourceHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return (resources[lhs.handle].SOBuffer == handle && resources[lhs.handle].type == OT_StreamOut);
		};
	}


	/************************************************************************************************/


	struct ResourceAcquisition
	{
		ResourceHandle resource;
		ResourceHandle overlap;
	};


	struct ResusableResourceQuery
	{
		FrameResourceHandle	object = InvalidHandle;
		bool				success = false;
	};

	struct FrameGraphNodeWorkItem
	{
		using FN_NodeAction = TypeErasedCallable<void (FrameGraphNode& node, FrameResources& Resources, IDirectContext& ctx, iAllocator& tempAllocator), 64>;

		size_t			workWeight		= 0;
		size_t			submissionID	= -1;
		FN_NodeAction	action;
		FrameGraphNode* node;

		void operator () (IDirectContext& ctx, FrameResources& resources, iAllocator& allocator)
		{
			action(*node, resources, ctx, allocator);
		}

	};

	class FrameGraphNode
	{
	public:
		typedef void (*FN_NodeGetWorkItems)	(FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& OUT_workItem, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator);

		FrameGraphNode(FrameGraphNodeHandle handle, FN_NodeGetWorkItems IN_action, void* IN_nodeData, iAllocator* IN_allocator = nullptr);

		FrameGraphNode(FrameGraphNode&& RHS);
		FrameGraphNode(const FrameGraphNode& RHS) = delete;

		~FrameGraphNode() = default;

		void HandleBarriers	(FrameResources& Resouces, IDirectContext& Ctx);
		void AddBarrier		(const Barrier& Dep);

		void RestoreResourceStates	(IDirectContext& ctx, FrameResources& resources, LocallyTrackedObjectList& locallyTrackedObjects);
		void AcquireResources		(FrameResources& resources, IDirectContext& ctx);
		void ReleaseResources		(FrameResources& resources, IDirectContext& ctx);

		ResusableResourceQuery			FindReuseableResource(PoolAllocatorInterface& allocator, size_t allocationSize, uint32_t flags, FrameResources& handler, std::span<FrameGraphNode> nodes);
		std::optional<InputObject>		GetInputObject(FrameResourceHandle);


		FrameGraphNodeHandle			handle;
		void*							nodeData;
		uint32_t						submissionID = -1;
		bool							executed = false;
		FN_NodeGetWorkItems				nodeAction;
		Vector<FrameGraphNodeHandle>	sources;// Nodes that this node reads from
		Vector<InputObject>				inputObjects;
		Vector<FrameObjectLink>			outputObjects;
		Vector<Barrier>					barriers;

		Vector<ResourceAcquisition>		acquiredObjects;
		Vector<FrameObjectLink>			retiredObjects;
		Vector<FrameObjectLink>			createdObjects;
		Vector<UpdateTask*>				dataDependencies;

		LocallyTrackedObjectList		subNodeTracking;
	};


	/************************************************************************************************/


	struct PendingAcquire
	{
		ResourceHandle			resourecHandle;
		uint64_t				offset;
		uint64_t				size;

		const GPUResourceDesc	desc;
	};


	class FrameGraphResourceContext
	{
	public:
		FrameGraphResourceContext(FrameResources& IN_resources, ThreadManager& IN_threads, IRenderSystem& IN_renderSystem, iAllocator& Temp) :
			frameResources	{ IN_resources		},
			writables		{ Temp				},
			readables		{ Temp				},
			retirees		{ Temp				},
			threads			{ IN_threads		},
			pendingTasks	{ IN_threads		},
			usedResources	{ Temp				},
			renderSystem	{ IN_renderSystem	},
			tempAllocator	{ Temp				} {}


		void AddWriteable(const FrameObjectLink& NewObject)
		{
			writables.push_back(NewObject);
			usedResources.push_back(NewObject.handle);
		}


		void AddReadable(const FrameObjectLink& NewObject)
		{
			readables.push_back(NewObject);
			usedResources.push_back(NewObject.handle);
		}


		template<typename TY>
		void RemoveWriteable(TY handle)
		{
			writables.remove_unstable(find(writables, MakePred(handle, frameResources.objects)));
		}


		template<typename TY>
		void RemoveReadable(TY handle)
		{
			readables.remove_unstable(find(readables, MakePred(handle, frameResources.objects)));
		}


		template<typename TY>
		FrameObjectLink& GetReadable(TY handle)
		{
			return *find(readables, MakePred(handle, frameResources.objects));
		}


		template<typename TY>
		FrameObjectLink& GetWriteable(TY handle)
		{
			return *find(writables, MakePred(handle, frameResources.objects));
		}


		Vector<FrameObjectLink> GetFinalStates()
		{
			Vector<FrameObjectLink> Objects(writables.Allocator);
			Objects.reserve(writables.size() + readables.size() + retirees.size());

			Objects += writables;
			Objects += readables;
			Objects += retirees;

			return std::move(Objects);
		}


		template<typename TY>
		FrameResourceHandle GetFrameObject(TY handle)
		{
			if (auto res = IsTrackedReadable(handle); res)
				return res.V2.handle;
			else if (auto res = IsTrackedWriteable(handle); res)
				return res.V2.handle;
			else
				return frameResources.FindFrameResource(handle);;
		}


		void AddDeferredCreation(ResourceHandle resource, size_t offset, DeviceHeapHandle heap, const GPUResourceDesc& desc)
		{
			auto& workItem = CreateWorkItem(
				[=, &renderSystem = renderSystem, desc = desc](auto& allocator) mutable
				{
					desc.bufferCount			= 1;
					desc.allocationType			= ResourceAllocationType::Placed;
					desc.placed.heap			= heap;
					desc.placed.offset			= offset;
					desc.placed.initialState	= DASNOACCESS;

					renderSystem.BackResource(resource, desc);
					renderSystem.SetDebugName(resource, "Deferred Resource");
					
					FK_LOG_9("Allocated Resource: %u", renderSystem.GetDeviceResource(resource));
				});

			pendingTasks.AddWork(workItem);
			threads.AddWork(&workItem);
		}


		void AddResourceTask(iWork& workItem)
		{
			pendingTasks.AddWork(workItem);
			threads.AddWork(&workItem);
		}

		void WaitFor()
		{
			pendingTasks.JoinLocal();
		}


		template<typename _pred>
		Pair<bool, FrameObjectLink&> _IsTrackedReadable(_pred pred)
		{
			auto Res = find(readables, pred);
			return { Res != readables.end(), *Res };
		}


		template<typename _pred>
		Pair<bool, FrameObjectLink&> _IsTrackedWriteable(_pred pred)
		{
			auto Res = find(writables, pred);
			return { Res != writables.end(), *Res };
		}


		template<typename TY>
		Pair<bool, FrameObjectLink&> IsTrackedReadable	(TY handle)
		{
			return _IsTrackedReadable(MakePred(handle, frameResources.objects));
		}


		template<typename TY>
		Pair<bool, FrameObjectLink&> IsTrackedWriteable	(TY handle)
		{
			return _IsTrackedWriteable(MakePred(handle, frameResources.objects));
		}


		void SetLastUsed(uint32_t ID)
		{
			std::ranges::sort(usedResources);
			usedResources.resize(usedResources.size() - std::ranges::unique(usedResources).size());

			for (auto resource : usedResources)
				frameResources.GetResourceObject(resource)->lastSubmission = ID;
		}

		void Reset()
		{
			writables.clear();
			readables.clear();
			retirees.clear();
			usedResources.clear();
		}

		Vector<FrameObjectLink>		writables;
		Vector<FrameObjectLink>		readables;
		Vector<FrameObjectLink>		retirees;
		Vector<FrameResourceHandle>	usedResources;

		iAllocator&				tempAllocator;
		ThreadManager&			threads;
		WorkBarrier				pendingTasks;
		FrameResources&			frameResources;
		IRenderSystem&			renderSystem;
	};


	/************************************************************************************************/


	enum class VirtualResourceScope
	{
		Temporary,
		Frame,
	};

	DeviceLayout	GuessLayoutFromAccess(DeviceAccessState access);
	uint32_t		GetNeededFlags(const GPUResourceDesc& desc);

	class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			std::span<FrameGraphNode>	IN_nodeTable,
			FrameResources*				IN_Resources, 
			FrameGraphNode&				IN_Node,
			FrameGraphResourceContext&	IN_context,
			iAllocator*					IN_allocator);


		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;

		void BuildNode(FrameGraph* FrameGraph);
		void BuildPass(FrameGraph* FrameGraph, FrameGraphNode& endNode);

		void AddDataDependency(UpdateTask& task);
		void AddNodeDependency(FrameGraphNodeHandle node);


		FrameGraphNodeHandle	GetNodeHandle() const;

		FrameResourceHandle GetHandle(ResourceHandle) const;

		FrameResourceHandle CreateConstantBuffer();
		FrameResourceHandle ReadConstantBuffer(FrameResourceHandle);

		FrameResourceHandle AccelerationStructureRead(ResourceHandle);
		FrameResourceHandle AccelerationStructureWrite(ResourceHandle);

		FrameResourceHandle Common					(ResourceHandle);

		FrameResourceHandle PixelShaderResource		(ResourceHandle);

		FrameResourceHandle NonPixelShaderResource	(ResourceHandle);
		FrameResourceHandle NonPixelShaderResource	(FrameResourceHandle);

		FrameResourceHandle CopyDest			(ResourceHandle);
		FrameResourceHandle CopySource			(ResourceHandle);

		FrameResourceHandle RenderTarget		(ResourceHandle);
		FrameResourceHandle RenderTarget		(FrameResourceHandle);

		FrameResourceHandle	Present				(ResourceHandle);

		FrameResourceHandle	DepthRead			(ResourceHandle);
		FrameResourceHandle	DepthTarget			(ResourceHandle, DeviceAccessState finalState = DeviceAccessState::DASDEPTHBUFFERWRITE);

		FrameResourceHandle	AcquireResourceHandle(DeviceAccessState, DeviceLayout, PoolAllocatorInterface* = nullptr);
		FrameResourceHandle	AcquireVirtualResource(const GPUResourceDesc& desc, DeviceAccessState, VirtualResourceScope lifeSpan = VirtualResourceScope::Temporary);
		FrameResourceHandle	AcquireVirtualResource(const GPUResourceDesc& desc, DeviceAccessState, PoolAllocatorInterface*, VirtualResourceScope lifeSpan = VirtualResourceScope::Temporary);
		FrameResourceHandle	AcquireVirtualResource(PoolAllocatorInterface& allocator, const GPUResourceDesc& desc, DeviceAccessState, VirtualResourceScope lifeSpan = VirtualResourceScope::Temporary);

		void				ReleaseVirtualResource(FrameResourceHandle handle);


		FrameResourceHandle	UnorderedAccess (ResourceHandle, DeviceAccessState state = DeviceAccessState::DASUAV);

		FrameResourceHandle	VertexBuffer	(SOResourceHandle);
		FrameResourceHandle	StreamOut		(SOResourceHandle);

		FrameResourceHandle ReadTransition	(FrameResourceHandle handle, DeviceAccessState state, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All });
		FrameResourceHandle WriteTransition	(FrameResourceHandle handle, DeviceAccessState state, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All });

		FrameResourceHandle	ReadBack(ReadBackResourceHandle);

		auto ReserveCB(const size_t size) { return GetResources().ReserveCB(size); }
		auto ReserveVB(const size_t size) { return GetResources().ReserveVB(size); }

		void SetDebugName(FrameResourceHandle handle, const char* debugName);

		const DesciptorHeapLayout&	GetDescriptorTableLayout		(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot

		IRenderSystem&	GetRenderSystem();
		FrameResources& GetResources() { return *resources; }

		operator FrameResources&	() const;
		operator IRenderSystem&		();

		void Requires(PSOHandle handle);

	private:

		template<typename TY>
		FrameResourceHandle AddReadableResource(TY handle, DeviceAccessState access, DeviceLayout layout, std::optional<std::pair<DeviceAccessState, DeviceLayout>> finalTransition = {}, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All })
		{
			FrameResourceHandle frameResourceHandle = context.GetFrameObject(handle);

			if (frameResourceHandle == InvalidHandle)
				return frameResourceHandle;

			FrameObject& frameObject = context.frameResources.objects[frameResourceHandle];

			for(auto& writer : frameObject.lastUsers)
				inputNodes.push_back(writer);

			if (layout == DeviceLayout::Unknown)
				layout = frameObject.layout;

			InputObject inputObject{ node.handle, frameResourceHandle, frameObject.access, frameObject.layout };
			node.inputObjects.push_back(inputObject);

			if (frameObject.layout != layout || !CheckCompatibleAccessState(frameObject.layout, frameObject.access))
			{
				frameObject.lastUsers.clear();

				if (IsWriteAccessState(frameObject.access))
					context.RemoveWriteable(frameResourceHandle);

				FrameObjectLink dependency{ frameResourceHandle, node.handle, access, layout };
				context.AddReadable(dependency);

				Barrier barrier;
				barrier.accessBefore	= frameObject.access;
				barrier.accessAfter		= access;
				barrier.src				= std::get<0>(syncPoints);
				barrier.dst				= std::get<1>(syncPoints);

				switch (frameObject.dimensions)
				{
				case TextureDimension::Buffer:
					barrier.type					= BarrierType::Buffer;
					barrier.resource				= frameObject.shaderResource;
					break;
				default:
					barrier.type					= BarrierType::Texture;
					barrier.resource				= frameObject.shaderResource;
					barrier.texture.layoutBefore	= frameObject.layout;
					barrier.texture.layoutAfter		= layout;
					break;
				}


				barriers.push_back(barrier);

				frameObject.access	= access;
				frameObject.layout	= layout;
			}

			frameObject.lastUsers.push_back(node.handle);

			const auto p = std::make_pair(access, layout);
			node.subNodeTracking.push_back({ frameResourceHandle, frameObject.access, frameObject.layout, std::get<0>(finalTransition.value_or(p)), std::get<1>(finalTransition.value_or(p)) });

			return frameResourceHandle;
		}


		template<typename TY>
		FrameResourceHandle AddWriteableResource(TY handle, DeviceAccessState access, DeviceLayout layout, std::optional<std::pair<DeviceAccessState, DeviceLayout>> finalTransition = {}, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All })
		{
			FrameResourceHandle frameResourceHandle = context.GetFrameObject(handle);

			if (frameResourceHandle == InvalidHandle)
				return InvalidHandle;

			FrameObject& frameObject = context.frameResources.objects[frameResourceHandle];

			for (auto& user : frameObject.lastUsers)
				inputNodes.push_back(user);

			if (layout == DeviceLayout::Unknown)
				layout = frameObject.layout;

			if (!CheckCompatibleLayout(frameObject.layout, layout) || !CheckCompatibleAccessState(frameObject.layout, frameObject.access))
			{
				frameObject.lastUsers.clear();

				if (IsReadAccessState(frameObject.access) && !(finalTransition.has_value() && IsReadAccessState(finalTransition.value().first)))
				{
					context.RemoveReadable(frameResourceHandle);

					FrameObjectLink dependency{ frameResourceHandle, node.handle, access, layout };
					context.AddWriteable(dependency);
				}

				Barrier barrier;
				barrier.accessBefore	= frameObject.access;
				barrier.accessAfter		= access;
				barrier.src				= std::get<0>(syncPoints);
				barrier.dst				= std::get<1>(syncPoints);

				switch (frameObject.dimensions)
				{
				case TextureDimension::Buffer:
					barrier.type					= BarrierType::Buffer;
					barrier.resource				= frameObject.shaderResource;
					break;
				default:
					barrier.type					= BarrierType::Texture;
					barrier.resource				= frameObject.shaderResource;
					barrier.texture.layoutBefore	= frameObject.layout;
					barrier.texture.layoutAfter		= layout;
					break;
				}

				barriers.push_back(barrier);

				frameObject.access	= access;
				frameObject.layout	= layout;
			}

			frameObject.lastUsers.push_back(node.handle);

			const auto p = std::make_pair(access, layout);
			node.subNodeTracking.push_back({ frameResourceHandle, frameObject.access, frameObject.layout, std::get<0>(finalTransition.value_or(p)), std::get<1>(finalTransition.value_or(p)) });

			return frameResourceHandle;
		}

		enum class CheckStateRes
		{
			TransitionNeeded,
			CorrectState,
			ResourceNotTracked,
			Error,
		};


		static CheckStateRes CheckResourceSituation(
			Vector<FrameObjectLink>&	Set1,
			Vector<FrameObjectLink>&	Set2,
			FrameObjectLink&			Object);

		std::span<FrameGraphNode>		nodeTable;

		Vector<FrameGraphNodeHandle>	inputNodes;
		Vector<Barrier>					barriers;
		Vector<FrameObjectLink>			retiredObjects;
		Vector<FrameObjectLink>			temporaryObjects;
		Vector<ResourceAcquisition>		acquiredResources;

		FrameGraphResourceContext&		context;
		FrameGraphNode&					node;
		FrameResources*					resources;
		iAllocator*						allocator;
	};


	/************************************************************************************************/


	using DataDependencyList = Vector<UpdateTask*>;
	struct PassPVS;

	template<typename Shared_TY, typename TY = const BrushEntry>
	struct PassDescription
	{
		Shared_TY							sharedData;
		TypeErasedCallable<std::span<TY>()>	getPVS;

		using Shared		= Shared_TY;
		using GetPass_TY	= TypeErasedCallable<std::span<TY> ()>;
	};

	template<typename Shared_TY, typename TY_PassData, typename TY_PVSElements = const BrushEntry>
	struct DataDrivenMultiPassDescription
	{
		Shared_TY														sharedData;
		TypeErasedCallable<std::span<TY_PVSElements> (TY_PassData&)>	getPVS;
		TypeErasedCallable<Vector<TY_PassData> (iAllocator&)>			getPasses;

		using Shared			= Shared_TY;
		using GetPassPVS_TY		= TypeErasedCallable<std::span<TY_PVSElements> (TY_PassData&)>;
		using GetPassData_TY	= TypeErasedCallable<Vector<TY_PassData> (iAllocator&)>;
	};


	struct ResourceAllocation
	{
		Vector<FrameResourceHandle> handles;
		FrameGraphNodeHandle		node;

		operator FrameGraphNodeHandle() const noexcept { return node; }

		FrameResourceHandle operator [](size_t idx) const noexcept { return handles[idx]; }

		auto begin()	const noexcept { return handles.begin(); }
		auto end()		const noexcept { return handles.end(); }
	};

	template<typename FillData_TY, typename GetPass_TY>
	struct PassDrivenResourceAllocation
	{
		GetPass_TY				getPass;				// std::span<ty> ()
		FillData_TY				initializeResources;	// void (std::span<ty>, std::span<FrameResourceHandle>, ResourceInitializationContext& transferCtx, iAllocator&);

		DeviceLayout			layout;
		DeviceAccessState		access;
		size_t					max			= 16;
		PoolAllocatorInterface* pool		= nullptr;
		UpdateTask*				dependency	= nullptr;
	};


	class FrameGraph
	{
	public:
		FrameGraph(IRenderSystem& RS, ThreadManager& IN_threads, iAllocator& Temp) :
			resources			{ RS, Temp },
			threads				{ IN_threads },
			globalDependencies	{ Temp },
			computeStateContext	{ resources, IN_threads, RS, Temp },
			directStateContext	{ resources, IN_threads, RS, Temp },
			memory				{ Temp },
			nodes				{ Temp },
			pendingDirectNodes	{ Temp },
			pendingComputeNodes	{ Temp },
			submissions			{ Temp },
			acquiredResources	{ Temp },
			pendingAcquire		{ Temp }
		{
			nodes.reserve(64);
		}

		FrameGraph				(const FrameGraph& RHS) = delete;
		FrameGraph& operator =	(const FrameGraph& RHS) = delete;


		template<typename TY, typename SetupFN, typename DrawFN>
		TY& AddNode(TY&& initial, SetupFN&& setup, DrawFN&& draw)
		{
			struct NodeData
			{
				NodeData(TY&& IN_initial, DrawFN&& IN_drawFN) :
					draw	{ std::move(IN_drawFN)  },
					fields	{ std::move(IN_initial) } {}

				~NodeData() = default;

				TY		fields;
				DrawFN	draw;
			};

			auto& data	= memory->allocate_aligned<NodeData>(std::move(std::forward<TY>(initial)), std::move(draw));

			auto idx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					FrameGraphNodeWorkItem newWorkItem;
					newWorkItem.node			= &node;
					newWorkItem.submissionID	= node.submissionID;

					newWorkItem.action	=
						[](	FrameGraphNode&	node,
							FrameResources&	resources,
							IDirectContext&	ctx,
							iAllocator&		tempAllocator)
						{
							ProfileFunction();

							NodeData& data = *reinterpret_cast<NodeData*>(node.nodeData);

							node.HandleBarriers(resources, ctx);

							LocallyTrackedObjectList localTracking{ &tempAllocator };
							localTracking = node.subNodeTracking;

							ResourceHandler handler{ resources, localTracking };
							data.draw(data.fields, handler, ctx, tempAllocator);

							node.RestoreResourceStates(ctx, resources, localTracking);

							{
								ProfileFunctionLabeled(Destruction);
								data.fields.~TY();
							}
						};

					tasks_out.push_back(newWorkItem);
				},
				&data,
				memory);

			FrameGraphNodeBuilder builder(nodes, &resources, nodes[idx], directStateContext, memory);

			setup(builder, data.fields);
			builder.BuildNode(this);

			pendingDirectNodes.push_back(&nodes[idx]);

			return data.fields;
		}

		template<typename TY_Shared, typename TY_Setup, typename TY_Creation, typename TY_Task>
		TY_Shared& BuildSharedConstants(TY_Setup&& setupLinkage, TY_Creation&& resourceCreation, TY_Task&& threadedTask)
		{
			using UserData_t = decltype(setupLinkage(std::declval<FrameGraphNodeBuilder&>()));

			struct NodeData
			{
				UserData_t	userData;
				TY_Creation	creation;
				TY_Task		task;
			};

			auto nodeTask =
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& out, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					NodeData* data = reinterpret_cast<NodeData*>(node.nodeData);
					auto& threadedTask = CreateWorkItem(
						[data](auto& tempAllocator)
						{
							data->task(data->userData, tempAllocator);
						}, tempAllocator);

					data->creation(data->userData, resources, tempAllocator);

					barrier.AddWork(threadedTask);
					PushToLocalQueue(threadedTask);
				};

			auto idx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				nodeTask,
				nullptr,
				memory);

			FrameGraphNodeBuilder builder(nodes, &resources, nodes[idx], directStateContext, memory);

			auto& data = memory->allocate_aligned<NodeData>(setupLinkage(builder), std::move(resourceCreation), std::move(threadedTask));
			nodes[idx].nodeData = &data;

			builder.BuildNode(this);

			pendingDirectNodes.push_back(&nodes[idx]);

			return data.userData;
		}


		template<typename TY_Shared, typename TY, typename TY_Setup, typename TY_Draw>
		decltype(auto) AddPass(PassDescription<TY_Shared, TY>& IN_shared, TY_Setup setupLinkage, TY_Draw draw)
		{
			using GetPass_TY = PassDescription<TY_Shared, TY>::GetPass_TY;

			struct PassData
			{
				TY_Shared			shared;
				TY_Draw				draw;
				GetPass_TY			getPVS;
				std::atomic_uint	refCount = 1;
			} &start = memory->allocate_aligned<PassData>(
									std::move(std::forward<TY_Shared>(IN_shared.sharedData)),
									std::move(draw),
									std::move(IN_shared.getPVS));

			auto startNodeIdx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					PassData* passData = reinterpret_cast<PassData*>(node.nodeData);

					const auto& passPVS	= passData->getPVS();
					const auto size		= passPVS.size();

					if(size > 0)
					{ 
						uint16_t blockCount = (uint16_t)(size / 1000 + (size % 1000 > 0 ? 1 : 0));
						uint16_t blockSize	= (uint16_t)(size / blockCount);

						passData->refCount = blockCount;

						for (uint16_t i = 0; i < blockCount; i++)
						{
							auto begin	= Min((i + 0) * blockSize, size);
							auto end	= Min((i + 1) * blockSize, size);

							FrameGraphNodeWorkItem newWorkItem;
							newWorkItem.node			= &node;
							newWorkItem.workWeight		= end - begin;
							newWorkItem.submissionID	= node.submissionID;

							newWorkItem.action	=
								[i, blockCount, blockSize, passPVS](
									FrameGraphNode&	node,
									FrameResources&	resources,
									IDirectContext&	ctx,
									iAllocator&		localAllocator)
								{
									ProfileFunction();

									PassData& data = *reinterpret_cast<PassData*>(node.nodeData);

									auto begin	= passPVS.begin() + Min((i + 0) * blockSize, passPVS.size());
									auto end	= passPVS.begin() + Min((i + 1) * blockSize, passPVS.size());

									if(i == 0)
										node.HandleBarriers(resources, ctx);

									LocallyTrackedObjectList localTracking{ &localAllocator };
									localTracking = node.subNodeTracking;

									ResourceHandler handler{ resources, localTracking };
									data.draw(begin, end, passPVS, data.shared, resources, ctx, localAllocator);

									if(i == blockCount - 1)
										node.RestoreResourceStates(ctx, resources, localTracking);

									auto refCount = data.refCount--;

									if(refCount == 1)
									{
										ProfileFunctionLabeled(Destruction);
										data.~PassData();
									}
								};

							tasks_out.emplace_back(std::move(newWorkItem));
						}
					}
					else
					{
						tasks_out.emplace_back(
							FrameGraphNodeWorkItem{
								.workWeight		= 1,
								.submissionID	= node.submissionID,
								.action			=
									[](
										FrameGraphNode& node,
										FrameResources& resources,
										IDirectContext&	ctx,
										iAllocator&		localAllocator)
									{
										ProfileFunction();

										PassData& data = *reinterpret_cast<PassData*>(node.nodeData);
										const auto& pvs = data.getPVS();

										node.HandleBarriers(resources, ctx);

										LocallyTrackedObjectList localTracking{ &localAllocator };
										localTracking = node.subNodeTracking;

										ResourceHandler handler{ resources, localTracking };
										data.draw(pvs.begin(), pvs.end(), pvs, data.shared, resources, ctx, localAllocator);

										node.RestoreResourceStates(ctx, resources, localTracking);

										ProfileFunctionLabeled(Destruction);
										data.~PassData();
									},
								.node = &node,
							});
					}
				},
				&start,
				memory);


			FrameGraphNodeBuilder builder(nodes, &resources, nodes[startNodeIdx], directStateContext, memory);
			setupLinkage(builder, start.shared);
			builder.BuildNode(this);

			pendingDirectNodes.push_back(&nodes[startNodeIdx]);

			return start;
		}

		template<typename TY_Shared, typename TY_Pass, typename TY_PassElements, typename TY_Setup, typename TY_Draw>
		decltype(auto) AddDataDrivenMultiPass(DataDrivenMultiPassDescription<TY_Shared, TY_Pass, TY_PassElements>& IN_shared, TY_Setup setupLinkage, TY_Draw draw)
		{
			using GetPassPVS_TY		= std::decay_t<decltype(IN_shared)>::GetPassPVS_TY;
			using GetPassData_TY	= std::decay_t<decltype(IN_shared)>::GetPassData_TY;

			struct PassData
			{
				TY_Shared			shared;
				TY_Draw				draw;
				GetPassPVS_TY		getPVS;
				GetPassData_TY		getPasses;
				Vector<TY_Pass>		passes;
				std::atomic_uint	refCount = 0;
			} &start = memory->allocate_aligned<PassData>(
									std::move(std::forward<TY_Shared>(IN_shared.sharedData)),
									std::move(draw),
									std::move(IN_shared.getPVS),
									std::move(IN_shared.getPasses));

			const auto startNodeIdx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					PassData* passData	= reinterpret_cast<PassData*>(node.nodeData);
					passData->passes	= passData->getPasses(tempAllocator);

					for(auto& pass : passData->passes)
					{
						auto passPVS		= passData->getPVS(pass);
						auto size			= passPVS.size();

						uint16_t blockCount = (uint16_t)(size / 1000 + (size % 1000 > 0 ? 1 : 0));
						uint16_t blockSize	= blockCount > 0 ? (uint16_t)(size / blockCount) : 1;

						passData->refCount = blockCount;

						for (uint16_t i = 0; i < blockCount; i++)
						{
							auto begin	= Min((i + 0) * blockSize, size);
							auto end	= Min((i + 1) * blockSize, size);

							FrameGraphNodeWorkItem newWorkItem;
							newWorkItem.node			= &node;
							newWorkItem.workWeight		= end - begin;
							newWorkItem.submissionID	= node.submissionID;

							newWorkItem.action	=
								[i, blockCount, blockSize, passPVS, &pass](
									FrameGraphNode&	node,
									FrameResources&	resources,
									IDirectContext&	ctx,
									iAllocator&		localAllocator)
								{
									ProfileFunction();

									PassData& data = *reinterpret_cast<PassData*>(node.nodeData);

									auto begin	= passPVS.begin() + Min((i + 0) * blockSize, passPVS.size());
									auto end	= passPVS.begin() + Min((i + 1) * blockSize, passPVS.size());

									if (i == 0)
									{
										node.HandleBarriers(resources, ctx);
										ctx.BeginEvent_DEBUG("Multi-Pass Begin");
									}
									LocallyTrackedObjectList localTracking{ &localAllocator };
									localTracking = node.subNodeTracking;

									ResourceHandler handler{ resources, localTracking };
									data.draw(begin, end, passPVS, pass, data.shared, resources, ctx, localAllocator);

									if (i == blockCount - 1)
									{
										node.RestoreResourceStates(ctx, resources, localTracking);
										ctx.EndEvent_DEBUG();
									}

									auto refCount = data.refCount--;

									if(refCount == 1)
									{
										ProfileFunctionLabeled(Destruction);
										data.~PassData();
									}
								};

							tasks_out.emplace_back(std::move(newWorkItem));
						}
					}
				},
				&start,
				memory);


			FrameGraphNodeBuilder builder(nodes, &resources, nodes[startNodeIdx], directStateContext, memory);
			setupLinkage(builder, start.shared);
			builder.BuildNode(this);

			pendingDirectNodes.push_back(&nodes[startNodeIdx]);

			return start;
		}

		template<typename DESC_TY>
		const ResourceAllocation& AllocateResourceSet(DESC_TY& desc)
		{
			struct InitialData
			{
				void*					_ptr;
				size_t					size;
				FrameResourceHandle		handle;
			};

			struct NodeData
			{
				decltype(desc.getPass)				getPass;
				decltype(desc.initializeResources)	initializeResources;

				ResourceAllocation		resources;
				Vector<InitialData>		pendingCopies;
				size_t					uploadSize = 0;
				PoolAllocatorInterface* pool;

				DeviceAccessState			access;
				FrameGraphResourceContext&	resourceContext;
				iAllocator*					allocator;
				UploadReservation			uploadReservation;

				struct BLSABuild
				{
					ResourceHandle					resource;
					typename TriMesh::LOD_Runtime*	source;
				};

				size_t					scratchPadSize = 0;
				FrameResourceHandle		scratchPad = InvalidHandle;
				Vector<BLSABuild>		BVHBuilds;
			};

			auto nodeHandle	= FrameGraphNodeHandle{ (uint32_t)nodes.size() }; 
			NodeData& data	= memory->allocate_aligned<NodeData>(
								NodeData{
									.getPass				= desc.getPass,
									.initializeResources	= desc.initializeResources,
									.resources				= ResourceAllocation{ memory, nodeHandle },
									.pendingCopies			= Vector<InitialData>{ memory },
									.uploadSize				= 0,
									.pool					= desc.pool,
									.access					= desc.access,
									.resourceContext		= directStateContext,
									.allocator				= memory,
									.scratchPadSize			= 0,
									.scratchPad				= InvalidHandle,
									.BVHBuilds{	memory },
								});

			data.pendingCopies.reserve(desc.max);

			struct ResourceInitializationContext
			{
				FrameResources& frameResources;
				NodeData*		nodeData;

				AcquireResult CreateResource(FrameResourceHandle dstResource, size_t resourceSize, void* initialData)
				{
					nodeData->pendingCopies.emplace_back(initialData, resourceSize, dstResource);

					auto GetMemoryPool = [&]
					{
						if (!nodeData->pool)
						{
							const auto& desc	= GPUResourceDesc::StructuredResource((uint32_t)resourceSize);
							const auto	flags	= GetNeededFlags(desc);
							return frameResources.FindMemoryPool(flags);
						}
						else
							return nodeData->pool;
					};

					PoolAllocatorInterface* pool = GetMemoryPool();
					if (!pool)
						return { InvalidHandle };

					std::atomic_ref arc_ref{ frameResources.virtualResourceCount };
					arc_ref++;

					auto [resourceHandle, overlap] = pool->Acquire(GPUResourceDesc::StructuredResource((uint32_t)resourceSize));

					auto frameObject				= frameResources.GetResourceObject(dstResource);
					frameObject->shaderResource		= resourceHandle;
					frameObject->pool				= pool;
					frameObject->virtualState		= VirtualResourceState::Virtual_Created;

					nodeData->uploadSize += resourceSize;

					return { resourceHandle, overlap };
				}

				AcquireResult CreateResource(FrameResourceHandle dstResource, std::span<const char> byteView)
				{
					nodeData->pendingCopies.emplace_back((void*)byteView.data(), byteView.size_bytes(), dstResource);

					auto GetMemoryPool = [&]
					{
						if (!nodeData->pool)
						{
							const auto& desc	= GPUResourceDesc::StructuredResource((uint32_t)byteView.size_bytes());
							const auto	flags	= GetNeededFlags(desc);
							return frameResources.FindMemoryPool(flags);
						}
						else
							return nodeData->pool;
					};

					PoolAllocatorInterface* pool = GetMemoryPool();
					if (!pool)
						return { InvalidHandle };

					auto [resourceHandle, overlap] = pool->Acquire(GPUResourceDesc::StructuredResource((uint32_t)byteView.size_bytes()));

					std::atomic_ref arc_ref{ frameResources.virtualResourceCount };
					arc_ref++;

					auto frameObject				= frameResources.GetResourceObject(dstResource);
					frameObject->shaderResource		= resourceHandle;
					frameObject->pool				= pool;
					frameObject->virtualState		= VirtualResourceState::Virtual_Created;

					nodeData->uploadSize += byteView.size_bytes();

					return { resourceHandle, overlap };
				}

				AcquireResult AcquireTemporary(FrameResourceHandle dstResource, const GPUResourceDesc& desc)
				{
					auto GetMemoryPool = [&]
					{
						if (!nodeData->pool)
						{
							const auto	flags = GetNeededFlags(desc);
							return frameResources.FindMemoryPool(flags);
						}
						else
							return nodeData->pool;
					};

					PoolAllocatorInterface* pool = GetMemoryPool();
					if (!pool)
						return { FlexKit::InvalidHandle };

					std::atomic_ref arc_ref{ frameResources.virtualResourceCount };
					arc_ref++;

					auto [resourceHandle, overlap] = pool->Acquire(desc);

					auto frameObject				= frameResources.GetResourceObject(dstResource);
					frameObject->shaderResource		= resourceHandle;
					frameObject->pool				= pool;
					frameObject->virtualState		= VirtualResourceState::Virtual_Created;
					frameObject->dimensions			= desc.Dimensions;

					return { resourceHandle, overlap };
				}

				AcquireResult AllocateResource(FrameResourceHandle dstResource, const GPUResourceDesc& desc)
				{
					auto GetMemoryPool = [&]
					{
						if (!nodeData->pool)
						{
							const auto	flags = GetNeededFlags(desc);
							return frameResources.FindMemoryPool(flags);
						}
						else
							return nodeData->pool;
					};

					PoolAllocatorInterface* pool = GetMemoryPool();
					if (!pool)
						return { InvalidHandle };

					auto [resourceHandle, overlap] = pool->Acquire(desc);

					auto frameObject				= frameResources.GetResourceObject(dstResource);
					frameObject->shaderResource		= resourceHandle;
					frameObject->pool				= pool;
					frameObject->virtualState		= VirtualResourceState::Virtual_Persistent;

					return { resourceHandle, overlap };
				}

				void BuildBLAS(FrameResourceHandle resource, TriMesh::LOD_Runtime& src_lod)
				{
					const auto prebuildInfo	= frameResources.renderSystem->GetBLASPreBuildInfo(*src_lod.bufferSet);
					const auto desc			= GPUResourceDesc::RayTracingStructure(prebuildInfo.BLAS_byteSize);

					auto [handle, _] = AllocateResource(resource, desc);

					if (handle == InvalidHandle)
					{
						FK_LOG_ERROR("Failed to Allocate BLAS!");
						return;
					}

					nodeData->scratchPadSize = Max(nodeData->scratchPadSize, prebuildInfo.BLAS_byteSize);
					nodeData->BVHBuilds.emplace_back(handle, &src_lod);

					src_lod.blAS = GetResource(resource);
				}

				ResourceHandle	GetResource(FrameResourceHandle resource) const
				{
					return frameResources.GetResource(resource);
				}

				DevicePointer	GetDevicePointer(FrameResourceHandle frameResource) const
				{
                    return frameResources.renderSystem->GetDevicePointer(GetResource(frameResource));
				}

			};

			/*
			static_assert(
				requires(DESC_TY& getPass)
				{
					desc.initializeResources(
						std::span<const PVEntry>{},
						std::span<const FrameResourceHandle>{},
						std::declval<ResourceInitializationContext&>(),
						std::declval<iAllocator&>());
				}, "Invalid Resource Initializer!");
			*/

			auto nodeIdx	= nodes.emplace_back(
				nodeHandle,
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					NodeData* nodeData = reinterpret_cast<NodeData*>(node.nodeData);

					const auto	pass	= nodeData->getPass();

					if (!pass.size())
						return;

					auto& threadedTask = CreateWorkItem(
						[nodeData, &resources, pass](auto& tempAllocator)
						{
							ResourceInitializationContext transferCtx{ resources, nodeData };
							nodeData->initializeResources(pass, nodeData->resources.handles, transferCtx, *nodeData->allocator);

							if (nodeData->scratchPadSize != 0)
							{
								auto pool = resources.FindMemoryPool(GetNeededFlags(GPUResourceDesc::UAVResource(0)));
								if (pool)
								{
									std::atomic_ref{ resources.virtualResourceCount }++;

									auto resource = pool->Acquire(GPUResourceDesc::UAVResource(nodeData->scratchPadSize), true).resource;

									auto& object			= *resources.GetResourceObject(nodeData->scratchPad);
									object.shaderResource	= resource;
									object.pool				= pool;
									object.virtualState		= VirtualResourceState::Virtual_Created;
								}
							}

							auto uploadReservation = resources.renderSystem->ReserveDirectUploadSpace(nodeData->uploadSize, 256);
							size_t offset = 0;

							// Copy Data into resources, insert barrier/transition
							for (auto&& [_ptr, size, resource] : nodeData->pendingCopies)
							{
								memcpy(uploadReservation.buffer + offset, _ptr, size);
								offset += size;
							}

							nodeData->uploadReservation = uploadReservation;
						}, tempAllocator);

					nodeData->resourceContext.AddResourceTask(threadedTask);

					FrameGraphNodeWorkItem newWorkItem;
					newWorkItem.node			= &node;
					newWorkItem.workWeight		= pass.size();
					newWorkItem.submissionID	= node.submissionID;

					newWorkItem.action =
						[nodeData](
							FrameGraphNode& node,
							FrameResources& resources,
							IDirectContext&	ctx,
							iAllocator&		localAllocator)
						{
							ctx.BeginEvent_DEBUG("Initiate Resources");
							EXITSCOPE(ctx.EndEvent_DEBUG());

							auto uploadReservation = nodeData->uploadReservation;
							size_t offset = 0;

							if (nodeData->pendingCopies.size() == 0 && uploadReservation.size == 0 && nodeData->BVHBuilds.size() == 0)
								return;

							// Insert Barrier
							for (auto&& [_ptr, size, resource] : nodeData->pendingCopies)
								ctx.AddBufferBarrier(resources.GetResource(resource), DeviceAccessState::DASCommon, DeviceAccessState::DASCopyDest, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_Copy);

							// Copy Data into resources
							for (auto&& [_ptr, size, resource] : nodeData->pendingCopies)
							{
								ctx.CopyBufferRegion(
									resources.GetDeviceResource(resource), uploadReservation.resource,	// dst_res, src_res
									size,																// copy size
									0, uploadReservation.offset + offset);								// dst_offset, src offset

								offset += size;
							}

							if (nodeData->BVHBuilds.size())
							{
								auto scratchPad = resources.GetResource(nodeData->scratchPad);

								for (auto&& [resource, lod] : nodeData->BVHBuilds)
								{
									ctx.BuildBLAS(*lod->bufferSet, resource, scratchPad);
									ctx.AddUAVBarrier(scratchPad);
									ctx.AddBufferBarrier(resource, DASACCELERATIONSTRUCTURE_WRITE, nodeData->access, Sync_BuildRaytracingAccelerationStructure, Sync_All_Shading);
								}
							}

							for (auto&& [_ptr, size, resource] : nodeData->pendingCopies)
								ctx.AddBufferBarrier(resources.GetResource(resource), DeviceAccessState::DASCopyDest, nodeData->access, DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_All);

							//for (auto&& [resource, _] : nodeData->BVHBuilds)
							//	ctx.AddBufferBarrier(resource, DeviceAccessState::DASACCELERATIONSTRUCTURE_WRITE, nodeData->access, DeviceSyncPoint::Sync_BuildRaytracingAccellerationStructure, DeviceSyncPoint::Sync_All);
						};

					tasks_out.emplace_back(std::move(newWorkItem));
				},
				&data,
				memory);

			FrameGraphNodeBuilder builder(nodes, &resources, nodes[nodeIdx], directStateContext, memory);

			if (desc.dependency)
				builder.AddDataDependency(*desc.dependency);

			data.resources.handles.reserve(desc.max);

			for (size_t i = 0; i < desc.max; i++)
			{
				auto resource	= builder.AcquireResourceHandle(desc.access, desc.layout);

				data.resources.handles.push_back(resource);
			}

			data.scratchPad = builder.AcquireResourceHandle(DASUAV, DeviceLayout::UnorderedAccess);

			builder.BuildNode(this);

			pendingDirectNodes.push_back(&nodes[nodeIdx]);

			return data.resources;
		}

		void AddMemoryPool		(PoolAllocatorInterface& poolAllocator);
		void AddMemoryPool		(PoolAllocatorInterface* poolAllocator);
		void AddConstantBuffer	(ConstantBufferHandle);
		void AddVertexBuffer	(VertexBufferHandle);
		void AddTaskDependency	(UpdateTask& task);

		FrameResourceHandle		AddResource	(ResourceHandle resource);
		FrameResourceHandle		AddOutput	(ResourceHandle resource);

		uint32_t				SubmitDirect	(UpdateDispatcher& dispatcher, iAllocator* persistentAllocator);
		uint32_t				SubmitCompute	(UpdateDispatcher& dispatcher, iAllocator* persistentAllocator);

		void SyncDirectTo(uint32_t);

		UpdateTask&		Finish(UpdateDispatcher& dispatcher, iAllocator* persistentAllocator);
		IRenderSystem&	GetRenderSystem() noexcept { return *resources.renderSystem; }

		FrameResources				resources;
		Vector<ResourceHandle>		acquiredResources;
		Vector<PendingAcquire>		pendingAcquire;
		FrameGraphResourceContext	computeStateContext;
		FrameGraphResourceContext	directStateContext;
		iAllocator*					memory;
		Vector<FrameGraphNode>		nodes;
		ThreadManager&				threads;
		DataDependencyList			globalDependencies;


	private:

		void _FinalSubmit(iAllocator& allocator);

		struct Submission
		{
			Vector<FrameGraphNode*>	nodes;

			enum class Queue
			{
				Direct,
				Compute,
				Error
			}	queue = Queue::Error;

			static_vector<uint32_t, 8> syncs;
		};

		static_vector<uint32_t, 8>	directSyncs;
		static_vector<uint32_t, 8>	computeSyncs;

		Vector<FrameGraphNode*>		pendingDirectNodes;
		Vector<FrameGraphNode*>		pendingComputeNodes;
		Vector<Submission>			submissions;

		void ReleaseVirtualObjects(std::span<FrameGraphNode*> workList);
		void UpdateResourceFinalState();
	};


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.renderSystem.VertexBufferPush(Buffer, (void*)&Vertex, sizeof(TY_V));
		FK_ASSERT(res, "Failed to Push Vertex!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources, size_t PushSize)
	{
		bool res = Resources.renderSystem.VertexBufferPush(Buffer, (void*)&Vertex, PushSize);
		FK_ASSERT(res, "Failed to Push Vertex!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_FN_V>
	bool PushRectToVertexBuffer(TY_FN_V FN_PointConvert, VertexBufferHandle buffer, FrameResources& resources)
	{
		auto upperLeft		= FN_PointConvert(float2{ 0, 1 }, float2{0, 1});
		auto bottomRight	= FN_PointConvert(float2{ 1, 0 }, float2{1, 0});
		auto upperRight		= FN_PointConvert(float2{ 1, 1 }, float2{1, 1});
		auto bottomLeft		= FN_PointConvert(float2{ 0, 0 }, float2{0, 0});

		bool res = true;
		res |= PushVertex(upperLeft,	buffer, resources);
		res |= PushVertex(bottomRight,	buffer, resources);
		res |= PushVertex(bottomLeft,	buffer, resources);

		res |= PushVertex(upperLeft,	buffer, resources);
		res |= PushVertex(upperRight,	buffer, resources);
		res |= PushVertex(bottomRight,	buffer, resources);

		return res;
	}


	/************************************************************************************************/


	template<typename TY_V>
	inline size_t GetCurrentVBufferOffset(VertexBufferHandle Buffer, FrameResources& Resources)
	{
		return Resources.renderSystem->GetVertexBufferOffset(Buffer) / sizeof(TY_V);
	}


	inline size_t BeginNewConstantBuffer(ConstantBufferHandle CB, FrameResources& Resources)
	{
		return Resources.renderSystem->ConstantBufferAlign(CB);
	}

	/************************************************************************************************/


	struct Rectangle
	{
		float4 Color	= { 1.0f, 1.0f, 1.0f, 1.0f };
		float2 Position;
		float2 WH;

		static Rectangle FullScreenQuad()
		{
			return Rectangle{ 
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				{ 0.0f, 0.0f },
				{ 1.0f, 1.0f }
			};
		}
	};

	typedef Vector<Rectangle> RectangleList;


	/************************************************************************************************/


	void ClearBackBuffer	(FrameGraph& Graph, ResourceHandle backBuffer, float4 Color = {0.0f, 0.0f, 0.0f, 0.0f });// Clears BackBuffer to Black
	void ClearDepthBuffer	(FrameGraph& Graph, ResourceHandle Handle, float D);
	void PresentBackBuffer	(FrameGraph& Graph, IRenderWindow& Window);


	inline void PresentBackBuffer(FrameGraph& frameGraph, ResourceHandle backBuffer)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& pass = frameGraph.AddNode<PassData>(
			PassData{},
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.Present(backBuffer);
			},
			[](const PassData& Data, const ResourceHandler& Resources, IDirectContext& ctx, iAllocator&)
			{
			});
	}


	void ClearVertexBuffer	(FrameGraph& FG, VertexBufferHandle PushBuffer);


	/************************************************************************************************/


	[[nodiscard]]
	inline auto CreateOnceReserveBuffer2(FrameResources& resources, iAllocator& allocator)
	{
		return MakeLazyObject<CBPushBuffer>(
			allocator,
			[&resources](size_t size) mutable
			{
				return resources.ReserveCB(size);
			});
	}


	using CreateOnceReserveBufferFunction2 = decltype(CreateOnceReserveBuffer2(std::declval<FrameResources&>(), std::declval<iAllocator&>()));


	/************************************************************************************************/


	struct ShapeVert {
		float2 POS;
		float2 UV;
		float4 Color;
	};


	/************************************************************************************************/


	struct ShapeDraw
	{
		enum class RenderMode
		{
			Line,
			Triangle,
			Textured,
		}Mode = RenderMode::Triangle;

		ConstantBufferDataSet   constants;
		VertexBufferDataSet     vertices;
		size_t                  vertexCount     = 0;
		ResourceHandle          texture         = InvalidHandle;
	};

	typedef Vector<ShapeDraw> ShapeList;


	/************************************************************************************************/


	struct alignas(256) Constants
	{
		float4			Albedo;
		float4			Specular;
		float4x4_GPU	WT;
	};


	/************************************************************************************************/


	class ShapeProtoType
	{
	public:
		ShapeProtoType() {}
		virtual ~ShapeProtoType() {}
		ShapeProtoType(const ShapeProtoType& rhs) = delete;

		virtual void AddShapeDraw(
			ShapeList&				        DrawList,
			FrameResources&			        Resources) = 0;
	};


	/************************************************************************************************/


	class ShapeDrawList final : public ShapeProtoType
	{
	public:
		ShapeDrawList(iAllocator* allocator = SystemAllocator) :
			shapes{ allocator } {}

		~ShapeDrawList()
		{
			shapes.Release();
		}

		void AddShape(ShapeProtoType* Shape)
		{
			shapes.push_back(Shape);
		}

	protected:
		void AddShapeDraw(
			ShapeList&				        shapeList,
			FrameResources&			        Resources) override
		{
			for (auto shape : shapes)
				shape->AddShapeDraw(
					shapeList, 
					Resources);
		}

		Vector<ShapeProtoType*> shapes;
	};


	/************************************************************************************************/


	class CircleShape final : public ShapeProtoType
	{
	public:
		CircleShape(
			float2	IN_POS, 
			float	IN_Radius, 
			float4	IN_Color		= float4(1.0f),
			float	IN_AspectRatio	= 16.0f/9.0f,
			size_t	IN_Divisions	= 64) :
				Color		{ IN_Color			},
				POS			{ IN_POS			},
				R			{ IN_Radius			},
				Divisions	{ IN_Divisions		},
				AspectRatio { IN_AspectRatio	}{}

		void AddShapeDraw(
			ShapeList&		shapeList, 
			FrameResources&	resources) override;

		float2	POS;
		float4	Color;
		float	R;
		float	AspectRatio;
		size_t	Divisions;
	};


	/************************************************************************************************/


	class LineShape final : public ShapeProtoType
	{
	public:
		LineShape(LineSegments& lines) : 
			Lines	{ lines } {}

		void AddShapeDraw(
			ShapeList&		shapeList,
			FrameResources&	resources) override;

	private:
		LineSegments& Lines;
	};

	// for rendering lines already in Screen Space
	class SSLineShape final : public ShapeProtoType
	{
	public:
		SSLineShape(LineSegments& lines) :
			Lines	{ lines } {}

		SSLineShape(LineSegments&& lines) :
			Lines{ lines } {}


		void AddShapeDraw(
			ShapeList&		shapeList,
			FrameResources&	resources) override;


	private:
		LineSegments& Lines;
	};


	class RectangleShape final : public ShapeProtoType
	{
	public:
		RectangleShape(float2 POS_IN, float2 WH_IN, float4 Color_IN = float4(1.0f)) :
			POS		{ POS_IN	},
			WH		{ WH_IN		},
			Color	{ Color_IN	}{}


		void AddShapeDraw(
			ShapeList&		shapeList, 
			FrameResources&	resources) override;

		float2 POS;
		float2 WH;
		float4 Color;
	};


	/************************************************************************************************/


	class SolidRectangleListShape final : public ShapeProtoType
	{
	public:
		SolidRectangleListShape(Vector<Rectangle>&& rects_in) :
			rects	{ std::move(rects_in) }{}

		
		~SolidRectangleListShape() {}

		void AddShapeDraw(
			ShapeList&				        shapeList, 
			FrameResources&			        resources) override;

		Vector<Rectangle> rects;
	};


	/************************************************************************************************/

	using TextureList = Vector<ResourceHandle>;

	class TexturedRectangleListShape final : public ShapeProtoType
	{
	public:
		TexturedRectangleListShape(RectangleList&& rects_in, TextureList&& textures_in) :
			rects		{ std::move(rects_in)		},
			textures	{ std::move(textures_in )	}{}


		~TexturedRectangleListShape() {}

		void AddShapeDraw(
			ShapeList&				            shapeList,
			FrameResources&			            resources) override
		{
			/*
			FK_ASSERT(rects.size() == textures.size());
			if (rects.size() != textures.size())
				return;

			Constants CB_Data = {
			float4(1, 1, 1, 1),
			float4(1, 1, 1, 1),
			float4x4::Identity() };

			auto CBOffset = BeginNewConstantBuffer(CB, resources);
			PushConstantBufferData(CB_Data, CB, resources);

			const size_t VBOffset   = resources.GetVertexBufferOffset(pushBuffer);
			size_t vertexOffset     = 0;

			const size_t rectCount = rects.size();
			for (size_t I = 0; I < rectCount; ++I)
			{
				auto rect		= rects[I];
				auto texture	= textures[I];
			
				float2 rectUpperLeft	= rect.Position;
				float2 rectBottomRight	= rect.Position + rect.WH;
				float2 rectUpperRight	= { rectBottomRight.x,	rectUpperLeft.y };
				float2 rectBottomLeft	= { rectUpperLeft.x,	rectBottomRight.y };

				PushRectToVertexBuffer(
					[&](float2 POS, float2 UV) -> ShapeVert {
						return {
							Position2SS(float2(
								rect.Position.x + rect.WH.x * POS.x,
								rect.Position.y + rect.WH.y * (1 - POS.y))),
							UV, 
							rect.Color};
					}, pushBuffer, resources);

				drawList.push_back({ ShapeDraw::RenderMode::Textured, CBOffset, VBOffset, 6, vertexOffset, texture });
				vertexOffset += 6;
			}
			*/
		}

		Vector<Rectangle>		rects;
		Vector<ResourceHandle>	textures;
	};


	/************************************************************************************************/


	template<typename ... TY_OTHER>
	void DrawShapes(
		PSOHandle                       state, 
		FrameGraph&                     frameGraph,
		ResourceHandle                  renderTarget, 
		iAllocator*                     allocator, 
		TY_OTHER ...                    args)
	{
		struct ShapeParams
		{
			PSOHandle				        state;
			ShapeList				        draws;

			FrameResourceHandle		        renderTarget;
		};

		auto& Pass = frameGraph.AddNode<ShapeParams>(
			ShapeParams{
				state,
				ShapeList{ allocator },
			},
			[&](FrameGraphNodeBuilder& builder, ShapeParams& data)
			{
				// Single Thread Section
				// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
				// for thread safety
				data.renderTarget	= builder.RenderTarget(renderTarget);

				(args.AddShapeDraw(data.draws, frameGraph.resources), ...);
			},
			[=](const ShapeParams& data, const ResourceHandler& frameResources, IDirectContext& context, iAllocator& allocator)
			{	// Multi-threadable Section
				auto WH = frameResources.GetTextureWH(data.renderTarget);

				context.SetScissorAndViewports({ frameResources.GetResource(data.renderTarget)} );
				context.SetRenderTargets(
					{ frameResources.GetResource(data.renderTarget) },
					false);

				static auto rootSig = frameResources.renderSystem().Library(FlexKit::ROOTLIBRARYSIG::RS6CBVs4SRVs);
				context.SetRootSignature	(rootSig);
				context.SetPipelineState	(frameResources.GetPipelineState(data.state, allocator));
				context.SetInputPrimitive	(INPUTPRIMITIVETRIANGLELIST);

				size_t TextureDrawCount = 0;
				ShapeDraw::RenderMode PreviousMode = ShapeDraw::RenderMode::Triangle;
				for (const auto& D : data.draws)
				{

					switch (D.Mode) {
						case ShapeDraw::RenderMode::Line:
						{
							context.SetInputPrimitive(INPUTPRIMITIVELINELIST);
						}	break;
						case ShapeDraw::RenderMode::Triangle:
						{
							context.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);
						}	break;
						case ShapeDraw::RenderMode::Textured:
						{
							context.SetInputPrimitive(INPUTPRIMITIVETRIANGLELIST);

							DescriptorHeap descHeap;
							auto& desciptorTableLayout = rootSig->GetDescHeap(0);

							descHeap.Init2(context, desciptorTableLayout, 1, &allocator);
							descHeap.NullFill(context, 1);
							descHeap.SetSRV(context, 0, D.texture);

							context.SetGraphicsDescriptorTable(0, descHeap);
						}	break;
					}

					context.SetVertexBuffers({ D.vertices });
					context.SetGraphicsConstantBufferView(2, D.constants);
					context.Draw(D.vertexCount, 0);
					
					PreviousMode = D.Mode;
				}
			});
	} 



	/************************************************************************************************/


	struct DrawCollection_Desc
	{
		TriMeshHandle			Mesh;
		ResourceHandle			RenderTarget;
		ResourceHandle			DepthBuffer;
		VertexBufferHandle		instanceBuffer;
		ConstantBufferHandle	constantBuffer;
		PSOHandle				PSO;
		bool					enableDepthBuffer = true;

		size_t	reserveCount			= 512;
	};

	struct _DCConstantData
	{
		size_t	idx;
		char*	buffer;
		size_t	bufferSize;
	};

	struct _DCConstantBuffer
	{
		size_t					idx;
		ConstantBufferHandle	constantBuffer;
		size_t					offset;
	};

	template<typename FETCHINSTANCES_FN, typename FETCHCONSTANTS_FN, typename FORMATINSTANCE_FN, typename PIPELINESETUP_FN>
	void DrawCollection(
		FrameGraph&							frameGraph,
		static_vector<UpdateTask*>			dependencies,
		static_vector<_DCConstantData>		constantData,
		static_vector<_DCConstantBuffer>	constantBuffers,
		FETCHCONSTANTS_FN&&					fetchConstants,
		FETCHINSTANCES_FN&&					fetchInstances,
		FORMATINSTANCE_FN&&					formatInstanceData,
		PIPELINESETUP_FN&&					setupPipeline,
		const DrawCollection_Desc&			desc,
		iAllocator*							tempAllocator)
	{
		using FetchInstancesFN_t	= decltype(fetchInstances);
		using FetchConstantsFN_t	= decltype(fetchConstants);

		

		struct _DrawCollection
		{
			_DrawCollection(FETCHINSTANCES_FN&& in_fetchInstances, FETCHCONSTANTS_FN&& in_fetchConstants) :
				fetchInstances{ std::move(in_fetchInstances) },
				fetchConstants{ std::move(in_fetchConstants) }{}


			FETCHCONSTANTS_FN					fetchConstants;
			FETCHINSTANCES_FN					fetchInstances;

			FrameResourceHandle					renderTarget;
			FrameResourceHandle					depthBuffer;

			TriMeshHandle						mesh;
			VBPushBuffer						instanceBuffer;
			CBPushBuffer						constantBuffer;

			static_vector<_DCConstantData>		constants;
			static_vector<_DCConstantBuffer>	constantBuffers;

			size_t								instanceElementSize;
		};

		constexpr size_t instanceElementSize = sizeof(decltype(formatInstanceData(fetchInstances())));

		frameGraph.AddNode<_DrawCollection>(
			_DrawCollection{ std::move(fetchInstances), std::move(fetchConstants) },
			[&](FrameGraphNodeBuilder& builder, _DrawCollection& data)
			{
				for (auto& dep : dependencies)
					builder.AddDataDependency(*dep);

				data.renderTarget	= builder.RenderTarget(desc.RenderTarget);
				data.depthBuffer	= desc.enableDepthBuffer ? builder.DepthTarget(desc.DepthBuffer) : InvalidHandle;

				size_t MaxElementSize = 0;
				for (auto& i : constantData)
					MaxElementSize = Max(MaxElementSize, i.bufferSize);

				data.constantBuffer			= Reserve(desc.constantBuffer, MaxElementSize, constantData.size(), frameGraph.resources);
				data.instanceBuffer			= Reserve(desc.instanceBuffer, instanceElementSize * desc.reserveCount, frameGraph.resources);
				data.mesh					= desc.Mesh;
				data.instanceElementSize	= instanceElementSize;
				data.constants				= constantData;
				data.constantBuffers		= constantBuffers;
			},
			[=, setup = std::move(setupPipeline)](_DrawCollection& data, const FrameResources& resources, IDirectContext& ctx)
			{
				data.fetchConstants(data);
				auto entities = data.fetchInstances();

				for (auto& entity : entities)
					data.instanceBuffer.Push(formatInstanceData(entity));


				auto* triMesh			= GetMeshResource(data.mesh);
				size_t MeshVertexCount	= triMesh->IndexCount;

				setup(data, resources, ctx);

				ctx.SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx.SetRenderTargets(
					{	resources.GetResource(data.renderTarget) }, true,
						resources.GetResource(data.depthBuffer));

				// Bind resources
				VertexBufferList instancedBuffers;
				instancedBuffers.push_back(VertexBufferEntry{
					data.instanceBuffer,
					(uint32_t)data.instanceElementSize,
					(uint32_t)data.instanceBuffer.begin() });


				ctx.AddIndexBuffer(triMesh);
				ctx.AddVertexBuffers(triMesh,
					{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,    },
					&instancedBuffers);


				for (auto& CBEntry : data.constants) 
					ctx.SetGraphicsConstantBufferView(
						1u + CBEntry.idx, 
						data.constantBuffer, 
						data.constantBuffer.Push(CBEntry.buffer, CBEntry.bufferSize));

				for (auto& constantBuffer : data.constantBuffers)
					ctx.SetGraphicsConstantBufferView(1u + constantBuffer.idx, data.constantBuffer, constantBuffer.offset);

				ctx.DrawIndexedInstanced(MeshVertexCount, 0, 0, entities.size());
			});
	}


	/************************************************************************************************/


	struct DrawWireframeRectangle_Desc
	{
		ResourceHandle			RenderTarget;
		VertexBufferHandle		VertexBuffer;
		ConstantBufferHandle	constantBuffer;
		CameraHandle			camera;
		PSOHandle				PSO;
	};


	void WireframeRectangleList(
		FrameGraph&						frameGraph,
		DrawWireframeRectangle_Desc&	desc,
		Vector<Rectangle>&				rects,
		iAllocator*						TempMem);


	/************************************************************************************************/

	// Requires a registered DRAW_LINE3D_PSO pipeline state!
	void Draw3DGrid(
		FrameGraph&				frameGraph,
		const size_t			ColumnCount,
		const size_t			RowCount,
		const float2			GridWH,
		const float4			GridColor,
		ResourceHandle			RenderTarget,
		ResourceHandle			DepthBuffer,
		VertexBufferHandle		vertexBuffer,
		ConstantBufferHandle	constants,
		CameraHandle			Camera,
		iAllocator*				TempMem);


	/************************************************************************************************/


	struct ClearIntegerRenderTarget_RG32_Clear
	{
		ReserveConstantBufferFunction   ReserveCB;
		FrameResourceHandle             feedbackTarget;
	};

	ClearIntegerRenderTarget_RG32_Clear& ClearIntegerRenderTarget_RG32(
		FrameGraph& frameGraph,
		ResourceHandle                  target,
		ReserveConstantBufferFunction&	reserveCB,
		uint2                           value = { 0u, 0u });


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/

#endif
