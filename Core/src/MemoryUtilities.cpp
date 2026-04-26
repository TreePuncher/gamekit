/**********************************************************************

Copyright (c) 2015 - 2025 Robert May

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

#include "MemoryUtilities.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#endif

#include <stacktrace>
#include <format>

namespace FlexKit
{
	
	/************************************************************************************************/


	void StackAllocator::Init(std::byte* _ptr, size_t s)
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
		std::byte* _ptr     = (std::byte*)malloc(s + alignment);
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
	bool LoadFileIntoBuffer(const char* strLoc, std::byte* buffer, size_t bufferSize, bool TextFile )
	{
#if 1
		// Use cstdlib
		size_t newSize = 0;
		std::byte Temp[512];
		//mbstowcs_s(&newSize, Temp, strLoc, ::strlen(strLoc));

		FILE* file = fopen(strLoc, "rb");

		if (!file)
			return false;

		auto res = fread(buffer, 1, bufferSize, file);

		return true;
#else
		// Use Win32
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
		else
			return false;
#endif
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


	void BlockAllocator::Init(BlockAllocator_desc& in)
	{
		Small	= in.SmallBlock;
		Medium	= in.MediumBlock;
		Large	= in.LargeBlock;

		if (in._ptr == nullptr)
			in._ptr = (std::byte*)::_aligned_malloc(Small + Medium + Large, 16);

		Buffer_ptr = (std::byte*)in._ptr;

		SmallBlockAlloc.Initialise	(in.SmallBlock,		in._ptr + 0);
		MediumBlockAlloc.Initialise	(in.MediumBlock,	in._ptr + Small);
		LargeBlockAlloc.Initialise	(in.LargeBlock,		in._ptr + Small + Medium);

		new(&AllocatorInterface) iBlockAllocator(this);
	}


	/************************************************************************************************/


	std::byte* BlockAllocator::malloc(const size_t size, bool MarkAligned, bool MarkDebugMetaData)
	{
		std::unique_lock ul{ mu };

		std::byte* ret = nullptr;

		if (size <= SmallBlockAllocator::MaxAllocationSize())
			ret = SmallBlockAlloc.malloc(size, MarkAligned);
		if (size <=  (MediumBlockAllocator::MaxBlockSize() - 64) && !ret)
		{
#if USING(STACKTRACEMALLOC)
			ret = MediumBlockAlloc.malloc(size + 64, MarkAligned, true);

			auto currentTrace = std::stacktrace::current();
			auto traceStr = new(ret) std::string{};

			for (auto frame : currentTrace)
				*traceStr += std::format("File: {}, line: {}, Description: {}\n", frame.source_file(), frame.source_line(), frame.description());
#else
			ret = MediumBlockAlloc.malloc(size, MarkAligned, MarkDebugMetaData);
#endif

			ret += 64;
		}
		if (!ret)
		{
#if USING(STACKTRACEMALLOC)
			ret = LargeBlockAlloc.malloc(size + 64, MarkAligned);

			auto currentTrace = std::stacktrace::current();
			auto traceStr = new(ret) std::string{};

			for (auto frame : currentTrace)
				*traceStr += std::format("File: {}, line: {}, Description: {}\n", frame.source_file(), frame.source_line(), frame.description());

			ret += 64;
#else
			ret = LargeBlockAlloc.malloc(size, MarkAligned);
#endif
		}

		if (ret == nullptr) {
			throw std::bad_alloc();
			FK_ASSERT(false, "BAD ALLOC!");
		}

		return ret;
	}


	/************************************************************************************************/

	// Debug String Must be below 64 Bytes
	std::byte* BlockAllocator::malloc_debug(const size_t size, const char* Debug, size_t DebugSize, bool Aligned)
	{
		if (Debug == nullptr || DebugSize > 64)
		{
			FK_LOG_ERROR("Invalid Debug Section Header passed into allocator!");
			throw std::invalid_argument("Invalid Debug Section Header passed into allocator!");
		}

		std::byte* ret = nullptr;
		const size_t MetaDataSectionSize = Aligned ? 0x40 : 0x00;

		if (size <= SmallBlockAllocator::MaxAllocationSize())
			ret = _aligned_malloc(size + MetaDataSectionSize, 0x40);
		if (size <=  MediumBlockAllocator::MaxBlockSize() && !ret)
			ret = _aligned_malloc(size + MetaDataSectionSize, 0x40, true);
		if (!ret)
			ret = _aligned_malloc(size + MetaDataSectionSize, 0x40);

		if (size > SmallBlockAllocator::MaxAllocationSize() && 
			size < MediumBlockAllocator::MaxBlockSize())
		{
			auto DebugSectionSize	= (DebugSize < MetaDataSectionSize ? DebugSize : MetaDataSectionSize);
			auto DebugStringLen		= std::strlen("DEBUG ALLOCATION");
			strncpy_s(reinterpret_cast<char*>(ret), DebugStringLen, "DEBUG ALLOCATION", DebugSectionSize);
		}

		return ret + MetaDataSectionSize;
	}


	/************************************************************************************************/


	std::byte* BlockAllocator::_aligned_malloc(size_t s, size_t alignment, bool MarkDebugMetaData)
	{
		std::byte* NewBuffer		= (std::byte*)malloc(s + alignment, true, MarkDebugMetaData);
		const size_t alignoffset	= (size_t)(NewBuffer) % alignment;
		const size_t Offset			= alignment - alignoffset;

		return NewBuffer + Offset;
	}


	/************************************************************************************************/

		
	void BlockAllocator::free(void* _ptr)
	{
		if (_ptr == nullptr)
			return;

		std::unique_lock ul(mu);

		if (InSmallRange(reinterpret_cast<std::byte*>(_ptr)))
			SmallBlockAlloc.free(reinterpret_cast<void*>(_ptr));
		if (InMediumRange(reinterpret_cast<std::byte*>(_ptr)))
			MediumBlockAlloc.free(reinterpret_cast<void*>(_ptr));
		else if (InLargeRange(reinterpret_cast<std::byte*>(_ptr)))
			LargeBlockAlloc.free(reinterpret_cast<void*>(_ptr));
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

#if USING(STACKTRACEMALLOC)
					BlockAlloc->MediumBlockAlloc.Blocks[I].data[0x41] = (std::byte)'\0';
					auto str = reinterpret_cast<std::string*>(BlockAlloc->MediumBlockAlloc.Blocks[I].data);

					std::cout << *str << "\n";
#else
					std::cout << (const char*)BlockAlloc->MediumBlockAlloc.Blocks[I].data << "\n";
#endif
				}
			}
		}

		{
			std::cout << "Large Blocks Allocated\n";

			auto LB		= BlockAlloc->LargeBlockAlloc.BlockTable;
			auto blocks = BlockAlloc->LargeBlockAlloc.Blocks;
			size_t LB_size_t = BlockAlloc->LargeBlockAlloc.Size;
			for (size_t I = 0; I < LB_size_t;)
			{
				if (LB[I].state != FlexKit::LargeBlockAllocator::BlockData::Free)
				{
					std::cout << "Block: " << I << " : " << LB[I].AllocationSize << "\n";
					if ((uint32_t)LB[I].state & (uint32_t)FlexKit::MediumBlockAllocator::BlockData::Aligned)
						std::cout << " Aligned\n";
					else
						std::cout << " Allocated\n";
#if USING(STACKTRACEMALLOC)
					auto stackString = reinterpret_cast<std::string*>(blocks[I].data);
					std::cout << *stackString << "\n";
#endif
				}
				I += LB[I].AllocationSize;
			}
		}
	}
}
