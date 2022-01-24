#include "Materials.h"
#include "Scene.h"
#include "TextureStreamingUtilities.h"

namespace FlexKit
{   /************************************************************************************************/


    void MaterialComponent::ReleaseMaterial(MaterialHandle material)
    {
        const auto idx = handles[material];

        if(materials[idx].refCount)
            materials[idx].refCount--;

        if (materials[idx].refCount == 0)
        {
            auto& textures = materials[idx].Textures;
            auto parent = materials[idx].parent;

            if (parent != InvalidHandle_t)
                ReleaseMaterial(parent);

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
                renderSystem.ReleaseResource(res->texture);
                textures.remove_unstable(res);
            }
        }
    }


    /************************************************************************************************/


    MaterialHandle MaterialComponent::CloneMaterial(MaterialHandle sourceMaterial)
    {
        const auto clone = (index_t)materials.push_back(materials[handles[sourceMaterial]]);
        materials[clone].refCount = 0;

        auto& material = materials[clone];
        material.refCount = 0;

        if (material.parent != InvalidHandle_t)
            AddRef(material.parent);

        const auto handle = handles.GetNewHandle();
        handles[handle] = clone;

        return handle;
    }


    /************************************************************************************************/


    void MaterialComponent::AddTexture(GUID_t textureAsset, MaterialHandle material, ReadContext& readContext, const bool loadLowest)
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
            const auto [MIPCount, DDSTextureWH, format] = GetDDSInfo(textureAsset, readContext);
            const auto resourceStrID = GetResourceStringID(textureAsset);

            if (DDSTextureWH.Product() == 0)
                return;

            auto textureResource = renderSystem.CreateGPUResource(
                GPUResourceDesc::ShaderResource(
                    DDSTextureWH,
                    format,
                    MIPCount,
                    1,
                    ResourceAllocationType::Tiled));

            renderSystem.SetDebugName(textureResource, "Virtual Texture");

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
    /************************************************************************************************/


    MaterialComponent::MaterialView::MaterialView(GameObject & gameObject, MaterialHandle IN_handle) noexcept :
        handle{ IN_handle }
    {
        GetComponent().AddRef(handle);

        SetMaterialHandle(gameObject, handle);
    }

    MaterialComponent::MaterialView::MaterialView(GameObject& gameObject) noexcept : handle{ GetComponent().CreateMaterial() }
    {
        GetComponent().AddRef(handle);

        SetMaterialHandle(gameObject, handle);
    }

    /************************************************************************************************/


    MaterialComponent::MaterialView::~MaterialView()
    {
        GetComponent().ReleaseMaterial(handle);
    }


    /************************************************************************************************/


    MaterialComponentData MaterialComponent::MaterialView::GetData() const
    {
        return GetComponent()[handle];
    }


    /************************************************************************************************/


    bool MaterialComponent::MaterialView::Shared() const
    {
        return GetComponent()[handle].refCount > 1;
    }


    /************************************************************************************************/


    void MaterialComponent::MaterialView::Add2Pass(const PassHandle ID)
    {
        GetComponent().Add2Pass(handle, ID);
    }


    /************************************************************************************************/


    static_vector<PassHandle> MaterialComponent::MaterialView::GetPasses() const
    {
        return GetComponent().GetPasses(handle);
    }


    /************************************************************************************************/


    void MaterialComponent::MaterialView::AddTexture(GUID_t textureAsset, bool LoadLowest)
    {
        if (Shared())
        {
            auto newHandle = GetComponent().CloneMaterial(handle);
            GetComponent().ReleaseMaterial(handle);

            handle = newHandle;
        }

        ReadContext rdCtx{};
        GetComponent().AddTexture(textureAsset, handle, rdCtx, LoadLowest);
    }


    /************************************************************************************************/


    bool MaterialComponent::MaterialView::HasSubMaterials() const
    {
        return !GetComponent()[handle].SubMaterials.empty();
    }


    /************************************************************************************************/


    MaterialHandle MaterialComponent::MaterialView::CreateSubMaterial()
    {
        auto& materials = GetComponent();

        if (Shared())
        {
            auto newHandle = GetComponent().CloneMaterial(handle);
            GetComponent().ReleaseMaterial(handle);

            handle = newHandle;
        }

        if (GetComponent()[handle].SubMaterials.full())
            return InvalidHandle_t;

        auto subMaterial = materials.CreateMaterial();
        materials.AddSubMaterial(handle, subMaterial);

        return subMaterial;
    }


    /************************************************************************************************/


    void MaterialComponent::AddComponentView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
    {
        MaterialComponentBlob materialBlob;
        memcpy(&materialBlob, buffer, Min(sizeof(materialBlob), bufferSize));

        const static auto parentMaterial = [&]() {
            auto newMaterial = CreateMaterial();

            Add2Pass(newMaterial, PassHandle{ GetCRCGUID(PBR_CLUSTERED_DEFERRED) });
            Add2Pass(newMaterial, PassHandle{ GetCRCGUID(VXGI_STATIC) });

            SetProperty(newMaterial, GetCRCGUID(PBR_ALBEDO), float4(0.5f, 0.5f, 0.5f, 0.3f));
            SetProperty(newMaterial, GetCRCGUID(PBR_SPECULAR), float4(0.9f, 0.9f, 0.9f, 0.0f));

            return newMaterial;
        }();

        auto newMaterial = CreateMaterial(parentMaterial);

        auto rdCtx          = ReadContext{};
        if (materialBlob.materials.size() > 1)
        {
            for (auto& subMaterial : materialBlob.materials)
            {
                auto newSubMaterial = CreateMaterial(newMaterial);
                AddSubMaterial(newMaterial, newSubMaterial);


                for (auto& texture : subMaterial.textures)
                    AddTexture(texture, newSubMaterial, rdCtx);
            }
        }
        else if (materialBlob.materials.size() == 1)
        {

            for (auto& texture : materialBlob.materials[0].textures)
                AddTexture(texture, newMaterial, rdCtx);
        }

        gameObject.AddView<MaterialView>(newMaterial);
        SetMaterialHandle(gameObject, newMaterial);
    }



    void SetMaterialHandle(GameObject& go, MaterialHandle material) noexcept
    {
        Apply(
            go,
            [&](BrushView& brush)
            {
                brush.GetBrush().material = material;
            });
    }


    MaterialHandle GetMaterialHandle(GameObject& go) noexcept
    {
        return Apply(
            go,
            [&](MaterialComponentView& material)
            {
                return material.handle;
            },
            []() -> MaterialHandle
            {
                return InvalidHandle_t;
            });
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

