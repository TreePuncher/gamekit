#include "Materials.h"
#include "TextureStreamingUtilities.h"

namespace FlexKit
{

    void MaterialComponent::AddTexture(GUID_t textureAsset, MaterialHandle material)
    {
        const auto res = std::find_if(
            std::begin(textures),
            std::end(textures),
            [&](auto element)
            {
                return element.assetID == textureAsset;
            }
        );

        if (res == std::end(textures))
        {
            const auto [MIPCount, DDSTextureWH, _] = GetDDSInfo(textureAsset);

            auto textureResource = renderSystem.CreateGPUResource(
                GPUResourceDesc::ShaderResource(
                    DDSTextureWH,
                    DeviceFormat::BC3_UNORM,
                    MIPCount,
                    1, true));

            streamEngine.BindAsset(textureAsset, textureResource);
            textures.push_back({ textureResource, textureAsset });

            materials[material].Textures.push_back(textureResource);
        }
        else
            materials[material].Textures.push_back(res->texture);
    }
}
