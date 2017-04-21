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

#ifndef MEMORYUTILITIES_INLUDED
#define MEMORYUTILITIES_INLUDED

#pragma warning( disable : 4251 )
#pragma warning( disable : 4267 )

#include "..\buildsettings.h"
#include <mutex>

namespace FlexKit
{
	/************************************************************************************************/

	struct iAllocator
	{
		virtual void* malloc(size_t) = 0;
		virtual void  free(void* _ptr) = 0;
		virtual void*  _aligned_malloc(size_t, size_t A = 0x10) = 0;
		virtual void  _aligned_free(void* _ptr) = 0;
		virtual void  clear(void) {};


		template<typename T, typename ... Params>
		T& allocate(Params ... Args)
		{
			auto mem = malloc(sizeof(T));

			auto t = new (mem) T(Args...);
			return *t;
		}

		template<typename T, size_t a = 16, typename ... Params>
		T& allocate_aligned(Params ... Args)
		{
			auto mem = _aligned_malloc(sizeof(T), a);

			auto t = new (mem) T(Args...);
			return *t;
		}
	};


	struct FLEXKITAPI StackAllocator
	{
		StackAllocator()
		{
			used	= 0;
			size	= 0;
			Buffer	= 0;
		}

		StackAllocator( StackAllocator&& RHS)
		{
			used   = RHS.used;
			size   = RHS.size;
			Buffer = RHS.Buffer;

			RHS.used   = 0;
			RHS.size   = 0;
			RHS.Buffer = nullptr;
		}

		void	Init				( byte* memory, size_t );
		void*	malloc				( size_t s );
		void*	malloc_MT			( size_t s );
		void*	_aligned_malloc		( size_t s, size_t alignement = 0x10 );
		void*	_aligned_malloc_MT	( size_t s, size_t alignement = 0x10 );
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

		size_t used		= 0;
		size_t size		= 0;
		byte*  Buffer	= 0;

		struct iStackAllocator : public iAllocator
		{	
			iStackAllocator(StackAllocator* Allocator = nullptr) : ParentAllocator(Allocator){}

			void* malloc(size_t size){
				return ParentAllocator->malloc(size);
			}

			void free(void* _ptr){
			}

			void* _aligned_malloc(size_t size, size_t A){
				return ParentAllocator->_aligned_malloc(size, A);
			}

			void _aligned_free(void* _ptr){
			}

			void clear(void){ 
				ParentAllocator->clear();
			}


			StackAllocator*	ParentAllocator;
		}AllocatorInterface;

		operator iAllocator* (){return &AllocatorInterface;}

		std::mutex	critsection;
	};

	/************************************************************************************************/
	// 64 Byte Allocator

