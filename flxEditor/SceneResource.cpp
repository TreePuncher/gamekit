#include "PCH.h"
#include "SceneResource.h"
#include "ComponentBlobs.h"
#include "MeshUtils.h"
#include "TextureResourceUtilities.h"
#include "MeshProcessing.h"
#include "ResourceUtilities.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <regex>
#include <filesystem>
#include <fmt\format.h>
#include <stb_image.h>
#include <stb_image_write.h>

namespace FlexKit
{   /************************************************************************************************/


    std::optional<MeshResource_ptr> ExtractGeometry(tinygltf::Mesh& mesh)
    {
        return {};
    }

    std::pair<ResourceList, std::vector<size_t>>  GatherGeometry(tinygltf::Model& model)
    {
        using namespace tinygltf;

        ResourceList        resources;
        std::vector<size_t> meshMap;

        for (auto& mesh : model.meshes)
        {
            uint64_t GUID = rand();
            std::string LOD;

            if (mesh.extras.Has("internal"))
            {
                meshMap.push_back(-1);
                continue;
            }

            if (mesh.extras.Has("ResourceID"))
            {
                auto& resourceID = mesh.extras.Get("ResourceID");
                if (resourceID.IsInt())
                    GUID = resourceID.Get<int>();
            }

            if (mesh.extras.Has("LOD"))
            {
                auto& LODproperty = mesh.extras.Get("LOD");
                if (LODproperty.IsString())
                    LOD = LODproperty.Get<std::string>();
            }

            if (auto res = std::find_if(
                std::begin(resources),
                std::end(resources),
                [&](Resource_ptr& resource)
                {
                    return resource->GetResourceGUID() == GUID;
                }); res != std::end(resources))
            {
                meshMap.push_back((*res)->GetResourceGUID());
                continue;
            }

            std::vector<tinygltf::Mesh*> lodSources;
            // Fetch LOD's

            auto fetchLOD =
                [&](const std::string lodMeshName, auto _THIS) -> void
                {
                    for (auto& mesh : model.meshes)
                    {
                        if (mesh.name == lodMeshName)
                        {
                            lodSources.push_back(&mesh);

                            if (mesh.extras.Has("LOD"))
                            {
                                auto LODid = mesh.extras.Get("LOD").Get<std::string>();
                                _THIS(LODid, _THIS);
                            }
                        }
                    }
                };

            lodSources.push_back(&mesh);

            if(LOD.size())
                fetchLOD(LOD, fetchLOD);

            for (auto& source : lodSources)
            {
                if (source->primitives.size() != lodSources[0]->primitives.size())
                {
                    std::cout << "Invalid Input LOD";
                    return {};
                }
            }

            std::vector<LODLevel> lodLevels;

            for(auto sourceMesh : lodSources)
            {
                auto&       primitives = sourceMesh->primitives;
                auto        meshTokens = FlexKit::MeshUtilityFunctions::TokenList{ SystemAllocator };

                LODLevel lod;

                for(auto& primitive : primitives)
                {
                    MeshDesc newMesh;

                    for(auto& attribute : primitive.attributes)
                    {
                        auto& bufferAcessor = model.accessors[attribute.second];
                        auto& bufferView    = model.bufferViews[bufferAcessor.bufferView];
                        auto* buffer        = model.buffers[bufferView.buffer].data.data() + bufferView.byteOffset;

                        auto stride         = bufferView.byteStride == 0 ? GetComponentSizeInBytes(bufferAcessor.componentType) * GetNumComponentsInType(bufferAcessor.type) : bufferView.byteStride;
                        auto elementCount   = bufferView.byteLength / stride;

                        std::regex texcoordPattern  { R"(TEXCOORD_[0-9]+)" };
                        std::regex jointPattern     { R"(JOINTS_[0-9]+)" };


                        if (attribute.first == "POSITION")
                        {
                            for (size_t I = 0; I < elementCount; I++)
                            {
                                float3 xyz;

                                memcpy(&xyz, buffer + stride * I, stride);
                                MeshUtilityFunctions::OBJ_Tools::AddVertexToken(xyz, meshTokens);
                            }
                            
                            for (size_t i = 0; i < bufferAcessor.minValues.size(); i++)
                            {
                                newMesh.MinV[i] = (float)bufferAcessor.minValues[i];
                                newMesh.MaxV[i] = (float)bufferAcessor.maxValues[i];
                            }
                        }
                        else if (attribute.first == "NORMAL")
                        {
                            if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
                            {
                                newMesh.Normals     = true;
                                newMesh.Tangents    = true;

                                auto& tangentAcessor    = model.accessors[primitive.attributes["TANGENT"]];
                                auto& tangentView       = model.bufferViews[tangentAcessor.bufferView];
                                auto* tangentbuffer     = model.buffers[tangentView.buffer].data.data() + tangentView.byteOffset;

                                auto tangentStride      = tangentView.byteStride == 0 ? GetComponentSizeInBytes(tangentAcessor.componentType) * GetNumComponentsInType(tangentAcessor.type) : tangentView.byteStride;

                                for (size_t I = 0; I < elementCount; I++)
                                {
                                    float4 normal;
                                    float4 tangent;

                                    memcpy(&normal, buffer + stride * I, stride);
                                    memcpy(&tangent, tangentbuffer + tangentStride * I, tangentStride);

                                    tangent = float4{ tangent.xyz() * tangent.w, tangent.w };

                                    MeshUtilityFunctions::OBJ_Tools::AddNormalToken(normal.xyz(), tangent.xyz(), meshTokens);
                                }
                            }
                            else
                            {
                                newMesh.Normals = true;

                                for (size_t I = 0; I < elementCount; I++)
                                {
                                    float3 normal;

                                    memcpy(&normal, buffer + stride * I, stride);
                                    MeshUtilityFunctions::OBJ_Tools::AddNormalToken(normal, meshTokens);
                                }
                            }
                        }
                        else if (std::regex_search(attribute.first, texcoordPattern))
                        {
                            newMesh.UV = true;

                            uint32_t idx = 0; 
                            static const std::regex idx_pattern{ R"([0-9]+)" };

                            std::smatch results;
                            auto res = std::regex_search(
                                attribute.first,
                                results, 
                                idx_pattern);


                            idx = res ? std::atoi(results.begin()->str().c_str()) : 0;


                            for (size_t I = 0; I < elementCount; I++)
                            {
                                float3 coord;

                                memcpy(&coord, buffer + stride * I, stride);
                                MeshUtilityFunctions::OBJ_Tools::AddTexCordToken(coord, idx, meshTokens);
                            }
                        }
                        else if (std::regex_search(attribute.first, jointPattern))
                        {
                            newMesh.Weights = true;

                            uint32_t idx = 0; 
                            static const std::regex idx_pattern{ R"([0-9]+)" };

                            std::smatch results;
                            auto res = std::regex_search(
                                attribute.first,
                                results, 
                                idx_pattern);

                            idx = res ? std::atoi(results.begin()->str().c_str()) : 0;

                            auto& weightsAcessor    = model.accessors[primitive.attributes[fmt::format("WEIGHTS_{0}", idx)]];
                            auto& weightsView       = model.bufferViews[weightsAcessor.bufferView];
                            auto* weightsbuffer     = model.buffers[weightsView.buffer].data.data() + weightsView.byteOffset;

                            const auto weightsStride = weightsView.byteStride == 0 ? GetComponentSizeInBytes(weightsAcessor.componentType) * GetNumComponentsInType(weightsAcessor.type) : weightsView.byteStride;

                            for (size_t I = 0; I < elementCount; I++)
                            {
                                uint4_16    joints;
                                float4      weights;


                                if(stride == 4)
                                {
                                    uint8_t joints_small[4];
                                    memcpy(joints_small, buffer + stride * I, 4);

                                    joints[0] = joints_small[0];
                                    joints[1] = joints_small[1];
                                    joints[2] = joints_small[2];
                                    joints[3] = joints_small[3];
                                }
                                else if (stride == 8)
                                    memcpy(&joints, buffer + stride * I, stride);

                                memcpy(&weights, weightsbuffer + weightsStride * I, weightsStride);

                                const float sum = weights[0] + weights[1] + weights[2] + weights[3];

                                MeshUtilityFunctions::OBJ_Tools::AddWeightToken({ weights.xyz(), joints }, meshTokens);
                            }
                        }

                    }

                    auto& indexAccessor     = model.accessors[primitive.indices];
                    auto& indexBufferView   = model.bufferViews[indexAccessor.bufferView];
                    auto& indexBuffer       = model.buffers[indexBufferView.buffer];
                     
                    const auto componentType  = indexAccessor.componentType;
                    const auto type           = indexAccessor.type;

                    const auto stride         = indexBufferView.byteStride == 0 ? GetComponentSizeInBytes(componentType) * GetNumComponentsInType(type) : indexBufferView.byteStride;
                    const auto elementCount   = indexBufferView.byteLength / stride;

                    auto* buffer = model.buffers[indexBufferView.buffer].data.data() + indexBufferView.byteOffset;

                    for (size_t I = 0; I < elementCount; I+=3)
                    {
                        VertexToken tokens[3];

                        for(size_t II = 0; II < 3; II++)
                        {
                            uint32_t idx = 0;
                            memcpy(&idx, buffer + stride * (I + II), stride);

                            tokens[II].vertex.push_back(VertexField{ idx, VertexField::Point });
                            tokens[II].vertex.push_back(VertexField{ idx, VertexField::Normal });


                            if (newMesh.Tangents)
                                tokens[II].vertex.push_back(VertexField{ idx, VertexField::Tangent });

                            if (newMesh.UV)
                                tokens[II].vertex.push_back(VertexField{ idx, VertexField::TextureCoordinate });

                            if (newMesh.Weights) {
                                tokens[II].vertex.push_back(VertexField{ idx, VertexField::JointIndex });
                                tokens[II].vertex.push_back(VertexField{ idx, VertexField::JointWeight });
                            }

                            std::sort(
                                tokens[II].vertex.begin(), tokens[II].vertex.end(),
                                [&](VertexField& lhs, VertexField& rhs)
                                {
                                    return lhs.type < rhs.type;
                                });
                        }

                        meshTokens.push_back(tokens[0]);
                        meshTokens.push_back(tokens[2]);
                        meshTokens.push_back(tokens[1]);
                    }

                    newMesh.tokens      = std::move(meshTokens);
                    newMesh.faceCount   = elementCount;

                    lod.subMeshs.emplace_back(std::move(newMesh));
                }

                lodLevels.emplace_back(std::move(lod));
            }

            auto meshResource = CreateMeshResource(lodLevels,  mesh.name, {}, false);
            meshResource->TriMeshID = GUID;

            resources.emplace_back(std::move(meshResource));

            meshMap.push_back(GUID);
        }

        return { resources, meshMap };
    }


