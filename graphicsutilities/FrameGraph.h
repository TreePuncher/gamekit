/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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
		OT_ShaderResource,
		OT_VertexBuffer,
		OT_IndirectArguments,
		OT_UAVBuffer,
		OT_UAVTexture,
		OT_Virtual,
	};


	/************************************************************************************************/
	typedef Handle_t<16> FrameResourceHandle;
	typedef Handle_t<16> TemporaryFrameResourceHandle;


	class FrameGraphNode;

	enum class virtualResourceState
	{
		NonVirtual,
		Virtual_Null,
		Virtual_Created,
		Virtual_Released,
	};

	struct FrameObject
	{
		FrameObject() :
			Handle{ (uint32_t)INVALIDHANDLE }{}


		FrameObject(const FrameObject& rhs) :
			Type	{ rhs.Type		},
			State	{ rhs.State		},
			Handle	{ rhs.Handle	}
		{
			Buffer = rhs.Buffer;
		}

		FrameResourceHandle		Handle; // For Fast Search
		FrameObjectResourceType Type;
		DeviceResourceState		State;
		FrameGraphNode*         lastModifier = nullptr;
		virtualResourceState    virtualState = virtualResourceState::NonVirtual;

		union
		{
			UAVTextureHandle	UAVTexture;
			ResourceHandle      shaderResource;

			QueryHandle			query;
			SOResourceHandle	SOBuffer;
			UAVResourceHandle	UAVBuffer;

			struct {
				char	buff[256];
			}Buffer;
		};


		static FrameObject ReadRenderTargetObject(ResourceHandle handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State			= DeviceResourceState::DRS_ShaderResource;
			RenderTarget.Type			= OT_RenderTarget;
			RenderTarget.shaderResource = handle;

			return RenderTarget;
		}


		static FrameObject WriteRenderTargetObject(ResourceHandle handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State		    = DeviceResourceState::DRS_RenderTarget;
			RenderTarget.Type		    = OT_RenderTarget;
			RenderTarget.shaderResource = handle;

			return RenderTarget;
		}


		static FrameObject BackBufferObject(ResourceHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			FrameObject RenderTarget;
			RenderTarget.State			= InitialState;
			RenderTarget.Type			= OT_BackBuffer;
			RenderTarget.shaderResource = handle;

			return RenderTarget;
		}


		static FrameObject DepthBufferObject(ResourceHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
		{
			FrameObject RenderTarget;
			RenderTarget.State          = InitialState;
			RenderTarget.Type           = OT_DepthBuffer;
			RenderTarget.shaderResource = handle;

			return RenderTarget;
		}


		static FrameObject TextureObject(ResourceHandle handle, DeviceResourceState InitialState)
		{
			FrameObject shaderResource;
			shaderResource.State            = InitialState;
			shaderResource.Type             = OT_ShaderResource;
			shaderResource.shaderResource   = handle;

			return shaderResource;
		}


		static FrameObject UAVBufferObject(UAVResourceHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
		{
			FrameObject UnorderedAccessViewObject;
			UnorderedAccessViewObject.State                = InitialState;
			UnorderedAccessViewObject.Type                 = OT_UAVBuffer;
			UnorderedAccessViewObject.UAVBuffer			   = handle;

			return UnorderedAccessViewObject;
		}


		static FrameObject UAVTextureObject(UAVTextureHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
		{
			FrameObject UnorderedAccessViewObject;
			UnorderedAccessViewObject.State         = InitialState;
			UnorderedAccessViewObject.Type          = OT_UAVTexture;
			UnorderedAccessViewObject.UAVTexture    = handle;

			return UnorderedAccessViewObject;
		}


		static FrameObject SOBufferObject(SOResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_GENERIC)
		{
			FrameObject Streamout;
			Streamout.State                = InitialState;
			Streamout.Type                 = OT_StreamOut;
			Streamout.SOBuffer			   = Handle;

			return Streamout;
		}


		static FrameObject QueryObject(QueryHandle handle, DeviceResourceState initialState = DeviceResourceState::DRS_GENERIC)
		{
			FrameObject query;
			query.State		= initialState;
			query.Type		= OT_Query;
			query.query		= handle;

			return query;
		}


		// Virtual Objects are objects that are not yet backed by any GPU resource
		// Can be used as placeholder handles for temporaries that are not yet created
		// after creation, they will function as a normal frame object, but at the end of the frame,
		// they are destroyed. All virtual objects well be either placed or tiled resources and be GPU resources.
		static FrameObject VirtualObject()
		{
			FrameObject virtualObject;
			virtualObject.virtualState  = virtualResourceState::Virtual_Null;
			virtualObject.State         = DeviceResourceState::DRS_UNKNOWN;
			virtualObject.Type		    = OT_Virtual;

			return virtualObject;
		}
	};


	/************************************************************************************************/


	struct LocallyTrackedResource
	{
		FrameResourceHandle resource;
		DeviceResourceState currentState;
		DeviceResourceState nodeState;
	};

	typedef Vector<FrameObject>		        PassObjectList;
	typedef Vector<LocallyTrackedResource>	LocallyTrackedObjectList;
	typedef Vector<FrameResourceHandle>	    TemporaryPassObjectList;

	class FrameGraph;


	/************************************************************************************************/


	FLEXKITAPI class FrameResources
	{
	public:
		FrameResources(RenderSystem& IN_renderSystem, iAllocator* IN_allocator) : 
			Resources		    { IN_allocator		},
			virtualResources    { IN_allocator      },
			renderSystem	    { IN_renderSystem	},
			memoryPools         { IN_allocator      },
			allocator           { IN_allocator      } {}

		PassObjectList			        Resources;
		TemporaryPassObjectList         virtualResources;
		Vector<PoolAllocatorInterface*> memoryPools;


		void AddMemoryPool(PoolAllocatorInterface* heapAllocator)
		{
			memoryPools.push_back(heapAllocator);
		}


		RenderSystem&			renderSystem;
		iAllocator*             allocator;

		/************************************************************************************************/


		void AddBackBuffer(ResourceHandle Handle)
		{
			AddRenderTarget(
				Handle,
				renderSystem.GetObjectState(Handle));
		}


		/************************************************************************************************/


		void AddRenderTarget(ResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			Resources.push_back(
				FrameObject::BackBufferObject(Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddDepthBuffer(ResourceHandle Handle)
		{
			AddDepthBuffer(
				Handle,
				renderSystem.GetObjectState(Handle));
		}


		/************************************************************************************************/


		void AddDepthBuffer(ResourceHandle Handle, DeviceResourceState InitialState)
		{
			Resources.push_back(
				FrameObject::DepthBufferObject(Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddUAVResource(UAVResourceHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_Write)
		{
			Resources.push_back(
				FrameObject::UAVBufferObject(handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}

		void AddUAVResource(UAVTextureHandle handle, DeviceResourceState InitialState = DeviceResourceState::DRS_Write)
		{
			Resources.push_back(
				FrameObject::UAVTextureObject(handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddSOResource(SOResourceHandle handle)
		{
			DeviceResourceState initialState = renderSystem.GetObjectState(handle);

			Resources.push_back(
				FrameObject::SOBufferObject(handle, initialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddShaderResource(ResourceHandle handle, const bool renderTarget = false)
		{
			DeviceResourceState initialState = renderSystem.GetObjectState(handle);

			Resources.push_back(
				FrameObject::TextureObject(handle, initialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddQuery(QueryHandle handle)
		{
			DeviceResourceState initialState = renderSystem.GetObjectState(handle);

			Resources.push_back(
				FrameObject::QueryObject(handle, initialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		template<typename TY>
		ID3D12Resource* GetObjectResource(TY handle) const
		{
			return renderSystem.GetDeviceResource(handle);
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


		DeviceResourceState GetAssetObjectState(FrameResourceHandle Handle)
		{
			return Resources[Handle].State;
		}


		/************************************************************************************************/


		FrameObject* GetAssetObject(FrameResourceHandle Handle)
		{
			return &Resources[Handle];
		}


		/************************************************************************************************/


		ResourceHandle GetRenderTarget(FrameResourceHandle Handle) const
		{
			return Resources[Handle].shaderResource;
		}


		/************************************************************************************************/


		ResourceHandle GetTexture(FrameResourceHandle Handle) const
		{
			return Resources[Handle].shaderResource;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(ResourceHandle Handle)
		{
			auto res = find(Resources, 
				[&](const FrameObject& LHS)
				{
					auto CorrectType = (
						LHS.Type == OT_ShaderResource	||
						LHS.Type == OT_RenderTarget		||
						LHS.Type == OT_DepthBuffer      ||
						LHS.Type == OT_BackBuffer);

					return (CorrectType && LHS.shaderResource == Handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(ShaderResourceHandle Handle)
		{
			auto res = find(Resources,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == OT_ShaderResource;

					return (CorrectType && LHS.shaderResource == Handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(UAVResourceHandle handle)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == OT_UAVBuffer;

					return (CorrectType && LHS.UAVBuffer == handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(UAVTextureHandle handle)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == OT_UAVTexture;

					return (CorrectType && LHS.UAVTexture == handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(SOResourceHandle handle)
		{
			auto res = find(Resources,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == OT_StreamOut;

					return (CorrectType && LHS.SOBuffer == handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(QueryHandle handle)
		{
			auto res = find(Resources,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == FrameObjectResourceType::OT_Query;

					return (CorrectType && LHS.query == handle);
				});

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(FrameResourceHandle handle)
		{
			auto res = find(Resources, [&](const FrameObject& LHS) { return LHS.Handle == handle; });

			if (res != Resources.end())
				return res->Handle;

			return InvalidHandle_t;
		}


		operator RenderSystem& ()
		{
			return *renderSystem;
		}

		uint2 GetTextureWH(FrameResourceHandle handle) const
		{
			if (auto res = GetTexture(handle); res != InvalidHandle_t)
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
		ID3D12Resource*         GetObjectResource(TY handle) const  { return globalResources.GetObjectResource(handle); }
		ID3D12PipelineState*    GetPipelineState(PSOHandle state)	const { return globalResources.GetPipelineState(state); }

		size_t                  GetVertexBufferOffset(VertexBufferHandle handle, size_t vertexSize) { return globalResources.GetVertexBufferOffset(handle, vertexSize); }
		size_t                  GetVertexBufferOffset(VertexBufferHandle handle) { return globalResources.GetVertexBufferOffset(handle); }

		DeviceResourceState     GetAssetObjectState(FrameResourceHandle handle) { return globalResources.GetAssetObjectState(handle); }
		FrameObject*            GetAssetObject(FrameResourceHandle handle) { return globalResources.GetAssetObject(handle); }

		ResourceHandle          GetRenderTarget(FrameResourceHandle handle) const { return globalResources.GetRenderTarget(handle); }
		ResourceHandle          GetTexture(FrameResourceHandle handle) const { return globalResources.GetTexture(handle); }


		/************************************************************************************************/


		ID3D12Resource* GetUAVDeviceResource(FrameResourceHandle handle) const
		{
			return GetObjectResource(globalResources.Resources[handle].UAVBuffer);
		}


		UAVTextureHandle GetUAVTextureResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const LocallyTrackedResource& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle && rhs.Type == OT_UAVTexture;
					});

				FK_ASSERT(res != globalResources.Resources.end());
				SubNodeTracking.push_back({ res->Handle, res->State });

				return res->UAVTexture;
			}
			else
				return globalResources.Resources[res->resource].UAVTexture;
		}


		UAVResourceHandle GetUAVBufferResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const auto& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle && rhs.Type == OT_UAVBuffer;
					});

				FK_ASSERT(res != globalResources.Resources.end());
				SubNodeTracking.push_back({ res->Handle, res->State });

				return res->UAVBuffer;
			}
			else
				return globalResources.Resources[res->resource].UAVBuffer;
		}

		SOResourceHandle GetSOResource(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const auto& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle;
					});

				FK_ASSERT(res != globalResources.Resources.end());
				SubNodeTracking.push_back({ res->Handle, res->State });

				return res->SOBuffer;
			}
			else
				return globalResources.Resources[res->resource].SOBuffer;
		}

		D3D12_VERTEX_BUFFER_VIEW ReadStreamOut(FrameResourceHandle handle, Context* ctx, size_t vertexSize) const
		{
			/*
			typedef struct D3D12_VERTEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			UINT StrideInBytes;
			} 	D3D12_VERTEX_BUFFER_VIEW;
			*/

			auto& res			= _FindSubNodeResource(handle);
			auto SOHandle		= globalResources.Resources[res.resource].SOBuffer;
			auto deviceResource = renderSystem().GetDeviceResource(SOHandle);

			if (res.currentState != DRS_VERTEXBUFFER) 
				ctx->AddStreamOutBarrier(SOHandle, res.currentState, DRS_VERTEXBUFFER);

			res.currentState = DRS_VERTEXBUFFER;

			D3D12_VERTEX_BUFFER_VIEW view = {
				deviceResource->GetGPUVirtualAddress(),
				static_cast<UINT>(renderSystem().GetStreamOutBufferSize(SOHandle)),
				static_cast<UINT>(vertexSize)
			};

			return view;
		}

		ResourceHandle ReadRenderTarget(FrameResourceHandle resource, Context* ctx) const
		{
			auto state          = GetObjectState(resource);
			auto renderTarget   = globalResources.GetRenderTarget(resource);

			if (state != DRS_ShaderResource)
			{
				ctx->AddShaderResourceBarrier(renderTarget, state, DRS_ShaderResource);
				_FindSubNodeResource(resource).currentState = DRS_ShaderResource;
			}

			return renderTarget;
		}

		UAVTextureHandle ReadWriteUAVTexture(FrameResourceHandle resource, Context* ctx) const
		{
			auto state  = GetObjectState(resource);
			auto UAV2D  = globalResources.Resources[resource].UAVTexture;

			if (state != DRS_UAV)
			{
				ctx->AddUAVBarrier(UAV2D, state, DRS_UAV);
				_FindSubNodeResource(resource).currentState = DRS_ShaderResource;
			}

			return UAV2D;
		}

		UAVResourceHandle ReadWriteUAVBuffer(FrameResourceHandle resource, Context* ctx) const
		{
			auto state = GetObjectState(resource);
			auto UAVBuffer = globalResources.Resources[resource].UAVBuffer;

			if (state != DRS_UAV)
			{
				ctx->AddUAVBarrier(UAVBuffer, state, DRS_UAV);
				_FindSubNodeResource(resource).currentState = DRS_ShaderResource;
			}

			return UAVBuffer;
		}

		UAVResourceHandle ReadUAVBuffer(FrameResourceHandle resource, DeviceResourceState state, Context& ctx) const
		{
			auto currentState = GetObjectState(resource);
			auto UAVBuffer = globalResources.Resources[resource].UAVBuffer;

			if (state != currentState)
			{
				ctx.AddUAVBarrier(UAVBuffer, currentState, state);
				_FindSubNodeResource(resource).currentState = state;
			}

			return UAVBuffer;
		}

		UAVResourceHandle ReadIndirectArgs(FrameResourceHandle resource, Context& ctx) const
		{
			return ReadUAVBuffer(resource, DRS_INDIRECTARGS, ctx);
		}

		UAVResourceHandle WriteUAV(FrameResourceHandle resource, Context& ctx) const
		{
			return ReadUAVBuffer(resource, DRS_Write, ctx);
		}

		ResourceHandle CopyToTexture(FrameResourceHandle resource, Context& ctx)
		{
			auto state = GetObjectState(resource);
			auto renderTarget = globalResources.GetRenderTarget(resource);

			if (state != DRS_Write)
			{
				ctx.AddShaderResourceBarrier(renderTarget, state, DRS_Write);
				_FindSubNodeResource(resource).currentState = DRS_Write;
			}

			return renderTarget;
		}

		UAVTextureHandle CopyUAVTexture(FrameResourceHandle resource, Context& ctx)
		{
			auto state = GetObjectState(resource);
			auto UAV2D = globalResources.Resources[resource].UAVTexture;

			if (state != DRS_Read)
			{
				ctx.AddUAVBarrier(UAV2D, state, DRS_Read);
				_FindSubNodeResource(resource).currentState = DRS_Read;
			}

			return UAV2D;
		}

		UAVResourceHandle CopyToUAV(FrameResourceHandle resource, Context& ctx)
		{
			auto state = GetObjectState(resource);
			auto UAV = globalResources.Resources[resource].UAVBuffer;

			if (state != DRS_Write)
			{
				ctx.AddUAVBarrier(UAV, state, DRS_Write);
				_FindSubNodeResource(resource).currentState = DRS_Write;
			}

			return UAV;
		}

		DeviceResourceState GetObjectState(FrameResourceHandle handle) const
		{
			auto res = find(SubNodeTracking,
				[&](const auto& rhs) -> bool
				{
					return rhs.resource == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(globalResources.Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle;
					});

				FK_ASSERT(res != globalResources.Resources.end());
				SubNodeTracking.push_back({ res->Handle, res->State });

				return res->State;
			}
			else
				return res->currentState;
		}


		D3D12_STREAM_OUTPUT_BUFFER_VIEW WriteStreamOut(FrameResourceHandle handle, Context* ctx, size_t inputStride) const
		{
			auto& localResourceObj  = _FindSubNodeResource(handle);
			auto& resource          = globalResources.Resources[localResourceObj.resource];

			if (localResourceObj.currentState != DRS_STREAMOUT)
				ctx->AddStreamOutBarrier(resource.SOBuffer, localResourceObj.currentState, DRS_STREAMOUT);

			localResourceObj.currentState = DRS_STREAMOUT;

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


		SOResourceHandle ClearStreamOut(FrameResourceHandle handle, Context& ctx)
		{
			auto& resource = _FindSubNodeResource(handle);
			
			if (resource.currentState != DRS_STREAMOUTCLEAR)
				ctx.AddStreamOutBarrier(
					globalResources.Resources[resource.resource].SOBuffer,
					resource.currentState, DRS_STREAMOUTCLEAR);

			resource.currentState = DRS_STREAMOUTCLEAR;

			return GetSOResource(handle);
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

		LocallyTrackedObjectList&   SubNodeTracking;
		FrameResources&             globalResources;
	};

	/************************************************************************************************/


	class FrameGraphNode;

	struct FrameObjectLink
	{
		FrameObjectLink(
			FrameResourceHandle IN_handle           = InvalidHandle_t,
			FrameGraphNode*		IN_SourceObject		= nullptr,
			DeviceResourceState	IN_neededState		= DRS_UNKNOWN) :
				Source			{ IN_SourceObject },
				neededState		{ IN_neededState },
				handle          { IN_handle } {}


		FrameGraphNode*		Source;
		FrameResourceHandle handle;
		DeviceResourceState	neededState;
	};


	auto MakePred(FrameResourceHandle handle, const PassObjectList& resources)
	{
		return [handle](FrameObjectLink& lhs)
		{
			return lhs.handle == handle;
		};
	}

	auto MakePred(ResourceHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return resources[lhs.handle].shaderResource == handle;
		};
	}

	auto MakePred(UAVTextureHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return (resources[lhs.handle].UAVTexture == handle && resources[lhs.handle].Type == OT_UAVTexture);
		};
	}

	auto MakePred(UAVResourceHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return (resources[lhs.handle].UAVBuffer == handle && resources[lhs.handle].Type == OT_UAVBuffer);
		};
	}

	auto MakePred(SOResourceHandle handle, const PassObjectList& resources)
	{
		return [handle, &resources](FrameObjectLink& lhs)
		{
			return (resources[lhs.handle].SOBuffer == handle && resources[lhs.handle].Type == OT_StreamOut);
		};
	}

	
	/************************************************************************************************/


	class ResourceTransition
	{
	public:
		void ProcessTransition	(FrameResources& Resources, Context* Ctx) const;

		FrameResourceHandle Object      = InvalidHandle_t;
		DeviceResourceState	BeforeState = DRS_UNKNOWN;
		DeviceResourceState	AfterState  = DRS_UNKNOWN;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNode
	{
	public:
		typedef void (*FN_NodeAction)(FrameGraphNode& node, FrameResources& Resources, Context& ctx, iAllocator& tempAllocator);

		FrameGraphNode(FN_NodeAction IN_action, void* IN_nodeData, iAllocator* IN_allocator = nullptr) :
			InputObjects	{ IN_allocator		    },
			OutputObjects	{ IN_allocator		    },
			Sources			{ IN_allocator		    },
			Transitions		{ IN_allocator		    },
			NodeAction		{ IN_action             },
			nodeData        { IN_nodeData           },
			Executed		{ false				    },
			SubNodeTracking { IN_allocator          },
			RetiredObjects  { IN_allocator          }{}


		FrameGraphNode(const FrameGraphNode& RHS) :
			Sources			{ RHS.Sources		        },
			InputObjects	{ RHS.InputObjects	        },
			OutputObjects	{ RHS.OutputObjects	        },
			Transitions		{ RHS.Transitions	        },
			NodeAction		{ std::move(RHS.NodeAction)	},
			nodeData        { RHS.nodeData              },
			RetiredObjects  { RHS.RetiredObjects        },
			SubNodeTracking { RHS.SubNodeTracking       },
			Executed		{ false				        } {}


		~FrameGraphNode() = default;


		void HandleBarriers	(FrameResources& Resouces, Context& Ctx);
		void AddTransition	(ResourceTransition& Dep);

		void RestoreResourceStates  (Context* ctx, FrameResources& resources, LocallyTrackedObjectList& locallyTrackedObjects);
		void AcquireResources       (FrameResources& resources, Context& ctx);
		void ReleaseResources       (FrameResources& resources, Context& ctx);


		Vector<FrameGraphNode*>		GetNodeDependencies()	{ return (nullptr); } 

		void*                           nodeData;
		bool							Executed;
		FN_NodeAction					NodeAction;
		Vector<FrameGraphNode*>			Sources;// Nodes that this node reads from
		Vector<FrameObjectLink>	        InputObjects;
		Vector<FrameObjectLink>	        OutputObjects;
		Vector<FrameObjectLink>	        RetiredObjects;
		Vector<ResourceTransition>		Transitions;
		LocallyTrackedObjectList        SubNodeTracking;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphResourceContext
	{
	public:
		FrameGraphResourceContext(FrameResources& IN_resources, iAllocator* Temp) :
			resources   { IN_resources },
			Writables	{ Temp },
			Readables	{ Temp },
			Retirees	{ Temp }{}


		void AddWriteable(const FrameObjectLink& NewObject)
		{
			Writables.push_back(NewObject);
		}


		void AddReadable(const FrameObjectLink& NewObject)
		{
			Readables.push_back(NewObject);
		}


		template<typename TY>
		void RemoveWriteable(TY handle)
		{
			Writables.remove_unstable(find(Writables, MakePred(handle, resources.Resources)));
		}


		template<typename TY>
		void RemoveReadable(TY handle)
		{
			Readables.remove_unstable(find(Readables, MakePred(handle, resources.Resources)));
		}


		template<typename TY>
		FrameObjectLink& GetReadable(TY handle)
		{
			return *find(Readables, MakePred(handle, resources.Resources));
		}


		template<typename TY>
		FrameObjectLink& GetWriteable(TY handle)
		{
			return *find(Writables, MakePred(handle, resources.Resources));
		}


		Vector<FrameObjectLink> GetFinalStates()
		{
			Vector<FrameObjectLink> Objects(Writables.Allocator);
			Objects.reserve(Writables.size() + Readables.size() + Retirees.size());

			Objects += Writables;
			Objects += Readables;
			Objects += Retirees;

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
				return resources.FindFrameResource(handle);;
		}


		template<typename _pred>
		Pair<bool, FrameObjectLink&> _IsTrackedReadable(_pred pred)
		{
			auto Res = find(Readables, pred);
			return { Res != Readables.end(), *Res };
		}


		template<typename _pred>
		Pair<bool, FrameObjectLink&> _IsTrackedWriteable(_pred pred)
		{
			auto Res = find(Writables, pred);
			return { Res != Writables.end(), *Res };
		}


		template<typename TY>
		Pair<bool, FrameObjectLink&> IsTrackedReadable	(TY handle)
		{
			return _IsTrackedReadable(MakePred(handle, resources.Resources));
		}


		template<typename TY>
		Pair<bool, FrameObjectLink&> IsTrackedWriteable	(TY handle)
		{
			return _IsTrackedWriteable(MakePred(handle, resources.Resources));
		}


		Vector<FrameObjectLink>	Writables;
		Vector<FrameObjectLink>	Readables;
		Vector<FrameObjectLink>	Retirees;

		FrameResources&         resources;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			Vector<UpdateTask*>&	    IN_DataDependencies,
			FrameResources*				IN_Resources, 
			FrameGraphNode&				IN_Node,
			FrameGraphResourceContext&	IN_context,
			iAllocator*					IN_allocator) :
				DataDependencies{ IN_DataDependencies	},
				Context			{ IN_context			},
				inputNodes      { IN_allocator			},
				Node			{ IN_Node				},
				Resources		{ IN_Resources			},
				RetiredObjects  { IN_allocator          },
				Transitions		{ IN_allocator			}{}


		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;

		void BuildNode(FrameGraph* FrameGraph);


		void AddDataDependency(UpdateTask& task);

		FrameResourceHandle ReadShaderResource	(ResourceHandle Handle);
		FrameResourceHandle WriteShaderResource	(ResourceHandle Handle);

		FrameResourceHandle ReadRenderTarget	(ResourceHandle   Handle);
		FrameResourceHandle WriteRenderTarget	(ResourceHandle   Handle);
		FrameResourceHandle WriteRenderTarget   (UAVTextureHandle Handle);

		FrameResourceHandle	PresentBackBuffer	(ResourceHandle handle);
		FrameResourceHandle	ReadBackBuffer		(ResourceHandle handle);
		FrameResourceHandle	WriteBackBuffer		(ResourceHandle handle);

		FrameResourceHandle	WriteDepthBuffer	(ResourceHandle handle);

		FrameResourceHandle AcquireVirtualResource(const GPUResourceDesc desc, DeviceResourceState initialState);
		void                ReleaseVirtualResource(FrameResourceHandle handle);


		FrameResourceHandle	ReadWriteUAV    (UAVResourceHandle, DeviceResourceState state = DeviceResourceState::DRS_Write);
		FrameResourceHandle	ReadWriteUAV    (UAVTextureHandle,	DeviceResourceState	state = DeviceResourceState::DRS_Write);

		FrameResourceHandle	ReadSOBuffer	(SOResourceHandle);
		FrameResourceHandle	WriteSOBuffer	(SOResourceHandle);

		FrameResourceHandle ReadResource(FrameResourceHandle handle, DeviceResourceState state);
		FrameResourceHandle WriteResource(FrameResourceHandle handle, DeviceResourceState state);


		size_t							GetDescriptorTableSize			(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot
		const DesciptorHeapLayout<16>&	GetDescriptorTableLayout		(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot

		operator FrameResources&	() const	{ return *Resources; }
		operator RenderSystem&		()			{ return *Resources->renderSystem;}

	private:

		template<typename TY>
		FrameResourceHandle AddReadableResource(TY handle, DeviceResourceState state)
		{
			FrameResourceHandle frameResourceHandle = Context.GetFrameObject(handle);

			if (frameResourceHandle == InvalidHandle_t)
				return frameResourceHandle;

			FrameObject& frameObject = Context.resources.Resources[frameResourceHandle];

			if (frameObject.lastModifier)
				inputNodes.push_back(frameObject.lastModifier);

			if (frameObject.State != state)
			{
				if (frameObject.State & DRS_Write)
					Context.RemoveWriteable(frameResourceHandle);

				frameObject.lastModifier = &Node;

				FrameObjectLink dependency{ frameResourceHandle, &Node, state };
				Context.AddReadable(dependency);

				ResourceTransition transition = {
					.Object       = frameResourceHandle,
					.BeforeState  = frameObject.State,
					.AfterState   = state,
				};

				Transitions.push_back(transition);

				frameObject.State = state;
			}

			Node.SubNodeTracking.push_back({ frameResourceHandle, state });

			return frameResourceHandle;
		}


		template<typename TY>
		FrameResourceHandle AddWriteableResource(TY handle, DeviceResourceState state)
		{
			FrameResourceHandle frameResourceHandle = Context.GetFrameObject(handle);

			if (frameResourceHandle == InvalidHandle_t)
				return InvalidHandle_t;

			FrameObject& frameObject = Context.resources.Resources[frameResourceHandle];

			if (frameObject.lastModifier)
				inputNodes.push_back(frameObject.lastModifier);

			if (frameObject.State != state)
			{
				if (frameObject.State & DRS_Read)
					Context.RemoveReadable(frameResourceHandle);

				FrameObjectLink dependency{ frameResourceHandle, &Node, state };
				Context.AddWriteable(dependency);

				ResourceTransition transition = {
					.Object       = frameResourceHandle,
					.BeforeState  = frameObject.State,
					.AfterState   = state,
				};

				Transitions.push_back(transition);

				frameObject.State = state;
			}

			frameObject.lastModifier = &Node;

			Node.SubNodeTracking.push_back({ frameResourceHandle, state, state });

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
			Vector<FrameObjectLink>&  Set1,
			Vector<FrameObjectLink>&  Set2,
			FrameObjectLink&			Object);

		Vector<FrameGraphNode*>	    inputNodes;

		Vector<ResourceTransition>	Transitions;
		Vector<FrameObjectLink>	    RetiredObjects;
		Vector<UpdateTask*>&	    DataDependencies;

		FrameGraphResourceContext&		Context;
		FrameGraphNode&					Node;
		FrameResources*					Resources;
	};


	/************************************************************************************************/


	using DataDependencyList = Vector<UpdateTask*>;


	FLEXKITAPI class FrameGraph
	{
	public:
		FrameGraph(RenderSystem& RS, ThreadManager& IN_threads, iAllocator* Temp) :
			Resources		{ RS, Temp },
			threads         { IN_threads },
			dataDependencies{ Temp },
			ResourceContext	{ Resources, Temp },
			Memory			{ Temp },
			Nodes			{ Temp } {}

		FrameGraph				(const FrameGraph& RHS) = delete;
		FrameGraph& operator =	(const FrameGraph& RHS) = delete;


		template<typename TY, typename INITIAL_TY, typename SetupFN, typename DrawFN>
		TY& AddNode(INITIAL_TY&& initial, SetupFN&& Setup, DrawFN&& Draw)
		{
			struct NodeData
			{
				NodeData(INITIAL_TY&& IN_initial, DrawFN&& IN_DrawFN) :
					draw    { std::move(IN_DrawFN)  },
					fields  { std::move(IN_initial) } {}

				~NodeData() = default;

				TY      fields;
				DrawFN  draw;
			};

			auto& data  = Memory->allocate_aligned<NodeData>(std::move(std::forward<INITIAL_TY>(initial)), std::move(Draw));
			auto idx    = Nodes.emplace_back(
				FrameGraphNode{
					[](
					FrameGraphNode& node,
					FrameResources& resources,
					Context&        ctx,
					iAllocator&     tempAllocator)
					{
						NodeData& data = *reinterpret_cast<NodeData*>(node.nodeData);

						node.HandleBarriers(resources, ctx);

						ResourceHandler handler{ resources, node.SubNodeTracking };
						data.draw(data.fields, handler, ctx, tempAllocator);

						node.RestoreResourceStates(&ctx, resources, node.SubNodeTracking);
						data.fields.~TY();
					},
					&data,
					Memory});

			FrameGraphNodeBuilder Builder(dataDependencies, &Resources, Nodes[idx], ResourceContext, Memory);

			Setup(Builder, data.fields);
			Builder.BuildNode(this);

			return data.fields;
		}


		void AddRenderTarget	(ResourceHandle Texture);
		void AddMemoryPool      (PoolAllocatorInterface* allocator);

		struct FrameGraphNodeWork
		{
			FrameGraphNode::FN_NodeAction   action;
			FrameGraphNode*                 node;
			FrameResources*                 resources;

			void operator () (Context* ctx, iAllocator& allocator)
			{
				action(*node, *resources, *ctx, allocator);
			}
		};

		void ProcessNode		(FrameGraphNode* N, FrameResources& Resources, Vector<FrameGraphNodeWork>& taskList, iAllocator& allocator);
		
		void UpdateFrameGraph	(RenderSystem* RS, iAllocator* Temp);// 
		void SubmitFrameGraph	(UpdateDispatcher& dispatcher, RenderSystem* RS, iAllocator* persistentAllocator);

		RenderSystem& GetRenderSystem() { return Resources.renderSystem; }


		FrameResources				Resources;
		FrameGraphResourceContext	ResourceContext;
		iAllocator*					Memory;
		Vector<FrameGraphNode>		Nodes;
		ThreadManager&              threads;
		DataDependencyList			dataDependencies;

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
	inline bool PushConstantBufferData(char* _ptr, size_t Size, ConstantBufferHandle Buffer, FrameResources& Resources)
	{
		const auto res = Resources.renderSystem.ConstantBuffers.Push(Buffer, _ptr, Size);
		FK_ASSERT(res, "Failed to Push Constants!");
		return res.has_value();
	}


	/************************************************************************************************/


	template<typename TY_CB>
	[[deprecated]] bool PushConstantBufferData(const TY_CB& Constants, ConstantBufferHandle Buffer, FrameResources& Resources)
	{
		const auto res = Resources.renderSystem.ConstantBuffers.Push(Buffer, (void*)&Constants, sizeof(TY_CB));
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

	inline void PresentBackBuffer(FrameGraph& frameGraph, ResourceHandle& backBuffer)
	{
		struct PassData
		{
			FrameResourceHandle BackBuffer;
		};
		auto& Pass = frameGraph.AddNode<PassData>(
			PassData{},
			[&](FrameGraphNodeBuilder& Builder, PassData& Data)
			{
				Data.BackBuffer = Builder.PresentBackBuffer(backBuffer);
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
		size_t                  vertexCount;
		ResourceHandle          texture = InvalidHandle_t;
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
		Range begin()
		{
			return { 0, end_, step_ };
		}


		Range end()
		{
			return { end_, end_, step_ };
		}


		size_t operator * ()
		{
			return itr * step_;
		}


		bool operator == (Range& rhs)
		{
			return  (rhs.itr == itr) & (rhs.end_ == end_) & (rhs.step_ == step_);
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


	auto MakeRange(size_t begin, size_t end, size_t step = 1)
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
			DrawList&				        DrawList, 
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        Resources) override
		{
			auto VBBuffer   = reserveVB(sizeof(ShapeVert) * 3 * Divisions);

			const float Step = 2 * pi / Divisions;

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				MakeRange(0, Divisions),
				[&](size_t I, auto& pushBuffer) -> int
				{
					float2 V1 = { POS.x + R * cos(Step * (I + 1)),	POS.y - AspectRatio * (R * sin(Step * (I + 1))) };
					float2 V2 = { POS.x + R * cos(Step * I),		POS.y - AspectRatio * (R * sin(Step * I)) };

					pushBuffer.Push(ShapeVert{ Position2SS(POS),    { 0.0f, 1.0f }, Color });
					pushBuffer.Push(ShapeVert{ Position2SS(V1),	    { 0.0f, 1.0f }, Color });
					pushBuffer.Push(ShapeVert{ Position2SS(V2),	    { 1.0f, 0.0f }, Color });
				},
				VBBuffer };

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};
			ConstantBufferDataSet constants{ CB_Data, reserveCB(256) };

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

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				MakeRange(0, Lines.size()),
				[&](size_t I, auto& pushBuffer) -> ShapeVert
				{
					auto positionA = Lines[I].A;
					auto positionB = Lines[I].B;

					auto pointA = ShapeVert{ positionA, { 0.0f, 1.0f }, Lines[I].AColour };
					auto pointB = ShapeVert{ positionB, { 0.0f, 1.0f }, Lines[I].BColour };

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

			ConstantBufferDataSet constants{ CB_Data, reserveCB(256) };
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
			DrawList&				        DrawList,
			ReserveVertexBufferFunction&    reserveVB,
			ReserveConstantBufferFunction&  reserveCB,
			FrameResources&			        Resources) override
		{
			auto VBBuffer   = reserveVB(sizeof(ShapeVert) * 2 * Lines.size());

			VertexBufferDataSet vertices{
				SET_TRANSFORM_OP,
				MakeRange(0, Lines.size()),
				[&](size_t I, auto& pushBuffer) -> ShapeVert
				{
					auto positionA = Position2SS(Lines[I].A);
					auto positionB = Position2SS(Lines[I].B);

					auto pointA = ShapeVert{ positionA, { 0.0f, 1.0f }, Lines[I].AColour };
					auto pointB = ShapeVert{ positionB, { 0.0f, 1.0f }, Lines[I].BColour };

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

			ConstantBufferDataSet constants{ CB_Data, reserveCB(256) };
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

			ShapeVert vertices[] = {
				ShapeVert{ Position2SS(RectUpperLeft),	 { 0.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectBottomRight), { 1.0f, 0.0f }, Color },
				ShapeVert{ Position2SS(RectBottomLeft),	 { 0.0f, 1.0f }, Color },

				ShapeVert{ Position2SS(RectUpperLeft),	 { 0.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectUpperRight),	 { 1.0f, 1.0f }, Color },
				ShapeVert{ Position2SS(RectBottomRight), { 1.0f, 0.0f }, Color } };

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			DrawList.push_back(
				{   ShapeDraw::RenderMode::Triangle,
					ConstantBufferDataSet   { CB_Data, reserveCB(sizeof(Constants))         },
					VertexBufferDataSet     { vertices, reserveVB(sizeof(ShapeVert) * 6)    },
					6 });
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
			/*
			Constants CB_Data = {
			float4(1, 1, 1, 1),
			float4(1, 1, 1, 1),
			float4x4::Identity() };

			auto CBOffset = BeginNewConstantBuffer(CB, resources);
			PushConstantBufferData(CB_Data, CB, resources);

			const size_t VBOffset = resources.GetVertexBufferOffset(pushBuffer, sizeof(ShapeVert));
			size_t vertexOffset = 0;

			for(auto rect : rects)
			{
				float2 rectUpperLeft	= rect.Position;
				float2 rectBottomRight	= rect.Position + rect.WH;
				float2 rectUpperRight	= { rectBottomRight.x,	rectUpperLeft.y };
				float2 rectBottomLeft	= { rectUpperLeft.x,	rectBottomRight.y };

				PushVertex(ShapeVert{ Position2SS(rectUpperLeft),	{ 0.0f, 1.0f }, rect.Color }, pushBuffer, resources);
				PushVertex(ShapeVert{ Position2SS(rectBottomRight),	{ 1.0f, 0.0f }, rect.Color }, pushBuffer, resources);
				PushVertex(ShapeVert{ Position2SS(rectBottomLeft),	{ 0.0f, 1.0f }, rect.Color }, pushBuffer, resources);

				PushVertex(ShapeVert{ Position2SS(rectUpperLeft),	{ 0.0f, 1.0f }, rect.Color }, pushBuffer, resources);
				PushVertex(ShapeVert{ Position2SS(rectUpperRight),	{ 1.0f, 1.0f }, rect.Color }, pushBuffer, resources);
				PushVertex(ShapeVert{ Position2SS(rectBottomRight),	{ 1.0f, 0.0f }, rect.Color }, pushBuffer, resources);

				drawList.push_back({ ShapeDraw::RenderMode::Triangle, CBOffset, VBOffset, 6, vertexOffset });
				vertexOffset += 6;
			}
			*/
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


	inline void AddShapes(
		DrawList&				        List, 
		ReserveVertexBufferFunction&    reserveCB,
		ReserveConstantBufferFunction&  reserveVB,
		FrameResources&			        Resources) {}


	/************************************************************************************************/


	template<typename TY_1, typename ... TY_OTHER_SHAPES>
	void AddShapes(
		DrawList&				        List, 
		ReserveVertexBufferFunction&    reserveVB,
		ReserveConstantBufferFunction&  reserveCB,
		FrameResources&			        Resources,
		TY_1&					        Shape, 
		TY_OTHER_SHAPES ...		        ShapePack)
	{
		Shape.AddShapeDraw(List, reserveVB, reserveCB, Resources);
		AddShapes(List, reserveVB, reserveCB, Resources, std::forward<TY_OTHER_SHAPES&&>(ShapePack)...);
	}


	/************************************************************************************************/


	template<typename ... TY_OTHER>
	void DrawShapes(
		PSOHandle                       state, 
		FrameGraph&                     frameGraph,
		ReserveVertexBufferFunction     reserveVB,
		ReserveConstantBufferFunction   reserveCB,
		ResourceHandle                  renderTarget, 
		iAllocator*                     allocator, 
		TY_OTHER ... Args)
	{
		struct ShapeParams
		{
			ReserveVertexBufferFunction     reserveVB;
			ReserveConstantBufferFunction   reserveCB;
			PSOHandle				        State;
			FrameResourceHandle		        RenderTarget;
			DrawList				        Draws;
		};


		auto& Pass = frameGraph.AddNode<ShapeParams>(
			ShapeParams{
				reserveVB,
				reserveCB,
				state,
			},
			[&](FrameGraphNodeBuilder& Builder, ShapeParams& Data)
			{
				// Single Thread Section
				// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
				// for thread safety

				Data.RenderTarget	= Builder.WriteRenderTarget(renderTarget);
				Data.Draws			= DrawList(allocator);

				AddShapes(Data.Draws, reserveVB, reserveCB, frameGraph.Resources, std::forward<TY_OTHER&&>(Args)...);
			},
			[=](const ShapeParams& Data, const ResourceHandler& frameResources, Context& context, iAllocator& allocator)
			{	// Multi-threadable Section
				auto WH = frameResources.GetTextureWH(Data.RenderTarget);

				context.SetScissorAndViewports({ frameResources.GetRenderTarget(Data.RenderTarget)} );
				context.SetRenderTargets(
					{ frameResources.GetRenderTarget(Data.RenderTarget) },
					false);

				context.SetRootSignature		(frameResources.renderSystem().Library.RS6CBVs4SRVs);
				context.SetPipelineState		(frameResources.GetPipelineState(Data.State));
				context.SetPrimitiveTopology	(EInputTopology::EIT_TRIANGLE);

				size_t TextureDrawCount = 0;
				ShapeDraw::RenderMode PreviousMode = ShapeDraw::RenderMode::Triangle;
				for (auto D : Data.Draws)
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

				data.renderTarget	= builder.WriteRenderTarget(desc.RenderTarget);
				data.depthBuffer	= desc.enableDepthBuffer ? builder.WriteDepthBuffer(desc.DepthBuffer) : InvalidHandle_t;

				size_t MaxElementSize = 0;
				for (auto& i : constantData)
					MaxElementSize = std::max(MaxElementSize, i.bufferSize);

				data.constantBuffer			= Reserve(desc.constantBuffer, MaxElementSize, constantData.size(), frameGraph.Resources);
				data.instanceBuffer			= Reserve(desc.instanceBuffer, instanceElementSize * desc.reserveCount, frameGraph.Resources);
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

				ctx->SetScissorAndViewports({ resources.GetRenderTarget(data.renderTarget) });
				ctx->SetRenderTargets(
					{	resources.GetRenderTargetObject(data.renderTarget) }, true,
						resources.GetRenderTargetObject(data.depthBuffer));

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


	void WireframeRectangleList(
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
				Data.RenderTarget	= Builder.WriteRenderTarget(desc.RenderTarget);

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
					{ resources.GetRenderTarget(Data.RenderTarget) }, false);

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
		iAllocator*				TempMem)
	{
		LineSegments Lines(TempMem);
		Lines.reserve(ColumnCount + RowCount);

		const auto RStep = 1.0f / RowCount;

		//  Vertical Lines on ground
		for (size_t I = 1; I < RowCount; ++I)
			Lines.push_back(
				{ { RStep  * I * GridWH.x, 0, 0 },
				GridColor,
				{ RStep  * I * GridWH.x, 0, GridWH.y },
				GridColor });


		// Horizontal lines on ground
		const auto CStep = 1.0f / ColumnCount;
		for (size_t I = 1; I < ColumnCount; ++I)
			Lines.push_back(
				{ { 0,			0, CStep  * I * GridWH.y },
				GridColor,
				{ GridWH.x,		0, CStep  * I * GridWH.y },
				GridColor });


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

				Data.RenderTarget	        = builder.WriteRenderTarget(RenderTarget);
				Data.DepthBuffer	        = builder.WriteDepthBuffer(DepthBuffer);
				Data.CB				        = constants;


				Drawable::VConstantsLayout DrawableConstants = {	
					.MP			= Drawable::MaterialProperties{},
					.Transform	= float4x4::Identity()
				};

				CBPushBuffer cbPushBuffer(
					constants,
					AlignedSize<Drawable::VConstantsLayout>() + AlignedSize<Camera::ConstantBuffer>(),
					frameGraph.GetRenderSystem());

				Data.passConstants      = ConstantBufferDataSet(constants, cbPushBuffer);
				Data.cameraConstants    = ConstantBufferDataSet(GetCameraConstants(Camera), cbPushBuffer);

				VBPushBuffer vbPushBuffer(
					vertexBuffer,
					sizeof(VertexLayout) * Lines.size() * 2,
					frameGraph.GetRenderSystem());

				VertexBufferDataSet vertices{
					SET_TRANSFORM_OP,
					MakeRange(0, Lines.size()),
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

				ctx.SetScissorAndViewports({ resources.GetRenderTarget(Data.RenderTarget) });
				ctx.SetRenderTargets(
					{	resources.GetRenderTarget(Data.RenderTarget) }, false,
						resources.GetRenderTarget(Data.DepthBuffer));

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
				data.feedbackTarget = builder.WriteRenderTarget(target);
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
