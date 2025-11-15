#include "vkRenderSystem.hpp"
#include "vkPushBuffers.hpp"

namespace VK_internal
{


    VertexBufferHandle vkVertexPushBuffers::CreateBuffer(uint32_t size, bool gpuResident)
    {
        auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());

        auto res0 = CreateVertexBuffer(vkRS, size, gpuResident);
        auto res1 = CreateVertexBuffer(vkRS, size, gpuResident);
        auto res2 = CreateVertexBuffer(vkRS, size, gpuResident);

        if (!res0 || !res1 || !res2)
        {
            if (res0)
            {
                auto [buffer, memory, offset] = res0.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            if (res1)
            {
                auto [buffer, memory, offset] = res1.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            if (res2)
            {
                auto [buffer, memory, offset] = res2.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            return InvalidHandle;
        }

        auto [buffer0, memory0, offset0] = res0.value();
        auto [buffer1, memory1, offset1] = res1.value();
        auto [buffer2, memory2, offset2] = res2.value();

        auto GetBufferPointer = [&] () -> std::byte*
            {
                if (gpuResident)
                    return nullptr;
                else
                    return vkRS.MapDeviceAddress(memory0, offset0);
            };

        std::unique_lock lock{ mutex };

        const auto handle = handles.GetNewHandle(fields.size());
        fields.push_back(
                VertexBuffer{ .used = 0, .size = size, .buffer = GetBufferPointer(), },
                gpuResident ? FlagBits::gpuResident : 0,
                { APIObjects{
                    .buffers        = { buffer0, buffer1, buffer2 },
                    .memory         = { memory0, memory2, memory2 },
                    .locks          = { 0u, 0u, 0u },
                    .memoryOffset   = { offset0, offset1, offset2 },
                    .current        = 0 }
                },
                handle);

        return handle;
    }

    void vkVertexPushBuffers::ReleaseBuffer(VertexBufferHandle handle)
    {
        if (fields.size() == 0)
        {
            FK_LOG_ERROR("VK: trying to release invalid handle!");
            return;
        }

        std::unique_lock lock{ mutex };

        if (auto idx = handles[handle]; fields.size() - 1 != idx)
        {
            auto temp   = fields.Get<PushBufferFields::Handle>(fields.size() - 1);
            fields[idx] = fields.back();
            handles[temp] = idx;
        }

        fields.pop_back();
    }

    uint32_t vkVertexPushBuffers::GetOffset(VertexBufferHandle handle) const
    {
        std::shared_lock lock{ const_cast<std::shared_mutex&>(mutex) };
        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get<PushBufferFields::BufferState>(idx);
        return used;
    }

    bool vkVertexPushBuffers::Push(VertexBufferHandle handle, void* _ptr, size_t size)
    {
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);

        auto alignedSize = AlignedSize(size);

        // double check
        if (used + alignedSize >= bufferSize) // relaxed check
            return false;

        auto begin = std::atomic_ref(used).fetch_add(alignedSize);
        if (begin + alignedSize > bufferSize) // atomic check plus attempt to allocate space
            return false;

        memcpy(buffer + begin, _ptr, size);
        return true;
    }

    void vkVertexPushBuffers::Reset(VertexBufferHandle handle, uint64_t id)
    {
        auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);
        auto&& [buffers, memory, locks, memoryOffset, current] = fields.Get_ref<PushBufferFields::apiObjects>(idx);
        auto flag = fields.Get<PushBufferFields::Flags>(idx);

        if ((flag & FlagBits::gpuResident) == 0)
            vkRS.UnMapDeviceAddress(memory[current]);

        locks[current] = vkRS.GetCurrentCounter();
        current = ++current % 3;
        used = 0;

        if ((flag & FlagBits::gpuResident) == 0)
            buffer = vkRS.MapDeviceAddress(memory[current], memoryOffset[current]);
    }

    SubAllocation vkVertexPushBuffers::Reserve(VertexBufferHandle handle, uint64_t reserveSize)
    {
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);

        auto alignedSize = AlignedSize(reserveSize);

        // double check
        if (used + alignedSize >= bufferSize) // relaxed check
            return SubAllocation{ nullptr, 0, 0 };

        auto begin = std::atomic_ref(used).fetch_add(alignedSize);
        if (begin + alignedSize > bufferSize) // atomic check plus attempt to allocate space
            return SubAllocation{ nullptr, 0, 0 };

