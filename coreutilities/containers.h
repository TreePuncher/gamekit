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

// Define the Containers used in the application here
#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\static_vector.h"

#if USING(USESTL)

#include <atomic>
#include <deque>
#include <list>
#include <new>
#include <map>
#include <stdint.h>
#include <utility>
#include <vector>
#include <condition_variable>

template<typename Ty>					using deque_t	= std::deque<Ty>;
template<typename Ty>					using vector_t	= std::vector<Ty>;
template<typename Ty, typename Ty_key>	using map_t		= std::map<Ty, Ty_key>;
template<typename Ty>					using list_t	= std::list<Ty>;

namespace FlexKit
{
	/************************************************************************************************/


	template<typename Ty, size_t SIZE = 16>
	struct RingBuffer
	{
		RingBuffer()
		{
			head	= 0;
			end		= 0;
			count	= 0;
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
			if (count){
				--count;
				count = count % SIZE;

				i = head++;
				head = head % SIZE;
			}
			return _t[i];
		}

		Ty& back(){
			return _t[end];
		}

		Ty& front(){
			return _t[head];
		}

		Ty& operator[] (uint32_t i)	{return _t[i];}

		void clear(){
			count = 0;
			head  = 0;
			end   = 0;
		}

		uint32_t size(){
			return count;
		}

		Ty		_t[SIZE];
		size_t	head;
		size_t	end;
		size_t	count;
	};


	/************************************************************************************************/


	// NOTE: Doesn't call destructors automatically, but does free held memory!
	template<typename Ty>
	struct Vector
	{
		typedef Vector<Ty> THISTYPE;

		typedef Ty*			Iterator;
		typedef const Ty*	Iterator_const;

		inline  Vector(
			iAllocator*		Alloc				= nullptr, 
			const size_t	InitialReservation	= 0) : 
				Allocator	(Alloc), 
				Max			(InitialReservation),
				Size		(0),
				A			(nullptr) 
		{
			if (InitialReservation > 0)
			{
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * InitialReservation);
				FK_ASSERT(NewMem);

				A	= NewMem;
				Max = InitialReservation;
			}
		}


		inline  Vector(const THISTYPE& RHS) :
			Allocator	(RHS.Allocator),
			Max			(RHS.Max),
			Size		(RHS.Size) 
		{ 
			(*this) = RHS; 
		}


		inline  Vector(THISTYPE&& RHS) : 
			A			(RHS.A),
			Allocator	(RHS.Allocator),
			Max			(RHS.Max),
			Size		(RHS.Size)
		{
			RHS.A	 = nullptr;
			RHS.Max  = 0;
			RHS.Size = 0;
		}


		inline ~Vector(){if(A)Allocator->_aligned_free(A);}


		inline			Ty& operator [](size_t index)		{return A[index];}
		inline const	Ty& operator [](size_t index) const {return A[index];}


		/************************************************************************************************/


		THISTYPE& operator =(const THISTYPE& RHS)
		{
			if (!Allocator) Allocator = RHS.Allocator;
			
			Release();
			reserve(RHS.size());

			for (const auto& E : RHS)
				push_back(E);

			return *this;
		}


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


		THISTYPE& operator =(THISTYPE&& RHS)
		{
			if (!Allocator) Allocator = RHS.Allocator;

			Release();

			A        = RHS.A;
			Max      = RHS.Max;
			Size     = RHS.Size;

			RHS.Size = 0;
			RHS.Max  = 0;
			RHS.A    = nullptr;

			return *this;
		}


		/************************************************************************************************/


