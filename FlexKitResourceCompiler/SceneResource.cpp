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


/************************************************************************************************/


void ProcessNodes(fbxsdk::FbxNode* Node, SceneResource_ptr scene, const MetaDataList& MD = MetaDataList{}, size_t Parent = -1)
{
	bool SkipChildren		= false;
	size_t AttributeCount	= Node->GetNodeAttributeCount();
	SceneNode NewNode;
	
	auto Position = Node->LclTranslation.	Get();
	auto LclScale = Node->LclScaling.		Get();
	auto rotation = Node->LclRotation.		Get();
	auto NodeName = Node->GetName();

	NewNode.parent		= Parent;
	NewNode.position	= TranslateToFloat3(Position); 
	NewNode.scale		= TranslateToFloat3(LclScale);
	NewNode.Q			= Quaternion(rotation.mData[0], rotation.mData[1], rotation.mData[2]);

	size_t Nodehndl = scene->AddSceneNode(NewNode);

	for (size_t i= 0; i < AttributeCount; ++i)
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
			auto FBXMesh	= static_cast<fbxsdk::FbxMesh*>(Attr);
			auto UniqueID	= FBXMesh->GetUniqueID();
			auto name		= Node->GetName();

			SceneEntity entity;
			entity.MeshGuid = UniqueID;
			entity.Node		= Nodehndl;
			entity.id		= name;
			entity.albedo	= { 0.8f, 0.8f, 0.8f, 0.1f };
			entity.specular = { 0.8f, 0.8f, 0.8f, 0.0f };

			scene->AddSceneEntity(entity);
		}	break;
		case FbxNodeAttribute::eLight:
		{
#if USING(RESCOMPILERVERBOSE)
			std::cout << "Light Found: " << Node->GetName() << "\n";
#endif
			auto FBXLight    = static_cast<fbxsdk::FbxLight*>(Attr);
			auto Type        = FBXLight->LightType.Get();
			auto Cast        = FBXLight->CastLight.Get();
			auto I           = FBXLight->Intensity.Get()/10;
			auto K           = FBXLight->Color.Get();
			auto R           = FBXLight->OuterAngle.Get();
			auto radius		 = FBXLight->FarAttenuationEnd.Get();

			ScenePointLight light;
			light.K        = TranslateToFloat3(K);			// COLOR for the Confused
			light.I        = I;
			light.Node	   = Nodehndl;
			light.R        = I;

			scene->AddPointLight(light);
		}	break;
		case FbxNodeAttribute::eMarker:
		case FbxNodeAttribute::eUnknown:
		default:
			break;
		}
	}

	if (!SkipChildren)
	{
		size_t ChildCount = Node->GetChildCount();
		for (size_t I = 0; I < ChildCount; ++I)
			ProcessNodes(Node->GetChild(I), scene, MD, Nodehndl);
	}
}