        return SubAllocation{ (char*)buffer, begin, reserveSize };
    }

    ConstantBufferHandle vkConstantPushBuffers::CreateBuffer(uint32_t size, bool gpuResident)
    {
        auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());

        auto res0 = CreateConstantBuffer(vkRS, size, gpuResident);
        auto res1 = CreateConstantBuffer(vkRS, size, gpuResident);
        auto res2 = CreateConstantBuffer(vkRS, size, gpuResident);

        if (!res0 || !res1 || !res2)
        {
            if (res0)
            {
                auto [buffer, memory, offset] = res0.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            if (res1)
            {
                auto [buffer, memory, offset] = res1.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            if (res2)
            {
                auto [buffer, memory, offset] = res2.value();
                vkDestroyBuffer(vkRS.device, buffer, nullptr);
                vkRS.memoryAllocator.Release(memory);
            }
            return InvalidHandle;
        }

        auto [buffer0, memory0, offset0] = res0.value();
        auto [buffer1, memory1, offset1] = res1.value();
        auto [buffer2, memory2, offset2] = res2.value();

        auto GetBufferPointer = [&] () -> std::byte*
            {
                if (gpuResident)
                    return nullptr;
                else
                {
                    return vkRS.MapDeviceAddress(memory0, offset0);
                }
            };

        std::unique_lock lock{ mutex };

        const auto handle = handles.GetNewHandle(fields.size());
        fields.push_back(
                ConstantBuffer{ .used = 0, .size = size, .buffer = GetBufferPointer(), },
                gpuResident ? FlagBits::gpuResident : 0,
                { APIObjects{
                    .buffers        = { buffer0, buffer1, buffer2 },
                    .memory         = { memory0, memory1, memory2 },
                    .locks          = { 0u, 0u, 0u },
                    .memoryOffset   = { offset0, offset1, offset2 }, }
                },
                handle);

        return handle;
    }

    void vkConstantPushBuffers::ReleaseBuffer(ConstantBufferHandle handle)
    {
        if (fields.size() == 0)
        {
            FK_LOG_ERROR("VK: trying to release invalid handle!");
            return;
        }

        std::unique_lock lock{ mutex };

        if (auto idx = handles[handle]; fields.size() - 1 != idx)
        {
            auto temp   = fields.Get<PushBufferFields::Handle>(fields.size() - 1);
            fields[idx] = fields.back();
            handles[temp] = idx;
        }

        fields.pop_back();
    }

    uint32_t vkConstantPushBuffers::GetOffset(ConstantBufferHandle handle) const
    {
        std::shared_lock lock{ const_cast<std::shared_mutex&>(mutex) };
        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get<PushBufferFields::BufferState>(idx);
        return used;
    }

    bool vkConstantPushBuffers::Push(ConstantBufferHandle handle, void* _ptr, size_t size)
    {
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);

        auto alignedSize = AlignedSize(size);

        // double check
        if (used + alignedSize >= bufferSize) // relaxed check
            return false;

        auto begin = std::atomic_ref(used).fetch_add(alignedSize);
        if (begin + alignedSize > bufferSize) // atomic check plus attempt to allocate space
            return false;

        memcpy(buffer + begin, _ptr, size);
        return true;
    }

    void vkConstantPushBuffers::Reset(ConstantBufferHandle handle, uint64_t id)
    {
        auto& vkRS = static_cast<vkRenderSystem&>(IRenderSystem::GetInstance());
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);
        auto&& [buffers, memory, locks, memoryOffset, current] = fields.Get_ref<PushBufferFields::apiObjects>(idx);
        auto flag = fields.Get<PushBufferFields::Flags>(idx);

        if ((flag & FlagBits::gpuResident) == 0)
            vkRS.UnMapDeviceAddress(memory[current]);

        locks[current] = vkRS.GetCurrentCounter();
        current = ++current % 3;
        used = 0;

        if ((flag & FlagBits::gpuResident) == 0)
            buffer = vkRS.MapDeviceAddress(memory[current], memoryOffset[current]);
    }

    SubAllocation vkConstantPushBuffers::Reserve(ConstantBufferHandle handle, uint64_t reserveSize)
    {
        std::shared_lock lock{ mutex };

        auto idx = handles[handle];
        auto&& [used, bufferSize, buffer] = fields.Get_ref<PushBufferFields::BufferState>(idx);

        auto alignedSize = AlignedSize(reserveSize);

        // double check
        if (used + alignedSize >= bufferSize) // relaxed check
            return SubAllocation{ nullptr, 0, 0 };

        auto begin = std::atomic_ref(used).fetch_add(alignedSize);
        if (begin + alignedSize > bufferSize) // atomic check plus attempt to allocate space
            return SubAllocation{ nullptr, 0, 0 };

        return SubAllocation{ (char*)buffer, begin, reserveSize };
    }

    VkBuffer vkConstantPushBuffers::GetAPIBuffer(ConstantBufferHandle handle)
    {
        std::shared_lock lock{ mutex };
        auto idx = handles[handle];
        auto&& [buffers, memory, locks, memoryOffset, current] = fields.Get_ref<PushBufferFields::apiObjects>(idx);

        return buffers[current];
    }

}
