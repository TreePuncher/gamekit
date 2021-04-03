/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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

#include "buildsettings.h"
#include "Logging.h"
#include <atomic>
#include <mutex>

namespace FlexKit
{
	/************************************************************************************************/


	class iAllocator
	{
	public:
		iAllocator				(const iAllocator& rhs) = delete;
		iAllocator& operator =	(const iAllocator& rhs) = delete;

		virtual ~iAllocator() {}

		virtual void* malloc(size_t)							= 0;
		virtual void  free(void* _ptr)							= 0;
		virtual void* _aligned_malloc(size_t, size_t A = 0x10)	= 0;
		virtual void  _aligned_free(void* _ptr)					= 0;
		virtual void  clear(void) {};

		virtual void* malloc_Debug(size_t, const char* MD, size_t MDSectionSize) = 0;

		template<typename T, typename ... Params>
		T& allocate(Params&& ... Args)
		{
			auto mem = malloc(sizeof(T));

			auto t = new (mem) T( std::forward<Params>(Args)... );
			return *t;
		}

		template<typename T, size_t a = 16, typename ... Params>
		T& allocate_aligned(Params&& ... Args)
		{
			auto mem = _aligned_malloc(sizeof(T), a);

			auto t = new (mem) T(std::forward<Params>(Args)...);
			return *t;
		}

		template<typename T>
		void release_aligned(T* _ptr)
		{
			_ptr->~T();
			_aligned_free(_ptr);
		}

		template<typename T>
		void release(T* _ptr)
		{
			_ptr->~T();
			free(_ptr);
		}

		template<typename T>
		void release(T& _ref)
		{
			_ref.~T();
			free(&_ref);
		}

	protected:
		iAllocator() noexcept {}
	};


    class ThreadSafeAllocator : public iAllocator
    {
    public:
        ThreadSafeAllocator(iAllocator* IN_allocator) :
            allocator{ IN_allocator } {}

        void* malloc(size_t s)
        {
            std::unique_lock l{ m };
            return allocator->malloc(s);
        }

        void  free(void* _ptr)
        {
            std::unique_lock l{ m };
            allocator->free(_ptr);
        }

        void* _aligned_malloc(size_t s, size_t A = 0x10)
        {
            std::unique_lock l{ m };
            return allocator->_aligned_malloc(s, A);
        }

        void  _aligned_free(void* _ptr)
        {
            std::unique_lock l{ m };
            return allocator->_aligned_free(_ptr);
        }

        void  clear(void)
        {
            std::unique_lock l{ m };
            return allocator->clear();
        }

        void* malloc_Debug(size_t s, const char* MD, size_t MDSectionSize)
        {
            std::unique_lock l{ m };
            return allocator->malloc_Debug(s, MD, MDSectionSize);
        }

        operator iAllocator* () { return this; }

    private:

        iAllocator* allocator;
        std::mutex  m;
    };

	/************************************************************************************************/


	class _SystemAllocator : public iAllocator
	{
	public:
		_SystemAllocator() noexcept {}

		void* malloc(size_t n) override
		{
			return ::malloc(n);
		}

		void  free(void* _ptr) override
		{
			::free(_ptr);
		}

		void* _aligned_malloc(size_t n, size_t A = 0x10) override
		{
			return ::_aligned_malloc(n, A);
		}

		void  _aligned_free(void* _ptr) override
		{
			::_aligned_free(_ptr);
		}

		void* malloc_Debug(size_t n, const char*, size_t) override
		{
			return ::malloc(n);
		}

		operator iAllocator* (){return this;}
	};


	static _SystemAllocator SystemAllocator;


	/************************************************************************************************/


	template<typename TY>
	class FLEXKITAPI STLAllocatorAdapter
	{
	public:
		using size_type         = size_t;
		using difference_type   = std::ptrdiff_t;
		using value_type        = TY;

		STLAllocatorAdapter(iAllocator* IN_allocator) noexcept : allocator{ IN_allocator } {}

		[[nodiscard]]
		TY* allocate(const size_t n = 1)
		{
			auto* _ptr = allocator->malloc(sizeof(TY) * n);

			if (_ptr == nullptr)
				throw std::bad_alloc{};

			return nullptr;
		}

		void deallocate(TY* _ptr, const size_t s) noexcept
		{
			allocator->free(_ptr);
		}

