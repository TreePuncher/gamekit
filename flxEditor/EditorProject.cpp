#include "EditorProject.h"
#include "../FlexKitResourceCompiler/MeshProcessing.h"
#include <../FlexKitResourceCompiler/TextureResourceUtilities.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <fstream>

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
    std::ifstream ifs{ projectDir, std::ios::binary | std::ios::in };

    boost::archive::binary_iarchive archive{ ifs };
    archive.register_type<FlexKit::ResourceBuilder::TextureResource>();

    archive >> resources;

    ifs.close();


    return true;
}


/************************************************************************************************/


bool EditorProject::SaveProject(const std::string& projectDir)
{
    std::ofstream ofs(projectDir, std::ios::binary | std::ios::out);

    boost::archive::binary_oarchive archive{ ofs,  };
    archive.register_type<FlexKit::ResourceBuilder::TextureResource>();

    archive << resources;

    ofs.close();

    return false;
}


/************************************************************************************************/
