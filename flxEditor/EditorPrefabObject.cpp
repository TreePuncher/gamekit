#include "PCH.h"
#include "EditorPrefabObject.h"
#include "ResourceIDs.h"
#include "EditorScriptEngine.h"


/************************************************************************************************/


FlexKit::ResourceBlob ScriptResource::CreateBlob() const
{
    auto res = EditorScriptEngine::CompileToBlob(source);

    if (!res)
        return {};

    auto& blob = res.value();

    FlexKit::ScriptResourceBlob header{ blob.size() };
    header.GUID = GetResourceGUID();
    strncpy(header.ID, ID.c_str(), FlexKit::Min(64, ID.size()));
    blob = FlexKit::Blob{ header } + blob;

    FlexKit::ResourceBlob outBlob;
    auto [_ptr, size] = blob.Release();

    outBlob.buffer          = (char*)_ptr;
    outBlob.bufferSize      = size;
    outBlob.GUID            = GetResourceGUID();
    outBlob.ID              = ID;
    outBlob.resourceType    = FlexKit::EResource_ByteCode;

    return outBlob;
}


/************************************************************************************************/


const std::string& ScriptResource::GetResourceID() const noexcept
{
    return ID;
}


/************************************************************************************************/


const uint64_t ScriptResource::GetResourceGUID() const noexcept
{
    return resourceId;
}


/************************************************************************************************/


const ResourceID_t ScriptResource::GetResourceTypeID() const noexcept
{
    return ScriptResourceTypeID;
}

void ScriptResource::SetResourceID(std::string& id) { ID = id; }


/************************************************************************************************/


FlexKit::ResourceBlob PrefabGameObjectResource::CreateBlob() const
{
    FlexKit::Blob blob;

    for (auto& component : components)
        blob += component->GetBlob();

    FlexKit::PrefabResource header{ blob.size(), (uint32_t)components.size() };
    blob = FlexKit::Blob{ header } + blob;

    FlexKit::ResourceBlob outBlob;
    auto [_ptr, size] = blob.Release();

    outBlob.buffer          = (char*)_ptr;
    outBlob.bufferSize      = size;
    outBlob.GUID            = GetResourceGUID();
    outBlob.ID              = ID;
    outBlob.resourceType    = FlexKit::EResource_Prefab;

    return outBlob;
}

const std::string& PrefabGameObjectResource::GetResourceID() const noexcept
{
    return ID;
}

const uint64_t PrefabGameObjectResource::GetResourceGUID() const noexcept
{
    return resourceId;
}

const ResourceID_t PrefabGameObjectResource::GetResourceTypeID() const noexcept
{
    return FlexKit::EResource_Prefab;
}

void PrefabGameObjectResource::SetResourceID(std::string& newID)
{
    ID = newID;
}


/************************************************************************************************/


void LoadEntity(FlexKit::ComponentVector& components, LoadEntityContextInterface& ctx)
{
    for (auto& componentEntry : components)
    {
        switch (componentEntry->id)
        {
        case FlexKit::TransformComponentID:
        {
            auto nodeComponent = std::static_pointer_cast<FlexKit::EntitySceneNodeComponent>(componentEntry);
            ctx.GameObject().AddView<FlexKit::SceneNodeView<>>(ctx.GetNode(nodeComponent->nodeIdx));
        }   break;
        case FlexKit::BrushComponentID:
        {
            auto brushComponent = std::static_pointer_cast<FlexKit::EntityBrushComponent>(componentEntry);

            if (brushComponent)
            {
                auto res = ctx.FindSceneResource(brushComponent->MeshGuid);

                if (!res) // TODO: Mesh not found, use placeholder model?
                    continue;

                auto& materials = FlexKit::MaterialComponent::GetComponent();
                auto material   = materials.CreateMaterial(ctx.DefaultMaterial());


                //TODO: Seems the issue if further up the asset pipeline. Improve material generation?
                if (brushComponent->material.subMaterials.size() > 1)
                {
                    for (auto& subMaterialData : brushComponent->material.subMaterials)
                    {
                        auto subMaterial = materials.CreateMaterial();
                        materials.AddSubMaterial(material, subMaterial);

                        FlexKit::ReadContext rdCtx{};
                        for (auto texture : subMaterialData.textures)
                            materials.AddTexture(texture, subMaterial, rdCtx);
                    }
                }
                else if(brushComponent->material.subMaterials.size() == 1)
                {
                    auto& subMaterialData = brushComponent->material.subMaterials[0];

                    FlexKit::ReadContext rdCtx{};

                    for (auto texture : subMaterialData.textures)
                        materials.AddTexture(texture, material, rdCtx);
                }

                FlexKit::TriMeshHandle handle = ctx.LoadTriMeshResource(res);

                //auto& view = (FlexKit::BrushView&)EditorInspectorView::ConstructComponent(FlexKit::BrushComponentID, *viewObject, *viewportScene);
                auto& component     = FlexKit::ComponentBase::GetComponent(componentEntry->id);
                auto  blob          = brushComponent->GetBlob();
                component.AddComponentView(ctx.GameObject(), &ctx, blob, blob.size(), FlexKit::SystemAllocator);

                auto& brush         = FlexKit::GetView<FlexKit::BrushView>(ctx.GameObject()).GetBrush();
                brush.material      = material;
                brush.MeshHandle    = handle;
                
                ctx.GameObject().AddView<FlexKit::MaterialComponentView>(material);

                if(ctx.Scene() && !ctx.GameObject().hasView(FlexKit::SceneVisibilityComponentID))
                   ctx.Scene()->AddGameObject(ctx.GameObject(), FlexKit::GetSceneNode(ctx.GameObject()));

                FlexKit::SetBoundingSphereFromMesh(ctx.GameObject());
            }
        }   break;
        case FlexKit::PointLightComponentID:
        {
            if (ctx.Scene() && !ctx.GameObject().hasView(FlexKit::SceneVisibilityComponentID))
                ctx.Scene()->AddGameObject(ctx.GameObject(), FlexKit::GetSceneNode(ctx.GameObject()));

            auto  blob          = componentEntry->GetBlob();
            auto& component     = FlexKit::ComponentBase::GetComponent(componentEntry->id);
            //auto& componentView = EditorInspectorView::ConstructComponent(componentEntry->id, *viewObject, *viewportScene);

            component.AddComponentView(ctx.GameObject(), &ctx, blob, blob.size(), FlexKit::SystemAllocator);
            FlexKit::SetBoundingSphereFromLight(ctx.GameObject());
        }   break;
        default:
        {
            auto  blob              = componentEntry->GetBlob();
            //auto& componentView     = EditorInspectorView::ConstructComponent(componentEntry->id, *viewObject, *viewportScene);
            auto& component         = FlexKit::ComponentBase::GetComponent(componentEntry->id);

            component.AddComponentView(ctx.GameObject(), &ctx, blob, blob.size(), FlexKit::SystemAllocator);
        }   break;
        }
    }
}


/************************************************************************************************/


/**********************************************************************

Copyright (c) 2021 - 2022 Robert May

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
