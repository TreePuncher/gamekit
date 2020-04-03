/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
#include <mutex>
#include <shared_mutex>
#include <stdint.h>
#include <utility>
#include <vector>
#include <condition_variable>
#include <random>

template<typename Ty>					using deque_t = std::deque<Ty>;
template<typename Ty>					using vector_t = std::vector<Ty>;
template<typename Ty, typename Ty_key>	using map_t = std::map<Ty, Ty_key>;
template<typename Ty>					using list_t = std::list<Ty>;

namespace FlexKit
{
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


	// NOTE: Doesn't call destructors automatically, but does free held memory!
	template<typename Ty>
	struct Vector
	{
		typedef Vector<Ty> ELEMENT_TYPE;
		typedef Vector<Ty> THISTYPE;

		typedef Ty*			Iterator;
		typedef const Ty*	Iterator_const;

		inline  Vector(
			iAllocator*		Alloc = nullptr,
			const size_t	InitialReservation = 0) :
			    Allocator(Alloc),
			    Max(InitialReservation),
			    Size(0),
			    A(nullptr)
		{
			if (InitialReservation > 0)
			{
				FK_ASSERT(Allocator);
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * InitialReservation);
				FK_ASSERT(NewMem);

				A = NewMem;
				Max = InitialReservation;
			}
		}

        template<typename TY_Initial>
        inline  Vector(
			iAllocator*		    Alloc,
			const size_t	    InitialSize,
            const TY_Initial&   Initial_V) :
			    Allocator   { Alloc },
			    Max         { InitialSize },
			    Size        { 0 },
			    A           { nullptr }
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

		inline  Vector(const THISTYPE& RHS) :
            Allocator   { RHS.Allocator },
			Max         { RHS.Max       },
			Size        { RHS.Size      }
		{
			(*this) = RHS;
		}


		inline  Vector(THISTYPE&& RHS) noexcept :
			A(RHS.A),
			Allocator(RHS.Allocator),
			Max(RHS.Max),
			Size(RHS.Size)
		{
			RHS.A = nullptr;
			RHS.Max = 0;
			RHS.Size = 0;
		}


		inline ~Vector()
        {
            if (A && Allocator)
                Allocator->_aligned_free(A);
            A = nullptr;
            Allocator = nullptr;
        }


		inline			Ty& operator [](size_t index) noexcept          { return A[index]; }
		inline const	Ty& operator [](size_t index) const noexcept    { return A[index]; }


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
			Allocator = RHS.Allocator;

			Release();

			A       = RHS.A;
			Max     = RHS.Max;
			Size    = RHS.Size;

			RHS.Size    = 0;
			RHS.Max     = 0;
			RHS.A       = nullptr;

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

        Ty back() const
        {
            FK_ASSERT(Size > 0);
            return A[Size - 1];
        }

		/************************************************************************************************/


		void resize(const size_t newSize)
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


		size_t push_back(const Ty& in) {
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
				FK_ASSERT(NewMem != nullptr);
				if (Size)
					FK_ASSERT(NewMem != A);
#endif

				if (A) {
					size_t itr = 0;
					size_t End = Size;
					for (; itr < End; ++itr)
						NewMem[itr] = std::move(A[itr]);

					Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}

			const size_t idx = Size++;
            new(A + idx) Ty{ in }; // 
			return idx;
		}


		/************************************************************************************************/

		template<typename ... ARGS_t>
		size_t emplace_back(ARGS_t&& ... in) {
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
					size_t End = Size;
					for (; itr < End; ++itr)
						new(NewMem + itr) Ty{ std::move(A[itr]) };

					Allocator->_aligned_free(A);
				}

