#include "vkDescriptorHeap.hpp"

namespace VK_internal
{
    vkDesctriptorHeap::~vkDesctriptorHeap()
    {
    }

    IDescriptorHeap& vkDesctriptorHeap::operator=(IDescriptorHeap&&)
    {
        return *this;
    }

    void vkDesctriptorHeap::Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, iAllocator& TempMemory)
    {
    }

    void vkDesctriptorHeap::Init(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory)
    {
    }

    void vkDesctriptorHeap::Init2(IContext& ctx, const DesciptorHeapLayout& Layout_IN, const size_t reserveCount, iAllocator& TempMemory)
    {
    }

    void vkDesctriptorHeap::NullFill(IContext& ctx, const size_t end)
    {
    }

    void vkDesctriptorHeap::SetCBV(IContext& ctx, size_t idx, const ConstantBufferDataSet& constants)
    {
    }

    void vkDesctriptorHeap::SetCBV(IContext& ctx, size_t idx, ConstantBufferHandle, size_t offset, size_t bufferSize)
    {
    }

    void vkDesctriptorHeap::SetCBV(IContext& ctx, size_t idx, ResourceHandle, size_t offset, size_t bufferSize)
    {
    }

    void vkDesctriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle)
    {
    }

    void vkDesctriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetSRV(IContext& ctx, size_t idx, ResourceHandle, uint MipOffset, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetSRVArray(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetSRV3D(IContext& ctx, size_t idx, ResourceHandle)
    {
    }

    void vkDesctriptorHeap::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle)
    {
    }

    void vkDesctriptorHeap::SetSRVCubemap(IContext& ctx, size_t idx, ResourceHandle Handle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetUAVBuffer(IContext& ctx, size_t idx, ResourceHandle, size_t offset)
    {
    }

    void vkDesctriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle)
    {
    }

    void vkDesctriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetUAVTexture(IContext& ctx, size_t idx, size_t mipLevel, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetUAVCubemap(IContext& ctx, size_t idx, ResourceHandle handle)
    {
    }

    void vkDesctriptorHeap::SetUAVTexture3D(IContext& ctx, size_t idx, ResourceHandle, DeviceFormat format)
    {
    }

    void vkDesctriptorHeap::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
    {
    }

    void vkDesctriptorHeap::SetUAVStructured(IContext& ctx, size_t idx, ResourceHandle resource, ResourceHandle counter, size_t stride, size_t Offset)
    {
    }

    void vkDesctriptorHeap::SetStructuredResource(IContext& ctx, size_t idx, ResourceHandle, size_t stride, size_t offset)
    {
    }

    DevicePointer vkDesctriptorHeap::GetGPUDescriptorHandle() const
    {
        return {};
    }

    DescriptorHeap vkDesctriptorHeap::GetHeapOffsetted(size_t offset, IContext& ctx) const
    {
        return {};
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
