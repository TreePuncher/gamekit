#include "Materials.h"
#include "TextureStreamingUtilities.h"

namespace FlexKit
{   /************************************************************************************************/


    void MaterialComponent::ReleaseMaterial(MaterialHandle material)
    {
        const auto idx = handles[material];

        materials[idx].refCount--;

        if (materials[idx].refCount == 0)
        {
            auto& textures = materials[idx].Textures;

            for (auto& texture : textures)
                ReleaseTexture(texture);


            materials[idx] = materials.back();
            materials.pop_back();

            if (materials.size())
            {
                handles[materials[idx].handle] = idx;
                handles.RemoveHandle(material);
            }
        }
    }


    /************************************************************************************************/


    void MaterialComponent::ReleaseTexture(ResourceHandle texture)
    {
        const auto res = std::find_if(
            std::begin(textures),
            std::end(textures),
            [&](auto element)
            {
                return element.texture == texture;
            }
        );

        if (res != std::end(textures))
        {
            res->refCount--;

            if (res->refCount == 0)
            {
                renderSystem.ReleaseTexture(res->texture);
                textures.remove_unstable(res);
            }
        }
    }


    /************************************************************************************************/


    MaterialHandle MaterialComponent::CloneMaterial(MaterialHandle sourceMaterial)
    {
        const auto clone = materials.push_back(materials[handles[sourceMaterial]]);
        materials[clone].refCount = 1;

        const auto handle = handles.GetNewHandle();
        handles[handle] = clone;

        return handle;
    }


    /************************************************************************************************/


    void MaterialComponent::AddTexture(GUID_t textureAsset, MaterialHandle material, const bool loadLowest)
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

            if(loadLowest)
                streamEngine.LoadLowestLevel(textureResource);

            textures.push_back({ 1, textureResource, textureAsset });

            materials[handles[material]].Textures.push_back(textureResource);
        }
        else
        {
            res->refCount++;
            materials[handles[material]].Textures.push_back(res->texture);
        }
    }


}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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

