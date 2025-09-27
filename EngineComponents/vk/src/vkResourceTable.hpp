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
        };

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
        Dimension   = 2
    };

    struct vkResourceTable
    {
        using MultiFieldType = MultiField<vkResourceEntry, DeviceFormat, TextureDimension>;

        vkResourceTable(iAllocator& IN_allocator) :
            fields{ IN_allocator }{}

        [[nodiscard]]
        ResourceHandle AddResource()
        {
            auto ul         = std::unique_lock{ m };

            auto idx        = fields.push_back({}, {}, {});
            return handles.GetNewHandle(idx);;
        }

        template<uint32_t fieldID>
        auto Get(ResourceHandle handle) 
        {
            FK_ASSERT(handles.IsValid(handle), "Invalid Handle!");
            std::shared_lock sl{ m };

            return fields.Get<fieldID>(handles[handle]);
        }

        std::mutex                                      m;
        HandleUtilities::HandleTable<ResourceHandle>    handles;
        MultiFieldType                                  fields;
    };
}
