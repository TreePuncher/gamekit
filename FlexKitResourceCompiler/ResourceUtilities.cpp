
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
#include "..\graphicsutilities\graphics.h"

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


typedef Vector<TriMesh> GeometryList;


/************************************************************************************************/


TriMesh* FindGeoByID(GeometryList* GB, size_t ID)
{
	for (auto& I : *GB)
	{
		if (I.TriMeshID == ID)
			return &I;
	}

	return nullptr;
}


/************************************************************************************************/


struct CompileMeshInfo
{
	bool   Success;
	size_t BuffersFound;
};

CompileMeshInfo CompileMeshResource(TriMesh& out, iAllocator* TempMem, iAllocator* Memory, FbxMesh* Mesh, bool EnableSubDiv = false, const char* ID = nullptr, MD_Vector* MD = nullptr)
{
	using FlexKit::FillBufferView;
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	Skeleton*	S		= LoadSkeleton(Mesh, Memory, TempMem, ID, MD);
	TokenList& Tokens	= TokenList::Create_Aligned(2048000, TempMem);
	auto MeshInfo		= TranslateToTokens(Mesh, TempMem, Tokens, S);

	CombinedVertexBuffer& CVB = CombinedVertexBuffer::Create_Aligned(1024000, TempMem);
	IndexList& IB			  = IndexList::Create_Aligned(MeshInfo.FaceCount * 8, TempMem);

	auto BuildRes = MeshUtilityFunctions::BuildVertexBuffer(Tokens, CVB, IB, TempMem, TempMem, MeshInfo.Weights);
	FK_ASSERT(BuildRes.V1 == true, "Mesh Failed to Build");

	size_t IndexCount  = GetByType<MeshBuildInfo>(BuildRes).IndexCount;
	size_t VertexCount = GetByType<MeshBuildInfo>(BuildRes).VertexCount;

	static_vector<Pair<VERTEXBUFFER_TYPE, VERTEXBUFFER_FORMAT>> BuffersFound ={
		{VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32}
	};

	if ((MeshInfo.UV))
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32 });

	if ((MeshInfo.Normals))
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });

	if ((MeshInfo.Weights)) {
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32 });
		BuffersFound.push_back({ VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16G16B16A16 });
	}

	for (size_t i = 0; i < BuffersFound.size(); ++i) {
		CreateBufferView(VertexCount, Memory, out.Buffers[i], (VERTEXBUFFER_TYPE)BuffersFound[i], (VERTEXBUFFER_FORMAT)BuffersFound[i]);

		switch ((VERTEXBUFFER_TYPE)BuffersFound[i])
		{
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexPOS);		break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchVertexNormal);	break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteUV, FetchVertexUV);			break;
		case VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_TANGENT:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchFloat3ZERO);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION1:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], WriteVertex, FetchWeights);		break;
		case  VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_ANIMATION2:
			FillBufferView(&CVB, CVB.size(), out.Buffers[i], Writeuint4, FetchWeightIndices);	break;
		default:
			break;
		}
	}

	CreateBufferView(IB.size(), Memory, out.Buffers[15], VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32);
	FillBufferView(&IB, IB.size(), out.Buffers[15], WriteIndex, FetchIndex2);

	
	if (EnableSubDiv)
	{
	}

	TempMem->clear();

	out.IndexCount    = IndexCount;
	out.Skeleton      = S;
	out.AnimationData = MeshInfo.Weights ? TriMesh::EAD_Skin : 0;
	out.Info.max	  = MeshInfo.MaxV;
	out.Info.min	  = MeshInfo.MinV;
	out.Info.r		  = MeshInfo.R;
	out.TriMeshID	  = Mesh->GetUniqueID();
	out.ID			  = ID;
	out.SkeletonGUID  = S ? S->guid : -1;

	return {true, BuffersFound.size()};
}


/************************************************************************************************/


struct CompiledMeshInfo
{
	FBXMeshDesc	MeshInfo;
	Skeleton*	S;
};


