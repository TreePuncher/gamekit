
/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#include "stdafx.h"
#include "Common.h"
#include "ResourceUtilities.h"

#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\AnimationUtilities.h"

#include "Scenes.h"
#include "MeshProcessing.h"

#include <inttypes.h>
#include <Windows.h>
#include <shobjidl.h> 

#include "PxPhysicsAPI.h"

using namespace FlexKit;



/************************************************************************************************/


struct FBXIDTranslation
{
	size_t	FBXID;
	GUID_t	Guid;
};
typedef Vector<FBXIDTranslation> FBXIDTranslationTable;


GUID_t	TranslateID(size_t FBXID, FBXIDTranslationTable& Table)
{
	for (auto ID : Table)
		if (ID.FBXID == FBXID)
			return ID.Guid;

	return INVALIDHANDLE;
}

bool FBXIDPresentInTable(size_t FBXID, FBXIDTranslationTable& Table)
{
	for (auto ID : Table)
		if (ID.FBXID == FBXID)
			return true;

	return false;
}

/************************************************************************************************/


Pair<bool, fbxsdk::FbxScene*>  
LoadFBXScene(char* file, fbxsdk::FbxManager* lSdkManager, fbxsdk::FbxIOSettings* settings)
{
	fbxsdk::FbxNode*	 node	  = nullptr;
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
	return{ true, scene };
}


/************************************************************************************************/


struct Engine
{
	iAllocator*		MemoryOut;
	iAllocator*		TempMem;
	iAllocator*		LevelMem;
	RenderSystem*	RS;
};

struct Compiler
{
};


typedef Vector<TriMeshResource> GeometryList;


/************************************************************************************************/


TriMeshResource* FindGeoByID(GeometryList* GB, size_t ID)
{
	for (auto& I : *GB)
	{
		if (I.TriMeshID == ID)
			return &I;
	}

	return nullptr;
}


struct CompiledMeshInfo
{
	FBXMeshDesc	MeshInfo;
	Skeleton*	S;
};



/************************************************************************************************/


typedef Pair<GeometryList*, iAllocator*> GBAPair;
Pair<size_t, GBAPair> 
CompileAllGeometry(fbxsdk::FbxNode* node, iAllocator* Memory, GeometryList* GL, iAllocator* TempMem, FBXIDTranslationTable* Table, MD_Vector* MD = nullptr, bool SubDiv = false)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	if (!GL) GL = &Memory->allocate<GeometryList>(Memory);

	auto AttributeCount = node->GetNodeAttributeCount();
	for (int itr = 0; itr < AttributeCount; ++itr){
		auto Attr = node->GetNodeAttributeByIndex(itr);
		switch (Attr->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
		{
			const char* MeshName = node->GetName();
			auto test     = Attr->GetUniqueID();
			auto Mesh	  = (fbxsdk::FbxMesh*)Attr;
			bool found	  = false;
			bool LoadMesh = false;
			size_t uniqueID	  = (size_t)Mesh->GetUniqueID();
			auto Geo	  = FindGeoByID( GL, uniqueID);

			Vector<size_t> RelatedMetaData;

#if USING(RESCOMPILERVERBOSE)
			std::cout << "Found Mesh: " << MeshName << "\n";
#endif
			if (MD)
			{
				RelatedMetaData = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName, TempMem );
			}
			else
				LoadMesh = true;

			auto MeshInfo = FlexKit::GetMeshMetaData(MD, RelatedMetaData);

			auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();
			size_t NameLen	= strlen( Name );

			if (!FBXIDPresentInTable(Mesh->GetUniqueID(), *Table))
			{
				if (!NameLen) 
				{
					Name = node->GetName();
					NameLen	= strlen( Name );
				}

				TriMeshResource	out;
				memset(&out, 0, sizeof(out));
				char* ID = nullptr;

				if ( NameLen++ ) 
				{
					ID = (char*)Memory->malloc( NameLen );
					strcpy_s( (char*)ID, NameLen, Name );
				}

				auto Res = CompileMeshResource(out, TempMem, Memory, Mesh, false, ID, MD);
				
				if(MeshInfo){
					out.TriMeshID	= MeshInfo->guid;
					out.ID			= MeshInfo->MeshID;
				}
				else
				{
					out.TriMeshID = CreateRandomID();
					out.ID = Mesh->GetName();
				}

					Table->push_back({ Mesh->GetUniqueID(), out.TriMeshID });

	#if USING(RESCOMPILERVERBOSE)
				std::cout << "Compiled Resource: " << Name << "\n";
	#endif

				GL->push_back(out);
			}
		}	break;
		}
	}

	size_t NodeCount = node->GetChildCount();
	for(int itr = 0; itr < NodeCount; ++itr)
		CompileAllGeometry(node->GetChild(itr), Memory, GL, TempMem, Table, MD);

	return{ GL->size(), {GL, TempMem} };
}


