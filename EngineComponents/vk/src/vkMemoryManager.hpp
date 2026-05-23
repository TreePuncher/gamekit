#pragma once
#include <Containers.hpp>
#include <expected>
#include <MemoryUtilities.hpp>
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

    class vkRenderSystem;

    struct vkAllocation
    {
        uint32_t        offset;
        VkDeviceMemory  memory;
    };

    enum class AllocationError
    {
        OutOfMemory,
        UnknownError
    };

    enum class AllocationState
    {
        Free,
        Allocated
    };

    class vkMemoryAllocator
    {
    public:

        struct SlabRange
        {
            uint64_t offset;
            uint64_t size;
        };

        struct Slab
        {
            uint32_t            flags;
            uint32_t            typeBit;
            AllocationState     state;
            uint64_t            size;
            uint64_t            used;
            VkDeviceMemory      memory;
            Vector<SlabRange>   allocations;
        };

        vkMemoryAllocator(iAllocator& allocator);

        void                                            Init(vkRenderSystem& renderSystem);
        std::expected<vkAllocation, AllocationError>    Allocate(uint32_t flags, uint32_t heapFlags, uint64_t size, uint32_t alignment = 16);
        std::expected<vkAllocation, AllocationError>    Allocate2(uint32_t usableHeaps, uint32_t heapFlags, uint64_t size, uint32_t alignment = 16);
        void                                            Release(VkDeviceMemory);

        void  CreateSlab(uint32_t flags, uint64_t size, uint32_t heapMask = 0xff);
        Slab* FindSlab(uint32_t heapFlags, uint64_t requiredSize, uint32_t alignment = 1, uint32_t heapMask = 0xff);


        Vector<Slab>                        slabs;
        VkPhysicalDeviceMemoryProperties2   properties;

        VkDevice    device;
        iAllocator* allocator;
    };
}
