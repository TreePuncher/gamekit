/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "Resources.h"

namespace FlexKit
{
	/************************************************************************************************/


	size_t ReadResourceTableSize(FILE* F)
	{
		byte Buffer[128];

		int s = fseek(F, 0, SEEK_SET);
		s = fread(Buffer, 1, 128, F);

		ResourceTable* T = (ResourceTable*)Buffer;
		return T->ResourceCount * sizeof(ResourceEntry) + sizeof(ResourceTable);
	}


	/************************************************************************************************/


	bool ReadResourceTable(FILE* F, ResourceTable* Out, size_t TableSize)
	{
		int s = fseek(F, 0, SEEK_SET);
		s = fread(Out, 1, TableSize, F);
		return (s == TableSize);
	}


	/************************************************************************************************/


	size_t ReadResourceSize(FILE* F, ResourceTable* Table, size_t Index)
	{
		byte Buffer[8];

		int s = fseek(F, Table->Entries[Index].ResourcePosition, SEEK_SET);
		s = fread(Buffer, 1, 8, F);

		Resource* R = (Resource*)Buffer;
		return R->ResourceSize;
	}


	/************************************************************************************************/


	bool ReadResource(FILE* F, ResourceTable* Table, size_t Index, Resource* out)
	{
		byte Buffer[8];
		Resource* R = (Resource*)Buffer;

		int s = fseek(F, Table->Entries[Index].ResourcePosition, SEEK_SET);
		s = fread(Buffer, 1, 8, F);
		s = fseek(F, Table->Entries[Index].ResourcePosition, SEEK_SET);
		s = fread(out, 1, R->ResourceSize, F);

		return (s == R->ResourceSize);
	}


	/************************************************************************************************/


	void AddResourceFile(char* FILELOC, Resources* RM)
	{
		Resources::DIR Dir;
		strcpy_s(Dir.str, FILELOC);

		FILE* F = 0;
		int S   = fopen_s(&F, FILELOC, "rb");

		size_t TableSize	 = ReadResourceTableSize(F);
		ResourceTable* Table = (ResourceTable*)RM->ResourceMemory->_aligned_malloc(TableSize);

		if (ReadResourceTable(F, Table, TableSize))
		{
			RM->ResourceFiles.push_back(Dir);
			RM->Tables.push_back(Table);
		}
		else
			RM->ResourceMemory->_aligned_free(Table);
	}


	/************************************************************************************************/


	Resource* GetResource(Resources* RM, ResourceHandle RHandle)
	{
		if (RHandle == INVALIDHANDLE)
			return nullptr;

		RM->ResourcesLoaded[RHandle]->RefCount++;
		return RM->ResourcesLoaded[RHandle];
	}


	/************************************************************************************************/


	void FreeAllResources(Resources* RM)
	{
		for (auto R : RM->ResourcesLoaded)
			if(RM->ResourceMemory) RM->ResourceMemory->_aligned_free(R);
	}


	/************************************************************************************************/


	void FreeAllResourceFiles(Resources* RM)
	{
		for (auto T : RM->Tables)
			RM->ResourceMemory->_aligned_free(T);
	}


	/************************************************************************************************/


	void FreeResource(Resources* RM, ResourceHandle RHandle)
	{
		RM->ResourcesLoaded[RHandle]->RefCount--;
		if (RM->ResourcesLoaded[RHandle]->RefCount == 0)
		{
			// Evict
			// TODO: Resource Eviction
		}
	}


	/************************************************************************************************/


