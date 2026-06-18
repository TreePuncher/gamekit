#include "ShaderBindingTable.hpp"
#include "Scene.hpp"
#include "FrameGraph.hpp"
#include <filesystem>
#include <Type.hpp>
#include <dxRenderSystem.hpp>

namespace FlexKit
{
    struct gpuHitGroup
    {
        uint64_t anyHit; // GPU shader program address
        uint64_t closestHit; // GPU shader program address
        uint64_t IntersectionHit; // GPU shader program address
    };

    IPipelineStateLibrary* LoadShaderLibrary(const std::filesystem::path& shaderFile, IPipelineInterface* globalInterface)
    {
        if (!std::filesystem::exists(shaderFile))
        {
            FK_LOG_ERROR("ShaderLibrary::LoadShader: File not found!: %s", shaderFile.c_str());
            return nullptr;
        }


        PipelineStateLibraryDesc library;
        auto& renderSystem = static_cast<dx_Internal::dxRenderSystem&>(IRenderSystem::GetInstance());

        auto shader = renderSystem.LoadShaderLibrary(
                        shaderFile.string().c_str(),
                        ShaderOptions{ 
                            .hlsl2021 = true, 
                            .enableDebug = true });

        ShaderExport  exports[] = {
            ShaderExport{ .function = "miss_main" },
            ShaderExport{ .function = "raygen_main" },
            ShaderExport{ .function = "anyhit_main" },
            ShaderExport{ .function = "closesthit_main" },
            ShaderExport{ .function = "MyLocalRootSignature" },
            ShaderExport{ .function = "defaultHitGroup" },
        };

        HitGroup hitGroups[] = {
            HitGroup{
                .type       = HitGroupType::HitGroupType_Triangles,
                .ID         = "defaultHitGroup",
                .anyHit     = "anyhit_main",
                .closestHit = "closesthit_main",
            }
        };

        LibrarySection rtLibrary = LibraryRT
        {
            .byteCode           = &shader,
            .globalInterface    = globalInterface,
            .maxRayDepth        = 4,
            .payloadSize        = 20,
            .attributesByteSize = 32,
            .exports            = std::span{ exports },
            //.hitGroups          = std::span{ hitGroups }
        };

        return renderSystem.CreateLibrary(std::span{ &rtLibrary, 1 });
    }

    ShaderBindingTable::ShaderBindingTable(iAllocator& IN_allocator ) : 
        objectMappings  { IN_allocator },
        freeList        { IN_allocator } {}

    ShaderBindingTable::~ShaderBindingTable()
    {
    }

    void ShaderBindingTable::Update(class Scene&, class UpdateDispatcher& dispatcher, class FrameGraph& graph)
    {
        graph.AddNode2(
            [&](FrameGraphNodeBuilder& builder)
            {
                struct UpdateTask
                {
                    
                } task;

                return task;
            },
            [=](const auto& data, const ResourceHandler& resources, IDirectContext& ctx, iAllocator& threadLocalAllocator)
            {
            });
    }
}
