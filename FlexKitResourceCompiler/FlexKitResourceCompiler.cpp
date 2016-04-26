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

#include "stdafx.h"

#include "..\Application\GameUtilities.cpp"
#include "..\Application\ResourceUtilities.cpp"
#include "..\Application\GameMemory.h"
#include "..\coreutilities\AllSourceFiles.cpp"

#include <algorithm>
#include <initializer_list>

using namespace FlexKit;

int main(int argc, char* argv[])
{
	bool FileChosen = false;

	char* Target	= nullptr;
	char* Out		= "assets\\ResourceFile.gameres";
	char* MetaInfo	= nullptr;

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
			Target = argv[I + 1];
			I++;
		}
		else if (!strcmp(argv[I], "out") || !strcmp(argv[I], "-o"))
		{
			Out = argv[I + 1];
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
		if (!Target) {
			auto Dir	= SelectFile();
			FileChosen	= Dir.Valid;
			Target		= Dir.str;
		}
	}

	switch (Mode)
	{
	case TOOL_MODE::ETOOLMODE_COMPILERESOURCE:
		if(FileChosen)
		{
			std::cout << "Compiling File: " << Target << "\n";

			FlexKit::BlockAllocator_desc BlockDesc;
			BlockDesc.LargeBlock  = MEGABYTE * 200;
			BlockDesc.MediumBlock = MEGABYTE * 100;
			BlockDesc.SmallBlock  = MEGABYTE * 50;
			BlockDesc.PoolSize	  = BlockDesc.LargeBlock + BlockDesc.MediumBlock + BlockDesc.SmallBlock;
			BlockDesc._ptr		  = (byte*)_aligned_malloc(BlockDesc.PoolSize + MEGABYTE, 0x40);

			FlexKit::BlockAllocator BlockMemory;
			BlockMemory.Init(BlockDesc);

			CompileSceneFromFBXFile_DESC Desc;
			Desc.BlockMemory    = &BlockMemory;
			Desc.CloseFBX	    = true;
			Desc.IncludeShaders = false;

			{
				auto res = CompileGeometryFromFBXFile(Target, &Desc, nullptr);
				size_t ResourceSize  = 0;
				size_t TableSize     = sizeof(ResourceEntry) * res->Resources.size() + sizeof(ResourceTable);
				ResourceTable* Table = (ResourceTable*)malloc(TableSize);
				memset(Table, 0, TableSize);
				Table->MagicNumber	 = 0xF4F3F2F1F4F3F2F1;
				Table->Version		 = 0x0000000000000001;
				Table->ResourceCount = res->Resources.size();

				std::cout << "Resources Count: " << Table->ResourceCount << "\n";

				size_t Position = TableSize;
				for (size_t I = 0; I < Table->ResourceCount; ++I)
				{
					Table->Entries[I].ResourcePosition = Position;
					Table->Entries[I].GUID = res->Resources[I]->GUID;
					Table->Entries[I].Type = res->Resources[I]->Type;
					memcpy(Table->Entries[I].ID, res->Resources[I]->ID, ID_LENGTH);
					Position += res->Resources[I]->ResourceSize;
					std::cout << "Resource Found: " << res->Resources[I]->ID << " ID: " << Table->Entries[I].GUID << "\n";
				}

				{
					FILE* F = nullptr;
					auto openRes = fopen_s(&F, Out, "wb");
					fwrite(Table, sizeof(char), TableSize, F);
					for (size_t I = 0; I < Table->ResourceCount; ++I)
						fwrite(res->Resources[I], sizeof(char), res->Resources[I]->ResourceSize, F);
					fclose(F);
				}
				free(Table);
			}

			_aligned_free(BlockDesc._ptr);
		}
		break;
	case TOOL_MODE::ETOOLMODE_LISTCONTENTS:
	{	if (FileChosen)
		{
			FILE* F           = nullptr;
			auto openRes      = fopen_s(&F, Target, "rb");
			size_t TableSize  = ReadResourceTableSize(F);
			byte* TableMemory = (byte*)malloc(TableSize);
			ResourceTable* RT = (ResourceTable*)TableMemory;

			ReadResourceTable(F, RT, TableSize);

			for (size_t I = 0; I < RT->ResourceCount; ++I)
			{
				std::cout << "Resource Found: " << RT->Entries[I].ID << " ID: " << RT->Entries[I].GUID;
				switch (RT->Entries[I].Type)
				{
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

				default:
					break;
				}
			}
			fclose(F);
			free(TableMemory);
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