/************************************************************************************************/


template<typename TY, size_t ALIGNMENT = 0x40>
TY& _AlignedAllocate()
{
	TY& V = new(_aligned_malloc(sizeof(TY, ALIGNMENT))) TY();
	return V;
}


/************************************************************************************************/


size_t CalculateTriResourceMeshSize(TriMeshResource* TriMesh)
{
	size_t Size = 0;
	for (auto B : TriMesh->Buffers)
		Size += B ? B->GetBufferSizeRaw() : 0;

	return Size;
}


/************************************************************************************************/


void FillTriMeshBlob(TriMeshResourceBlob* out, TriMeshResource* Mesh)
{
	size_t BufferPosition = 0;
	out->GUID			= Mesh->TriMeshID;
	out->HasAnimation	= Mesh->AnimationData > 0;
	out->HasIndexBuffer = true;
	out->BufferCount	= 0;
	out->SkeletonGuid	= Mesh->SkeletonGUID;
	out->Type			= EResourceType::EResource_TriMesh;

	out->IndexCount = Mesh->IndexCount;
	out->Info.minx  = Mesh->Info.min.x;
	out->Info.miny  = Mesh->Info.min.y;
	out->Info.minz  = Mesh->Info.min.z;
	out->Info.maxx  = Mesh->Info.max.x;
	out->Info.maxy  = Mesh->Info.max.y;
	out->Info.maxz  = Mesh->Info.max.z;
	out->Info.r	    = Mesh->Info.r;
	
	memcpy(out->BS, &Mesh->BS, sizeof(BoundingSphere));
	out->AABB[0] = Mesh->AABB.BottomLeft[0];
	out->AABB[1] = Mesh->AABB.BottomLeft[1];
	out->AABB[2] = Mesh->AABB.BottomLeft[2];
	out->AABB[3] = Mesh->AABB.TopRight[0];
	out->AABB[4] = Mesh->AABB.TopRight[1];
	out->AABB[5] = Mesh->AABB.TopRight[2];

	if(Mesh->ID)
		strcpy_s(out->ID, ID_LENGTH, Mesh->ID);

	for (size_t I = 0; I < 16; ++I)
	{
		if (Mesh->Buffers[I])
		{
			memcpy(out->Memory + BufferPosition, Mesh->Buffers[I]->GetBuffer(), Mesh->Buffers[I]->GetBufferSizeRaw());

			auto View = Mesh->Buffers[I];
			out->Buffers[I].Begin  = BufferPosition;
			out->Buffers[I].Format = (size_t)Mesh->Buffers[I]->GetElementSize();
			out->Buffers[I].Type   = (size_t)Mesh->Buffers[I]->GetBufferType();
			out->Buffers[I].size   = (size_t)Mesh->Buffers[I]->GetBufferSizeRaw();
			BufferPosition		  += Mesh->Buffers[I]->GetBufferSizeRaw();

			out->BufferCount++;
		}
	}
}


/************************************************************************************************/


Resource* CreateTriMeshResourceBlob(TriMeshResource* Mesh, iAllocator* MemoryOut)
{
	size_t BlobSize          = CalculateTriResourceMeshSize(Mesh);
	BlobSize				+= sizeof(TriMeshResourceBlob);
	char* Blob               = (char*)MemoryOut->_aligned_malloc(BlobSize);
	TriMeshResourceBlob* Res = (TriMeshResourceBlob*)Blob;


	memset(Blob, 0, BlobSize);
	Res->ResourceSize = BlobSize;

	FillTriMeshBlob(Res, Mesh);

	return (Resource*)Res;
}


