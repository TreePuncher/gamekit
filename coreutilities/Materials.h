#pragma once

#include "Components.h"
#include "ResourceHandles.h"
#include "graphics.h"
#include "Assets.h"
#include "ComponentBlobs.h"

namespace FlexKit
{   /************************************************************************************************/


    class TextureStreamingEngine;


    /************************************************************************************************/

    struct MaterialComponentData
    {
        static_vector<MaterialHandle, 16>   SubMaterials;
        static_vector<ResourceHandle, 16>   Textures;
        uint32_t                            refCount;
        MaterialHandle                      handle;
    };

    struct MaterialTextureEntry
    {
        uint32_t        refCount;
        ResourceHandle  texture;
        GUID_t          assetID;
    };


    /************************************************************************************************/


    struct MaterialComponentEventHandler
    {
        void OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
    };


    /************************************************************************************************/


    struct MaterialComponent : public Component<MaterialComponent, MaterialComponentID>
    {
        MaterialComponent(RenderSystem& IN_renderSystem, TextureStreamingEngine& IN_TSE, iAllocator* allocator) :
            streamEngine    { IN_TSE },  
            renderSystem    { IN_renderSystem },
            materials       { allocator },
            textures        { allocator },
            handles         { allocator } {}

        MaterialComponentData operator [](const MaterialHandle handle) const
        {
            if (handle == InvalidHandle_t)
                return { {},  {}, 0, InvalidHandle_t };

            return materials[handles[handle]];
        }

        MaterialHandle CreateMaterial()
        {
            auto materialIdx = materials.push_back({ {}, {}, 1 });
            auto handle = handles.GetNewHandle();

            handles[handle] = materialIdx;

            return handle;
        }


        void AddRef(MaterialHandle material)
        {
            materials[handles[material]].refCount++;
        }

        void            AddSubMaterial(MaterialHandle material, MaterialHandle subMaterial)
        {
            materials[handles[material]].SubMaterials.push_back(subMaterial);
        }

        void            ReleaseMaterial(MaterialHandle material);
        void            ReleaseTexture(ResourceHandle texture);

        MaterialHandle  CloneMaterial(MaterialHandle sourceMaterial);

        // The Material View is a ref counted reference to a material instance.
        // On Writes to the material, if it is shared, the MaterialView does a copy on write and creates a new instance of the material.
        struct MaterialView : public ComponentView_t<MaterialComponent>
        {
            MaterialView(MaterialHandle IN_handle) : handle{ IN_handle }
            {
                GetComponent().AddRef(handle);
            }

            MaterialView() : handle{ GetComponent().CreateMaterial() } {}

            ~MaterialView()
            {
                GetComponent().ReleaseMaterial(handle);
            }


            MaterialComponentData GetData() const
            {
                return GetComponent()[handle];
            }


            bool Shared() const
            {
                return GetComponent()[handle].refCount > 1;
            }


            void AddTexture(GUID_t textureAsset, bool LoadLowest = false)
            {
                if (Shared())
                {
                    auto newHandle = GetComponent().CloneMaterial(handle);
                    GetComponent().ReleaseMaterial(handle);

                    handle = newHandle;
                }

                GetComponent().AddTexture(textureAsset, handle, LoadLowest);
            }


            bool HasSubMaterials() const
            {
                return !GetComponent()[handle].SubMaterials.empty();
            }

            MaterialHandle CreateSubMaterial()
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


            MaterialHandle handle;
        };

        using View = MaterialView;

        void AddTexture(GUID_t textureAsset, MaterialHandle material, const bool LoadLowest = false, ReadContext& readContext = ReadContext{});
        void AddComponentView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

        RenderSystem&                   renderSystem;
        TextureStreamingEngine&         streamEngine;

        Vector<MaterialComponentData>                   materials;
        Vector<MaterialTextureEntry>                    textures;
        HandleUtilities::HandleTable<MaterialHandle>    handles;
    };


    using MaterialComponentView     = MaterialComponent::View;


    void SetMaterialHandle(GameObject& go, MaterialHandle material);
    MaterialHandle GetMaterialHandle(GameObject& go);


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
