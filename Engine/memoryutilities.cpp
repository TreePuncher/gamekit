/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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

#include "memoryutilities.h"
#include <fstream>
#include <iostream>
#include <windows.h>

namespace FlexKit
{
	
	/************************************************************************************************/


	void StackAllocator::Init(byte* _ptr, size_t s)
	{
		used   = 0;
		size   = s;
		Buffer = _ptr;

		new(&AllocatorInterface) AllocatorAdapter(this);
	}

	
	/************************************************************************************************/


	void* StackAllocator::malloc(size_t s)
	{
		//std::unique_lock l{ m };

		const auto adjustedSize = s + 64;

		void* memory = nullptr;
		if (used + adjustedSize < size)
		{
			memory = Buffer + used;
			used += adjustedSize;
		}
#if USING(FATALERROR)
		FK_ASSERT(memory);
#endif
		if (!memory)
			printf("Memory Failed to Allocate\n");

		return memory;
	}


	/************************************************************************************************/


	void* StackAllocator::_aligned_malloc(size_t s, size_t alignment)
	{
		byte* _ptr          = (byte*)malloc(s + alignment);
		size_t alignOffset  = (size_t)_ptr % alignment;
		_ptr               += alignment - alignOffset;

		return _ptr;
	}


	/************************************************************************************************/


	void StackAllocator::clear()
	{
		//std::unique_lock l{ m };

		used = 0;
#ifdef _DEBUG
		//memset(Buffer, 0xBB, size);
#endif
	}


/************************************************************************************************/
	
// Generic Utiliteies
	bool LoadFileIntoBuffer(const char* strLoc, byte* buffer, size_t bufferSize, bool TextFile )
	{
		size_t newSize = 0;
		WCHAR Temp[512];
		mbstowcs_s(&newSize, Temp, strLoc, ::strlen(strLoc));

		FILE* file;
		fopen_s(&file, strLoc, "rb");

		if (!file)
			return false;

		auto res = fread(buffer, 1, bufferSize, file);

		//__debugbreak();

		return true;

		/*
		auto F = CreateFile(Temp, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		LARGE_INTEGER LSize; 
		auto res        = ::GetFileSizeEx(F, &LSize);
		const auto Size = LSize.LowPart;

		if ((size_t)Size < strlen)
		{
			DWORD BytesRead = 0;
			const auto res  = ReadFile(F, out, Size, &BytesRead, nullptr);
			
			CloseHandle(F);

			if (TextFile) {
				for (size_t I = 0; I < Size; ++I)
				{
					switch (out[I])
					{
					case '\r':
						out[I] = '\n';
					default:
						break;
					}
				}
			}

			return (BytesRead == Size);

		}
		*/

		return false;
	}


	/************************************************************************************************/


	size_t GetFileSize(const char* strLoc)
	{
		std::fstream File(strLoc);
		if (File.is_open())
		{
			File.seekg(std::ios::beg);
			size_t size = File.tellg();
			File.seekg(0, std::ios::end);
			size = File.tellg();
			File.seekg(std::ios::beg);
			File.close();
			return size + 1;
		}
		return 0;
	}

	
	/************************************************************************************************/


	size_t	GetLineToBuffer(const char* Buffer, size_t position, char* out, size_t OutBuffSize)
	{

		for (int I = 0; I < OutBuffSize; ++I)
		{
			out[I] = Buffer[position + I];
			if (out[I] == '\n' || out[I] == '\0')
				return position + I;
		}

		return 0;
	}


	/************************************************************************************************/


	void PrintBlockStatus(BlockAllocator* BlockAlloc)
	{
		{
			std::cout << "Small Blocks Allocated\n";

			auto SB = BlockAlloc->SmallBlockAlloc.Blocks;
			size_t SB_size_t = BlockAlloc->SmallBlockAlloc.Size;
			for (size_t I = 0; I < SB_size_t; ++I)
			{
				bool Headed = false;

				for (size_t II = 0; II < 4; ++II)
				{
					auto S = SB[I].state[II];
					if (S == FlexKit::SmallBlockAllocator::Block::Free)
						continue;
					if (!Headed)
					{
						Headed = true;
						std::cout << "Block: " << I;
					}
					std::cout << "\n	" << II << ":";
					if (S & FlexKit::SmallBlockAllocator::Block::Aligned)
						std::cout << "/Aligned/";
					else
						std::cout << "/Allocated/";

					std::cout << "\n";
				}
			}
		}

		{
			std::cout << "Medium Blocks Allocated\n";

			auto MB				= BlockAlloc->MediumBlockAlloc.BlockTable;
			size_t MB_size_t	= BlockAlloc->MediumBlockAlloc.Size;
			for (size_t I = 0; I < MB_size_t; ++I)
			{
				if (!MB[I].state)
					continue;

				std::cout << "Block: " << I << " : " << BlockAlloc->MediumBlockAlloc.Blocks + I;
				if (MB[I].state & FlexKit::MediumBlockAllocator::BlockData::Aligned)
					std::cout << " Aligned\n";
				else
					std::cout << " Allocated\n";

				if (MB[I].state & FlexKit::MediumBlockAllocator::BlockData::DebugMD) {
					std::cout << "Meta Data Found: \n";
					BlockAlloc->MediumBlockAlloc.Blocks[I].data[0x41] = '\0';
					std::cout << BlockAlloc->MediumBlockAlloc.Blocks[I].data << "\n";
				}
			}
		}

		{
			std::cout << "Large Blocks Allocated\n";

			auto LB = BlockAlloc->LargeBlockAlloc.BlockTable;
			size_t LB_size_t = BlockAlloc->LargeBlockAlloc.Size;
			for (size_t I = 0; I < LB_size_t;)
			{
				if (LB[I].state != FlexKit::LargeBlockAllocator::BlockData::Free)
				{
					std::cout << "Block: " << I;
					if (LB[I].state & FlexKit::MediumBlockAllocator::BlockData::Aligned)
						std::cout << " Aligned\n";
					else
						std::cout << " Allocated\n";
				}
				I += LB[I].AllocationSize;
			}
		}
	}
}
