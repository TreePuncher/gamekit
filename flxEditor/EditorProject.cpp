#include "EditorProject.h"
#include "../FlexKitResourceCompiler/MeshProcessing.h"
#include <../FlexKitResourceCompiler/TextureResourceUtilities.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include <fstream>

/************************************************************************************************/


void EditorScene::LoadScene()
{
    FlexKit::GraphicScene scene{ FlexKit::SystemAllocator };

    std::vector<FlexKit::NodeHandle> nodes;

    for (auto node : sceneResource->nodes)
    {
        auto newNode = FlexKit::GetZeroedNode();
        nodes.push_back(newNode);

        const auto position = node.position;
        const auto q = node.Q;

        FlexKit::SetOrientationL(newNode, q);
        FlexKit::SetPositionL(newNode, position);
    }

    for (auto& entity : sceneResource->entities)
    {
        GameObject_ptr gameObject = std::make_shared<FlexKit::GameObject>();
        sceneObjects.push_back(gameObject);

        if (entity.Node != -1)
            gameObject->AddView<FlexKit::SceneNodeView<>>(nodes[entity.Node]);

        for (auto& component : entity.components)
        {
            switch (component->id)
            {
            case FlexKit::TransformComponentID:
            {

            }   break;
            case FlexKit::DrawableComponentID:
            {
                auto drawableComponent = std::static_pointer_cast<FlexKit::ResourceBuilder::DrawableComponent>(component);

                if (drawableComponent)
                {
                    auto res = FindSceneResource(drawableComponent->MeshGuid);

                    if (res)
                    {
                        auto meshResource   = std::static_pointer_cast<FlexKit::ResourceBuilder::MeshResource>(res);
                        auto optimizedMesh  = FlexKit::MeshUtilityFunctions::CreateOptimizedMesh(*meshResource->kdbTree);
                    }
                }
            }   break;
            default:
            {
            }   break;
            }
        }
    }
}


/************************************************************************************************/


FlexKit::ResourceBuilder::Resource_ptr EditorScene::FindSceneResource(uint64_t resourceID)
{
    for (auto& resource : sceneResources)
        if (resource.resource->GetResourceGUID() == resourceID)
            return resource.resource;

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