		THISTYPE& operator +=(const THISTYPE& RHS)
		{
			reserve(size() + RHS.size());
			for (auto I : RHS)
				push_back(I);

			return (*this);
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


		/************************************************************************************************/


		Ty& back()
		{
			FK_ASSERT(Size > 0);
			return A[Size - 1];
		}


		/************************************************************************************************/


		void resize(size_t newSize)
		{
			if (Max < newSize)
				reserve(newSize);

			if (newSize > size()) {
				auto I = newSize - size();
				while(I--)
					push_back(Ty());
			}
			else if(newSize < size())
			{
				auto I = size() - newSize;
				while (I--)
					pop_back();
			}
		}


		/************************************************************************************************/


		/*
		void push_back(Ty&& in){
			if (Size + 1 > Max)
			{// Increase Size
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * 2 * C->Max);
				FK_ASSERT(NewMem);

				{
					size_t itr = 0;
					size_t End = Size + 1;
					while (itr < End) new(NewMem + itr) ();
				}
				{
					size_t itr = 0;
					size_t End = Size;
					while (itr < End) NewMem[itr] = A[itr];
				}

				mem->_aligned_free(A);
				Size++;
				A    = NewMem;
				Max *= 2;
			}

			A[Size++] = in;
		}
		*/


		/************************************************************************************************/


#if 0
		void push_back(Ty in) {
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				auto NewSize = sizeof(Ty) * 2 * (Max < 1)? 1 : Max;
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(NewSize);
				
#ifdef _DEBUG
				FK_ASSERT(NewMem);
				FK_ASSERT(NewMem != A);
#endif

				if(A){
					size_t itr = 0;
					size_t End = Size;
					for (;itr < End; ++itr) 
						NewMem[itr] = A[itr];

					Allocator->_aligned_free(A);
				}

				A	  = NewMem;
				Max   = (Max) ? Max * 2 : 1;
			}

			A[Size++] = in;
		}
#else

		void push_back(const Ty& in) {
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
				{
					size_t itr = 0;
					size_t End = Size;
					for (; itr < End; ++itr)
					{
						new(NewMem + itr) Ty();
					}
				}
#ifdef _DEBUG
				FK_ASSERT(NewMem);
				if(Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A) {
					size_t itr = 0;
					size_t End = Size;
					for (; itr < End; ++itr)
						NewMem[itr] = A[itr];

					Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}

			new(A + Size++) Ty(in); // 
		}

#endif


		/************************************************************************************************/

		template<typename ... ARGS_t>
		void emplace_back(ARGS_t&& ... in) {
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				auto NewSize = ((Max < 1) ? 2 : (2 * Max));
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
#ifdef _DEBUG
				FK_ASSERT(NewMem);
				if (Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A) 
				{
					size_t itr = 0;
					size_t End = Size;
					for (; itr < End; ++itr)
						new(NewMem + itr) Ty{ std::move(A[itr]) };

					Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}

			new(A + Size++) Ty(std::forward<ARGS_t>(in)...);
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


		void remove_stable(Iterator I)
		{
			if (I == end())
				return;

			while (I != end() && (I + 1) != end())
			{
				*I = *(I + 1);
				++I;
			}
			pop_back();
		}


		/************************************************************************************************/


		Ty pop_back()
		{
			auto Temp = back();
			A[Size--].~Ty();

			return Temp;
		}


		/************************************************************************************************/


		void reserve(size_t NewSize)
		{
			if (Max < NewSize)
			{// Increase Size
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * NewSize);
				FK_ASSERT(NewMem);

				if (A)
				{
					size_t itr = 0;
					size_t End = Size;
					for (; itr < End; ++itr) 
						new(NewMem + itr) Ty(std::move(A[itr])); // move if Possible

					Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}
		}


		/************************************************************************************************/


		void clear()
		{
			for (size_t i = 0; i < size(); ++i){
				(A + i)->~Ty();
			}
			Size = 0;
		}


		/************************************************************************************************/

		// Releases Memory, does call Destructors!
		void Release()
		{
			clear();

			if(A && Allocator)
				Allocator->_aligned_free(A);

			A	= nullptr;
			Max = 0;
		}


		/************************************************************************************************/


		Ty& at(size_t index) { return A[index]; }


		Iterator begin()	{ return A;			}
		Iterator end()		{ return A + Size;	}

		Iterator_const begin()	const { return A; }
		Iterator_const end()	const { return A + Size; }
		size_t size()			const { return Size; }


		/************************************************************************************************/
		
		
		Ty*	A;

		size_t Size;
		size_t Max;

		iAllocator* Allocator;


		/************************************************************************************************/
	};


