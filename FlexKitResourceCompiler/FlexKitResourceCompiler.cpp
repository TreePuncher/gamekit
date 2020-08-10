#include "stdafx.h"

// Headers
#include "buildsettings.h"
#include "Geometry.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>

// Sources Files
#include "common.cpp"
#include "Animation.cpp"
#include "MeshProcessing.cpp"
#include "MetaData.cpp"
#include "ResourceUtilities.cpp"
#include "TTFontLoader.cpp"
#include "SceneResource.cpp"
#include "TextureResourceUtilities.cpp"

#include "AnimationUtilities.cpp"
#include "MeshUtils.cpp"
#include "Geometry.cpp"
#include "TextureUtilities.cpp"
#include "memoryutilities.cpp"
#include "Logging.cpp"
#include "MathUtils.cpp"
#include "ThreadUtilities.h"

#include <algorithm>
#include <initializer_list>
#include <physx/extensions/PxDefaultAllocator.h>

using namespace FlexKit;
using namespace FlexKit::ResourceBuilder;


class NullErrorCallback :
    public physx::PxErrorCallback
{
public:

    virtual ~NullErrorCallback() {}

    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        std::cout << message << " : " << file << " : Line: " << line << "\n";
    }

};



int main(int argc, char* argv[])
{
	bool FileChosen = false;

    std::vector<char*> Inputs;
    std::vector<char*> glTFAssets;
    std::vector<char*> MetaDataFiles;
	
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
        else if (!strcmp(argv[I], "gltf") || !strcmp(argv[I], "-tf"))
        {
            if (argc + 1 > I)
                glTFAssets.push_back(argv[I + 1]);

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


	switch (Mode)
	{
	case TOOL_MODE::ETOOLMODE_COMPILERESOURCE:
		if(true)
		{
#if USING(TOOTLE)
			auto RES = TootleInit();
			if(RES != TOOTLE_OK)
				return -1;
#endif
            physx::PxCooking*				Cooker		= nullptr;
			physx::PxFoundation*			Foundation	= nullptr;
			physx::PxDefaultAllocator		DefaultAllocatorCallback;

            NullErrorCallback errorCallback;

			Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, DefaultAllocatorCallback, errorCallback);
			FK_ASSERT(Foundation);

			Cooker = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, physx::PxCookingParams(physx::PxTolerancesScale()));
			FK_ASSERT(Cooker);

			FINALLY{
				if (Foundation)
					Foundation->release();

				if (Cooker)
					Cooker->release();
			}FINALLYOVER;

			ResourceList resources;
			MetaDataList MetaData;

			for (auto MD_Location : MetaDataFiles)
				ReadMetaData(MD_Location, FlexKit::SystemAllocator, FlexKit::SystemAllocator, MetaData);

#if USING(RESCOMPILERVERBOSE)
			std::cout << "FOUND FOLLOWING METADATA:\n";
			std::cout << "Metadata Entry Count: " << MetaData.size() << "\n";
			PrintMetaDataList(MetaData);
#endif

			if (!MetaData.size())
			{
				std::cout << "No Meta Data Found!\n";
				//return -1;
			}

			for (const auto& MD : MetaData)
			{
                switch (MD->type)
                {
                case MetaData::EMETAINFOTYPE::EMI_TEXTURE:
                {
                    auto textureMetaData = std::static_pointer_cast<Texture_MetaData>(MD);
                    resources.push_back(CreateTextureResource(textureMetaData));
                }   break;
                case MetaData::EMETAINFOTYPE::EMI_CUBEMAPTEXTURE:
                {
                    auto cubeMap = std::static_pointer_cast<TextureCubeMap_MetaData>(MD);
                    resources.push_back(CreateCubeMapResource(cubeMap));
                }   break;
                case MetaData::EMETAINFOTYPE::EMI_FONT:
                {
                    auto Font = std::static_pointer_cast<Font_MetaData>(MD);
                    auto res = LoadTTFFile(Font->FontFile, FlexKit::SystemAllocator);
                }   break;
                case MetaData::EMETAINFOTYPE::EMI_TERRAINCOLLIDER:
                {
                    if (MD->type == MetaData::EMETAINFOTYPE::EMI_TERRAINCOLLIDER)
					{
						std::cout << "Cooking HeightField!\n";

						auto TerrainCollider	= std::static_pointer_cast<TerrainCollider_MetaData>(MD);
						ColliderStream Stream	= ColliderStream(FlexKit::SystemAllocator, 4096);

						struct TerrainSample
						{
							int16_t Height;
							int8_t  Material_1;
							int8_t  RESERVED;
						}* Samples;


						FlexKit::TextureBuffer Buffer;
						FlexKit::LoadBMP(TerrainCollider->BitmapFileLoc.c_str(), FlexKit::SystemAllocator, &Buffer);

						Samples = (TerrainSample*)FlexKit::SystemAllocator.malloc(Buffer.WH[0] * Buffer.WH[1] * sizeof(TerrainSample));

						FlexKit::TextureBufferView<RGBA> View(Buffer);
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
						FlexKit::SystemAllocator.free(Samples);
						std::cout << "Done Cooking HeightField!\n";

						FK_ASSERT(0);
						//auto Blob	= CreateColliderResourceBlob(Stream.Buffer, Stream.used, TerrainCollider->Guid, TerrainCollider->ColliderID, FlexKit::SystemAllocator);
						//Blob->Type	= EResourceType::EResource_TerrainCollider;

						//ResourcesFound.push_back(Blob);
					}
                }   break;
                default:
                    break;
                }
                    
			}

			// Scan Input Files for Resources
			for (auto assetLocation : Inputs)
			{
				CompileSceneFromFBXFile_DESC Desc;
				Desc.CloseFBX		= true;
				Desc.IncludeShaders = false;
				Desc.CookingEnabled = true;
				Desc.Foundation;
				Desc.Cooker;

				std::cout << "Compiling File: " << assetLocation << "\n";
					
				fbxsdk::FbxManager*		Manager		= fbxsdk::FbxManager::Create();
				fbxsdk::FbxIOSettings*	Settings	= fbxsdk::FbxIOSettings::Create(Manager, IOSROOT);
				Manager->SetIOSettings(Settings);

				auto [res, scene] = LoadFBXScene(assetLocation, Manager, Settings);
				if(res)
				{
					ResourceList sceneResources = CreateSceneFromFBXFile(scene, Desc, MetaData);

					for (auto& resource : sceneResources)
						resources.push_back(resource);
				}
				else
				{
					std::cout << "Failed to Open FBX File: " << assetLocation << "\n";
					MessageBox(0, L"Failed to Load File!", L"ERROR!", MB_OK);
				}
			}

            for (const auto glTF_Asset : glTFAssets)
            {
                const filesystem::path assetPath{ glTF_Asset };

                if (!assetPath.empty() && (assetPath.extension() == ".glb"))
                {
                    auto sceneResources = CreateSceneFromGlTF(assetPath, MetaData);

                    for (auto& resource : sceneResources)
                        resources.push_back(resource);
                }
            }


			if (!resources.size()) 
			{
				std::cout << "No Resources Found!\n";
				return -1;
			}

			std::vector<ResourceBlob> blobs;

			for (auto resource : resources)
				blobs.push_back(resource->CreateBlob());

			sort(blobs.begin(),
				blobs.end(),
				[](auto& lhs, auto& rhs) 
				{
					return lhs.GUID < rhs.GUID;
				});


			size_t TableSize     = sizeof(ResourceEntry) * resources.size() + sizeof(ResourceTable);
			ResourceTable& Table = *(ResourceTable*)malloc(TableSize);
			EXITSCOPE(free(&Table));

			FK_ASSERT(&Table != nullptr, "Allocation Error!");

			memset(&Table, 0, TableSize);
			Table.MagicNumber	= 0xF4F3F2F1F4F3F2F1;
			Table.Version       = 0x0000000000000002;
			Table.ResourceCount = resources.size();

			std::cout << "Resources Found: " << resources.size() << "\n";

			size_t Position = TableSize;

			for(size_t I = 0; I < blobs.size(); ++I)
			{
				Table.Entries[I].ResourcePosition	= Position;
				Table.Entries[I].GUID				= blobs[I].GUID;
				Table.Entries[I].Type				= blobs[I].resourceType;
					
				memcpy(Table.Entries[I].ID, blobs[I].ID.c_str(), ID_LENGTH);

				Position += blobs[I].bufferSize;
				std::cout << "Resource Found: " << blobs[I].ID << " ID: " << Table.Entries[I].GUID << "\n";
			}

			FILE* F			= nullptr;
			if (fopen_s(&F, Out, "wb") == EINVAL)
				return -1;

			EXITSCOPE(fclose(F));

			fwrite(&Table, sizeof(char), TableSize, F);

			std::cout << "writing resource " << Out << '\n';

            for (auto& blob : blobs)
            {
                if (auto res = fwrite(blob.buffer, sizeof(char), blob.bufferSize, F); res != blob.bufferSize)
                    __debugbreak();
            }
	}	break;
	case TOOL_MODE::ETOOLMODE_LISTCONTENTS:
	{	if (FileChosen)
		{
			for(auto i : Inputs)
			{
				FILE* F						= nullptr;
				if (fopen_s(&F, i, "rb") == EINVAL)
					return -1;

				EXITSCOPE(fclose(F));

				size_t TableSize			= ReadAssetTableSize(F);
				FlexKit::byte* TableMemory	= (FlexKit::byte*)malloc(TableSize);
				ResourceTable* RT			= (ResourceTable*)TableMemory;

				if (TableMemory == nullptr)
					return -1;

				EXITSCOPE(free(TableMemory));

				ReadAssetTable(F, RT, TableSize);

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
					std::cout << " Type: TerrainCollider" << "\n";		break;
                    case EResourceType::EResource_CubeMapTexture:
                    std::cout << " Type: CubeMapResource" << "\n";      break;
					default:
						break;
					}
				}
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


/**********************************************************************

Copyright (c) 2015 - 2020 Robert May

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
