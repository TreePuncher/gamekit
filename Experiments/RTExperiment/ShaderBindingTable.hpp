#include <Containers.hpp>
#include <RenderSystemInterface.hpp>
#include <string_view>

namespace FlexKit
{
    IPipelineStateLibrary* LoadShaderLibrary(const std::filesystem::path& shaderFile, IPipelineInterface* globalInterface = nullptr);

    class ShaderBindingTable
    {
    public:
        ~ShaderBindingTable();
        void Update(class Scene&, class UpdateDispatcher&, class FrameGraph& graph);

        HashTable<uint32_t, uint32_t>   objectMappings; // Map brush handles to table entries
        DeviceAddressRange              gpuTablebuffer;
    };
}
