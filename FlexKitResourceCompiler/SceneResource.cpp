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
#include "TextureResourceUtilities.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <regex>
#include <filesystem>
#include <fmt\format.h>

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    void ProcessNodes(fbxsdk::FbxNode* Node, SceneResource_ptr scene, const MetaDataList& MD = MetaDataList{}, size_t Parent = -1)
    {
        bool SkipChildren	= false;
        auto AttributeCount	= Node->GetNodeAttributeCount();
        
        const auto Position = Node->LclTranslation.Get();
        const auto LclScale = Node->LclScaling.Get();
        const auto rotation = Node->LclRotation.Get();
        const auto NodeName = Node->GetName();

        SceneNode NewNode;
        NewNode.parent		= Parent;
        NewNode.position	= TranslateToFloat3(Position); 
        NewNode.scale		= TranslateToFloat3(LclScale);
        NewNode.Q			= Quaternion((float)rotation.mData[0], (float)rotation.mData[1], (float)rotation.mData[2]);

        const uint32_t Nodehndl = scene->AddSceneNode(NewNode);

        for (int i = 0; i < AttributeCount; ++i)
        {
            auto Attr       = Node->GetNodeAttributeByIndex(i);
            auto AttrType   = Attr->GetAttributeType();

            switch (AttrType)
            {
            case FbxNodeAttribute::eMesh:
            {
    #if USING(RESCOMPILERVERBOSE)
                std::cout << "Entity Found: " << Node->GetName() << "\n";
    #endif
                auto FBXMesh	= static_cast<fbxsdk::FbxMesh*>(Attr);
                auto UniqueID	= FBXMesh->GetUniqueID();
                auto name		= Node->GetName();

                const auto materialCount    = Node->GetMaterialCount();
                const auto shadingMode      = Node->GetShadingMode();

                for (int I = 0; I < materialCount; I++)
                {
                    auto classID        = Node->GetMaterial(I)->GetClassId();
                    auto material       = Node->GetSrcObject<FbxSurfacePhong>(I);
                    auto materialName   = material->GetName();
                    auto diffuse        = material->sDiffuse;
                    auto normal         = material->sNormalMap;
                    bool multilayer     = material->MultiLayer;
                }

                SceneEntity entity;
                entity.components.push_back(std::make_shared<DrawableComponent>(UniqueID));
                entity.Node		= Nodehndl;
                entity.id		= name;
                entity.id       = std::string(name);

                scene->AddSceneEntity(entity);
            }	break;
            case FbxNodeAttribute::eLight:
            {
    #if USING(RESCOMPILERVERBOSE)
                std::cout << "Light Found: " << Node->GetName() << "\n";
    #endif
                const auto FBXLight    = static_cast<fbxsdk::FbxLight*>(Attr);
                const auto Type        = FBXLight->LightType.Get();
                const auto Cast        = FBXLight->CastLight.Get();
                const auto I           = (float)FBXLight->Intensity.Get()/10;
                const auto K           = FBXLight->Color.Get();
                const auto R           = FBXLight->OuterAngle.Get();
                const auto radius	   = FBXLight->FarAttenuationStart.Get();
                const auto decay       = FBXLight->DecayStart.Get();

                SceneEntity entity;
                entity.Node = Nodehndl;
                entity.id   = std::string(Node->GetName());

                entity.components.push_back(std::make_shared<PointLightComponent>(TranslateToFloat3(K), float2{ 40, 40 }));

                scene->AddSceneEntity(entity);
            }	break;
            case FbxNodeAttribute::eMarker:
            case FbxNodeAttribute::eUnknown:
            default:
                break;
            }
        }

        if (!SkipChildren)
        {
            const auto ChildCount = Node->GetChildCount();
            for (int I = 0; I < ChildCount; ++I)
                ProcessNodes(Node->GetChild(I), scene, MD, Nodehndl);
        }
    }


    /************************************************************************************************/


    ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, FBXIDTranslationTable& Table, const bool LoadSkeletalData = false, const MetaDataList& MD = MetaDataList{}, const bool subDivEnabled = false)
    {
        GeometryList geometryFound;
        ResourceList resources;

        GatherAllGeometry(geometryFound, S->GetRootNode(), Table, MD, subDivEnabled);

    #if USING(RESCOMPILERVERBOSE)
        std::cout << "CompileAllGeometry Compiled " << geometryFound.size() << " Resources\n";
    #endif
        for (auto geometry : geometryFound)
        {
            resources.push_back(geometry);

            if (geometry->Skeleton)
            {
                resources.push_back(geometry->Skeleton);

                // TODO: scan metadata for animation clips, then add those to the resource list, everything in the resource list gets output in the resource file
                auto pred = [&](MetaData_ptr metaInfo) -> bool
                    {
                        return
                            metaInfo->UserType  == MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON &&
                            metaInfo->ID        == geometry->ID;
                    };

                auto results = filter(MD, pred);

                for( const auto r : results )
                {
                    switch (r->type)
                    {
                    case MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP:    break;
                    case MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT:   break;
                    case MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION:
                    {   
                        auto skeletonMetaData = static_pointer_cast<Skeleton_MetaData>(r);
                    }   break;
                    default:
                        break;
                    }
                }
            }
            /*
            auto RelatedMD	= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, geometry.ID);

            for(size_t J = 0; J < RelatedMD.size(); ++J)
            {
                switch (RelatedMD[J]->type)
                {
                case MetaData::EMETAINFOTYPE::EMI_COLLIDER:
                {
                    if (!Cooker)
                        continue;

                    Collider_MetaData*	ColliderInfo	= (Collider_MetaData*)RelatedMD[J];
                    ColliderStream		Stream			= ColliderStream(SystemAllocator, 2048);

                    using physx::PxTriangleMeshDesc;
                    PxTriangleMeshDesc meshDesc;
                    meshDesc.points.count     = geometry.Buffers[0]->GetBufferSizeUsed();
                    meshDesc.points.stride    = geometry.Buffers[0]->GetElementSize();
                    meshDesc.points.data      = geometry.Buffers[0]->GetBuffer();

                    uint32_t* Indexes = (uint32_t*)SystemAllocator._aligned_malloc(geometry.Buffers[15]->GetBufferSizeRaw());

                    {
                        struct Tri {
                            uint32_t Indexes[3];
                        };

                        auto Proxy		= geometry.Buffers[15]->CreateTypedProxy<Tri>();
                        size_t Position = 0;
                        auto itr		= Proxy.begin();
                        auto end		= Proxy.end();

                        while(itr < end) {
                            Indexes[Position + 0] = (*itr).Indexes[1];
                            Indexes[Position + 2] = (*itr).Indexes[2];
                            Indexes[Position + 1] = (*itr).Indexes[0];
                            Position += 3;
                            itr++;
                        }
                    }

                    auto IndexBuffer		  = geometry.IndexBuffer_Idx;
                    meshDesc.triangles.count  = geometry.IndexCount / 3;
                    meshDesc.triangles.stride = geometry.Buffers[15]->GetElementSize() * 3;
                    meshDesc.triangles.data   = Indexes;

    #if USING(RESCOMPILERVERBOSE)
                    printf("BEGINNING MODEL BAKING!\n");
                    std::cout << "Baking: " << geometry.ID << "\n";
    #endif
                    bool success = false;
                    if(Cooker) success = Cooker->cookTriangleMesh(meshDesc, Stream);

    #if USING(RESCOMPILERVERBOSE)
                    if(success)
                        printf("MODEL FINISHED BAKING!\n");
                    else
                        printf("MODEL FAILED BAKING!\n");

    #else
                    FK_ASSERT(success, "FAILED TO COOK MESH!");
    #endif

                    if(success)
                    {
                        auto Blob = CreateColliderResourceBlob(Stream.Buffer, Stream.used, ColliderInfo->Guid, ColliderInfo->ColliderID, SystemAllocator);
                        ResourcesFound.push_back(Blob);
                    }
                }	break;
                default:
                    break;
                } 
            }
            */
        }

    #if USING(RESCOMPILERVERBOSE)
        std::cout << "Created "<< resources.size() << " Resource Blobs\n";
    #endif

        return resources;
    }


    /************************************************************************************************/


    void ScanChildrenNodesForScene(
        fbxsdk::FbxNode*		Node, 
        const MetaDataList&		MetaData, 
        FBXIDTranslationTable&	translationTable, 
        ResourceList&			Out)
    {
        const auto nodeName			= Node->GetName();
        const auto RelatedMetaData	= FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName());
        const auto NodeCount		= Node->GetChildCount();

        if (RelatedMetaData.size())
        {
            for (auto& i : RelatedMetaData)
            {
                if (i->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
                {
                    const auto			MD		= std::static_pointer_cast<Scene_MetaData>(i);
                    SceneResource_ptr	scene	= std::make_shared<SceneResource>();

                    scene->GUID				= MD->Guid;
                    scene->ID				= MD->SceneID;
                    scene->translationTable = translationTable;

                    ProcessNodes(Node, scene, MD->sceneMetaData);
                    Out.push_back(scene);
                }
            }
        }
        else
        {
            for (int itr = 0; itr < NodeCount; ++itr) {
                auto Child = Node->GetChild(itr);
                ScanChildrenNodesForScene(Child, MetaData, translationTable, Out);
            }
        }
    }


    /************************************************************************************************/


    void GetScenes(fbxsdk::FbxScene* S, const MetaDataList& MetaData, FBXIDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();
        ScanChildrenNodesForScene(Root, MetaData, translationTable, Out);
    }


    /************************************************************************************************/


    void MakeScene(fbxsdk::FbxScene* S, FBXIDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();

        SceneResource_ptr	scene = std::make_shared<SceneResource>();

        scene->ID               = Root->GetName();
        scene->translationTable = translationTable;

        ProcessNodes(Root, scene, {});
        Out.push_back(scene);
    }


    /************************************************************************************************/


    ResourceList CreateSceneFromFBXFile(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc, const MetaDataList& metaData)
    {
        FBXIDTranslationTable	translationTable;
        ResourceList			resources = GatherSceneResources(scene, Desc.Cooker, translationTable, true, metaData);

        GetScenes(scene, metaData, translationTable, resources);

        return resources;
    }


    /************************************************************************************************/


    std::pair<ResourceList, std::shared_ptr<SceneResource>>  CreateSceneFromFBXFile2(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc)
    {
        FBXIDTranslationTable	translationTable;
        ResourceList			resources = GatherSceneResources(scene, Desc.Cooker, translationTable, true, {});
        ResourceList            sceneRes;

        MakeScene(scene, translationTable, sceneRes);
        // Translate ID's right now
        auto localScene = std::dynamic_pointer_cast<SceneResource>(sceneRes.back());
        for (auto& entity : localScene->entities)
        {
            for (auto& component : entity.components)
            {
                if (component->id == GetTypeGUID(DrawableComponent))
                {
                    auto drawable = std::dynamic_pointer_cast<DrawableComponent>(component);
                    drawable->MeshGuid = TranslateID(drawable->MeshGuid, translationTable);
                }
            }
        }
        return { resources, localScene };
    }


    /************************************************************************************************/


