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
                entity.components.push_back(make_shared<DrawableComponent>(UniqueID));
                entity.Node		= Nodehndl;
                entity.id		= name;

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
                entity.id   = Node->GetName();

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
            entityHeader.componentCount    = 2 + entity.components.size();

            auto componentBlock  = CreateIDComponent(entity.id);
            componentBlock      += CreateSceneNodeComponent(entity.Node);

            for (auto& component : entity.components)
            {
                if (component->id == GetTypeGUID(DrawableComponent))
                {
                    auto drawableComponent = std::dynamic_pointer_cast<DrawableComponent>(component);
                    const auto ID = TranslateID(drawableComponent->MeshGuid, translationTable);
                    componentBlock += CreateDrawableComponent(ID, drawableComponent->albedo, drawableComponent->specular);
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
        strncpy_s(IDblob.ID, string.c_str(), min(strnlen_s(IDblob.ID, sizeof(IDblob.ID)), string.size()));

        return { IDblob };
    }


    Blob CreateDrawableComponent(GUID_t meshGUID, float4 albedo_S, float4 specular_M)
    {
        DrawableComponentBlob drawableComponent;
        drawableComponent.resourceID        = meshGUID;
        drawableComponent.albedo_smoothness = albedo_S;
        drawableComponent.specular_metal    = specular_M;

        return { drawableComponent };
    }


    Blob CreatePointLightComponent(float3 K, float2 IR)
    {
        PointLightComponentBlob blob;
        blob.IR = IR;
        blob.K  = K;

        return blob;
    }


}/************************************************************************************************/
