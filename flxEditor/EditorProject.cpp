#include "EditorProject.h"
#include "../FlexKitResourceCompiler/MeshProcessing.h"
#include <../FlexKitResourceCompiler/TextureResourceUtilities.h>
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

    FlexKit::LoadArchiveContext archive{ f };
    archive& resources;
    archive& scenes;

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


void EditorProject::AddResource(FlexKit::Resource_ptr resource)
{
    resources.emplace_back(ProjectResource{ resource });
}


/************************************************************************************************/


FlexKit::ResourceList EditorProject::GetResources() const
{
    FlexKit::ResourceList out;

    for (auto& r : resources)
        out.push_back(r.resource);

    return out;
}


/************************************************************************************************/


void EditorProject::RemoveResource(FlexKit::Resource_ptr resource)
{
    std::erase_if(resources, [&](auto& res) -> bool { return (res.resource == resource); });
}


/************************************************************************************************/
