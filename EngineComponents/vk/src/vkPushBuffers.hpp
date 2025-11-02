#pragma once
#include <MultiField.hpp>
#include <Handle.hpp>
#include <MemoryUtilities.hpp>
#include <mutex>

namespace VK_internal
{
    using namespace FlexKit;

    struct vkVertexPushBuffers
    {
        enum PushBufferFields
        {
            BufferState = 0,
            Flags       = 1,
            apiObjects  = 2,
            Handle      = 3,
        };

        struct VertexBuffer
        {
            uint32_t    used;
            uint32_t    size;
            std::byte*  buffer;
        };

        enum FlagBits
        {
            none        = 0,
            gpuResident = 1,
        };

        struct APIObjects
        {
            VkBuffer        buffers[3];
            VkDeviceMemory  memory[3];
            uint64_t        locks[3];
            uint32_t        memoryOffset[3];
            uint32_t        current;
        };

        vkVertexPushBuffers(iAllocator& allocator) :
            handles { allocator },
            fields  { allocator } {}


        VertexBufferHandle  CreateBuffer(uint32_t size, bool gpuResident);
        void                ReleaseBuffer(VertexBufferHandle);

        uint32_t            GetOffset(VertexBufferHandle) const;
        
        bool                Push(VertexBufferHandle, void*, size_t);
        void                Reset(VertexBufferHandle, uint64_t id);
        SubAllocation       Reserve(VertexBufferHandle, uint64_t reserveSize);

        using Fields_TY = MultiField<VertexBuffer, uint32_t, APIObjects, VertexBufferHandle>;

        HandleUtilities::HandleTable<VertexBufferHandle>        handles;
        Fields_TY                                               fields;
        std::shared_mutex                                       mutex;
    };

    struct vkConstantPushBuffers
    {
        enum PushBufferFields
        {
            BufferState = 0,
            Flags       = 1,
            apiObjects  = 2,
            Handle      = 3,
        };

        struct ConstantBuffer
        {
            uint32_t    used;
            uint32_t    size;
            std::byte*  buffer;
        };

        enum FlagBits
        {
            none        = 0,
            gpuResident = 1,
        };

        struct APIObjects
        {
            VkBuffer        buffers[3];
            VkDeviceMemory  memory[3];
            uint64_t        locks[3];
            uint32_t        memoryOffset[3];
            uint32_t        current;
        };

        vkConstantPushBuffers(iAllocator& allocator) :
            handles { allocator },
            fields  { allocator } {}


        ConstantBufferHandle    CreateBuffer(uint32_t size, bool gpuResident);
        void                    ReleaseBuffer(ConstantBufferHandle);

        uint32_t                GetOffset(ConstantBufferHandle) const;
        
        bool                    Push(ConstantBufferHandle, void*, size_t);
        void                    Reset(ConstantBufferHandle, uint64_t id);
        SubAllocation           Reserve(ConstantBufferHandle, uint64_t reserveSize);

        using Fields_TY = MultiField<ConstantBuffer, uint32_t, APIObjects, ConstantBufferHandle>;

        HandleUtilities::HandleTable<ConstantBufferHandle>        handles;
        Fields_TY                                               fields;
        std::shared_mutex                                       mutex;
    };

}