/************************************************************************************************/


size_t CalculateAnimationSize(Skeleton* S)
{
	size_t Size = 0;
	auto I = S->Animations;

	while (I)
	{
		for (size_t II = 0; II < I->Clip.FrameCount; ++II)
		{
			Size += I->Clip.Frames[II].JointCount * sizeof(JointPose);
			Size += I->Clip.Frames[II].JointCount * sizeof(JointHandle);
		}
		I = I->Next;
	}
	return Size;
}


/************************************************************************************************/


size_t CalculateSkeletonSize(Skeleton* S)
{
	size_t Size = 0;
	Size += S->JointCount * sizeof(SkeletonResourceBlob::JointEntry);

	for (size_t I = 0; I < S->JointCount; ++I)
		Size += strlen(S->Joints[I].mID);

	return Size;
}

Resource* CreateSkeletonResourceBlob(Skeleton* S, iAllocator* MemoryOut)
{
	size_t Size = CalculateSkeletonSize(S);
	Size += sizeof(SkeletonResourceBlob);

	SkeletonResourceBlob* R = (SkeletonResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->GUID					= S->guid;
	R->ResourceSize			= Size;
	R->Type					= EResource_Skeleton;
	R->JointCount			= S->JointCount;

	strcpy_s(R->ID, 64, "SKELETON");

	for (size_t I= 0; I < S->JointCount; ++I)
	{
		strcpy_s(R->Joints[I].ID, S->Joints[I].mID);
		R->Joints[I].Parent = S->Joints[I].mParent;
		memcpy(&R->Joints[I].IPose, &S->IPose[I], sizeof(float4x4));
		memcpy(&R->Joints[I].Pose, &S->JointPoses[I], sizeof(JointPose));
	}

	return (Resource*)R;
}


/************************************************************************************************/


size_t CalculateAnimationSize(AnimationClip* AC)
{
	size_t Size = 0;
	Size += AC->FrameCount * sizeof(AnimationResourceBlob::FrameEntry);
	Size += strlen(AC->mID);

	for (size_t I = 0; I < AC->FrameCount; ++I)
	{
		Size += AC->Frames[I].JointCount * sizeof(JointHandle);
		Size += AC->Frames[I].JointCount * sizeof(JointPose);
	}

	return Size;
}

Resource* CreateSkeletalAnimationResourceBlob(AnimationClip* AC, GUID_t Skeleton, iAllocator* MemoryOut)
{
	size_t Size = CalculateAnimationSize(AC);
	Size += sizeof(AnimationResourceBlob);

	AnimationResourceBlob* R = (AnimationResourceBlob*)MemoryOut->_aligned_malloc(Size);
	R->Skeleton		= Skeleton;
	R->FrameCount	= AC->FrameCount;
	R->FPS			= AC->FPS;
	R->GUID			= AC->guid;
	R->IsLooping	= AC->isLooping;
	R->Type			= EResourceType::EResource_SkeletalAnimation;
	R->ResourceSize = Size;
	strcpy_s(R->ID, AC->mID);

	AnimationResourceBlob::FrameEntry*	Frames = (AnimationResourceBlob::FrameEntry*)R->Buffer;
	size_t Position = sizeof(AnimationResourceBlob::FrameEntry) * R->FrameCount;

	for (size_t I = 0; I < R->FrameCount; ++I)
	{
		Frames[I].JointCount  = AC->Frames[I].JointCount;
		
		JointHandle* Joints   = (JointHandle*)(R->Buffer + Position);
		Frames[I].JointStarts = Position;
		Position += sizeof(JointHandle) * AC->Frames[I].JointCount;
		memcpy(Joints, AC->Frames[I].Joints, sizeof(JointHandle) * AC->Frames[I].JointCount);
	
		JointPose* Poses      = (JointPose*)(Joints + AC->Frames[I].JointCount);
		Frames[I].PoseStarts  = Position;
		Position += sizeof(JointPose) * AC->Frames[I].JointCount;
		memcpy(Poses, AC->Frames[I].Poses, sizeof(JointPose) * AC->Frames[I].JointCount);

		int x = 0;// Debug Point
	}

	return (Resource*)R;
}


/************************************************************************************************/


Resource* CreateColliderResourceBlob(char* Buffer, size_t BufferSize, GUID_t ColliderID, const char* AssetID, iAllocator* MemoryOut)
{
	size_t ResourceSize = BufferSize + sizeof(ColliderResourceBlob);
	ColliderResourceBlob* R = (ColliderResourceBlob*)MemoryOut->_aligned_malloc(ResourceSize);
	
	memcpy(R->Buffer, Buffer, BufferSize);
	R->GUID			= ColliderID;
	R->ResourceSize = ResourceSize;
	R->RefCount		= 0;
	R->State		= Resource::EResourceState_UNLOADED;
	R->Type			= EResourceType::EResource_Collider;
	strncpy(R->ID, AssetID, 63);

	return (Resource*)R;
}


/************************************************************************************************/


class ColliderStream : public physx::PxOutputStream
{
public :
	ColliderStream(iAllocator* Allocator, size_t InitialSize) : Memory(Allocator), size(InitialSize)
	{
		Buffer = (char*)Memory->malloc(InitialSize);
	}

	~ColliderStream()
	{
		Memory->free(Buffer);
	}

	physx::PxU32 write(const void* src, physx::PxU32 count)
	{
		if (used + count > size) {
			char* NewBuffer = (char*)Memory->malloc(size * 2);
			FK_ASSERT(NewBuffer, "Ran Out Of Memory!");
			memcpy(NewBuffer, Buffer, used);
			Memory->free(Buffer);
			Buffer = NewBuffer;
			size = 2 * size;
		}

		memcpy(Buffer + used, src, count);
		used += count;

		return count;
	}

	size_t		size;
	size_t		used;
	char*		Buffer;
	iAllocator*	Memory;
};


/************************************************************************************************/


ResourceList GatherSceneResources(fbxsdk::FbxScene* S, physx::PxCooking* Cooker, iAllocator* MemoryOut, FBXIDTranslationTable* Table, bool LoadSkeletalData = false, MD_Vector* MD = nullptr, bool SUBDIV = false)
{
	ResourceList ResourcesFound;

	auto& [resourceCount, resourceList] = CompileAllGeometry(S->GetRootNode(), MemoryOut, nullptr, MemoryOut, Table, MD);

#if USING(RESCOMPILERVERBOSE)
	std::cout << "CompileAllGeometry Compiled " << (size_t)resourceCount << " Resources\n";
#endif
	if ((size_t)resourceCount > 0)
	{
		auto G = (GeometryList*)resourceList;
		for (size_t I = 0; I < G->size(); ++I)
		{
			ResourcesFound.push_back(CreateTriMeshResourceBlob(&G->at(I), MemoryOut));

			if (G->at(I).Skeleton) {
				auto Res = CreateSkeletonResourceBlob(G->at(I).Skeleton, MemoryOut);
				ResourcesFound.push_back(Res);

				auto CurrentClip	= G->at(I).Skeleton->Animations;
				auto SkeletonID		= Res->GUID;
					
				while (true)
				{
					if (CurrentClip)
					{
						auto R = CreateSkeletalAnimationResourceBlob(&CurrentClip->Clip, SkeletonID, MemoryOut);
						ResourcesFound.push_back(R);
					} else
						break;
					CurrentClip = CurrentClip->Next;
				}
			}

			auto& Mesh		= G->at(I);
			auto RelatedMD	= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, Mesh.ID, MemoryOut);

			for(size_t J = 0; J < RelatedMD.size(); ++J)
			{
				switch (MD->at(RelatedMD[J])->type)
				{
				case MetaData::EMETAINFOTYPE::EMI_COLLIDER:
				{
					if (!Cooker)
						continue;

					Collider_MetaData* ColliderInfo = (Collider_MetaData*)MD->at(RelatedMD [J]);
					ColliderStream Stream = ColliderStream(MemoryOut, 2048);

					using physx::PxTriangleMeshDesc;
					PxTriangleMeshDesc meshDesc;
					meshDesc.points.count     = Mesh.Buffers[0]->GetBufferSizeUsed();
					meshDesc.points.stride    = Mesh.Buffers[0]->GetElementSize();
					meshDesc.points.data      = Mesh.Buffers[0]->GetBuffer();

					uint32_t* Indexes = (uint32_t*)MemoryOut->_aligned_malloc(Mesh.Buffers[15]->GetBufferSizeRaw());

					{
						struct Tri {
							uint32_t Indexes[3];
						};

						auto Proxy		= Mesh.Buffers[15]->CreateTypedProxy<Tri>();
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

					auto IndexBuffer		  = Mesh.IndexBuffer_Idx;
					meshDesc.triangles.count  = Mesh.IndexCount / 3;
					meshDesc.triangles.stride = Mesh.Buffers[15]->GetElementSize() * 3;
					meshDesc.triangles.data   = Indexes;

#if USING(RESCOMPILERVERBOSE)
					printf("BEGINNING MODEL BAKING!\n");
					std::cout << "Baking: " << Mesh.ID << "\n";
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
						auto Blob = CreateColliderResourceBlob(Stream.Buffer, Stream.used, ColliderInfo->Guid, ColliderInfo->ColliderID, MemoryOut);
						ResourcesFound.push_back(Blob);
					}
				}	break;
				default:
					break;
				} 
			}
		}
	}

#if USING(RESCOMPILERVERBOSE)
	std::cout << "Created "<< ResourcesFound.size() << " Resource Blobs\n";
#endif

	return ResourcesFound;
}


