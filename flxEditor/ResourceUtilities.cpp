#include "PCH.h"
#include "ResourceUtilities.h"

#include "buildsettings.h"
#include "memoryutilities.h"
#include "AnimationUtilities.h"

#include "MeshResource.h"

#include <string>
#include <string_view>

#include <inttypes.h>
#include <Windows.h>
#include <shobjidl.h> 

#include <PxPhysicsAPI.h>

using namespace FlexKit;

namespace FlexKit
{   /************************************************************************************************/


	struct Engine
	{
		iAllocator*		MemoryOut;
		iAllocator*		TempMem;
		iAllocator*		LevelMem;
		RenderSystem*	RS;
	};


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
		R->header.GUID					= S->guid;
		R->header.ResourceSize			= Size;
		R->header.Type					= EResource_Skeleton;
		R->header.JointCount			= S->JointCount;

		strcpy_s(R->header.ID, 64, "SKELETON");

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


	Resource* CreateColliderResourceBlob(char* Buffer, size_t BufferSize, GUID_t ColliderID, const char* AssetID, iAllocator* MemoryOut)
	{
		size_t ResourceSize = BufferSize + sizeof(ColliderResourceBlob);
		ColliderResourceBlob* R = (ColliderResourceBlob*)MemoryOut->_aligned_malloc(ResourceSize);
		
		memcpy(R + 1, Buffer, BufferSize);
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


	size_t AddSceneNode(CompiledScene::SceneNode* Node, CompiledScene* Scene)
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


	size_t CreateRandomID()
	{
		std::random_device generator;
		std::uniform_int_distribution<size_t> distribution(
			std::numeric_limits<size_t>::min(),
			std::numeric_limits<size_t>::max());

		return distribution(generator);
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
						auto res = WideCharToMultiByte( CP_UTF8, 0, FilePath, (int)length, Dir.str, 256, nullptr, nullptr);
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


	/************************************************************************************************/


	bool ExportGameRes(const std::string& file, const ResourceList& resources)
	{
		std::vector<ResourceBlob> blobs;

		for (auto resource : resources)
			blobs.push_back(resource->CreateBlob());

		sort(blobs.begin(),
			blobs.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.GUID < rhs.GUID;
			});

		size_t TableSize = sizeof(ResourceEntry) * blobs.size() + sizeof(ResourceTable);
		ResourceTable& Table = *(ResourceTable*)malloc(TableSize);
		EXITSCOPE(free(&Table));

		FK_ASSERT(&Table != nullptr, "Allocation Error!");

		memset(&Table, 0, TableSize);
		Table.MagicNumber   = 0xF4F3F2F1F4F3F2F1;
		Table.Version       = 0x0000000000000002;
		Table.ResourceCount = blobs.size();

		std::cout << "Resources Found: " << blobs.size() << "\n";

		size_t Position = TableSize;

		for (size_t I = 0; I < blobs.size(); ++I)
		{
			Table.Entries[I].ResourcePosition = Position;
			Table.Entries[I].GUID = blobs[I].GUID;
			Table.Entries[I].Type = blobs[I].resourceType;

			memcpy(Table.Entries[I].ID, blobs[I].ID.c_str(), ID_LENGTH);

			Position += blobs[I].bufferSize;
			std::cout << "Resource Found: " << blobs[I].ID << " ID: " << Table.Entries[I].GUID << "\n";
		}

		FILE* F = nullptr;
		if (fopen_s(&F, file.c_str(), "wb") == EINVAL)
			return false;

		EXITSCOPE(fclose(F));

		fwrite(&Table, sizeof(char), TableSize, F);

		std::cout << "writing resource " << file << '\n';

		for (auto& blob : blobs)
			fwrite(blob.buffer, sizeof(char), blob.bufferSize, F);

		return true;
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