    ResourceList GatherScenes(tinygltf::Model& model, std::vector<size_t>& meshMap, std::map<int, Resource_ptr>& imageMap, std::vector<Resource_ptr>& skinMap)
    {
        using namespace tinygltf;

        ResourceList resources;

        for (auto& scene : model.scenes)
        {
            SceneResource_ptr	sceneResource_ptr = std::make_shared<SceneResource>();

            auto& name      = scene.name;
            //auto& metaData  = FindRelatedMetaData({}, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, name); // TODO: allow for meta data to be included from external files

            std::map<int, int> sceneNodeMap;

            auto AddNode =
                [&](const int nodeIdx, const int parent, auto& AddNode) -> void
            {
                auto& node = model.nodes[nodeIdx];

                SceneNode newNode;
                newNode.parent = parent != -1 ? sceneNodeMap[parent] : -1;

                if (node.translation.size())
                    for (size_t I = 0; I < 3; I++)
                        newNode.position[I] = (float)node.translation[I];
                else
                    newNode.position = float3{ 0, 0, 0 };

                if (node.rotation.size())
                    for (size_t I = 0; I < 4; I++)
                        newNode.orientation[I] = (float)node.rotation[I];
                else
                    newNode.orientation = Quaternion{ 0, 0, 0, 1 };


                if (node.scale.size())
                    newNode.scale = float3{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
                else
                    newNode.scale = { 1, 1, 1 };

                if (node.mesh != -1 ||
                    node.skin != -1 ||
                    node.children.size() ||
                    node.extensions.find("KHR_lights_punctual") != node.extensions.end())
                {
                    const auto nodeHndl = sceneResource_ptr->AddSceneNode(newNode);
                    sceneNodeMap[nodeIdx] = nodeHndl;

                    SceneEntity entity;
                    entity.id = node.name;

                    auto nodeComponent = std::make_shared<EntitySceneNodeComponent>(nodeHndl);
                    entity.components.push_back(nodeComponent);

                    if (node.mesh != -1)
                    {
                        auto brush = std::make_shared<EntityBrushComponent>(meshMap[node.mesh]);

                        for (auto& subEntity : model.meshes[node.mesh].primitives)
                        {
                            BrushMaterial newMaterial;

                            if (subEntity.material != -1)
                            {
                                auto& material = model.materials[subEntity.material];

                                auto AddTexture = [&](const ParameterMap& material, auto& string) {
                                    if (auto res = material.find(string); res != material.end() && res->first == string)
                                    {
                                        int index = res->second.json_double_value.find("index")->second;

                                        if (imageMap.find(index) != imageMap.end())
                                        {
                                            auto resource   = imageMap[index];
                                            auto assetGUID  = resource->GetResourceGUID();

                                            std::cout << "Searched for " << string << ". Found resource: " << resource->GetResourceID() << "\n";

                                            newMaterial.textures.push_back(assetGUID);
                                        }
                                        else
                                        {
                                            std::cout << "Texture Not Found!\n" << "Failed to find: " << string << "\n";
                                        }
                                    }
                                };


                                if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
                                {
                                    if (model.textures.size() > material.pbrMetallicRoughness.baseColorTexture.index)
                                    {
                                        auto idx = model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source;

                                        if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
                                            newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                    }
                                }
                                if (material.normalTexture.index != -1)
                                {
                                    if (model.textures.size() > material.normalTexture.index)
                                    {
                                        auto idx = model.textures[material.normalTexture.index].source;

                                        if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
                                            newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                    }
                                }

                                if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
                                {
                                    if (model.textures.size() > material.pbrMetallicRoughness.metallicRoughnessTexture.index)
                                    {
                                        auto idx = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;

                                        if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
                                            newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                    }
                                }
                            }
                            brush->material.subMaterials.push_back(newMaterial);
                        }
                        entity.components.push_back(brush);
                    }

                    if (node.skin != -1)
                    {
                        auto assetID = skinMap[node.skin]->GetResourceGUID();
                        auto skeletonComponent = std::make_shared<EntitySkeletonComponent>(assetID);
                        entity.components.push_back(skeletonComponent);
                    }

                    auto GetNumber = [&](auto& value)
                    {
                        return float(value.IsNumber() ? value.Get<double>() : value.Get<int>());
                    };

                    for (auto& ext : node.extensions)
                    {
                        if (ext.first == "KHR_lights_punctual")
                        {
                            auto lightIdx = ext.second.Get<tinygltf::Value::Object>()["light"].Get<int>();

                            auto& extension     = model.extensions["KHR_lights_punctual"];
                            auto& lights        = extension.Get<tinygltf::Value::Object>()["lights"];

                            auto& light         = lights.Get(lightIdx);
                            auto& color         = light.Get("color");
                            float intensity     = GetNumber(light.Get("intensity"));
                            float range         = light.Has("range") ? ([](float F) { return F == 0.0 ? 1000 : F; }(GetNumber(light.Get("range")))) : 1000;

                            float3 K = {
                                GetNumber(color.Get(0)),
                                GetNumber(color.Get(1)),
                                GetNumber(color.Get(2)),
                            };

                            const auto pointLight = std::make_shared<EntityPointLightComponent>(K, float2{ intensity, range });
                            entity.components.push_back(pointLight);
                        }
                    }

                    sceneResource_ptr->AddSceneEntity(entity);
                }

                for (auto child : node.children)
                    AddNode(child, nodeIdx, AddNode);
            };

            for (auto nodeIdx : scene.nodes)
                AddNode(nodeIdx, -1, AddNode);


            if (scene.extras.Has("ResourceID"))
                sceneResource_ptr->GUID = scene.extras.Get("ResourceID").Get<int>();

            sceneResource_ptr->ID = scene.name;

            resources.push_back(sceneResource_ptr);
        }

        return resources;
    }


    /************************************************************************************************/


    struct ImageLoaderDesc
    {
        tinygltf::Model&                model;
        ResourceList&                   resources;
        std::map<int, Resource_ptr>&    imageMap;
    };


    bool loadImage(tinygltf::Image* image, const int image_idx, std::string* err, std::string* warn, int req_width, int req_height, const unsigned char* bytes, int size, void* user_ptr)
    {
        ImageLoaderDesc* loader = reinterpret_cast<ImageLoaderDesc*>(user_ptr);

        if (!std::filesystem::exists("tmp"))
            std::filesystem::create_directory("tmp");

        fmt::print("Saving image to disk, {}\n", image->name.c_str());
        std::string tempLocation = "tmp\\" + image->name;

        int x       = 0;
        int y       = 0;
        int comp    = 0;

        auto datass = stbi_load_from_memory(bytes, size, &x, &y, &comp, 3);
        auto res    = stbi_write_png(tempLocation.c_str(), x, y, comp, datass, comp * x);

        if (!res)
        {
            fmt::print("Failed to save image to disk, {}\n", tempLocation);
            return false;
        }

        stbi_image_free(datass);
        float* floats = (float*)datass;

        std::filesystem::path path{ tempLocation };

        auto resource = CreateTextureResource(path,std::string("DXT7"));
        resource->SetResourceID(image->name);

        std::filesystem::remove(path);

        std::cout << loader->resources.size() << " : " << image_idx << "\n";

        loader->resources.push_back(resource);
        loader->imageMap[image_idx] = resource;

        return true;
    }


    /************************************************************************************************/


    ResourceList GatherDeformers(tinygltf::Model& model)
    {
        ResourceList resources;

        for (auto& skin : model.skins)
        {
            std::vector<uint32_t>           parentLinkage;
            std::map<uint32_t, uint32_t>    nodeMap;
            std::vector<float4x4>           jointPoses;

            // Build linkage
            for (auto _ : skin.joints)
                parentLinkage.push_back((uint32_t)INVALIDHANDLE);

            for (auto _ : skin.joints)
                jointPoses.emplace_back();

            for (uint32_t I = 0; I < skin.joints.size(); I++)
                nodeMap[skin.joints[I]] = I;

            GUID_t ID = rand();

            const auto bufferViewIdx    = model.accessors[skin.inverseBindMatrices].bufferView;
            const auto& bufferView      = model.bufferViews[bufferViewIdx];
            const auto& buffer          = model.buffers[bufferView.buffer];

            const auto inverseMatrices  = reinterpret_cast<const FlexKit::float4x4*>(buffer.data.data() + bufferView.byteOffset);

            // Fetch properties
            for (auto& joint : skin.joints)
            {
                auto& node = model.nodes[joint];

                if (node.extras.Has("ResourceID") && node.extras.Get("ResourceID").IsInt())
                    ID = node.extras.Get("ResourceID").GetNumberAsInt();

                auto jointIdx = nodeMap[joint];

                for (auto& child : node.children)
                    parentLinkage[nodeMap[child]] = jointIdx;
            }

            auto skeleton   = std::make_shared<SkeletonResource>();
            skeleton->guid  = ID;
            skeleton->ID    = skin.name + ".skeleton";

            // Fetch Bind Pose
            for (size_t I = 0; I < parentLinkage.size(); I++)
            {
                auto parent         = JointHandle(parentLinkage[I]);
                auto inversePose    = Float4x4ToXMMATIRX(inverseMatrices[I]);

                SkeletonJoint joint;
                joint.mParent   = parent;
                joint.mID       = model.nodes[skin.joints[I]].name;
                skeleton->AddJoint(joint, inversePose);
            }

            resources.push_back(skeleton);
        }

        return resources;
    }


    /************************************************************************************************/


    ResourceList GatherAnimations(tinygltf::Model& model)
    {
        ResourceList resources;

        for (auto& animation : model.animations)
        {
            auto animationResource  = std::make_shared<AnimationResource>();

            animationResource->ID   = animation.name;
            auto& channels          = animation.channels;
            auto& samplers          = animation.samplers;

            for (auto& channel : channels)
            {
                AnimationResource::Track track;

                auto& sampler = samplers[channel.sampler];

                auto& input     = model.accessors[sampler.input];
                auto& output    = model.accessors[sampler.output];

                auto& inputView     = model.bufferViews[input.bufferView];
                auto& outputView    = model.bufferViews[output.bufferView];

                auto name = model.nodes[channel.target_node].name;

                track.targetChannel = AnimationTrackTarget{
                                        .Channel    = channel.target_path,
                                        .Target     = name,
                                        .type       = TrackType::Skeletal };

                union float3Value
                {
                    float floats[3];
                };



                const size_t    frameCount          = outputView.byteLength / 16;
                float*          startTime           = (float*)(model.buffers[inputView.buffer].data.data() + inputView.byteOffset);
                float3Value*    float3Channel       = (float3Value*)(model.buffers[outputView.buffer].data.data() + outputView.byteOffset);
                Quaternion*     roatationChannel    = (Quaternion*)(model.buffers[outputView.buffer].data.data() + outputView.byteOffset);

                const uint channelID =  channel.target_path == "rotation" ? 0 :
                                        channel.target_path == "translation" ? 1 :
                                        channel.target_path == "scale" ? 2 : -1;
                                    

                for (size_t I = 0; I < frameCount; ++I)
                {
                    AnimationKeyFrame keyFrame;
                    keyFrame.Begin          = startTime[I];
                    keyFrame.End            = startTime[I + 1];
                    keyFrame.interpolator   = AnimationInterpolator{ .Type = AnimationInterpolator::InterpolatorType::Linear };

                    switch(channelID)
                    {
                    case 0:
                    {
                        auto& outputValue   = roatationChannel[I];
                        keyFrame.Value      = float4{ outputValue[0], outputValue[1], outputValue[2], outputValue[3] };
                    }   break;
                    case 1:
                    {
                        auto& outputValue   = float3Channel[I];
                        keyFrame.Value      = float4{ outputValue.floats[0], outputValue.floats[1], outputValue.floats[2], 0 };
                    }   break;
                    }

                    track.KeyFrames.emplace_back(keyFrame);
                }

                animationResource->tracks.emplace_back(std::move(track));
            }

            resources.emplace_back(animationResource);
        }

        return resources;
    }


    /************************************************************************************************/


    ResourceList CreateSceneFromGlTF(const std::filesystem::path& fileDir, MetaDataList& MD)
    {
        using namespace tinygltf;
        Model model;
        TinyGLTF loader;
        std::string err;
        std::string warn;

        ResourceList                resources;
        std::map<int, Resource_ptr> imageMap;

        ImageLoaderDesc imageLoader{ model, resources, imageMap };
        loader.SetImageLoader(loadImage, &imageLoader);

        if (auto res = loader.LoadBinaryFromFile(&model, &err, &warn, fileDir.string()); res == true)
        {

            auto deformers                  = GatherDeformers(model);
            auto animations                 = GatherAnimations(model);
            auto [meshResources, meshMap]   = GatherGeometry(model);

            auto scenes = GatherScenes(model, meshMap, imageMap, deformers);

            for (auto resource : animations)
                resources.push_back(resource);

            for (auto resource : deformers)
                resources.push_back(resource);

            for (auto resource : meshResources)
                resources.push_back(resource);

            for (auto resource : scenes)
                resources.push_back(resource);
        }

        return resources;
    }


    /************************************************************************************************/


    ResourceBlob SceneResource::CreateBlob() const
    { 
        // Create Scene Node Table
        SceneNodeBlock::Header sceneNodeHeader;
        sceneNodeHeader.blockSize = (uint32_t)(SceneNodeBlock::GetHeaderSize() + sizeof(SceneNodeBlock::SceneNode) * nodes.size());
        sceneNodeHeader.blockType = SceneBlockType::NodeTable;
        sceneNodeHeader.nodeCount = (uint32_t)nodes.size();

        Blob nodeBlob{ sceneNodeHeader };
        for (auto& node : nodes)
        {
            SceneNodeBlock::SceneNode n;
            n.orientation	= node.orientation;
            n.position		= node.position;
            n.scale			= node.scale;
            n.parent		= node.parent;

            /*
            if (n.parent == 0)
            {// if Parent is root, bake rotation, fixes weird blender rotation issues
                n.orientation   = nodes[0].Q * n.orientation;
                n.position      = nodes[0].Q.Inverse() * n.position;
            }
            */

            nodeBlob += Blob{ n };
        }


        Blob entityBlock;
        for (auto& entity : entities)
        {
            EntityBlock::Header entityHeader;
            entityHeader.blockType         = SceneBlockType::EntityBlock;
            entityHeader.blockSize         = sizeof(EntityBlock::Header);
            entityHeader.componentCount    = 1;//2 + entity.components.size() + 1; // TODO: Create a proper material component

            Blob componentBlock;
            componentBlock      += CreateIDComponent(entity.id);
            //componentBlock      += CreateSceneNodeComponent(entity.);

            for (auto& component : entity.components)
            {
                if (component->id == GetTypeGUID(Brush))
                {
                    auto brushComponent = std::dynamic_pointer_cast<EntityBrushComponent>(component);
                    const auto ID       = TranslateID(brushComponent->MeshGuid, translationTable);

                    componentBlock += CreateBrushComponent(ID,
                                        brushComponent->material.albedo,
                                        brushComponent->material.specular);

                    componentBlock += CreateMaterialComponent(brushComponent->material);

                    entityHeader.componentCount+= 2;
                }
                else
                {
                    componentBlock += component->GetBlob();
                    entityHeader.componentCount++;
                }
            }

            entityHeader.blockSize += componentBlock.size();

            entityBlock += Blob{ entityHeader };
            entityBlock += componentBlock;
        }

        // Create Scene Resource Header
        SceneResourceBlob	header;
        header.blockCount   = 2 + entities.size();
        header.GUID         = GUID;
        strncpy(header.ID, ID.c_str(), 64);
        header.Type         = EResourceType::EResource_Scene;
        header.ResourceSize = sizeof(header) + nodeBlob.size() + entityBlock.size();
        Blob headerBlob{ header };


        auto [_ptr, size] = (headerBlob + nodeBlob + entityBlock).Release();

        ResourceBlob out;
        out.buffer			= (char*)_ptr;
        out.bufferSize		= size;
        out.GUID			= GUID;
        out.ID				= ID;
        out.resourceType	= EResourceType::EResource_Scene;

        return out;
    }


    /************************************************************************************************/


    EntityComponent_ptr SceneEntity::FindComponent(ComponentID id)
    {
        auto res = std::find_if(
            std::begin(components),
            std::end(components),
            [&](FlexKit::EntityComponent_ptr& component)
            {
                return component->id == id;
            });

        return res != std::end(components) ? *res : nullptr;
    }


    Blob CreateSceneNodeComponent(uint32_t nodeIdx)
    {
        SceneNodeComponentBlob nodeblob;
        nodeblob.excludeFromScene   = false;
        nodeblob.nodeIdx            = nodeIdx;

        return { nodeblob };
    }


    Blob CreateIDComponent(const std::string& string)
    {
        IDComponentBlob IDblob;
        strncpy_s(IDblob.ID, 64, string.c_str(), string.size());

        return { IDblob };
    }


    Blob CreateBrushComponent(GUID_t meshGUID, const float4 albedo, const float4 specular)
    {
        BrushComponentBlob brushComponent;
        brushComponent.resourceID        = meshGUID;
        brushComponent.albedo_smoothness = albedo;
        brushComponent.specular_metal    = specular;

        return { brushComponent };
    }


    Blob CreateMaterialComponent(BrushMaterial material)
    {
        MaterialComponentBlob materialComponent;

        for (auto& subMat : material.subMaterials)
        {
            SubMaterial subMaterialBlob;
            for (const auto texture : subMat.textures)
                subMaterialBlob.textures.push_back(texture);

            materialComponent.materials.push_back(subMaterialBlob);
        }

        return { materialComponent };
    }


    Blob CreatePointLightComponent(float3 K, float2 IR)
    {
        PointLightComponentBlob blob;
        blob.IR = IR;
        blob.K  = K;

        return blob;
    }


    Blob EntitySkeletonComponent::GetBlob()
    {
        SkeletonComponentBlob blob;
        blob.assetID = skeletonResourceID;

        return { blob };
    }


}/************************************************************************************************/



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