/************************************************************************************************/


struct LoadGeometry_RES
{
	fbxsdk::FbxManager*		Manager;
	fbxsdk::FbxIOSettings*	Settings;
	fbxsdk::FbxScene*		LoadedFbxScene;

	ResourceList Resources;
};


/************************************************************************************************/


int	AddSceneNode(CompiledScene::SceneNode* Node, CompiledScene* Scene)
{
	auto index = Scene->Nodes.size();
	return index;
}


/************************************************************************************************/


void AddPointLight(CompiledScene::PointLight Light, CompiledScene* Scene)
{
	Scene->SceneLights.push_back(Light);
}


/************************************************************************************************/


void AddEntity(CompiledScene::Entity Entity, CompiledScene* Scene)
{
	Scene->SceneEntities.push_back(Entity);
}


/************************************************************************************************/


void AddStaticEntity(CompiledScene::Entity Static, CompiledScene* Scene)
{
	Scene->SceneStatics.push_back(Static);
}


/************************************************************************************************/


void InitiateCompiledScene(CompiledScene* Scene, iAllocator* Memory)
{
	Scene->Nodes.Allocator			= Memory;
	Scene->SceneEntities.Allocator	= Memory;
	Scene->SceneGeometry.Allocator	= Memory;
	Scene->SceneLights.Allocator	= Memory;
}


