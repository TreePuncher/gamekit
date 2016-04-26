/**********************************************************************

Copyright (c) 2015 Robert May

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

#ifndef MEMORYUTILITIES_INLUDED
#define MEMORYUTILITIES_INLUDED

#pragma warning( disable : 4251 )
#pragma warning( disable : 4267 )

#include "..\buildsettings.h"
#include <mutex>


namespace FlexKit
{
	/************************************************************************************************/


	struct FLEXKITAPI StackAllocator
	{
		void	Init				(char* memory, size_t);
		char*	malloc				(size_t s);
		char*	malloc_MT			(size_t s);
		char*	_aligned_malloc		(size_t s, size_t alignement);
		char*	_aligned_malloc_MT	(size_t s, size_t alignement);
		void	clear();

		template<typename T>
		T& allocate()
		{ 
			auto mem = malloc(sizeof(T));

			auto t = new (mem) T();
			return *t; 
		}

		template<typename T, size_t a = 16>
		T& allocate_aligned()
		{ 
			auto mem = _aligned_malloc(sizeof(T), a);

			auto t = new (mem) T();
			return *t; 
		}

		size_t used;
		size_t size;
		char*  Buffer;

		std::mutex	critsection;
	};


	/************************************************************************************************/
	// 8 Byte Allocator

	struct SmallBlockAllocator
	{
		void Initialise(size_t BufferSize, void* Buffer)// Size in Bytes
		{
			size_t AllocationFootPrint	= sizeof(Block);
			Size						= BufferSize / AllocationFootPrint;
			Blocks						= (Block*)Buffer;

#ifdef _DEBUG
			for (size_t itr = 0; itr < Size; ++itr)
			{
				Blocks[itr].BlockFull = false;
				for (size_t itr2 = 0; itr2 < 4; ++itr2)
					Blocks[itr].state[itr2] = Block::Free;
			}
#endif
		}

		// TODO: maybe Multi-Thread?
		void* malloc()
		{
			for (size_t itr = 0; itr < Size; ++itr)
			{
				if (!Blocks[itr].BlockFull)
				{
					for (size_t itr2 = 0; itr2 < 7; ++itr2)
					{
						auto state = Blocks[itr].state[itr2];
						if (state == Block::Free)
						{
							Blocks[itr].state[itr2] = Block::Allocated;
							return &Blocks[itr].data[itr2];
						}
					}
					Blocks[itr].BlockFull = true;
				}
			}

			FK_ASSERT(0);

			return nullptr;
		}

		void free(void* _ptr)
		{
			size_t temp1       = (size_t)_ptr;
			size_t temp2       = (size_t)Blocks;
			size_t index       = (temp1 - temp2) / sizeof(Block);
			size_t BlockOffset = (temp1 - temp2) % sizeof(Block) / sizeof(Block::c);

#ifdef _DEBUG
			FK_ASSERT(Blocks[index].state[BlockOffset] != Block::Free);
#endif

			Blocks[index].state[BlockOffset] = Block::Free;
			if(Blocks[index].BlockFull) Blocks[index].BlockFull = false;
		}

		struct Block
		{
			struct c
			{
				char v[64];
			}data[7];

			enum Flags : char
			{
				Free		= 0x00,
				Allocated	= 0x01
			}state[7];
			bool   BlockFull;
			size_t Padding[7];
		}*Blocks;

		size_t Size;
	};
	
	
	/************************************************************************************************/
	// 2048 Byte Allocator

	struct MediumBlockAllocator
	{
		void Initialise(size_t BufferSize, void* Buffer)// Size in Bytes
		{
			size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
			Size                       = BufferSize / AllocationFootPrint;
			Blocks                     = (Block*)Buffer;
			BlockTable                 = (BlockData*)(Blocks + Size);
		}

		// TODO: maybe Multi-Thread?
		void* malloc(size_t size)
		{
		#ifdef _DEBUG
			if (size > 1024)
			{
				FK_ASSERT(0);
				return nullptr;
			}
		#endif

			for (size_t i = 0; i < Size; ++i)
				if (BlockTable[i].state == BlockData::Free)
				{
					BlockTable[i].state = BlockData::Allocated;
					return Blocks[i].data;
				}

			FK_ASSERT(0);
			return nullptr;
		}

		void free(void* _ptr)
		{
			size_t temp		= (size_t)_ptr;
			size_t temp2	= (size_t)Blocks;
			size_t index	= (temp - temp2) / sizeof(Blocks);

			BlockTable[index].state = BlockData::Free;
		}

		struct Block
		{
			char data[2048];
		}*Blocks;

		struct BlockData
		{
			enum Flags
			{
				Free = 0x00,
				Allocated = 0x01
			}state;
		}*BlockTable;

		size_t Size;
	};


	/************************************************************************************************/
	// 1 MB MultiBlock Allocator
	
	struct LargeBlockAllocator
	{
		void Initialise(size_t BufferSize, void* Buffer)// Size in Bytes
		{
			FK_ASSERT(BufferSize < (size_t)uint32_t(-1));

			size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
			Size		= BufferSize / AllocationFootPrint;
			Blocks		= (Block*)Buffer;
			size_t temp	= (size_t)(Blocks + Size);

			BlockTable = (BlockData*)(temp  + (temp && 0x3f));

			for (size_t itr = 0; itr < Size; ++itr)
				BlockTable[itr] = {BlockData::UNUSED, 0};

			BlockTable[0] = { BlockData::Free, uint16_t(BufferSize / AllocationFootPrint) };
		}

		// TODO: maybe Multi-Thread?
		void* malloc(size_t requestsize)
		{

#ifdef _DEBUG
			// Deny Use on on too small Blocks
			if (requestsize < MEGABYTE)
			{
				FK_ASSERT(0);
				return nullptr;
			}
#endif

			size_t BlocksNeeded = requestsize / sizeof(Block) + ((requestsize%sizeof(Block)) > 0);

			for (size_t i = 0; i < Size;)
			{
				if (BlockTable[i].state == BlockData::Free && BlockTable[i].AllocationSize > BlocksNeeded)
				{
					BlockTable[i].state = BlockData::Allocated;

					if (BlockTable[i].AllocationSize > BlocksNeeded)
					{
						size_t currentBlock	 = i;
						size_t NextBlockData = i+BlocksNeeded;
						while( NextBlockData < Size && BlockTable[NextBlockData].state == BlockData::UNUSED)
						{	// Collapse Nodes
							BlockTable[NextBlockData].state          = BlockData::Free;
							BlockTable[NextBlockData].AllocationSize = BlockTable[currentBlock].AllocationSize - BlocksNeeded;
							BlockTable[currentBlock].AllocationSize  = BlocksNeeded;

							currentBlock	= NextBlockData;
							NextBlockData	= NextBlockData + BlockTable[currentBlock].AllocationSize;
						}
					}

					return Blocks[i].data;
				}
				i += BlockTable[i].AllocationSize;
			}

			FK_ASSERT(0);
			return nullptr;
		}

		void free(void* _ptr)
		{
			size_t temp = (size_t)_ptr;
			size_t temp2 = (size_t)Blocks;
			size_t index = (temp - temp2) / sizeof(Blocks);

			BlockTable[index].state = BlockData::Free;
			if (BlockTable[index].AllocationSize + index < Size)
			{
				Collapse(index);
#if 0
				size_t nextBlock = index + BlockTable[index].AllocationSize;
				if (BlockTable[nextBlock].state == BlockData::Free)
				{
					BlockTable[index].AllocationSize += BlockTable[nextBlock].AllocationSize;
					BlockTable[nextBlock].state           = BlockData::UNUSED;
					BlockTable[nextBlock].AllocationSize  = 0;
				}
#endif
			}
		}

		void Collapse(size_t itr = 0)
		{
			size_t NextBlock = itr + BlockTable[itr].AllocationSize;

			while (NextBlock < Size)
			{
				if (BlockTable[itr].state == BlockData::Free && BlockTable[NextBlock].state == BlockData::Free)
				{// CollapseBlock
					BlockTable[itr].AllocationSize += BlockTable[NextBlock].AllocationSize;
					BlockTable[NextBlock].state		= BlockData::UNUSED;
					NextBlock = itr + BlockTable[itr].AllocationSize;
					continue;
				}
				itr = NextBlock;
				NextBlock = itr + BlockTable[itr].AllocationSize;
			}
		}

		struct Block
		{
			char data[1024*1024];
		}*Blocks;

		struct BlockData
		{
			enum Flags : char
			{
				Free		= 0x00,
				Allocated	= 0x01,
				UNUSED		= 0x02
			}state;
			uint16_t AllocationSize;
			char	 Padding[1];
		}*BlockTable;

		size_t Size;
	};

	struct BlockAllocator_desc
	{
		void* _ptr; 
		size_t PoolSize;

		size_t SmallBlock;
		size_t MediumBlock;
		size_t LargeBlock;
	};

	struct BlockAllocator
	{
		void Init(BlockAllocator_desc& in)
		{
			char* Position = (char*)in._ptr;
			SmallBlockAlloc.	Initialise(in.SmallBlock, in._ptr);
			MediumBlockAlloc.	Initialise(in.SmallBlock, (Position += in.SmallBlock));
			LargeBlockAlloc.	Initialise(in.SmallBlock, (Position += in.MediumBlock));

			Small	= in.SmallBlock;
			Medium	= in.MediumBlock;
			Large	= in.LargeBlock;

			_ptr	= (char*)in._ptr;
			in.PoolSize = Small + Medium + Large;
		}

		void* malloc(size_t size)
		{
			void* ret = nullptr;

			if (size < sizeof(SmallBlockAllocator::Block))
				ret = SmallBlockAlloc.malloc();
			if (size < sizeof(MediumBlockAllocator::Block) && !ret)
				ret = MediumBlockAlloc.malloc(size);
			if (!ret)
				ret = LargeBlockAlloc.malloc(size);

			return ret;
		}

		void free(void* _ptr)
		{
			if (InSmallRange((char*)_ptr))
				SmallBlockAlloc.free(_ptr);
			else if (InMediumRange((char*)_ptr))
				MediumBlockAlloc.free(_ptr);
			else if (InLargeRange((char*)_ptr))
				LargeBlockAlloc.free(_ptr);
		}

		SmallBlockAllocator		SmallBlockAlloc;
		MediumBlockAllocator	MediumBlockAlloc;
		LargeBlockAllocator		LargeBlockAlloc;

		char*	_ptr;
		size_t	Small, Medium, Large;


		bool InSmallRange	(char* a_ptr)	
		{ 
			char* top = _ptr + (Small);
			char* bottom = _ptr;

			return( a_ptr >= bottom && a_ptr < top ); 
		}
		bool InMediumRange	(char* a_ptr)	
		{	
			char* top = _ptr + (Small + Medium);
			char* bottom = _ptr + (Small);

			return( a_ptr >= bottom && a_ptr < top ); 
		}
		bool InLargeRange	(char* a_ptr)	
		{ 
			char* top = _ptr + ( Small + Medium + Large);
			char* bottom = _ptr + ( Small + Medium);

			return( a_ptr >= bottom && a_ptr < top ); 
		}

	};


	/************************************************************************************************/
	
	
	FLEXKITAPI bool	LoadFileIntoBuffer( char* strLoc, char* out, size_t strlenmax);


	/************************************************************************************************/
}

#endif