#pragma once

#include "BuildSettings.hpp"
#include "Logging.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <type_traits>

namespace FlexKit
{	/************************************************************************************************/

		
	constexpr size_t AlignedSize(const size_t unalignedSize, const size_t alignment = 256)
	{
		const auto mask             = alignment - 1;
		const auto offset           = unalignedSize & mask;
		const auto adjustedOffset   = offset != 0 ? alignment - offset : 0;

		return unalignedSize + adjustedOffset;
	}


	template<typename TY>
	constexpr size_t AlignedSize()
	{
		return AlignedSize(sizeof(TY));
	}


	constexpr auto Align(size_t x, const size_t alignment)
	{
		size_t adjustment = alignment - x % alignment;
		adjustment = adjustment == alignment ? 0 : adjustment;

		return x += adjustment;
	}


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
			if (_ptr == nullptr)
				return;

			_ptr->~T();
			free(_ptr);
		}

		template<typename T>
		void release(T& _ref)
		{
			std::destroy_at(std::addressof(_ref));
			free(&_ref);
		}

		operator iAllocator* () { return this; }

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
#ifdef WIN32
			return ::_aligned_malloc(n, A);
#else
			return aligned_alloc(A, AlignedSize(n, A));
#endif
		}

		void  _aligned_free(void* _ptr) override
		{
#ifdef WIN32
			::_aligned_free(_ptr);
#endif
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
	class STLAllocatorAdapter
	{
	public:
		using size_type         = size_t;
		using difference_type   = std::ptrdiff_t;
		using value_type        = TY;

		STLAllocatorAdapter(iAllocator* IN_allocator) noexcept : allocator{ IN_allocator } {}
		STLAllocatorAdapter(iAllocator& IN_allocator) noexcept : allocator{ IN_allocator } {}

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

		std::size_t MaxAllocationSize() const noexcept
		{
			return 0xffffffffffffffff;
		}

		bool operator==(const STLAllocatorAdapter& rhs) const noexcept
		{
			return rhs.allocator == allocator;
		}

		bool operator!=(const STLAllocatorAdapter& rhs) const noexcept
		{
			return !(*this == rhs);
		}

		iAllocator* allocator;
	};

        
	/************************************************************************************************/


	class StackAllocator
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

			Init((std::byte*)allocator->_aligned_malloc(bufferSize, 16), bufferSize);
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

		void	Init				(std::byte* memory, size_t);
		void*	malloc				(size_t s);
		void*	_aligned_malloc		(size_t s, size_t alignement = 0x10);
		void	clear				();

		void*   buffer() { return Buffer; }

		operator iAllocator* () { return &AllocatorInterface; }
		operator iAllocator& () { return  AllocatorInterface; }
	private:

		size_t		used	= 0;
		size_t		size	= 0;
		std::byte*  Buffer	= 0;


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

		static int MaxAllocationSize() { return Block::BlockSize; }

		void Initialise(size_t BufferSize, std::byte* Buffer)// Size in Bytes
		{
			size_t AllocationFootPrint = sizeof(Block);
			Size = BufferSize / AllocationFootPrint;
			Blocks = reinterpret_cast<Block*>(Buffer);
			allocated = 0;
#ifdef _DEBUG
			for (size_t itr = 0; itr < Size; ++itr)
			{
				Blocks[itr].BlockFull = false;
				for (size_t itr2 = 0; itr2 < Block::BlockSize; ++itr2)
					Blocks[itr].state[itr2] = Block::Free;
			}
#endif
		}

		// TODO: maybe Multi-Thread?
		std::byte* malloc(size_t, bool Aligned = false)
		{
			for (size_t itr = 0; itr < Size; ++itr)
			{
				if (!Blocks[itr].BlockFull)
				{
					for (size_t itr2 = 0; itr2 < Block::BlockCount; ++itr2)
					{
						auto state = Blocks[itr].state[itr2];
						if (state == Block::Free)
						{
							Blocks[itr].state[itr2] = (Block::Allocated || Aligned) ? Block::Aligned : 0;
							allocated++;
							return (std::byte*)&Blocks[itr].data[itr2];
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
#ifdef _DEBUG
			FK_ASSERT(Blocks[BlockID].state[SBlockID] != Block::Free);
#endif
			Blocks[BlockID].state[SBlockID] = Block::Free;
			Blocks[BlockID].BlockFull = false;

			allocated--;
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
				std::byte v[BlockSize];
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
		size_t allocated;
	};


	/************************************************************************************************/
	// 2048 Byte Allocator

	struct MediumBlockAllocator
	{
		void Initialise(size_t byteSize, std::byte* Buffer);

		// TODO: maybe Multi-Thread?
		std::byte* malloc(size_t size, bool ALIGNED = false, bool DebugMetaData = false);


		static size_t MaxBlockSize();

		void free(void* _ptr);
		void _aligned_free(void* _ptr);

		struct Block
		{
			std::byte data[2048];
		}*blocks;

		struct BlockData
		{
			enum Flags : uint8_t
			{
				Free		= 0x00,
				Allocated	= 0x01,
				Aligned		= 0x02,
				DebugMD		= 0x04,
			};
			uint8_t state;
		}*blockTable;

		size_t blockCount;
		size_t blocksAllocated;
	};


	/************************************************************************************************/
	// 1 MB MultiBlock Allocator

	struct LargeBlockAllocator
	{
		void Initialise(size_t byteSize, std::byte* Buffer);
		
		std::byte* malloc(size_t requestsize, bool aligned = false); // TODO: maybe add thread safety?

		void free(void* _ptr);
		void _aligned_free(void* _ptr);
		void Collapse(size_t block = 0);

		struct Block
		{
			std::byte data[KILOBYTE * 128];
		}*Blocks;

		struct BlockData
		{
			enum Flags : uint16_t
			{
				Free		= 0x00,
				Allocated	= 0x01,
				UNUSED		= 0x02,
				Aligned		= 0x04,
				Debug		= 0x08
			}state;
			uint16_t Parent;
			uint16_t AllocationSize;
			std::byte Padding_2[0x40 - 0x06]; // To Put Data Blocks on 64byte Lines for Multi-Threading
		}*BlockTable;

		size_t Size;
		size_t allocatedBlockCount;
	};


	struct BlockAllocator_desc
	{
		std::byte*	_ptr = nullptr;
		size_t		PoolSize;

		size_t SmallBlock;
		size_t MediumBlock;
		size_t LargeBlock;
	};


	struct BlockAllocatorStats
	{
		size_t smallBlocksAllocated		= -1;
		size_t totalSmallBlocks			= -1;

		size_t mediumBlocksAllocated	= -1;
		size_t totalMediumBlocks		= -1;

		size_t largeBlocksAllocated		= -1;
		size_t totalLargeBlocks			= -1;
	};


	struct BlockAllocator
	{
		BlockAllocator() noexcept;

		BlockAllocator(BlockAllocator&) = delete;
		BlockAllocator& operator = (const BlockAllocator&) = delete;


		void Init(BlockAllocator_desc& in);

		std::byte* malloc			(const size_t size, bool MarkAligned = false, bool MarkDebugMetaData = false);

		// Debug String Must be below 64 Bytes
		std::byte* malloc_debug		(const size_t size, const char* Debug, size_t DebugSize, bool Aligned);
		std::byte* _aligned_malloc	(size_t s, size_t alignment = 0x10, bool MarkDebugMetaData = false);
		

		template<typename TY>
		void Delete(TY* _ptr)
		{
			if (!_ptr)
				return;

			_ptr->~TY();
			free(_ptr);
		}

		void free(void* _ptr);
		void _aligned_free(void* _ptr);

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

		SmallBlockAllocator		smallBlockAlloc;
		MediumBlockAllocator	mediumBlockAlloc;
		LargeBlockAllocator		largeBlockAlloc;
		std::mutex				mu;

		std::byte* buffer_ptr = nullptr;
		uint64_t smallBufferSize;
		uint64_t mediumBufferSize;
		uint64_t largeBufferSize;

		bool InSmallRange(std::byte* a_ptr);
		bool InMediumRange(std::byte* a_ptr);
		bool InLargeRange(std::byte* a_ptr);

		BlockAllocatorStats GetStats() const;

		operator iAllocator* ();
		operator iAllocator& ();

		struct iBlockAllocator : public iAllocator
		{
			explicit iBlockAllocator(BlockAllocator* parent = nullptr) noexcept : ParentAllocator(parent){}

			void*	malloc(size_t size) override;
			void	free(void* _ptr) override;
			void*	_aligned_malloc(size_t size, size_t A) override;
			void	_aligned_free(void* _ptr) override;
			void*	malloc_Debug(size_t n, const char* MD, size_t MDSectionSize) override;
			
		    BlockAllocator* ParentAllocator;

			operator iAllocator* ()	{ return this; }
		}	AllocatorInterface;
	};


	/************************************************************************************************/

	void	PrintBlockStatus	(FlexKit::BlockAllocator* BlockAlloc);
	bool	LoadFileIntoBuffer	(const char* strLoc, std::byte* out, size_t strlenmax, bool textfile = true);
	size_t	GetFileSize			(const char* strLoc);
	size_t	GetLineToBuffer		(const char* Buffer, size_t position, char* out, size_t OutBuffSize);


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
		decltype(auto) operator ()(TY_Args ... args) requires(std::is_invocable_v<TY, TY_Args...>)
		{
			return Get()(std::forward<TY_Args>(args)...);
		}

		operator TY& ()		noexcept { return reference; }
		TY&		Get()		noexcept { return reference; }
		TY*		ptr()		noexcept { return &reference; }

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
				allocator->release(counter_ref);
			}
		}

		[[nodiscard]] int GetRefCount() const noexcept { return counter_ref.load(std::memory_order_relaxed); }

	protected:
		DELETER_FN			deleter;
		iAllocator*			allocator;
		TY&					reference;
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
		return std::shared_ptr<TY>(
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


	inline uint64_t hash_DJB2(char* str)
	{
		unsigned long hash = 5381;

		for (int c = *str; c != '\0' ; str++)
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


	template<typename TY>
	struct NonAliasingPointer
	{
		explicit NonAliasingPointer(TY* IN_ptr) : _ptr{ IN_ptr } {}

		TY* __restrict _ptr;

		TY* operator -> ()	{ return _ptr; }
		TY& operator * ()	{ return *_ptr; }
	};


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