	template< typename Ty_Get, template<typename Ty, typename... Ty_V> class TC, typename Ty_, typename... TV2> Ty_Get GetByType(TC<Ty_, TV2...>& in)	{ return in.GetByType<Ty_Get>(); }

	template<typename Ty_1, typename Ty_2>
	struct Pair
	{
		typedef Pair<Ty_1, Ty_2> ThisType;
		
		template<typename Ty_Get, typename Ty_2>	static  inline Ty_Get& _GetByType (Pair<Ty_Get, Ty_2>&		in)	{ return in.V1; }
		template<typename Ty_Get, typename Ty_2>	static  inline Ty_Get& _GetByType (Pair<Ty_2, Ty_Get>&		in)	{ return in.V2; }
		template<typename Ty_Get>					static  inline Ty_Get& _GetByType (Pair<Ty_Get, Ty_Get>&	in)	{ static_assert(false, "NON_UNIQUE TYPES IN Pair!");  return in.V2; }
		template<typename Ty_Get>							inline Ty_Get&  GetByType ()							{ return _GetByType<Ty_Get>(*this); }

		template<typename Ty_Assign> ThisType operator = (const Ty_Assign& in)	{ Ty_Assign& thisVar = GetByType<Ty_Assign>(); thisVar = in; return *this;}

		explicit operator bool()					{ return GetByType<bool>(); }
		template<typename Ty_1>	operator Ty_1()		{ return GetByType<Ty_1>(); }

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

		SL_list(iAllocator* M = nullptr) : Memory(M)
		{
			FirstNode = nullptr;
			LastNode  = nullptr;
			Count     = 0;
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
			Node(const TY& Initial) : Data(Initial)
			{
			}

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
			Node* NewNode  = &Memory->allocate_aligned<Node>(std::move(e));

			NullCheck(NewNode);

			Node* PrevNext = Itr._ptr->Next;
			Itr._ptr->Next = NewNode;
			NewNode->Next  = PrevNext;

			Itr->Container->Count++;
		}

		Iterator push_back(TY e)
		{
			Node* NewNode	= &Memory->allocate_aligned<Node>(e);

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

			return {NewNode, this};
;		}

		TY pop_front()
		{
			FK_ASSERT(Count != 0);

			auto* First(FirstNode.load());

			if (First)
				FirstNode.store(First->Next);

			if (First->Next.load() == nullptr)
				LastNode.store(nullptr);

			FINAL(Count--; Memory->free(First); );

			return First->Data;
		}

		TY& first()
		{
			return FirstNode.load()->Data;
		}

		Iterator begin()
		{
			FK_ASSERT(Count != 0);
			return {FirstNode, this};
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

		iAllocator* Memory;
		std::atomic<size_t>	Count;
	};


	/************************************************************************************************/


	template<typename Ty, int SIZE = 64>
	struct CircularBuffer
	{
		CircularBuffer() : _Head(0), _Size(0)
		{}


		~CircularBuffer()
		{
			Release();
		}

		void Release()
		{
			for (auto& Element : *this)
				Element.~Ty();

			_Size = 0;
		}

		Ty& operator [](size_t idx)
		{
			return at(idx);
		}

		Ty& at(size_t idx)
		{
			idx = (_Head - idx - 1) % SIZE;
			return Buffer[idx];
		}

		bool full() noexcept {
			return (_Size > 0);
		}

		bool empty() noexcept {
			return (_Size == 0);
		}