	ResourceHandle LoadGameResource(Resources* RM, GUID_t guid, EResourceType T)
	{
		for (size_t I = 0; I < RM->ResourcesLoaded.size(); ++I)
			if (RM->ResourceGUIDs[I] == guid && RM->ResourcesLoaded[I]->Type == T)
				return I;

		ResourceHandle RHandle = INVALIDHANDLE;
		for (size_t TI = 0; TI < RM->Tables.size(); ++TI)
		{
			auto& t = RM->Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == guid && t->Entries[I].Type == T)
				{

					FILE* F             = 0;
					int S               = fopen_s(&F, RM->ResourceFiles[TI].str, "rb");
					size_t ResourceSize = ReadResourceSize(F, t, I);

					Resource* NewResource = (Resource*)RM->ResourceMemory->_aligned_malloc(ResourceSize);
					if (!NewResource)
					{
						// Memory Full
						// Evict A Unused Resource
						// TODO: THIS
						FK_ASSERT(false, "OUT OF MEMORY!");
					}

					if (!ReadResource(F, t, I, NewResource))
					{
						RM->ResourceMemory->_aligned_free(NewResource);

						FK_ASSERT(false, "FAILED TO LOAD RESOURCE!");
					}
					else
					{
						NewResource->State		= Resource::EResourceState_UNLOADED;
						NewResource->RefCount	= 0;
						RHandle					= RM->ResourcesLoaded.size();
						RM->ResourcesLoaded.push_back(NewResource);
						RM->ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


	/************************************************************************************************/


	ResourceHandle LoadGameResource(Resources* RM, const char* ID)
	{
		for (size_t I = 0; I < RM->ResourcesLoaded.size(); ++I)
			if (!strcmp(RM->ResourcesLoaded[I]->ID, ID))
				return I;

		ResourceHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < RM->Tables.size(); ++TI)
		{
			auto& t = RM->Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
				{
					FILE* F             = 0;
					int S               = fopen_s(&F, RM->ResourceFiles[TI].str, "rb");
					size_t ResourceSize = FlexKit::ReadResourceSize(F, t, I);

					Resource* NewResource = (Resource*)RM->ResourceMemory->_aligned_malloc(ResourceSize);
					if (!NewResource)
					{
						// Memory Full
						// Evict A Unused Resource
						// TODO: THIS
						FK_ASSERT(false, "OUT OF MEMORY!");
					}

					if (!FlexKit::ReadResource(F, t, I, NewResource))
					{
						FK_ASSERT(false, "FAILED TO LOAD RESOURCE!");
					}
					else
					{
						NewResource->State		= Resource::EResourceState_LOADED;
						NewResource->RefCount	= 0;
						RHandle					= RM->ResourcesLoaded.size();
						RM->ResourcesLoaded.push_back(NewResource);
						RM->ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


	/************************************************************************************************/


	bool isResourceAvailable(Resources* RM, GUID_t ID, EResourceType Type)
	{
		for (size_t I = 0; I < RM->ResourcesLoaded.size(); ++I)
			if (RM->ResourcesLoaded[I]->GUID == ID)
				return true;

		ResourceHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < RM->Tables.size(); ++TI)
		{
			auto& t = RM->Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == ID)
					return true;
			}
		}

		return false;
	}


	bool isResourceAvailable(Resources* RM, const char* ID)
	{
			for (size_t I = 0; I < RM->ResourcesLoaded.size(); ++I)
			if (!strcmp(RM->ResourcesLoaded[I]->ID, ID))
				return true;

		ResourceHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < RM->Tables.size(); ++TI)
		{
			auto& t = RM->Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
					return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


	TriMesh* Resource2TriMesh(RenderSystem* RS, Resources* RM, ResourceHandle RHandle, BlockAllocator* Memory, ShaderSetHandle SH, ShaderTable* ST, bool ClearBuffers)
	{
		Resource* R = FlexKit::GetResource(RM, RHandle);
		if (R->State == Resource::EResourceState_LOADED && R->Type == EResource_TriMesh)
		{
			TriMeshResourceBlob* Blob = (TriMeshResourceBlob*)R;
			TriMesh* Out              = (TriMesh*)Memory->_aligned_malloc(sizeof(TriMesh));
			size_t BufferCount        = 0;
			size_t Index              = 0; 

			Out->Info.min.x = Blob->Info.maxx;
			Out->Info.min.y = Blob->Info.maxy;
			Out->Info.min.z = Blob->Info.maxz;
			Out->Info.max.x = Blob->Info.minx;
			Out->Info.max.y = Blob->Info.miny;
			Out->Info.max.z = Blob->Info.minz;
			Out->IndexCount	= Blob->IndexCount;
			Out->Info.r		= Blob->Info.r;

			if (strlen(Blob->ID))
			{
				Out->ID = (char*)Memory->malloc(64);
				strcpy_s((char*)Out->ID, 64, Blob->ID);
			} else
				Out->ID = nullptr;

			for (size_t I = 0; I < Blob->BufferCount; ++I)
			{
				auto b = Blob->Buffers[I];
				if (b.size)
				{
					auto View = new(Memory->_aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)(Blob->Memory + b.Begin), b.size);
					View->SetTypeFormatSize((FlexKit::VERTEXBUFFER_TYPE)b.Type, (FlexKit::VERTEXBUFFER_FORMAT)b.Format, b.size/b.Format );
					Out->Buffers[Index] = View;
					BufferCount++;
				}
				++Index;
			}

			if (Blob->HasIndexBuffer)
			{
				auto b = Blob->Buffers[15];
				auto View = new(Memory->_aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)(Blob->Memory + b.Begin), b.size);
				View->SetTypeFormatSize((FlexKit::VERTEXBUFFER_TYPE)b.Type, (FlexKit::VERTEXBUFFER_FORMAT)b.Format, b.size/b.Format );
				Out->Buffers[15] = View;
			}

			Shader S = Blob->HasAnimation ? ST->GetVertexShader_Animated(SH) : ST->GetVertexShader(SH);
			FlexKit::CreateVertexBuffer(RS, Out->Buffers, BufferCount, Out->VertexBuffer);
			bool res = FlexKit::CreateInputLayout(RS, Out->Buffers, BufferCount, S, &Out->VertexBuffer);

			if (ClearBuffers)
			{
				for (size_t I = 0; I < 16; ++I)
				{
					if (Out->Buffers[I])
						Memory->_aligned_free(Out->Buffers[I]);
					Out->Buffers[I] = nullptr;
				}
				++Index;
				FreeResource(RM, RHandle);
			}
		
			Out->TriMeshID = R->GUID;
			return Out;
		}
		return nullptr;
	}


	/************************************************************************************************/


	Skeleton* Resource2Skeleton(Resources* RM, ResourceHandle RHandle, BlockAllocator* Memory)
	{
		SkeletonResourceBlob* Blob = (SkeletonResourceBlob*)FlexKit::GetResource(RM, RHandle);
		Skeleton*	S = &Memory->allocate_aligned<Skeleton, 0x40>();
		S->InitiateSkeleton(Memory, Blob->JointCount);

		char* StringPool = (char*)Memory->malloc(32 * Blob->JointCount);

		for (size_t I = 0; I < Blob->JointCount; ++I)
		{
			Joint		J;
			JointPose	JP;
			float4x4	IP;
			memcpy(&IP, &Blob->Joints[I].IPose, sizeof(float4x4));
			memcpy(&JP,	&Blob->Joints[I].Pose,	sizeof(JointPose));

			J.mID		= StringPool + (32 * I);
			J.mParent	= Blob->Joints[I].Parent;
			strcpy_s((char*)J.mID, 32, Blob->Joints[I].ID);

			S->AddJoint(J, *(XMMATRIX*)&IP);
		}

		FreeResource(RM, RHandle);
		return S;
	}


	/************************************************************************************************/


	AnimationClip Resource2AnimationClip(Resource* R, BlockAllocator* Memory)
	{
		AnimationResourceBlob* Anim = (AnimationResourceBlob*)R;
		AnimationClip	AC;// = &Memory->allocate_aligned<AnimationClip, 0x10>();
		AC.FPS            = Anim->FPS;
		AC.FrameCount     = Anim->FrameCount;
		AC.isLooping      = Anim->IsLooping;
		size_t StrSize     = 1 + strlen(Anim->ID);
		AC.mID	           = (char*)Memory->malloc(strlen(Anim->ID));
		strcpy_s(AC.mID, StrSize, Anim->ID);
		AC.Frames		  = (AnimationClip::KeyFrame*) Memory->_aligned_malloc(sizeof(AnimationClip::KeyFrame) * AC.FrameCount);
		
		AnimationResourceBlob::FrameEntry* Frames = (AnimationResourceBlob::FrameEntry*)(Anim->Buffer);
		for (size_t I = 0; I < AC.FrameCount; ++I)
		{
			size_t jointcount        = Frames[I].JointCount;
			AC.Frames[I].JointCount = jointcount;
			AC.Frames[I].Joints     = (JointHandle*)	Memory->_aligned_malloc(sizeof(JointHandle) * jointcount, 0x10);
			AC.Frames[I].Poses      = (JointPose*)		Memory->_aligned_malloc(sizeof(JointPose)   * jointcount, 0x10);
			memcpy(AC.Frames[I].Joints, Anim->Buffer + Frames[I].JointStarts,  sizeof(JointHandle)  * jointcount);
			memcpy(AC.Frames[I].Poses,  Anim->Buffer + Frames[I].PoseStarts,   sizeof(JointPose)    * jointcount);
		}

		return AC;
	}


	/************************************************************************************************/

}