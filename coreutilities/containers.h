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
		RingBuffer()
		{
			head	= 0;
			end		= 0;
			count	= 0;
		}

		void push_back(const Ty& in)
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
			if (count)
			{
				--count;
				count = count % SIZE;

				i = head++;
				head = head % SIZE;
			}
			return _t[i];
		}

		Ty& back()
		{
			return _t[end];
		}

		Ty& front()
		{
			return _t[head];
		}

		Ty& operator[] (uint32_t i)
		{

			return _t[i];
		}

		void clear()
		{
			count = 0;
			head = 0;
			end = 0;
		}

		uint32_t size()
		{
			return count;
		}

		Ty		_t[SIZE];
		size_t	head;
		size_t	end;
		size_t	count;
	};

	template<typename Ty, typename Ty_Allocator>
	struct DynArray
	{
		inline DynArray(Ty_Allocator& Alloc = Ty_Allocator) : Allocator(Alloc) {}
				
		inline Ty& operator [](size_t index) {return A[index];}
		Ty*	A;

		typedef Ty* Iterator; 

		Iterator begin()	{ return A;			}
		Iterator end()		{ return A + Size;	}

		size_t Size;
		size_t Max;

		Ty_Allocator Allocator;
	};

	template<typename Ty, typename ALLOC>
	void PushVectCM(DynArray<Ty, ALLOC>* C, Ty& in, FlexKit::BlockAllocator* mem)
	{
		if (C->Size + 1 > C->Max)
		{// Increase Size
			Ty* NewMem = (Ty*)mem->malloc(sizeof(Ty) * 2 * C->Max);
			
			FK_ASSERT(NewMem);

			{
				size_t itr = 0;
				size_t End = C->Max * 2;
				while (itr < End) new(NewMem + itr) ();
			}
			{
				size_t itr = 0;
				size_t End = C->Size;
				while (itr < End) NewMem[itr] = C->A[itr];
			}

			mem->free(C->A);
			C->A = NewMem;
			C->Size *= 2;
		}

		C->A[Size++] = in;
	}

	template<typename Ty, typename ALLOC>
	Ty PopVect(DynArray<Ty, ALLOC>* C)
	{
#ifdef _DEBUG
		FK_ASSERT(C->Size > 1);
#endif
		return C->A[--C->Size];
	}

	template<typename Ty, typename ALLOC>
	Ty& GetAt(DynArray<Ty, ALLOC>* C, size_t index){ return C->A[index]; }

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

		explicit operator bool()					{ return V1; }
		template<typename Ty_1>	operator Ty_1()		{ return GetByType<Ty_1>(); }

		Ty_1 V1;
		Ty_2 V2;
	};
}

/************************************************************************************************/

#endif
#endif