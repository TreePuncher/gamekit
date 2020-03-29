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

#include "Assets.h"
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{
	/************************************************************************************************/


	void InitiateAssetTable(iAllocator* Memory)
	{
		Resources.Tables			= Vector<ResourceTable*>(Memory);
		Resources.ResourceFiles		= Vector<ResourceDirectory>(Memory);
		Resources.ResourcesLoaded	= Vector<Resource*>(Memory);
		Resources.ResourceGUIDs		= Vector<GUID_t>(Memory);
		Resources.ResourceMemory	= Memory;
	}

	
	/************************************************************************************************/


	void ReleaseAssetTable()
	{
		for (auto* Table : Resources.Tables)
			Resources.ResourceMemory->free(Table);

		for (auto* Resource : Resources.ResourcesLoaded)
			Resources.ResourceMemory->free(Resource);

		Resources.Tables.Release();
		Resources.ResourceFiles.Release();
		Resources.ResourcesLoaded.Release();
		Resources.ResourceGUIDs.Release();
	}



	void AddAssetFile(char* FILELOC)
	{
		ResourceDirectory Dir;
		strcpy_s(Dir.str, FILELOC);

		FILE* F = 0;
		int S   = fopen_s(&F, FILELOC, "rb");

		size_t TableSize	 = ReadAssetTableSize(F);
		ResourceTable* Table = (ResourceTable*)Resources.ResourceMemory->_aligned_malloc(TableSize);

		if (ReadAssetTable(F, Table, TableSize))
		{
			Resources.ResourceFiles.push_back(Dir);
			Resources.Tables.push_back(Table);
		}
		else
			Resources.ResourceMemory->_aligned_free(Table);
	}


	Pair<GUID_t, bool>	FindAssetGUID(char* Str)
	{
		bool Found = false;
		GUID_t Guid = INVALIDHANDLE;

		for (auto T : Resources.Tables)
		{
			auto end = T->ResourceCount;
			for (size_t I = 0; I < end; ++I)
			{
				if (!strncmp(T->Entries[I].ID, Str, 64))
				{
					Found = true;
					Guid = T->Entries[I].GUID;
					return{ Guid, Found };
				}
			}
		}

		return{ Guid, Found };
	}


	/************************************************************************************************/


	Resource* GetAsset(AssetHandle RHandle)
	{
		if (RHandle == INVALIDHANDLE)
			return nullptr;

		Resources.ResourcesLoaded[RHandle]->RefCount++;
		return Resources.ResourcesLoaded[RHandle];
	}


	/************************************************************************************************/


	void FreeAllAssets()
	{
		for (auto R : Resources.ResourcesLoaded)
			if(Resources.ResourceMemory) Resources.ResourceMemory->_aligned_free(R);
	}


	/************************************************************************************************/


	void FreeAllAssetFiles()
	{
		for (auto T : Resources.Tables)
			Resources.ResourceMemory->_aligned_free(T);
	}


	/************************************************************************************************/


	void FreeAsset(AssetHandle RHandle)
	{
		Resources.ResourcesLoaded[RHandle]->RefCount--;
		if (Resources.ResourcesLoaded[RHandle]->RefCount == 0)
		{
			// Evict
			// TODO: Resource Eviction
		}
	}


	/************************************************************************************************/


	AssetHandle LoadGameAsset(GUID_t guid)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (Resources.ResourceGUIDs[I] == guid)
				return I;

        AssetHandle RHandle = INVALIDHANDLE;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == guid)
				{

					FILE* F             = 0;
					int S               = fopen_s(&F, Resources.ResourceFiles[TI].str, "rb");
					size_t ResourceSize = ReadAssetSize(F, t, I);

					Resource* NewResource = (Resource*)Resources.ResourceMemory->_aligned_malloc(ResourceSize);
					if (!NewResource)
					{
						// Memory Full
						// Evict A Unused Resource
						// TODO: Handle running out of memory
						FK_ASSERT(false, "OUT OF MEMORY!");
					}

					if (!ReadResource(F, t, I, NewResource))
					{
						Resources.ResourceMemory->_aligned_free(NewResource);

						FK_ASSERT(false, "FAILED TO LOAD RESOURCE!");
					}
					else
					{
						NewResource->State		= Resource::EResourceState_LOADED;
						NewResource->RefCount	= 0;
						RHandle					= Resources.ResourcesLoaded.size();
						Resources.ResourcesLoaded.push_back(NewResource);
						Resources.ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


	/************************************************************************************************/


    AssetHandle LoadGameAsset(const char* ID)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (!strcmp(Resources.ResourcesLoaded[I]->ID, ID))
				return I;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
				{
					FILE* F             = 0;
					int S               = fopen_s(&F, Resources.ResourceFiles[TI].str, "rb");
					size_t ResourceSize = FlexKit::ReadAssetSize(F, t, I);

					Resource* NewResource = (Resource*)Resources.ResourceMemory->_aligned_malloc(ResourceSize);
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
						RHandle					= Resources.ResourcesLoaded.size();
						Resources.ResourcesLoaded.push_back(NewResource);
						Resources.ResourceGUIDs.push_back(NewResource->GUID);
					}

					::fclose(F);
					return RHandle;
				}
			}
		}

		return RHandle;
	}


	/************************************************************************************************/


	bool isAssetAvailable(GUID_t ID)
	{
		for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (Resources.ResourcesLoaded[I]->GUID == ID)
				return true;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (t->Entries[I].GUID == ID)
					return true;
			}
		}

		return false;
	}


	bool isAssetAvailable(const char* ID)
	{
			for (size_t I = 0; I < Resources.ResourcesLoaded.size(); ++I)
			if (!strcmp(Resources.ResourcesLoaded[I]->ID, ID))
				return true;

        AssetHandle RHandle = 0xFFFFFFFFFFFFFFFF;
		for (size_t TI = 0; TI < Resources.Tables.size(); ++TI)
		{
			auto& t = Resources.Tables[TI];
			for (size_t I = 0; I < t->ResourceCount; ++I)
			{
				if (!strcmp(t->Entries[I].ID, ID))
					return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


	bool Asset2TriMesh(RenderSystem* RS, CopyContextHandle handle, AssetHandle RHandle, iAllocator* Memory, TriMesh* Out, bool ClearBuffers)
	{
		Resource* R = GetAsset(RHandle);
		if (R->State == Resource::EResourceState_LOADED && R->Type == EResource_TriMesh)
		{
			TriMeshAssetBlob* Blob = (TriMeshAssetBlob*)R;
			size_t BufferCount        = 0;

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
			Out->Memory		  = Memory;
			Out->VertexBuffer.clear();

			Out->BS			  = { { Blob->BS[0], Blob->BS[1], Blob->BS[2] },Blob->BS[3] };
			Out->AABB		 = 
			{ 
				{ Blob->AABB[0], Blob->AABB[1], Blob->AABB[2] },
				{ Blob->AABB[3], Blob->AABB[4], Blob->AABB[5] }
			};

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
					auto View = new (Memory->_aligned_malloc(sizeof(VertexBufferView))) VertexBufferView((byte*)(Blob->Memory + b.Begin), b.size);
					View->SetTypeFormatSize((VERTEXBUFFER_TYPE)b.Type, (VERTEXBUFFER_FORMAT)b.Format, b.size/b.Format );
					Out->Buffers.push_back(View);
				}
			}

            // Generate Tangents if they weren't loaded
            //if (!Out->HasTangents() && Out->HasNormals())
            //    GenerateTangents(Out->Buffers, Memory);

			CreateVertexBuffer(RS, handle, Out->Buffers, Out->Buffers.size(), Out->VertexBuffer);

			if (ClearBuffers)
			{
				for (size_t I = 0; I < 16; ++I)
				{
					if (Out->Buffers[I])
						Memory->_aligned_free(Out->Buffers[I]);
					Out->Buffers[I] = nullptr;
				}
				FreeAsset(RHandle);
			}
		
			Out->TriMeshID = R->GUID;
			return true;
		}
		return false;
	}


	/************************************************************************************************/


	TextureSet* Asset2TextureSet(AssetHandle RHandle, iAllocator* Memory)
	{	
		using FlexKit::TextureSet;

		TextureSet* NewTextureSet	= &Memory->allocate<TextureSet>();
		TextureSetBlob* Blob		= (TextureSetBlob*)GetAsset(RHandle);

		if (!Blob)
			return nullptr;

		for (size_t I = 0; I < 2; ++I) {
			memcpy(NewTextureSet->TextureLocations[I].Directory, Blob->Textures[I].Directory, 64);
			NewTextureSet->TextureGuids[I] = Blob->Textures[I].guid;
		}

		FreeAsset(RHandle);
		return NewTextureSet;
	}

    
    /************************************************************************************************/


    Vector<TextureBuffer> LoadCubeMapAsset(GUID_t resourceID, size_t& OUT_MIPCount, uint2& OUT_WH, DeviceFormat& OUT_format, iAllocator* allocator)
    {
        Vector<TextureBuffer> textureArray{ allocator };

        auto assetHandle            = LoadGameAsset(resourceID);
        CubeMapAssetBlob* resource  = reinterpret_cast<CubeMapAssetBlob*>(FlexKit::GetAsset(assetHandle));

        OUT_MIPCount    = resource->GetFace(0)->MipCount;
        OUT_WH          = { (uint32_t)resource->Width, (uint32_t)resource->Height };
        OUT_format      = (DeviceFormat)resource->Format;

        for (size_t I = 0; I < 6; I++)
        {
            for (size_t MIPLevel = 0; MIPLevel < resource->MipCount; MIPLevel++)
            {
                float* buffer = (float*)resource->GetFace(I)->GetMip(MIPLevel);
                const size_t bufferSize = resource->GetFace(I)->GetMipSize(MIPLevel);

                textureArray.emplace_back(
                    TextureBuffer{
                        uint2{(uint32_t)resource->Width >> MIPLevel, (uint32_t)resource->Height >> MIPLevel},
                        (char*)buffer,
                        bufferSize,
                        sizeof(float4),
                        nullptr });
            }
        }

        return textureArray;
    }


	/************************************************************************************************/


	TextureSet* LoadTextureSet(GUID_t ID, iAllocator* Memory)
	{
		bool Available = isAssetAvailable(ID);
		TextureSet* Set = nullptr;

		if (Available)
		{
			auto Handle = LoadGameAsset(ID);
			if (Handle != INVALIDHANDLE) {
				Set = Asset2TextureSet(Handle, Memory);
			}
		}

		return Set;
	}


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(RenderSystem* RS, CopyContextHandle handle, size_t GUID)
	{	// Make this atomic
		TriMeshHandle Handle;

		for (size_t idx = 0;
			 idx < GeometryTable.Geometry.size();
			 ++idx)
		{
			if (GeometryTable.Guids[idx] == GUID)
				return GeometryTable.Handle[idx];
		}

		if(!GeometryTable.FreeList.size())
		{
			auto Index	= GeometryTable.Geometry.size();
			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back(TriMesh());
			GeometryTable.GeometryIDs.push_back(nullptr);
			GeometryTable.Guids.push_back(0);
			GeometryTable.ReferenceCounts.push_back	(0);
			GeometryTable.Handle.push_back(Handle);

			auto Available = isAssetAvailable(GUID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(GUID);
			auto GameRes = GetAsset(RHandle);
			if( Asset2TriMesh(RS, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle = GeometryTable.Handles.GetNewHandle();

			auto Available = isAssetAvailable(GUID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(GUID);
			auto GameRes = GetAsset(RHandle);
			
			if(Asset2TriMesh(RS, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles			[Handle]	= Index;
				GeometryTable.GeometryIDs		[Index]		= GameRes->ID;
				GeometryTable.Guids				[Index]		= GUID;
				GeometryTable.ReferenceCounts	[Index]		= 1;
				GeometryTable.Handle			[Index]		= Handle;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}

		return Handle;
	}


	/************************************************************************************************/


	TriMeshHandle LoadTriMeshIntoTable(RenderSystem* RS, CopyContextHandle handle, const char* ID)
	{	// Make this atomic
		TriMeshHandle Handle;

		if(!GeometryTable.FreeList.size())
		{
			auto Index	= GeometryTable.Geometry.size();
			Handle		= GeometryTable.Handles.GetNewHandle();

			GeometryTable.Geometry.push_back		(TriMesh());
			GeometryTable.GeometryIDs.push_back		(nullptr);
			GeometryTable.Guids.push_back			(0);
			GeometryTable.ReferenceCounts.push_back	(0);

			auto Available = isAssetAvailable(ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(ID);
			auto GameRes = GetAsset(RHandle);
			
			if(Asset2TriMesh(RS, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= (index_t)Index;
				GeometryTable.GeometryIDs[Index]		= ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
		}
		else
		{
			auto Index	= GeometryTable.FreeList.back();
			GeometryTable.FreeList.pop_back();

			Handle		= GeometryTable.Handles.GetNewHandle();

			auto Available = isAssetAvailable(ID);
			FK_ASSERT(Available);

			auto RHandle = LoadGameAsset(ID);
			auto GameRes = GetAsset(RHandle);

			if(Asset2TriMesh(RS, handle, RHandle, GeometryTable.Memory, &GeometryTable.Geometry[Index]))
			{
				FreeAsset(RHandle);

				GeometryTable.Handles[Handle]			= Index;
				GeometryTable.GeometryIDs[Index]		= GameRes->ID;
				GeometryTable.Guids[Index]				= GameRes->GUID;
				GeometryTable.ReferenceCounts[Index]	= 1;
			}
			else
			{
				Handle = InvalidHandle_t;
			}
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
		uint16_t	framework;
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
		LoadFileIntoBuffer(TEMP, mem, Size, false);

		char*	FontPath   = nullptr;
		size_t  PathLength = strlen(dir);

		INFOBLOCK	Info;
		COMMONBLOCK	CB;

		SpriteFontAsset*	Fonts      = nullptr;
		size_t				FontCount  = 0;

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
					Fonts = (SpriteFontAsset*)outMem->malloc(sizeof(SpriteFontAsset) * FontCount);

					for (size_t I = 0; I < FontCount; ++I) {
                        memset(Fonts + I, 0, sizeof(SpriteFontAsset));

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

						auto Texture					= LoadDDSTextureFromFile(Fonts[I].FontDir, RS, RS->GetImmediateUploadQueue(), outMem);
						Fonts[I].Texture				= Texture;
						Fonts[I].TextSheetDimensions	= { CB.ScaleW, CB.ScaleH };
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


	void Release(SpriteFontAsset* asset, RenderSystem* RS)
	{
		RS->ReleaseTexture(asset->Texture);
		asset->Memory->free(asset->FontDir);
		asset->Memory->free(asset);
	}


	/************************************************************************************************/
}
