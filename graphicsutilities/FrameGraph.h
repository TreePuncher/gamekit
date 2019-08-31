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

#include "..\graphicsutilities\graphics.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\Components.h"

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
	};


	/************************************************************************************************/
	typedef Handle_t<16> FrameResourceHandle;
	typedef Handle_t<16> StaticFrameResourceHandle;


	struct FrameObject
	{
		FrameObject() :
			Handle{ (uint32_t)INVALIDHANDLE }{}


		FrameObject(const FrameObject& rhs) :
			Type	{ rhs.Type		},
			State	{ rhs.State		},
			Tag		{ rhs.Tag		},
			Handle	{ rhs.Handle	}
		{
			Buffer = rhs.Buffer;
		}

		FrameResourceHandle		Handle; // For Fast Search
		FrameObjectResourceType Type;
		DeviceResourceState		State;
		uint32_t				Tag;

		union
		{
			struct {
				TextureHandle	Texture;
				DescHeapPOS		HeapPOS;
			}RenderTarget;

			QueryHandle				query;
			ShaderResourceHandle	ShaderResource;
			GPUResourceHandle		byteBuffer;
			SOResourceHandle		SOBuffer;
			UAVResourceHandle		UAVBuffer;
			UAVTextureHandle		UAVTexture;
			TextureHandle			Texture;

			struct {
				char	buff[256];
			}Buffer;
		};

		static FrameObject ReadRenderTargetObject(uint32_t Tag, TextureHandle Handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State					= DeviceResourceState::DRS_ShaderResource;
			RenderTarget.Type					= OT_RenderTarget;
			RenderTarget.Tag					= Tag;
			RenderTarget.RenderTarget.Texture	= Handle;

			return RenderTarget;
		}


		static FrameObject WriteRenderTargetObject(uint32_t Tag, TextureHandle Handle)
		{
			FrameObject RenderTarget;
			RenderTarget.State		            = DeviceResourceState::DRS_RenderTarget;
			RenderTarget.Type		            = OT_RenderTarget;
			RenderTarget.Tag		            = Tag;
			RenderTarget.RenderTarget.Texture   = Handle;

			return RenderTarget;
		}

		static FrameObject BackBufferObject(uint32_t Tag, TextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			FrameObject RenderTarget;
			RenderTarget.State					= InitialState;
			RenderTarget.Type					= OT_BackBuffer;
			RenderTarget.Tag					= Tag;
			RenderTarget.RenderTarget.Texture	= Handle;

			return RenderTarget;
		}

		static FrameObject DepthBufferObject(uint32_t Tag, TextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
		{
			FrameObject RenderTarget;
			RenderTarget.State                = InitialState;
			RenderTarget.Type                 = OT_DepthBuffer;
			RenderTarget.Tag                  = Tag;
			RenderTarget.RenderTarget.Texture = Handle;

			return RenderTarget;
		}

		static FrameObject TextureObject(uint32_t Tag, TextureHandle Handle, DeviceResourceState InitialState)
		{
			FrameObject shaderResource;
			shaderResource.State                = InitialState;
			shaderResource.Type                 = OT_ShaderResource;
			shaderResource.Tag                  = Tag;
			shaderResource.ShaderResource		= handle_cast<ShaderResourceHandle>(Handle);

			return shaderResource;
		}


		static FrameObject GPUResourceObject(uint32_t Tag, GPUResourceHandle Handle, DeviceResourceState InitialState)
		{
			FrameObject shaderResource;
			shaderResource.State			= InitialState;
			shaderResource.Type				= OT_ByteBuffer;
			shaderResource.Tag				= Tag;
			shaderResource.ShaderResource	= handle_cast<ShaderResourceHandle>(Handle);

			return shaderResource;
		}


		static FrameObject UAVBufferObject(uint32_t Tag, UAVResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
		{
			FrameObject UnorderedAccessViewObject;
			UnorderedAccessViewObject.State                = InitialState;
			UnorderedAccessViewObject.Type                 = OT_UAVBuffer;
			UnorderedAccessViewObject.Tag                  = Tag;
			UnorderedAccessViewObject.UAVBuffer			   = Handle;

			return UnorderedAccessViewObject;
		}


		static FrameObject UAVTextureObject(uint32_t Tag, UAVTextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
		{
			FrameObject UnorderedAccessViewObject;
			UnorderedAccessViewObject.State             = InitialState;
			UnorderedAccessViewObject.Type              = OT_UAVTexture;
			UnorderedAccessViewObject.Tag               = Tag;
			UnorderedAccessViewObject.UAVTexture		= Handle;

			return UnorderedAccessViewObject;
		}


		static FrameObject SOBufferObject(uint32_t Tag, SOResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_GENERIC)
		{
			FrameObject Streamout;
			Streamout.State                = InitialState;
			Streamout.Type                 = OT_StreamOut;
			Streamout.Tag                  = Tag;
			Streamout.SOBuffer			   = Handle;

			return Streamout;
		}

		static FrameObject QueryObject(QueryHandle handle, DeviceResourceState initialState = DeviceResourceState::DRS_GENERIC)
		{
			FrameObject query;
			query.State		= initialState;
			query.Type		= OT_Query;
			query.Tag		= 0;
			query.query		= handle;

			return query;
		}
	};


	/************************************************************************************************/


	typedef Vector<FrameObject>		PassObjectList;
	class FrameGraph;

	FLEXKITAPI class FrameResources
	{
	public:
		FrameResources(RenderSystem& IN_renderSystem, iAllocator* IN_allocator) : 
			Resources		{ IN_allocator		},
			Textures		{ IN_allocator		},
			SubNodeTracking	{ IN_allocator		},
			renderSystem	{ IN_renderSystem	}{}


		PassObjectList			Resources;
		PassObjectList			Textures;	// State should be mostly Static across frame
		mutable PassObjectList	SubNodeTracking;
		RenderSystem&			renderSystem;


		/************************************************************************************************/


		void AddRenderTarget(TextureHandle Handle)
		{
			AddRenderTarget(
				Handle,
				renderSystem.GetTag(Handle),
				renderSystem.RenderTargets.GetState(Handle));
		}


		/************************************************************************************************/


		void AddRenderTarget(TextureHandle Handle, uint32_t Tag, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
		{
			Resources.push_back(
				FrameObject::BackBufferObject(Tag, Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddDepthBuffer(TextureHandle Handle)
		{
			AddDepthBuffer(
				Handle,
				renderSystem.GetTag(Handle),
				renderSystem.RenderTargets.GetState(Handle));
		}


		/************************************************************************************************/


		void AddDepthBuffer(TextureHandle Handle, uint32_t Tag, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
		{
			Resources.push_back(
				FrameObject::DepthBufferObject(Tag, Handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddUAVResource(UAVResourceHandle handle, uint32_t tag, DeviceResourceState InitialState = DeviceResourceState::DRS_Write)
		{
			Resources.push_back(
				FrameObject::UAVBufferObject(tag, handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}

		void AddUAVResource(UAVTextureHandle handle, uint32_t tag, DeviceResourceState InitialState = DeviceResourceState::DRS_Write)
		{
			Resources.push_back(
				FrameObject::UAVTextureObject(tag, handle, InitialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddSOResource(SOResourceHandle handle, uint32_t tag)
		{
			DeviceResourceState initialState = renderSystem.GetObjectState(handle);

			Resources.push_back(
				FrameObject::SOBufferObject(tag, handle, initialState));

			Resources.back().Handle = FrameResourceHandle{ (uint32_t)Resources.size() - 1 };
		}


		/************************************************************************************************/


		void AddShaderResource(TextureHandle handle, uint32_t tag = 0)
		{
			DeviceResourceState initialState = renderSystem.GetObjectState(handle);

			Resources.push_back(
				FrameObject::TextureObject(tag, handle, initialState));

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
			return renderSystem.GetObjectDeviceResource(handle);
		}


		ID3D12Resource* GetUAVDeviceResource(FrameResourceHandle handle) const
		{
			auto res = _FindSubNodeResource(handle);
			return GetObjectResource(res->UAVBuffer);
		}

		/************************************************************************************************/


		TextureObject				GetRenderTargetObject(FrameResourceHandle Handle) const
		{
			return { Resources[Handle].RenderTarget.HeapPOS, Resources[Handle].RenderTarget.Texture };
		}


		/************************************************************************************************/


		static_vector<DescHeapPOS>	GetRenderTargetObjects(const static_vector<FrameResourceHandle> Handles) const
		{
			static_vector<DescHeapPOS> Out;
			for (auto Handle : Handles)
				Out.push_back(Resources[Handle].RenderTarget.HeapPOS);

			return Out;
		}


		/************************************************************************************************/


		ID3D12PipelineState*	GetPipelineState(PSOHandle State)	const
		{
			return renderSystem.GetPSO(State);
		}


		/************************************************************************************************/


		size_t					GetVertexBufferOffset(VertexBufferHandle Handle, size_t VertexSize)
		{
			return renderSystem.VertexBuffers.GetCurrentVertexBufferOffset(Handle) / VertexSize;
		}


		/************************************************************************************************/


		size_t					GetVertexBufferOffset(VertexBufferHandle Handle)
		{
			return renderSystem.VertexBuffers.GetCurrentVertexBufferOffset(Handle);
		}


		/************************************************************************************************/


		DeviceResourceState		GetResourceObjectState(FrameResourceHandle Handle)
		{
			return Resources[Handle].State;
		}


		/************************************************************************************************/


		FrameObject*			GetResourceObject(FrameResourceHandle Handle)
		{
			return &Resources[Handle];
		}


		/************************************************************************************************/


		TextureHandle			GetRenderTarget(FrameResourceHandle Handle) const
		{
			return Resources[Handle].RenderTarget.Texture;
		}


		/************************************************************************************************/


		uint2					GetRenderTargetWH(FrameResourceHandle Handle) const
		{
			return renderSystem.GetRenderTargetWH(GetRenderTarget(Handle));
		}


		/************************************************************************************************/


		DescHeapPOS				GetRenderTargetDescHeapEntry(FrameResourceHandle Handle) const
		{
			return Resources[Handle].RenderTarget.HeapPOS;
		}


		/************************************************************************************************/


		TextureHandle			GetTexture(FrameResourceHandle Handle) const
		{
			return handle_cast<TextureHandle>(Textures[Handle].ShaderResource);
		}


		/************************************************************************************************/


		StaticFrameResourceHandle AddTexture(TextureHandle Handle, uint32_t Tag)
		{
			FK_ASSERT(0);
			return InvalidHandle_t;
		}


		/************************************************************************************************/


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

			auto res			= _FindSubNodeResource(handle);
			auto SOHandle		= res->SOBuffer;
			auto deviceResource = renderSystem.GetObjectDeviceResource(SOHandle);

			if (res->State != DRS_VERTEXBUFFER) 
				ctx->AddStreamOutBarrier(SOHandle, res->State, DRS_VERTEXBUFFER);

			res->State = DRS_VERTEXBUFFER;

			D3D12_VERTEX_BUFFER_VIEW view = {
				deviceResource->GetGPUVirtualAddress(),
				static_cast<UINT>(renderSystem.GetStreamOutBufferSize(SOHandle)),
				static_cast<UINT>(vertexSize)
			};

			return view;
		}


		/************************************************************************************************/


		SOResourceHandle GetSOResource(ResourceHandle handle) const
		{
			FrameObject* resource = nullptr;
			auto res = find(SubNodeTracking,
				[&](const FrameObject& rhs) -> bool
				{
					return rhs.Handle == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle;
					});

				FK_ASSERT(res != Resources.end());
				SubNodeTracking.push_back(*res);
				resource = &SubNodeTracking.back();
			}
			else
				resource = res;

			return resource->SOBuffer;
		}


		/************************************************************************************************/


		UAVResourceHandle GetUAVBufferResource(FrameResourceHandle handle) const
		{
			FrameObject* resource = nullptr;
			auto res = find(SubNodeTracking,
				[&](const FrameObject& rhs) -> bool
				{
					return rhs.Handle == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle && rhs.Type == OT_UAVBuffer;
					});

				FK_ASSERT(res != Resources.end());
				SubNodeTracking.push_back(*res);
				resource = &SubNodeTracking.back();
			}
			else
				resource = res;

			return resource->UAVBuffer;
		}


		/************************************************************************************************/


		UAVTextureHandle GetUAVTextureResource(FrameResourceHandle handle) const
		{
			FrameObject* resource = nullptr;
			auto res = find(SubNodeTracking,
				[&](const FrameObject& rhs) -> bool
				{
					return rhs.Handle == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle && rhs.Type == OT_UAVTexture;
					});

				FK_ASSERT(res != Resources.end());
				SubNodeTracking.push_back(*res);
				resource = &SubNodeTracking.back();
			}
			else
				resource = res;

			return resource->UAVTexture;
		}


		/************************************************************************************************/


		UAVTextureHandle ReadWriteUAVTexture(FrameResourceHandle resource, Context* ctx) const
		{
			auto res = _FindSubNodeResource(resource);
			if (res->State != DRS_UAV)
				ctx->AddUAVBarrier(res->UAVTexture, res->State, DRS_UAV);

			res->State = DRS_UAV;

			return res->UAVTexture;
		}

		UAVResourceHandle ReadWriteUAVBuffer(FrameResourceHandle resource, Context* ctx) const
		{
			auto res = _FindSubNodeResource(resource);
			if (res->State != DRS_UAV)
				ctx->AddUAVBarrier(res->UAVBuffer, res->State, DRS_UAV);

			res->State = DRS_UAV;

			return res->UAVBuffer;
		}

		UAVResourceHandle ReadUAVBuffer(FrameResourceHandle resource, DeviceResourceState state, Context* ctx) const
		{
			auto res = _FindSubNodeResource(resource);
			if (res->State != state)
				ctx->AddUAVBarrier(res->UAVBuffer, res->State, state);

			res->State = state;

			return res->UAVBuffer;
		}


		/************************************************************************************************/


		UAVResourceHandle ReadIndirectArgs(FrameResourceHandle resource, Context* ctx) const
		{
			auto res = _FindSubNodeResource(resource);
			if (res->State != DRS_INDIRECTARGS)
				ctx->AddUAVBarrier(res->UAVBuffer, res->State, DRS_INDIRECTARGS);

			res->State = DRS_INDIRECTARGS;

			return res->UAVBuffer;
		}


		/************************************************************************************************/


		UAVResourceHandle WriteUAV(FrameResourceHandle resource, Context* ctx) const
		{
			auto res = _FindSubNodeResource(resource);
			if (res->State != DRS_Write)
				ctx->AddUAVBarrier(res->UAVBuffer, res->State, DRS_Write);

			res->State = DRS_Write;

			return res->UAVBuffer;
		}


		/************************************************************************************************/


		DeviceResourceState GetObjectState(FrameResourceHandle handle) const
		{
			FrameObject* resource = nullptr;
			auto res = find(SubNodeTracking, 
				[&](const FrameObject& rhs) -> bool
				{
					return rhs.Handle == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle;
					});

				FK_ASSERT(res != Resources.end());
				SubNodeTracking.push_back(*res);
				resource = &SubNodeTracking.back();
			}
			else
				resource = res;

			return resource->State;
		}


		/************************************************************************************************/


		D3D12_STREAM_OUTPUT_BUFFER_VIEW WriteStreamOut(FrameResourceHandle handle, Context* ctx, size_t inputStride) const
		{
			auto resource = _FindSubNodeResource(handle);

			if (resource->State != DRS_STREAMOUT)
				ctx->AddStreamOutBarrier(resource->SOBuffer, resource->State, DRS_STREAMOUT);

			resource->State = DRS_STREAMOUT;

			/*
			typedef struct D3D12_STREAM_OUTPUT_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT64 SizeInBytes;
			D3D12_GPU_VIRTUAL_ADDRESS BufferFilledSizeLocation;
			} 	D3D12_STREAM_OUTPUT_BUFFER_VIEW;
			*/

			auto SOHandle			= resource->SOBuffer;

			D3D12_STREAM_OUTPUT_BUFFER_VIEW view =
			{
				renderSystem.GetObjectDeviceResource(SOHandle)->GetGPUVirtualAddress(),
				renderSystem.GetStreamOutBufferSize(SOHandle),
				renderSystem.GetSOCounterResource(SOHandle)->GetGPUVirtualAddress(),
			};

			return view;
		}



		/************************************************************************************************/


		SOResourceHandle ClearStreamOut(FrameResourceHandle handle, Context* ctx) const
		{
			auto resource = _FindSubNodeResource(handle);

			if (resource->State != DRS_STREAMOUTCLEAR)
				ctx->AddStreamOutBarrier(resource->SOBuffer, resource->State, DRS_STREAMOUTCLEAR);

			resource->State = DRS_STREAMOUTCLEAR;

			return GetSOResource(handle);
		}


		/************************************************************************************************/



		FrameResourceHandle	FindFrameResource(uint32_t Tag)
		{
			auto res = find(Resources, 
				[&](const auto& LHS)
				{
					return (
						LHS.Tag == Tag && 
						(	LHS.Type == OT_RenderTarget || 
							LHS.Type == OT_BackBuffer	|| 
							LHS.Type == OT_DepthBuffer ));	
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(TextureHandle Handle)
		{
			auto res = find(Resources, 
				[&](const FrameObject& LHS)
				{
					auto CorrectType = (
						LHS.Type == OT_ShaderResource	||
						LHS.Type == OT_RenderTarget		||
						LHS.Type == OT_BackBuffer		||
						LHS.Type == OT_DepthBuffer);

					return (CorrectType && LHS.ShaderResource.to_uint() == Handle.to_uint());
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}


		/************************************************************************************************/


		FrameResourceHandle	FindFrameResource(ShaderResourceHandle Handle)
		{
			auto res = find(Resources,
				[&](const auto& LHS)
				{
					auto CorrectType = LHS.Type == OT_ShaderResource;

					return (CorrectType && LHS.ShaderResource == Handle);
				});

			if (res != Resources.end())
				return res->Handle;

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
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

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
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

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
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

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
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

			// Create New Resource
			FrameResourceHandle NewResource;

			FK_ASSERT(0);

			return NewResource;
		}


		/************************************************************************************************/


		FrameObject* _FindSubNodeResource(FrameResourceHandle handle) const
		{
			FrameObject* resource = nullptr;
			auto res = find(SubNodeTracking,
				[&](const FrameObject& rhs) -> bool
				{
					return rhs.Handle == handle;
				});

			if (res == SubNodeTracking.end())
			{
				auto res = find(Resources,
					[&](const FrameObject& rhs) -> bool
					{
						return rhs.Handle == handle;
					});

				FK_ASSERT(res != Resources.end());
				SubNodeTracking.push_back(*res);
				resource = &SubNodeTracking.back();
			}
			else
				resource = res;

			FK_ASSERT(resource != nullptr);

			return resource;
		}


		operator RenderSystem& ()
		{
			return *renderSystem;
		}

	};/************************************************************************************************/


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
		return Resources.renderSystem.ConstantBuffers.BeginNewBuffer(CB);
	}


	/************************************************************************************************/


	inline bool PushConstantBufferData(char* _ptr, size_t Size, ConstantBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.renderSystem.ConstantBuffers.Push(Buffer, _ptr, Size);
		FK_ASSERT(res, "Failed to Push Constants!");
		return res;
	}


	/************************************************************************************************/


	template<typename TY_CB>
	bool PushConstantBufferData(const TY_CB& Constants, ConstantBufferHandle Buffer, FrameResources& Resources)
	{
		bool res = Resources.renderSystem.ConstantBuffers.Push(Buffer, (void*)&Constants, sizeof(TY_CB));
		FK_ASSERT(res, "Failed to Push Constants!");
		return res;
	}


	/************************************************************************************************/


	class FrameGraphNode;

	struct FrameObjectDependency
	{
		FrameObjectDependency(
			FrameObject*		FO_IN				= nullptr,
			FrameGraphNode*		SourceObject_IN		= nullptr,
			DeviceResourceState	ExpectedState_IN	= DRS_UNKNOWN,
			DeviceResourceState	State_IN			= DRS_UNKNOWN) :
				FO				(FO_IN),
				Source			(SourceObject_IN),
				ExpectedState	(ExpectedState_IN),
				State			(State_IN),
				Tag				(FO_IN ? FO_IN->Tag : -1)
		{
			SOHandle = InvalidHandle_t;
		}

		bool TransitionNeeded()
		{
			return ExpectedState != State;
		}

		FrameObject*		FO;
		FrameGraphNode*		Source;
		uint32_t			Tag;

		union {
			uint32_t				ID;	// Extra ID
			UAVResourceHandle		UAVBuffer;
			UAVTextureHandle		UAVTexture;
			SOResourceHandle		SOHandle;
			ShaderResourceHandle	ShaderResource;
			TextureHandle			Texture;
		};

		DeviceResourceState	ExpectedState;
		DeviceResourceState	State;
	};


	/************************************************************************************************/


	class ResourceTransition
	{
	public:
		ResourceTransition() :
			Object		{ nullptr },
			BeforeState	{ DRS_UNKNOWN },
			AfterState	{ DRS_UNKNOWN }{}

		ResourceTransition		(FrameObjectDependency& Dep);
		void ProcessTransition	(FrameResources& Resources, Context* Ctx) const;

		FrameObject*		Object;
		DeviceResourceState	BeforeState;
		DeviceResourceState	AfterState;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNode
	{
	public:
		typedef void (*FN_NodeAction)(FrameGraphNode& node, FrameResources& Resources, Context* Ctx) ;

		FrameGraphNode(FN_NodeAction IN_action, void* IN_nodeData, iAllocator* IN_allocator = nullptr) :
			InputObjects	{ IN_allocator		    },
			OutputObjects	{ IN_allocator		    },
			Sources			{ IN_allocator		    },
			Transitions		{ IN_allocator		    },
			NodeAction		{ IN_action             },
            nodeData        { IN_nodeData           },
			Executed		{ false				    } {}


		FrameGraphNode(const FrameGraphNode& RHS) : 
			Sources			{ RHS.Sources		        },
			InputObjects	{ RHS.InputObjects	        },
			OutputObjects	{ RHS.OutputObjects	        },
			Transitions		{ RHS.Transitions	        },
			NodeAction		{ std::move(RHS.NodeAction)	},
            nodeData        { RHS.nodeData              },
			Executed		{ false				        } {}


		~FrameGraphNode() = default;


		void HandleBarriers	(FrameResources& Resouces, Context* Ctx);
		void AddTransition	(FrameObjectDependency& Dep);
		bool DependsOn		(uint32_t Tag);
		bool Outputs		(uint32_t Tag);

		void RestoreResourceStates(Context* ctx, PassObjectList& locallyTrackedObjects);


		Vector<FrameGraphNode*>		GetNodeDependencies()	{ return (nullptr); } 


        void*                           nodeData;
		bool							Executed;
		FN_NodeAction					NodeAction;
		Vector<FrameGraphNode*>			Sources;// Nodes that this node reads from
		Vector<FrameObjectDependency>	InputObjects;
		Vector<FrameObjectDependency>	OutputObjects;
		Vector<ResourceTransition>		Transitions;
	};


	/************************************************************************************************/


	auto MakePred(QueryHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_Query;
			if (A && lhs.UAVBuffer == InvalidHandle_t)
				lhs.UAVBuffer = lhs.FO->UAVBuffer;

			return A && (lhs.UAVBuffer.to_uint() == handle.to_uint());
		};
	}


	auto MakePred(uint32_t tag)
	{
		return [tag](auto& lhs)
		{
			return (lhs.Tag == tag);
		};
	}

	template<typename TY>
	auto MakePred(TY handle, const FrameObjectResourceType type)
	{
		return [handle, type](auto& lhs)
		{
			auto A = lhs.FO->Type == type;
			return A && (lhs.ID == handle);
		};
	}


	auto MakePred(UAVResourceHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_UAVBuffer;
			if (A && lhs.UAVBuffer == InvalidHandle_t)
				lhs.UAVBuffer = lhs.FO->UAVBuffer;

			return A && (lhs.UAVBuffer == handle);
		};
	}


	auto MakePred(UAVTextureHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_UAVTexture;
			if (A && lhs.UAVTexture == InvalidHandle_t)
				lhs.UAVTexture = lhs.FO->UAVTexture;

			return A && (lhs.UAVTexture == handle);
		};
	}


	auto MakePred(TextureHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_BackBuffer || lhs.FO->Type == OT_RenderTarget;
			if (A && lhs.UAVTexture == InvalidHandle_t)
				lhs.Texture = lhs.FO->Texture;

			return A && (lhs.Texture == handle);
		};
	}

	auto MakePred(SOResourceHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_StreamOut;
			if (A && lhs.SOHandle == InvalidHandle_t)
				lhs.SOHandle = lhs.FO->SOBuffer;

			return A && (lhs.SOHandle == handle);
		};
	}


	auto MakePred(ShaderResourceHandle handle)
	{
		return [handle](FrameObjectDependency& lhs)
		{
			auto A = lhs.FO->Type == OT_ShaderResource;
			if (A && lhs.ShaderResource == InvalidHandle_t)
				lhs.ShaderResource = lhs.FO->ShaderResource;

			return A && (lhs.SOHandle.to_uint() == handle.to_uint());
		};
	}


	FLEXKITAPI class FrameGraphResourceContext
	{
	public:
		FrameGraphResourceContext(iAllocator* Temp) :
			Writables	{Temp},
			Readables	{Temp},
			Retirees	{Temp}{}


		void AddWriteable(const FrameObjectDependency& NewObject)
		{
			Writables.push_back(NewObject);
		}


		void AddReadable(const FrameObjectDependency& NewObject)
		{
			Readables.push_back(NewObject);
		}


		template<typename TY>
		void RemoveWriteable(TY handle)
		{
			Writables.remove_unstable(find(Writables, MakePred(handle)));
		}


		template<typename TY>
		void RemoveReadable(TY handle)
		{
			Readables.remove_unstable(find(Readables, MakePred(handle)));
		}


		void Retire(FrameObjectDependency& Object)
		{
			RemoveReadable	(Object.Tag);
			RemoveWriteable	(Object.Tag);

			Object.ExpectedState = Object.State;
			Retirees.push_back(Object);
		}


		template<typename TY>
		FrameObjectDependency& GetReadable(TY handle)
		{
			return *find(Readables, MakePred(handle));
		}


		template<typename TY>
		FrameObjectDependency& GetWriteable(TY handle)
		{
			return *find(Writables, MakePred(handle));
		}


		Vector<FrameObjectDependency>	GetFinalStates()
		{
			Vector<FrameObjectDependency> Objects(Writables.Allocator);
			Objects.reserve(Writables.size() + Readables.size() + Retirees.size());

			Objects += Writables;
			Objects += Readables;
			Objects += Retirees;

			return std::move(Objects);
		}


		template<typename _pred>
		Pair<bool, FrameObjectDependency&> _IsTrackedReadable(_pred pred)
		{
			auto Res = find(Readables, pred);
			return { Res != Readables.end(), *Res };
		}


		template<typename _pred>
		Pair<bool, FrameObjectDependency&> _IsTrackedWriteable(_pred pred)
		{
			auto Res = find(Writables, pred);
			return { Res != Writables.end(), *Res };
		}


		template<typename TY>
		Pair<bool, FrameObjectDependency&> IsTrackedReadable	(TY handle)
		{
			return _IsTrackedReadable(MakePred(handle));
		}


		template<typename TY>
		Pair<bool, FrameObjectDependency&> IsTrackedWriteable	(TY handle)
		{
			return _IsTrackedWriteable(MakePred(handle));
		}


		template<typename TY>
		Pair<bool, FrameObjectDependency&> IsTrackedReadable	(TY handle, FrameObjectResourceType type)
		{
			return _IsTrackedReadable(MakePred(handle, type));
		}


		template<typename TY>
		Pair<bool, FrameObjectDependency&> IsTrackedWriteable	(TY handle, FrameObjectResourceType type)
		{
			return _IsTrackedWriteable(MakePred(handle, type));
		}

		Vector<FrameObjectDependency>	Writables;
		Vector<FrameObjectDependency>	Readables;
		Vector<FrameObjectDependency>	Retirees;
	};


	/************************************************************************************************/


	FLEXKITAPI class FrameGraphNodeBuilder
	{
	public:
		FrameGraphNodeBuilder(
			Vector<UpdateTask*>&		IN_DataDependencies,
			FrameResources*				IN_Resources, 
			FrameGraphNode&				IN_Node,
			FrameGraphResourceContext&	IN_context,
			iAllocator*					IN_allocator) :
				DataDependencies{ IN_DataDependencies	},
				Context			{ IN_context			},
				LocalInputs		{ IN_allocator			},
				LocalOutputs	{ IN_allocator			},
				Node			{ IN_Node				},
				Resources		{ IN_Resources			},
				Transitions		{ IN_allocator			}{}


		// No Copying
		FrameGraphNodeBuilder				(const FrameGraphNodeBuilder& RHS) = delete;
		FrameGraphNodeBuilder&	operator =	(const FrameGraphNodeBuilder& RHS) = delete;

		void BuildNode(FrameGraph* FrameGraph);


		void AddDataDependency(UpdateTask& task);

		FrameResourceHandle ReadShaderResource	(TextureHandle Handle);
		FrameResourceHandle WriteShaderResource	(TextureHandle Handle);

		FrameResourceHandle WriteShaderResource	(GPUResourceHandle Handle);

		FrameResourceHandle ReadRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);
		FrameResourceHandle WriteRenderTarget	(uint32_t Tag, RenderTargetFormat Formt = TRF_Auto);

		FrameResourceHandle ReadRenderTarget	(TextureHandle Handle);
		FrameResourceHandle WriteRenderTarget	(TextureHandle Handle);

		FrameResourceHandle	PresentBackBuffer	(TextureHandle Tag);
		FrameResourceHandle	ReadBackBuffer		(TextureHandle Tag);
		FrameResourceHandle	WriteBackBuffer		(TextureHandle Tag);

		//FrameResourceHandle	ReadDepthBuffer		(uint32_t Tag);
		//FrameResourceHandle	WriteDepthBuffer	(uint32_t Tag);

		//FrameResourceHandle	ReadDepthBuffer		(TextureHandle Handle);
		FrameResourceHandle	WriteDepthBuffer	(TextureHandle Handle);

		FrameResourceHandle	ReadWriteUAV(UAVResourceHandle, DeviceResourceState state = DeviceResourceState::DRS_Write);
		FrameResourceHandle	ReadWriteUAV(UAVTextureHandle,	DeviceResourceState	state = DeviceResourceState::DRS_Write);

		FrameResourceHandle	ReadSOBuffer	(SOResourceHandle);
		FrameResourceHandle	WriteSOBuffer	(SOResourceHandle);

		size_t							GetDescriptorTableSize			(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot
		const DesciptorHeapLayout<16>&	GetDescriptorTableLayout		(PSOHandle State, size_t index) const;// PSO index + handle to desciptor table slot

		DescriptorHeap					ReserveDescriptorTableSpaces	(const DesciptorHeapLayout<16>& IN_layout, size_t requiredTables, iAllocator* tempMemory);

		operator FrameResources&	() const	{ return *Resources; }
		operator RenderSystem&		()			{ return *Resources->renderSystem;}

	private:

		template<typename TY>
		FrameResourceHandle AddReadableResource		(TY handle, DeviceResourceState state)
		{
			bool TrackedReadable = Context.IsTrackedReadable	(handle);
			bool TrackedWritable = Context.IsTrackedWriteable	(handle);

			if (!TrackedReadable && !TrackedWritable)
			{
				FrameResourceHandle Resource = Resources->FindFrameResource(handle);

				LocalInputs.push_back(
					FrameObjectDependency{
						Resources->GetResourceObject(Resource),
						nullptr,
						Resources->GetResourceObjectState(Resource),
						state });

				Context.AddReadable(LocalOutputs.back());
				Transitions.push_back(LocalOutputs.back());

				return Resource;
			}
			else
			{
				auto Object = TrackedReadable ?
					Context.GetReadable(handle) : Context.GetWriteable(handle);

				Object.ExpectedState	= Object.State;
				Object.State			= state;
				LocalOutputs.push_back(Object);

				if (TrackedWritable) {
					Context.RemoveWriteable(handle);
					Context.AddReadable(Object);
					Transitions.push_back(Object);
				}

				return Object.FO->Handle;
			}
		}


		template<typename TY>
		FrameResourceHandle AddWriteableResource	(TY handle, DeviceResourceState state)
		{
			bool TrackedReadable = Context.IsTrackedReadable	(handle);
			bool TrackedWritable = Context.IsTrackedWriteable	(handle);

			if (!TrackedReadable && !TrackedWritable)
			{
				FrameResourceHandle Resource = Resources->FindFrameResource(handle);

				LocalOutputs.push_back(
					FrameObjectDependency{
						Resources->GetResourceObject(Resource),
						nullptr,
						Resources->GetResourceObjectState(Resource),
						state });

				Context.AddWriteable(LocalOutputs.back());
				Transitions.push_back(LocalOutputs.back());

				return Resource;
			}
			else
			{
				auto Object = TrackedReadable ?
					Context.GetReadable(handle) : Context.GetWriteable(handle);

				Object.ExpectedState	= Object.State;
				Object.State			= state;
				LocalOutputs.push_back(Object);

				if (Object.ExpectedState != state) {
					Context.RemoveReadable(handle);
					
					if (TrackedWritable)
						Context.RemoveWriteable(handle);

					Context.AddWriteable(Object);
					Transitions.push_back(Object);
				}

				return Object.FO->Handle;
			}
		}

		FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(TextureHandle handle, DeviceResourceState state, FrameObjectResourceType type);

		enum class CheckStateRes
		{
			TransitionNeeded,
			CorrectState,
			ResourceNotTracked,
			Error,
		};


		static CheckStateRes CheckResourceSituation(
			Vector<FrameObjectDependency>&  Set1,
			Vector<FrameObjectDependency>&  Set2,
			FrameObjectDependency&			Object);

		Vector<FrameObjectDependency>	LocalOutputs;
		Vector<FrameObjectDependency>	LocalInputs;
		Vector<FrameObjectDependency>	Transitions;

		Vector<UpdateTask*>&			DataDependencies;
		FrameGraphResourceContext&		Context;
		FrameGraphNode&					Node;
		FrameResources*					Resources;
	};


	/************************************************************************************************/


	using DataDependencyList = Vector<UpdateTask*>;


	FLEXKITAPI class FrameGraph
	{
	public:
		FrameGraph(RenderSystem& RS, iAllocator* Temp) :
			Resources		{ RS, Temp },
			dataDependencies{ Temp },
			ResourceContext	{ Temp },
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

                TY      fields;
                DrawFN  draw;
            };

            auto& data  = Memory->allocate_aligned<NodeData>(std::move(std::forward<INITIAL_TY>(initial)), std::move(Draw));
			auto idx    = Nodes.emplace_back(
                FrameGraphNode{
                    [](
                    FrameGraphNode& node,
                    FrameResources& resources,
                    Context* ctx)
                    {
                        auto& data = *reinterpret_cast<NodeData*>(node.nodeData);

                        node.HandleBarriers(resources, ctx);
                        data.draw(data.fields, resources, ctx);
                        node.RestoreResourceStates(ctx, resources.SubNodeTracking);
                        data.fields.~TY();
                    },
                    &data,
                    Memory});

			FrameGraphNodeBuilder Builder(dataDependencies, &Resources, Nodes[idx], ResourceContext, Memory);

			Setup(Builder, data.fields);
			Builder.BuildNode(this);

			return data.fields;
		}


		void AddRenderTarget	(TextureHandle Texture);

		void ProcessNode		(FrameGraphNode* N, FrameResources& Resources, Context& Context);
		
		void UpdateFrameGraph	(RenderSystem* RS, RenderWindow* Window, iAllocator* Temp);// 
		void SubmitFrameGraph	(UpdateDispatcher& dispatcher, RenderSystem* RS, RenderWindow* Window);

		RenderSystem& GetRenderSystem() { return Resources.renderSystem; }

		FrameResources				Resources;
		FrameGraphResourceContext	ResourceContext;
		iAllocator*					Memory;
		Vector<FrameGraphNode>		Nodes;
		DataDependencyList			dataDependencies;

		void _SubmitFrameGraph(Vector<Context>& contexts);
	private:

		void ReadyResources();
		void UpdateResourceFinalState();
	};


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


	void ClearBackBuffer	(FrameGraph& Graph, TextureHandle backBuffer, float4 Color = {0.0f, 0.0f, 0.0f, 0.0f });// Clears BackBuffer to Black
	void ClearDepthBuffer	(FrameGraph& Graph, TextureHandle Handle, float D);
	void PresentBackBuffer	(FrameGraph& Graph, RenderWindow* Window);


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

		size_t ConstantBufferOffset	= 0;
		size_t VertexBufferOffset	= 0;
		size_t VertexCount			= 0;
		size_t VertexOffset			= 0;
		TextureHandle texture		= InvalidHandle_t;
	};

	typedef Vector<ShapeDraw> DrawList;


	/************************************************************************************************/


	struct Constants
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
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) = 0;
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
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			for (auto Shape : Shapes)
				Shape->AddShapeDraw(
					DrawList, 
					PushBuffer, 
					CB, 
					Resources);
		}

		FlexKit::Vector<ShapeProtoType*> Shapes;
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
			DrawList&				DrawList, 
			VertexBufferHandle		PushBuffer, 
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			float Step = 2 * pi / Divisions;
			for (size_t I = 0; I < Divisions; ++I)
			{
				float2 V1 = { POS.x + R * cos(Step * (I + 1)),	POS.y - AspectRatio * (R * sin(Step * (I + 1)))};
				float2 V2 = { POS.x + R * cos(Step * I),		POS.y - AspectRatio * (R * sin(Step * I)) };

				PushVertex(ShapeVert{ Position2SS(POS),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
				PushVertex(ShapeVert{ Position2SS(V1),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
				PushVertex(ShapeVert{ Position2SS(V2),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);
			}

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ ShapeDraw::RenderMode::Triangle, CBOffset, VBOffset, Divisions * 3});
		}

		float2	POS;
		float4	Color;
		float	R;
		float	AspectRatio;
		size_t	Divisions;
	};



	/************************************************************************************************/


	size_t LoadCameraConstants(CameraHandle camera, ConstantBufferHandle constantBuffer, FrameResources& resources)
	{

		auto CBOffset = BeginNewConstantBuffer(constantBuffer, resources);
		PushConstantBufferData(CameraComponent::GetComponent().GetCameraConstants(camera), constantBuffer, resources);

		return CBOffset;
	}


	/************************************************************************************************/


	template<typename TY>
	size_t LoadConstants(const TY& data, ConstantBufferHandle constantBuffer, FrameResources& resources)
	{
		auto CBOffset = BeginNewConstantBuffer(constantBuffer, resources);
		PushConstantBufferData(data, constantBuffer, resources);

		return CBOffset;
	}


	/************************************************************************************************/


	class LineShape final : public ShapeProtoType
	{
	public:
		LineShape(LineSegments& lines) : 
			Lines	{ lines } {}

		void AddShapeDraw(
			DrawList&				DrawList,
			VertexBufferHandle		PushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			for (auto Segment : Lines)
			{
				ShapeVert Vert1 = { Position2SS(Segment.A),{ 0.0f, 0.0f }, float4(Segment.AColour, 1) };
				ShapeVert Vert2 = { Position2SS(Segment.B),{ 0.0f, 0.0f }, float4(Segment.BColour, 1) };
				PushVertex(Vert1, PushBuffer, Resources);
				PushVertex(Vert2, PushBuffer, Resources);
			}

			Constants CB_Data = {
				float4{ 0 },
				float4{ 0 },
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ ShapeDraw::RenderMode::Line, CBOffset, VBOffset, Lines.size() * 2 });
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
			DrawList&				DrawList, 
			VertexBufferHandle		PushBuffer, 
			ConstantBufferHandle	CB,
			FrameResources&			Resources) override
		{
			float2 RectUpperLeft	= POS;
			float2 RectBottomRight	= POS + WH;
			float2 RectUpperRight	= { RectBottomRight.x,	RectUpperLeft.y };
			float2 RectBottomLeft	= { RectUpperLeft.x,	RectBottomRight.y };

			size_t VBOffset = Resources.GetVertexBufferOffset(PushBuffer, sizeof(ShapeVert));

			PushVertex(ShapeVert{ Position2SS(RectUpperLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);

			PushVertex(ShapeVert{ Position2SS(RectUpperLeft),	{ 0.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectUpperRight),	{ 1.0f, 1.0f }, Color }, PushBuffer, Resources);
			PushVertex(ShapeVert{ Position2SS(RectBottomRight),	{ 1.0f, 0.0f }, Color }, PushBuffer, Resources);

			Constants CB_Data = {
				Color,
				Color,
				float4x4::Identity()
			};

			auto CBOffset = BeginNewConstantBuffer(CB, Resources);
			PushConstantBufferData(CB_Data, CB, Resources);

			DrawList.push_back({ ShapeDraw::RenderMode::Triangle, CBOffset, VBOffset, 6 });
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
			DrawList&				drawList, 
			VertexBufferHandle		pushBuffer, 
			ConstantBufferHandle	CB,
			FrameResources&			resources) override
		{

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
		}

		Vector<FlexKit::Rectangle> rects;
	};


	/************************************************************************************************/

	using TextureList = Vector<TextureHandle>;

	class TexturedRectangleListShape final : public ShapeProtoType
	{
	public:
		TexturedRectangleListShape(RectangleList&& rects_in, TextureList&& textures_in) :
			rects		{ std::move(rects_in)		},
			textures	{ std::move(textures_in )	}{}


		~TexturedRectangleListShape() {}

		void AddShapeDraw(
			DrawList&				drawList,
			VertexBufferHandle		pushBuffer,
			ConstantBufferHandle	CB,
			FrameResources&			resources) override
		{
			FK_ASSERT(rects.size() == textures.size());
			if (rects.size() != textures.size())
				return;

			Constants CB_Data = {
			float4(1, 1, 1, 1),
			float4(1, 1, 1, 1),
			float4x4::Identity() };

			auto CBOffset = BeginNewConstantBuffer(CB, resources);
			PushConstantBufferData(CB_Data, CB, resources);

			const size_t VBOffset = resources.GetVertexBufferOffset(pushBuffer);
			size_t vertexOffset = 0;

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
		}

		Vector<Rectangle>		rects;
		Vector<TextureHandle>	textures;
	};


	/************************************************************************************************/


	inline void AddShapes(
		DrawList&				List, 
		VertexBufferHandle		VertexBuffer,
		ConstantBufferHandle	CB,
		FrameResources&			Resources) {}


	/************************************************************************************************/


	template<typename TY_1, typename ... TY_OTHER_SHAPES>
	void AddShapes(
		DrawList&				List, 
		VertexBufferHandle		VertexBuffer, 
		ConstantBufferHandle	CB,
		FrameResources&			Resources,
		TY_1&					Shape, 
		TY_OTHER_SHAPES ...		ShapePack)
	{
		Shape.AddShapeDraw(List, VertexBuffer, CB, Resources);
		AddShapes(List, VertexBuffer, CB, Resources, std::forward<TY_OTHER_SHAPES&&>(ShapePack)...);
	}


	/************************************************************************************************/


	template<typename ... TY_OTHER>
	void DrawShapes(
		PSOHandle State, 
		FrameGraph& Graph, 
		VertexBufferHandle PushBuffer,
		ConstantBufferHandle CB, 
		TextureHandle RenderTarget, 
		iAllocator* Memory, 
		TY_OTHER ... Args)
	{
		struct DrawRect
		{
			PSOHandle				State;
			FrameResourceHandle		RenderTarget;
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	ConstantBuffer;
			DescriptorHeap			descriptorTables;
			DrawList				Draws;
		};


		auto& Pass = Graph.AddNode<DrawRect>(
			DrawRect{},
			[&](FrameGraphNodeBuilder& Builder, DrawRect& Data)
			{
				// Single Thread Section
				// All Rendering Data Must be pushed into buffers here in advance, or allocated in advance
				// for thread safety

				Data.RenderTarget	= Builder.WriteRenderTarget(RenderTarget);
				Data.VertexBuffer	= PushBuffer;
				Data.ConstantBuffer = CB;
				Data.Draws			= DrawList(Memory);
				Data.State			= State;

				AddShapes(Data.Draws, PushBuffer, CB, Graph.Resources, std::forward<TY_OTHER&&>(Args)...);

				// Reserve needed Descriptor Table Space
				size_t textureCount = 0;
				for (const auto& Draw : Data.Draws)
					if (Draw.Mode == ShapeDraw::RenderMode::Textured)
						++textureCount;

				if (textureCount){
					auto& desciptorTableLayout	= Builder.GetDescriptorTableLayout(State, 0);
					Data.descriptorTables		= Builder.ReserveDescriptorTableSpaces(desciptorTableLayout, textureCount, Memory);
					Data.descriptorTables.NullFill(Graph.Resources.renderSystem);
				}
			},
			[=](const DrawRect& Data, const FrameResources& Resources, Context* Ctx)
			{	// Multi-threadable Section
				auto WH = Resources.GetRenderTargetWH(Data.RenderTarget);

				Ctx->SetScissorAndViewports({Resources.GetRenderTarget(Data.RenderTarget)});
				Ctx->SetRenderTargets(
					{	Resources.GetRenderTargetObject(Data.RenderTarget) }, 
					false);

				Ctx->SetRootSignature		(Resources.renderSystem.Library.RS6CBVs4SRVs);
				Ctx->SetPipelineState		(Resources.GetPipelineState(Data.State));
				Ctx->SetPrimitiveTopology	(EInputTopology::EIT_TRIANGLE);

				size_t TextureDrawCount = 0;
				ShapeDraw::RenderMode PreviousMode = ShapeDraw::RenderMode::Triangle;
				for (auto D : Data.Draws)
				{
					Ctx->SetVertexBuffers(VertexBufferList{ { Data.VertexBuffer, sizeof(ShapeVert), (UINT)D.VertexBufferOffset} });

					switch (D.Mode) {
						case ShapeDraw::RenderMode::Line:
						{
							Ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
						}	break;
						case ShapeDraw::RenderMode::Triangle:
						{
							Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
						}	break;
						case ShapeDraw::RenderMode::Textured:
						{
							auto table = Data.descriptorTables.GetHeapOffsetted(TextureDrawCount++, Resources.renderSystem);
							table.SetSRV(Resources.renderSystem, 0, D.texture);
							Ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLE);
							Ctx->SetGraphicsDescriptorTable(0, table);
							
						}	break;
					}

					Ctx->SetGraphicsConstantBufferView(2, Data.ConstantBuffer, D.ConstantBufferOffset);
					Ctx->Draw(D.VertexCount, D.VertexOffset);
					
					PreviousMode = D.Mode;
				}
			});
	} 



	/************************************************************************************************/


	struct DrawCollection_Desc
	{
		TriMeshHandle			Mesh;
		TextureHandle			RenderTarget;
		TextureHandle			DepthBuffer;
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
		TextureHandle			RenderTarget;
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

			size_t cameraOffset;
			size_t constantsOffset;
			size_t vertexCount;
			size_t vertexOffset;

			PSOHandle PSO;	
			FlexKit::DescriptorHeap	Heap; // Null Filled
		};
	

		struct Vertex
		{
			float4 POS;
			float4 Color;
			float2 UV;
		};

		frameGraph.AddNode<DrawWireframes>(
			DrawWireframes{},
			[&](FrameGraphNodeBuilder& Builder, auto& Data)
			{
				Data.RenderTarget	= Builder.WriteRenderTarget(frameGraph.Resources.renderSystem.GetTag(desc.RenderTarget));

				Data.CB		= desc.constantBuffer;
				Data.VB		= desc.VertexBuffer;
				Data.PSO	= desc.PSO;

				Data.Heap.Init(
					frameGraph.Resources.renderSystem,
					frameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
					TempMem).NullFill(frameGraph.Resources.renderSystem);

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

				auto cameraBuffer	= GetCameraConstants(desc.camera);
				auto pushBuffer		= desc.VertexBuffer;
				Data.vertexOffset	= frameGraph.Resources.GetVertexBufferOffset(pushBuffer);

				Data.constantsOffset	= BeginNewConstantBuffer(desc.constantBuffer,frameGraph.Resources);
				PushConstantBufferData(locals, desc.constantBuffer, frameGraph.Resources);
				Data.cameraOffset		= BeginNewConstantBuffer(desc.constantBuffer, frameGraph.Resources);
				PushConstantBufferData	(cameraBuffer, desc.constantBuffer, frameGraph.Resources);

				for (const Rectangle& rect : rects)
				{
					float4 upperLeft	= { rect.Position.x, 0, rect.Position.y,		1};
					float4 bottomRight	=   upperLeft + float4{rect.WH.x, 0, rect.WH.y, 0};
					float4 upperRight	= { bottomRight.x,	0, upperLeft.z,				1};
					float4 bottomLeft	= { upperLeft.x,	0, bottomRight.z,			1};

					// Top
					PushVertex(Vertex{ upperLeft,	rect.Color, { 0.0f, 1.0f } },	pushBuffer, frameGraph.Resources);
					PushVertex(Vertex{ upperRight,	rect.Color, { 1.0f, 1.0f } },	pushBuffer, frameGraph.Resources);

					// Right
					PushVertex(Vertex{ upperRight,	rect.Color, { 1.0f, 1.0f } },	pushBuffer, frameGraph.Resources);
					PushVertex(Vertex{ bottomRight,	rect.Color, { 1.0f, 0.0f } },	pushBuffer, frameGraph.Resources);

					// Bottom
					PushVertex(Vertex{ bottomRight,	rect.Color, { 0.0f, 0.0f } },	pushBuffer, frameGraph.Resources);
					PushVertex(Vertex{ bottomLeft,	rect.Color, { 1.0f, 0.0f } },	pushBuffer, frameGraph.Resources);

					// Left
					PushVertex(Vertex{ bottomLeft,	rect.Color, { 0.0f, 0.0f } },	pushBuffer, frameGraph.Resources);
					PushVertex(Vertex{ upperLeft,	rect.Color, { 0.0f, 1.0f } },	pushBuffer, frameGraph.Resources);
				}

				Data.vertexCount = rects.size() * 8;
			},
			[](auto& Data, const FrameResources& Resources, Context* ctx)
			{
				ctx->SetRootSignature(Resources.renderSystem.Library.RS6CBVs4SRVs);
				ctx->SetPipelineState(Resources.GetPipelineState(Data.PSO));
				ctx->SetVertexBuffers({ { Data.VB, sizeof(Vertex), (UINT)Data.vertexOffset } });

				ctx->SetRenderTargets(
					{ (DescHeapPOS)Resources.GetRenderTargetObject(Data.RenderTarget) }, false);

				ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
				ctx->SetGraphicsDescriptorTable		(0, Data.Heap);
				ctx->SetGraphicsConstantBufferView	(1, Data.CB, Data.cameraOffset);
				ctx->SetGraphicsConstantBufferView	(2, Data.CB, Data.constantsOffset);

				ctx->NullGraphicsConstantBufferView	(4);
				ctx->NullGraphicsConstantBufferView	(5);
				ctx->NullGraphicsConstantBufferView	(6);

				ctx->Draw(Data.vertexCount, 0);
			});
	}


	/************************************************************************************************/

	// Requires a registered DRAW_LINE3D_PSO pipeline state!
	void Draw3DGrid(
		FrameGraph&				FrameGraph,
		const size_t			ColumnCount,
		const size_t			RowCount,
		const float2			GridWH,
		const float4			GridColor,
		TextureHandle			RenderTarget,
		TextureHandle			DepthBuffer,
		VertexBufferHandle		VertexBuffer,
		ConstantBufferHandle	Constants,
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
			VertexBufferHandle		VertexBuffer;
			ConstantBufferHandle	CB;

			size_t					CameraConstantsOffset;
			size_t					LocalConstantsOffset;

			DescriptorHeap	Heap; // Null Filled
		};


		struct VertexLayout
		{
			float4 POS;
			float4 Color;
			float2 UV;
		};

		FrameGraph.AddNode<DrawGrid>(
			DrawGrid{},
			[&](FrameGraphNodeBuilder& Builder, auto& Data)
			{
				Data.RenderTarget	= Builder.WriteRenderTarget(RenderTarget);
				Data.DepthBuffer	= Builder.WriteDepthBuffer(DepthBuffer);
				Data.CB				= Constants;

				Data.CameraConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
				PushConstantBufferData(
					GetCameraConstants(Camera),
					Constants,
					FrameGraph.Resources);

				Data.Heap.Init(
					FrameGraph.Resources.renderSystem,
					FrameGraph.Resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
					TempMem);
				Data.Heap.NullFill(FrameGraph.Resources.renderSystem);

				Drawable::VConsantsLayout DrawableConstants = 
				{	// Someday
					/*.MP			= */Drawable::MaterialProperties{},
					/*.Transform	= */DirectX::XMMatrixIdentity()
				};

				Data.LocalConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
				PushConstantBufferData(
					DrawableConstants,
					Constants,
					FrameGraph.Resources);

				Data.VertexBuffer = VertexBuffer;
				Data.VertexBufferOffset = FrameGraph.Resources.GetVertexBufferOffset(VertexBuffer);

				// Fill Vertex Buffer Section
				for (auto& LineSegment : Lines)
				{
					VertexLayout Vertex;
					Vertex.POS		= float4(LineSegment.A, 1);
					Vertex.Color	= float4(LineSegment.AColour, 1) * float4 { 1.0f, 0.0f, 0.0f, 1.0f };
					Vertex.UV		= { 0.0f, 0.0f };

					PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);

					Vertex.POS		= float4(LineSegment.B, 1);
					Vertex.Color	= float4(LineSegment.BColour, 1) * float4 { 0.0f, 1.0f, 0.0f, 1.0f };
					Vertex.UV		= { 1.0f, 1.0f };

					PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);
				}

				Data.VertexCount = Lines.size() * 2;
			},
			[](auto& Data, const FlexKit::FrameResources& Resources, FlexKit::Context* Ctx)
			{
				Ctx->SetRootSignature(Resources.renderSystem.Library.RS6CBVs4SRVs);
				Ctx->SetPipelineState(Resources.GetPipelineState(DRAW_LINE3D_PSO));

				Ctx->SetScissorAndViewports({ Resources.GetRenderTarget(Data.RenderTarget) });
				Ctx->SetRenderTargets(
					{	(DescHeapPOS)Resources.GetRenderTargetObject(Data.RenderTarget) }, false,
						(DescHeapPOS)Resources.GetRenderTargetObject(Data.DepthBuffer));

				Ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
				Ctx->SetVertexBuffers(VertexBufferList{ { Data.VertexBuffer, sizeof(VertexLayout), (UINT)Data.VertexBufferOffset } });

				Ctx->SetGraphicsDescriptorTable(0, Data.Heap);
				Ctx->SetGraphicsConstantBufferView(1, Data.CB, Data.CameraConstantsOffset);
				Ctx->SetGraphicsConstantBufferView(2, Data.CB, Data.LocalConstantsOffset);

				Ctx->NullGraphicsConstantBufferView(3);
				Ctx->NullGraphicsConstantBufferView(4);
				Ctx->NullGraphicsConstantBufferView(5);
				Ctx->NullGraphicsConstantBufferView(6);

				Ctx->Draw(Data.VertexCount, 0);
			});
	}


}	/************************************************************************************************/
#endif
