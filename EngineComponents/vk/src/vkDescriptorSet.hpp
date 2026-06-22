#pragma once
#include <RenderSystemInterface.hpp>
#include "vkPipelineLayout.hpp"
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

    struct vkDescriptorSet : FlexKit::IDescriptorHeap
    {
        vkDescriptorSet(struct vkRenderSystem&);
        ~vkDescriptorSet() override;
        IDescriptorHeap& operator=(IDescriptorHeap&&) override;

        void Init(IContext& ctx, const DescriptorSetLayout& Layout_IN, iAllocator& TempMemory) override;
        void Init(IContext& ctx, const DescriptorSetLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory) override;   
        void Init2(IContext& ctx, const DescriptorSetLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory) override;
        void NullFill(IContext& ctx, const size_t end) override;

        void SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants) override;
        void SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize) override;
        void SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize) override;

        void SetSRV(IContext& ctx, size_t idx, ResourceHandle) override;
        void SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) override;
        void SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format) override;
        void SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) override;
        void SetSRV3D(IContext& ctx, size_t idx, ResourceHandle) override;
        void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle) override;
        void SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle, DeviceFormat format) override;

        void SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t offset) override;
        void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle) override;
        void SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) override;
        void SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format) override;
        void SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle) override;
        void SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format) override;
        void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset) override;
        void SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset) override;

        void SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset) override;

        DevicePointer GetGPUDescriptorHandle() const override;
        DescriptorSet GetHeapOffsetted(size_t offset, IContext& ctx) const override;

        struct vkRenderSystem& GetVKRS() noexcept;

        VkDescriptorSetLayout       apiLayout           = nullptr;
        const DescriptorSetLayout*  layout              = nullptr;
        std::byte*                  descriptorBuffer    = nullptr;
        uint64_t                    size                = 0;
        uint32_t                    bufferOffset        = 0;
    };
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