#include "MeshUtils.h"


    std::pair<ResourceList, std::vector<size_t>>  GatherGeometry(tinygltf::Model& model)
    {
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

                    auto stride         = bufferView.byteStride == 0 ? GetComponentSizeInBytes(bufferAcessor.componentType) * GetTypeSizeInBytes(bufferAcessor.type) : bufferView.byteStride;
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
                            newMesh.MinV[i] = bufferAcessor.minValues[i];
                            newMesh.MaxV[i] = bufferAcessor.maxValues[i];
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

                            auto tangentStride      = tangentView.byteStride == 0 ? GetComponentSizeInBytes(tangentAcessor.componentType) * GetTypeSizeInBytes(tangentAcessor.type) : tangentView.byteStride;

                            for (size_t I = 0; I < elementCount; I++)
                            {
                                float4 normal;
                                float4 tangent;

                                memcpy(&normal, buffer + stride * I, stride);
                                memcpy(&tangent, tangentbuffer + tangentStride * I, tangentStride);

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

                        const auto weightsStride = weightsView.byteStride == 0 ? GetComponentSizeInBytes(weightsAcessor.componentType) * GetTypeSizeInBytes(weightsAcessor.type) : weightsView.byteStride;

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

                const auto stride         = indexBufferView.byteStride == 0 ? GetComponentSizeInBytes(componentType) * GetTypeSizeInBytes(type) : indexBufferView.byteStride;
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

            meshResource->BS.r      = 1000;
            meshResource->TriMeshID = GUID;

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
            auto& metaData  = FindRelatedMetaData({}, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, name); // TODO: allow for meta data to be included from external files

            std::map<int, int> sceneNodeMap;

            auto AddNode =
                [&](const int nodeIdx, const int parent, auto& AddNode) -> void
            {
                auto& node = model.nodes[nodeIdx];

                SceneNode newNode;
                newNode.parent = parent != -1 ? sceneNodeMap[parent] : -1;

                if (node.translation.size())
                    for (size_t I = 0; I < 3; I++)
                        newNode.position[I] = node.translation[I];
                else
                    newNode.position = float3{ 0, 0, 0 };

                if (node.rotation.size())
                    for (size_t I = 0; I < 4; I++)
                        newNode.Q[I] = node.rotation[I];
                else
                    newNode.Q = Quaternion{ 0, 0, 0, 1 };


                newNode.scale = float3(1, 1, 1);

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
                        auto drawable = std::make_shared<DrawableComponent>(meshMap[node.mesh]);

                        for (auto& materialID : model.meshes[node.mesh].primitives)
                        {
                            DrawableMaterial newMaterial;

                            if (materialID.material != -1)
                            {
                                auto& material = model.materials[materialID.material];

                                auto AddTexture = [&](const ParameterMap& material, auto& string) {
                                    if (auto res = material.find(string); res != material.end())
                                    {
                                        auto index = res->second.json_double_value.find("index")->second;

                                        if (imageMap.find(index) != imageMap.end())
                                        {
                                            auto assetGUID = imageMap[uint32_t(index)]->GetResourceGUID();
                                            newMaterial.textures.push_back(assetGUID);
                                        }
                                        else
                                            newMaterial.textures.push_back(INVALIDHANDLE);
                                    }
                                };

                                AddTexture(material.values, "baseColorTexture");

                                for (auto& value : material.additionalValues)
                                {
                                    if (value.first == "normalTexture")
                                    {
                                        auto asset = imageMap[uint32_t(value.second.TextureIndex())];

                                        if(asset)
                                            newMaterial.textures.push_back(asset->GetResourceGUID());
                                    }
                                }
                            }

                            drawable->material.subMaterials.push_back(newMaterial);
                        }



                        entity.components.push_back(drawable);
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

        int x = 0;
        int y = 0;
        int comp = 0;

        auto datass = stbi_load_from_memory(bytes, size, &x, &y, &comp, 3);
        auto res = stbi_write_png(image->name.c_str(), x, y, comp, datass, comp * x);

        stbi_image_free(datass);
        float* floats = (float*)datass;


        std::filesystem::path path{ image->name };

        auto resource = CreateTextureResource(path,std::string("DXT7"));

        std::filesystem::remove(path);

        loader->resources.push_back(resource);
        loader->imageMap[image_idx] = resource;

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
                    parentLinkage.push_back(INVALIDHANDLE);

                for (size_t I = 0; I < skin.joints.size(); I++)
                    nodeMap[skin.joints[I]] = I;

                for (auto& joint :skin.joints)
                {
                    auto node = model.nodes[joint];

                    Joint j;
                    j.mID = node.name.c_str();

                    JointPose pose;
                    if (node.translation.size())
                    {
                        pose.ts.x = node.translation[0];
                        pose.ts.y = node.translation[1];
                        pose.ts.z = node.translation[2];
                    }
                    else
                        pose.ts = float4{ 0, 0, 0, 1 };

                    if (node.rotation.size())
                    {
                        pose.r[0] = node.rotation[0];
                        pose.r[1] = node.rotation[1];
                        pose.r[2] = node.rotation[2];
                        pose.r[2] = node.rotation[3];
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
