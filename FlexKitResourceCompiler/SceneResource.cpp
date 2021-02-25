/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


#ifdef _DEBUG
#define DEBUGPASS 0
#else
#define DEBUGPASS 0
#endif

    std::pair<ResourceList, std::vector<size_t>>  GatherGeometry(tinygltf::Model& model)
    {
#if DEBUGPASS 
        return {};
#else
        using namespace tinygltf;

        ResourceList        resources;
        std::vector<size_t> meshMap;

        for (auto& mesh : model.meshes)
        {
            std::vector<MeshDesc> subMeshes;
            uint64_t GUID = rand();

            if (mesh.extras.Has("ResourceID"))
            {
                auto& resourceID = mesh.extras.Get("ResourceID");
                if (resourceID.IsInt())
                    GUID = resourceID.Get<int>();
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

            for (auto& primitive : mesh.primitives)
            {
                MeshDesc newMesh;
                newMesh.tokens = FlexKit::MeshUtilityFunctions::TokenList{ SystemAllocator };

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
                            MeshUtilityFunctions::OBJ_Tools::AddVertexToken(xyz, newMesh.tokens);
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

                                MeshUtilityFunctions::OBJ_Tools::AddNormalToken(normal.xyz(), tangent.xyz(), newMesh.tokens);
                            }
                        }
                        else
                        {
                            newMesh.Normals = true;

                            for (size_t I = 0; I < elementCount; I++)
                            {
                                float3 normal;

                                memcpy(&normal, buffer + stride * I, stride);
                                MeshUtilityFunctions::OBJ_Tools::AddNormalToken(normal, newMesh.tokens);
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
                            MeshUtilityFunctions::OBJ_Tools::AddTexCordToken(coord, idx, newMesh.tokens);
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
                            float3      weights;

                            memcpy(&joints, buffer + stride * I, stride);
                            memcpy(&weights, weightsbuffer + stride * I, weightsStride);
                            MeshUtilityFunctions::OBJ_Tools::AddWeightToken({ weights, joints }, newMesh.tokens);
                        }
                    }

                }

                auto& indexAccessor     = model.accessors[primitive.indices];
                auto& indexBufferView   = model.bufferViews[indexAccessor.bufferView];
                auto& indexBuffer       = model.buffers[indexBufferView.buffer];
                     
                const auto componentType    = indexAccessor.componentType;
                const auto type             = indexAccessor.type;

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

                    newMesh.tokens.push_back(tokens[0]);
                    newMesh.tokens.push_back(tokens[2]);
                    newMesh.tokens.push_back(tokens[1]);
                }

                subMeshes.push_back(newMesh);
            }

            auto meshResource = CreateMeshResource(subMeshes.data(), subMeshes.size(), mesh.name, {}, false);
            resources.push_back(meshResource);


            meshResource->TriMeshID = GUID;

            meshMap.push_back(GUID);
        }

        return { resources, meshMap };
#endif
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
                        newNode.Q[I] = (float)node.rotation[I];
                else
                    newNode.Q = Quaternion{ 0, 0, 0, 1 };


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
                    entity.Node = nodeHndl;

                    if (node.mesh != -1)
                    {

#if DEBUGPASS
#else
                        auto drawable = std::make_shared<DrawableComponent>(meshMap[node.mesh]);
#endif
                        for (auto& subEntity : model.meshes[node.mesh].primitives)
                        {
                            DrawableMaterial newMaterial;

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
                                            std::cout << "Texture Not Found!\n" << "Failed to find: " << string << "\n";
                                    }
                                };


                                if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
                                {
                                    auto idx = model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source;
                                    newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                }
                                if (material.normalTexture.index != -1)
                                {
                                    auto idx = model.textures[material.normalTexture.index].source;
                                    newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                }

                                if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
                                {
                                    auto idx = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
                                    newMaterial.textures.push_back(imageMap.at(idx)->GetResourceGUID());
                                }
                            }
#if DEBUGPASS
#else
                            drawable->material.subMaterials.push_back(newMaterial);
#endif
                        }
#if DEBUGPASS
#else
                        entity.components.push_back(drawable);
