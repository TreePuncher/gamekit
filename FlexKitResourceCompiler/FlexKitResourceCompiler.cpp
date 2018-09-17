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

// Headers
#include "..\buildsettings.h"
#include "..\graphicsutilities\Geometry.h"

// Sources Files
#include "common.cpp"
#include "Animation.cpp"
#include "MeshProcessing.cpp"
#include "MetaData.cpp"
#include "ResourceUtilities.cpp"
#include "TTFontLoader.cpp"

#include "..\graphicsutilities\AnimationUtilities.cpp"
#include "..\graphicsutilities\MeshUtils.cpp"
#include "..\graphicsutilities\Geometry.cpp"
#include "..\graphicsutilities\TextureUtilities.cpp"
#include "..\coreutilities\memoryutilities.cpp"
#include "..\coreutilities\Logging.cpp"
#include "..\coreutilities\MathUtils.cpp"


#include <algorithm>
#include <initializer_list>
#include <PxDefaultAllocator.h>
#include <PxErrorCallback.h>

using namespace FlexKit;


class PhysXErrorCallback : public physx::PxErrorCallback
{
public:
	PhysXErrorCallback() {}

	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
	{
	}
};

int main(int argc, char* argv[])
{
	bool FileChosen = false;

	static_vector<char*, 24> Inputs;
	static_vector<char*, 24> MetaDataFiles;
	
	char* Out = "assets\\ResourceFile.gameres";

	enum class TOOL_MODE
	{
		ETOOLMODE_COMPILERESOURCE,
		ETOOLMODE_LISTCONTENTS,
		ETOOLMODE_HELP,
	};

	TOOL_MODE Mode = TOOL_MODE::ETOOLMODE_HELP;

	for (int I = 0; I < argc; ++I)
	{
		if (!strcmp(argv[I], "target") || !strcmp(argv[I], "-f"))
		{
			FileChosen = true;
			if (argc + 1 > I)
				Inputs.push_back(argv[I + 1]);

			I++;
		}
		else if (!strcmp(argv[I], "out") || !strcmp(argv[I], "-o"))
		{
			if(argc + 1 > I)
				Out = argv[I + 1];

			I++;
		}
		else if (!strcmp(argv[I], "metadata") || !strcmp(argv[I], "-md"))
		{
			if (argc + 1 > I)
				MetaDataFiles.push_back(argv[I + 1]);

			I++;
		}
		else if (!strcmp(argv[I], "list") || !strcmp(argv[I], "-ls"))
		{
			Mode = TOOL_MODE::ETOOLMODE_LISTCONTENTS;
		}
		else if (!strcmp(argv[I], "compile") || !strcmp(argv[I], "-c"))
		{
			Mode = TOOL_MODE::ETOOLMODE_COMPILERESOURCE;
		}
		else if (!strcmp(argv[I], "help") || !strcmp(argv[I], "-h"))
		{
			Mode = TOOL_MODE::ETOOLMODE_HELP;
		}
	}

	if (Mode == TOOL_MODE::ETOOLMODE_COMPILERESOURCE || Mode == TOOL_MODE::ETOOLMODE_LISTCONTENTS)
	{
		if (!Inputs.size()) {
			auto Dir	= SelectFile();
			FileChosen	= Dir.Valid;
			Inputs.push_back(Dir.str);
		}
	}

	switch (Mode)
	{
	case TOOL_MODE::ETOOLMODE_COMPILERESOURCE:
		if(FileChosen)
		{
#if USING(TOOTLE)
			auto RES = TootleInit();
			if(RES != TOOTLE_OK)
				return -1;
#endif
			FlexKit::BlockAllocator_desc BlockDesc;
			BlockDesc.LargeBlock  = MEGABYTE * 500;
			BlockDesc.MediumBlock = MEGABYTE * 200;
			BlockDesc.SmallBlock  = MEGABYTE * 150;
			BlockDesc.PoolSize	  = BlockDesc.LargeBlock + BlockDesc.MediumBlock + BlockDesc.SmallBlock;
			BlockDesc._ptr		  = (FlexKit::byte*)_aligned_malloc(BlockDesc.PoolSize + MEGABYTE, 0x40);

			FlexKit::BlockAllocator BlockMemory;
			BlockMemory.Init(BlockDesc);

			FlexKit::StackAllocator TempMemory;
			TempMemory.Init((FlexKit::byte*)_aligned_malloc(MEGABYTE, 0x40), MEGABYTE);

			{
				static_vector<LoadGeometryRES_ptr, 16>	Resources;

				MD_Vector MetaData(BlockMemory);
				for (auto MD_Location : MetaDataFiles)
					ReadMetaData(MD_Location, BlockMemory, TempMemory, MetaData);

#if USING(RESCOMPILERVERBOSE)
				std::cout << "FOUND FOLLOWING METADATA:\n";
				std::cout << "Metadata Entry Count: " << MetaData.size() << "\n";
				FlexKit::PrintMetaDataList(MetaData);
#endif

				if (!MetaData.size())
				{
					std::cout << "No Meta Data Found!\n";
					return -1;
				}

				ResourceList ResourcesFound;
				for (auto MD : MetaData)
				{
					if (MD->UserType == MetaData::EMETA_RECIPIENT_TYPE::EMR_NONE) {
						auto NewResource = MetaDataToBlob(MD, BlockMemory);
						if(NewResource)
							ResourcesFound.push_back(NewResource);
					}
				}

				// Load Fonts
#ifdef _DEBUG
				for (auto MD : MetaData)
				{
					if (MD->type == MetaData::EMETAINFOTYPE::EMI_FONT)
					{
						auto* Font = (Font_MetaData*)MD;
						auto res = LoadTTFFile(Font->FontFile, BlockMemory);
					}
				}
#endif

				physx::PxCooking*				Cooker			= nullptr;
				physx::PxFoundation*			Foundation		= nullptr;
				physx::PxDefaultAllocator		DefaultAllocatorCallback;
				PhysXErrorCallback				DefaultErrorCallback;

				Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, DefaultErrorCallback);
				FK_ASSERT(Foundation);

				Cooker = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, physx::PxCookingParams(physx::PxTolerancesScale()));
				FK_ASSERT(Cooker);

				FINALLY{
					if (Foundation)
						Foundation->release();

					if (Cooker)
						Cooker->release();
				}FINALLYOVER;

				{
					// Load Terrain Collider
					for (auto MD : MetaData)
					{
						if (MD->type == MetaData::EMETAINFOTYPE::EMI_TERRAINCOLLIDER)
						{
							std::cout << "Cooking HeightField!\n";

							auto* TerrainCollider = (TerrainCollider_MetaData*)MD;
							ColliderStream Stream = ColliderStream(BlockMemory, 4096);

							struct TerrainSample{
								int16_t Height;
								int8_t Material_1;
								int8_t RESERVED;

							}* Samples;


							FlexKit::TextureBuffer Buffer;
							FlexKit::LoadBMP(TerrainCollider->BitmapFileLoc, BlockMemory, &Buffer);

							Samples = (TerrainSample*)BlockMemory.malloc(Buffer.WH[0] * Buffer.WH[1] * sizeof(TerrainSample));

							FlexKit::TextureBufferView<RGBA> View(&Buffer);
							auto SampleCount = Buffer.WH[0] * Buffer.WH[1];
							for (size_t I = 0; I < SampleCount; ++I){
								double Height = ((RGBA*)Buffer.Buffer)[I].Red;
								Height = Height / double(unsigned char(0xff));

								Samples[I].Height		= uint16_t(Height * 32767.0f);
								Samples[I].Material_1	= 0;
								Samples[I].RESERVED		= 0;
							}

							physx::PxHeightFieldDesc Desc;
							Desc.nbColumns		= Buffer.WH[0];
							Desc.nbRows			= Buffer.WH[1];
							Desc.samples.data	= Samples;
							Desc.samples.stride	= 4;
							auto RES = Cooker->cookHeightField(Desc, Stream);
							if (!RES)
							{
								int x = 0;
							}
							int c = 0;

							Buffer.Release();
							BlockMemory.free(Samples);
							std::cout << "Done Cooking HeightField!\n";

							auto Blob = CreateColliderResourceBlob(Stream.Buffer, Stream.used, TerrainCollider->Guid, TerrainCollider->ColliderID, BlockMemory);
							Blob->Type = EResourceType::EResource_TerrainCollider;

							ResourcesFound.push_back(Blob);
						}
					}
				}

				// Scan Input Files for Resources
				for (auto Input : Inputs)
				{
					CompileSceneFromFBXFile_DESC Desc;
					Desc.BlockMemory	= &BlockMemory;
					Desc.CloseFBX		= true;
					Desc.IncludeShaders = false;
					Desc.CookingEnabled = true;
					Desc.Foundation;
					Desc.Cooker;
					std::cout << "Compiling File: " << Input << "\n";
					Resources.push_back(CompileSceneFromFBXFile(Input, &Desc, &MetaData));

					if(Resources.back())
						ResourcesFound += Resources.back()->Resources;
				}

				size_t ResourceCount = ResourcesFound.size();
				size_t ResourceSize  = 0;

				if (!ResourceCount) {
					std::cout << "No Resources Found!\n";
					break;
				}

				size_t TableSize     = sizeof(ResourceEntry) * ResourceCount + sizeof(ResourceTable);
				ResourceTable* Table = (ResourceTable*)malloc(TableSize);
				memset(Table, 0, TableSize);
				Table->MagicNumber	 = 0xF4F3F2F1F4F3F2F1;
				Table->Version       = 0x0000000000000001;
				Table->ResourceCount = ResourceCount;

				std::cout << "Resources Found: " << ResourceCount << "\n";

				size_t Position = TableSize;

				for(size_t I = 0; I < ResourcesFound.size(); ++I)
				{
					if (!ResourcesFound[I])
						continue;

					auto& res = ResourcesFound[I];
					Table->Entries[I].ResourcePosition  = Position;
					Table->Entries[I].GUID				= res->GUID;
					Table->Entries[I].Type				= res->Type;
					memcpy(Table->Entries[I].ID, res->ID, ID_LENGTH);

					Position += res->ResourceSize;
					std::cout << "Resource Found: " << res->ID << " ID: " << Table->Entries[I].GUID << "\n";
				}

				FILE* F = nullptr;
				auto openRes = fopen_s(&F, Out, "wb");
				fwrite(Table, sizeof(char), TableSize, F);

				for (auto& res : ResourcesFound)
					fwrite(res, sizeof(char), res->ResourceSize, F);


				fclose(F);
				free(Table);
			}
			_aligned_free(BlockDesc._ptr);
	}	break;
	case TOOL_MODE::ETOOLMODE_LISTCONTENTS:
	{	if (FileChosen)
		{
			for(auto i : Inputs)
			{
				FILE* F						= nullptr;
				auto openRes				= fopen_s(&F, i, "rb");
				size_t TableSize			= ReadResourceTableSize(F);
				FlexKit::byte* TableMemory	= (FlexKit::byte*)malloc(TableSize);
				ResourceTable* RT = (ResourceTable*)TableMemory;

				ReadResourceTable(F, RT, TableSize);

				for (size_t I = 0; I < RT->ResourceCount; ++I)
				{
					std::cout << "Resource Found: " << RT->Entries[I].ID << " ID: " << RT->Entries[I].GUID;
					switch (RT->Entries[I].Type)
					{
					case EResourceType::EResource_Collider:
					std::cout << "Type : Collider" << "\n";				break;
					case EResourceType::EResource_Font:
					std::cout << " Type: Font" << "\n";					break;
					case EResourceType::EResource_Scene:
					std::cout << " Type: Scene" << "\n";				break;
					case EResourceType::EResource_Shader:
					std::cout << " Type: Shader" << "\n";				break;
					case EResourceType::EResource_SkeletalAnimation:
					std::cout << " Type: Skeletal Animation" << "\n";	break;
					case EResourceType::EResource_Skeleton:
					std::cout << " Type: Skeleton" << "\n";				break;
					case EResourceType::EResource_Texture:
					std::cout << " Type: Texture" << "\n";				break;
					case EResourceType::EResource_TriMesh:
					std::cout << " Type: TriangleMesh" << "\n";			break;
					case EResourceType::EResource_TextureSet:
					std::cout << " Type: TextureSet" << "\n";			break;
					case EResourceType::EResource_TerrainCollider:
					std::cout << " Type: TextureSet" << "\n";			break;
					default:
						break;
					}
				}
				fclose(F);
				free(TableMemory);
			}
	}	}	break;
	case TOOL_MODE::ETOOLMODE_HELP:
	{	std::cout << "COMPILES RESOURCE FILES FOR RUNTIME ENGINE\n"
			"compile or -c to set it to compile mode\n"
			"target or -f Species a FBX file for COMPILING\n"
			"list or -ls will Print the Targeted Resource File\n";
	}	break;
	default:
		break;
	}

	return 0;
}
