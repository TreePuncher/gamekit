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
        virtualResourceState    virtualState = virtualResourceState::NonVirtual;

        union
        {
            struct {
                ResourceHandle	Texture;
                DescHeapPOS		HeapPOS;
            }RenderTarget;

            struct {
                UAVTextureHandle	handle;
                DescHeapPOS		    HeapPOS;
                bool                renderTargetUse;

                operator UAVTextureHandle () { return handle; }
            }UAVTexture;

            struct {
                ResourceHandle           handle;
                DescHeapPOS		        HeapPOS;
                bool                    renderTargetUse;

                operator ResourceHandle() { return handle; }
            }ShaderResource;

            QueryHandle				query;
            SOResourceHandle		SOBuffer;
            UAVResourceHandle		UAVBuffer;
            ResourceHandle			Texture;

            struct {
                char	buff[256];
            }Buffer;
        };


        static FrameObject ReadRenderTargetObject(ResourceHandle Handle)
        {
            FrameObject RenderTarget;
            RenderTarget.State					= DeviceResourceState::DRS_ShaderResource;
            RenderTarget.Type					= OT_RenderTarget;
            RenderTarget.RenderTarget.Texture	= Handle;

            return RenderTarget;
        }


        static FrameObject WriteRenderTargetObject(ResourceHandle Handle)
        {
            FrameObject RenderTarget;
            RenderTarget.State		            = DeviceResourceState::DRS_RenderTarget;
            RenderTarget.Type		            = OT_RenderTarget;
            RenderTarget.RenderTarget.Texture   = Handle;

            return RenderTarget;
        }


        static FrameObject BackBufferObject(ResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_RenderTarget)
        {
            FrameObject RenderTarget;
            RenderTarget.State					= InitialState;
            RenderTarget.Type					= OT_BackBuffer;
            RenderTarget.RenderTarget.Texture	= Handle;

            return RenderTarget;
        }


        static FrameObject DepthBufferObject(ResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_DEPTHBUFFER)
        {
            FrameObject RenderTarget;
            RenderTarget.State                = InitialState;
            RenderTarget.Type                 = OT_DepthBuffer;
            RenderTarget.RenderTarget.Texture = Handle;

            return RenderTarget;
        }


        static FrameObject TextureObject(ResourceHandle Handle, DeviceResourceState InitialState)
        {
            FrameObject shaderResource;
            shaderResource.State                    = InitialState;
            shaderResource.Type                     = OT_ShaderResource;
            shaderResource.ShaderResource.handle    = Handle;

            return shaderResource;
        }


        static FrameObject UAVBufferObject(UAVResourceHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
        {
            FrameObject UnorderedAccessViewObject;
            UnorderedAccessViewObject.State                = InitialState;
            UnorderedAccessViewObject.Type                 = OT_UAVBuffer;
            UnorderedAccessViewObject.UAVBuffer			   = Handle;

            return UnorderedAccessViewObject;
        }


        static FrameObject UAVTextureObject(UAVTextureHandle Handle, DeviceResourceState InitialState = DeviceResourceState::DRS_UAV)
        {
            FrameObject UnorderedAccessViewObject;
            UnorderedAccessViewObject.State                         = InitialState;
            UnorderedAccessViewObject.Type                          = OT_UAVTexture;
            UnorderedAccessViewObject.UAVTexture.handle             = Handle;
            UnorderedAccessViewObject.UAVTexture.renderTargetUse    = false;

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


    typedef Vector<FrameObject>		    PassObjectList;
    typedef Vector<FrameResourceHandle>	TemporaryPassObjectList;

    class FrameGraph;

    class MemoryPoolAllocator
    {
    public:
        MemoryPoolAllocator(RenderSystem&, DeviceHeapHandle IN_heap, size_t IN_blockCount, size_t IN_blockSize, iAllocator* IN_allocator);
        MemoryPoolAllocator(const MemoryPoolAllocator& rhs)             = delete;
        MemoryPoolAllocator& operator =(const MemoryPoolAllocator& rhs) = delete;

        struct _Leaf
        {
            size_t offset;
            size_t blockCount;
            size_t resource;
        };


        struct _Link
        {
            union
            {
                struct {
                    _Link*  left;
                    _Link*  right;
                    size_t  freeBlocks;
                };

                _Leaf   leaf;
            };


            size_t GetOffset() const
            {
                if (isLeaf)
                    return leaf.offset;
                else
                    return left->GetOffset();
            }

            size_t GetSize() const
            {
                if (isLeaf)
                    return leaf.blockCount;
                else
                    return left->GetSize() + right->GetSize();
            }

            bool free() const
            {
                return  isLeaf && ResourceHandle{ leaf.resource } == InvalidHandle_t;
            }

            _Link*              parent = nullptr;
            bool                isLeaf = true;
        };



        _Link* FindFreeLeaf(_Link& node, const size_t requestCount)
        {
            if (node.isLeaf)
            {
                if (!node.free())
                    return nullptr;

                if (node.leaf.blockCount / 2 > requestCount)
                {
                    _Link* left =
                        &allocator->allocate<_Link>(_Link{
                            .leaf = {
                                .offset     = node.leaf.offset,
                                .blockCount = node.leaf.blockCount / 2,
                                .resource   = ResourceHandle{ InvalidHandle_t }
                            },
                            .parent = &node,
                            .isLeaf = true
                            });

                    _Link* right =
                        &allocator->allocate<_Link>(_Link{
                            .leaf = {
                                .offset     = (uint32_t)(node.leaf.offset + left->leaf.blockCount),
                                .blockCount = node.leaf.blockCount - left->leaf.blockCount,
                                .resource   = ResourceHandle{ InvalidHandle_t }
                            },
                            .parent = &node,
                            .isLeaf = true
                            });

                    FK_ASSERT(left != nullptr);
                    FK_ASSERT(right != nullptr);

                    node.freeBlocks = node.leaf.blockCount;
                    node.isLeaf     = false;
                    node.left       = left;
                    node.right      = right;

                    auto res = FindFreeLeaf(*left, requestCount);
                    res = res == nullptr ? FindFreeLeaf(*right, requestCount) : res;

                    node.freeBlocks = res ? node.freeBlocks - res->leaf.blockCount : node.freeBlocks;

                    return res;

                }
                else if (node.leaf.blockCount > requestCount)
                {
                    _Link* left =
                        &allocator->allocate<_Link>(_Link{
                            .leaf = {
                                .offset     = node.leaf.offset,
                                .blockCount = (uint32_t)requestCount,
                                .resource   = ResourceHandle{ InvalidHandle_t }
                            },
                            .parent = &node,
                            .isLeaf = true
                            });

                    _Link* right =
                        &allocator->allocate<_Link>(_Link{
                            .leaf = {
                                .offset     = (uint32_t)(node.leaf.offset + requestCount),
                                .blockCount = node.leaf.blockCount - (uint32_t)requestCount,
                                .resource   = ResourceHandle{ InvalidHandle_t }
                                },
                            .parent = &node,
                            .isLeaf = true
                            });

                    FK_ASSERT(left != nullptr);
                    FK_ASSERT(right != nullptr);

                    node.freeBlocks = node.leaf.blockCount - requestCount;
                    node.isLeaf     = false;
                    node.left       = left;
                    node.right      = right;

                    return left;
                }
                else if (node.leaf.blockCount == requestCount)
                {
                    return &node;
                }
                else
                    return nullptr;
            }
            else
            {
                if (node.freeBlocks < requestCount)
                    return nullptr;

                auto res = FindFreeLeaf(*node.left, requestCount);

                if(!res)
                    res = FindFreeLeaf(*node.right, requestCount);

                if (!res)
                    return nullptr;

                node.freeBlocks -= res->leaf.blockCount;
                return res;
            }

            return nullptr;
        }


        struct HeapAllocation
        {
            size_t offset   = 0;
            size_t size     = 0;
        };


        ResourceHandle Aquire(GPUResourceDesc desc)
        {
            std::scoped_lock localLock{ lock };

            auto size = renderSystem.GetResourceSize(desc);
            const size_t requestedBlockCount = size / blockSize;

            auto node = FindFreeLeaf(rootNode, requestedBlockCount);

            if (!node)
                return InvalidHandle_t;

            desc.bufferCount    = 1;
            desc.allocationType = ResourceAllocationType::Placed;
            desc.placed.heap    = heap;
            desc.placed.offset  = node->leaf.offset * blockSize;

            auto resource = renderSystem.CreateGPUResource(desc);

            if (resource != InvalidHandle_t) {
                allocations.push_back({ node, resource });
                node->leaf.resource = resource;
            }

            return resource;
        }


        void Release(ResourceHandle handle, const bool releaseImmediate = true)
        {
            auto res = std::find_if(
                std::begin(allocations), std::end(allocations),
                [&](const auto& e) -> bool
                {
                    return e.resource == handle;
                });

            if (res != std::end(allocations))
            {
                res->node->leaf.resource = ResourceHandle{ InvalidHandle_t };

                if(releaseImmediate)
                    renderSystem.ReleaseTexture(handle);

                auto node = res->node->parent;

                while(node && node->left->free() && node->right->free())
                {
                    const auto offset   = node->GetOffset();
                    const auto size     = node->GetSize();

                    allocator->free(node->left);
                    allocator->free(node->right);

                    node->left  = nullptr;
                    node->right = nullptr;

                    node->isLeaf            = true;
                    node->leaf.blockCount   = size;
                    node->leaf.offset       = offset;
                    node->leaf.resource     = ResourceHandle{ InvalidHandle_t };

                    node = node->parent;
                }

                allocations.remove_unstable(res);
            }
        }

        _Link               rootNode;
        size_t              blockCount;
        size_t              blockSize;

        DeviceHeapHandle    heap;
        RenderSystem&       renderSystem;

        struct Allocation
        {
            _Link*          node;
            ResourceHandle  resource;
        };

        Vector<Allocation> allocations;

        std::mutex   lock;
        iAllocator*  allocator;
    };


    /************************************************************************************************/


    FLEXKITAPI class FrameResources
    {
    public:
        FrameResources(RenderSystem& IN_renderSystem, iAllocator* IN_allocator) : 
            Resources		    { IN_allocator		},
            SubNodeTracking	    { IN_allocator		},
            virtualResources    { IN_allocator      },
            renderSystem	    { IN_renderSystem	},
            memoryPools         { IN_allocator      },
            allocator           { IN_allocator      } {}

        PassObjectList			Resources;
        TemporaryPassObjectList virtualResources;



        Vector<MemoryPoolAllocator*> memoryPools;


        void AddMemoryPool(MemoryPoolAllocator* heapAllocator)
        {
            memoryPools.push_back(heapAllocator);
        }


        mutable PassObjectList	SubNodeTracking;
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
            Resources.back().ShaderResource.renderTargetUse = renderTarget;
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


        ID3D12Resource* GetUAVDeviceResource(FrameResourceHandle handle) const
        {
            auto res = _FindSubNodeResource(handle);
            return GetObjectResource(res->UAVBuffer);
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
            return Resources[Handle].RenderTarget.Texture;
        }


        /************************************************************************************************/


        DescHeapPOS GetRenderTargetDescHeapEntry(FrameResourceHandle Handle) const
        {
            return Resources[Handle].RenderTarget.HeapPOS;
        }


        /************************************************************************************************/


        ResourceHandle GetTexture(FrameResourceHandle Handle) const
        {
            return handle_cast<ResourceHandle>(Resources[Handle].ShaderResource.handle);
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
            auto deviceResource = renderSystem.GetDeviceResource(SOHandle);

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


        SOResourceHandle GetSOResource(FrameResourceHandle handle) const
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


        ResourceHandle ReadRenderTarget(FrameResourceHandle resource, Context* ctx) const
        {
            auto res = _FindSubNodeResource(resource);
            if (res->State != DRS_ShaderResource)
                ctx->AddShaderResourceBarrier(res->Texture, res->State, DRS_ShaderResource);

            res->State = DRS_ShaderResource;

            return res->ShaderResource;
        }


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


        ResourceHandle CopyToTexture(FrameResourceHandle resource, Context& ctx)
        {
            auto res = _FindSubNodeResource(resource);
            if (res->State != DRS_Write)
                ctx.AddCopyResourceBarrier(res->Texture, res->State, DRS_Write);

            return res->Texture;
        }


        UAVTextureHandle CopyUAVTexture(FrameResourceHandle resource, Context& ctx)
        {
            auto res = _FindSubNodeResource(resource);
            if (res->State != DRS_Read)
                ctx.AddUAVBarrier(res->UAVTexture, res->State, DRS_Read);

            return res->UAVTexture;
        }

        UAVResourceHandle CopyToUAV(FrameResourceHandle resource, Context& ctx)
        {
            auto res = _FindSubNodeResource(resource);
            if (res->State != DRS_Read)
                ctx.AddUAVBarrier(res->UAVBuffer, res->State, DRS_Write);

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
                renderSystem.GetDeviceResource(SOHandle)->GetGPUVirtualAddress(),
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

                    return (CorrectType && LHS.ShaderResource.handle.to_uint() == Handle.to_uint());
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

                    return (CorrectType && LHS.ShaderResource.handle == Handle);
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

                    return (CorrectType && LHS.UAVTexture.handle == handle);
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


        FrameResourceHandle	FindFrameResource(FrameResourceHandle handle)
        {
            auto res = find(Resources, [&](const FrameObject& LHS) { return LHS.Handle == handle; });

            if (res != Resources.end())
                return res->Handle;

            return InvalidHandle_t;
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

        uint2 GetTextureWH(FrameResourceHandle handle) const
        {
            if (auto res = GetTexture(handle); res != InvalidHandle_t)
                return renderSystem.GetTextureWH(res);
            else
                return { 0, 0 };
        }
    };/************************************************************************************************/


    /************************************************************************************************/


    class FrameGraphNode;

    struct FrameObjectDependency
    {
        FrameObjectDependency(
            FrameObject*		FO_IN				= nullptr,
            FrameGraphNode*		SourceObject_IN		= nullptr,
            DeviceResourceState	ExpectedState_IN	= DRS_UNKNOWN,
            DeviceResourceState	State_IN			= DRS_UNKNOWN) :
                FO				{ FO_IN },
                Source			{ SourceObject_IN },
                ExpectedState	{ ExpectedState_IN },
                State			{ State_IN }
        {
            SOHandle = InvalidHandle_t;
        }

        bool TransitionNeeded()
        {
            return ExpectedState != State;
        }

        FrameObject*        FO;
        FrameGraphNode*		Source;

        union {
            uint32_t				ID;	// Extra ID
            UAVResourceHandle		UAVBuffer;
            UAVTextureHandle		UAVTexture;
            SOResourceHandle		SOHandle;
            ResourceHandle          ShaderResource;
        };

        DeviceResourceState	ExpectedState;
        DeviceResourceState	State;
    };


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


    auto MakePred(ResourceHandle handle)
    {
        return [handle](FrameObjectDependency& lhs)
        {
            auto A = lhs.FO->Type == OT_BackBuffer || lhs.FO->Type == OT_RenderTarget || lhs.FO->Type == OT_ShaderResource;
            if (A && lhs.UAVTexture == InvalidHandle_t)
                lhs.ShaderResource = lhs.FO->Texture;

            return A && (lhs.ShaderResource == handle);
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


    auto MakePred(FrameResourceHandle handle)
    {
        return [handle](FrameObjectDependency& lhs)
        {
            return lhs.FO->Handle == handle;
        };
    }


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
        typedef void (*FN_NodeAction)(FrameGraphNode& node, FrameResources& Resources, Context& ctx, iAllocator& tempAllocator);

        FrameGraphNode(FN_NodeAction IN_action, void* IN_nodeData, iAllocator* IN_allocator = nullptr) :
            InputObjects	{ IN_allocator		    },
            OutputObjects	{ IN_allocator		    },
            Sources			{ IN_allocator		    },
            Transitions		{ IN_allocator		    },
            NodeAction		{ IN_action             },
            nodeData        { IN_nodeData           },
            Executed		{ false				    },
            RetiredObjects  { IN_allocator          }{}


        FrameGraphNode(const FrameGraphNode& RHS) :
            Sources			{ RHS.Sources		        },
            InputObjects	{ RHS.InputObjects	        },
            OutputObjects	{ RHS.OutputObjects	        },
            Transitions		{ RHS.Transitions	        },
            NodeAction		{ std::move(RHS.NodeAction)	},
            nodeData        { RHS.nodeData              },
            RetiredObjects  { RHS.RetiredObjects        },
            Executed		{ false				        } {}


        ~FrameGraphNode() = default;


        void HandleBarriers	(FrameResources& Resouces, Context& Ctx);
        void AddTransition	(FrameObjectDependency& Dep);

        void RestoreResourceStates(Context* ctx, PassObjectList& locallyTrackedObjects);
        void AcquireResources(FrameResources& resources, Context& ctx);
        void ReleaseResources(FrameResources& resources, Context& ctx);


        Vector<FrameGraphNode*>		GetNodeDependencies()	{ return (nullptr); } 

        void*                           nodeData;
        bool							Executed;
        FN_NodeAction					NodeAction;
        Vector<FrameGraphNode*>			Sources;// Nodes that this node reads from
        Vector<FrameObjectDependency>	InputObjects;
        Vector<FrameObjectDependency>	OutputObjects;
        Vector<FrameObjectDependency>	RetiredObjects;
        Vector<ResourceTransition>		Transitions;
    };


    /************************************************************************************************/


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


        void Retire(FrameObjectDependency retireMe)
        {

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
            Vector<UpdateTask*>&	    IN_DataDependencies,
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
            bool TrackedReadable = Context.IsTrackedReadable(handle);
            bool TrackedWritable = Context.IsTrackedWriteable(handle);

            if (!TrackedReadable && !TrackedWritable)
            {
                FrameResourceHandle Resource = Resources->FindFrameResource(handle);

                LocalInputs.push_back(
                    FrameObjectDependency{
                        Resources->GetAssetObject(Resource),
                        nullptr,
                        Resources->GetAssetObjectState(Resource),
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
                        Resources->GetAssetObject(Resource),
                        nullptr,
                        Resources->GetAssetObjectState(Resource),
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

                if (Object.ExpectedState != state)
                {
                    if(TrackedReadable)
                        Context.RemoveReadable(handle);
                    if (TrackedWritable)
                        Context.RemoveWriteable(handle);

                    Context.AddWriteable(Object);
                    Transitions.push_back(Object);
                }

                return Object.FO->Handle;
            }
        }

        FrameResourceHandle FrameGraphNodeBuilder::AddWriteableResource(ResourceHandle handle, DeviceResourceState state, FrameObjectResourceType type);

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
        Vector<FrameObjectDependency>	RetiredObjects;
        Vector<UpdateTask*>&	        DataDependencies;

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

                ~NodeData() = default;

                TY      fields;
                DrawFN  draw;
            };

            auto& data  = Memory->allocate_aligned<NodeData>(std::move(std::forward<INITIAL_TY>(initial)), std::move(Draw));
            auto idx = Nodes.emplace_back(
                FrameGraphNode{
                    [](
                    FrameGraphNode& node,
                    FrameResources& resources,
                    Context& ctx,
                    iAllocator& tempAllocator)
                    {
                        NodeData& data = *reinterpret_cast<NodeData*>(node.nodeData);

                        node.HandleBarriers(resources, ctx);
                        data.draw(data.fields, resources, ctx, tempAllocator);
                        node.RestoreResourceStates(&ctx, resources.SubNodeTracking);
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
        void AddMemoryPool      (MemoryPoolAllocator* allocator);

        void ProcessNode		(FrameGraphNode* N, FrameResources& Resources, Context& Context, iAllocator& allocator);
        
        void UpdateFrameGraph	(RenderSystem* RS, iAllocator* Temp);// 
        void SubmitFrameGraph	(UpdateDispatcher& dispatcher, RenderSystem* RS, iAllocator& allocator);

        RenderSystem& GetRenderSystem() { return Resources.renderSystem; }


        FrameResources				Resources;
        FrameGraphResourceContext	ResourceContext;
        iAllocator*					Memory;
        Vector<FrameGraphNode>		Nodes;
        DataDependencyList			dataDependencies;


        void _SubmitFrameGraph(Vector<Context*>& contexts, iAllocator& allocator);
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
            [](const PassData& Data, const FrameResources& Resources, Context& ctx, iAllocator&)
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
            [=](const ShapeParams& Data, const FrameResources& frameResources, Context& context, iAllocator& allocator)
            {	// Multi-threadable Section
                auto WH = frameResources.GetTextureWH(Data.RenderTarget);

                context.SetScissorAndViewports({ frameResources.GetRenderTarget(Data.RenderTarget)} );
                context.SetRenderTargets(
                    { frameResources.GetRenderTarget(Data.RenderTarget) },
                    false);

                context.SetRootSignature		(frameResources.renderSystem.Library.RS6CBVs4SRVs);
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
                            auto& desciptorTableLayout = frameResources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0);

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
            [](auto& Data, const FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                DescriptorHeap descHeap;
                descHeap.Init(
                    ctx,
                    resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator);
                descHeap.NullFill(ctx);

                ctx.SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
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
            [](auto& Data, const FrameResources& resources, Context& ctx, iAllocator& allocator)
            {
                DescriptorHeap descHeap;
                descHeap.Init(
                    ctx,
                    resources.renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                    &allocator);
                descHeap.NullFill(ctx);

                ctx.SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
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
            [](const _Clear& data, FrameResources& resources, Context& ctx, iAllocator&)
            {
                /*
                struct _Constants
                {
                    uint4 constants = {};
                }constants{ { value[0], value[1], value[0], value[1] } };

                ctx.SetRootSignature(resources.renderSystem.Library.RSDefault);
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