	struct SmallBlockAllocator
	{
		static int MaxAllocationSize() { return sizeof(Block::BlockSize); }
		void Initialise( size_t BufferSize, byte* Buffer )// Size in Bytes
		{
			size_t AllocationFootPrint = sizeof(Block);
			Size = BufferSize / AllocationFootPrint;
			Blocks = (Block*)Buffer;

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
		byte* malloc(size_t, bool Aligned = false)
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
							Blocks[itr].state[itr2] = (Block::Allocated | Aligned) ? Block::Aligned : 0;
							//if(itr2 == Block::BlockCount) Blocks[itr].BlockFull = true;
							return (byte*)&Blocks[itr].data[itr2];
						}
					}
					Blocks[itr].BlockFull = true;
				}
			}

			FK_ASSERT(0);

			return nullptr;
		}

		void _FreeBlock(size_t BlockID, size_t SBlockID)
		{
			Blocks[BlockID].state[SBlockID] = Block::Free;
			Blocks[BlockID].BlockFull = false;
		}

		void free(void* _ptr)
		{
			size_t temp1		= (size_t)_ptr;
			size_t temp2		= (size_t)Blocks;
			size_t BlockID		= (temp1 - temp2) / sizeof(Block);
			size_t SBlockID		= (temp1 - temp2) % sizeof(Block) / sizeof(Block::c);

			if ( SBlockID == 0x08 )
#ifdef _DEBUG
			{
				FK_ASSERT( 0, "INVALID ADDRESS!!" );
				return;
			}
#endif
				_FreeBlock(BlockID, SBlockID);

		}

		void _aligned_free(void* _ptr)
		{
			size_t temp1		= (size_t)_ptr;
			size_t temp2		= (size_t)Blocks;
			size_t BlockID		= (temp1 - temp2) / sizeof(Block);
			size_t SBlockID		= (temp1 - temp2) % sizeof(Block) / sizeof(Block::c);

			_FreeBlock(BlockID, SBlockID);
		}

		struct Block
		{
			static const size_t BlockSize  = 64;
			static const size_t BlockCount = 7;
			struct c
			{
				byte v[BlockSize];
			}data[BlockCount];

			enum Flags : char
			{
				Free	  = 0x00,
				Allocated = 0x01,
				Aligned	  = 0x02
			};
			char	state[BlockCount];
			bool	BlockFull;
			size_t	Padding[7];
		}*Blocks;

		size_t Size;
	};

	/************************************************************************************************/
	// 2048 Byte Allocator

	struct MediumBlockAllocator
	{
		void Initialise(size_t BufferSize, byte* Buffer)// Size in Bytes
		{
			size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
			Size = BufferSize / AllocationFootPrint;
			Blocks = (Block*)Buffer;
			BlockTable = (BlockData*)(Blocks + Size);
			for (size_t I = 0; I < Size; ++I)
				BlockTable[I].state = BlockData::Free;
		}

		// TODO: maybe Multi-Thread?
		byte* malloc(size_t size, bool ALIGNED = false)
		{
#ifdef _DEBUG
			if (size > MaxBlockSize())
			{
				FK_ASSERT(0);
				return nullptr;
			}
#endif

			for (size_t i = 0; i < Size; ++i)
				if (BlockTable[i].state == BlockData::Free)
				{
					BlockTable[i].state = BlockData::Allocated | (ALIGNED ? BlockData::Aligned : 0);
					return Blocks[i].data;
				}

			FK_ASSERT(0);
			return nullptr;
		}

		static size_t MaxBlockSize()
		{
			return sizeof(Block);
		}
		
		void free(void* _ptr)
		{
			size_t temp  = (size_t)_ptr;
			size_t temp2 = (size_t)Blocks;
			size_t index = (temp - temp2) / sizeof(Block);

			BlockTable[index].state = BlockData::Free;
		}

		void _aligned_free(void* _ptr)
		{
			size_t temp  = (size_t)_ptr;
			size_t temp2 = (size_t)Blocks;
			size_t index = (temp - temp2) / sizeof(Block);

#ifdef _DEBUG
			FK_ASSERT(BlockTable[index].state & BlockData::Aligned, "_ALIGNED_FREE CALLED ON NON_ALIGNED FLAGGED BLOCK!!");
#endif

			BlockTable[index].state = BlockData::Free;
		}

		struct Block
		{
			byte data[2048];
		}*Blocks;

		struct BlockData
		{
			enum Flags
			{
				Free		= 0x00,
				Allocated	= 0x01,
				Aligned		= 0x02,
			};
			byte state;
		}*BlockTable;

		size_t Size;
	};

	/************************************************************************************************/
	// 1 MB MultiBlock Allocator

	struct LargeBlockAllocator
	{
		void Initialise(size_t BufferSize, byte* Buffer)// Size in Bytes
		{
			FK_ASSERT(BufferSize < (size_t)uint32_t(-1));

			size_t AllocationFootPrint = sizeof(Block) + sizeof(BlockData);
			Size		= BufferSize / AllocationFootPrint;
			Blocks		= (Block*)Buffer;
			size_t temp = (size_t)(Blocks + Size);

			BlockTable	= (BlockData*)(temp + (temp && 0x3f));

			for (size_t itr = 0; itr < Size; ++itr)
				BlockTable[itr] = { BlockData::UNUSED, 0 };

			BlockTable[0] = { BlockData::Free, 0, uint16_t(BufferSize / AllocationFootPrint) };
		}

		// TODO: maybe Multi-Thread?
		byte* malloc(size_t requestsize, bool aligned = false)
		{
			size_t BlocksNeeded = requestsize / sizeof( Block ) + ( ( requestsize%sizeof( Block ) ) > 0 );
			FK_ASSERT(BlocksNeeded);

			for (size_t i = 0; i < Size;i += BlockTable[i].AllocationSize)
			{
				if (!BlockTable[i].AllocationSize)
					break;
				if (BlockTable[i].state == BlockData::Free && BlockTable[i].AllocationSize > BlocksNeeded)
				{
					BlockTable[i].state = (BlockData::Flags)(BlockData::Allocated | (aligned ? BlockData::Aligned : 0));
					if (BlockTable[i].AllocationSize > BlocksNeeded)
					{
						size_t currentBlock  = i;
						size_t NextBlockData = i + BlocksNeeded;
						for ( size_t II = i; II < NextBlockData; ++II )
						{
							BlockTable[II].state  = BlockTable[i].state;
							BlockTable[II].Parent = currentBlock;
						}
						if(NextBlockData < Size && BlockTable[NextBlockData].state == BlockData::UNUSED)
						{// Split Block
							BlockTable[NextBlockData].state          = BlockData::Free;
							BlockTable[NextBlockData].AllocationSize = BlockTable[currentBlock].AllocationSize - BlocksNeeded;
							BlockTable[currentBlock].AllocationSize  = BlocksNeeded;

							FK_ASSERT(BlockTable[NextBlockData].AllocationSize);
						}
					}

					return (byte*)Blocks[i].data;
				}
			}

#ifdef _DEBUG
			auto LB = BlockTable;
			size_t LB_size_t = Size;
			for (size_t I = 0; I < LB_size_t;)
			{
				if (LB[I].state != FlexKit::LargeBlockAllocator::BlockData::Free)
				{
					printf( "Block: %i : %i : ", (int)I, (int)BlockTable[I].AllocationSize);
					if (LB[I].state && FlexKit::MediumBlockAllocator::BlockData::Aligned)
						printf("Aligned\n");
					else
						printf("Allocated\n");
				}
				I += LB[I].AllocationSize;
			}
			FK_ASSERT(0, "MEMORY ALLOCATION ERROR!");
#endif
			return nullptr;
		}

		void free(void* _ptr)
		{
			size_t temp  = (size_t)_ptr;
			size_t temp2 = (size_t)Blocks;
			size_t index = (temp - temp2) / sizeof(Block);

#if _DEBUG
			FK_ASSERT((index < Size),  "FREE ERROR!\n");
#endif

			BlockTable[index].state = BlockData::Free;
			Collapse(index);
		}

		void _aligned_free(void* _ptr)
		{
			size_t temp  = (size_t)_ptr;
			size_t temp2 = (size_t)Blocks;
			size_t index = (temp - temp2) / sizeof(Block);

			BlockTable[index].state = BlockData::Free;
			Collapse(index);
		}

		void Collapse(size_t itr = 0)
		{
			while (true)
			{
				size_t next = itr + BlockTable[itr].AllocationSize;
				if (BlockTable[next].state == BlockData::Free)
				{
					BlockTable[itr].AllocationSize += BlockTable[next].AllocationSize;
					BlockTable[next].state			= BlockData::UNUSED;
				}
				else break;
			}
		}


		struct Block
		{
			char data[KILOBYTE * 128];
		}*Blocks;

		struct BlockData
		{
			enum Flags : uint16_t
			{
				Free		= 0x00,
				Allocated	= 0x01,
				UNUSED		= 0x02,
				Aligned		= 0x04,
				DEBUG		= 0x08
			}state;
			uint16_t Parent;
			uint16_t AllocationSize;
			char	 Padding_2[0x40 - 0x06]; // To Put Data Blocks on 64byte Lines for Multi-Threading
		}*BlockTable;

		size_t Size;
	};

	struct BlockAllocator_desc
	{
		byte* _ptr;
		size_t PoolSize;

		size_t SmallBlock;
		size_t MediumBlock;
		size_t LargeBlock;
	};

	struct BlockAllocator
	{
		BlockAllocator(){}

		BlockAllocator(BlockAllocator&) = delete;
		BlockAllocator& operator = (const BlockAllocator&) = delete;


		void Init( BlockAllocator_desc& in )
		{
			char* Position = (char*)in._ptr;
			SmallBlockAlloc.Initialise(in.SmallBlock, in._ptr);
			MediumBlockAlloc.Initialise(in.MediumBlock,	((byte*)in._ptr + in.SmallBlock));
			LargeBlockAlloc.Initialise(in.LargeBlock,	((byte*)in._ptr + in.SmallBlock + in.MediumBlock));

			Small = in.SmallBlock;
			Medium = in.MediumBlock;
			Large = in.LargeBlock;

			_ptr = (char*)in._ptr;
			in.PoolSize = Small + Medium + Large;

			new(&AllocatorInterface) iBlockAllocator(this);
		}

		byte* malloc(size_t size, bool MarkAllocated = false)
		{
			byte* ret = nullptr;

			if (size <= SmallBlockAllocator::MaxAllocationSize())
				ret = SmallBlockAlloc.malloc(size, MarkAllocated);
			if (size <=  MediumBlockAllocator::MaxBlockSize() && !ret)
				ret = MediumBlockAlloc.malloc(size, MarkAllocated);
			if (!ret)
				ret = LargeBlockAlloc.malloc(size, MarkAllocated);

			return ret;
		}

		char*	_aligned_malloc(size_t s, size_t alignement = 0x10)
		{
			char* _ptr = (char*)malloc(s + alignement, true);
			size_t alignoffset = (size_t)_ptr % alignement;

			if(alignoffset)
				_ptr += alignement - alignoffset;

			return _ptr;
		}

		void free(void* _ptr)
		{
			if (InSmallRange((byte*)_ptr))
				SmallBlockAlloc.free((void*)_ptr);
			else if (InMediumRange((byte*)_ptr))
				MediumBlockAlloc.free((void*)_ptr);
			else if (InLargeRange((byte*)_ptr))
				LargeBlockAlloc.free((void*)_ptr);
		}

		void _aligned_free(void* _ptr)
		{
			if (InSmallRange((byte*)_ptr))
				SmallBlockAlloc._aligned_free(_ptr);
			else if (InMediumRange((byte*)_ptr))
				MediumBlockAlloc._aligned_free(_ptr);
			else if (InLargeRange((byte*)_ptr))
				LargeBlockAlloc._aligned_free(_ptr);
		}

		template<typename T, size_t a = 16>
		T& allocate_aligned()
		{
			auto mem = _aligned_malloc(sizeof(T) + a, a);

			auto t = new (mem) T();
			return *t;
		}

		template<typename T, size_t a = 16, typename ... PARAM_TY>
		T& allocate_aligned(PARAM_TY ... Params)
		{
			auto* mem = _aligned_malloc(sizeof(T) + a, a);

			auto* t = new (mem) T(Params...);
			return *t;
		}

		template<typename T>
		T& allocate()
		{
			auto mem = malloc(sizeof(T), a);

			auto t = new (mem) T();
			return *t;
		}

		template<typename T, typename ... PARAM_TY>
		T& allocate(PARAM_TY ... Params)
		{
			auto mem = malloc(sizeof(T), a);

			auto t = new (mem) T(Params...);
			return *t;
		}

		SmallBlockAllocator		SmallBlockAlloc;
		MediumBlockAllocator	MediumBlockAlloc;
		LargeBlockAllocator		LargeBlockAlloc;

		char*	_ptr;
		size_t	Small, Medium, Large;

		bool InSmallRange(byte* a_ptr)
		{
			byte* bottom = (byte*)(SmallBlockAlloc.Blocks);
			byte* top    = ((byte*)SmallBlockAlloc.Blocks) + Small;

			return(bottom <= a_ptr && a_ptr < top);
		}

		bool InMediumRange(byte* a_ptr)
		{
			byte* bottom = ((byte*)MediumBlockAlloc.Blocks);
			byte* top    = ((byte*)MediumBlockAlloc.Blocks) + Medium;

			return(bottom <= a_ptr && a_ptr < top);
		}

		bool InLargeRange(byte* a_ptr)
		{
			byte* bottom = ((byte*)LargeBlockAlloc.Blocks);
			byte* top    = ((byte*)LargeBlockAlloc.Blocks) + Large;

			return(bottom <= a_ptr && a_ptr < top);
		}

		struct iBlockAllocator : public iAllocator
		{
			iBlockAllocator(BlockAllocator* parent = nullptr) : ParentAllocator(parent){}

			void* malloc(size_t size)
			{
				return ParentAllocator->malloc(size);
			}

			void free(void* _ptr)
			{
				ParentAllocator->free(_ptr);
			}

			void* _aligned_malloc(size_t size, size_t A)
			{
				return ParentAllocator->_aligned_malloc(size, A);
			}

			void _aligned_free(void* _ptr)
			{
				ParentAllocator->_aligned_free(_ptr);
			}

			BlockAllocator* ParentAllocator;

			operator iAllocator* ()	{ return this; }
		}AllocatorInterface;

		operator iAllocator* ()
		{ 
			return &AllocatorInterface; 
		}
	};

	/************************************************************************************************/

	FLEXKITAPI void		PrintBlockStatus	(FlexKit::BlockAllocator* BlockAlloc);
	FLEXKITAPI bool		LoadFileIntoBuffer	(const char* strLoc, byte* out, size_t strlenmax, bool textfile = true);
	FLEXKITAPI size_t	GetFileSize			(const char* strLoc);
	FLEXKITAPI size_t	GetLineToBuffer		(const char* Buffer, size_t position, char* out, size_t OutBuffSize);

	/************************************************************************************************/


	template<typename TY>
	TY ConvertEndianness(TY ValueToConvert)
	{
		union BoilerPlate
		{
			char	Bytes[sizeof(TY)];
			TY		Ty;
			operator TY () { return Ty; }
		} In, Out;

		In.Ty = ValueToConvert;

		for (size_t I = 0; I < sizeof(TY); ++I) {
			Out.Bytes[sizeof(TY) - I - 1] = In.Bytes[I];
		}

		return Out.Ty;
	}


	/************************************************************************************************/


	static uint64_t hash_DJB2(char* str)
	{
		unsigned long hash = 5381;
		int c;

		while (c = *str++)
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

		return hash;
	}


	/************************************************************************************************/

}

#endif