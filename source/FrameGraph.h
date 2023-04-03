/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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


#ifndef RENDERGRAPH_H
#define RENDERGRAPH_H

#include "graphics.h"
#include "GraphicsComponents.h"
#include "containers.h"
#include "Components.h"
#include "defaultpipelinestates.h"
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
		Virtual_LongLived,
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
		DeviceAccessState				access			= DeviceAccessState::DASCommon;
		DeviceLayout					layout;
		TextureDimension				dimensions;
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
			shaderResource.layout			= DeviceLayout::DeviceLayout_ShaderResource;
			shaderResource.type				= OT_RenderTarget;
			shaderResource.shaderResource	= resource;
			shaderResource.dimensions		= dimensions;
			shaderResource.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return shaderResource;
		}


		static FrameObject RenderTargetObject(ResourceHandle resource, iAllocator& allocator)
		{
			FrameObject renderTarget;
			renderTarget.layout			= DeviceLayout::DeviceLayout_RenderTarget;
			renderTarget.type			= OT_RenderTarget;
			renderTarget.shaderResource = resource;
			renderTarget.dimensions		= TextureDimension::Texture2D;
			renderTarget.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return renderTarget;
		}


		static FrameObject BackBufferObject(ResourceHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::DeviceLayout_RenderTarget)
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


		static FrameObject SOBufferObject(SOResourceHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::DeviceLayout_Common)
		{
			FrameObject Streamout;
			Streamout.layout		= initialLayout;
			Streamout.type			= OT_StreamOut;
			Streamout.SOBuffer		= resource;
			Streamout.dimensions	= TextureDimension::Texture2D;
			Streamout.lastUsers		= Vector<FrameGraphNodeHandle>{ allocator };

			return Streamout;
		}


		static FrameObject QueryObject(QueryHandle resource, iAllocator& allocator, DeviceLayout initialLayout = DeviceLayout::DeviceLayout_Common)
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
			virtualObject.layout			= DeviceLayout::DeviceLayout_Common;
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


	FLEXKITAPI class FrameResources
	{
	public:
		FrameResources(RenderSystem& IN_renderSystem, iAllocator* IN_allocator) : 
			allocator			{ IN_allocator		},
			memoryPools			{ IN_allocator		},
			outputObjects		{ IN_allocator		},
			renderSystem		{ IN_renderSystem	},
			objects				{ IN_allocator		},
			virtualResources	{ IN_allocator		} {}

		PassObjectList					objects;
		Vector<FrameResourceHandle>		outputObjects;
		TemporaryPassObjectList			virtualResources;
		Vector<PoolAllocatorInterface*>	memoryPools;
		std::mutex						m;

		RenderSystem&	renderSystem;
		iAllocator*		allocator;

		uint32_t virtualResourceCount = 0;

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
				renderSystem.GetObjectLayout(Handle));
		}


		/************************************************************************************************/


		void AddRenderTarget(ResourceHandle handle, DeviceLayout layout = DeviceLayout_Common)
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
			objects.push_back(FrameObject::ConstantBufferObject(handle, * allocator));

			return handle;
		}


		/************************************************************************************************/


		FrameResourceHandle AddReadBackBuffer(ReadBackResourceHandle)
		{
			auto handle = FrameResourceHandle{ (uint32_t)objects.size() };
			objects.push_back(FrameObject::ConstantBufferObject(handle, * allocator));

			MarkResourceObjectAsOutput(handle);

			return handle;
		}


		/************************************************************************************************/


		void AddDepthBuffer(ResourceHandle Handle)
		{
			AddDepthBuffer(
				Handle,
				renderSystem.GetObjectLayout(Handle));
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
			DeviceLayout layout = renderSystem.GetObjectLayout(handle);

			objects.push_back(
				FrameObject::SOBufferObject(handle, *allocator, layout));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
		}


		/************************************************************************************************/


		FrameResourceHandle AddResource(ResourceHandle handle, const bool renderTarget = false)
		{
			DeviceLayout		layout		= renderSystem.GetObjectLayout(handle);
			TextureDimension	dimensions	= renderSystem.GetTextureDimension(handle);

			if (auto res = FindFrameResource(handle); res != InvalidHandle)
				return res;
			else
			{
				objects.push_back(
					FrameObject::TextureObject(handle, layout, dimensions, *allocator));

				auto resourceHandle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
				objects.back().handle = resourceHandle;

				return resourceHandle;
			}
		}


		/************************************************************************************************/


		FrameResourceHandle AddResourceMT(ResourceHandle handle, const bool renderTarget = false)
		{
			std::scoped_lock lock{};

			DeviceLayout layout = renderSystem.GetObjectLayout(handle);
			auto	dimensions	= renderSystem.GetTextureDimension(handle);

			objects.push_back(
				FrameObject::TextureObject(handle, layout, dimensions, *allocator));

			auto resourceHandle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
			objects.back().handle = resourceHandle;

			return resourceHandle;
		}


		/************************************************************************************************/


		void AddQuery(QueryHandle handle)
		{
			DeviceLayout layout = renderSystem.GetObjectLayout(handle);

			objects.push_back(
				FrameObject::QueryObject(handle, *allocator, layout));

			objects.back().handle = FrameResourceHandle{ (uint32_t)objects.size() - 1 };
		}


		/************************************************************************************************/


		template<typename TY>
		ID3D12Resource* GetDeviceResource(TY handle) const
		{
			return renderSystem.GetDeviceResource(handle);
		}

		ID3D12Resource* GetDeviceResource(FrameResourceHandle handle) const
		{
			return renderSystem.GetDeviceResource(GetResource(handle));
		}


		/************************************************************************************************/


		ID3D12PipelineState* GetPipelineState(PSOHandle State)	const
		{
			return renderSystem.GetPSO(State);
		}


		/************************************************************************************************/


		size_t GetVertexBufferOffset(VertexBufferHandle Handle, size_t VertexSize)
		{
			return renderSystem.VertexBuffers.GetCurrentVertexBufferOffset(Handle) / VertexSize;
		}


		/************************************************************************************************/


		size_t GetVertexBufferOffset(VertexBufferHandle Handle)
		{
			return renderSystem.VertexBuffers.GetCurrentVertexBufferOffset(Handle);
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
						LHS.type == OT_Resource	||
						LHS.type == OT_RenderTarget		||
						LHS.type == OT_DepthBuffer      ||
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


		operator RenderSystem& ()
		{
			return *renderSystem;
		}

		uint2 GetTextureWH(FrameResourceHandle handle) const
		{
			if (auto res = GetResource(handle); res != InvalidHandle)
				return renderSystem.GetTextureWH(res);
			else
				return { 0, 0 };
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
		ID3D12Resource*			GetDeviceResource(TY handle) const						{ return globalResources.GetDeviceResource(handle); }
		ID3D12Resource*			GetDeviceResource(FrameResourceHandle handle) const		{ return globalResources.GetDeviceResource(GetResource(handle)); }

		template<typename TY>
		DevicePointer			GetDevicePointer(TY handle) const						{ return { GetDeviceResource(handle)->GetGPUVirtualAddress() }; }

		ID3D12PipelineState*	GetPipelineState(PSOHandle state) const					{ return globalResources.GetPipelineState(state); }

		size_t					GetVertexBufferOffset(VertexBufferHandle handle, size_t vertexSize)	{ return globalResources.GetVertexBufferOffset(handle, vertexSize); }
		size_t					GetVertexBufferOffset(VertexBufferHandle handle)					{ return globalResources.GetVertexBufferOffset(handle); }

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
				DebugBreak();
				//SubNodeTracking.push_back({ res->Handle, res->State });

				return res->SOBuffer;
			}
			else
				return globalResources.objects[res->resource].SOBuffer;
		}


		D3D12_VERTEX_BUFFER_VIEW ReadStreamOut(FrameResourceHandle handle, Context& ctx, size_t vertexSize) const
		{
			auto& res			= _FindSubNodeResource(handle);
			auto SOHandle		= globalResources.objects[res.resource].SOBuffer;
			auto deviceResource = renderSystem().GetDeviceResource(SOHandle);

			DebugBreak();
			if (res.access != DASVERTEXBUFFER && res.layout != DeviceLayout_GenericRead) 
				ctx.AddStreamOutBarrier(SOHandle, res.access, DASVERTEXBUFFER);

			res.access = DASVERTEXBUFFER;

			D3D12_VERTEX_BUFFER_VIEW view = {
				deviceResource->GetGPUVirtualAddress(),
				static_cast<UINT>(renderSystem().GetStreamOutBufferSize(SOHandle)),
				static_cast<UINT>(vertexSize)
			};

			return view;
		}


		ResourceHandle Transition(const FrameResourceHandle resource, DeviceAccessState access, DeviceLayout layout, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
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

		ResourceHandle CopyDest(FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All)
		{
			return Transition(resource, DASCopyDest, DeviceLayout::DeviceLayout_DirectQueueCopyDst, ctx, before, after);
		}

		ResourceHandle CopySrc(FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All)
		{
			return Transition(resource, DASCopySrc, DeviceLayout::DeviceLayout_DirectQueueCopySrc, ctx, before, after);
		}

		ResourceHandle UAV(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASUAV, DeviceLayout::DeviceLayout_UnorderedAccess, ctx, before, after);
		}

		ResourceHandle RenderTarget(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASRenderTarget, DeviceLayout::DeviceLayout_RenderTarget, ctx, before, after);
		}

		ResourceHandle PixelShaderResource(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASPixelShaderResource, DeviceLayout::DeviceLayout_DirectQueueShaderResource, ctx, before, after);
		}

		ResourceHandle NonPixelShaderResource(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASNonPixelShaderResource, DeviceLayout::DeviceLayout_ShaderResource, ctx, before, after);
		}

		ResourceHandle IndirectArgs(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASINDIRECTARGS, DeviceLayout::DeviceLayout_DirectQueueGenericRead, ctx, before, after);
		}

		ResourceHandle VertexBuffer(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::DeviceLayout_DirectQueueGenericRead, ctx, before, after);
		}

		ResourceHandle ResolveDst(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::DeviceLayout_ResolveDst, ctx, before, after);
		}

		ResourceHandle ResolveSrc(const FrameResourceHandle resource, Context& ctx, DeviceSyncPoint before = DeviceSyncPoint::Sync_All, DeviceSyncPoint after = DeviceSyncPoint::Sync_All) const
		{
			return Transition(resource, DASVERTEXBUFFER, DeviceLayout::DeviceLayout_ResolveSrc, ctx, before, after);
		}

		ID3D12Resource* ResolveDst(const ReadBackResourceHandle resource, Context& ctx) const
		{
			return renderSystem().GetDeviceResource(resource);
			//return Transition(resource, DASVERTEXBUFFER, DeviceLayout::DeviceLayout_ResolveDst, ctx, before, after);
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
				DebugBreak();
				//SubNodeTracking.push_back({ res->Handle, res->State });

				return { res->access, res->layout };
			}
			else
				return { res->access, res->layout };
		}

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

		RenderSystem& renderSystem() const { return *globalResources.renderSystem; }


		operator RenderSystem& ()
		{
			return *globalResources.renderSystem;
		}


		uint2 GetTextureWH(FrameResourceHandle handle) const
		{
			return globalResources.GetTextureWH(handle);
		}


		private:

		LocallyTrackedResource& _FindSubNodeResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](auto& res)
				{
					return res.resource == handle;
				});

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
			DeviceLayout			IN_neededLayout		= DeviceLayout::DeviceLayout_Unknown) :
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
		using FN_NodeAction = TypeErasedCallable<void (FrameGraphNode& node, FrameResources& Resources, Context& ctx, iAllocator& tempAllocator), 64>;

		size_t			workWeight = 0;
		FN_NodeAction	action;
		FrameGraphNode* node;

		void operator () (Context& ctx, FrameResources& resources, iAllocator& allocator)
		{
			action(*node, resources, ctx, allocator);
		}

	};

	FLEXKITAPI class FrameGraphNode
	{
	public:
		typedef void (*FN_NodeGetWorkItems)	(FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& OUT_workItem, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator);

		FrameGraphNode(FrameGraphNodeHandle handle, FN_NodeGetWorkItems IN_action, void* IN_nodeData, iAllocator* IN_allocator = nullptr);

		FrameGraphNode(FrameGraphNode&& RHS);
		FrameGraphNode(const FrameGraphNode& RHS) = delete;

		~FrameGraphNode() = default;

		void HandleBarriers	(FrameResources& Resouces, Context& Ctx);
		void AddBarrier		(const Barrier& Dep);

		void RestoreResourceStates	(Context* ctx, FrameResources& resources, LocallyTrackedObjectList& locallyTrackedObjects);
		void AcquireResources		(FrameResources& resources, Context& ctx);
		void ReleaseResources		(FrameResources& resources, Context& ctx);

		ResusableResourceQuery			FindReuseableResource(PoolAllocatorInterface& allocator, size_t allocationSize, uint32_t flags, FrameResources& handler, std::span<FrameGraphNode> nodes);
		std::optional<InputObject>		GetInputObject(FrameResourceHandle);


		FrameGraphNodeHandle			handle;
		void*							nodeData;
		bool							executed = false;
		FN_NodeGetWorkItems				nodeAction;
		Vector<FrameGraphNodeHandle>	sources;// Nodes that this node reads from
		Vector<InputObject>				inputObjects;
		Vector<FrameObjectLink>			outputObjects;
		Vector<Barrier>					barriers;

		Vector<ResourceAcquisition>		acquiredObjects;
		Vector<FrameObjectLink>			retiredObjects;
		Vector<FrameObjectLink>			createdObjects;

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


	FLEXKITAPI class FrameGraphResourceContext
	{
	public:
		FrameGraphResourceContext(FrameResources& IN_resources, ThreadManager& IN_threads, RenderSystem& IN_renderSystem, iAllocator* Temp) :
			frameResources	{ IN_resources		},
			writables		{ Temp				},
			readables		{ Temp				},
			retirees		{ Temp				},
			threads			{ IN_threads		},
			pendingTasks	{ IN_threads		},
			renderSystem	{ IN_renderSystem	} {}


		void AddWriteable(const FrameObjectLink& NewObject)
		{
			writables.push_back(NewObject);
		}


		void AddReadable(const FrameObjectLink& NewObject)
		{
			readables.push_back(NewObject);
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


		Vector<FrameObjectLink>	writables;
		Vector<FrameObjectLink>	readables;
		Vector<FrameObjectLink>	retirees;

		iAllocator*				tempAllocator;
		ThreadManager&			threads;
		WorkBarrier				pendingTasks;
		FrameResources&			frameResources;
		RenderSystem&			renderSystem;
	};


	/************************************************************************************************/


	enum class VirtualResourceScope
	{
		Temporary,
		Frame,
	};

	DeviceLayout	GuessLayoutFromAccess(DeviceAccessState access);
	uint32_t		GetNeededFlags(const GPUResourceDesc& desc);

	FLEXKITAPI class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			std::span<FrameGraphNode>	IN_nodeTable,
			Vector<UpdateTask*>&		IN_DataDependencies,
			FrameResources*				IN_Resources, 
			FrameGraphNode&				IN_Node,
			FrameGraphResourceContext&	IN_context,
			iAllocator*					IN_allocator) :
				dataDependencies	{ IN_DataDependencies	},
				context				{ IN_context			},
				inputNodes			{ IN_allocator			},
				node				{ IN_Node				},
				nodeTable			{ IN_nodeTable			},
				resources			{ IN_Resources			},
				retiredObjects		{ IN_allocator			},
				barriers			{ IN_allocator			},
				temporaryObjects	{ IN_allocator			},
				acquiredResources	{ IN_allocator			},
				allocator			{ IN_allocator			} {}


		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;

		void BuildNode(FrameGraph* FrameGraph);
		void BuildPass(FrameGraph* FrameGraph, FrameGraphNode& endNode);

		void AddDataDependency(UpdateTask& task);
		void AddNodeDependency(FrameGraphNodeHandle node);


		FrameGraphNodeHandle GetNodeHandle() const;

		FrameResourceHandle CreateConstantBuffer();
		FrameResourceHandle ReadConstantBuffer(FrameResourceHandle);

		FrameResourceHandle AccelerationStructureRead(ResourceHandle);
		FrameResourceHandle AccelerationStructureWrite(ResourceHandle);

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
		FrameResourceHandle	AcquireVirtualResource(PoolAllocatorInterface& allocator, const GPUResourceDesc& desc, DeviceAccessState, VirtualResourceScope lifeSpan = VirtualResourceScope::Temporary);

		void				ReleaseVirtualResource(FrameResourceHandle handle);


		FrameResourceHandle	UnorderedAccess (ResourceHandle, DeviceAccessState state = DeviceAccessState::DASUAV);

		FrameResourceHandle	VertexBuffer	(SOResourceHandle);
		FrameResourceHandle	StreamOut		(SOResourceHandle);

		FrameResourceHandle ReadTransition	(FrameResourceHandle handle, DeviceAccessState state, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All });
		FrameResourceHandle WriteTransition	(FrameResourceHandle handle, DeviceAccessState state, std::pair<DeviceSyncPoint, DeviceSyncPoint> syncPoints = { Sync_All, Sync_All });

		FrameResourceHandle	ReadBack(ReadBackResourceHandle);

		void SetDebugName(FrameResourceHandle handle, const char* debugName);

		size_t							GetDescriptorTableSize			(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot
		const DesciptorHeapLayout<16>&	GetDescriptorTableLayout		(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot

		RenderSystem& GetRenderSystem() { return resources->renderSystem; }

		operator FrameResources&	() const	{ return *resources; }
		operator RenderSystem&		()			{ return *resources->renderSystem;}

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

			if (layout == DeviceLayout_Unknown)
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
			node.subNodeTracking.push_back({ frameResourceHandle, access, layout, std::get<0>(finalTransition.value_or(p)), std::get<1>(finalTransition.value_or(p)) });

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

			if (layout == DeviceLayout_Unknown)
				layout = frameObject.layout;

			if (frameObject.layout != layout || !CheckCompatibleAccessState(frameObject.layout, frameObject.access))
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
			node.subNodeTracking.push_back({ frameResourceHandle, access, layout, std::get<0>(finalTransition.value_or(p)), std::get<1>(finalTransition.value_or(p)) });

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
		Vector<UpdateTask*>&			dataDependencies;
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

	template<typename Shared_TY, typename TY = const PVEntry>
	struct PassDescription
	{
		Shared_TY							sharedData;
		TypeErasedCallable<std::span<TY>()>	getPVS;

		using Shared		= Shared_TY;
		using GetPass_TY	= TypeErasedCallable<std::span<TY> ()>;
	};

	template<typename Shared_TY, typename TY_PassData, typename TY_PVSElements = const PVEntry>
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
	};

	template<typename FillData_TY, typename GetPass_TY>
	struct PassDrivenResourceAllocation
	{
		GetPass_TY				getPass;				// std::span<ty> ()
		FillData_TY				initializeResources;	// void (std::span<ty>, std::span<FrameResourceHandles>, ResourceInitializationContext& transferCtx, iAllocator&);

		DeviceLayout			layout;
		DeviceAccessState		access;
		size_t					max			= 16;
		PoolAllocatorInterface* pool		= nullptr;
		UpdateTask*				dependency	= nullptr;
	};


	FLEXKITAPI class FrameGraph
	{
	public:
		FrameGraph(RenderSystem& RS, ThreadManager& IN_threads, iAllocator* Temp) :
			resources			{ RS, Temp },
			threads				{ IN_threads },
			dataDependencies	{ Temp },
			resourceContext		{ resources, IN_threads, RS, Temp },
			memory				{ Temp },
			nodes				{ Temp },
			acquiredResources	{ Temp },
			pendingAcquire		{ Temp },
			submissionTicket	{ RS.GetSubmissionTicket() }{}

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
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					FrameGraphNodeWorkItem newWorkItem;
					newWorkItem.node	= &node;
					newWorkItem.action	=
						[](	FrameGraphNode&	node,
							FrameResources&	resources,
							Context&		ctx,
							iAllocator&		tempAllocator)
						{
							ProfileFunction();

							NodeData& data = *reinterpret_cast<NodeData*>(node.nodeData);

							node.HandleBarriers(resources, ctx);

							LocallyTrackedObjectList localTracking{ &tempAllocator };
							localTracking = node.subNodeTracking;

							ResourceHandler handler{ resources, localTracking };
							data.draw(data.fields, handler, ctx, tempAllocator);

							node.RestoreResourceStates(&ctx, resources, localTracking);

							{
								ProfileFunctionLabeled(Destruction);
								data.fields.~TY();
							}
						};

					tasks_out.push_back(newWorkItem);
				},
				&data,
				memory);

			FrameGraphNodeBuilder builder(nodes, dataDependencies, &resources, nodes[idx], resourceContext, memory);

			setup(builder, data.fields);
			builder.BuildNode(this);

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

			FrameGraphNodeBuilder builder(nodes, dataDependencies, &resources, nodes[idx], resourceContext, memory);

			auto& data = memory->allocate_aligned<NodeData>(setupLinkage(builder), std::move(resourceCreation), std::move(threadedTask));
			nodes[idx].nodeData = &data;

			builder.BuildNode(this);

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
				std::atomic_uint	refCount = 0;
			} &start = memory->allocate_aligned<PassData>(
									std::move(std::forward<TY_Shared>(IN_shared.sharedData)),
									std::move(draw),
									std::move(IN_shared.getPVS));

			auto startNodeIdx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
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
							newWorkItem.node		= &node;
							newWorkItem.workWeight	= end - begin;

							newWorkItem.action	=
								[i, blockCount, blockSize, passPVS](
									FrameGraphNode&	node,
									FrameResources&	resources,
									Context&		ctx,
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
										node.RestoreResourceStates(&ctx, resources, localTracking);

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
						FrameGraphNodeWorkItem newWorkItem;
						newWorkItem.node		= &node;
						newWorkItem.workWeight	= 1;

						newWorkItem.action	=
							[](
								FrameGraphNode&	node,
								FrameResources&	resources,
								Context&		ctx,
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

								node.RestoreResourceStates(&ctx, resources, localTracking);

								ProfileFunctionLabeled(Destruction);
								data.~PassData();
							};

						tasks_out.emplace_back(std::move(newWorkItem));
					}
				},
				&start,
				memory);


			FrameGraphNodeBuilder builder(nodes, dataDependencies, &resources, nodes[startNodeIdx], resourceContext, memory);
			setupLinkage(builder, start.shared);
			builder.BuildNode(this);

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

			auto startNodeIdx = nodes.emplace_back(
				FrameGraphNodeHandle{ nodes.size() },
				[](FrameGraphNode& node, Vector<FrameGraphNodeWorkItem>& tasks_out, FlexKit::WorkBarrier& barrier, FrameResources& resources, iAllocator& tempAllocator)
				{
					PassData* passData	= reinterpret_cast<PassData*>(node.nodeData);
					passData->passes	= passData->getPasses(tempAllocator);

					for(auto& pass : passData->passes)
					{
						auto passPVS		= passData->getPVS(pass);
						auto size			= passPVS.size();

						uint16_t blockCount = (uint16_t)(size / 1000 + (size % 1000 > 0 ? 1 : 0));
						uint16_t blockSize	= (uint16_t)(size / blockCount);

						passData->refCount = blockCount;

						for (uint16_t i = 0; i < blockCount; i++)
						{
							auto begin	= Min((i + 0) * blockSize, size);
							auto end	= Min((i + 1) * blockSize, size);

							FrameGraphNodeWorkItem newWorkItem;
							newWorkItem.node		= &node;
							newWorkItem.workWeight	= end - begin;

							newWorkItem.action	=
								[i, blockCount, blockSize, passPVS, &pass](
									FrameGraphNode&	node,
									FrameResources&	resources,
									Context&		ctx,
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
										node.RestoreResourceStates(&ctx, resources, localTracking);
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


			FrameGraphNodeBuilder builder(nodes, dataDependencies, &resources, nodes[startNodeIdx], resourceContext, memory);
			setupLinkage(builder, start.shared);
			builder.BuildNode(this);

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
				Vector<InitialData>		resourceAllocations;
				size_t					uploadSize = 0;
				PoolAllocatorInterface* pool;

				DeviceAccessState			access;
				FrameGraphResourceContext&	resourceContext;
				iAllocator*					allocator;
				UploadSegment				uploadSegment;
			};

			auto nodeHandle	= FrameGraphNodeHandle{ nodes.size() }; 
			NodeData& data	= memory->allocate_aligned<NodeData>(
								desc.getPass,
								desc.initializeResources,
								ResourceAllocation	{ memory, nodeHandle },
								Vector<InitialData>	{ memory },
								0,
								desc.pool,
								desc.access,
								resourceContext,
								memory);

			data.resourceAllocations.reserve(desc.max);

			struct ResourceInitializationContext
			{
				FrameResources& frameResources;
				NodeData*		nodeData;

				void CreateResource(FrameResourceHandle dstResource, size_t resourceSize, void* initialData)
				{
					nodeData->resourceAllocations.emplace_back(initialData, resourceSize, dstResource);

					std::atomic_ref ref{ frameResources.virtualResourceCount };
					ref++;


					auto GetMemoryPool = [&]
					{
						if (!nodeData->pool)
						{
							const auto& desc = GPUResourceDesc::StructuredResource(resourceSize);
							const auto	flags = GetNeededFlags(desc);
							return frameResources.FindMemoryPool(flags);
						}
						else
							return nodeData->pool;
					};

					auto pool = GetMemoryPool();
					auto [resourceHandle, overlap] = pool->Acquire(GPUResourceDesc::StructuredResource(resourceSize));


					auto frameObject				= frameResources.GetResourceObject(dstResource);
					frameObject->shaderResource		= resourceHandle;
					frameObject->pool				= pool;
					frameObject->virtualState		= VirtualResourceState::Virtual_Created;

					nodeData->uploadSize += resourceSize;
				}

				void CreateZeroedResource(FrameResourceHandle dstResource)
				{

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

					const auto	passPVS = nodeData->getPass();
					const auto	size	= passPVS.size();

					if (!size)
						return;

					auto& threadedTask = CreateWorkItem(
						[nodeData, &resources, passPVS](auto& tempAllocator)
						{
							ResourceInitializationContext transferCtx{ resources, nodeData };
							nodeData->initializeResources(passPVS, nodeData->resources.handles, transferCtx, *nodeData->allocator);

							auto uploadSegment = resources.renderSystem._ReserveDirectUploadSpace(nodeData->uploadSize, 256);
							size_t offset = 0;

							// Copy Data into resources, insert barrier/transition
							for (auto&& [_ptr, size, resource] : nodeData->resourceAllocations)
							{
								memcpy(uploadSegment.buffer + offset, _ptr, size);
								offset += size;
							}

							nodeData->uploadSegment = uploadSegment;
						}, tempAllocator);

					nodeData->resourceContext.AddResourceTask(threadedTask);

					FrameGraphNodeWorkItem newWorkItem;
					newWorkItem.node		= &node;
					newWorkItem.workWeight	= size;

					newWorkItem.action =
						[nodeData](
							FrameGraphNode& node,
							FrameResources& resources,
							Context&		ctx,
							iAllocator&		localAllocator)
						{
							ctx.BeginEvent_DEBUG("Initiate Resources");

							auto uploadSegment = nodeData->uploadSegment;
							size_t offset = 0;

							if (nodeData->resourceAllocations.size() == 0 && uploadSegment.uploadSize == 0)
							{
								FK_LOG_ERROR("Failed to get allocation upload segment");
								return;
							}

							// Insert Barrier
							for (auto&& [_ptr, size, resource] : nodeData->resourceAllocations)
								ctx.AddBufferBarrier(resources.GetResource(resource), DeviceAccessState::DASCommon, DeviceAccessState::DASCopyDest, DeviceSyncPoint::Sync_All, DeviceSyncPoint::Sync_Copy);

							// Copy Data into resources
							for (auto&& [_ptr, size, resource] : nodeData->resourceAllocations)
							{
								ctx.CopyBufferRegion(
									resources.GetDeviceResource(resource), uploadSegment.resource,	// dst_res, src_res
									size,															// copy size
									0, uploadSegment.offset + offset);								// dst_offset, src offset

								offset += size;
							}

							for (auto&& [_ptr, size, resource] : nodeData->resourceAllocations)
								ctx.AddBufferBarrier(resources.GetResource(resource), DeviceAccessState::DASCopyDest, nodeData->access, DeviceSyncPoint::Sync_Copy, DeviceSyncPoint::Sync_All);

							ctx.EndEvent_DEBUG();
						};

					tasks_out.emplace_back(std::move(newWorkItem));
				},
				&data,
				memory);

			FrameGraphNodeBuilder builder(nodes, dataDependencies, &resources, nodes[nodeIdx], resourceContext, memory);

			if (desc.dependency)
				builder.AddDataDependency(*desc.dependency);

			data.resources.handles.reserve(desc.max);

			for (size_t i = 0; i < desc.max; i++)
			{
				auto resource = builder.AcquireResourceHandle(desc.access, desc.layout);
				data.resources.handles.push_back(resource);
			}
			builder.BuildNode(this);

			return data.resources;
		}

		void AddMemoryPool		(PoolAllocatorInterface* poolAllocator);
		void AddTaskDependency	(UpdateTask& task);

		FrameResourceHandle		AddResource	(ResourceHandle resource);
		FrameResourceHandle		AddOutput	(ResourceHandle resource);

		UpdateTask&		SubmitFrameGraph(UpdateDispatcher& dispatcher, RenderSystem* RS, iAllocator* persistentAllocator);

		RenderSystem&	GetRenderSystem() { return resources.renderSystem; }

		FrameResources				resources;
		Vector<ResourceHandle>		acquiredResources;
		Vector<PendingAcquire>		pendingAcquire;
		FrameGraphResourceContext	resourceContext;
		iAllocator*					memory;
		Vector<FrameGraphNode>		nodes;
		ThreadManager&				threads;
		DataDependencyList			dataDependencies;
		SyncPoint					submissionTicket;

		void _SubmitFrameGraph(iAllocator& allocator);
	private:

		void UpdateResourceFinalState();
	};


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.renderSystem.VertexBuffers.PushVertex(Buffer, (void*)&Vertex, sizeof(TY_V));
		FK_ASSERT(res, "Failed to Push Vertex!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_V>
	bool PushVertex(const TY_V& Vertex, VertexBufferHandle Buffer, FrameResources& Resources, size_t PushSize)
	{
		bool res = Resources.renderSystem.VertexBuffers.PushVertex(Buffer, (void*)&Vertex, PushSize);
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
		return Resources.renderSystem.VertexBuffers.GetCurrentVertexBufferOffset(Buffer) / sizeof(TY_V);
	}


	inline size_t BeginNewConstantBuffer(ConstantBufferHandle CB, FrameResources& Resources)
	{
		return Resources.renderSystem.ConstantBuffers.AlignNext(CB);
	}


	/************************************************************************************************/

	[[deprecated]]
	inline bool PushConstantBufferData(char* _ptr, size_t size, ConstantBufferHandle buffer, FrameResources& resources)
	{
		const auto res = resources.renderSystem.ConstantBuffers.Push(buffer, _ptr, size);
		FK_ASSERT(res, "Failed to Push Constants!");
		return res.has_value();
	}


	/************************************************************************************************/


	template<typename TY_CB>
	[[deprecated]] bool PushConstantBufferData(const TY_CB& constants, ConstantBufferHandle buffer, FrameResources& resources)
	{
		const auto res = resources.renderSystem.ConstantBuffers.Push(Buffer, (void*)&constants, sizeof(TY_CB));
		FK_ASSERT(res, "Failed to Push Constants!");
		return res.has_value();
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
		auto& Pass = frameGraph.AddNode<PassData>(
			PassData{},
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.Present(backBuffer);
			},
			[](const PassData& Data, const ResourceHandler& Resources, Context& ctx, iAllocator&)
			{
			});
	}


	void SetRenderTargets	(Context* Ctx, static_vector<FrameResourceHandle> RenderTargets, FrameResources& FG);
	void ClearVertexBuffer	(FrameGraph& FG, VertexBufferHandle PushBuffer);


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

	typedef Vector<ShapeDraw> DrawList;


	/************************************************************************************************/


	struct alignas(256) Constants
	{
		float4		Albedo;
		float4		Specular;
		float4x4	WT;
	};


	/************************************************************************************************/


	class ShapeProtoType
	{
	public:
		ShapeProtoType() {}
		virtual ~ShapeProtoType() {}
		ShapeProtoType(const ShapeProtoType& rhs) = delete;

		virtual void AddShapeDraw(
			DrawList&				        DrawList,
			ReserveVertexBufferFunction&    reserveCB,
			ReserveConstantBufferFunction&  reserveVB,
			FrameResources&			        Resources) = 0;
	};


	/************************************************************************************************/


	class ShapeList final : public ShapeProtoType
	{
	public:
		ShapeList(iAllocator* Memory = SystemAllocator) :
			Shapes{ Memory } {}

		~ShapeList()
		{
			Shapes.Release();
		}

		void AddShape(ShapeProtoType* Shape)
		{
			Shapes.push_back(Shape);
		}

	protected:
		void AddShapeDraw(
			DrawList&				        DrawList,
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        Resources) override
		{
			for (auto Shape : Shapes)
				Shape->AddShapeDraw(
					DrawList, 
					reserveVB,
					reserveCB,
					Resources);
		}

		FlexKit::Vector<ShapeProtoType*> Shapes;
	};


	/************************************************************************************************/


	class Range
	{
	public:
		Range begin() const 
		{
			return { 0, end_, step_ };
		}


		Range end() const
		{
			return { end_, end_, step_ };
		}


		size_t operator * ()
		{
			return itr * step_;
		}


		bool operator == (const Range& rhs) const 
		{
			return  (rhs.itr == itr) && (rhs.end_ == end_) && (rhs.step_ == step_);
		}

		bool operator != (const Range& rhs) const
		{
			return  !(*this == rhs);
		}


		Range operator ++ ()
		{
			itr++;
			return *this;
		}

		size_t front() const
		{
			return 0;
		}

		size_t       itr;
		const size_t end_;
		const size_t step_;
	};


	inline auto MakeRange(size_t begin, size_t end, size_t step = 1)
	{
		return Range{ begin, end, step };
	}


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
			DrawList&						DrawList, 
			ReserveVertexBufferFunction&	reserveVB,
			ReserveConstantBufferFunction&	reserveCB,
			FrameResources&					Resources) override
		{
			VBPushBuffer VBBuffer   = reserveVB(sizeof(ShapeVert) * 3 * Divisions);

			const float Step = 2.0f * (float)pi / Divisions;
			const auto range = MakeRange(0, Divisions);

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				range,
				[&](size_t I, auto& pushBuffer) -> int
				{
					const float2 V1 = { POS.x + R * cos(Step * (I + 1)),	POS.y - AspectRatio * (R * sin(Step * (I + 1))) };
					const float2 V2 = { POS.x + R * cos(Step * I),			POS.y - AspectRatio * (R * sin(Step * I)) };

					pushBuffer.Push(ShapeVert{ Position2SS(POS),	float2{ 0.0f, 1.0f }, Color });
					pushBuffer.Push(ShapeVert{ Position2SS(V1),		float2{ 0.0f, 1.0f }, Color });
					pushBuffer.Push(ShapeVert{ Position2SS(V2),		float2{ 1.0f, 0.0f }, Color });
				},
				VBBuffer };

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto constantBuffer = reserveCB(256);
			ConstantBufferDataSet constants{ CB_Data, constantBuffer };

			DrawList.push_back({ ShapeDraw::RenderMode::Triangle, constants, vertices, Divisions * 3});
		}

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
			DrawList&				        DrawList,
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        Resources) override
		{
			auto VBBuffer = reserveVB(sizeof(ShapeVert) * 2 * Lines.size());
			auto range = MakeRange(0, Lines.size());

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				range,
				[&](size_t I, auto& pushBuffer) -> ShapeVert
				{
					auto positionA = Lines[I].A;
					auto positionB = Lines[I].B;

					auto pointA = ShapeVert{ positionA, float2{ 0.0f, 1.0f }, float4(Lines[I].AColour, 0) };
					auto pointB = ShapeVert{ positionB, float2{ 0.0f, 1.0f }, float4(Lines[I].BColour, 0) };

					pushBuffer.Push(pointA);
					pushBuffer.Push(pointB);

					return {};
				},
				VBBuffer };

			const Constants CB_Data = {
				{ 1, 1, 1, 1 },
				{ 1, 1, 1, 1 },
				float4x4::Identity()
			};

			auto constantBuffer = reserveCB(AlignedSize<Constants>());
			ConstantBufferDataSet constants{ CB_Data, constantBuffer };
			DrawList.push_back({ ShapeDraw::RenderMode::Line, constants, vertices, 2 * Lines.size() });
		}


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
			DrawList&						DrawList,
			ReserveVertexBufferFunction&	reserveVB,
			ReserveConstantBufferFunction&	reserveCB,
			FrameResources&					Resources) override
		{
			auto VBBuffer   = reserveVB(sizeof(ShapeVert) * 2 * Lines.size());
			auto range      = MakeRange(0, Lines.size());

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				range,
				[&](size_t I, auto& pushBuffer) -> ShapeVert
				{
					auto positionA = Position2SS(Lines[I].A);
					auto positionB = Position2SS(Lines[I].B);

					auto pointA = ShapeVert{ positionA, float2{ 0.0f, 1.0f }, Lines[I].AColour };
					auto pointB = ShapeVert{ positionB, float2{ 0.0f, 1.0f }, Lines[I].BColour };

					pushBuffer.Push(pointA);
					pushBuffer.Push(pointB);

					return {};
				},
				VBBuffer };

			Constants CB_Data = {
				{ 1, 1, 1, 1 },
				{ 1, 1, 1, 1 },
				float4x4::Identity()
			};

			auto constantBuffer = reserveCB(256);
			ConstantBufferDataSet constants{ CB_Data, constantBuffer };
			DrawList.push_back({ ShapeDraw::RenderMode::Line, constants, vertices, 2 * Lines.size() });
		}


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
			DrawList&				        DrawList, 
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        Resources) override
		{
			float2 RectUpperLeft	= POS;
			float2 RectBottomRight	= POS + WH;
			float2 RectUpperRight	= { RectBottomRight.x,	RectUpperLeft.y };
			float2 RectBottomLeft	= { RectUpperLeft.x,	RectBottomRight.y };

			const ShapeVert verticeData[] = {
				ShapeVert{ Position2SS(RectUpperLeft),	 { 0.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectBottomRight), { 1.0f, 0.0f }, Color },
				ShapeVert{ Position2SS(RectBottomLeft),	 { 0.0f, 1.0f }, Color },

				ShapeVert{ Position2SS(RectUpperLeft),	 { 0.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectUpperRight),	 { 1.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectBottomRight), { 1.0f, 0.0f }, Color } };

			const Constants constantData = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto constantBuffer     = reserveCB(sizeof(Constants));
			auto vertexBuffer       = reserveVB(sizeof(ShapeVert) * 6);

			DrawList.emplace_back(
					ShapeDraw::RenderMode::Triangle,
					ConstantBufferDataSet{ constantData, constantBuffer },
					VertexBufferDataSet{ verticeData, 6, vertexBuffer },
					6u);
		}

		float2 POS;
		float2 WH;
		float4 Color;
	};


	/************************************************************************************************/


	class SolidRectangleListShape final : public ShapeProtoType
	{
	public:
		SolidRectangleListShape(Vector<FlexKit::Rectangle>&& rects_in) :
			rects	{ std::move(rects_in) }{}

		
		~SolidRectangleListShape() {}

		void AddShapeDraw(
			DrawList&				        drawList, 
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        resources) override
		{
			Constants CB_Data = {
			float4(1, 1, 1, 1),
			float4(1, 1, 1, 1),
			float4x4::Identity() };

			auto constantBuffer = reserveCB(sizeof(CB_Data));
			auto constants      = ConstantBufferDataSet{CB_Data, constantBuffer };
			auto vertexBuffer   = reserveVB(sizeof(CB_Data) * 6 * rects.size());

			auto vertices = VertexBufferDataSet(
				SET_TRANSFORM_OP, rects,
				[](auto& rect, auto& buffer)
				{
					float2 rectUpperLeft	= rect.Position;
					float2 rectBottomRight	= rect.Position + rect.WH;
					float2 rectUpperRight	= { rectBottomRight.x,	rectUpperLeft.y };
					float2 rectBottomLeft	= { rectUpperLeft.x,	rectBottomRight.y };

					buffer.Push(ShapeVert{ Position2SS(rectUpperLeft),	    { 0.0f, 1.0f }, rect.Color });
					buffer.Push(ShapeVert{ Position2SS(rectBottomRight),	{ 1.0f, 0.0f }, rect.Color });
					buffer.Push(ShapeVert{ Position2SS(rectBottomLeft),	    { 0.0f, 1.0f }, rect.Color });

					buffer.Push(ShapeVert{ Position2SS(rectUpperLeft),	    { 0.0f, 1.0f }, rect.Color });
					buffer.Push(ShapeVert{ Position2SS(rectUpperRight),	    { 1.0f, 1.0f }, rect.Color });
					buffer.Push(ShapeVert{ Position2SS(rectBottomRight),	{ 1.0f, 0.0f }, rect.Color });

					return ShapeVert{};
				}, vertexBuffer);

			drawList.push_back({ ShapeDraw::RenderMode::Triangle, constants, vertices, 6 * rects.size() });
		}

		Vector<FlexKit::Rectangle> rects;
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
			DrawList&				            drawList,
			ReserveVertexBufferFunction&        reserveCB,
			ReserveConstantBufferFunction&      reserveVB,
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
		ReserveVertexBufferFunction     reserveVB,
		ReserveConstantBufferFunction   reserveCB,
		ResourceHandle                  renderTarget, 
		iAllocator*                     allocator, 
		TY_OTHER ...                    args)
	{
		struct ShapeParams
		{
			ReserveVertexBufferFunction     reserveVB;
			ReserveConstantBufferFunction   reserveCB;
			PSOHandle				        state;
			DrawList				        draws;

			FrameResourceHandle		        renderTarget;
		};

		auto& Pass = frameGraph.AddNode<ShapeParams>(
			ShapeParams{
				reserveVB,
				reserveCB,
				state,
				DrawList{ allocator },
			},
			[&](FrameGraphNodeBuilder& Builder, ShapeParams& data)
			{
				// Single Thread Section
				// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
				// for thread safety
				data.renderTarget	= Builder.RenderTarget(renderTarget);

				(args.AddShapeDraw(data.draws, reserveVB, reserveCB, frameGraph.resources), ...);
			},
			[=](const ShapeParams& data, const ResourceHandler& frameResources, Context& context, iAllocator& allocator)
			{	// Multi-threadable Section
				auto WH = frameResources.GetTextureWH(data.renderTarget);

				context.SetScissorAndViewports({ frameResources.GetResource(data.renderTarget)} );
				context.SetRenderTargets(
					{ frameResources.GetResource(data.renderTarget) },
					false);

				context.SetRootSignature		(frameResources.renderSystem().Library.RS6CBVs4SRVs);
				context.SetPipelineState		(frameResources.GetPipelineState(data.state));
				context.SetPrimitiveTopology	(EInputTopology::EIT_TRIANGLE);

				size_t TextureDrawCount = 0;
				ShapeDraw::RenderMode PreviousMode = ShapeDraw::RenderMode::Triangle;
				for (auto D : data.draws)
				{

					switch (D.Mode) {
						case ShapeDraw::RenderMode::Line:
						{
							context.SetPrimitiveTopology(EInputTopology::EIT_LINE);
						}	break;
						case ShapeDraw::RenderMode::Triangle:
						{
							context.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
						}	break;
						case ShapeDraw::RenderMode::Textured:
						{
							context.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);

							DescriptorHeap descHeap;
							auto& desciptorTableLayout = frameResources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0);

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
			[=, setup = std::move(setupPipeline)](_DrawCollection& data, const FrameResources& resources, Context* ctx)
			{
				data.fetchConstants(data);
				auto entities = data.fetchInstances();

				for (auto& entity : entities)
					data.instanceBuffer.Push(formatInstanceData(entity));


				auto* triMesh			= GetMeshResource(data.mesh);
				size_t MeshVertexCount	= triMesh->IndexCount;

				setup(data, resources, ctx);

				ctx->SetScissorAndViewports({ resources.GetResource(data.renderTarget) });
				ctx->SetRenderTargets(
					{	resources.GetResource(data.renderTarget) }, true,
						resources.GetResource(data.depthBuffer));

				// Bind resources
				VertexBufferList instancedBuffers;
				instancedBuffers.push_back(VertexBufferEntry{
					data.instanceBuffer,
					(UINT)data.instanceElementSize,
					(UINT)data.instanceBuffer.begin() });


				ctx->AddIndexBuffer(triMesh);
				ctx->AddVertexBuffers(triMesh,
					{	VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,
						VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,    },
					&instancedBuffers);


				for (auto& CBEntry : data.constants) 
					ctx->SetGraphicsConstantBufferView(
						1u + CBEntry.idx, 
						data.constantBuffer, 
						data.constantBuffer.Push(CBEntry.buffer, CBEntry.bufferSize));

				for (auto& constantBuffer : data.constantBuffers)
					ctx->SetGraphicsConstantBufferView(1u + constantBuffer.idx, data.constantBuffer, constantBuffer.offset);

				ctx->DrawIndexedInstanced(MeshVertexCount, 0, 0, entities.size());
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


	inline void WireframeRectangleList(
		FrameGraph&						frameGraph,
		DrawWireframeRectangle_Desc&	desc,
		Vector<Rectangle>&				rects,
		iAllocator*						TempMem
	)
	{
		struct DrawWireframes
		{
			FrameResourceHandle		RenderTarget;

			ConstantBufferHandle	CB;
			VertexBufferHandle		VB;

			ConstantBufferDataSet   cameraConstants;
			ConstantBufferDataSet   constants;

			VertexBufferDataSet     vertexBuffer;
			uint32_t                vertexCount;

			PSOHandle PSO;	
		};
	

		struct Vertex
		{
			float4 POS;
			float4 Color;
			float2 UV;
		};

		frameGraph.AddNode<DrawWireframes>(
			DrawWireframes{},
			[&](FrameGraphNodeBuilder& Builder, DrawWireframes& Data)
			{
				Data.RenderTarget	= Builder.RenderTarget(desc.RenderTarget);

				Data.CB		= desc.constantBuffer;
				Data.VB		= desc.VertexBuffer;
				Data.PSO	= desc.PSO;

				struct LocalConstants// Register b1
				{
					float4	 Color; // + roughness
					float4	 Padding;
					float4x4 WT;
				}locals{
					{1, 1, 1, 1},
					{1, 1, 1, 1},
					float4x4::Identity()
				};

				CBPushBuffer constantBuffer{ desc.constantBuffer, 1000, frameGraph.GetRenderSystem() };
				VBPushBuffer vertexBuffer{ desc.VertexBuffer, sizeof(Vertex) * 8 * rects.size(), frameGraph.GetRenderSystem() };

				auto cameraBuffer	= GetCameraConstants(desc.camera);
				auto pushBuffer		= desc.VertexBuffer;

				Data.constants          = ConstantBufferDataSet(locals, constantBuffer);
				Data.cameraConstants    = ConstantBufferDataSet(cameraBuffer, constantBuffer);


				FK_ASSERT(0, "ERROR");

				/*
				VertexBufferDataSet vertices{
					SET_TRANSFORM_OP,
					MakeRange(0, rects.size()),
					[&](size_t I, auto& pushBuffer) -> ShapeVert
					{
						const auto rect             = rects[I];
						const float4 upperLeft	    = { rect.Position.x, 0, rect.Position.y,		1};
						const float4 bottomRight	=   upperLeft + float4{rect.WH.x, 0, rect.WH.y, 0};
						const float4 upperRight	    = { bottomRight.x,	0, upperLeft.z,				1};
						const float4 bottomLeft	    = { upperLeft.x,	0, bottomRight.z,			1};

						pushBuffer.Push(Vertex{ upperLeft, rect.Color, { 0.0f, 1.0f } });
						pushBuffer.Push(Vertex{ upperRight,	rect.Color, { 1.0f, 1.0f } });

						// Right
						pushBuffer.Push(Vertex{ upperRight,	rect.Color, { 1.0f, 1.0f } });
						pushBuffer.Push(Vertex{ bottomRight, rect.Color, { 1.0f, 0.0f } });

						// Bottom
						pushBuffer.Push(Vertex{ bottomRight, rect.Color, { 0.0f, 0.0f } });
						pushBuffer.Push(Vertex{ bottomLeft,	rect.Color, { 1.0f, 0.0f } });

						// Left
						pushBuffer.Push(Vertex{ bottomLeft,	rect.Color, { 0.0f, 0.0f } });
						pushBuffer.Push(Vertex{ upperLeft,  rect.Color, { 0.0f, 1.0f } });

						return {};
					},
					vertexBuffer };

				Data.vertexCount = (uint32_t)rects.size() * 8;
				*/
			},
			[](auto& Data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				DescriptorHeap descHeap;
				descHeap.Init(
					ctx,
					resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
					&allocator);
				descHeap.NullFill(ctx);

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(Data.PSO));
				ctx.SetVertexBuffers({ Data.vertexBuffer });

				ctx.SetRenderTargets(
					{ resources.GetResource(Data.RenderTarget) }, false);

				ctx.SetPrimitiveTopology(EInputTopology::EIT_LINE);
				ctx.SetGraphicsDescriptorTable		(0, descHeap);
				ctx.SetGraphicsConstantBufferView	(1, Data.cameraConstants);
				ctx.SetGraphicsConstantBufferView	(2, Data.constants);

				ctx.NullGraphicsConstantBufferView	(4);
				ctx.NullGraphicsConstantBufferView	(5);
				ctx.NullGraphicsConstantBufferView	(6);

				ctx.Draw(Data.vertexCount, 0);
			});
	}


	/************************************************************************************************/

	// Requires a registered DRAW_LINE3D_PSO pipeline state!
	inline void Draw3DGrid(
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
		iAllocator*				TempMem)
	{
		LineSegments Lines(TempMem);
		Lines.reserve(ColumnCount + RowCount);

		const auto RStep = 1.0f / RowCount;

		//  Vertical Lines on ground
		for (size_t I = 1; I < RowCount; ++I)
			Lines.emplace_back(
				float3{ RStep  * I * GridWH.x, 0, 0 },
				GridColor.xyz(),
				float3{ RStep  * I * GridWH.x, 0, GridWH.y },
				GridColor.xyz());


		// Horizontal lines on ground
		const auto CStep = 1.0f / ColumnCount;
		for (size_t I = 1; I < ColumnCount; ++I)
			Lines.emplace_back(
				float3{ 0,			0, CStep  * I * GridWH.y },
				GridColor.xyz(),
				float3{ GridWH.x,   0, CStep  * I * GridWH.y },
				GridColor.xyz());


		struct DrawGrid
		{
			FrameResourceHandle		RenderTarget;
			FrameResourceHandle		DepthBuffer;

			size_t					VertexBufferOffset;
			size_t					VertexCount;

			VertexBufferDataSet		vertexBuffer;
			ConstantBufferHandle	CB;

			ConstantBufferDataSet   cameraConstants;
			ConstantBufferDataSet   passConstants;
		};


		struct VertexLayout
		{
			float4 POS;
			float4 Color;
			float2 UV;
		};

		frameGraph.AddNode<DrawGrid>(
			DrawGrid{},
			[&](FrameGraphNodeBuilder& builder, auto& Data)
			{

				Data.RenderTarget	        = builder.RenderTarget(RenderTarget);
				Data.DepthBuffer	        = builder.DepthTarget(DepthBuffer);
				Data.CB				        = constants;


				Brush::VConstantsLayout brushConstants{	
					.MP         = Brush::MaterialProperties{},
					.Transform  = float4x4::Identity()
				};

				CBPushBuffer cbPushBuffer(
					constants,
					AlignedSize<Brush::VConstantsLayout>() + AlignedSize<Camera::ConstantBuffer>(),
					frameGraph.GetRenderSystem());

				Data.passConstants      = ConstantBufferDataSet(constants, cbPushBuffer);
				Data.cameraConstants    = ConstantBufferDataSet(GetCameraConstants(Camera), cbPushBuffer);

				VBPushBuffer vbPushBuffer(
					vertexBuffer,
					sizeof(VertexLayout) * Lines.size() * 2,
					frameGraph.GetRenderSystem());

				auto range = MakeRange(0, Lines.size());
				VertexBufferDataSet vertices{
					SET_TRANSFORM_OP,
					range,
					[&](size_t I, auto& pushBuffer) -> VertexLayout
					{
						const LineSegment& lineSegment = Lines[I];

						VertexLayout Vertex;
						Vertex.POS		= float4(lineSegment.A, 1);
						Vertex.Color	= float4(lineSegment.AColour, 1) * float4 { 1.0f, 0.0f, 0.0f, 1.0f };
						Vertex.UV		= { 0.0f, 0.0f };

						pushBuffer.Push(Vertex);

						Vertex.POS		= float4(lineSegment.B, 1);
						Vertex.Color	= float4(lineSegment.BColour, 1) * float4 { 0.0f, 1.0f, 0.0f, 1.0f };
						Vertex.UV		= { 1.0f, 1.0f };

						pushBuffer.Push(Vertex);

						return {};
					},
					vbPushBuffer };

				Data.vertexBuffer   = vertices;
				Data.VertexCount    = Lines.size() * 2;
			},
			[](auto& Data, const ResourceHandler& resources, Context& ctx, iAllocator& allocator)
			{
				DescriptorHeap descHeap;
				descHeap.Init(
					ctx,
					resources.renderSystem().Library.RS6CBVs4SRVs.GetDescHeap(0),
					&allocator);
				descHeap.NullFill(ctx);

				ctx.SetRootSignature(resources.renderSystem().Library.RS6CBVs4SRVs);
				ctx.SetPipelineState(resources.GetPipelineState(DRAW_LINE3D_PSO));

				ctx.SetScissorAndViewports({ resources.GetResource(Data.RenderTarget) });
				ctx.SetRenderTargets(
					{	resources.GetResource(Data.RenderTarget) }, false,
						resources.GetResource(Data.DepthBuffer));

				ctx.SetPrimitiveTopology(EInputTopology::EIT_LINE);
				ctx.SetVertexBuffers({ Data.vertexBuffer });

				ctx.SetGraphicsDescriptorTable(0, descHeap);
				ctx.SetGraphicsConstantBufferView(1, Data.cameraConstants);
				ctx.SetGraphicsConstantBufferView(2, Data.passConstants);

				ctx.NullGraphicsConstantBufferView(3);
				ctx.NullGraphicsConstantBufferView(4);
				ctx.NullGraphicsConstantBufferView(5);
				ctx.NullGraphicsConstantBufferView(6);

				ctx.Draw(Data.VertexCount, 0);
			});
	}


	inline auto& ClearIntegerRenderTarget_RG32(
		FrameGraph&                     frameGraph,
		ResourceHandle                  target,
		ReserveConstantBufferFunction&  reserveCB,
		uint2                           value = { 0, 0 })
	{
		struct _Clear
		{
			ReserveConstantBufferFunction   ReserveCB;
			FrameResourceHandle             feedbackTarget;
		};

		return frameGraph.AddNode<_Clear>(
			_Clear{ reserveCB },
			[&](FrameGraphNodeBuilder& builder, _Clear& data)
			{
				data.feedbackTarget = builder.RenderTarget(target);
			},
			[](const _Clear& data, ResourceHandler& resources, Context& ctx, iAllocator&)
			{
				/*
				struct _Constants
				{
					uint4 constants = {};
				}constants{ { value[0], value[1], value[0], value[1] } };

				ctx.SetRootSignature(resources.renderSystem().Library.RSDefault);
				ctx.SetPipelineState(resources.GetPipelineState(CLEARRENDERTARGET_RG32));

				ctx.SetScissorRects();
				ctx.SetRenderTargets();
				*/
				ctx.SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
				ctx.Draw(6);
			});
	}


}	/************************************************************************************************/
#endif
