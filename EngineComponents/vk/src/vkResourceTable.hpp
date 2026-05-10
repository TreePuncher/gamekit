#include <Handle.hpp>
#include <MemoryUtilities.hpp>
#include <MultiField.hpp>
#include <mutex>
#include <RenderSystemInterface.hpp>
#include <ResourceHandles.hpp>
#include <vulkan/vulkan.hpp>

namespace VK_internal
{
    using namespace FlexKit;

    struct vkResourceEntry
    {
        enum class Type
        {
            Buffer,
            Image,
            RenderTarget
        }   type;

        union
        {
            void*       _ptr;
            VkBuffer    buffer;
            VkImage     image;
        };
    };


    union vkResourceViews
    {
        VkBufferView    bufferView;
        VkImageView     imageView;
    };


    enum ResourceFieldID : uint32_t
    {
        APIHandle   = 0,
        Format      = 1,
        Dimension   = 2,
        Layout      = 3,
        XYZW        = 4,
        Flags       = 5,
        View        = 6,
        Clear       = 7, 
        Extra       = 8
    };

    enum class ExtraBlockType
    {
        SignalBlock
    };

    struct Extra_SignalBlock
    {
        ExtraBlockType  type;
        VkSemaphore     wait    = nullptr;
        VkSemaphore     signal  = nullptr;
        VkFence         fence   = nullptr;
        void*           next    = nullptr;
    };


    struct vkResourceTable
    {
        using MultiFieldType = MultiField<vkResourceEntry, DeviceFormat, TextureDimension, DeviceLayout, uint4, uint32_t, vkResourceViews, VkClearValue, void*>;

        vkResourceTable(iAllocator& IN_allocator) :
            fields  { IN_allocator },
            handles { IN_allocator } {}

        [[nodiscard]]
        ResourceHandle AddResource()
        {
            auto ul = std::unique_lock{ m };

            auto idx = fields.push_back({}, {}, {}, DeviceLayout::Common, { 0, 0, 0, 0 }, 0, {}, VkClearValue{ .color = { 0.0f, 0.0f, 0.0f, 0.0f } }, {});
            return handles.GetNewHandle(idx);;
        }

        template<uint32_t ... ids>
        auto Get(ResourceHandle handle) const
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");
            std::shared_lock sl{ const_cast<std::shared_mutex&>(m) };

            return fields.Get<ids...>(handles[handle]);
        }

        template<uint32_t ... ids>
        auto Get(ResourceHandle handle, auto ... IN_fields) const
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");
            std::shared_lock sl{ const_cast<std::shared_mutex&>(m) };

            return fields.Get<ids...>(handles[handle]);
        }

        template<uint32_t ... ids>
        auto Set(ResourceHandle handle, auto&& ... IN_fields) requires (sizeof ... (ids) == sizeof ... (IN_fields))
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");

            fields.Set<ids...>(handles[handle], IN_fields...);
        }


        std::shared_mutex                               m;
        HandleUtilities::HandleTable<ResourceHandle>    handles;
        MultiFieldType                                  fields;
    };
}


/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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
