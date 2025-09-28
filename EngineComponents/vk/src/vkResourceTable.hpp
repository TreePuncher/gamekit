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
            VkBuffer    buffer;
            VkImage     image;
        };
    };

    enum ResourceFieldID : uint32_t
    {
        APIHandle   = 0,
        Format      = 1,
        Dimension   = 2,
        Layout      = 3
    };

    struct vkResourceTable
    {
        using MultiFieldType = MultiField<vkResourceEntry, DeviceFormat, TextureDimension, DeviceLayout>;

        vkResourceTable(iAllocator& IN_allocator) :
            fields  { IN_allocator },
            handles { IN_allocator } {}

        [[nodiscard]]
        ResourceHandle AddResource()
        {
            auto ul = std::unique_lock{ m };

            auto idx = fields.push_back({}, {}, {}, DeviceLayout::Common);
            return handles.GetNewHandle(idx);;
        }

        template<uint32_t ... ids>
        auto Get(ResourceHandle handle) 
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");
            std::shared_lock sl{ m };

            return fields.Slice<ids...>(handles[handle]);
        }

        template<uint32_t ... ids>
        auto Get(ResourceHandle handle, auto ... fields)
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");
            std::shared_lock sl{ m };

            return fields.Slice<ids...>(handles[handle]);
        }

        template<uint32_t ... ids>
        auto Set(ResourceHandle handle, auto&& ... IN_fields) requires (sizeof ... (ids) == sizeof ... (IN_fields))
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");

            fields.Set<ids...>(handles[handle], IN_fields...);
        }


        std::mutex                                      m;
        HandleUtilities::HandleTable<ResourceHandle>    handles;
        MultiFieldType                                  fields;
    };
}