/************************************************************************************************/


size_t CalculateSceneResourceSize(CompiledScene* SceneIn)
{
	size_t BlobSize = sizeof(SceneResourceBlob);

	BlobSize += SceneIn->Nodes.size()			* sizeof(CompiledScene::SceneNode);
	BlobSize += SceneIn->SceneEntities.size()	* sizeof(CompiledScene::Entity);
	BlobSize += SceneIn->SceneGeometry.size()	* sizeof(CompiledScene::SceneGeometry);
	BlobSize += SceneIn->SceneLights.size()		* sizeof(CompiledScene::PointLight);
	BlobSize += SceneIn->SceneStatics.size()	* sizeof(CompiledScene::Entity);

	for (auto E : SceneIn->SceneEntities)
		BlobSize += E.idlength + 1;

	return BlobSize;
}


/************************************************************************************************/


Resource* CreateSceneResourceBlob(iAllocator* Memory, CompiledScene* SceneIn, FBXIDTranslationTable* Table)
{
	size_t sceneTableOffset			= 0;
	size_t lightTableOffset			= SceneIn->SceneEntities.size()							* sizeof(CompiledScene::Entity);;
	size_t nodeTableOffset			= lightTableOffset		+ SceneIn->SceneLights.size()	* sizeof(CompiledScene::PointLight);
	size_t staticsTableOffset		= nodeTableOffset		+ SceneIn->Nodes.size()			* sizeof(CompiledScene::SceneNode);
	size_t stringTableOffset		= staticsTableOffset	+ SceneIn->SceneStatics.size()	* sizeof(CompiledScene::Entity);;

	size_t ResourceSize				= CalculateSceneResourceSize(SceneIn);
	SceneResourceBlob* SceneBlob	= (SceneResourceBlob*)Memory->malloc(ResourceSize);
	auto blob						= SceneBlob->Buffer;

	size_t strOffset	= stringTableOffset;
	size_t strCount		= 0;
	for (auto& entity : SceneIn->SceneEntities)
	{
		entity.MeshGuid = TranslateID(entity.MeshGuid, *Table);

		if (entity.idlength) {
			strCount++;
			memcpy(blob + strOffset, entity.id, entity.idlength - 1);// discarding the null terminator

			entity.id = reinterpret_cast<const char*>(strOffset);
			strOffset += entity.idlength - 1;
		}
	}

	memset(SceneBlob->ID, 0, 64);
	strncpy(SceneBlob->ID, SceneIn->ID, SceneIn->IDSize);

	SceneBlob->SceneTable.EntityCount	= SceneIn->SceneEntities.size();
	SceneBlob->SceneTable.NodeCount		= SceneIn->Nodes.size();
	SceneBlob->SceneTable.LightCount	= SceneIn->SceneLights.size();
	SceneBlob->ResourceSize				= ResourceSize;
	SceneBlob->GUID						= SceneIn->Guid;
	SceneBlob->RefCount					= 0;
	SceneBlob->State					= Resource::EResourceState_UNLOADED;
	SceneBlob->Type						= EResource_Scene;

	SceneBlob->SceneTable.SceneStringCount		= strCount;
	SceneBlob->SceneTable.SceneStringsOffset	= stringTableOffset;

	SceneBlob->SceneTable.EntityOffset			= sceneTableOffset;
	SceneBlob->SceneTable.LightOffset			= lightTableOffset;
	SceneBlob->SceneTable.NodeOffset			= nodeTableOffset;
	SceneBlob->SceneTable.StaticsOffset			= staticsTableOffset;

	memcpy(blob + sceneTableOffset, SceneIn->SceneEntities.begin(), SceneIn->SceneEntities.size() * sizeof(CompiledScene::Entity));
	memcpy(blob + lightTableOffset, SceneIn->SceneLights.begin(), SceneIn->SceneLights.size() * sizeof(CompiledScene::PointLight));
	memcpy(blob + nodeTableOffset, SceneIn->Nodes.begin(), SceneIn->Nodes.size() * sizeof(CompiledScene::SceneNode));
	memcpy(blob + staticsTableOffset, SceneIn->SceneStatics.begin(), SceneIn->SceneStatics.size() * sizeof(CompiledScene::Entity));

	return (Resource*)SceneBlob;
}


