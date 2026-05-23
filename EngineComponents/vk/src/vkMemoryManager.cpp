#include "vkMemoryManager.hpp"
#include "vkRenderSystem.hpp"

namespace VK_internal
{
    vkMemoryAllocator::vkMemoryAllocator(iAllocator& IN_allocator) :
        allocator{ IN_allocator },
        properties{
            .sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
            .pNext = nullptr,
        },
        slabs{ IN_allocator } {}

    void vkMemoryAllocator::Init(vkRenderSystem& renderSystem)
    {
        device = renderSystem.device;

        vkGetPhysicalDeviceMemoryProperties2(renderSystem.device.physical_device, &properties);
    }

    std::expected<vkAllocation, AllocationError> vkMemoryAllocator::Allocate(uint32_t flags, uint32_t heapFlags, uint64_t size, uint32_t alignment)
    {
        auto alignedSize = AlignedSize(size, alignment);

        auto slab = FindSlab(heapFlags, alignedSize);

        // for now using linear allocators
        auto address = slab->used;
        auto alignedOffset = Align(address, alignment);
        slab->used = alignedOffset + alignedSize;

        slab->allocations.push_back(SlabRange{
            .offset = address,
            .size = size });

        vkAllocation allocation{
            .offset = (uint32_t)alignedOffset,
            .memory = slab->memory,
        };

        return allocation;
    }

    std::expected<vkAllocation, AllocationError> vkMemoryAllocator::Allocate2(uint32_t usabletypes, uint32_t heapFlags, uint64_t size, uint32_t alignment)
    {
        auto alignedSize = AlignedSize(size, alignment);

        auto slab = FindSlab(heapFlags, alignedSize, 1, usabletypes);

        // for now using linear allocators
        auto address        = slab->used;
        auto alignedOffset  = Align(address, alignment);
        slab->used          = alignedOffset + alignedSize;

        slab->allocations.push_back(SlabRange{
            .offset = address,
            .size = size });

        vkAllocation allocation{
            .offset = (uint32_t)alignedOffset,
            .memory = slab->memory,
        };

        return allocation;
    }


    void vkMemoryAllocator::Release(VkDeviceMemory memory)
    {
        for (auto& slab : slabs)
        {
            if ((uint64_t)slab.memory < (uint64_t)memory && (uint64_t)memory < ((uint64_t)slab.memory + slab.size))
            {// TODO: Handle free
            }
        }
    }

    void  vkMemoryAllocator::CreateSlab(uint32_t requiredBits, uint64_t requiredSize, uint32_t heapMask)
    {
        uint32_t    typeCount   = properties.memoryProperties.memoryTypeCount;
        auto*       types       = properties.memoryProperties.memoryTypes;

        unsigned long index;

        #if WIN32
        _BitScanReverse64(&index, requiredSize);
        #else
        index = ceil(log2(requiredSize));
        #endif
        
        uint64_t size = 0x1 << (index + 2);

        VkMemoryAllocateInfo allocateInfo{
            .sType              = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext              = nullptr,
            .allocationSize     = size,
            .memoryTypeIndex    = -1u 
        };

        for (uint32_t i = 0; i < typeCount; i++)
        {
            if ((types[i].propertyFlags & requiredBits) == requiredBits && (heapMask & (0x1 << i)))
            {
                allocateInfo.memoryTypeIndex = i;
                break;
            }
        }

        if (allocateInfo.memoryTypeIndex == -1u)
            throw std::runtime_error{ "VK: Failed to find usable memory heap" };

        VkDeviceMemory memory;
        if (auto res = vkAllocateMemory(device, &allocateInfo, nullptr, &memory); res != VK_SUCCESS)
        {
            FK_LOG_ERROR("VK: Failed to allocate memory! Size: %uz", requiredSize);
            throw std::runtime_error{ "VK: Failed to to allocate memory" };
        }

        slabs.push_back(Slab{
                .flags          = types[allocateInfo.memoryTypeIndex].propertyFlags,
                .typeBit        = uint32_t(0x1 << allocateInfo.memoryTypeIndex),
                .size           = size,
                .used           = 0,
                .memory         = memory,
                .allocations    = Vector<SlabRange>{ allocator }
        });
    }

    vkMemoryAllocator::Slab* vkMemoryAllocator::FindSlab(uint32_t heapFlags, uint64_t requiredSize, uint32_t alignment, uint32_t typeMask)
    {
        for (auto& slab : slabs)
        {
            if ((slab.typeBit & typeMask) && (slab.flags & heapFlags) == heapFlags && slab.size > AlignedSize(slab.used, alignment) + requiredSize)
                return &slab;
        }
        auto neededSize = Max(32 * MEGABYTE,  1 << ((uint64_t)(std::ceil(std::log2(requiredSize)))));
        
        // if no slabs create one
        CreateSlab(heapFlags, neededSize, typeMask);

        // Try again
        return FindSlab(heapFlags, requiredSize, alignment, typeMask);
    }
}