Vector<size_t> FindRelatedMetaData(MD_Vector* MetaData, MetaData::EMETA_RECIPIENT_TYPE Type, const char* ID, iAllocator* TempMem)
{
	Vector<size_t> RelatedData(TempMem);
	size_t IDLength = strlen(ID);

	for (size_t I = 0; I < MetaData->size(); ++I)
	{
		auto& MD = (*MetaData)[I];
		if (MD->UserType == Type)
			if (!strncmp(MD->ID, ID, IDLength) )
				RelatedData.push_back(I);
	}

	return RelatedData;
}

/************************************************************************************************/


Mesh_MetaData* GetMeshMetaData(MD_Vector* MetaData, Vector<size_t>& related)
{
	for (auto I : related)
		if(MetaData->at(I)->type == MetaData::EMETAINFOTYPE::EMI_MESH)
			return (Mesh_MetaData*)MetaData->at(I);

	return nullptr;
}

/************************************************************************************************/


MetaData* ScanRelatedFor(MD_Vector* MetaData, RelatedMetaData& Related, MetaData::EMETAINFOTYPE Type)
{
	for (auto I : Related)
		if (MetaData->at(I)->type == Type)
			return (Mesh_MetaData*)MetaData->at(I);

	return nullptr;
}


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
			size_t ID	  = (size_t)Mesh->GetUniqueID();
			auto Geo	  = FindGeoByID( GL, ID );

			Vector<size_t> RelatedMetaData;

#if USING(RESCOMPILERVERBOSE)
			std::cout << "Found Mesh: " << MeshName << "\n";