				A = NewMem;
				Max = NewSize;
			}

			const size_t idx = Size++;

			new(A + idx) Ty(std::forward<ARGS_t>(in)...);
			return idx;
		}


		/************************************************************************************************/


        void insert(Iterator I, const Ty& in)
        {
            if (!size()) {
                push_back(in);
                return;
            }

            auto II     = end() - 1;

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
			auto itr = first;
			while (itr != last)
			{
				remove_stable(first);
				++itr;
			}
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
			A[--Size].~Ty();

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


        Ty* data()
        {
            return A;
        }


        /************************************************************************************************/


		void clear()
		{
			for (size_t i = 0; i < size(); ++i) {
				(A + i)->~Ty();
			}
			Size = 0;
		}


		/************************************************************************************************/


		void Release()
		{
			clear();

			if (A && Allocator)
				Allocator->_aligned_free(A);

			A = nullptr;
			Max = 0;
		}


		/************************************************************************************************/


        Ty& at(size_t index)                { return A[index]; }
        const Ty& at(size_t index) const    { return A[index]; }


		Iterator begin() { return A; }
		Iterator end() { return A + Size; }

		Iterator_const begin()	const { return A; }
		Iterator_const end()	const { return A + Size; }
		size_t size()			const { return Size; }


		/************************************************************************************************/


		Ty*	A                   = nullptr;

		size_t Size             = 0;
		size_t Max              = 0;

		iAllocator* Allocator   = 0;


		/************************************************************************************************/
	};


	template< typename Ty_Get, template<typename Ty, typename... Ty_V> class TC, typename Ty_, typename... TV2> Ty_Get GetByType(TC<Ty_, TV2...>& in) { return in.GetByType<Ty_Get>(); }

	template<typename Ty_1, typename Ty_2>
	struct Pair
	{
		typedef Pair<Ty_1, Ty_2> ThisType;

		template<typename Ty_Get, typename Ty_2>	static  inline Ty_Get& _GetByType(Pair<Ty_Get, Ty_2>&		in) { return in.V1; }
		template<typename Ty_Get, typename Ty_2>	static  inline Ty_Get& _GetByType(Pair<Ty_2, Ty_Get>&		in) { return in.V2; }
		template<typename Ty_Get>					static  inline Ty_Get& _GetByType(Pair<Ty_Get, Ty_Get>&	in) { static_assert(false, "NON_UNIQUE TYPES IN Pair!");  return in.V2; }
		template<typename Ty_Get>							inline Ty_Get&  GetByType() { return _GetByType<Ty_Get>(*this); }

		template<typename Ty_Assign> ThisType operator = (const Ty_Assign& in) { Ty_Assign& thisVar = GetByType<Ty_Assign>(); thisVar = in; return *this; }

		explicit operator bool() { return GetByType<bool>(); }
		template<typename Ty_1>	operator Ty_1() { return GetByType<Ty_1>(); }

		template<size_t index>	auto& Get() { static_assert(false, "Invalid Index"); }
		template<>	constexpr auto& Get<0>() noexcept { return V1; }
		template<>	constexpr auto& Get<1>() noexcept { return V2 ;}

		template<size_t index>	const auto& Get() const { static_assert(false, "Invalid Index"); }
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

			_Size = std::min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;

			return true;
		}

		bool push_back(Ty&& Item) noexcept
		{
			_Size = std::min(++_Size, SIZE);
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

			_Size = min(++_Size, SIZE);
			size_t idx = _Head++;
			_Head = _Head % SIZE;
			Buffer[idx] = Item;

			return true;
		}

		template<typename FN>
		bool push_back(Ty&& Item, FN callOnTail) noexcept
		{
			_Size = min(++_Size, SIZE);
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
				Idx--;
			}

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

            bool operator <		(Const_CircularIterator rhs) { return Idx < rhs.Idx; }
            bool operator ==	(Const_CircularIterator rhs) { return Idx == rhs.Idx; }

            bool operator !=	(Const_CircularIterator rhs) { return !(*this == rhs); }


            void Increment()
            {
                Idx++;
            }

            void Decrement()
            {
                Idx--;
            }

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

		int _Head, _Size;
		Ty Buffer[SIZE];
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

			bool try_LockLeft(uint32_t flag)
			{
				return !left->Lock(flag);
			}

			bool try_LockRight(uint32_t flag)
			{
				return !right->Lock(flag);
			}

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
			Pool{ reinterpret_cast<TY*>(Allocator->_aligned_malloc(sizeof(TY) * PoolSize)) },
			FreeObjectList{ Allocator },
			PoolMaxSize{ PoolSize },
			Allocator{ Allocator }
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


    /************************************************************************************************/


    template<typename TY_COL, typename FN_FilterOP>
    auto filter(TY_COL collection, const FN_FilterOP filter_op)
    {
        auto res = std::remove_if(begin(collection), end(collection), [&](auto& i) { return !filter_op(i); });
        collection.erase(res, end(collection));

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
					itr = ? itr->prev_ptr : nullptr;

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
					_ptr = ? _ptr->prev_ptr : nullptr;

				return *this;
			}


			Iterator& operator -= (size_t rhs) noexcept
			{
				for (size_t i = 0; i < rhs; ++i)
					_ptr = ? _ptr->prev_ptr : nullptr;

				return { itr };
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


    template<unsigned int storage = 64 - sizeof(void*) * 2, typename TY_return = void, typename ... TY_ARGS>
    class TypeErasedCallable
    {
    public:
        TypeErasedCallable() = default;


        template<typename TY_CALLABLE>
        TypeErasedCallable(const TY_CALLABLE& callable) noexcept
        {
            Assign(callable);
        }


        TypeErasedCallable(const TypeErasedCallable& callable) noexcept
        {
            memcpy(buffer, callable.buffer, sizeof(buffer));

            proxy       = callable.proxy;
            destructor  = callable.destructor;
        }


        TypeErasedCallable(TypeErasedCallable&& callable) noexcept
        {
            memcpy(buffer, callable.buffer, sizeof(buffer));

            proxy       = callable.proxy;
            destructor  = callable.destructor;

            callable.proxy      = nullptr;
            callable.destructor = nullptr;
        }


        ~TypeErasedCallable()
        {
            if (destructor)
                destructor(buffer);
        }


        template<typename TY_CALLABLE>
        TypeErasedCallable& operator = (TY_CALLABLE callable)
        {
            if (destructor)
                destructor(buffer);

            Assign(callable);

            return *this;
        }


        TypeErasedCallable& operator = (TypeErasedCallable&) = delete;


        TypeErasedCallable& operator = (TypeErasedCallable&& rhs)
        {
            if (destructor)
                destructor(buffer);

            memcpy(buffer, rhs.buffer, sizeof(buffer));
            proxy       = rhs.proxy;
            destructor  = rhs.destructor;

            return *this;
        }


        template<typename TY_CALLABLE>
        void Assign(TY_CALLABLE& callable)
        {
            static_assert(sizeof(TY_CALLABLE) <= storage, "Callable object too large for this TypeErasedCallable!");

            struct data
            {
                TY_CALLABLE callable;
            };

            new(buffer) data{ callable };

            proxy = [](char* _ptr, TY_ARGS ... args) -> TY_return
                {
                    auto functor = reinterpret_cast<data*>(_ptr);
                    return functor->callable(args...);
                };

            destructor = [](char* _ptr)
                {
                    auto functor = reinterpret_cast<data*>(_ptr);
                    functor->~data();
                };
        }


        auto operator()(TY_ARGS... args)
        {
            return proxy(buffer, std::forward<TY_ARGS>(args)...);
        }


    private:
        typedef TY_return(*fnProxy)(char*, TY_ARGS ... args);
        typedef void (*fnDestructor)(char*);

        fnProxy         proxy       = nullptr;
        fnDestructor    destructor  = nullptr;
        char            buffer[storage];
    };


}	// namespace FlexKit;
	/************************************************************************************************/
#endif
#endif
