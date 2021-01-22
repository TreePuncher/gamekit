#include "stdafx.h"
#include "buildsettings.h"

#include "Common.h"
#include "FBXResourceUtilities.h"

#include "AnimationUtilities.h"

#include "Scenes.h"
#include "MeshProcessing.h"

namespace FlexKit::ResourceBuilder
{   /************************************************************************************************/


    MeshResource_ptr FindGeoByID(GeometryList& geometry, size_t ID)
    {
        auto res = std::find_if(begin(geometry), end(geometry), [&](auto& v) { return v->TriMeshID == ID; });

        return (res != geometry.end()) ? *res : MeshResource_ptr{ nullptr };
    }


    /************************************************************************************************/


    void GatherAllGeometry(
        GeometryList&			geometry,
        fbxsdk::FbxNode*		node, 
        IDTranslationTable&	    Table, 
        const MetaDataList&		MD			= MetaDataList{}, 
        bool					subDiv		= false)
    {
        using FlexKit::AnimationClip;
        using FlexKit::Skeleton;

        auto AttributeCount = node->GetNodeAttributeCount();

        for (int itr = 0; itr < AttributeCount; ++itr)
        {
            auto Attr		= node->GetNodeAttributeByIndex(itr);
            auto nodeName	= node->GetName();

            switch (Attr->GetAttributeType())
            {
            case fbxsdk::FbxNodeAttribute::EType::eMesh:
            {
                const char* MeshName    = node->GetName();
                auto Mesh		        = (fbxsdk::FbxMesh*)Attr;
                bool found		        = false;
                bool LoadMesh	        = false;
                size_t uniqueID	        = (size_t)Mesh->GetUniqueID();
                auto Geo		        = FindGeoByID(geometry, uniqueID);

                MetaDataList RelatedMetaData;

    #if USING(RESCOMPILERVERBOSE)
                std::cout << "Found Mesh: " << MeshName << "\n";
    #endif
                RelatedMetaData = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName);

                if(!RelatedMetaData.size())
                    LoadMesh = true;

                const auto MeshInfo	= GetMeshMetaData(RelatedMetaData);
                const auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();

                if (!IDPresentInTable(Mesh->GetUniqueID(), Table))
                {
                    std::cout << "Building Mesh: " << MeshName << "\n";

                    auto skeleton       = CreateSkeletonResource(*Mesh, Name, MD);
                    MeshDesc meshDesc   = TranslateToTokens(*Mesh, skeleton, false);

                    const FbxAMatrix transform  = node->EvaluateGlobalTransform();
                    MeshResource_ptr resource   = CreateMeshResource(&meshDesc, 1, Name, MD, false);

                    resource->BakeTransform(FBXMATRIX_2_FLOAT4X4((transform)));
                    
                    if(MeshInfo)
                    {
                        resource->TriMeshID		= MeshInfo->guid;
                        resource->ID			= MeshInfo->MeshID;
                    }
                    else
                    {
                        resource->TriMeshID		= CreateRandomID();
                        resource->ID			= Mesh->GetName();
                    }

                    Table.push_back({ Mesh->GetUniqueID(), resource->TriMeshID });

                    if constexpr (USING(RESCOMPILERVERBOSE))
                        std::cout << "Compiled Resource: " << Name << "\n";

                    geometry.push_back(resource);
                }
            }	break;
            }
        }

        const size_t NodeCount = node->GetChildCount();
        for(int itr = 0; itr < NodeCount; ++itr)
            GatherAllGeometry(geometry, node->GetChild(itr), Table, MD, subDiv);
    }


    /************************************************************************************************/


    ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, IDTranslationTable& Table, const bool LoadSkeletalData = false, const MetaDataList& MD = MetaDataList{}, const bool subDivEnabled = false)
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
                        metaInfo->UserType == MetaData::EMETA_RECIPIENT_TYPE::EMR_SKELETON &&
                        metaInfo->ID == geometry->ID;
                };

                auto results = filter(MD, pred);

                for (const auto r : results)
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
        std::cout << "Created " << resources.size() << " Resource Blobs\n";