/************************************************************************************************/


void ProcessNodes(fbxsdk::FbxNode* Node, iAllocator* Memory, CompiledScene* SceneOut, MD_Vector* MD, size_t Parent = -1)
{
	bool SkipChildren = false;
	size_t AttributeCount = Node->GetNodeAttributeCount();
	CompiledScene::SceneNode NewNode;
	
	auto Position = Node->LclTranslation.	Get();
	auto LclScale = Node->LclScaling.		Get();
	auto rotation = Node->LclRotation.		Get();
	auto NodeName = Node->GetName();

	NewNode.Parent	= Parent;
	NewNode.TS		= float4(TranslateToFloat3(Position), LclScale.mData[0]);
	NewNode.Q		= Quaternion(rotation.mData[0], rotation.mData[1], rotation.mData[2]);

	size_t Nodehndl = AddSceneNode(&NewNode, SceneOut);
	SceneOut->Nodes.push_back(NewNode);

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
			auto FBXMesh  = static_cast<fbxsdk::FbxMesh*>(Attr);
			auto UniqueID = FBXMesh->GetUniqueID();
			auto name     = Node->GetName();
			auto name2    = FBXMesh->GetName();

			auto Related	= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, Node->GetName(), Memory);
			auto MeshData	= (Mesh_MetaData*)ScanForRelated(MD, Related, MetaData::EMETAINFOTYPE::EMI_MESH);

			CompiledScene::Entity Entity;
			Entity.MeshGuid = UniqueID;
			Entity.Node		= Nodehndl;
			Entity.id		= name;
			Entity.idlength = strnlen(Entity.id, 128);
			AddEntity(Entity, SceneOut);

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

			CompiledScene::PointLight Light;
			Light.K        = TranslateToFloat3(K);			// COLOR for the Confused
			Light.I        = I;
			Light.Node	   = Nodehndl;
			Light.R        = I * 100;

			AddPointLight(Light, SceneOut);
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
			ProcessNodes(Node->GetChild(I), Memory, SceneOut, MD, Nodehndl);
	}
}


