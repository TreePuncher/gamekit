#pragma once

#include "buildsettings.h"
#include "memoryutilities.h"
#include "static_vector.h"
#include "MathUtils.h"

#if USING(USESTL)

#include <atomic>
#include <deque>
#include <list>
#include <new>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <stdint.h>
#include <utility>
#include <vector>
#include <condition_variable>
#include <random>
#include <type_traits>
#include <concepts>


namespace FlexKit
{   /************************************************************************************************/


	// Thank you cppReference.com
	FLEXKITAPI template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	FLEXKITAPI template<class... Ts> overloaded(Ts...)->overloaded<Ts...>; // not needed as of C++20


	/************************************************************************************************/


	template<typename Ty, size_t SIZE = 16>
	struct RingBuffer
	{
		RingBuffer()
		{
			head = 0;
			end = 0;
			count = 0;
		}

		void push_back(const Ty&& in)
		{
			_t[end++] = in;
			end = end % SIZE;
			++count;
			count = count % SIZE;
			if (head == end)
				head++;
		}

		Ty pop_front()
		{
			size_t i = end;
			if (count) {
				--count;
				count = count % SIZE;

				i = head++;
				head = head % SIZE;
			}
			return _t[i];
		}

		Ty& back() {
			return _t[end];
		}

		Ty& front() {
			return _t[head];
		}

		Ty& operator[] (uint32_t i) { return _t[i]; }

		void clear() {
			count = 0;
			head = 0;
			end = 0;
		}

		uint32_t size() {
			return count;
		}

		Ty		_t[SIZE];
		size_t	head;
		size_t	end;
		size_t	count;
	};


	/************************************************************************************************/


	template<typename TY, size_t Size>
	struct alignas(std::alignment_of_v<TY>) _VectorBuffer
	{
		char buffer[sizeof(TY) * Size];

		constexpr size_t size() const { return Size; }
		TY* GetBuffer()			const { return (TY*)&buffer; }
	};

	template<typename TY>
	struct _VectorBuffer<TY, 0>
	{
		constexpr size_t size() const { return 0; }
		TY* GetBuffer()			const { return nullptr; }
	};

	// NOTE: Doesn't call destructors automatically, but does free held memory!
	template<typename Ty, size_t InternalBufferSize = 0, typename TYSize = size_t>
	struct Vector
	{
		using value_t = Ty;
		typedef Vector<Ty, InternalBufferSize, TYSize> THISTYPE;

		typedef Ty*			Iterator;
		typedef const Ty*	Iterator_const;

		Vector(
			iAllocator*		Alloc = nullptr,
			const TYSize	initialReservation = 0) :
				Allocator	{ Alloc },
				Max			{ initialReservation },
				Size		{ 0 }
		{
			if (initialReservation > internalBuffer.size())
			{
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * initialReservation);
				FK_ASSERT(NewMem);

				A = NewMem;
				Max = initialReservation;
			}
			else
				Max = (TYSize)internalBuffer.size();
		}


		template<typename TY_Initial>
		Vector(
			iAllocator*			alloc,
			const TYSize		initialSize,
			const TY_Initial&	initial_V) :
				Allocator	{ alloc },
				Max			{ FlexKit::Max(initialSize, internalBuffer.size()) },
				Size		{ 0 }
		{
			if (initialSize > 0)
			{
				reserve(initialSize);

				for (size_t itr = 0; itr < initialSize; ++itr)
					emplace_back(initial_V);
			}
		}