#endif

        return resources;
    }




    void ProcessNodes(fbxsdk::FbxNode* Node, SceneResource_ptr scene, const MetaDataList& MD = MetaDataList{}, size_t Parent = -1)
    {
        bool SkipChildren = false;
        auto AttributeCount = Node->GetNodeAttributeCount();

        const auto Position = Node->LclTranslation.Get();
        const auto LclScale = Node->LclScaling.Get();
        const auto rotation = Node->LclRotation.Get();
        const auto NodeName = Node->GetName();

        SceneNode NewNode;
        NewNode.parent = Parent;
        NewNode.position = TranslateToFloat3(Position);
        NewNode.scale = TranslateToFloat3(LclScale);
        NewNode.Q = Quaternion((float)rotation.mData[0], (float)rotation.mData[1], (float)rotation.mData[2]);

        const uint32_t Nodehndl = scene->AddSceneNode(NewNode);

        for (int i = 0; i < AttributeCount; ++i)
        {
            auto Attr = Node->GetNodeAttributeByIndex(i);
            auto AttrType = Attr->GetAttributeType();

            switch (AttrType)
            {
            case FbxNodeAttribute::eMesh:
            {
#if USING(RESCOMPILERVERBOSE)
                std::cout << "Entity Found: " << Node->GetName() << "\n";
#endif
                auto FBXMesh = static_cast<fbxsdk::FbxMesh*>(Attr);
                auto UniqueID = FBXMesh->GetUniqueID();
                auto name = Node->GetName();

                const auto materialCount = Node->GetMaterialCount();
                const auto shadingMode = Node->GetShadingMode();

                for (int I = 0; I < materialCount; I++)
                {
                    auto classID = Node->GetMaterial(I)->GetClassId();
                    auto material = Node->GetSrcObject<FbxSurfacePhong>(I);
                    auto materialName = material->GetName();
                    auto diffuse = material->sDiffuse;
                    auto normal = material->sNormalMap;
                    bool multilayer = material->MultiLayer;
                }

                SceneEntity entity;
                entity.components.push_back(std::make_shared<DrawableComponent>(UniqueID));
                entity.Node = Nodehndl;
                entity.id = name;
                entity.id = std::string(name);

                scene->AddSceneEntity(entity);
            }	break;
            case FbxNodeAttribute::eLight:
            {
#if USING(RESCOMPILERVERBOSE)
                std::cout << "Light Found: " << Node->GetName() << "\n";
#endif
                const auto FBXLight = static_cast<fbxsdk::FbxLight*>(Attr);
                const auto Type = FBXLight->LightType.Get();
                const auto Cast = FBXLight->CastLight.Get();
                const auto I = (float)FBXLight->Intensity.Get() / 10;
                const auto K = FBXLight->Color.Get();
                const auto R = FBXLight->OuterAngle.Get();
                const auto radius = FBXLight->FarAttenuationStart.Get();
                const auto decay = FBXLight->DecayStart.Get();

                SceneEntity entity;
                entity.Node = Nodehndl;
                entity.id = std::string(Node->GetName());

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


    void ScanChildrenNodesForScene(
        fbxsdk::FbxNode*    Node,
        const MetaDataList& MetaData,
        IDTranslationTable& translationTable,
        ResourceList&       Out)
    {
        const auto nodeName = Node->GetName();
        const auto RelatedMetaData = FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName());
        const auto NodeCount = Node->GetChildCount();

        if (RelatedMetaData.size())
        {
            for (auto& i : RelatedMetaData)
            {
                if (i->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
                {
                    const auto			MD = std::static_pointer_cast<Scene_MetaData>(i);
                    SceneResource_ptr	scene = std::make_shared<SceneResource>();

                    scene->GUID = MD->Guid;
                    scene->ID = MD->SceneID;
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


    void GetScenes(fbxsdk::FbxScene* S, const MetaDataList& MetaData, IDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();
        ScanChildrenNodesForScene(Root, MetaData, translationTable, Out);
    }


    /************************************************************************************************/


    void MakeScene(fbxsdk::FbxScene* S, IDTranslationTable& translationTable, ResourceList& Out)
    {
        auto Root = S->GetRootNode();

        SceneResource_ptr	scene = std::make_shared<SceneResource>();

        scene->ID = Root->GetName();
        scene->translationTable = translationTable;

        ProcessNodes(Root, scene, {});
        Out.push_back(scene);
    }


    /************************************************************************************************/


    ResourceList CreateSceneFromFBXFile(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc, const MetaDataList& metaData)
    {
        IDTranslationTable  translationTable;
        ResourceList	    resources = GatherSceneResources(scene, Desc.Cooker, translationTable, true, metaData);

        GetScenes(scene, metaData, translationTable, resources);

        return resources;
    }


    /************************************************************************************************/


    std::pair<ResourceList, std::shared_ptr<SceneResource>>  CreateSceneFromFBXFile2(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc)
    {
        IDTranslationTable	    translationTable;
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


    Pair<bool, fbxsdk::FbxScene*>
    LoadFBXScene(const char* file, fbxsdk::FbxManager* lSdkManager, fbxsdk::FbxIOSettings* settings)
    {
        fbxsdk::FbxNode* node = nullptr;
        fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(lSdkManager, "");

        if (!importer->Initialize(file, -1, lSdkManager->GetIOSettings()))
        {
            printf("Failed to Load: %s\n", file);
            printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
            return{ false, nullptr };
        }

        fbxsdk::FbxScene* scene = FbxScene::Create(lSdkManager, "Scene");
        if (!importer->Import(scene))
        {
            printf("Failed to Load: %s\n", file);
            printf("Error Returned: %s\n", importer->GetStatus().GetErrorString());
            return{ false, nullptr };
        }

        if (const auto& axisSystem = scene->GetGlobalSettings().GetAxisSystem();
            axisSystem != FbxAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded))
        {
            std::cout << "Converting scene axis system\n";

            FbxAxisSystem newAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::EFrontVector::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded);
            newAxisSystem.DeepConvertScene(scene);
        }

        return{ true, scene };
    }

}   /************************************************************************************************/


/**********************************************************************

Copyright (c) 2019-2021 Robert May

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