#endif
			if (MD)
			{
				MoveVector(	
					RelatedMetaData, 
					FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName, TempMem ));
			}
			else
				LoadMesh = true;

			auto MeshInfo = GetMeshMetaData(MD, RelatedMetaData);
			LoadMesh = MeshInfo != nullptr;

			if ( !Geo && LoadMesh)
			{
				auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();
				size_t NameLen	= strlen( Name );

				if (!NameLen) {
					Name = node->GetName();
					NameLen	= strlen( Name );
				}

				TriMesh	out;
				TriMesh Test = out;
				memset(&out, 0, sizeof(out));
				char* ID = nullptr;
				if ( NameLen++ ) {
					ID = (char*)Memory->malloc( NameLen );
					strcpy_s( (char*)ID, NameLen, Name );
				}

				auto Res = CompileMeshResource(out, TempMem, Memory, Mesh, false, ID, MD);
				
				if(MeshInfo){
					auto Info = MeshInfo;
					out.TriMeshID	= Info->guid;
					out.ID			= Info->MeshID;
				}

				if (!FBXIDPresentInTable(Mesh->GetUniqueID(), *Table))
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


size_t CalculateTriMeshSize(TriMesh* TriMesh)
{
	size_t Size = 0;
	for (auto B : TriMesh->Buffers)
		Size += B ? B->GetBufferSizeRaw() : 0;

	return Size;
}


/************************************************************************************************/


void FillTriMeshBlob(TriMeshResourceBlob* out, TriMesh* Mesh)
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


Resource* CreateTriMeshResourceBlob(TriMesh* Mesh, iAllocator* MemoryOut)
{
	size_t BlobSize          = CalculateTriMeshSize(Mesh);
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
	size_t TempMemorySize = MEGABYTE * 256;
	StackAllocator TempMemory;
	TempMemory.Init((byte*)_aligned_malloc(TempMemorySize, 0x40), TempMemorySize);

	auto Res = CompileAllGeometry(S->GetRootNode(), MemoryOut, nullptr, TempMemory, Table, MD);

#if USING(RESCOMPILERVERBOSE)
	std::cout << "CompileAllGeometry Compiled " << (size_t)Res << " Resources\n";
#endif
	ResourceList ResourcesFound;
	if ((size_t)Res > 0)
	{
		auto G = (GeometryList*)(GBAPair)Res;
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
			auto RelatedMD	= FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE, Mesh.ID, TempMemory);
			for(size_t J = 0; J < RelatedMD.size(); ++J)
			{
				switch (MD->at(RelatedMD[J])->type)
				{
				case MetaData::EMETAINFOTYPE::EMI_COLLIDER:
				{
					if (!Cooker)
						continue;

					Collider_MetaData* ColliderInfo = (Collider_MetaData*)MD->at(RelatedMD[J]);
					ColliderStream Stream = ColliderStream(MemoryOut, 2048);

					using physx::PxTriangleMeshDesc;
					PxTriangleMeshDesc meshDesc;
					meshDesc.points.count     = Mesh.Buffers[0]->GetBufferSizeUsed();
					meshDesc.points.stride    = Mesh.Buffers[0]->GetElementSize();
					meshDesc.points.data      = Mesh.Buffers[0]->GetBuffer();

					uint32_t* Indexes = (uint32_t*)TempMemory._aligned_malloc(Mesh.Buffers[15]->GetBufferSizeRaw());

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

					auto IndexBuffer		  = Mesh.VertexBuffer.MD.IndexBuffer_Index;
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

	return BlobSize;
}


/************************************************************************************************/


Resource* CreateSceneResourceBlob(iAllocator* Memory, CompiledScene* SceneIn, FBXIDTranslationTable* Table)
{
	size_t ResourceSize = CalculateSceneResourceSize(SceneIn);
	SceneResourceBlob* SceneBlob		= (SceneResourceBlob*)Memory->malloc(ResourceSize);
	SceneBlob->SceneTable.EntityCount	= SceneIn->SceneEntities.size();
	SceneBlob->SceneTable.NodeCount		= SceneIn->Nodes.size();
	SceneBlob->SceneTable.LightCount	= SceneIn->SceneLights.size();
	SceneBlob->ResourceSize				= ResourceSize;
	SceneBlob->GUID						= SceneIn->Guid;
	SceneBlob->RefCount					= 0;
	SceneBlob->State					= Resource::EResourceState_UNLOADED;
	SceneBlob->Type						= EResource_Scene;
	
	for (auto& A : SceneIn->SceneEntities)
		A.MeshGuid = TranslateID(A.MeshGuid, *Table);

	memset(SceneBlob->ID, 0, 64);
	strncpy(SceneBlob->ID, SceneIn->ID, SceneIn->IDSize);
	
	size_t Offset = 0;
	auto Data = SceneBlob->Buffer;

	SceneBlob->SceneTable.EntityOffset = Offset;
	memcpy(Data + Offset, SceneIn->SceneEntities.begin(), SceneIn->SceneEntities.size() * sizeof(CompiledScene::Entity));
	Offset += SceneIn->SceneEntities.size() * sizeof(CompiledScene::Entity);

	SceneBlob->SceneTable.LightOffset = Offset;
	memcpy(Data + Offset, SceneIn->SceneLights.begin(), SceneIn->SceneLights.size() * sizeof(CompiledScene::PointLight));
	Offset += SceneIn->SceneLights.size() * sizeof(CompiledScene::PointLight);

	SceneBlob->SceneTable.NodeOffset = Offset;
	memcpy(Data + Offset, SceneIn->Nodes.begin(), SceneIn->Nodes.size() * sizeof(CompiledScene::SceneNode));
	Offset += SceneIn->Nodes.size() * sizeof(CompiledScene::SceneNode);

	SceneBlob->SceneTable.StaticsOffset = Offset;
	memcpy(Data + Offset, SceneIn->SceneStatics.begin(), SceneIn->SceneStatics.size() * sizeof(CompiledScene::Entity));
	Offset += SceneIn->SceneStatics.size() * sizeof(CompiledScene::Entity);

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
			auto MeshData	= (Mesh_MetaData*)ScanRelatedFor(MD, Related, MetaData::EMETAINFOTYPE::EMI_MESH);

			CompiledScene::Entity Entity;
			Entity.MeshGuid = UniqueID;
			Entity.Node = Nodehndl;

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
	physx::PxCooking*				Cooker		= nullptr;
	physx::PxFoundation*			Foundation	= nullptr;
	physx::PxDefaultErrorCallback	DefaultErrorCallback;
	physx::PxDefaultAllocator		DefaultAllocatorCallback;

	Manager->SetIOSettings(Settings);

	if (Desc->CookingEnabled)
	{
#if USING(RESCOMPILERVERBOSE)
		std::cout << "Physx Model Baking Enabled!\n";
#endif
		Foundation	= PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
		FK_ASSERT(Foundation);

		Cooker		= PxCreateCooking(PX_PHYSICS_VERSION ,*Foundation, physx::PxCookingParams(physx::PxTolerancesScale()));
		FK_ASSERT(Cooker);
	}

	FINALLY{
		Manager->Destroy();
		if (Desc->CookingEnabled)
		{
			if (Foundation)
				Foundation->release();

			if (Cooker)
				Cooker->release();
		}
	}FINALLYOVER;

	auto res = LoadFBXScene( AssetLocation, Manager, Settings );
	if (res)
	{
		SceneList Scenes;
		FBXIDTranslationTable Table(*Desc->BlockMemory);
		GetScenes(res, *Desc->BlockMemory, *Desc->BlockMemory, METAINFO, &Scenes);
		ResourceList LoadRes = GatherSceneResources((FbxScene*)res, Cooker, *Desc->BlockMemory, &Table, true, METAINFO);

		for (auto Scene : Scenes){
			auto res = CreateSceneResourceBlob(*Desc->BlockMemory, Scene, &Table);
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


struct MD_Token
{
	char*	SubStr;
	size_t	size;
};


typedef Vector<MD_Token> TokenList;


/************************************************************************************************/


TokenList* GetMetaDataTokens(char* Buffer, size_t BufferSize, iAllocator* Memory)
{
	Vector<MD_Token>* Tokens = &Memory->allocate_aligned<Vector<MD_Token>>(Memory);

	size_t StartPos = 0;
	size_t CurrentPos = 0;

	auto RemoveWhiteSpaces = [&]()
	{
		bool WhitespaceSkipped = false;
		while (CurrentPos < BufferSize && !WhitespaceSkipped)
		{
			char CurrentChar = Buffer[CurrentPos];
			switch (CurrentChar)
			{
			case 0x20:
			case 0x00:
			case '\t':
			case '\n':
				++CurrentPos;
				break;
			default:
				WhitespaceSkipped = true;
				break;
			}
		}
	};

	while (CurrentPos < BufferSize)
	{
		auto C = Buffer[CurrentPos];
		if (Buffer[CurrentPos] == ' ' || Buffer[CurrentPos] == '\n' || Buffer[CurrentPos] == '\t')
		{
			RemoveWhiteSpaces();

			char CurrentChar = Buffer[CurrentPos];

			MD_Token NewToken = { Buffer + StartPos, CurrentPos - StartPos - 1 }; // -1 to remove Whitespace at end

			if (NewToken.size)
				Tokens->push_back(NewToken);

			StartPos = CurrentPos;

			RemoveWhiteSpaces();
		}
		else if (Buffer[CurrentPos] == '\0')
			break;
		else
			++CurrentPos;
	}

	return Tokens;
}


/************************************************************************************************/


struct Value
{
	enum TYPE
	{
		INT,
		STRING,
		FLOAT,
	}Type;

	union
	{
		float	F;
		int		I;
		struct
		{
			char*	S;
			size_t	size;
		}S;
	}Data;

	char*	ID;
	size_t	ID_Size;
};

typedef static_vector<Value> ValueList;


/************************************************************************************************/


void MoveTokenStr(MD_Token T, char* out)
{
	memset(out, 0x00, T.size + 1);
	strncpy(out, T.SubStr, T.size);

	for (size_t I = T.size; I > 0; --I)
	{
		switch (out[I])
		{
		case '\n':
		case ' ':
			out[I] = '\0';
			T.size--;
		}
	}
}


/************************************************************************************************/


FlexKit::Pair<ValueList, size_t> ProcessDeclaration(iAllocator* Memory, iAllocator* TempMemory, TokenList* Tokens, size_t StartingPosition)
{
	ValueList Values;
	size_t itr2 = StartingPosition;

	for (; itr2 < Tokens->size(); ++itr2)
	{
		auto T = Tokens->at(itr2);

		if (T.size)
			if (!strncmp(T.SubStr, "int", 3))
			{
				Value NewValue;
				NewValue.Type = Value::INT;

				auto IDToken    = Tokens->at(itr2 - 2);
				auto ValueToken = Tokens->at(itr2 + 2);

				char ValueBuffer[16];
				MoveTokenStr(ValueToken, ValueBuffer);
				int V				= atoi(ValueBuffer);
				NewValue.Data.I		= V;
				NewValue.ID			= (char*)TempMemory->malloc(IDToken.size + 1); // 1 Extra for the Null Terminator
				NewValue.ID_Size	= ValueToken.size;
				MoveTokenStr(IDToken, NewValue.ID);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "string", 6))
			{
				Value NewValue;
				NewValue.Type = Value::STRING;

				auto IDToken		= Tokens->at(itr2 - 2);
				auto ValueToken		= Tokens->at(itr2 + 2);

				size_t IDSize		= IDToken.size;
				NewValue.ID			= (char*)TempMemory->malloc(IDSize + 1); // 
				NewValue.ID_Size	= IDSize;
				MoveTokenStr(IDToken, NewValue.ID);

				size_t StrSize       = ValueToken.size;
				NewValue.Data.S.S    = (char*)TempMemory->malloc(StrSize + 1); // 
				NewValue.Data.S.size = StrSize;
				MoveTokenStr(ValueToken, NewValue.Data.S.S);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "float", 5))
			{
				Value NewValue;
				NewValue.Type = Value::FLOAT;

				auto IDToken = Tokens->at(itr2 - 2);
				auto ValueToken = Tokens->at(itr2 + 2);

				char ValueBuffer[16];
				MoveTokenStr(ValueToken, ValueBuffer);
				float V         = atof(ValueBuffer);
				NewValue.Data.F = V;

				NewValue.ID = (char*)TempMemory->malloc(IDToken.size + 1); // 
				MoveTokenStr(IDToken, NewValue.ID);

				Values.push_back(NewValue);
			}
			else if (!strncmp(T.SubStr, "};", min(T.size, 2)))
				return{ Values, itr2 };
	}

	// Should Be Un-reachable
	return{ Values, itr2 };
}


/************************************************************************************************/


size_t SkipBrackets(TokenList* Tokens, size_t StartingPosition)
{
	size_t itr2 = StartingPosition;

	for (; itr2 < Tokens->size(); ++itr2)
	{
		auto T = Tokens->at(itr2);

		if (T.size && (!strncmp(T.SubStr, "};", min(T.size, 2))))
			return itr2;
	}

	// Malformed Document if code reaches here
	FK_ASSERT(0);
	return -1;
}


/************************************************************************************************/


Value* FindValue(static_vector<Value>& Values, char* ValueID)
{
	auto res = FlexKit::find(Values, [&](Value& V) {
		return (!strncmp(V.ID, ValueID, strlen(ValueID))); });

	return (res == Values.end()) ? nullptr : res;
}


/************************************************************************************************/


bool ProcessTokens(iAllocator* Memory, iAllocator* TempMemory, TokenList* Tokens, MD_Vector& MD_Out)
{
	struct Value
	{
		enum TYPE
		{
			INT,
			STRING,
			FLOAT,
		}Type;

		Token T;
	};

	// Metadata Format
	// {Identifier} : {type} = {Value(s)};

	ValueList Values;
	for (size_t itr = 0; itr < Tokens->size(); ++itr)
	{
		auto T = Tokens->at(itr);

#define DOSTRCMP(A) (T.size && !strncmp(T.SubStr, A, max(strlen(A), T.size)))
#define CHECKTAG(A, TYPE) ((A != nullptr) && (A->Type == TYPE))

		// TODO: Reform this into a table
		if		(DOSTRCMP("AnimationClip"))
		{
			auto res    = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values = res.V1;
			auto ID     = FindValue(Values, "ID");			
			auto begin  = FindValue(Values, "Begin");		
			auto end    = FindValue(Values, "End");			
			auto GUID   = FindValue(Values, "AssetGUID");	

			// Check for ill formed data
#if _DEBUG
			FK_ASSERT((ID != nullptr),		"MISSING ID		TAG!");
			FK_ASSERT((begin != nullptr),	"MISSING Begin	Value!");
			FK_ASSERT((end != nullptr),		"MISSING End	Value!");
			FK_ASSERT((GUID != nullptr),	"MISSING GUID!");
			FK_ASSERT((ID->Type    == Value::STRING));
			FK_ASSERT((begin->Type == Value::FLOAT));
			FK_ASSERT((end->Type   == Value::FLOAT));
			FK_ASSERT((GUID->Type  == Value::INT));
#else	
			if ((!ID	|| ID->Type		!= Value::STRING) ||
				(!begin || begin->Type	!= Value::FLOAT)  ||
				(!end	|| end->Type	!= Value::FLOAT)  ||
				(!GUID	|| GUID->Type	!= Value::INT))
				return false;
#endif

			AnimationClip_MetaData* NewAnimationClip = &Memory->allocate_aligned<AnimationClip_MetaData>();

			auto Target = Tokens->at(itr - 2);

			strncpy(NewAnimationClip->ClipID, ID->Data.S.S, ID->Data.S.size);

			NewAnimationClip->SetID(Target.SubStr, Target.size);
			NewAnimationClip->T_Start	= begin->Data.F;
			NewAnimationClip->T_End		= end->Data.F;
			NewAnimationClip->guid		= GUID->Data.I;

			MD_Out.push_back(NewAnimationClip);

			itr = res;
		}
		else if (DOSTRCMP("Skeleton"))
		{
			auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		= res.V1;
			auto AssetID	= FindValue(Values, "AssetID");		
			auto AssetGUID	= FindValue(Values, "AssetGUID");	

#if _DEBUG
			FK_ASSERT((AssetID			!= nullptr), "MISSING ID!");
			FK_ASSERT((AssetID->Type	== Value::STRING));

			FK_ASSERT((AssetGUID		!= nullptr), "MISSING GUID!");
			FK_ASSERT((AssetGUID->Type	== Value::INT));
#else
			if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
				return false;
#endif

			Skeleton_MetaData* Skeleton = &Memory->allocate_aligned<Skeleton_MetaData>();

			auto Target = Tokens->at(itr - 2);

			strncpy(Skeleton->SkeletonID, AssetID->Data.S.S, AssetID->Data.S.size);


			Skeleton->SetID(Target.SubStr, Target.size);
			Skeleton->SkeletonGUID	= AssetGUID->Data.I;

			MD_Out.push_back(Skeleton);

			itr = res;
		}
		else if (DOSTRCMP("Model"))
		{
			auto res			= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values			= res.V1;
			auto AssetID		= FindValue(Values, "AssetID");
			auto AssetGUID		= FindValue(Values, "AssetGUID");
			auto ColliderGUID	= FindValue(Values, "ColliderGUID");
			auto Target			= Tokens->at(itr - 2);

#if _DEBUG
			FK_ASSERT((AssetID != nullptr), "MISSING ID!");
			FK_ASSERT((AssetID->Type == Value::STRING));

			FK_ASSERT((AssetGUID != nullptr), "MISSING GUID!");
			FK_ASSERT((AssetGUID->Type == Value::INT));
#else
			if ((!AssetID || AssetID->Type != Value::STRING) || (!AssetGUID || AssetGUID->Type != Value::INT))
				return false;
#endif

			Mesh_MetaData* Model = &Memory->allocate_aligned<Mesh_MetaData>();

			if (ColliderGUID != nullptr && ColliderGUID->Type == Value::INT)
				Model->ColliderGUID = ColliderGUID->Data.I;
			else
				Model->ColliderGUID = INVALIDHANDLE;

			strncpy(Model->MeshID, AssetID->Data.S.S, AssetID->Data.S.size);

			for (size_t I = AssetID->Data.S.size; I > 0; --I)
			{
				switch (Model->MeshID[I])
				{
					case '\n':
					case ' ':
						Model->MeshID[I] = '\0';
						AssetID->Data.S.size--;
				}
			}

			Model->SetID(Target.SubStr, Target.size + 1);
			Model->guid = AssetGUID->Data.I;

			MD_Out.push_back(Model);

			itr = res;
		}
		else if (DOSTRCMP("TextureSet"))
		{
			auto res		  = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		  = res.V1;
			auto AssetID	  = Tokens->at(itr - 2);
			auto AssetGUID	  = FindValue(Values, "AssetGUID");
			auto Albedo		  = FindValue(Values, "Albedo");
			auto AlbedoID	  = FindValue(Values, "AlbedoGUID");
			auto RoughMetal	  = FindValue(Values, "RoughMetal");
			auto RoughMetalID = FindValue(Values, "RoughMetalGUID");

			TextureSet_MetaData* TextureSet_Meta = &Memory->allocate_aligned<TextureSet_MetaData>();

			if (AssetGUID && AssetGUID->Type == Value::INT) 
				TextureSet_Meta->Guid = AssetGUID->Data.I;

			if (Albedo && Albedo->Type == Value::STRING){
				auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ALBEDO].Directory;
				strncpy(dest, Albedo->Data.S.S, Albedo->Data.S.size);
			}

			if (AlbedoID && AlbedoID->Type == Value::INT) {
				TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = AlbedoID->Data.I;
			} else {
				TextureSet_Meta->Textures.TextureID[ETT_ALBEDO] = INVALIDHANDLE;
			}

			if (RoughMetal && RoughMetal->Type == Value::STRING){
				auto dest = TextureSet_Meta->Textures.TextureLocation[ETT_ROUGHSMOOTH].Directory;
				strncpy(dest, RoughMetal->Data.S.S, RoughMetal->Data.S.size);
			}

			if (RoughMetalID && RoughMetalID->Type == Value::INT) {
				TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = RoughMetalID->Data.I;
			} else {
				TextureSet_Meta->Textures.TextureID[ETT_ROUGHSMOOTH] = INVALIDHANDLE;
			}

			MD_Out.push_back(TextureSet_Meta);
			itr = res;
		}
		else if (DOSTRCMP("Scene"))
		{
			auto res        = ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values     = res.V1;
			auto Target		= Tokens->at(itr - 2);
			auto AssetGUID  = FindValue(Values, "AssetGUID");
			auto AssetID	= FindValue(Values, "AssetID");

			Scene_MetaData& Scene		= Memory->allocate<Scene_MetaData>();
			Scene.SetID(Target.SubStr, Target.size);

			if(AssetGUID != nullptr && AssetGUID->Type == Value::INT)
				Scene.Guid = AssetGUID->Data.I;

			if (AssetID != nullptr && AssetID->Type == Value::STRING) {
				strncpy(Scene.SceneID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
				Scene.SceneIDSize = AssetID->Data.S.size;
			}

			MD_Out.push_back(&Scene);
			itr = res;
		}
		else if (DOSTRCMP("Collider"))
		{
			auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		= res.V1;
			auto Target		= Tokens->at(itr - 2);
			auto AssetGUID	= FindValue(Values,	"AssetGUID");
			auto AssetID	= FindValue(Values,	"AssetID");

			Collider_MetaData& Collider = Memory->allocate<Collider_MetaData>();
			Collider.SetID(Target.SubStr, Target.size);

			if (AssetGUID != nullptr && AssetGUID->Type == Value::INT)
				Collider.Guid = AssetGUID->Data.I;

			if (AssetID != nullptr && AssetID->Type == Value::STRING) {
				strncpy(Collider.ColliderID, AssetID->Data.S.S, min(AssetID->Data.S.size, 64));
				Collider.ColliderIDSize = AssetID->Data.S.size;
			}

			MD_Out.push_back(&Collider);
			itr = res;
		}
		else if (DOSTRCMP("Font")) 
		{
			auto res		= ProcessDeclaration(Memory, TempMemory, Tokens, itr);
			auto Values		= res.V1;
			auto AssetID	= Tokens->at(itr - 2);
			auto AssetGUID	= FindValue(Values,	"AssetGUID");
			auto FontFile	= FindValue(Values, "File");

			// Check for ill formed data
#if _DEBUG
			FK_ASSERT(CHECKTAG(FontFile,	Value::STRING));
			FK_ASSERT(CHECKTAG(AssetGUID,	Value::INT));
#else	
			if	  (	CHECKTAG(FontFile, Value::STRING) &&
					CHECKTAG(AssetGUID, Value::INT))
				return false;
#endif

			Font_MetaData& FontData = Memory->allocate<Font_MetaData>();
			FontData.UserType       = MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE;
			FontData.type           = MetaData::EMETAINFOTYPE::EMI_FONT;
			FontData.SetID(AssetID.SubStr, AssetID.size);

			if (CHECKTAG(FontFile, Value::STRING))
				strncpy(FontData.FontFile, FontFile->Data.S.S, min(FontFile->Data.S.size, sizeof(FontData.FontFile)));

			if (CHECKTAG(AssetGUID, Value::INT))
				FontData.Guid = AssetGUID->Data.I;

			MD_Out.push_back(&FontData);
			itr = res;
		}
		else if (DOSTRCMP("{"))
			itr = SkipBrackets(Tokens, itr);

#undef CHECKTAG
#undef DOSTRCMP
	}

	return true;
}


/************************************************************************************************/


bool ReadMetaData(const char* Location, iAllocator* Memory, iAllocator* TempMemory, MD_Vector& MD_Out)
{
	size_t BufferSize = FlexKit::GetFileSize(Location);
	char* Buffer = (char*)TempMemory->malloc(BufferSize);
	LoadFileIntoBuffer(Location, (byte*)Buffer, BufferSize);

	auto Tokens = GetMetaDataTokens(Buffer, BufferSize, TempMemory);
	auto res = ProcessTokens(Memory, TempMemory, Tokens, MD_Out);

	return res;
}


/************************************************************************************************/


void PrintMetaDataList(MD_Vector& MD)
{
	std::cout << "FOUND FOLLOWING METADATA:\n";
	for (auto& MetaData : MD)
	{
		std::cout << MetaData->ID << " : Type: ";
		switch (MetaData->type)
		{
		case MetaData::EMETAINFOTYPE::EMI_COLLIDER:				
			std::cout << "Collider\n";				break;
		case MetaData::EMETAINFOTYPE::EMI_MESH:					
			std::cout << "Mesh\n";					break;
		case MetaData::EMETAINFOTYPE::EMI_SCENE:				
			std::cout << "Scene\n";					break;
		case MetaData::EMETAINFOTYPE::EMI_SKELETAL:				
			std::cout << "Skeletal\n";				break;
		case MetaData::EMETAINFOTYPE::EMI_SKELETALANIMATION:	
			std::cout << "Skeletal Animation\n";	break;
		case MetaData::EMETAINFOTYPE::EMI_ANIMATIONCLIP:		
			std::cout << "Animation Clip\n";		break;
		case MetaData::EMETAINFOTYPE::EMI_ANIMATIONEVENT:		
			std::cout << "Animation Event\n";		break;
		case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:			
			std::cout << "TexureSet\n";				break;
		default:
			break;
		}
	}
}


/************************************************************************************************/


Resource* MetaDataToBlob(MetaData* Meta, iAllocator* Mem)
{
	Resource* Result = nullptr;
	switch (Meta->type)
	{
	case MetaData::EMETAINFOTYPE::EMI_TEXTURESET:
	{
		TextureSet_MetaData* TextureSet = (TextureSet_MetaData*)Meta;
		auto& NewTextureSet = Mem->allocate<TextureSetBlob>();

		NewTextureSet.GUID			= TextureSet->Guid;
		NewTextureSet.ResourceSize	= sizeof(TextureSetBlob);

		for (size_t I = 0; I < 2; ++I) {
			memcpy(NewTextureSet.Textures[I].Directory, TextureSet->Textures.TextureLocation[I].Directory, 64);
			NewTextureSet.Textures[I].guid = TextureSet->Textures.TextureID[I];
		}

		Result = (Resource*)&NewTextureSet;
		break;
	}
	default:
		break;
	}
	return Result;
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