		template<typename TY_Initial>
		explicit Vector(
			iAllocator&			Alloc,
			const size_t		InitialSize,
			const TY_Initial&	Initial_V) :
				Allocator	{ &Alloc },
				Max			{ (TYSize)InitialSize },
				Size		{ 0 },
				A			{ internalBuffer.GetBuffer() }
		{
			if (InitialSize > 0)
			{
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * InitialSize);
				FK_ASSERT(NewMem);

				A   = NewMem;
				Max = InitialSize;

				for (size_t itr = 0; itr < InitialSize; ++itr)
					emplace_back(Initial_V);
			}
		}


		Vector(const THISTYPE& RHS) :
			Allocator	{ RHS.Allocator },
			Max			{ (TYSize)internalBuffer.size() },
			Size		{ 0 }
		{
			if (!Allocator) Allocator = RHS.Allocator;

			clear();

			reserve(RHS.size());

			for (const auto& E : RHS)
				push_back(E);
		}

		template<size_t rhsSize, typename rhsSizeType>
		Vector(const Vector<Ty, rhsSize, rhsSizeType>& RHS) :
			Allocator	{ RHS.Allocator },
			Max			{ (TYSize)internalBuffer.size() },
			Size		{ 0 }
		{
			if (!Allocator) Allocator = RHS.Allocator;

			clear();

			reserve(RHS.size());

			for (const auto& E : RHS)
				push_back(E);
		}

		template<TYSize rhsSize, typename rhsSizeType>
		Vector(Vector<Ty, rhsSize, rhsSizeType>&& RHS) noexcept :
			Allocator	{ RHS.Allocator },
			Max			{ (TYSize)internalBuffer.size() },
			Size		{ 0 }
		{
			if constexpr(InternalBufferSize > 0 || rhsSize > 0)
			{
				if (RHS.A == RHS.internalBuffer.GetBuffer())
				{
					reserve(RHS.size());

					if constexpr (std::is_move_assignable_v<Ty>)
					{
						const TYSize end = RHS.size();
						for (TYSize I = 0; I < end; I++)
							A[I] = std::move(RHS[I]);

						Size		= RHS.size();
						RHS.Max		= (rhsSize > 0) ? (TYSize)RHS.internalBuffer.size() : 0;
						RHS.Size	= 0;

					}
					else
					{
						for (TYSize I = 0; I < RHS.size(); I++)
							emplace_back(RHS[I]);

						RHS.clear();
					}
				}
				else
				{
					A			= RHS.A;
					Max			= RHS.Max;
					Size		= RHS.Size;

					RHS.A		= RHS.internalBuffer.GetBuffer();
					RHS.Max		= (TYSize)RHS.internalBuffer.size();
					RHS.Size	= 0;
				}
			}
			else
			{
				A			= RHS.A;
				Max			= RHS.Max;
				Size		= RHS.Size;

				RHS.A		= RHS.internalBuffer.GetBuffer();
				RHS.Max		= RHS.internalBuffer.size();
				RHS.Size	= 0;
			}
		}


		inline ~Vector()
		{
			if (A && Allocator && A != internalBuffer.GetBuffer())
				Allocator->_aligned_free(A);

			A			= nullptr;
			Allocator	= nullptr;
			Max			= 0;
			Size		= 0;
		}


		inline			Ty& operator [](size_t index) noexcept
		{
			return A[index];
		}
		inline const	Ty& operator [](size_t index) const noexcept    { return A[index]; }


		/************************************************************************************************/


		template<TYSize rhsSize, typename rhsSizeType>
		THISTYPE& AssignmentOP(const Vector<Ty, rhsSize, rhsSizeType>& RHS)
		{
			if (!Allocator) Allocator = RHS.Allocator;

			if (RHS.A == RHS.internalBuffer.GetBuffer() && Allocator == nullptr)
			{
				if (RHS.size() > internalBuffer.size())
				{
					throw std::runtime_error{ "Impossible Copy. Vector has no allocator, attempted copy into a internal buffer if too small size" };
				}
				else if(RHS.size() == 0)
				{
					Size	= 0;
					A		= 0;
					Max		= 0;
					return *this;
				}
			}

			clear();

			reserve(RHS.size());

			for (const auto& E : RHS)
				push_back(E);

			return *this;
		}

		THISTYPE& operator =(const THISTYPE& RHS) { return AssignmentOP(RHS); }

		template<TYSize rhsSize, typename rhsSizeType>
		THISTYPE& operator =(const Vector<Ty, rhsSize, rhsSizeType>& RHS) { return AssignmentOP(RHS); }


		/************************************************************************************************/


		THISTYPE& operator =(std::initializer_list<Ty>&& ilist)
		{
			clear();

			if (ilist.size() > size())
				reserve(ilist.size());

			for (auto&& i : ilist)
				push_back(std::move(i));

			return *this;
		}


		/************************************************************************************************/

		template<typename ThisType, TYSize rhsSize, typename rhsSizeType>
		auto& operator =(this ThisType& self, Vector<Ty, rhsSize, rhsSizeType>&& RHS)
		{
			self.Release();
			self.Allocator = RHS.Allocator;

			if constexpr(InternalBufferSize > 0 || rhsSize > 0)
			{
				if (RHS.A == RHS.internalBuffer.GetBuffer())
				{
					self.reserve(RHS.size());

					if constexpr (std::is_move_assignable_v<Ty>)
					{
						const TYSize end = RHS.size();
						for (TYSize I = 0; I < end; I++)
							self.A[I] = std::move(RHS[I]);

						self.Size	= RHS.size();
						RHS.Max		= (rhsSize > 0) ? (TYSize)RHS.internalBuffer.size() : 0;
						RHS.Size	= 0;

					}
					else
					{
						for (TYSize I = 0; I < RHS.size(); I++)
							self.emplace_back(RHS[I]);

						RHS.clear();
					}
				}
				else
				{
					self.A		= RHS.A;
					self.Max	= RHS.Max;
					self.Size	= RHS.Size;

					RHS.A		= RHS.internalBuffer.GetBuffer();
					RHS.Max		= (TYSize)RHS.internalBuffer.size();
					RHS.Size	= 0;
				}
			}
			else
			{
				self.A		= RHS.A;
				self.Max	= RHS.Max;
				self.Size	= RHS.Size;

				RHS.A		= RHS.internalBuffer.GetBuffer();
				RHS.Max		= (TYSize)RHS.internalBuffer.size();
				RHS.Size	= 0;
			}

			return self;
		}


		/************************************************************************************************/

		template<typename thisType, TYSize rhsSize, typename rhsSizeType>
		auto& operator +=(this thisType&  self, const Vector<Ty, rhsSize, rhsSizeType>& RHS)
		{
			self.reserve(self.size() + RHS.size());

			for (auto I : RHS)
				self.push_back(I);

			return self;
		}


		/************************************************************************************************/


		Ty PopVect()
		{
			FK_ASSERT(Size >= 1);
			auto temp = A[--Size];
			A[Size].~Ty();
			return temp;
		}


		/************************************************************************************************/


		Ty& front()
		{
			FK_ASSERT(Size > 0);
			return A[0];
		}

		const Ty front() const
		{
			FK_ASSERT(Size > 0);
			return A[0];
		}


		/************************************************************************************************/


		Ty& back() 
		{
			FK_ASSERT(Size > 0);
			return A[Size - 1];
		}

		Ty back() const
		{
			FK_ASSERT(Size > 0);
			return A[Size - 1];
		}

		/************************************************************************************************/


		void resize(const TYSize newSize)
		{
			if (Max < newSize)
				reserve(newSize);

			if (newSize > size()) {
				auto I = newSize - size();
				while (I--)
					push_back(Ty());
			}
			else if (newSize < size())
			{
				auto I = size() - newSize;
				while (I--)
					pop_back();
			}
		}


		/************************************************************************************************/


		TYSize push_back(const Ty& in)
		{
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				auto NewSize = ((Max < 1) ? 2 : (2 * Max));

#if USING(DEBUGMEMORY)
				Ty* NewMem = (Ty*)Allocator->malloc_Debug(sizeof(Ty) * NewSize, "TEST", 4);
#else
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
#endif
				const TYSize End = Size;
				for (TYSize itr = 0; itr < End; ++itr)
					new(NewMem + itr) Ty();

#ifdef _DEBUG
				FK_ASSERT(NewMem != nullptr);
				if (Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A)
				{
					TYSize itr = 0;
					TYSize End = Size;
					for (; itr < End; ++itr)
						NewMem[itr] = std::move(A[itr]);

					if(A != internalBuffer.GetBuffer())
						Allocator->_aligned_free(A);
				}

				A   = NewMem;
				Max = NewSize;
			}

			const TYSize idx = Size++;
			new(A + idx) Ty{ in }; //

			return idx;
		}


		/************************************************************************************************/

		TYSize push_back(Ty&& in)
		{
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				auto NewSize = ((Max < 1) ? 2 : (2 * Max));

#if USING(DEBUGMEMORY)
				Ty* NewMem = (Ty*)Allocator->malloc_Debug(sizeof(Ty) * NewSize, "TEST", 4);
#else
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
#endif
				TYSize itr = 0;
				TYSize End = Size;
				for (; itr < End; ++itr)
					new(NewMem + itr) Ty();

#ifdef _DEBUG
				FK_ASSERT(NewMem != nullptr);
				if (Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A)
				{
					TYSize itr = 0;
					TYSize End = Size;
					for (; itr < End; ++itr)
						NewMem[itr] = std::move(A[itr]);

					if(A != internalBuffer.GetBuffer())
						Allocator->_aligned_free(A);
				}

				A   = NewMem;
				Max = NewSize;
			}

			const TYSize idx = Size++;
			new(A + idx) Ty{ std::move(in) }; //

			return idx;
		}

		/************************************************************************************************/

		template<typename ... ARGS_t>
		TYSize emplace_back(ARGS_t&& ... in)
		{
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				const auto NewSize = ((Max < 1) ? 2 : (2 * Max));
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
#ifdef _DEBUG
				FK_ASSERT(NewMem);
				if (Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A)
				{
					size_t itr = 0;
					const size_t End = Size;
					for (; itr < End; ++itr)
						new(NewMem + itr) Ty{ std::move(A[itr]) };

					if (A != internalBuffer.GetBuffer())
						Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}

			const TYSize idx = Size++;

			new(A + idx) Ty{ std::forward<ARGS_t>(in)... };

			return idx;
		}


		/************************************************************************************************/


		void insert(Iterator I, const Ty& in)
		{
			if (!size())
			{
				push_back(in);
				return;
			}

			auto II = end() - 1;

			push_back(back());

			while (II > I)
				*(II + 1) = *II;

			*I = in;
		}


		/************************************************************************************************/


		// Order Not Preserved
		void remove_unstable(Iterator I)
		{
			if (I == end())
				return;

			*I = back();
			pop_back();
		}


		/************************************************************************************************/


		void erase(const Iterator first, const Iterator last)
		{
			for (auto itr = first; itr != last; ++itr)
				remove_stable(first);
		}

		/************************************************************************************************/


		void remove_stable(Iterator I)
		{
			if (I == end())
				return;

			while (I != end() && (I + 1) != end())
			{
				*I = std::move(*(I + 1));
				++I;
			}
			pop_back();
		}


		/************************************************************************************************/


		Ty pop_back()
		{
			auto Temp = std::move(back());
			A[--Size].~Ty();

			return Temp;
		}


		/************************************************************************************************/


		void reserve(size_t NewSize)
		{
			if (!NewSize)
				return;

			if (!A || Max < NewSize)
			{// Increase Size
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
				FK_ASSERT(NewMem);

				if (A)
				{
					TYSize itr = 0;
					TYSize End = Size;
					for (; itr < End; ++itr)
						new(NewMem + itr) Ty{ std::move(A[itr]) }; // move if Possible

					if (A != internalBuffer.GetBuffer())
						Allocator->_aligned_free(A);
				}

				A   = NewMem;
				Max = (TYSize)NewSize;
			}
		}


		/************************************************************************************************/


		Ty* data() noexcept
		{
			return A;
		}


		const Ty* data() const noexcept
		{
			return A;
		}


		/************************************************************************************************/


		void clear()
		{
			for (TYSize i = 0; i < size(); ++i)
				(A + i)->~Ty();

			Size = 0;
		}

		bool empty() const noexcept
		{
			return Size == 0;
		}


		/************************************************************************************************/


		void Release()
		{
			clear();

			if (A && Allocator && A != internalBuffer.GetBuffer())
				Allocator->_aligned_free(A);

			A			= internalBuffer.GetBuffer();
			Allocator	= nullptr;
			Max			= (TYSize)internalBuffer.size();
			Size		= 0;
		}


		/************************************************************************************************/


		Ty& at(size_t index)				{ return A[index]; }
		const Ty& at(size_t index) const	{ return A[index]; }


		Iterator begin() { return A; }
		Iterator end()	 { return A + Size; }

		Iterator_const begin()	const { return A; }
		Iterator_const end()	const { return A + Size; }
		TYSize size()			const { return Size; }
		TYSize ByteSize()		const { return Size * sizeof(Ty); }


		Vector Copy(iAllocator& destination) const
		{
			Vector c{ &destination };

			for (const auto& e : *this)
				c.emplace_back(e);

			return c;
		}

		/************************************************************************************************/


		Ty*	A					= internalBuffer.GetBuffer();

		TYSize Size				= 0;
		TYSize Max				= internalBuffer.size();

		iAllocator* Allocator	= 0;

		[[no_unique_address]] _VectorBuffer<Ty, InternalBufferSize> internalBuffer;


	};	/************************************************************************************************/


	template< typename Ty_Get, template<typename Ty, typename... Ty_V> class TC, typename Ty_, typename... TV2> Ty_Get GetByType(TC<Ty_, TV2...>& in) { return in.GetByType<Ty_Get>(); }

	template<typename Ty_1, typename Ty_2>
	struct Pair
	{
		typedef Pair<Ty_1, Ty_2> ThisType;

		template<typename Ty_Get, typename Ty_V>	static  inline const auto& _GetByType(const Pair<Ty_Get, Ty_V>& in) { return in.V1; }
		template<typename Ty_Get, typename Ty_V>	static  inline const auto& _GetByType(const Pair<Ty_V, Ty_Get>& in) { return in.V2; }

		template<typename Ty_Get, typename Ty_V>	static  inline Ty_Get& _GetByType(Pair<Ty_Get, Ty_V>&     in)   { return in.V1; }
		template<typename Ty_Get, typename Ty_V>	static  inline Ty_Get& _GetByType(Pair<Ty_V, Ty_Get>&     in)   { return in.V2; }
		template<typename Ty_Get>					static  inline auto&   _GetByType(Pair<Ty_Get, Ty_Get>&	in)     { static_assert(!std::is_same_v<Ty_1, Ty_2>, "NON_UNIQUE TYPES IN Pair!");  return in.V2; }

		template<typename Ty_Get>							inline auto&  GetByType() const { return _GetByType<Ty_Get>(*this); }

		template<typename Ty_Assign> ThisType operator = (const Ty_Assign& in) { Ty_Assign& thisVar = GetByType<Ty_Assign>(); thisVar = in; return *this; }

		explicit operator bool() { return GetByType<bool>(); }
		template<typename Ty_1>	operator Ty_1() const { return GetByType<Ty_1>(); }
		template<typename Ty_1>	operator Ty_1() { return GetByType<Ty_1>(); }

		template<size_t index>	auto& Get() noexcept { static_assert(index >= 2, "Invalid Index"); }
		template<>	constexpr auto& Get<0>() noexcept { return V1; }
		template<>	constexpr auto& Get<1>() noexcept { return V2 ;}

		template<size_t index>	const auto& Get() const noexcept { static_assert(index >= 2, "Invalid Index"); }
		template<> const auto& Get<0>() const noexcept { return V1; }
		template<> const auto& Get<1>() const noexcept { return V2; }

		Ty_1 V1;
		Ty_2 V2;
	};


	/************************************************************************************************/


	template<typename TY_C, typename TY_K, typename TY_PRED>
	auto find(const TY_C& C, TY_K K, TY_PRED _Pred) noexcept
	{
		auto itr = C.begin();
		auto end = C.end();
		for (; itr != end; ++itr)
			if (_Pred(*itr, K))
				break;

		return itr;
	}


	template<typename TY_C, typename TY_PRED>
	auto find(const TY_C& C, TY_PRED _Pred) noexcept
	{
		auto itr = C.begin();
		auto end = C.end();
		for (; itr != end; ++itr)
			if (_Pred(*itr))
				return itr;

		return C.end();
	}


	template<typename TY_C, typename TY_PRED>
	auto find(TY_C& C, TY_PRED _Pred) noexcept
	{
		auto itr = C.begin();
		auto end = C.end();
		for (; itr != end; ++itr)
			if (_Pred(*itr))
				return itr;

		return C.end();
	}


	template<typename TY>
	constexpr bool TYPE_CHECK(TY A, TY B) { return true; }

	template<typename TY_1, typename TY_2>
	constexpr bool TYPE_CHECK(TY_1 A, TY_2 B) { return false; }


	template<typename TY>
	bool IsXInSet(const TY& X, std::initializer_list<TY> C)
	{
		for (auto& Ci : C)
			if (Ci == X)
				return true;

		return false;
	}


	template<typename TY_C, typename TY>
	bool IsXInSet(const TY& X, const TY_C& C)
	{
		//static_assert(TYPE_CHECK(TY(), TY_C::TYPE()), "TYPES MUST MATCH!");
		for (const auto& Ci : C)
			if (Ci == X)
				return true;

		return false;
	}

	template<typename TY_C, typename TY, typename PRED>
	bool IsXInSet(const TY& X, const TY_C& C, PRED Pred)
	{
		for (auto& Ci : C)
			if (Pred(X, Ci))
				return true;

		return false;
	}


	/************************************************************************************************/

	// Whole Point of this single linked list is for communication multiple Threads (Producer Consumer Queues)
	template<typename TY>
	struct SL_list
	{
		typedef SL_list<TY>		This_Type;

		SL_list(iAllocator* M = nullptr) : _allocator{ M }
		{
			FirstNode   = nullptr;
			LastNode    = nullptr;
			Count       = 0;
		}

		~SL_list()
		{
			Release();
		}

		void Release()
		{
			while (size())
				pop_front();
		}

		struct Node
		{
			Node(TY&& Initial)      : Data{ std::move(Initial) } {} // Move Constructor
			Node(const TY& Initial) : Data{ Initial } {} // Copy Constructor

			TY					Data;
			std::atomic<Node*>	Next;

			TY&	operator *()
			{
				return Data;
			}

			Node* GetNext()
			{
				return Next.load();
			}
		};

		typedef std::atomic<Node*>	Node_ptr;

		struct Iterator
		{
			Node* _ptr;
			SL_list* Container;

			Node* operator -> ()
			{
				return _ptr;
			}

			Node* Peek_Next()
			{
				return _ptr->GetNext();
			}


			bool operator == (Node* rhs)
			{
				return _ptr == rhs;
			}


			bool operator == (const Iterator& rhs)
			{
				FK_ASSERT(rhs.Container == Container);

				return (_ptr == rhs._ptr);
			}

			bool operator != (const Iterator& rhs)
			{
				FK_ASSERT(rhs.Container == Container);

				return (_ptr != rhs._ptr);
			}

			bool operator != (Node* rhs)
			{
				return _ptr != rhs;
			}

			void operator ++ (int)
			{
				_ptr = Peek_Next();
			}

			Iterator operator ++ ()
			{
				FINAL(_ptr = Peek_Next(););
				return *this;
			}

		};


		void Insert(Iterator Itr, TY&& e)
		{
			Node* NewNode = &_allocator->allocate_aligned<Node>(std::move(e));

			NullCheck(NewNode);

			Node* PrevNext  = Itr._ptr->Next;
			Itr._ptr->Next  = NewNode;
			NewNode->Next   = PrevNext;

			Itr->Container->Count++;
		}


		Iterator push_back(const TY e)
		{
			Node* NewNode = &_allocator->allocate_aligned<Node>(e);

			NullCheck(NewNode);

			if (!Count)
			{
				FirstNode.store(NewNode);
				LastNode.store(NewNode);
			}
			else
			{
				LastNode.load()->Next.store(NewNode);
				LastNode.store(NewNode);
			}

			Count++;

			return { NewNode, this };
		}


		Iterator emplace_back(TY&& e)
		{
			Node* NewNode = &_allocator->allocate_aligned<Node>(std::move(e));

			NullCheck(NewNode);

			if (!Count)
			{
				FirstNode.store(NewNode);
				LastNode.store(NewNode);
			}
			else
			{
				LastNode.load()->Next.store(NewNode);
				LastNode.store(NewNode);
			}

			Count++;

			return { NewNode, this };
		}


		auto pop_front()
		{
			FK_ASSERT(Count != 0);

			auto* First(FirstNode.load());

			if (First)
				FirstNode.store(First->Next);

			if (First->Next.load() == nullptr)
				LastNode.store(nullptr);

			FINAL(Count--; _allocator->free(First); );

			return std::move(First->Data);
		}


		TY& first()
		{
			return FirstNode.load()->Data;
		}


		Iterator begin()
		{
			FK_ASSERT(Count != 0);
			return { FirstNode, this };
		}


		Iterator end()
		{
			return{ LastNode, this };
		}


		template<typename FN>
		void For_Each(FN DoThis)
		{
			if (!Count)
				return;

			Iterator Itr = begin();

			while (size() && (Itr != nullptr))
			{
				if (!DoThis(Itr->Data))
					break;
				++Itr;
			}
		}


		size_t size()
		{
			return Count;
		}


		Node_ptr FirstNode;
		Node_ptr LastNode;

		iAllocator*         _allocator;
		std::atomic<size_t>	Count;
	};


	/************************************************************************************************/


	template<typename Ty, int SIZE = 64, int Alignment = 16>
	struct CircularBuffer
	{
		CircularBuffer() : _Head(0), _Size(0)
		{
			for (auto& e : Buffer)
				new(&e) Ty{};
		}


		~CircularBuffer()
		{
			Release();
		}

		void Release()
		{
			for (auto& Element : *this)
				Element.~Ty();

			_Head = 0;
			_Size = 0;
		}

		Ty& operator [](size_t idx)
		{
			return at(idx);
		}

		Ty  operator [](size_t idx) const
		{
			return at(idx);
		}

		Ty& at(size_t idx)
		{
			idx = (_Head - idx - 1) % SIZE;
			return Buffer[idx];
		}

		const Ty& at(size_t idx) const noexcept
		{
			idx = (_Head - idx - 1) % SIZE;
			return Buffer[idx];
		}

		bool full() const noexcept {
			return (_Size == SIZE);
		}

		bool empty() const noexcept {
			return (_Size == 0);
		}

		void clear() noexcept {
			_Size = 0;
			_Head = 0;
		}

		size_t size() const noexcept {
			return _Size;
		}

		Ty pop_front() noexcept
		{
			//if (!_Size)
			//	FK_ASSERT("BUFFER EMPTY!");


			Ty Out = std::move(front());
			_Size = FlexKit::Max(--_Size, 0);
			return std::move(Out);
		}

		Ty pop_back() noexcept
		{
			//if (!_Size)
			//	FK_ASSERT("BUFFER EMPTY!");


			Ty Out = std::move(back());
			_Size = FlexKit::Max(--_Size, 0);
			_Head = (SIZE + --_Head) % SIZE;
			return std::move(Out);
		}

		bool push_back(const Ty& Item) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Tail
				back().~Ty();

			_Size = Min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;

			return true;
		}

		bool push_back(Ty&& Item) noexcept
		{
			_Size = Min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;

			if (_Size + 1 > SIZE)// Call Destructor on Tail
				Buffer[idx].~Ty();

			new(Buffer + idx) Ty(std::move(Item));

			return true;
		}

		template<typename FN>
		bool push_back(const Ty& Item, FN callOnTail) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Tail
			{
				callOnTail(back());
				back().~Ty();
			}

			_Size = Min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;

			return true;
		}

		template<typename FN>
		bool push_back(Ty&& Item, FN callOnTail) noexcept
		{
			_Size = Min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;

			if (_Size + 1 > SIZE)// Call Destructor on Tail
			{
				callOnTail(back());
				Buffer[idx].~Ty();
			}

			new(Buffer + idx) Ty(std::move(Item));

			return true;
		}

		template<typename ... TY_ARGS>
		bool emplace_back(TY_ARGS&& ... args) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Tail
				back().~Ty();

			_Size       = Min(++_Size, SIZE);
			size_t idx  = _Head++;
			_Head       = _Head % SIZE;

			new(Buffer + idx) Ty{ std::forward<TY_ARGS>(args)... };

			return true;
		}

		template<typename ... TY>
		bool emplace_back(TY&& ... args, std::invocable<Ty&> auto&& callOnTail) noexcept
		{
			_Size       = Min(++_Size, SIZE);
			size_t idx  = _Head++;
			_Head       = _Head % SIZE;

			if (_Size + 1 > SIZE)// Call Destructor on Tail
			{
				callOnTail(back());
				Buffer[idx].~Ty();
			}

			new(Buffer + idx) Ty{ std::forward<TY>(args)... };

			return true;
		}

		void push_front(const Ty& Item) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Head
			{
				front().~Ty();
			}

			_Size = Min(++_Size, SIZE);
			const size_t idx = (SIZE + _Head - _Size) % SIZE;
			new (Buffer + idx) Ty{ Item };
		}

		template<typename FN>
		void push_front(const Ty& Item, FN callOnTail) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Head
			{
				front().~Ty();
				callOnTail(front());
			}

			_Size = Min(++_Size, SIZE);
			const size_t idx = (SIZE + _Head - _Size) % SIZE;
			new (Buffer + idx) Ty{ Item };
		}

		template<typename ... TY_args>
		void emplace_front(TY_args&& ... args) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Head
			{
				front().~Ty();
				callOnTail(front());
			}

			_Size = Min(++_Size, SIZE);
			const size_t idx = (SIZE + _Head - _Size) % SIZE;

			new (Buffer + idx) Ty{ std::forward<TY_args>(args)... };
		}

		Ty& front() noexcept
		{
			return Buffer[(SIZE + _Head - _Size) % SIZE];
		}

		Ty& back() noexcept
		{
			return Buffer[(SIZE + _Head - 1) % SIZE];
		}


		struct CircularIterator
		{
			CircularBuffer<Ty, SIZE>* Buffer;
			int					Idx;

			Ty& operator *()
			{
				return Buffer->at(Idx);
			}

			Ty* operator -> ()
			{
				return &Buffer->at(Idx);
			}

			bool operator <		(CircularIterator rhs) { return Idx < rhs.Idx; }
			bool operator ==	(CircularIterator rhs) { return Idx == rhs.Idx; }

			bool operator !=	(CircularIterator rhs) { return !(*this == rhs); }


			void Increment(int n = 1)
			{
				Idx += n;
			}

			void Decrement(int n = 1)
			{
				Idx -= n;
			}

			CircularIterator& operator + (int n) { Increment(n); return *this; }
			CircularIterator& operator - (int n) { Decrement(n); return *this; }

			CircularIterator operator ++ (int) { auto Temp = *this; Increment(); return Temp; }
			CircularIterator operator ++ () { Increment(); return (*this); }

			CircularIterator operator -- (int) { auto Temp = *this; Decrement(); return Temp; }
			CircularIterator operator -- () { Decrement(); return (*this); }
		};

		struct Const_CircularIterator
		{
			const CircularBuffer<Ty, SIZE>* Buffer;
			int					Idx;

			const Ty& operator *() const 
			{
				return Buffer->at(Idx);
			}

			const Ty* operator -> () const
			{
				return &Buffer->at(Idx);
			}

			bool operator <		(Const_CircularIterator rhs) { return Idx < rhs.Idx; }
			bool operator ==	(Const_CircularIterator rhs) { return Idx == rhs.Idx; }

			bool operator !=	(Const_CircularIterator rhs) { return !(*this == rhs); }


			void Increment(int n = 1)
			{
				Idx+= n;
			}

			void Decrement(int n = 1)
			{
				Idx-= n;
			}

			Const_CircularIterator& operator + (int n) { Increment(n); return *this; }
			Const_CircularIterator& operator - (int n) { Decrement(n); return *this; }

			Const_CircularIterator operator ++ (int) { auto Temp = *this; Increment(); return Temp; }
			Const_CircularIterator operator ++ () { Increment(); return (*this); }

			Const_CircularIterator operator -- (int) { auto Temp = *this; Decrement(); return Temp; }
			Const_CircularIterator operator -- () { Decrement(); return (*this); }
		};

		CircularIterator begin() noexcept
		{
			return{ this, 0 };
		}

		CircularIterator end() noexcept
		{
			return{ this, _Size };
		}

		Const_CircularIterator begin() const noexcept
		{
			return{ this, 0 };
		}

		Const_CircularIterator end() const noexcept
		{
			return{ this, _Size };
		}

		Ty* data() { return Buffer; }

		int _Head, _Size;
		alignas(Alignment) Ty Buffer[SIZE];
	};

	struct test
	{
		int x;
	};


	/************************************************************************************************/



	class DequeNode_MT;

	struct _Node
	{
		_Node(DequeNode_MT* IN_ptr, uint32_t IN_flag = 0) :
			_ptr{ IN_ptr },
			flag{ IN_flag } {}

		bool isLocked()
		{
			return flag != 0;
		}

		DequeNode_MT*	_ptr;
		uint32_t		flag;
		operator DequeNode_MT* () { return _ptr; }
		DequeNode_MT* operator -> () { return _ptr; }
	};


	typedef std::atomic<_Node> DEQ_Node_ptr;


	class DequeNode_MT
	{
		struct _Linkage
		{
			DequeNode_MT* prev;
			DequeNode_MT* next;
		};


	public:
		DequeNode_MT(const DequeNode_MT& E) = delete;
		DequeNode_MT& operator = (const DequeNode_MT&) = delete; // No copying allowed, this class does no memory management

		bool operator == (const DequeNode_MT& rhs) { return this == &rhs; }

		DequeNode_MT() = default;

		virtual ~DequeNode_MT() {}


		DequeNode_MT* GetNext() const
		{
			return links.load(std::memory_order_acquire).next;
		}


		DequeNode_MT* GetPrev() const
		{
			return links.load(std::memory_order_acquire).prev;
		}


		_Linkage GetLinks()
		{
			return links.load(std::memory_order_acquire);
		}

		bool TrySetNext(DequeNode_MT* _ptr)
		{
			auto current = links.load(std::memory_order_acquire);
			return links.compare_exchange_strong(current, { current.prev, _ptr }, std::memory_order_acq_rel);
		}


		bool TrySetPrev(DequeNode_MT* _ptr)
		{
			auto current = links.load(std::memory_order_acquire);
			return links.compare_exchange_strong(current, { _ptr, current.next }, std::memory_order_acq_rel);
		}


		bool CMPExchangeLinks(_Linkage newState, _Linkage expected)
		{
			return links.compare_exchange_strong(expected, newState);
		}

		void Clear()
		{
			links.store({ nullptr, nullptr }, std::memory_order_release);
		}

	private:

		std::atomic<_Linkage>	links = { { nullptr, nullptr } };
	};


	// Intrusive, Thread-Safe, Double Linked List
	// This Class assumes nodes have inherited the from DequeNode_MT 
	// Destructor does not free elements!!

	static const bool DEBUG_MUTEX_DEQUE_MT = true;

	template<typename TY>
	class Deque_MT
	{
	public:

		/************************************************************************************************/

		Deque_MT()	noexcept = default;
		~Deque_MT() noexcept = default;

		Deque_MT(const Deque_MT&) = delete;
		Deque_MT& operator =(const Deque_MT&) = delete;

		Deque_MT(Deque_MT&&) = delete;
		Deque_MT& operator =(Deque_MT&&) = delete;


		/************************************************************************************************/


		class Iterator
		{
		public:
			Iterator(DequeNode_MT* _ptr) noexcept : I{ _ptr } {}

			Iterator operator ++ (int) noexcept
			{
				auto prevI = I;
				I = I ? I->GetNext() : nullptr;
				return { prevI };
			}


			Iterator operator -- (int) noexcept
			{
				auto prevI = I;
				I = I ? I->GetPrev() : nullptr;
				return { prevI };
			}


			void operator ++ () noexcept
			{
				I = I ? I->GetNext() : nullptr;
			}


			void operator -- () noexcept
			{
				I = I ? I->GetPrev() : nullptr;
			}


			Iterator operator + (const int n) noexcept
			{
				auto itr = *this;

				for (size_t i = 0; i < n; itr++, i++);

				return itr;
			}


			Iterator operator - (const int n) noexcept
			{
				auto itr = *this;

				for (size_t i = 0; i < n; itr--, i++);

				return itr;
			}


			DequeNode_MT* GetPtr() noexcept
			{
				return I;
			}


			bool hasNext() noexcept
			{
				return I ? I->GetNext() != nullptr : false;
			}


			bool hasPrev() noexcept
			{
				return I ? I->GetPrev() != nullptr : false;
			}


			bool operator !=(const Iterator& rhs) const noexcept { return I != rhs.I; }
			bool operator ==(const Iterator& rhs) const noexcept { return I == rhs.I; }

			TY& operator* ()	noexcept { return *static_cast<TY*>(I); }
			TY* operator-> ()	noexcept { return  static_cast<TY*>(I); }

			const TY* operator& () const noexcept { return static_cast<TY*>(I); }
		private:

			DequeNode_MT* I;
		};


		/************************************************************************************************/


		Iterator begin()	noexcept 
		{ 
			auto current = links.load(std::memory_order_acquire);
			return { current.left };
		}


		Iterator end()		noexcept { return { nullptr }; }


		/************************************************************************************************/


		void push_back(TY* node) noexcept
		{
			uint32_t flag = CreateRandomNumber();
			std::unique_lock lock{ m };

			_Linkage current = links.load(std::memory_order_acquire);

			do
			{
				while (!node->TrySetPrev(current.right));

				if (!current.isLocked() && current.isEmpty() && links.compare_exchange_strong(current, { node, node, 0 })) // Empty Case handled here!
					return;

				// normal push Handled HERE!
				if (!current.isLocked() && !current.isEmpty() && links.compare_exchange_strong(current, { current.left, node, flag })) // exchange and LOCK!
					break;

				current = links.load(std::memory_order_acquire);
			} while (true);

			if (!current.isEmpty())
				while (!current.right->TrySetNext(node));
			while (!links.compare_exchange_strong(current,{ current.left, node, 0 })); // UNLOCK!
		}


		/************************************************************************************************/


		void push_front(TY* node) noexcept
		{
			uint32_t flag = CreateRandomNumber();
			std::unique_lock lock{ m };

			_Linkage current = links.load(std::memory_order_acquire);

			do
			{
				while (!node->TrySetNext(current.left));

				if (!current.isLocked() && current.isEmpty() && links.compare_exchange_strong(current, { node, node, 0 })) // Empty Case handled here!
					return;

				// normal push Handled HERE!
				if (!current.isLocked() && !current.isEmpty() && links.compare_exchange_strong(current, { node, current.right, flag }))// exchange and LOCK!
					break;

				current = links.load(std::memory_order_acquire);
			} while (true);

			if(!current.isEmpty())
				while (!current.left->TrySetPrev(node));
			while (!links.compare_exchange_strong(current, { node, current.right, 0 })); // UNLOCK!
		}


		/************************************************************************************************/


		bool try_pop_front(TY*& out)
		{
			uint32_t flag		= CreateRandomNumber();
			_Linkage current	= links.load(std::memory_order_acquire);

			std::unique_lock lock{ m };

			if (current.isEmpty())
				return false;

			auto leftLinks = current.left->GetLinks();

			if (!current.isLocked() && leftLinks.prev == nullptr)
			{
				// 1 Nodes
				if (current.isSingle() && leftLinks.next == nullptr && leftLinks.prev == nullptr)
				{
					if (!links.compare_exchange_strong(current, { 0, 0, 0 }))
						return false;

					current.left->Clear();
					out = static_cast<TY*>((DequeNode_MT*)current.left);
					current.left->Clear();

					return true;
				}// 2 Nodes

				if (leftLinks.next == current.right && links.compare_exchange_strong(current, { current.right, current.right, flag }, std::memory_order_acq_rel)) // ALSO LOCKS
				{
					out = static_cast<TY*>((DequeNode_MT*)current.left);
					current.left->Clear();
					current.right->Clear();

					links.exchange({ current.right, current.right, 0 }, std::memory_order_release); // UNLOCK

					return true;
				}// N - Nodes
				else if(links.compare_exchange_strong(current, { leftLinks.next, current.right, 0 }, std::memory_order_acq_rel))
				{
					do
					{
						auto leftNeighborLinks = leftLinks.next->GetLinks();
						if (leftNeighborLinks.prev != current.left || 
							leftLinks.next->CMPExchangeLinks({ nullptr, leftNeighborLinks.next }, leftNeighborLinks))
							break;
					} while (true);

					out = static_cast<TY*>((DequeNode_MT*)current.left);
					current.left->Clear();

					return true;
				}
			}

			return false;
		}


		/************************************************************************************************/


		bool try_pop_back(TY*& out)
		{
			uint32_t flag		= CreateRandomNumber();
			_Linkage current	= links.load(std::memory_order_acquire);

			std::unique_lock lock{ m };

			if (current.isEmpty())
				return false;

			auto rightLinks = current.right->GetLinks();

			if (!current.isLocked() && rightLinks.next == nullptr)
			{
				if (current.isSingle() && rightLinks.next == nullptr && rightLinks.prev == nullptr)
				{
					if (!links.compare_exchange_strong(current, { 0, 0, 0 }))
						return false;

					out = static_cast<TY*>((DequeNode_MT*)current.right);
					current.right->Clear();

					return true;
				}// 2 Nodes
				if (rightLinks.prev == current.left && links.compare_exchange_strong(current, { current.left, current.left, flag }, std::memory_order_acq_rel)) // ALSO LOCKS
				{
					out = static_cast<TY*>((DequeNode_MT*)current.right);
					current.right->Clear();
					current.left->Clear();

					links.exchange({ current.left, current.left, 0 }, std::memory_order_release); // UNLOCK

					return true;
				}
				else if (links.compare_exchange_strong(current, { current.left, rightLinks.prev, 0 }, std::memory_order_acq_rel))
				{
					do
					{
						auto rightNeighborLinks = rightLinks.prev->GetLinks();
						if (rightNeighborLinks.next != current.right ||
							rightLinks.prev->CMPExchangeLinks({ rightNeighborLinks.prev, nullptr}, rightNeighborLinks))
							break;

						int x = 0;
					} while (true);

					out = static_cast<TY*>((DequeNode_MT*)current.right);
					current.right->Clear();

					return true;
				}
			}
			return false;
		}


		/************************************************************************************************/


		void push_front(TY& E)
		{
			push_front(&E);
		}

		void push_back(TY& E)
		{
			push_back(&E);
		}


		/************************************************************************************************/


		bool empty()
		{
			_Linkage current = links.load(std::memory_order_acquire);

			return current.isEmpty() && !current.isLocked();
		}

	private:

		static inline uint32_t CreateRandomNumber()
		{
			static std::random_device rd;
			return rd();
		}

		struct _Linkage
		{
			DequeNode_MT*	left;
			DequeNode_MT*	right;
			uint32_t		flag;

			/*
			bool try_LockLeft(uint32_t flag)
			{
				return !left->Lock(flag);
			}

			bool try_LockRight(uint32_t flag)
			{
				return !right->Lock(flag);
			}
			*/
			bool isLocked()
			{
				return flag != 0;
			}

			bool isSingle() { return right == left; }
			bool isEmpty()	{ return isSingle() && right == nullptr; }
		};

		std::atomic< _Linkage>	links = { { nullptr, nullptr, 0 } };
		std::mutex				m;
	};


	/************************************************************************************************/

	template<typename TY>
	class ObjectPool
	{
	public:
		ObjectPool(iAllocator* Allocator, const size_t PoolSize) :
			Pool            { reinterpret_cast<TY*>(Allocator->_aligned_malloc(sizeof(TY) * PoolSize)) },
			FreeObjectList  { Allocator },
			PoolMaxSize     { PoolSize },
			Allocator       { Allocator }
		{
			FreeObjectList.resize(PoolSize);

			for (auto& FreeState : FreeObjectList)
				FreeState = true;
		}


		~ObjectPool()
		{
			Visit([&](auto& Element)
				{
					Element.~TY();
				});


			FreeObjectList.clear();
			Allocator->free(Pool);
		}


		template<typename ... TY_ARGS>
		TY& Allocate(TY_ARGS&& ... Args)
		{
			for (auto Idx = 0; Idx < FreeObjectList.size(); ++Idx)
			{
				if (FreeObjectList[Idx])
				{
					FreeObjectList[Idx] = false;
					new(Pool + Idx) TY(std::forward<TY_ARGS>(Args)...);
					return Pool[Idx];
				}
			}

			FK_ASSERT(false, "ERROR: POOL OUT OF OBJECTS!");
			return *(TY*)nullptr;
		}


		void Release(TY& Object)
		{
			if (&Object < Pool + PoolMaxSize &&
				&Object >= Pool)
			{
				size_t Idx = (
					reinterpret_cast<size_t>(&Object) -
					reinterpret_cast<size_t>(Pool)) / sizeof(TY);

				Pool[Idx].~TY();
				FreeObjectList[Idx] = true;
			}
		}


		template<typename VISIT_FN>
		void Visit(const VISIT_FN& FN_Visitor)
		{
			for (auto Idx = 0; Idx < FreeObjectList.size(); ++Idx)
				if (!FreeObjectList[Idx])
					FN_Visitor(Pool[Idx]);
		}


		template<typename VISIT_FN>
		void Visit(const VISIT_FN& FN_Visitor) const
		{
			for (auto Idx = 0; Idx < FreeObjectList.size(); ++Idx)
				if (!FreeObjectList[Idx])
					FN_Visitor(const_cast<const TY>(Pool[Idx]));
		}


	public:
		TY*				Pool;
		Vector<bool>	FreeObjectList;
		const size_t	PoolMaxSize;
		iAllocator*		Allocator;
	};


	/************************************************************************************************/


	template<typename TY_COL, typename FN_FilterOP>
	auto filter(TY_COL collection, const FN_FilterOP filter_op)
	{
		auto res = std::remove_if(std::begin(collection), std::end(collection), [&](auto& i) { return !filter_op(i); });
		collection.erase(res, std::end(collection));

		return collection;
	}


	template<typename TY_COL, typename FN_transform>
	auto transform(const TY_COL& collection, const FN_transform& transform, iAllocator* allocator = SystemAllocator)
	{
		Vector<decltype(collection[0])> output(allocator);
		output.reserve(collection.size());

		for (auto& c : collection)
			output.emplace_back(transform(c));

		return output;
	}


	template<typename TY_COL, typename FN_transform>
	decltype(auto) transform_stl(const TY_COL& collection, const FN_transform& transform)
	{
		std::vector<decltype(transform(collection[0]))> output{};
		output.reserve(collection.size());

		for (auto& c : collection)
			output.emplace_back(transform(c));

		return output;
	}


	/************************************************************************************************/


	template<typename TY>
	class IntrusiveLinkedList
	{
	public:

		class Iterator;

		class node : public TY
		{
		public:
			template<typename ... TY_ARGS>
			node(TY_ARGS... args) : TY{ std::forward<TY_ARGS>(args)... } {}

			// this class is non-copyable, non-moveable
			node(const node&	rhs) = delete;
			node(const node&&	rhs) = delete;

			node& operator = (const node&	rhs) = delete;
			node& operator = (const node&&	rhs) = delete;

		private:
			node* prev_ptr	= nullptr;
			node* next_ptr	= nullptr;

			friend Iterator;
			friend IntrusiveLinkedList;
		};

		class Iterator
		{
		public:
			Iterator() = default;


			Iterator(TY* IN_ptr) : 
				_ptr{ static_cast<node*>(IN_ptr) } {}


			Iterator operator + (size_t rhs) const noexcept
			{
				auto itr = _ptr;

				for (size_t i = 0; i < rhs; ++i)
					itr = itr ? itr->next_ptr : nullptr;

				return { itr };
			}


			Iterator operator - (size_t rhs) const noexcept
			{
				auto itr = _ptr;

				for (size_t i = 0; i < rhs; ++i)
					itr = itr ? itr->prev_ptr : nullptr;

				return { itr };
			}


			Iterator operator ++ (int) const noexcept
			{
				return { _ptr ? _ptr->next_ptr : nullptr };
			}


			Iterator& operator ++ () noexcept
			{
				_ptr = { _ptr ? _ptr->next_ptr : nullptr };
				return *this;
			}


			Iterator operator -- (int) const noexcept
			{

				return { _ptr ? _ptr->prev_ptr : nullptr };
			}


			Iterator& operator -- () noexcept
			{
				_ptr = { _ptr ? _ptr->prev_ptr : nullptr };
				return *this;
			}


			Iterator& operator += (size_t rhs) noexcept
			{
				for (size_t i = 0; i < rhs; ++i)
					_ptr = _ptr ? _ptr->prev_ptr : nullptr;

				return *this;
			}


			Iterator& operator -= (size_t rhs) noexcept
			{
				for (size_t i = 0; i < rhs; ++i)
					_ptr = _ptr ? _ptr->prev_ptr : nullptr;

				return *this;
			}


			node* operator -> ()
			{
				return _ptr;
			}


			node& operator * ()
			{
				return *_ptr;
			}


			bool operator != (const Iterator& rhs) const
			{
				return rhs._ptr != _ptr;
			}


			operator bool()
			{
				return _ptr != nullptr;
			}

		private: 
			node* _ptr;
		};

		using TY_Element = node;


		Iterator begin()
		{
			return { first };
		}

		Iterator end()
		{
			return { nullptr };
		}

		const Iterator begin() const
		{
			return { first };
		}

		const Iterator end() const
		{
			return { nullptr };
		}

		void push_back(TY_Element& element)
		{
			if(last)
			{
				last->next_ptr		= &element;
				element.prev_ptr	= last;
			}

			last = &element;

			if (first == nullptr)
				first = &element;
		}

		void push_front(TY_Element& element)
		{
			if (first) 
			{
				first->prev_ptr		= &element;
				element.next_ptr	= first;
			}

			first = &element;

			if (last == nullptr)
				last = &element;
		}
		
		bool try_pop_front(TY_Element*& out)
		{
			if (empty())
				return false;

			out		= first;

			if (last == first)
			{
				last	= nullptr;
				first	= nullptr;
			}
			else
			{
				first = first->next_ptr;

				if (first)
					first->prev_ptr = nullptr;
			}
			return true;
		}

		bool try_pop_back(TY_Element*& out)
		{


			if (empty())
				return false;

			out		= last;

			if (last == first) 
			{
				last	= nullptr;
				first	= nullptr;
			}
			else
			{
				last = last->prev_ptr;

				if (last)
					last->next_ptr = nullptr;
			}

			return true;
		}

		bool empty()
		{
			return first == nullptr;
		}

		private:
			TY_Element* first	= nullptr;
			TY_Element* last	= nullptr;
	};


	/************************************************************************************************/


	template<typename FuncSig, unsigned int STORAGESIZE = 64, unsigned int Alignment = 8>
	class alignas(Alignment) TypeErasedCallable
	{
	private:

		template<typename>
		struct ProxyTypeGenerator {};

		template<typename TY_R, typename ... TY_args>
		struct ProxyTypeGenerator<TY_R (TY_args...)>
		{
			using TY_Return = TY_R;

			using fnCopy		= void (*)(char* lhs, const char* rhs);
			using fnMove		= void (*)(char* lhs, char* rhs);
			using fnProxy		= TY_R (*)(char*, TY_args... args);
			using fnProxyConst	= TY_R (*)(const char*, TY_args... args);
			using fnDestructor	= void (*)(char*);

			struct VTable
			{
				fnCopy			copy;
				fnMove			move;
				fnProxy			proxy;
				fnDestructor	destructor;
			};

			static size_t GetOffset(size_t alignment, const void* _ptr, size_t size)
			{
				const size_t offset = alignment - (size_t(_ptr) % alignment);
				return offset == alignment ? 0 : offset;
			}

			template<typename TY_CALLABLE>
			static const VTable* GenerateVTable()
			{
				static const VTable sVTable{
					.copy =
						[](char* lhs_ptr, const char* rhs_ptr)
						{
							constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;

							if constexpr (alignment == 1)
							{
								const TY_CALLABLE* rhs = reinterpret_cast<const TY_CALLABLE*>(rhs_ptr);
								new(lhs_ptr) TY_CALLABLE(*rhs);
							}
							else
							{
								const TY_CALLABLE* rhs = reinterpret_cast<const TY_CALLABLE*>(rhs_ptr + GetOffset(alignment, rhs_ptr, STORAGESIZE));
								new(lhs_ptr + GetOffset(alignment, lhs_ptr, STORAGESIZE)) TY_CALLABLE{ *rhs };
							}
						},

					.move =
						[](char* lhs_ptr, char* rhs_ptr)
						{
							constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;

							if constexpr (alignment == 1)
							{
								TY_CALLABLE* rhs = reinterpret_cast<TY_CALLABLE*>(rhs_ptr);
								new(lhs_ptr) TY_CALLABLE(std::move(*rhs));
							}
							else
							{
								const TY_CALLABLE* rhs = reinterpret_cast<const TY_CALLABLE*>(rhs_ptr + GetOffset(alignment, rhs_ptr, STORAGESIZE));
								new(lhs_ptr + GetOffset(alignment, lhs_ptr, STORAGESIZE)) TY_CALLABLE{ std::move(*rhs) };
							}
						},

					.proxy =
						[](char* _ptr, TY_args ... args) -> TY_Return
						{
							constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;
							if constexpr (alignment == 1)
							{
								auto& callable = *reinterpret_cast<TY_CALLABLE*>(_ptr);
								return callable(std::forward<TY_args>(args)...);
							}
							else
							{
								auto& callable = *reinterpret_cast<TY_CALLABLE*>(_ptr + GetOffset(alignment, _ptr, STORAGESIZE));
								return callable(std::forward<TY_args>(args)...);
							}
						},

					.destructor =
						[](char* _ptr)
						{
							constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;

							if constexpr (alignment == 1)
							{
								auto functor = reinterpret_cast<TY_CALLABLE*>(_ptr);
								functor->~TY_CALLABLE();
							}
							else
							{
								auto functor = reinterpret_cast<TY_CALLABLE*>(_ptr + GetOffset(alignment, _ptr, STORAGESIZE));
								functor->~TY_CALLABLE();
							}
						}
					};

				return &sVTable;
			}

			template<typename TY_CALLABLE>
			static const VTable* Assign(void* buffer, TY_CALLABLE&& callable)
			{
				static_assert(sizeof(TY_CALLABLE) <= STORAGESIZE - sizeof(VTable*), "Callable object too large for this TypeErasedCallable!");

				constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;
				const size_t offset = GetOffset(alignment, buffer, STORAGESIZE);

				if (sizeof(TY_CALLABLE) + offset > STORAGESIZE - sizeof(VTable*))
					throw std::runtime_error("Failed to create type erased callable. size + alignment offset larger than buffer size.");

				new((char*)buffer + offset) std::decay_t<TY_CALLABLE>{ callable };

				return GenerateVTable<std::decay_t<TY_CALLABLE>>();
			}

			template<typename TY_CALLABLE>
			static const VTable* Assign(void* buffer, const TY_CALLABLE& callable)
			{
				static_assert(sizeof(TY_CALLABLE) <= STORAGESIZE - sizeof(VTable*), "Callable object too large for this TypeErasedCallable!");

				constexpr size_t alignment = std::alignment_of_v<TY_CALLABLE>;
				const size_t offset = GetOffset(alignment, buffer, STORAGESIZE);

				if (sizeof(TY_CALLABLE) + offset > STORAGESIZE - sizeof(VTable*))
					throw std::runtime_error("Failed to create type erased callable. size + alignment offset larger than buffer size.");

				new((char*)buffer + offset) TY_CALLABLE{ callable };

				return GenerateVTable<std::decay_t<TY_CALLABLE>>();
			}
		};

		using Generator		= ProxyTypeGenerator<FuncSig>;

		using TY_Return		= Generator::TY_Return;
		using FN_PTR		= FuncSig;
		using VTable		= Generator::VTable;

		using fnCopy		= void (*)(char* lhs, const char* rhs);
		using fnMove		= void (*)(char* lhs, char* rhs);
		using fnProxy		= Generator::fnProxy;
		using fnProxyConst	= Generator::fnProxyConst;
		using fnDestructor	= void (*)(char*);

	public:
		TypeErasedCallable() = default;


		template<typename TY_CALLABLE> requires(!std::is_same_v<std::decay_t<TY_CALLABLE>, TypeErasedCallable>)
		TypeErasedCallable(const TY_CALLABLE& callable) noexcept
		{
			Assign(callable);
		}


		template<typename TY_CALLABLE> requires(!std::is_same_v<std::decay_t<TY_CALLABLE>, TypeErasedCallable>)
		TypeErasedCallable(TY_CALLABLE&& callable) noexcept
		{
			Assign(callable);
		}


		TypeErasedCallable(FN_PTR* fn_ptr) noexcept
		{
			Assign([fn_ptr](auto&&... args)
				{
					return fn_ptr(args...);
				});
		}


		TypeErasedCallable(TypeErasedCallable&& callable) noexcept
		{
			if (vtable)
				vtable->destructor(buffer);

			if (callable.vtable)
				callable.vtable->move(buffer, callable.buffer);

			vtable			= callable.vtable;
			callable.vtable = nullptr;
		}


		TypeErasedCallable(const TypeErasedCallable& callable) noexcept
		{
			if (vtable)
				vtable->destructor(buffer);

			if (callable.vtable)
				callable.vtable->copy(buffer, callable.buffer);

			vtable = callable.vtable;
		}


		~TypeErasedCallable()
		{
			Release();
		}


		TypeErasedCallable& operator = (const TypeErasedCallable& rhs)
		{
			if(vtable && vtable->destructor)
				vtable->destructor(buffer);

			if (rhs.vtable)
				rhs.vtable->copy(buffer, rhs.buffer);

			vtable = rhs.vtable;

			return *this;
		}


		TypeErasedCallable& operator = (TypeErasedCallable&& rhs)
		{
			if (vtable && vtable->destructor)
				vtable->destructor(buffer);

			if(rhs.vtable)
				rhs.vtable->move(buffer, rhs.buffer);

			vtable		= rhs.vtable;
			rhs.vtable	= nullptr;

			return *this;
		}


		template<typename TY_CALLABLE> requires(!std::is_same_v<std::decay_t<TY_CALLABLE>, TypeErasedCallable>)
		TypeErasedCallable& operator = (const TY_CALLABLE& callable)
		{
			if (vtable && vtable->destructor)
				vtable->destructor(buffer);

			Assign(callable);

			return *this;
		}


		template<typename TY_CALLABLE> requires(!std::is_same_v<std::decay_t<TY_CALLABLE>, TypeErasedCallable>)
		TypeErasedCallable& operator = (TY_CALLABLE&& callable)
		{
			if (vtable && vtable->destructor)
				vtable->destructor(buffer);

			Assign(callable);

			return *this;
		}

		template<typename ... TY_args>
		auto operator()(TY_args&& ... args)
		{
			return vtable->proxy(buffer, std::forward<TY_args>(args)...);
		}


		template<typename ... TY_args>
		auto operator()(TY_args&& ... args) const
		{
			return reinterpret_cast<fnProxyConst>(vtable->proxy)(buffer, std::forward<TY_args>(args)...);
		}


		operator bool() const
		{
			return vtable != nullptr;
		}


		void Release()
		{
			if (vtable && vtable->destructor)
				vtable->destructor(buffer);

			vtable = nullptr;
		}

	private:


		template<typename TY_CALLABLE>
		void Assign(TY_CALLABLE&& callable)
		{
			static_assert(sizeof(TY_CALLABLE) <= STORAGESIZE, "Callable object too large for this TypeErasedCallable!");

			vtable = Generator::Assign(buffer, callable);
		}


		template<typename TY_CALLABLE>
		void Assign(const TY_CALLABLE& callable)
		{
			static_assert(sizeof(TY_CALLABLE) <= STORAGESIZE, "Callable object too large for this TypeErasedCallable!");

			vtable = Generator::Assign(buffer, callable);
		}

		const VTable*	vtable = nullptr;
		char			buffer[STORAGESIZE - sizeof(VTable*)];
	};


}	// namespace FlexKit;
	/************************************************************************************************/
#endif


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