		void clear() noexcept {
			_Size = 0;
			_Head = 0;
		}

		size_t size() noexcept {
			return _Size;
		}

		Ty pop_front() noexcept
		{
			//if (!_Size)
			//	FK_ASSERT("BUFFER EMPTY!");


			Ty Out = front();
			_Size = std::max(--_Size, 0);
			return Out;
		}

		Ty pop_back() noexcept
		{
			//if (!_Size)
			//	FK_ASSERT("BUFFER EMPTY!");


			Ty Out = back();
			_Size = std::max(--_Size, 0);
			_Head = (SIZE + --_Head) % SIZE;
			return Out;
		}

		bool push_back(const Ty& Item) noexcept
		{
			if (_Size + 1 > SIZE)// Call Destructor on Tail
				back().~Ty();

			_Size = min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;

			return false;
		}

		bool push_back(Ty&& Item) noexcept
		{
			_Size = min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;

			if (_Size + 1 > SIZE)// Call Destructor on Tail
				Buffer[idx].~Ty();

			Buffer[idx] = std::move(Item);

			return false;
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

			bool operator <		(CircularIterator rhs) { return Idx < rhs.Idx; }
			bool operator ==	(CircularIterator rhs) { return Idx == rhs.Idx; }

			bool operator !=	(CircularIterator rhs) { return !(*this == rhs); }


			void Increment()
			{
				Idx++;
			}

			void Decrement()
			{
				Idx--;;
			}

			CircularIterator operator ++ (int)	{ auto Temp = *this; Increment(); return Temp; }
			CircularIterator operator ++ ()		{ Increment(); return (*this); }

			CircularIterator operator -- (int)	{ auto Temp = *this; Decrement(); return Temp; }
			CircularIterator operator -- ()		{ Decrement(); return (*this); }

		};

		CircularIterator begin()
		{
			return{ this, 0 };
		}

		CircularIterator end()
		{
			return{ this, _Size };
		}

		int _Head, _Size;
		Ty Buffer[SIZE];
	};

	struct test
	{
		int x;
	};


	/************************************************************************************************/


	class DequeNode_MT
	{
	public:
		DequeNode_MT(const DequeNode_MT& E)				= delete;
		DequeNode_MT& operator = (const DequeNode_MT&)	= delete; // No copying allowed, this class does no memory management

		bool operator == (const DequeNode_MT& rhs) { return this == &rhs; }

		DequeNode_MT() :
			PrevNext{ {nullptr, nullptr} }{}

		virtual ~DequeNode_MT() {}


		DequeNode_MT* GetNext()
		{
			return PrevNext.load().Next;
		}

		DequeNode_MT* GetPrev()
		{
			return PrevNext.load().Prev;
		}

		void SetLinks(DequeNode_MT* NewPrev, DequeNode_MT* NewNext)
		{
			PrevNext.store({ NewNext, NewPrev });
		}

		struct NodeLinkage
		{
			DequeNode_MT* Next = nullptr;
			DequeNode_MT* Prev = nullptr;
		};

		std::atomic<NodeLinkage> PrevNext;
	};

	
	// Intrusive, Thread-Safe, Double Linked List
	// This Class assumes nodes have inherited the from DequeNode_MT 
	// Destructor does not free elements!!
	template<typename TY> 
	class Deque_MT
	{
	public:

		/************************************************************************************************/
		
		Deque_MT()	= default;
		~Deque_MT() = default;

		Deque_MT(const Deque_MT&)				= delete;
		Deque_MT& operator =(const Deque_MT&)   = delete;

		Deque_MT(Deque_MT&&)					= delete;
		Deque_MT& operator =(Deque_MT&&)		= delete;


		/************************************************************************************************/


		class Iterator
		{
		public:
			Iterator(DequeNode_MT* _ptr) : I{ _ptr } {}