/************************************************************************************************/


ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, FBXIDTranslationTable& Table, bool LoadSkeletalData = false, MetaDataList& MD = MetaDataList{}, bool subDivEnabled = false)
{
	GeometryList geometryFound;
	ResourceList resources;

	CompileAllGeometry(geometryFound, S->GetRootNode(), Table, MD, subDivEnabled);

#if USING(RESCOMPILERVERBOSE)
	std::cout << "CompileAllGeometry Compiled " << geometryFound.size() << " Resources\n";
#endif
	for (auto geometry : geometryFound)
	{
		resources.push_back(geometry);

		if (geometry->Skeleton)
		{
			resources.push_back(geometry->Skeleton);

			for (auto& animation : geometry->Skeleton->animations)
				resources.push_back(geometry->Skeleton);
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
	auto nodeName			= Node->GetName();
	auto RelatedMetaData	= FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName());
	auto NodeCount			= Node->GetChildCount();

	if (RelatedMetaData.size())
	{
		for (auto& i : RelatedMetaData)
		{
			if (i->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
			{
				auto				MD		= std::static_pointer_cast<Scene_MetaData>(i);
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


ResourceList CompileSceneFromFBXFile(fbxsdk::FbxScene* scene, const CompileSceneFromFBXFile_DESC& Desc, MetaDataList& METAINFO)
{
	FBXIDTranslationTable	translationTable;
	ResourceList			resources = GatherSceneResources(scene, Desc.Cooker, translationTable, true, METAINFO);;

	GetScenes(scene, METAINFO, translationTable, resources);

	return resources;
}


/************************************************************************************************/

ResourceBlob SceneResource::CreateBlob()
{ 
	std::vector<char> buffer;
	size_t currentBufferOffset = 0;
	SceneResourceBlob	header;
	SceneNodeBlock*		nodeblock			= nullptr;
	size_t				nodeBlockSize		= 0;
	EntityBlock*		entityBlock			= nullptr;
	size_t				entityBlockSize		= 0;

	{
		// Create Scene Node Table
			
		nodeBlockSize	= SceneNodeBlock::GetHeaderSize() + nodes.size() * sizeof(SceneNodeBlock::SceneNode);
		nodeblock		= reinterpret_cast<SceneNodeBlock*>(malloc(nodeBlockSize));

		nodeblock->blockSize = nodeBlockSize;
		nodeblock->blockType = SceneBlockType::NodeTable;
		nodeblock->nodeCount = nodes.size();

		for (size_t itr = 0; itr < nodes.size(); ++itr)
		{
			SceneNodeBlock::SceneNode n;

			auto node = nodes[itr];
			n.orientation	= node.Q;
			n.position		= node.position;
			n.scale			= node.scale;
			n.parent		= node.parent;
			nodeblock->nodes[itr] = n;
		}
	}

	{
		entityBlockSize = sizeof(EntityBlock) * entities.size();
		entityBlock		= (EntityBlock*)malloc(entityBlockSize);

		for (size_t itr = 0; itr < entities.size(); ++itr)
		{
			auto& entity = entities[itr];
			memset(entityBlock + itr, 0, sizeof(EntityBlock));
			// TODO: component blocks
			entityBlock[itr].nodeIdx		= entity.Node;
			entityBlock[itr].blockType		= SceneBlockType::Entity;
			entityBlock[itr].componentCount	= 0;
			entityBlock[itr].blockSize		= sizeof(EntityBlock);

			entityBlock[itr].MeshHandle		= TranslateID(entity.MeshGuid, translationTable);	// TODO: move into a component block!
			strncpy(entityBlock[itr].ID, entity.id.c_str(), min(64, entity.id.size()));			// TODO: move into a component block!
		}
	}


	// Create Scene Resource Header
	{
		header.blockCount = 2 + entities.size();
		header.GUID = GUID;
		strncpy(header.ID, ID.c_str(), 64);
		header.Type = EResourceType::EResource_Scene;
		header.ResourceSize = sizeof(header) + nodeBlockSize + entityBlockSize;
	}

	buffer.resize(sizeof(header) + nodeBlockSize + entityBlockSize);
	memcpy(&buffer[currentBufferOffset], &header, sizeof(header));
	currentBufferOffset += sizeof(header);
	memcpy(&buffer[currentBufferOffset], nodeblock, nodeBlockSize);
	currentBufferOffset += nodeBlockSize;
	memcpy(&buffer[currentBufferOffset], entityBlock, entityBlockSize);
	currentBufferOffset += nodeBlockSize;

	char* finalBuffer = (char*)malloc(buffer.size());
	memcpy(finalBuffer, &buffer[0], buffer.size());

	ResourceBlob out;
	out.buffer			= finalBuffer;
	out.bufferSize		= buffer.size();
	out.GUID			= GUID;
	out.ID				= ID;
	out.resourceType	= EResourceType::EResource_Scene;

	free(nodeblock);

	return out;
}


/************************************************************************************************/