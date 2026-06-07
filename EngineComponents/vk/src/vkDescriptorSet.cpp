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

    void vkDescriptorSet::Init(IContext& ctx, const DescriptorSetLayout& layout_IN, iAllocator& TempMemory)
    {
        auto& vkRS  = GetVKRS();
        auto& vkCtx = static_cast<vkDirectContext&>(ctx);
        layout      = &layout_IN;
        apiLayout   = layout_IN.deviceLayout.As<VkDescriptorSetLayout_T>();
        
        vkGetDescriptorSetLayoutSize(vkRS.device, apiLayout, &size);

        auto res = vkRS.heapAllocator.Alloc2Temp(static_cast<uint32_t>(size), vkRS.GetCurrentProgress(), vkCtx.dispatchValue, vkRS.descriptorBufferProperties.descriptorBufferOffsetAlignment);
        if (!res)
        {
            FK_LOG_ERROR("VK: Failed to allocate descriptor range! size %u", size);
            throw std::runtime_error("VK: Failed to allocate descriptor range! ");
        }

        auto [range, offset] = res.value();
        descriptorBuffer    = (std::byte*)range.begin.V1.to_uint();
        bufferOffset        = offset;
    }

    void vkDescriptorSet::Init(IContext& ctx, const DescriptorSetLayout& layout_IN, const size_t reserveCount, iAllocator& TempMemory)
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

    void vkDescriptorSet::Init2(IContext& ctx, const DescriptorSetLayout& layout_IN, const size_t reserveCount, iAllocator& TempMemory)
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
            .address    = address + constants.Offset(),
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

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            descriptorBuffer + offset);
    }

    void vkDescriptorSet::SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle handle, size_t bufferOffset, size_t bufferSize)
    {
        auto& vkRS = GetVKRS();

        auto buffer = vkRS.constantPushBuffers.GetAPIBuffer(handle);
        VkBufferDeviceAddressInfo getAddressInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext  = nullptr,
            .buffer = buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + bufferOffset,
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

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            descriptorBuffer + offset);
    }

    void vkDescriptorSet::SetCBV(IContext& ctx, size_t idx, ResourceHandle handle, size_t bufferOffset, size_t bufferSize)
    {
        auto& vkRS = GetVKRS();

        const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

        FK_ASSERT(dimension == FlexKit::TextureDimension::Buffer);

        VkBufferDeviceAddressInfo getAddressInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext      = nullptr,
            .buffer     = object.buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + bufferOffset,
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

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            descriptorBuffer + offset);
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle)
    {
        auto& vkRS = GetVKRS();

        const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

        switch (dimension)
        {
        case TextureDimension::Buffer:
        {
            auto& vkRS = GetVKRS();

            const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

            FK_ASSERT(dimension == FlexKit::TextureDimension::Buffer);

            VkBufferDeviceAddressInfo getAddressInfo{
                .sType      = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .pNext      = nullptr,
                .buffer     = object.buffer
            };

            const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

            VkDescriptorAddressInfoEXT bufferInfo{
                .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext      = nullptr,
                .address    = address + bufferOffset,
                .range      = vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
                .format     = VkFormat::VK_FORMAT_UNDEFINED
            };

            VkDescriptorGetInfoEXT getInfo{
                .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext  = nullptr,
                .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .data {
                    .pUniformBuffer = &bufferInfo
                }
            };

            size_t offset;
            vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

            vkGetDescriptor(
                vkRS.device, &getInfo,
                vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
                descriptorBuffer + offset);
        }   break;
        case TextureDimension::Texture1D:
        case TextureDimension::Texture2D:
        case TextureDimension::Texture3D:
        case TextureDimension::Texture2DArray:
        case TextureDimension::TextureCubeMap:
        {
            FK_ASSERT(dimension != FlexKit::TextureDimension::Buffer);
            auto& vkRS = GetVKRS();

            const auto [layout, view, xyzw, format] = vkRS.resources.Get<Layout, View, XYZW, Format>(handle);
                
            if (dimension == TextureDimension::TextureCubeMap)
                FK_ASSERT(xyzw[3] == 6);

            VkBufferDeviceAddressInfo getAddressInfo{
                .sType      = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .pNext      = nullptr,
                .buffer     = object.buffer
            };

            const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

            VkImageView imageView;
            VkImageViewCreateInfo createInfo{};
            createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format                           = FormatToVK(format);
            createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel    = 0;
            createInfo.subresourceRange.levelCount      = 1;
            createInfo.subresourceRange.baseArrayLayer  = 0;
            createInfo.subresourceRange.layerCount      = xyzw[3];
            vkCreateImageView(vkRS.device, &createInfo, nullptr, &imageView);

            VkDescriptorImageInfo image{
                    .sampler        = nullptr,
                    .imageView      = view.imageView,
                    .imageLayout    = (VkImageLayout)LayoutToVK(layout)
            };

            VkDescriptorGetInfoEXT getInfo{
                .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext  = nullptr,
                .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .data {
                    .pSampledImage= &image
                }
            };

            size_t offset;
            vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

            vkGetDescriptor(
                vkRS.device, &getInfo,
                vkRS.descriptorBufferProperties.sampledImageDescriptorSize,
                descriptorBuffer + offset);

            vkDestroyImageView(vkRS.device, imageView, nullptr);
        }   break;
        }
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
    {
        SetSRV(ctx, idx, handle);
    }

    void vkDescriptorSet::SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint mipOffset, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetSRVArray(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetSRV3D(IContext& ctx, size_t idx, ResourceHandle handle)
    {
    }

    void vkDescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
    {
    }

    void vkDescriptorSet::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle handle, DeviceFormat format)
    {
    }

    void vkDescriptorSet::SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle handle, size_t bufferOffset)
    {
        auto& vkRS = GetVKRS();

        const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

        FK_ASSERT(dimension == FlexKit::TextureDimension::Buffer);

        VkBufferDeviceAddressInfo getAddressInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext  = nullptr,
            .buffer = object.buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + bufferOffset,
            .range      = vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            .format     = VkFormat::VK_FORMAT_UNDEFINED
        };

        const VkDescriptorGetInfoEXT getInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext  = nullptr,
            .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data {
                .pStorageBuffer = &bufferInfo
            }
        };

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.storageBufferDescriptorSize,
            descriptorBuffer + offset);
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

    void vkDescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle handle, size_t bufferStride, size_t bufferOffset)
    {
        auto& vkRS = GetVKRS();

        const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

        FK_ASSERT(dimension == FlexKit::TextureDimension::Buffer);

        VkBufferDeviceAddressInfo getAddressInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext  = nullptr,
            .buffer = object.buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + bufferOffset,
            .range      = vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            .format     = VkFormat::VK_FORMAT_UNDEFINED
        };

        VkDescriptorGetInfoEXT getInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext  = nullptr,
            .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data {
                .pStorageBuffer = &bufferInfo
            }
        };

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.storageBufferDescriptorSize,
            descriptorBuffer + offset);
    }

    void vkDescriptorSet::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle handle, ResourceHandle counter, size_t bufferStride, size_t bufferOffset)
    {
        auto& vkRS = GetVKRS();

        const auto [object, dimension] = vkRS.resources.Get<APIHandle, Dimension>(handle);

        FK_ASSERT(dimension == FlexKit::TextureDimension::Buffer);

        VkBufferDeviceAddressInfo getAddressInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext  = nullptr,
            .buffer = object.buffer
        };

        const auto address = vkGetBufferDeviceAddress(vkRS.device, &getAddressInfo);

        VkDescriptorAddressInfoEXT bufferInfo{
            .sType      = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
            .pNext      = nullptr,
            .address    = address + bufferOffset,
            .range      = vkRS.descriptorBufferProperties.uniformBufferDescriptorSize,
            .format     = VkFormat::VK_FORMAT_UNDEFINED
        };

        VkDescriptorGetInfoEXT getInfo{
            .sType  = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .pNext  = nullptr,
            .type   = VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            .data {
                .pStorageBuffer = &bufferInfo
            }
        };

        size_t offset;
        vkGetDescriptorSetLayoutBindingOffset(vkRS.device, apiLayout, (uint32_t)idx, &offset);

        vkGetDescriptor(
            vkRS.device, &getInfo,
            vkRS.descriptorBufferProperties.storageBufferDescriptorSize,
            descriptorBuffer + offset);
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
