
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

#include <string>
#include <string_view>

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
typedef std::vector<FBXIDTranslation> FBXIDTranslationTable;


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


/************************************************************************************************/


MeshResource_ptr FindGeoByID(GeometryList& geometry, size_t ID)
{
	for (auto& I : geometry)
	{
		if (I->TriMeshID == ID)
			return I;
	}

	return nullptr;
}


struct CompiledMeshInfo
{
	FBXMeshDesc	MeshInfo;
	Skeleton*	S;
};



/************************************************************************************************/


void CompileAllGeometry(
	GeometryList&			geometry,
	fbxsdk::FbxNode*		node, 
	FBXIDTranslationTable&	Table, 
	MetaDataList&			MD			= MetaDataList{}, 
	bool					subDiv		= false)
{
	using FlexKit::AnimationClip;
	using FlexKit::Skeleton;
	using MeshUtilityFunctions::BuildVertexBuffer;
	using MeshUtilityFunctions::CombinedVertexBuffer;
	using MeshUtilityFunctions::IndexList;
	using MeshUtilityFunctions::TokenList;
	using MeshUtilityFunctions::MeshBuildInfo;

	auto AttributeCount = node->GetNodeAttributeCount();
	for (int itr = 0; itr < AttributeCount; ++itr)
	{
		auto Attr		= node->GetNodeAttributeByIndex(itr);
		auto nodeName	= node->GetName();

		switch (Attr->GetAttributeType())
		{
		case fbxsdk::FbxNodeAttribute::EType::eMesh:
		{
			const char* MeshName = node->GetName();
			auto test		= Attr->GetUniqueID();
			auto Mesh		= (fbxsdk::FbxMesh*)Attr;
			bool found		= false;
			bool LoadMesh	= false;
			size_t uniqueID	= (size_t)Mesh->GetUniqueID();
			auto Geo		= FindGeoByID(geometry, uniqueID);

			MetaDataList RelatedMetaData;

#if USING(RESCOMPILERVERBOSE)
			std::cout << "Found Mesh: " << MeshName << "\n";
#endif
			RelatedMetaData = FindRelatedMetaData(MD, MetaData::EMETA_RECIPIENT_TYPE::EMR_MESH, MeshName);

			if(!RelatedMetaData.size())
				LoadMesh = true;

			auto MeshInfo	= FlexKit::GetMeshMetaData(RelatedMetaData);
			auto Name		= MeshInfo ? MeshInfo->MeshID : Mesh->GetName();

			if (!FBXIDPresentInTable(Mesh->GetUniqueID(), Table))
			{
				Name = node->GetName();

				MeshResource_ptr resource = CompileMeshResource(Mesh, Name, MD, false);
				
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

	#if USING(RESCOMPILERVERBOSE)
				std::cout << "Compiled Resource: " << Name << "\n";
	#endif

				geometry.push_back(resource);
			}
		}	break;
		}
	}

	size_t NodeCount = node->GetChildCount();
	for(int itr = 0; itr < NodeCount; ++itr)
		CompileAllGeometry(geometry, node->GetChild(itr), Table, MD, subDiv);
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

