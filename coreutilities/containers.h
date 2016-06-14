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

// Define the Containers used in the application here
#ifndef CONTAINERS_H
#define CONTAINERS_H

#include "..\buildsettings.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\fixed_vector.h"
#include "..\coreutilities\static_vector.h"

#if USING(USESTL)

#include <deque>
#include <list>
#include <new>
#include <map>
#include <vector>
#include <stdint.h>

template<typename Ty>					using deque_t	= std::deque<Ty>;
template<typename Ty>					using vector_t	= std::vector<Ty>;
template<typename Ty, typename Ty_key>	using map_t		= std::map<Ty, Ty_key>;
template<typename Ty>					using list_t	= std::list<Ty>;

/************************************************************************************************/

namespace FlexKit
{
	template<typename Ty, size_t SIZE = 16>
	struct RingBuffer
	{
		RingBuffer(){
			head	= 0;
			end		= 0;
			count	= 0;
		}

		void push_back(const Ty& in){
			_t[end++] = in;
			end = end % SIZE;
			++count;
			count = count % SIZE;
			if (head == end)
				head++;
		}

		Ty pop_front(){
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


	template<typename Ty>
	struct DynArray
	{
		typedef DynArray<Ty> THISTYPE;
		inline  DynArray(iAllocator* Alloc = nullptr) : Allocator(Alloc), Max(0), Size(0), A(nullptr) {}

		inline  DynArray(THISTYPE&& RHS) : 
			A			(RHS.A),
			Allocator	(RHS.Allocator),
			Max			(RHS.Max),
			Size		(RHS.Size)
		{
			RHS.A	 = nullptr;
			RHS.Max  = 0;
			RHS.Size = 0;
		}

		inline ~DynArray(){if(A)Allocator->_aligned_free(A);}

		inline			Ty& operator [](size_t index)		{return A[index];}
		inline const	Ty& operator [](size_t index) const {return A[index];}


		/************************************************************************************************/


		Ty PopVect(){
			FK_ASSERT(C->Size >= 1);
			auto temp = C->A[--C->Size];
			A[C->Size].~Ty();
			return temp;
		}


		/************************************************************************************************/
		
		
		Ty& front()	{
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

		void push_back(Ty in) {
			if (Size + 1 > Max)
			{// Increase Size
#ifdef _DEBUG
				FK_ASSERT(Allocator);
#endif			
				Ty* NewMem = (Ty*)Allocator->_aligned_malloc(sizeof(Ty) * 2 * max(Max, 1));
				
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


		/************************************************************************************************/


		void pop_back()
		{
			A[Size--].~Ty();
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
					size_t End = Size + 1;
					for (; itr < End;) NewMem[itr] = A[itr];
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


		void Release()
		{
			clear();

			if(A)
				Allocator->_aligned_free(A);

			A	= nullptr;
			Max = 0;
		}


		/************************************************************************************************/


		Ty& at(size_t index) { return A[index]; }

		typedef Ty* Iterator; 

		Iterator begin()	{ return A;			}
		Iterator end()		{ return A + Size;	}

		size_t size(){return Size;}

		/************************************************************************************************/
		
		
		Ty*	A;

		size_t Size;
		size_t Max;

		iAllocator* Allocator;


		/************************************************************************************************/
	};

	template<typename Ty>
	void MoveDynArray(DynArray<Ty>& Dest, DynArray<Ty>& Source)
	{
		FK_ASSERT(&Dest != &Source);
		Dest.Release();

		Dest.A			= Source.A;
		Dest.Allocator	= Source.Allocator;
		Dest.Size		= Source.Size;
		Dest.Max		= Source.Max;

		Source.A    = nullptr;
		Source.Size = 0;
		Source.Max  = 0;
	}


	/************************************************************************************************/


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
				break;

		return itr;
	}

	template<typename TY_C, typename TY_PRED>
	auto find(TY_C& C, TY_PRED _Pred) noexcept
	{
		auto itr = C.begin();
		auto end = C.end();
		for (; itr != end; ++itr)
			if (_Pred(*itr))
				break;

		return itr;
	}


	/************************************************************************************************/
}
#endif
#endif