		size_t max_size() const noexcept { return -1; }

		template<typename ... TY_ARGS>
		[[nodiscard]] TY& construct(TY_ARGS&& ... args)
		{
			return nullptr;
		}

		void destroy(TY& _ref)
		{
			_ref.~TY();
			allocator->free(&_ref);
		}

		std::byte* address() { return nullptr; }


		iAllocator* allocator;
	};


	/************************************************************************************************/


	class FLEXKITAPI StackAllocator
	{
	public:
		StackAllocator() noexcept :
			AllocatorInterface	{ this }
		{
			used	= 0;
			size	= 0;
			Buffer	= 0;
		}

		StackAllocator(iAllocator* allocator, size_t bufferSize) noexcept :
			AllocatorInterface	{ this }
		{
			used	= 0;
			size	= 0;
			Buffer	= 0;

			Init((byte*)allocator->_aligned_malloc(bufferSize), bufferSize);
		}

		StackAllocator(StackAllocator&& rhs) noexcept :
			AllocatorInterface  { this }
		{
			used   = rhs.used;
			size   = rhs.size;
			Buffer = rhs.Buffer;

			rhs.used   = 0;
			rhs.size   = 0;
			rhs.Buffer = nullptr;
		}

		StackAllocator& operator = (StackAllocator&& rhs) noexcept
		{
			if (Buffer == nullptr)
			{
				used	= rhs.used;
				size	= rhs.size;
				Buffer	= rhs.Buffer;

				rhs.used	= 0;
				rhs.size	= 0;
				rhs.Buffer	= nullptr;
			}
			return *this;
		}

		bool operator == (const StackAllocator& rhs) const
		{
			return rhs.Buffer == Buffer;
		}

		void	Init				(byte* memory, size_t);
		void*	malloc				(size_t s);
		void*	_aligned_malloc		(size_t s, size_t alignement = 0x10);
		void	clear				();

        void*   buffer() { return Buffer; }

		operator iAllocator* () { return &AllocatorInterface; }
		operator iAllocator& () { return  AllocatorInterface; }
	private:

        size_t  used	= 0;
        size_t  size	= 0;
		byte*   Buffer	= 0;


		struct AllocatorAdapter : public iAllocator
		{	
			explicit AllocatorAdapter(StackAllocator* Allocator = nullptr) noexcept :
				ParentAllocator(Allocator){}

			void* malloc(size_t size){
				return ParentAllocator->malloc(size);
			}

			void free(void*){}

			void* _aligned_malloc(size_t size, size_t A){
				return ParentAllocator->_aligned_malloc(size, A);
			}

			void _aligned_free(void*){}

			void clear(void){ 
				ParentAllocator->clear();
			}


			void* malloc_Debug(size_t n, const char*, size_t)
			{
				return malloc(n);
			}

			StackAllocator*	ParentAllocator;
		}AllocatorInterface;
	};


	/************************************************************************************************/
	// 64 Byte Allocator

	struct SmallBlockAllocator
	{
		SmallBlockAllocator() : 
			Blocks	{nullptr},
			Size	{0}{}

		static int MaxAllocationSize() { return sizeof(Block::BlockSize); }

		void Initialise( size_t BufferSize, byte* Buffer )// Size in Bytes
		{
			size_t AllocationFootPrint = sizeof(Block);
			Size = BufferSize / AllocationFootPrint;
			Blocks = reinterpret_cast<Block*>(Buffer);

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
			Size		= (BufferSize / AllocationFootPrint) - 1;
			Blocks		= reinterpret_cast<Block*>(Buffer);
			BlockTable	= reinterpret_cast<BlockData*>(Blocks + Size + 1);

			for (size_t I = 0; I < Size; ++I)
				BlockTable[I].state = BlockData::Free;
		}

		// TODO: maybe Multi-Thread?
		byte* malloc(size_t size, bool ALIGNED = false, bool DebugMetaData = false)
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
					BlockTable[i].state = 
						BlockData::Allocated | 
						(ALIGNED		? BlockData::Aligned : 0) | 
						(DebugMetaData	? BlockData::DebugMD : 0);
					return (byte*)&Blocks[i];
				}

			throw(std::bad_alloc());