#endif
                    }

                    if (node.skin != -1)
                    {
                        auto assetID = skinMap[node.skin]->GetResourceGUID();
                        auto skeletonComponent = std::make_shared<SkeletonEntityComponent>(assetID);
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

                            const auto pointLight = std::make_shared<PointLightComponent>(K, float2{ intensity, range });
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

#if DEBUGPASS
        auto resource = std::make_shared<TextureResource>();
        resource->ID = image->name;

        loader->resources.push_back(resource);
        loader->imageMap[image_idx] = resource;

        std::cout << image_idx << "\n";

        return true;
#else

        int x = 0;
        int y = 0;
        int comp = 0;

        auto datass = stbi_load_from_memory(bytes, size, &x, &y, &comp, 3);
        auto res    = stbi_write_png(image->name.c_str(), x, y, comp, datass, comp * x);

        stbi_image_free(datass);
        float* floats = (float*)datass;


        std::filesystem::path path{ image->name };

        auto resource = CreateTextureResource(path,std::string("DXT7"));

        std::filesystem::remove(path);

        std::cout << loader->resources.size() << " : " << image_idx << "\n";

        loader->resources.push_back(resource);
        loader->imageMap[image_idx] = resource;

#endif

        return true;
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
        std::vector<Resource_ptr>   skinMap;

        ImageLoaderDesc imageLoader{ model, resources, imageMap };
        loader.SetImageLoader(loadImage, &imageLoader);

        if (auto res = loader.LoadBinaryFromFile(&model, &err, &warn, fileDir.string()); res == true)
        {
            // Gather Skins
            for (auto& skin : model.skins)
            {
                std::vector<uint32_t>           parentLinkage;
                std::map<uint32_t, uint32_t>    nodeMap;
                std::vector<float4x4>           jointPoses;

                for (auto _ : skin.joints)
                    parentLinkage.push_back((uint32_t)INVALIDHANDLE);

                for (uint32_t I = 0; I < skin.joints.size(); I++)
                    nodeMap[skin.joints[I]] = I;

                for (auto& joint :skin.joints)
                {
                    auto node = model.nodes[joint];

                    Joint j;
                    j.mID = node.name.c_str();

                    JointPose pose;
                    if (node.translation.size())
                    {
                        pose.ts.x = (float)node.translation[0];
                        pose.ts.y = (float)node.translation[1];
                        pose.ts.z = (float)node.translation[2];
                    }
                    else
                        pose.ts = float4{ 0, 0, 0, 1 };

                    if (node.rotation.size())
                    {
                        pose.r[0] = (float)node.rotation[0];
                        pose.r[1] = (float)node.rotation[1];
                        pose.r[2] = (float)node.rotation[2];
                        pose.r[2] = (float)node.rotation[3];
                    }
                    else
                        pose.r = Quaternion{ 0, 0, 0, 1 };

                    jointPoses.push_back(GetPoseTransform(pose));

                    auto jointIdx = nodeMap[joint];
                    for (auto& child : node.children)
                        parentLinkage[nodeMap[child]] = jointIdx;
                }

                auto skeleton   = std::make_shared<SkeletonResource>();
                skeleton->guid  = rand();
                skeleton->ID    = skin.name + ".skeleton";

                for (size_t I = 0; I < parentLinkage.size(); I++)
                {
                    auto parent         = JointHandle(parentLinkage[I]);
                    auto jointPose      = GetPose(Float4x4ToXMMATIRX(jointPoses[I]));
                    auto parentPose     = parent != 0xffff ? jointPoses[parent] : float4x4::Identity();
                    auto inversePose    = DirectX::XMMatrixInverse(nullptr, Float4x4ToXMMATIRX(jointPoses[I]));

                    SkeletonJoint joint;
                    joint.mParent = parent;
                    skeleton->AddJoint(joint, inversePose);
                }

                resources.push_back(skeleton);

                skinMap.push_back(skeleton);
            }

            auto [meshResources, meshMap] = GatherGeometry(model);
            auto scenes = GatherScenes(model, meshMap, imageMap, skinMap);

            for (auto resource : meshResources)
                resources.push_back(resource);

            for (auto resource : scenes)
                resources.push_back(resource);
        }

        return resources;
    }


    /************************************************************************************************/


    ResourceBlob SceneResource::CreateBlob()
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
            n.orientation	= node.Q;
            n.position		= node.position;
            n.scale			= node.scale;
            n.parent		= node.parent;

            if(n.parent == -1)
                n.orientation = Quaternion::Identity();

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
            entityHeader.blockType         = SceneBlockType::Entity;
            entityHeader.blockSize         = sizeof(EntityBlock);
            entityHeader.componentCount    = 2 + entity.components.size() + 1; // TODO: Create a proper material component

            Blob componentBlock;
            componentBlock      += CreateIDComponent(entity.id);
            componentBlock      += CreateSceneNodeComponent(entity.Node);

            for (auto& component : entity.components)
            {
                if (component->id == GetTypeGUID(DrawableComponent))
                {
                    auto drawableComponent = std::dynamic_pointer_cast<DrawableComponent>(component);
                    const auto ID = TranslateID(drawableComponent->MeshGuid, translationTable);
                    componentBlock += CreateDrawableComponent(ID,
                                        drawableComponent->material.albedo,
                                        drawableComponent->material.specular);

                    componentBlock += CreateMaterialComponent(drawableComponent->material);
                }
                else
                    componentBlock += component->GetBlob();
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


    Blob CreateSceneNodeComponent(uint32_t nodeIdx)
    {
        SceneNodeComponentBlob nodeblob;
        nodeblob.excludeFromScene   = false;
        nodeblob.nodeIdx            = nodeIdx;

        return { nodeblob };
    }


    Blob CreateIDComponent(std::string& string)
    {
        IDComponentBlob IDblob;
        strncpy_s(IDblob.ID, 64, string.c_str(), string.size());

        return { IDblob };
    }


    Blob CreateDrawableComponent(GUID_t meshGUID, const float4 albedo, const float4 specular)
    {
        DrawableComponentBlob drawableComponent;
        drawableComponent.resourceID        = meshGUID;
        drawableComponent.albedo_smoothness = albedo;
        drawableComponent.specular_metal    = specular;

        

        return { drawableComponent };
    }


    Blob CreateMaterialComponent(DrawableMaterial material)
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


    Blob SkeletonEntityComponent::GetBlob()
    {
        SkeletonComponentBlob blob;
        blob.assetID = skeletonResourceID;

        return { blob };
    }


}/************************************************************************************************/
