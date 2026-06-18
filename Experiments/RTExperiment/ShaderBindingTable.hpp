#include <Containers.hpp>
#include <RenderSystemInterface.hpp>
#include <filesystem>

namespace FlexKit
{
    IPipelineStateLibrary* LoadShaderLibrary(const std::filesystem::path& shaderFile, IPipelineInterface* globalInterface = nullptr);

    struct HitGroupAllocation
    {
        uint32_t idx;
        uint32_t lastUsed;
    };

    class ShaderBindingTable
    {
    public:
        ShaderBindingTable(iAllocator& IN_allocator);
        ~ShaderBindingTable();

        void        Update(class Scene&, class UpdateDispatcher&, class FrameGraph& graph);
        uint32_t    GetIdx() { return count++; }

        HashTable<HitGroupAllocation, uint32_t>     objectMappings; // Map brush handles to table entries
        DeviceAddressRange                          gpuTablebuffer;
        Vector<uint32_t>                            freeList;
        uint32_t                                    count = 0;
    };
}
