#include "PCH.h"
#include "EditorProject.h"
#include "MeshProcessing.h"
#include "TextureResourceUtilities.h"
#include <fstream>

#include "Serialization.hpp"

/************************************************************************************************/


ProjectResource_ptr EditorScene::FindSceneResource(uint64_t resourceID)
{
    for (auto& resource : sceneResources)
        if (resource->resource->GetResourceGUID() == resourceID)
            return resource;

    return nullptr;
}


/************************************************************************************************/


bool EditorProject::LoadProject(const std::string& projectDir)
{
    auto f = fopen(projectDir.c_str(), "rb");
    if (!f)
        return false;

    {
        FlexKit::LoadFileArchiveContext archive{ f };
        archive& resources;
        archive& scenes;
    }

    fclose(f);

    return true;
}


/************************************************************************************************/



bool EditorProject::SaveProject(const std::string& projectDir)
{
    try
    {
        FlexKit::SaveArchiveContext archive;
        archive& resources;
        archive& scenes;

        auto blob = archive.GetBlob();

        auto f = fopen(projectDir.c_str(), "wb");
        WriteBlob(blob, f);
        fclose(f);

        return true;
    }
    // swallow any exceptions
    // TODO(R.M): log this?
    catch (...) 
    {
        return false;
    }
}


/************************************************************************************************/


void EditorProject::AddScene(EditorScene_ptr scene)
{
    scenes.emplace_back(scene);
}


/************************************************************************************************/


ProjectResource_ptr EditorProject::AddResource(FlexKit::Resource_ptr resource)
{
    auto projectResource = std::make_shared<ProjectResource>(resource);
    resources.emplace_back(projectResource);

    return projectResource;
}


/************************************************************************************************/


FlexKit::ResourceList EditorProject::GetResources() const
{
    FlexKit::ResourceList out;

    for (auto& r : resources)
        out.push_back(r->resource);

    return out;
}


/************************************************************************************************/


void EditorProject::RemoveResource(FlexKit::Resource_ptr resource)
{
    std::erase_if(resources, [&](auto& res) -> bool { return (res->resource == resource); });
}


/************************************************************************************************/


ProjectResource_ptr EditorProject::FindProjectResource(uint64_t assetID)
{
    auto res = std::find_if(
        resources.begin(),
        resources.end(),
        [&](ProjectResource_ptr& resource)
        {
            return resource->resource->GetResourceGUID() == assetID;
        }
    );

    if (res != resources.end())
        return (*res);
    else
        return nullptr;
}


/************************************************************************************************/
