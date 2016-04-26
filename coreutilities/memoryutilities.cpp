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

#include "memoryutilities.h"
#include <fstream>
#include <iostream>

namespace FlexKit
{
	
	/************************************************************************************************/
	
	void StackAllocator::Init(byte* _ptr, size_t s)
	{
		used   = 0;
		size   = s;
		Buffer = _ptr;
	}
	
	/************************************************************************************************/
	
	void* StackAllocator::malloc(size_t s)
	{
		void* memory = nullptr;
		if (used + s < size)
		{
			memory = Buffer + used;
			used += s;
		}
#if USING(FATALERROR)
		FK_ASSERT(memory);
#endif
		if (!memory)
			printf("Memory Failed to Allocate\n");
#ifdef _DEBUG
		memset(memory, 0, s);
#endif
		return memory;
	}
	
	/************************************************************************************************/
	
	void* StackAllocator::malloc_MT(size_t s)
	{
		critsection.lock();
		void* _ptr = malloc(s);
		critsection.unlock();
		return _ptr;
	}
	
	/************************************************************************************************/
	
	void* StackAllocator::_aligned_malloc(size_t s, size_t alignement)
	{
		byte* _ptr = (byte*)malloc( s + alignement );
		size_t alignoffset = (size_t)_ptr % alignement;
		_ptr += alignement - alignoffset;
		return _ptr;
	}
	
	/************************************************************************************************/

	void* StackAllocator::_aligned_malloc_MT(size_t s, size_t alignement)
	{
		critsection.lock();
		void* _ptr = _aligned_malloc(s, alignement);
		critsection.unlock();
		return _ptr;
	}
	
	/************************************************************************************************/
	
	void StackAllocator::clear()
	{
		used = 0;
#ifdef _DEBUG
		//memset(Buffer, 0xBB, size);
#endif
	}
	
/************************************************************************************************/
	
// Generic Utiliteies
	bool LoadFileIntoBuffer( char* strLoc, byte* out, size_t strlen )
	{
		std::fstream File(strLoc);
		if( File.is_open() )
		{
			File.seekg( std::ios::beg );
			size_t size = File.tellg();
			File.seekg( 0, std::ios::end );
			size = File.tellg();
			File.seekg( std::ios::beg );

			if( strlen > size )
			{
				File.seekg( std::ios::beg );
				File.read( (char*)out, size );
				File.close();
				return true;
			}
			File.close();
		}
		return false;
	}


	/************************************************************************************************/


	size_t GetFileSize(char* strLoc)
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
			return size;
		}
		return 0;
	}

	
	/************************************************************************************************/


	size_t	GetLineToBuffer(char* Buffer, size_t position, char* out, size_t OutBuffSize)
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


	void PrintBlockStatus(FlexKit::BlockAllocator* BlockAlloc)
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

			auto MB = BlockAlloc->MediumBlockAlloc.BlockTable;
			size_t MB_size_t = BlockAlloc->MediumBlockAlloc.Size;
			for (size_t I = 0; I < MB_size_t; ++I)
			{
				if (!MB[I].state)
					continue;
				std::cout << "Block: " << I;
				if (MB[I].state & FlexKit::MediumBlockAllocator::BlockData::Aligned)
					std::cout << " Aligned\n";
				else
					std::cout << " Allocated\n";
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
					if (LB[I].state && FlexKit::MediumBlockAllocator::BlockData::Aligned)
						std::cout << " Aligned\n";
					else
						std::cout << " Allocated\n";
				}
				I += LB[I].AllocationSize;
			}
		}
	}
}