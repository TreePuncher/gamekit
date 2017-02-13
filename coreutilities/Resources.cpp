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
		size_t ResourceSize = 0;
		size_t Position = Table->Entries[Index].ResourcePosition;

#if _DEBUG
		std::chrono::system_clock Clock;
		auto Before = Clock.now();
		FINALLY
			auto After = Clock.now();
			auto Duration = chrono::duration_cast<chrono::microseconds>( After - Before );
			std::cout << "Loading Resource: " << Table->Entries[Index].ID << " : ResourceID: "<< Table->Entries[Index].GUID << "\n";
			std::cout << "Resource Load Duration: " << Duration.count() << "microseconds\n";
		FINALLYOVER
#endif

		int s = fseek(F, Position, SEEK_SET);
		s = fread(&ResourceSize, 1, 8, F);

		s = fseek(F, Position, SEEK_SET);
		s = fread(out, 1, ResourceSize, F);

		return (s == out->ResourceSize);
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


	ResourceHandle LoadGameResource(Resources* RM, GUID_t guid)
	{
		for (size_t I = 0; I < RM->ResourcesLoaded.size(); ++I)
			if (RM->ResourceGUIDs[I] == guid)
				return I;

		ResourceHandle RHandle = INVALIDHANDLE;
		for (size_t TI = 0; TI < RM->Tables.size(); ++TI)
		{
			auto& t = RM->Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == guid)
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


	bool isResourceAvailable(Resources* RM, GUID_t ID)
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


	TriMesh* Resource2TriMesh(RenderSystem* RS, Resources* RM, ResourceHandle RHandle, iAllocator* Memory, bool ClearBuffers)
	{
		Resource* R = FlexKit::GetResource(RM, RHandle);
		if (R->State == Resource::EResourceState_LOADED && R->Type == EResource_TriMesh)
		{
			TriMeshResourceBlob* Blob = (TriMeshResourceBlob*)R;
			TriMesh* Out              = &Memory->allocate_aligned<TriMesh>();
			size_t BufferCount        = 0;
			size_t Index              = 0; 

			Out->SkinTable	  = nullptr;
			Out->SkeletonGUID = Blob->SkeletonGuid;
			Out->Skeleton	  = nullptr;
			Out->Info.min.x   = Blob->Info.maxx;
			Out->Info.min.y   = Blob->Info.maxy;
			Out->Info.min.z   = Blob->Info.maxz;
			Out->Info.max.x   = Blob->Info.minx;
			Out->Info.max.y   = Blob->Info.miny;
			Out->Info.max.z   = Blob->Info.minz;
			Out->IndexCount	  = Blob->IndexCount;
			Out->Info.r		  = Blob->Info.r;
			Out->VertexBuffer.clear();

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
					View->SetTypeFormatSize((VERTEXBUFFER_TYPE)b.Type, (VERTEXBUFFER_FORMAT)b.Format, b.size/b.Format );
					Out->Buffers[Index] = View;
					BufferCount++;
				}
				++Index;
			}

			if (Blob->HasIndexBuffer)
			{
				auto b = Blob->Buffers[15];
				auto View = new(Memory->_aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)(Blob->Memory + b.Begin), b.size);
				View->SetTypeFormatSize((VERTEXBUFFER_TYPE)b.Type, (VERTEXBUFFER_FORMAT)b.Format, b.size/b.Format );
				Out->Buffers[15] = View;
			}

			FlexKit::CreateVertexBuffer(RS, Out->Buffers, BufferCount, Out->VertexBuffer);

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


	Skeleton* Resource2Skeleton(Resources* RM, ResourceHandle RHandle, iAllocator* Memory)
	{
		SkeletonResourceBlob* Blob = (SkeletonResourceBlob*)GetResource(RM, RHandle);
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


	AnimationClip Resource2AnimationClip(Resource* R, iAllocator* Memory)
	{
		AnimationResourceBlob* Anim = (AnimationResourceBlob*)R;
		AnimationClip	AC;// = &Memory->allocate_aligned<AnimationClip, 0x10>();
		AC.FPS             = Anim->FPS;
		AC.FrameCount      = Anim->FrameCount;
		AC.isLooping       = Anim->IsLooping;
		AC.guid			   = Anim->GUID;
		size_t StrSize     = 1 + strlen(Anim->ID);
		AC.mID	           = (char*)Memory->malloc(strlen(Anim->ID));
		strcpy_s(AC.mID, StrSize, Anim->ID);
		AC.Frames		   = (AnimationClip::KeyFrame*) Memory->_aligned_malloc(sizeof(AnimationClip::KeyFrame) * AC.FrameCount);
		
		AnimationResourceBlob::FrameEntry* Frames = (AnimationResourceBlob::FrameEntry*)(Anim->Buffer);
		for (size_t I = 0; I < AC.FrameCount; ++I)
		{
			size_t jointcount       = Frames[I].JointCount;
			AC.Frames[I].JointCount = jointcount;
			AC.Frames[I].Joints     = (JointHandle*)	Memory->_aligned_malloc(sizeof(JointHandle) * jointcount, 0x10);
			AC.Frames[I].Poses      = (JointPose*)		Memory->_aligned_malloc(sizeof(JointPose)   * jointcount, 0x10);
			memcpy(AC.Frames[I].Joints, Anim->Buffer + Frames[I].JointStarts,  sizeof(JointHandle)  * jointcount);
			memcpy(AC.Frames[I].Poses,  Anim->Buffer + Frames[I].PoseStarts,   sizeof(JointPose)    * jointcount);
		}

		return AC;
	}


	/************************************************************************************************/


	TextureSet* Resource2TextureSet(Resources* RM, ResourceHandle RHandle, iAllocator* Memory)
	{	
		using FlexKit::TextureSet;

		TextureSet* NewTextureSet	= &Memory->allocate<TextureSet>();
		TextureSetBlob* Blob		= (TextureSetBlob*)GetResource(RM, RHandle);

		if (!Blob)
			return nullptr;

		for (size_t I = 0; I < 2; ++I) {
			memcpy(NewTextureSet->TextureLocations[I].Directory, Blob->Textures[I].Directory, 64);
			NewTextureSet->TextureGuids[I] = Blob->Textures[I].guid;
		}

		FreeResource(RM, RHandle);
		return NewTextureSet;
	}

	
	/************************************************************************************************/


	TextureSet* LoadTextureSet(Resources* RM, GUID_t ID, iAllocator* Memory)
	{
		bool Available = isResourceAvailable(RM, ID);
		TextureSet* Set = nullptr;

		if (Available)
		{
			auto Handle = LoadGameResource(RM, ID);
			if (Handle != INVALIDHANDLE) {
				Set = Resource2TextureSet(RM, Handle, Memory);
			}
		}

		return Set;
	}


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(RenderSystem* RS, Resources* RM, GeometryTable* GT, size_t GUID)
	{	// Make this atomic
		TriMeshHandle Handle;

		if(!GT->FreeList.size())
		{
			auto Index	= GT->Geometry.size();
			Handle		= GT->Handles.GetNewHandle();

			GT->Geometry.push_back			(nullptr);
			GT->GeometryIDs.push_back		(nullptr);
			GT->Guids.push_back				(0);
			GT->ReferenceCounts.push_back	(0);

			auto Available = isResourceAvailable(RM, GUID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameResource(RM, GUID);
			auto GameRes = GetResource(RM, RHandle);
			auto NewMesh = Resource2TriMesh(RS, RM, RHandle, GT->Memory);
			FreeResource(RM, RHandle);

			GT->Handles[Handle]			= Index;
			GT->Geometry[Index]			= NewMesh;
			GT->GeometryIDs[Index]		= GameRes->ID;
			GT->Guids[Index]			= GUID;
			GT->ReferenceCounts[Index]	= 1;
		}
		else
		{
			auto Index	= GT->FreeList.back();
			GT->FreeList.pop_back();

			Handle		= GT->Handles.GetNewHandle();

			auto Available = isResourceAvailable(RM, GUID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameResource(RM, GUID);
			auto GameRes = GetResource(RM, RHandle);
			auto NewMesh = Resource2TriMesh(RS, RM, RHandle, GT->Memory);
			FreeResource(RM, RHandle);

			GT->Handles[Handle]			= Index;
			GT->Geometry[Index]			= NewMesh;
			GT->GeometryIDs[Index]		= GameRes->ID;
			GT->Guids[Index]			= GUID;
			GT->ReferenceCounts[Index]	= 1;
		}

		return Handle;
	}


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(RenderSystem* RS, Resources* RM, GeometryTable* GT, const char* ID)
	{	// Make this atomic
		TriMeshHandle Handle;

		if(!GT->FreeList.size())
		{
			auto Index	= GT->Geometry.size();
			Handle		= GT->Handles.GetNewHandle();

			GT->Geometry.push_back			(nullptr);
			GT->GeometryIDs.push_back		(nullptr);
			GT->Guids.push_back				(0);
			GT->ReferenceCounts.push_back	(0);

			auto Available = isResourceAvailable(RM, ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameResource(RM, ID);
			auto GameRes = GetResource(RM, RHandle);
			auto NewMesh = Resource2TriMesh(RS, RM, RHandle, GT->Memory);
			FreeResource(RM, RHandle);

			GT->Handles[Handle]			= Index;
			GT->Geometry[Index]			= NewMesh;
			GT->GeometryIDs[Index]		= ID;
			GT->Guids[Index]			= GameRes->GUID;
			GT->ReferenceCounts[Index]	= 1;
		}
		else
		{
			auto Index	= GT->FreeList.back();
			GT->FreeList.pop_back();

			Handle		= GT->Handles.GetNewHandle();

			auto Available = isResourceAvailable(RM, ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameResource(RM, ID);
			auto GameRes = GetResource(RM, RHandle);
			auto NewMesh = Resource2TriMesh(RS, RM, RHandle, GT->Memory);
			FreeResource(RM, RHandle);

			GT->Handles[Handle]			= Index;
			GT->Geometry[Index]			= NewMesh;
			GT->GeometryIDs[Index]		= GameRes->ID;
			GT->Guids[Index]			= GameRes->GUID;
			GT->ReferenceCounts[Index]	= 1;
		}

		return Handle;
	}


	/************************************************************************************************/


	struct INFOBLOCK
	{
		int16_t		FontSize;
		uint8_t		BitField;
		uint8_t		CharSet;
		uint16_t	stretchH;
		uint8_t		AA;
		uint8_t		PaddingUp;
		uint8_t		PaddingRight;
		uint8_t		PaddingDown;
		uint8_t		PaddingLeft;
		uint8_t		SpacingHoriz;
		uint8_t		SpacingVert;
		uint8_t		Outline;
		char		fontName;
	};


	/************************************************************************************************/


	struct COMMONBLOCK
	{
		uint16_t	lineHeight;
		uint16_t	Base;
		uint16_t	ScaleW;
		uint16_t	ScaleH;
		uint16_t	Pages;
		uint8_t		BitField;
		uint8_t		alphaChannel;
		uint8_t		RedChannel;
		uint8_t		GreenChannel;
		uint8_t		BlueChannel;
	};


	/************************************************************************************************/


	struct CHARARRAYBLOCK
	{
#pragma pack(push, 1)
		struct CHAR
		{
			uint32_t	id;
			uint16_t	x;
			uint16_t	y;
			uint16_t	width;
			uint16_t	height;
			uint16_t	xoffset;
			uint16_t	yoffset;
			uint16_t	xadvance;
			uint8_t		page;
			uint8_t		chnl;
		}First;
#pragma pack(pop)
	};


	/************************************************************************************************/


	struct KERNINGENTRY
	{
		uint32_t FirstChar;
		uint32_t SecondChar;
		uint16_t Amount;
	};

	struct KERNINGARRAYBLOCK
	{
		KERNINGENTRY First;
	};


	/************************************************************************************************/


	LoadFontResult LoadFontAsset(char* dir, char* file, RenderSystem* RS, iAllocator* tempMem, iAllocator* outMem)
	{
		char TEMP[256];
		strcpy_s(TEMP, dir);
		strcat_s(TEMP, file);

		size_t Size = 1 + GetFileSize(TEMP);
		byte* mem = (byte*)tempMem->malloc(Size);
		LoadFileIntoBuffer(TEMP, mem, Size);

		char*	FontPath   = nullptr;
		size_t  PathLength = strlen(dir);

		INFOBLOCK	Info;
		COMMONBLOCK	CB;

		FontAsset*		Fonts      = nullptr;
		size_t			FontCount  = 0;

		KERNINGENTRY*	KBlocks    = nullptr;
		size_t			KBlockUsed = 0;

		uint4	Padding = 0;// Ordering { Top, Left, Bottom, Right }

		char ID[128];
#pragma pack(push, 1)
		struct BlockDescriptor
		{
			char	 ID;
			uint32_t BlockSize;
		};
#pragma pack(pop)

		if (mem[0] == 'B' &&
			mem[1] == 'M' &&
			mem[2] == 'F' &&
			mem[3] == 0x03)
		{
			size_t Position = 4;
			while (Position < Size)
			{
				BlockDescriptor* Desc = (BlockDescriptor*)(mem + Position);
				switch (Desc->ID)
				{
				case 0x01:
				{
					auto test = sizeof(BlockDescriptor);
					INFOBLOCK* IB = (INFOBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					char* FontName = &IB->fontName;
					strcpy_s(ID, FontName);

					Padding[0] = IB->PaddingUp;
					Padding[1] = IB->PaddingLeft;
					Padding[2] = IB->PaddingDown;
					Padding[3] = IB->PaddingRight;

					Info = *IB;
				}break;
				case 0x02:
				{
					COMMONBLOCK* pCB = (COMMONBLOCK*)(mem + Position + sizeof(BlockDescriptor));

					FontCount = pCB->Pages;
					Fonts = (FontAsset*)outMem->malloc(sizeof(FontAsset) * FontCount);

					for (size_t I = 0; I < FontCount; ++I) {
						Fonts[I].Padding = Padding;
						Fonts[I].Memory  = outMem;
						strcpy_s(Fonts[I].FontName, ID);
					}

					CB = *pCB;
				}break;
				case 0x03:
				{
					int32_t BlockSize = Desc->BlockSize;
					char*	FONTPATH = (char*)(mem + Position + sizeof(BlockDescriptor));
					size_t	FontPathLen = strnlen_s(FONTPATH, BlockSize);

					for (size_t I = 0; I < FontCount; ++I) {
						size_t BufferSize = FontPathLen + PathLength + 1;
						Fonts[I].FontDir  = (char*)outMem->malloc(BufferSize);

						strcpy_s(Fonts[I].FontDir, BufferSize, dir);
						strcat_s(Fonts[I].FontDir, BufferSize, FONTPATH + I * FontPathLen);

						auto res = LoadTextureFromFile(Fonts[I].FontDir, RS, outMem);
						Fonts[I].Texture             = res;
						Fonts[I].TextSheetDimensions = { CB.ScaleW, CB.ScaleH };
					}
				}break;
				case 0x04:
				{
					CHARARRAYBLOCK* pCAB = (CHARARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
					size_t End = Desc->BlockSize / sizeof(CHARARRAYBLOCK::CHAR);
					auto Begin = &pCAB->First;

					for (size_t I = 0; I < End; ++I)
					{
						auto pCB                                      = Begin + I;
						Fonts[pCB->page].GlyphTable[pCB->id].Channel  = pCB->chnl;
						Fonts[pCB->page].GlyphTable[pCB->id].Offsets  = { float(pCB->xoffset),	float(pCB->yoffset) };
						Fonts[pCB->page].GlyphTable[pCB->id].WH       = { float(pCB->width),	float(pCB->height) };
						Fonts[pCB->page].GlyphTable[pCB->id].XY       = { float(pCB->x),		float(pCB->y) };
						Fonts[pCB->page].GlyphTable[pCB->id].Xadvance = pCB->xadvance;
						Fonts[pCB->page].FontSize[0]				  = max(Fonts[pCB->page].FontSize[0], pCB->width);
						Fonts[pCB->page].FontSize[1]				  = max(Fonts[pCB->page].FontSize[1], pCB->height);
					}
				}break;
				case 0x05:
				{
					KERNINGARRAYBLOCK* pKAB = (KERNINGARRAYBLOCK*)(mem + Position + sizeof(BlockDescriptor));
				}break;
				default:
					break;
				}
				Position = Position + sizeof(BlockDescriptor) + Desc->BlockSize;

			}
		}

		return{ FontCount, Fonts };
	}


	/************************************************************************************************/


	void FreeFontAsset(FontAsset* asset)
	{
		FreeTexture(&asset->Texture);
		asset->Memory->free(asset->FontDir);
	}


	/************************************************************************************************/
}