/************************************************************************************************/


void ScanChildrenNodesForScene(fbxsdk::FbxNode* Node, MD_Vector* MetaData, iAllocator* Temp, iAllocator* Memory, SceneList* Out)
{
	auto nodeName = Node->GetName();
	auto RelatedMetaData	= FindRelatedMetaData(MetaData, MetaData::EMETA_RECIPIENT_TYPE::EMR_NODE, Node->GetName(), Temp);
	auto NodeCount			= Node->GetChildCount();

	if (RelatedMetaData.size())
	{
		for (auto& i : RelatedMetaData)
		{
			if (MetaData->at(i)->type == MetaData::EMETAINFOTYPE::EMI_SCENE)
			{
				Scene_MetaData* MD = (Scene_MetaData*)MetaData->at(i);
				CompiledScene& Scene	= Memory->allocate<CompiledScene>();
				InitiateCompiledScene(&Scene, Memory);
				Scene.Guid = MD->Guid;
				strncpy(Scene.ID, MD->SceneID, MD->SceneIDSize);
				Scene.IDSize = MD->SceneIDSize;

				ProcessNodes(Node, Temp, &Scene, MetaData);
				Out->push_back(&Scene);
			}
		}
		// Get Scene
	}
	else
	{
		Temp->clear();
		for (int itr = 0; itr < NodeCount; ++itr) {
			auto Child = Node->GetChild(itr);
			ScanChildrenNodesForScene(Child, MetaData, Temp, Memory, Out);
		}
	}
}


/************************************************************************************************/


void GetScenes(fbxsdk::FbxScene* S, iAllocator* MemoryOut, iAllocator* TempMemory, MD_Vector* MetaData, SceneList* Out)
{
	auto Root = S->GetRootNode();
	ScanChildrenNodesForScene(Root, MetaData, TempMemory, MemoryOut, Out);
}


/************************************************************************************************/


LoadGeometryRES_ptr CompileSceneFromFBXFile(char* AssetLocation, CompileSceneFromFBXFile_DESC* Desc, MD_Vector* METAINFO)
{
	size_t MaxMeshCount							= 100;
	fbxsdk::FbxManager*				Manager     = fbxsdk::FbxManager::Create();
	fbxsdk::FbxIOSettings*			Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
	fbxsdk::FbxScene*				Scene       = nullptr;

	Manager->SetIOSettings(Settings);

	auto res = LoadFBXScene( AssetLocation, Manager, Settings );
	if (res)
	{
		SceneList Scenes(Desc->BlockMemory);
		FBXIDTranslationTable Table(Desc->BlockMemory);
		GetScenes(res, Desc->BlockMemory, Desc->BlockMemory, METAINFO, &Scenes);
		ResourceList LoadRes = GatherSceneResources((FbxScene*)res, Desc->Cooker, Desc->BlockMemory, &Table, true, METAINFO);

		for (auto Scene : Scenes){
			auto res = CreateSceneResourceBlob(Desc->BlockMemory, Scene, &Table);
			LoadRes.push_back(res);
		}

		size_t ResourceCount = 0;
		size_t FileSize		 = 0;

		for (auto& G : LoadRes){
			ResourceCount++;
			FileSize += G->ResourceSize;
		}

		auto G = &Desc->BlockMemory->allocate_aligned<LoadGeometry_RES>();
		G->Manager			= Manager;
		G->LoadedFbxScene	= Scene;
		G->Resources		= LoadRes;
		G->Settings			= Settings;

		return  LoadGeometryRES_ptr(G);
	}
	else
	{
		std::cout << "Failed to Open FBX File: " << AssetLocation << "\n";
		MessageBox(0, L"Failed to Load File!", L"ERROR!", MB_OK);
	}
	return LoadGeometryRES_ptr(nullptr);
}


