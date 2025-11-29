#include "vkDescriptorSet.hpp"
#include "vkDirectContext.hpp"
#include <vkRenderSystem.hpp>

#include "PushBuffers.hpp"

namespace VK_internal
{
    vkDescriptorSet::vkDescriptorSet(struct vkRenderSystem& vkRS)
    {
    }

    vkDescriptorSet::~vkDescriptorSet()
    {
    }

    IDescriptorHeap& vkDescriptorSet::operator=(IDescriptorHeap&&)
    {
        return *this;
    }

    void vkDescriptorSet::Init(IContext& ctx, const DescriptorHeapLayout& layout_IN, iAllocator& TempMemory)
    {
        auto& vkRS  = GetVKRS();
        auto& vkCtx = static_cast<vkDirectContext&>(ctx);
        layout      = &layout_IN;
        apiLayout   = layout_IN.deviceLayout.As<VkDescriptorSetLayout_T>();
        
        vkGetDescriptorSetLayoutSize(vkRS.device, apiLayout, &size);

        auto res = vkRS.heapAllocator.Alloc2Temp(static_cast<uint32_t>(size), vkRS.GetCurrentProgress(), vkCtx.dispatchValue);
        if (!res)
        {
            FK_LOG_ERROR("VK: Failed to allocate descriptor range! size %u", size);
            throw std::runtime_error("VK: Failed to allocate descriptor range! ");
        }

        auto [range, offset] = res.value();
        descriptorBuffer    = (std::byte*)range.begin.V1.to_uint();
        bufferOffset        = offset;
    }

    void vkDescriptorSet::Init(IContext& ctx, const DescriptorHeapLayout& layout_IN, const size_t reserveCount, iAllocator& TempMemory)
    {
        auto& vkRS = GetVKRS();
        apiLayout = layout_IN.deviceLayout.As<VkDescriptorSetLayout_T>();

        size = reserveCount * 64;
        auto res = ctx.GetRenderSystem().CreateDescriptorRange(size);
        if (!res)
        {
            FK_LOG_ERROR("VK: Failed to allocate descriptor range! size %u", size);
            throw std::runtime_error("VK: Failed to allocate descriptor range! ");
        }

        auto [begin, bufferSize, stride] = res.value();
        descriptorBuffer = (std::byte*)begin.V1.to_uint();
    }

    void vkDescriptorSet::Init2(IContext& ctx, const DescriptorHeapLayout& layout_IN, const size_t reserveCount, iAllocator& TempMemory)
    {
        auto& vkRS = GetVKRS();
        apiLayout = layout_IN.deviceLayout.As<VkDescriptorSetLayout_T>();

        size = reserveCount * 64;
        auto res = ctx.GetRenderSystem().CreateDescriptorRange(size);
        if (!res)
        {
            FK_LOG_ERROR("VK: Failed to allocate descriptor range! size %u", size);
            throw std::runtime_error("VK: Failed to allocate descriptor range! ");
        }

        auto [begin, bufferSize, stride] = res.value();
        descriptorBuffer = (std::byte*)begin.V1.to_uint();
    }

    void vkDescriptorSet::NullFill(IContext& ctx, const size_t end)
    {
    }

    void vkDescriptorSet::SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants)
    {
        auto& vkRS = GetVKRS();

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, idx, &offset);

        auto buffer = vkRS.constantPushBuffers.GetAPIBuffer(constants.Handle());
        VkBufferDeviceAddressInfo getAddressInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext  = nullptr,
            .buffer = buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + offset,
            .range      = vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            .format     = VkFormat::VK_FORMAT_UNDEFINED
        };

        VkDescriptorGetInfoEXT getInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext  = nullptr,
            .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .data {
                .pUniformBuffer = &bufferInfo
            }
        };

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            descriptorBuffer + offset);
    }

    void vkDescriptorSet::SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle handle, size_t offset, size_t bufferSize)
    {
    }

    void vkDescriptorSet::SetCBV(IContext& ctx, size_t idx, ResourceHandle handle, size_t offset, size_t bufferSize)
    {
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle)
    {
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetSRV3D(IContext& ctx, size_t idx, ResourceHandle)
    {
    }

    void vkDescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle)
    {
    }

    void vkDescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t offset)
    {
    }

    void vkDescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle)
    {
    }

    void vkDescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
    {
    }

    void vkDescriptorSet::SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
    {
    }

    void vkDescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset)
    {
    }

    void vkDescriptorSet::SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
    {
    }

    DevicePointer vkDescriptorSet::GetGPUDescriptorHandle() const
    {
        return {};
    }

    DescriptorSet vkDescriptorSet::GetHeapOffsetted(size_t offset, IContext& ctx) const
    {
        return {};
    }

    vkRenderSystem& vkDescriptorSet::GetVKRS() noexcept
    {
        return static_cast<vkRenderSystem&>(vkRenderSystem::GetInstance());
    }
}


/**********************************************************************

Copyright (c) 2025 Robert May

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
