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

    IPipelineStateLibrary* LoadShaderLibrary(
        const std::filesystem::path&    shaderFile,
        IPipelineInterface*             globalInterface,
        std::span<Association>          localInterfaces)
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
                            .hlsl2021       = true, 
                            .enableDebug    = true });

        ShaderExport  exports[] = {
            ShaderExport{ .id = "anyhit_light" },
            ShaderExport{ .id = "closesthit_light" },
            ShaderExport{ .id = "anyhit_material" },
            ShaderExport{ .id = "closesthit_material" },

            ShaderExport{ .id = "miss_main" },
            ShaderExport{ .id = "raygen_main" },
            
            ShaderExport{ .id = "DefaultMaterial" },
            ShaderExport{ .id = "LightMaterial" },
            
            ShaderExport{ .id = "LightInterface" },
            ShaderExport{ .id = "DefaultLightInterfaceAssociation" },

            ShaderExport{ .id = "MissInterface" },
            ShaderExport{ .id = "MissInterfaceAssociation" },

            ShaderExport{ .id = "DefaultMaterialInterface" },
            ShaderExport{ .id = "DefaultMaterialInterfaceAssociation" },
        };

        LibrarySection rtLibrary = LibraryRT
        {
            .byteCode           = &shader,
            .globalInterface    = globalInterface,
            .maxRayDepth        = 8,
            .payloadSize        = 24,
            .attributesByteSize = 32,
            .exports            = std::span{ exports },
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
