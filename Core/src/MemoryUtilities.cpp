#include "MemoryUtilities.hpp"

#include <fstream>
#include <iostream>

#ifdef WIN32
#include <windows.h>
#include <stacktrace>
#endif

#include <format>
#include <print>
#include <filesystem>

namespace FlexKit
{
	
	/************************************************************************************************/


	void StackAllocator::Init(std::byte* _ptr, size_t s)
	{
		used   = 0;
		size   = s;
		Buffer = _ptr;
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
	bool LoadFileIntoBuffer(const char* strLoc, std::byte* buffer, size_t bufferSize, bool TextFile)
	{
#if 1
		// Use cstdlib
		size_t newSize = 0;
		std::byte Temp[512];
		//mbstowcs_s(&newSize, Temp, strLoc, ::strlen(strLoc));

		FILE* file = fopen(strLoc, "rb");

		if (!file)
		{
			FK_LOG_ERROR("LoadFileIntoBuffer: Failed to open file!");
			return false;
		}
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
		if (std::filesystem::exists(strLoc))
			return std::filesystem::file_size(strLoc);
		else
			return 0;
	    
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


	void MediumBlockAllocator::Initialise(size_t byteSize, std::byte* buffer)// Size in Bytes
	{
		constexpr size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
		blockCount	= (byteSize / AllocationFootPrint) - 1;
		blocks		= reinterpret_cast<Block*>(buffer);
		blockTable	= reinterpret_cast<BlockData*>(buffer + blockCount + 1);

		memset(blockTable, BlockData::Free, blockCount + 1);

		blocksAllocated = 0;
	}


	/************************************************************************************************/


	std::byte* MediumBlockAllocator::malloc(size_t size, bool ALIGNED, bool DebugMetaData)
	{
#ifdef _DEBUG
		if (size > MaxBlockSize())
		{
			FK_ASSERT(0);
			return nullptr;
		}
#endif

		for (size_t i = 0; i < blockCount; ++i)
			if (blockTable[i].state == BlockData::Free)
			{
				blockTable[i].state =
					BlockData::Allocated |
					(ALIGNED ? BlockData::Aligned : 0) |
					(DebugMetaData ? BlockData::DebugMD : 0);

				blocksAllocated++;
				return blocks[i].data;
			}

		throw(std::bad_alloc());

		return nullptr;
	}


	/************************************************************************************************/


	size_t MediumBlockAllocator::MaxBlockSize()
	{
		return sizeof(Block);
	}


	/************************************************************************************************/


	void MediumBlockAllocator::free(void* _ptr)
	{
		const size_t temp = (size_t)_ptr;
		const size_t temp2 = (size_t)blocks;
		const size_t index = (temp - temp2) / sizeof(Block);

		if (index > blockCount)
			throw(std::runtime_error("Invalid Free"));

		blocksAllocated--;
		blockTable[index].state = BlockData::Free;
	}


	/************************************************************************************************/


	void MediumBlockAllocator::_aligned_free(void* _ptr)
	{
		const size_t temp = (size_t)_ptr;
		const size_t temp2 = (size_t)blocks;
		const size_t index = (temp - temp2) / sizeof(Block);

		if (index > blockCount)
			throw(std::runtime_error("Invalid Free"));

#if USING(STACKTRACEMALLOC)
		if (blockTable[index].state & BlockData::DebugMD)
		{
			auto str = reinterpret_cast<std::string*>(blocks[index].data);
			str->~basic_string();
		}
#endif

#ifdef _DEBUG
		FK_ASSERT(blockTable[index].state & BlockData::Aligned, "_ALIGNED_FREE CALLED ON NON_ALIGNED FLAGGED BLOCK!!");
#endif

		blocksAllocated--;
		blockTable[index].state = BlockData::Free;
	}


	/************************************************************************************************/


	void LargeBlockAllocator::Initialise(size_t BufferSize, std::byte* Buffer)// Size in Bytes
	{
		FK_ASSERT(BufferSize < (size_t)uint32_t(-1));

		size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
		Size = BufferSize / AllocationFootPrint;
		Blocks = reinterpret_cast<Block*>(Buffer);
		size_t temp = (size_t)(Blocks + Size);

		BlockTable = reinterpret_cast<BlockData*>(temp + (temp & 0x3f));

		for (size_t itr = 0; itr < Size; ++itr)
			BlockTable[itr] = { BlockData::UNUSED, 0 };

		BlockTable[0] = { BlockData::Free, 0, uint16_t(BufferSize / AllocationFootPrint) };

		allocatedBlockCount = 0;
	}


	std::byte* LargeBlockAllocator::malloc(size_t requestsize, bool aligned)
	{
		size_t BlocksNeeded = requestsize / sizeof(Block) + ((requestsize % sizeof(Block)) > 0);
		FK_ASSERT(BlocksNeeded);

		for (size_t i = 0; i < Size; i += BlockTable[i].AllocationSize)
		{
			if (!BlockTable[i].AllocationSize)
				break;
			if (BlockTable[i].state == BlockData::Free && BlockTable[i].AllocationSize > BlocksNeeded)
			{
				BlockTable[i].state = (BlockData::Flags)(BlockData::Allocated | (aligned ? BlockData::Aligned : 0));
				if (BlockTable[i].AllocationSize > BlocksNeeded)
				{
					size_t currentBlock = i;
					size_t NextBlockData = i + BlocksNeeded;
					//for ( size_t II = i; II < NextBlockData; ++II )
					//{
					//	BlockTable[II].state  = BlockTable[i].state;
					//	BlockTable[II].Parent = currentBlock;
					//}
					if (NextBlockData < Size && BlockTable[NextBlockData].state == BlockData::UNUSED)
					{// Split Block
						BlockTable[NextBlockData].state = BlockData::Free;
						BlockTable[NextBlockData].AllocationSize = static_cast<uint16_t>(BlockTable[currentBlock].AllocationSize - BlocksNeeded);
						BlockTable[currentBlock].AllocationSize = static_cast<uint16_t>(BlocksNeeded);

						FK_ASSERT(BlockTable[NextBlockData].AllocationSize);
					}
				}

				allocatedBlockCount += BlocksNeeded;
				return (std::byte*)Blocks[i].data;
			}
		}

#ifdef _DEBUG
		auto LB = BlockTable;
		size_t LB_size_t = Size;
		for (size_t I = 0; I < LB_size_t;)
		{
			if (LB[I].state != FlexKit::LargeBlockAllocator::BlockData::Free)
			{
				printf("Block: %i : %i : ", (int)I, (int)BlockTable[I].AllocationSize);
				if (LB[I].state & FlexKit::MediumBlockAllocator::BlockData::Aligned)
					printf("Allocated and Aligned\n");
				else
					printf("Allocated\n");
			}
			I += LB[I].AllocationSize;
		}
		FK_ASSERT(0, "MEMORY ALLOCATION ERROR!");
#endif
		return nullptr;
	}


	/************************************************************************************************/


	void LargeBlockAllocator::free(void* _ptr)
	{
		size_t temp  = (size_t)_ptr;
		size_t temp2 = (size_t)Blocks;
		size_t index = (temp - temp2) / sizeof(Block);

#if _DEBUG
		FK_ASSERT((index < Size), "LargeBlockAllocator: Invalid Pointer Detected!\n");
		FK_ASSERT(BlockTable[index].state != BlockData::Free, "LargeBlockAllocator: Double Free Detected!\n");
#endif

#if USING(STACKTRACEMALLOC)
		std::destroy_at(reinterpret_cast<std::string*>(Blocks[index].data));
#endif

		allocatedBlockCount -= BlockTable[index].AllocationSize;

		BlockTable[index].state = BlockData::Free;
		Collapse(index);
	}


	/************************************************************************************************/


	void LargeBlockAllocator::_aligned_free(void* _ptr)
	{
		size_t temp = (size_t)_ptr;
		size_t temp2 = (size_t)Blocks;
		size_t index = (temp - temp2) / sizeof(Block);

		BlockTable[index].state = BlockData::Free;
		Collapse(index);
	}


	/************************************************************************************************/


	void LargeBlockAllocator::Collapse(size_t block)
	{
		while (true)
		{
			size_t next = block + BlockTable[block].AllocationSize;
			if (next < Size && BlockTable[next].state == BlockData::Free)
			{
				BlockTable[block].AllocationSize += BlockTable[next].AllocationSize;
				BlockTable[next].state = BlockData::UNUSED;
			}
			else break;
		}
	}


	/************************************************************************************************/


	BlockAllocator::BlockAllocator() noexcept :
		smallBlockAlloc		{},
		mediumBlockAlloc	{},
		largeBlockAlloc		{},
		smallBufferSize		{ 0 },
		mediumBufferSize	{ 0 },
		largeBufferSize		{ 0 }
	{}


	/************************************************************************************************/


	void BlockAllocator::Init(BlockAllocator_desc& in)
	{
		smallBufferSize		= in.SmallBlock;
		mediumBufferSize	= in.MediumBlock;
		largeBufferSize		= in.LargeBlock;

		if (in._ptr == nullptr)
			in._ptr = (std::byte*)::_aligned_malloc(smallBufferSize + mediumBufferSize + largeBufferSize, 16);

		buffer_ptr = (std::byte*)in._ptr;

		//smallBlockAlloc.Initialise	(in.SmallBlock,		in._ptr + 0);
		//mediumBlockAlloc.Initialise	(in.MediumBlock,	in._ptr + smallBufferSize);
		//largeBlockAlloc.Initialise	(in.LargeBlock,		in._ptr + smallBufferSize + mediumBufferSize);

		new(&AllocatorInterface) iBlockAllocator(this);
	}


	/************************************************************************************************/


	std::byte* BlockAllocator::malloc(const size_t size, bool MarkAligned, bool MarkDebugMetaData)
	{
		return (std::byte*)::_aligned_malloc(size, 16);
		std::unique_lock ul{ mu };

		std::byte* ret = nullptr;

		if (size <= SmallBlockAllocator::MaxAllocationSize())
		{
			ret = smallBlockAlloc.malloc(size, MarkAligned);
		}
		if (size <=  (MediumBlockAllocator::MaxBlockSize() - 64) && !ret)
		{
#if USING(STACKTRACEMALLOC)
			ret = mediumBlockAlloc.malloc(size + 64, MarkAligned, true);

			auto currentTrace = std::stacktrace::current();
			auto traceStr = new(ret) std::string{};

			for (auto frame : currentTrace)
				*traceStr += std::format("File: {}, line: {}, Description: {}\n", frame.source_file(), frame.source_line(), frame.description());
#else
			return (std::byte*)::_aligned_malloc(size, 64);

			ret = mediumBlockAlloc.malloc(size, MarkAligned, MarkDebugMetaData);
#endif
		}
		if (!ret)
		{
#if USING(STACKTRACEMALLOC)
			ret = largeBlockAlloc.malloc(size + 64, MarkAligned);

			auto currentTrace = std::stacktrace::current();
			auto traceStr = new(ret) std::string{};

			for (auto frame : currentTrace)
				*traceStr += std::format("File: {}, line: {}, Description: {}\n", frame.source_file(), frame.source_line(), frame.description());

			ret += 64;
#else
			ret = largeBlockAlloc.malloc(size, MarkAligned);
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
		return (std::byte*)::_aligned_malloc(size, 64);

		//return (std::byte*)::_aligned_malloc(size, 64);

		if (Debug == nullptr || DebugSize > 64)
		{
			FK_LOG_ERROR("Invalid Debug Section Header passed into allocator!");
			throw std::invalid_argument("Invalid Debug Section Header passed into allocator!");
		}

		std::byte* ret = nullptr;
		const size_t MetaDataSectionSize = Aligned ? 0x40 : 0x00;

		if (size <= SmallBlockAllocator::MaxAllocationSize())
			return (std::byte*)::_aligned_malloc(size, 64);
		    //ret = _aligned_malloc(size + MetaDataSectionSize, 0x40);
		if (size <=  MediumBlockAllocator::MaxBlockSize() && !ret)
			return (std::byte*)::_aligned_malloc(size, 64);

			//ret = _aligned_malloc(size + MetaDataSectionSize, 0x40, true);
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
		std::byte* NewBuffer		= (std::byte*)malloc(AlignedSize(s, alignment), true, MarkDebugMetaData);
		const size_t alignoffset	= (size_t)(NewBuffer) % alignment;
		const size_t Offset			= (alignment - alignoffset);

		return NewBuffer + 0;
	}


	/************************************************************************************************/

		
	void BlockAllocator::free(void* _ptr)
	{
		::_aligned_free(_ptr);
		return;

		if (_ptr == nullptr)
			return;

		std::unique_lock ul(mu);

		if (InSmallRange(reinterpret_cast<std::byte*>(_ptr)))
			smallBlockAlloc.free(reinterpret_cast<void*>(_ptr));
		if (InMediumRange(reinterpret_cast<std::byte*>(_ptr)))
			mediumBlockAlloc.free(reinterpret_cast<void*>(_ptr));
		else if (InLargeRange(reinterpret_cast<std::byte*>(_ptr)))
			largeBlockAlloc.free(reinterpret_cast<void*>(_ptr));
	}


	/************************************************************************************************/


	void PrintBlockStatus(BlockAllocator* BlockAlloc)
	{
		{
			std::cout << "small Blocks Allocated\n";

			auto SB = BlockAlloc->smallBlockAlloc.Blocks;
			size_t SB_size_t = BlockAlloc->smallBlockAlloc.Size;
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

			auto MB				= BlockAlloc->mediumBlockAlloc.blockTable;
			size_t MB_size_t	= BlockAlloc->mediumBlockAlloc.blockCount;
			for (size_t I = 0; I < MB_size_t; ++I)
			{
				if (!MB[I].state)
					continue;

				std::cout << "Block: " << I << " : " << BlockAlloc->mediumBlockAlloc.blocks + I;
				if (MB[I].state & FlexKit::MediumBlockAllocator::BlockData::Aligned)
					std::cout << " Aligned\n";
				else
					std::cout << " Allocated\n";

				if (MB[I].state & FlexKit::MediumBlockAllocator::BlockData::DebugMD) {
					std::cout << "Meta Data Found: \n";

#if USING(STACKTRACEMALLOC)
					BlockAlloc->mediumBlockAlloc.blocks[I].data[0x41] = (std::byte)'\0';
					auto str = reinterpret_cast<std::string*>(BlockAlloc->mediumBlockAlloc.blocks[I].data);

					std::cout << *str << "\n";
#else
					std::cout << (const char*)BlockAlloc->mediumBlockAlloc.blocks[I].data << "\n";
#endif
				}
			}
		}

		{
			std::cout << "Large Blocks Allocated\n";

			auto LB		= BlockAlloc->largeBlockAlloc.BlockTable;
			auto blocks = BlockAlloc->largeBlockAlloc.Blocks;
			size_t LB_size_t = BlockAlloc->largeBlockAlloc.Size;
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


	/************************************************************************************************/


	void BlockAllocator::_aligned_free(void* _ptr)
	{
		if (!_ptr)
			return;

		std::unique_lock ul(mu);

		if (InSmallRange((std::byte*)_ptr))
			smallBlockAlloc._aligned_free(_ptr);
		if (InMediumRange(static_cast<std::byte*>(_ptr)))
			mediumBlockAlloc._aligned_free(_ptr);
		else if (InLargeRange(static_cast<std::byte*>(_ptr)))
			largeBlockAlloc._aligned_free(_ptr);
	}


	/************************************************************************************************/


	bool BlockAllocator::InSmallRange(std::byte* a_ptr)
	{
		size_t bottom	= (size_t)(buffer_ptr);
		size_t top		= (size_t)(buffer_ptr) + smallBufferSize;

		return (bottom <= (size_t)a_ptr) && ((size_t)a_ptr < top);
	}


    /************************************************************************************************/


	bool BlockAllocator::InMediumRange(std::byte* a_ptr)
	{
		size_t bottom	= ((size_t)buffer_ptr) + smallBufferSize;
		size_t top		= ((size_t)buffer_ptr) + smallBufferSize + mediumBufferSize;

		return(bottom <= (size_t)a_ptr && (size_t)a_ptr < top);
	}

    /************************************************************************************************/


	bool BlockAllocator::InLargeRange(std::byte* a_ptr)
	{
		size_t bottom	= ((size_t)buffer_ptr) + smallBufferSize + mediumBufferSize;
		size_t top		= ((size_t)buffer_ptr) + smallBufferSize + mediumBufferSize + largeBufferSize;

		return(bottom <= (size_t)a_ptr && (size_t)a_ptr < top);
	}

	/************************************************************************************************/


	BlockAllocatorStats BlockAllocator::GetStats() const
	{
		BlockAllocatorStats stats;
		stats.totalSmallBlocks		= smallBlockAlloc.Size;
		stats.smallBlocksAllocated	= smallBlockAlloc.allocated;

		stats.totalMediumBlocks		= mediumBlockAlloc.blockCount;;
		stats.mediumBlocksAllocated	= mediumBlockAlloc.blocksAllocated;

		stats.totalLargeBlocks		= largeBlockAlloc.Size;
		stats.largeBlocksAllocated	= largeBlockAlloc.allocatedBlockCount;

		return stats;
	}


	void* BlockAllocator::iBlockAllocator::malloc(size_t size)
	{
		return ParentAllocator->malloc(size);
	}


	void BlockAllocator::iBlockAllocator::free(void* _ptr)
	{
		ParentAllocator->free(_ptr);
	}


	void* BlockAllocator::iBlockAllocator::_aligned_malloc(size_t size, size_t A)
	{
		return ParentAllocator->_aligned_malloc(size, A);
	}


	void BlockAllocator::iBlockAllocator::_aligned_free(void* _ptr)
	{
		ParentAllocator->_aligned_free(_ptr);
	}


	void* BlockAllocator::iBlockAllocator::malloc_Debug(size_t n, const char* MD, size_t MDSectionSize)
	{
		return ParentAllocator->malloc_debug(n, MD, MDSectionSize, true);
	}
	
    
	BlockAllocator::operator iAllocator* ()
	{
		return &AllocatorInterface;
	}

	BlockAllocator::operator iAllocator& ()
	{
		return AllocatorInterface;
	}


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2026 Robert May

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