			Iterator operator ++ (int)
			{ 
				auto Next = I->GetNext();
				return Iterator(Next ? Next : nullptr);
			}

			Iterator operator -- (int)
			{ 
				auto Prev = I->GetNext();
				return Iterator(Prev ? Prev : nullptr);
			}

			void operator ++ () 
			{ 
				auto Next = I->GetNext();
				I = Next ? Next : nullptr;
			}
			void operator -- () 
			{ 
				auto Prev = I->GetNext();
				I = Prev ? Prev : nullptr; 
			}

			DequeNode_MT* GetPtr()
			{
				return I;
			}

			bool operator !=(const Iterator& rhs) { return I != rhs.I; }

			TY& operator* ()				{ return *static_cast<TY*>(I);	}
			TY* operator-> ()				{ return  static_cast<TY*>(I);	}

			const TY* operator& () const	{ return static_cast<TY*>(I); }
		private:

			DequeNode_MT* I;
		};


		/************************************************************************************************/


		Iterator begin()	{ return _GetFirst(); }
		Iterator end()		{ return nullptr; }


		/************************************************************************************************/

		void push_back(TY& E)
		{
			push_back(&E);
		}

 		void push_back(TY* E)
		{
			do
			{
				auto CurrentFirstLast = BeginEnd.load(std::memory_order_acquire);
				E->SetLinks(CurrentFirstLast.Last, nullptr);

				auto NewLinkage =
					(!CurrentFirstLast.First && !CurrentFirstLast.Last) ?
						NodeLinkage_BEGINEND{ E, E } :
						NodeLinkage_BEGINEND{ CurrentFirstLast.First, E };

				if (CurrentFirstLast.First && CurrentFirstLast.Last)
				{
					auto Temp = CurrentFirstLast.Last ? CurrentFirstLast.Last->GetPrev() : nullptr;
					if (BeginEnd.compare_exchange_weak(CurrentFirstLast, NewLinkage, std::memory_order_acquire))
					{
						CurrentFirstLast.Last->SetLinks(Temp, E);
						return;
					}
				}
				else
				{
					if (BeginEnd.compare_exchange_weak(
							CurrentFirstLast, NewLinkage, std::memory_order_acquire))
						return;
				}
			} while (true);
		}


		/************************************************************************************************/


		void push_front(TY& E)
		{
			push_front(&E);
		}


		void push_front(TY* E)
		{
			do
			{
				auto CurrentFirstLast = BeginEnd.load(std::memory_order_acquire);
				E->SetLinks(nullptr, CurrentFirstLast.First);

				auto NewLinkage = (!CurrentFirstLast.First && !CurrentFirstLast.Last) ?
					NodeLinkage_BEGINEND{ E, E } :
					NodeLinkage_BEGINEND{ E, CurrentFirstLast.Last };

				if (CurrentFirstLast.First && CurrentFirstLast.Last)
				{
					auto Temp = CurrentFirstLast.First ? CurrentFirstLast.First->GetNext() : nullptr;
					if (BeginEnd.compare_exchange_weak(CurrentFirstLast, NewLinkage, std::memory_order_acquire))
					{
						CurrentFirstLast.First->SetLinks(E, Temp);
						return;
					}
				}
				else
				{
					if (BeginEnd.compare_exchange_weak(
						CurrentFirstLast, NewLinkage, std::memory_order_acquire))
						return;
				}
			} while (true);
		}


		/************************************************************************************************/
		/*
		// Note, THIS WILL BLOCK IF THERE ARE NO ELEMENTS!
		Element_TY& pop_front()
		{
			std::mutex M;
			std::unique_lock Lock(M);

			Element_TY* Out;

			do
			{
				CV.wait(Lock, [&] { return !empty(); });
			} while (!try_pop_front(Out));

			return *Out;
		}
		*/

		/************************************************************************************************/