			return nullptr;
		}


		static size_t MaxBlockSize()
		{
			return sizeof(Block);
		}
		
		void free(void* _ptr)
		{
			const size_t temp  = (size_t)_ptr;
			const size_t temp2 = (size_t)Blocks;
			const size_t index = (temp - temp2) / sizeof(Block);

			if (index > Size)
				throw(std::runtime_error("Invalid Free"));

			BlockTable[index].state = BlockData::Free;
		}

		void _aligned_free(void* _ptr)
		{
			const size_t temp  = (size_t)_ptr;
			const size_t temp2 = (size_t)Blocks;
			const size_t index = (temp - temp2) / sizeof(Block);

			if (index > Size)
				throw(std::runtime_error("Invalid Free"));

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
			enum Flags : byte
			{
				Free		= 0x00,
				Allocated	= 0x01,
				Aligned		= 0x02,
				DebugMD		= 0x04,
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
			Blocks		= reinterpret_cast<Block*>(Buffer);
			size_t temp = (size_t)(Blocks + Size);

			BlockTable	= reinterpret_cast<BlockData*>(temp + (temp & 0x3f));

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
						//for ( size_t II = i; II < NextBlockData; ++II )
						//{
						//	BlockTable[II].state  = BlockTable[i].state;
						//	BlockTable[II].Parent = currentBlock;
						//}
						if(NextBlockData < Size && BlockTable[NextBlockData].state == BlockData::UNUSED)
						{// Split Block
							BlockTable[NextBlockData].state          = BlockData::Free;
							BlockTable[NextBlockData].AllocationSize = static_cast<uint16_t>(BlockTable[currentBlock].AllocationSize - BlocksNeeded);
							BlockTable[currentBlock].AllocationSize  = static_cast<uint16_t>(BlocksNeeded);

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


		void Collapse(size_t block = 0)
		{
			while (true)
			{
				size_t next = block + BlockTable[block].AllocationSize;
				if (next < Size && BlockTable[next].state == BlockData::Free)
				{
					BlockTable[block].AllocationSize += BlockTable[next].AllocationSize;
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
		BlockAllocator() noexcept :
			SmallBlockAlloc{},
			MediumBlockAlloc{},
			LargeBlockAlloc{},

			Small{0}, 
			Medium{0}, 
			Large{0}
		{}

		BlockAllocator(BlockAllocator&) = delete;
		BlockAllocator& operator = (const BlockAllocator&) = delete;


		void Init( BlockAllocator_desc& in )
		{
			Small	= in.SmallBlock;
			Medium	= in.MediumBlock;
			Large	= in.LargeBlock;

			SmallBlockAlloc.Initialise	(in.SmallBlock,		(byte*)::_aligned_malloc(Small,		0x40));
			MediumBlockAlloc.Initialise	(in.MediumBlock,	(byte*)::_aligned_malloc(Medium,	0x40));
			LargeBlockAlloc.Initialise	(in.LargeBlock,		(byte*)::_aligned_malloc(Large,		0x40));

			new(&AllocatorInterface) iBlockAllocator(this);
		}

		byte* malloc(const size_t size, bool MarkAligned = false, bool MarkDebugMetaData = false)
		{
			std::unique_lock ul{ mu };

			byte* ret = nullptr;

			if (size <= SmallBlockAllocator::MaxAllocationSize())
				ret = SmallBlockAlloc.malloc(size, MarkAligned);
			if (size <=  MediumBlockAllocator::MaxBlockSize() && !ret)
				ret = MediumBlockAlloc.malloc(size, MarkAligned, MarkDebugMetaData);
			if (!ret)
				ret = LargeBlockAlloc.malloc(size, MarkAligned);

			if (ret == nullptr) {
				throw std::bad_alloc();
				FK_ASSERT(false, "BAD ALLOC!");
			}

			return ret;
		}

		// Debug String Must be below 64 Bytes
		byte* malloc_debug(const size_t size, const char* Debug, size_t DebugSize, bool Aligned)
		{
			if (Debug != nullptr && DebugSize != 0)
			{
				FK_LOG_ERROR("Invalid Debug Section Header passed into allocator!");
				throw std::invalid_argument("Invalid Debug Section Header passed into allocator!");
			}

			byte* ret = nullptr;
			const size_t MetaDataSectionSize = Aligned ? 0x40 : 0x00;

			if (size <= SmallBlockAllocator::MaxAllocationSize())
				ret = (byte*)_aligned_malloc(size + MetaDataSectionSize, 0x40);
			if (size <=  MediumBlockAllocator::MaxBlockSize() && !ret)
				ret = (byte*)_aligned_malloc(size + MetaDataSectionSize, 0x40, true);
			if (!ret)
				ret = (byte*)_aligned_malloc(size + MetaDataSectionSize, 0x40);

			if (	
				size > SmallBlockAllocator::MaxAllocationSize() && 
				size < MediumBlockAllocator::MaxBlockSize())
			{
				auto DebugSectionSize	= (DebugSize < MetaDataSectionSize ? DebugSize : MetaDataSectionSize);
				auto DebugStringLen		= strlen("DEBUG ALLOCATION");
				strncpy_s(reinterpret_cast<char*>(ret), DebugStringLen, "DEBUG ALLOCATION", DebugSectionSize);
			}

			return ret + MetaDataSectionSize;
		}

		char*	_aligned_malloc(size_t s, size_t alignment = 0x10, bool MarkDebugMetaData = false)
		{
			const char* NewBuffer		= (char*)malloc(s + alignment, true, MarkDebugMetaData);
			const size_t alignoffset	= (size_t)(NewBuffer) % alignment;
			const size_t Offset			= alignment - alignoffset;

			return (char*)(NewBuffer + Offset);
		}
		
		void free(void* _ptr)
		{
			std::unique_lock ul(mu);

			if (InSmallRange(reinterpret_cast<byte*>(_ptr)) && false)
				SmallBlockAlloc.free(reinterpret_cast<void*>(_ptr));
			else if (InMediumRange(reinterpret_cast<byte*>(_ptr)))
				MediumBlockAlloc.free(reinterpret_cast<void*>(_ptr));
			else if (InLargeRange(reinterpret_cast<byte*>(_ptr)))
				LargeBlockAlloc.free(reinterpret_cast<void*>(_ptr));
		}

		template<typename TY>
		void Delete(TY* _ptr)
		{
			_ptr->~TY();
			free(_ptr);
		}

		void _aligned_free(void* _ptr)
		{
			std::unique_lock ul(mu);

			if (InSmallRange((byte*)_ptr) && false)
				SmallBlockAlloc._aligned_free(_ptr);
			else if (InMediumRange(static_cast<byte*>(_ptr)))
				MediumBlockAlloc._aligned_free(_ptr);
			else if (InLargeRange(static_cast<byte*>(_ptr)))
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
		T& allocate_aligned(PARAM_TY&& ... Params)
		{
			auto* mem = _aligned_malloc(sizeof(T) + a, a);

			auto* t = new (mem) T(std::forward<PARAM_TY>(Params)...);
			return *t;
		}

		template<typename T>
		T& allocate()
		{
			auto mem = malloc(sizeof(T));

			auto t = new (mem) T();
			return *t;
		}

		template<typename T, typename ... PARAM_TY>
		T& allocate(PARAM_TY&& ... Params)
		{
			auto mem = malloc(sizeof(T));

			auto t = new (mem) T(std::forward<PARAM_TY>(Params)...);
			return *t;
		}

		template<typename T>
		void release_allocation(T& I)
		{
			I.~T();
			free(&I);
		}

		SmallBlockAllocator		SmallBlockAlloc;
		MediumBlockAllocator	MediumBlockAlloc;
		LargeBlockAllocator		LargeBlockAlloc;
		std::mutex				mu;

		char*	Buffer_ptr;
		size_t	Small, Medium, Large;

		bool InSmallRange(byte* a_ptr)
		{
			byte* bottom = (byte*)(SmallBlockAlloc.Blocks);
			byte* top    = ((byte*)SmallBlockAlloc.Blocks) + Small;

			return (bottom <= a_ptr) && (a_ptr < top) && false;
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
			explicit iBlockAllocator(BlockAllocator* parent = nullptr) noexcept : ParentAllocator(parent){}

			void* malloc(size_t size) override
			{
				return ParentAllocator->malloc(size);
			}

			void free(void* _ptr) override
			{
				ParentAllocator->free(_ptr);
			}

			void* _aligned_malloc(size_t size, size_t A) override
			{
				return ParentAllocator->_aligned_malloc(size, A);
			}

			void _aligned_free(void* _ptr) override
			{
				ParentAllocator->_aligned_free(_ptr);
			}

			void* malloc_Debug(size_t n, const char* MD, size_t MDSectionSize) override
			{
				return ParentAllocator->malloc_debug(n, MD, MDSectionSize, true);
			}


			BlockAllocator* ParentAllocator;

			operator iAllocator* ()	{ return this; }
		}AllocatorInterface;

		operator iAllocator* ()
		{ 
			return &AllocatorInterface; 
		}

        operator iAllocator& ()
        {
            return AllocatorInterface;
        }
	};


	/************************************************************************************************/


	FLEXKITAPI void		PrintBlockStatus	(FlexKit::BlockAllocator* BlockAlloc);
	FLEXKITAPI bool		LoadFileIntoBuffer	(const char* strLoc, byte* out, size_t strlenmax, bool textfile = true);
	FLEXKITAPI size_t	GetFileSize			(const char* strLoc);
	FLEXKITAPI size_t	GetLineToBuffer		(const char* Buffer, size_t position, char* out, size_t OutBuffSize);


	/************************************************************************************************/


	template<typename TY, typename DELETER_FN>
	class Shared_ref
	{
	public:
		Shared_ref() = delete;

		constexpr Shared_ref(TY& IN_reference, DELETER_FN&& IN_deleter_fn, iAllocator* IN_allocator) noexcept :
			allocator	{ IN_allocator									},
			counter_ref	{ IN_allocator->allocate<std::atomic_int>(1)	},
			deleter		{ IN_deleter_fn									},
			reference	{ IN_reference									}	{}


		~Shared_ref()
		{
			Release();
		}


		constexpr Shared_ref(const Shared_ref& r_ref) noexcept :
			allocator	{ r_ref.allocator	},
			counter_ref { r_ref.counter_ref	},
			deleter		{ r_ref.deleter		},
			reference	{ r_ref.reference	}
		{
			AddRef();
		}

		Shared_ref& operator = (Shared_ref& rhs)	= delete;
		Shared_ref& operator = (Shared_ref&& rhs)	= delete;

		bool operator == (Shared_ref& rhs) const noexcept
		{
			return reference == rhs.Get();
		}

		template<typename ... TY_Args>
		decltype(auto) operator ()(TY_Args ... args) requires(std::is_invocable_v<TY, TY_Args>)
		{
			return Get()(std::forward<TY_Args>(args)...);
		}

		operator TY& ()	noexcept { return reference; }
		TY&		Get()		noexcept { return reference; }

		TY*		ptr()	    noexcept { return &reference; }

		void AddRef()
		{
#ifdef _DEBUG
			if (counter_ref < 0)
				throw(std::runtime_error("invalid reference detected!")); // dangling reference detected
#endif
			counter_ref++;
		}

		void Release()
		{
			counter_ref--;

			if (counter_ref == 0)
			{
				deleter(&reference);
				counter_ref.store((int)-1);
				allocator->release(&counter_ref);
			}
		}

	protected:
		DELETER_FN			deleter;
		iAllocator*			allocator;
		TY&				    reference;
		std::atomic_int&	counter_ref;
	};


	/************************************************************************************************/


	template<typename REF_TY, typename ... TY_ARGS>
	auto MakeSharedRef(iAllocator* allocator, TY_ARGS ... constructor_args)
	{
		auto deleter = [allocator](REF_TY* ptr)
		{
			ptr->~REF_TY();
			allocator->free(ptr);
		};

		return Shared_ref<REF_TY, decltype(deleter)>(allocator->allocate<REF_TY>(std::forward<TY_ARGS>(constructor_args)...), std::move(deleter), allocator);
	}


	/************************************************************************************************/


	template<typename TY, typename ... TY_ARGS>
	auto MakeSharedPtr(iAllocator* allocator, TY_ARGS ... constructor_args)
	{
		return shared_ptr<TY>(
				&allocator->allocate_aligned<TY>(std::forward<TY_ARGS>(constructor_args)..., 
					[=](TY* _ptr)
					{
						allocator->free(_ptr);
					}));
	}


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


	template<typename TY>
	TY& MakeHeapCopy(const TY& data, iAllocator* allocator)
	{
		return allocator->allocate_aligned<TY>(data);
	};


	/************************************************************************************************/

}

#endif