/************************************************************************************************/


struct SceneGeometry
{
	TriMesh*	Meshes;
	size_t		MeshCount;
};

void CountNodeContents(fbxsdk::FbxNode* N, Scene_Desc& Desc)
{
	size_t AttribCount = N->GetNodeAttributeCount();
	for (size_t I = 0; I < AttribCount; ++I)
	{
		auto A = N->GetNodeAttributeByIndex(I);
		switch (A->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eLight:
			Desc.MaxPointLightCount++; 	break;
		case fbxsdk::FbxNodeAttribute::EType::eSkeleton:
			Desc.MaxSkeletonCount++;	break;
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
			Desc.MaxTriMeshCount++;		break;
		}
	}

	size_t NodeCount = N->GetChildCount();
	for (size_t I = 0; I < NodeCount; ++I)
		CountNodeContents(N->GetChild(I), Desc);
}


/************************************************************************************************/


Scene_Desc CountSceneContents(fbxsdk::FbxScene* S)
{
	Scene_Desc	Desc = {};
	size_t NodeCount = S->GetNodeCount();
	for (size_t I = 0; I < NodeCount; ++I)
		CountNodeContents(S->GetNode(I), Desc);

	return Desc;
}


/************************************************************************************************/


FileDir SelectFile()
{
	IFileDialog* pFD = nullptr;
	FK_ASSERT( SUCCEEDED( CoInitialize( nullptr ) ) );
	auto HRES = CoCreateInstance( CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFD) );

	if ( SUCCEEDED( HRES ) )
	{
		DWORD Flags;
		HRES = pFD->GetOptions( &Flags );

		if ( SUCCEEDED( HRES ) )
		{
			pFD->SetOptions( FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM );
		} else return FileDir();
		
		FINALLY{
			if(pFD)
				pFD->Release();
		}FINALLYOVER;

		IShellItem* pItem = nullptr;

		WCHAR CurrentDirectory[ 256 ];
		GetCurrentDirectoryW( 256, CurrentDirectory );
			
		HRES = pFD->GetFolder(&pItem);
		if ( SUCCEEDED( HRES ) )
		{
			HRES = SHCreateItemFromParsingName( CurrentDirectory, nullptr, IID_PPV_ARGS( &pItem ) );
			pFD->SetDefaultFolder(pItem);
		}

		FINALLY{
			if(pItem)
				pItem->Release();
		}FINALLYOVER;

		HRES = pFD->Show( nullptr );
		if ( SUCCEEDED( HRES ) )
		{
			IShellItem* pItem = nullptr;
			HRES = pFD->GetResult( &pItem );
			if(SUCCEEDED(HRES))
			{
				FINALLY{
				if(pItem)
					pItem->Release();
				}FINALLYOVER;

				PWSTR FilePath;
				HRES = pItem->GetDisplayName( SIGDN_FILESYSPATH, &FilePath );
				if ( SUCCEEDED( HRES ) )
				{
					size_t length = wcslen( FilePath );
					FileDir	Dir;
					BOOL DefaultCharUsed = 0;
					CCHAR	DChar = ' ';
					auto res = WideCharToMultiByte( CP_UTF8, 0, FilePath, length, Dir.str, 256, nullptr, nullptr);
					if ( !res )
					{
						IErrorInfo*	INFO = nullptr;
						auto Err = GetLastError();
					}
					Dir.str[length] = '\0';
					Dir.Valid = true;
					return Dir;
				}
			}
		}
	}
	return FileDir();
}