		bool try_pop_front(TY*& Out)
		{
			auto CurrentFirstLast = BeginEnd.load(std::memory_order_acquire);
			
			if (CurrentFirstLast.First == nullptr || CurrentFirstLast.Last == nullptr)
				return false;

			auto NextNode	=  CurrentFirstLast.First->GetNext();
			auto NewLinkage = (CurrentFirstLast.First == CurrentFirstLast.Last) ?
				NodeLinkage_BEGINEND{ nullptr, nullptr } : 
				NodeLinkage_BEGINEND{ NextNode, CurrentFirstLast.Last };

			if (BeginEnd.compare_exchange_strong(
				CurrentFirstLast,
				NewLinkage,
				std::memory_order_release))
			{
				if(NextNode)
					NextNode->SetLinks(nullptr, NextNode->GetNext());
			}
			else
				return false;

			Out = static_cast<TY*>(CurrentFirstLast.First);

			return true;
		}


		/************************************************************************************************/


		bool try_pop_back(TY*& Out)
		{
			auto CurrentFirstLast = BeginEnd.load(std::memory_order_acquire);
			
			if ((CurrentFirstLast.First == nullptr || CurrentFirstLast.Last == nullptr))
				return false;

			auto PrevNode	=  CurrentFirstLast.Last->GetPrev();
			auto NewLinkage = (CurrentFirstLast.First == CurrentFirstLast.Last) ?
				NodeLinkage_BEGINEND{ nullptr, nullptr } : 
				NodeLinkage_BEGINEND{ CurrentFirstLast.First, PrevNode };

			if (BeginEnd.compare_exchange_strong(
				CurrentFirstLast,
				NewLinkage,
				std::memory_order_release))
			{
				if(PrevNode)
					PrevNode->SetLinks(PrevNode->GetPrev(), nullptr);
			}
			else
				return false;

			Out = static_cast<TY*>(CurrentFirstLast.Last);

			return true;
		}


		/************************************************************************************************/


		bool empty()
		{
			auto CurrentFirstLast = BeginEnd.load(std::memory_order_acquire);

			return (CurrentFirstLast.First == nullptr || CurrentFirstLast.Last == nullptr);
		}


	private:

		DequeNode_MT* _GetFirst()
		{
			auto Links = BeginEnd.load(std::memory_order_acquire);
			return Links.First;
		}


		DequeNode_MT* _GetLast()
		{
			auto Links = BeginEnd.load(std::memory_order_acquire);
			return Links.Last;
		}


		struct NodeLinkage_BEGINEND
		{
			DequeNode_MT* First		= nullptr;
			DequeNode_MT* Last		= nullptr;
		};

		std::atomic<NodeLinkage_BEGINEND>	BeginEnd;
	};


	/************************************************************************************************/

	template<typename TY>
	class ObjectPool
	{
	public:
		ObjectPool(iAllocator* Allocator, const size_t PoolSize) :
			Pool			{ reinterpret_cast<TY*>(Allocator->_aligned_malloc(sizeof(TY) * PoolSize)) },
			FreeObjectList	{ Allocator },
			PoolMaxSize		{ PoolSize	},
			Allocator		{ Allocator }
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
			if (&Object >= Pool.begin() &&
				&Object < Pool.end())
			{
				size_t Idx = (
					reinterpret_cast<size_t>(&Object) -
					reinterpret_cast<size_t>(Pool.begin())) / sizeof(TY);

				Pool[Idx].~TY();
				FreeObjectList[Idx] = true;
			}
		}


		template<typename VISIT_FN>
		void Visit(VISIT_FN& FN_Visitor)
		{
			for (auto Idx = 0; Idx < FreeObjectList.size(); ++Idx)
				if (!FreeObjectList[Idx])
					FN_Visitor(Pool[Idx]);
		}


		template<typename VISIT_FN>
		void Visit(VISIT_FN& FN_Visitor) const
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

}	// namespace FlexKit;
	/************************************************************************************************/
#endif
#endif