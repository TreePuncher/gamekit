#pragma once

#include "Components.h"
#include "ResourceHandles.h"
#include "graphics.h"
#include "Assets.h"
#include "ComponentBlobs.h"
#include <variant>

namespace FlexKit
{   /************************************************************************************************/


    class TextureStreamingEngine;


    /************************************************************************************************/

    
    using PassHandle = Handle_t<32u, GetTypeGUID(PassHandle)>;


    struct MaterialProperty
    {
        MaterialProperty() = default;

        template<typename TY>
        MaterialProperty(uint32_t IN_ID, const TY& IN_value) :
            ID      { IN_ID },
            value   { IN_value } {}

        uint32_t                                                                                ID = -1;
        std::variant<float, float2, float3, float4, uint, uint2, uint3, uint4, ResourceHandle>  value;
    };

    struct MaterialComponentData
    {
        uint32_t                            refCount;
        MaterialHandle                      handle;
        MaterialHandle                      parent;

        static_vector<PassHandle, 32>       Passes;
        static_vector<MaterialProperty, 32> Properties;
        static_vector<MaterialHandle, 32>   SubMaterials;
        static_vector<ResourceHandle, 32>   Textures;
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
            handles         { allocator },
            activePasses    { allocator }
        {
            materials.reserve(256);
        }

        MaterialComponentData operator [](const MaterialHandle handle) const
        {
            if(handle == InvalidHandle_t)
                return { 0, InvalidHandle_t, InvalidHandle_t, {}, {}, {}, {} };

            return materials[handles[handle]];
        }

        MaterialHandle CreateMaterial(MaterialHandle IN_parent = InvalidHandle_t)
        {
            std::scoped_lock lock{ m };

            const auto handle         = handles.GetNewHandle();
            const auto materialIdx    = (index_t)materials.push_back({ 0, handle, IN_parent, {}, {}, {}, {}  });

            if(IN_parent != InvalidHandle_t)
                AddRef(IN_parent);

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
            MaterialView(MaterialHandle IN_handle) :
                handle{ IN_handle }
            {
                GetComponent().AddRef(handle);
            }

            MaterialView() : handle{ GetComponent().CreateMaterial() }
            {
                int x = 0;
            }

            ~MaterialView() final override
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


            void Add2Pass(const PassHandle ID)
            {
                GetComponent().Add2Pass(handle, ID);
            }


            template<typename TY>
            void SetProperty(const uint32_t ID, TY value)
            {
                GetComponent().SetProperty(handle, ID, value);
            }


            template<typename TY>
            std::optional<TY> GetProperty(const uint32_t ID) const
            {
                return GetComponent().GetProperty(handle, ID);
            }

            void AddTexture(GUID_t textureAsset, bool LoadLowest = false)
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


            MaterialHandle  handle;
        };

        using View = MaterialView;

        void AddTexture(GUID_t textureAsset, MaterialHandle material, ReadContext& readContext, const bool LoadLowest = false);
        void AddComponentView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

        void Add2Pass(MaterialHandle& material, const PassHandle ID)
        {
            if (materials[handles[material]].refCount > 1)
            {
                auto newHandle = GetComponent().CloneMaterial(material);
                GetComponent().ReleaseMaterial(material);

                material = newHandle;
            }

            auto& passes = materials[handles[material]].Passes;
            passes.emplace_back(ID);

            if (auto res = std::find(activePasses.begin(), activePasses.end(), ID); res == activePasses.end())
                activePasses.push_back(ID);
        }

        static_vector<PassHandle>   GetPasses(MaterialHandle material) const
        {
            if (material == InvalidHandle_t)
                return {};

            auto& materialData = materials[handles[material]];
            if (materialData.Passes.size())
                return materialData.Passes;

            if (materialData.parent != InvalidHandle_t)
                return GetPasses(materialData.parent);
            else
                return {};
        }

        Vector<PassHandle>          GetActivePasses(iAllocator& allocator) const
        {
            Vector<PassHandle> passes{ &allocator };
            passes = activePasses;

            return passes;
        }


        template<typename TY>
        void SetProperty(MaterialHandle& material, const uint32_t ID, TY value)
        {
            if (materials[handles[material]].refCount > 1)
            {
                auto newHandle = GetComponent().CloneMaterial(material);
                GetComponent().ReleaseMaterial(material);

                material = newHandle;
            }

            auto& properties = materials[handles[material]].Properties;
            properties.emplace_back(ID, value);
        }


        template<typename TY> 
        std::optional<TY> GetProperty(MaterialHandle handle, const uint32_t ID) const
        {
            auto& material      = materials[handles[handle]];
            auto& properties    = material.Properties;

            if (auto res = std::find_if(
                properties.begin(),
                properties.end(),
                [&](const auto& prop) -> bool
                {
                    return prop.ID == ID;
                }); res != properties.end())
            {
                if (auto* property = std::get_if<TY>(&res->value))
                    return { *property };
                else
                    return {};
            }
            else if (material.parent != InvalidHandle_t)
                return GetProperty<TY>(material.parent, ID);
            else
                return {};
        }


        RenderSystem&                   renderSystem;
        TextureStreamingEngine&         streamEngine;

        Vector<MaterialComponentData>                   materials;
        Vector<MaterialTextureEntry>                    textures;
        Vector<PassHandle>                              activePasses;

        HandleUtilities::HandleTable<MaterialHandle>    handles;
        std::mutex                                      m;
    };


    using MaterialComponentView     = MaterialComponent::View;


    void SetMaterialHandle(GameObject& go, MaterialHandle material);
    MaterialHandle GetMaterialHandle(GameObject& go);


}   /************************************************************************************************/

/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
