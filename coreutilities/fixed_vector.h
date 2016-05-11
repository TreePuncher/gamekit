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

#ifndef FIXED_VECTOR_H
#define FIXED_VECTOR_H

#pragma warning(disable:4200)

namespace FlexKit
{
	// Like Vector but of size determined at Instantiation, not compile Time
	// Over-Flow Safe
	// Element Position Not Guaranteed 
	template<typename TY_>
	class fixed_vector
	{
	public:
		typedef fixed_vector<TY_> THIS_TYPE;
		class iterator_t
		{
		public:
			iterator_t(TY_* i) : I(i) {}

			TY_& operator *() { return *I; };
			TY_* operator ->() { return I; };

			iterator_t operator ++ () { ++I; return *this; }
			iterator_t operator -- () { --I; return *this; }
			TY_* operator ++ (int) { I++; return I - 1; }
			TY_* operator -- (int) { I++; return I - 1; }
			size_t operator - (iterator_t i) { return I - i.I; }

			operator TY_* () { return I; }

			TY_* I;
		};
		typedef typename iterator_t iterator;

		class reverse_iterator_t
		{
		public:
			reverse_iterator_t(TY_* i) : I(i) {}

			inline TY_& operator *() { return *I; };
			inline TY_* operator ->() { return I; };

			inline void operator ++ () { --I; }
			inline void operator -- () { ++I; }
			inline TY_* operator ++ (int) { return I--; }
			inline TY_* operator -- (int) { return I++; }

			inline operator TY_* () { return I; }

			TY_* I;
		};
		typedef typename reverse_iterator_t reverse_iterator;

		fixed_vector(size_t Size) : Length(Size), Size(0)
		{
#ifdef _DEBUG
			if (sizeof(TY_) == sizeof(byte*))
				memset(Elements, 0, sizeof(TY_) * Length);
#endif
		}

		fixed_vector(std::initializer_list<TY_> list) : static_vector()
		{
			for (auto e : list)
				push_back(e);
		}

		fixed_vector(const fixed_vector<TY_>& in)
		{
			Size = 0;

			for (auto i : in)
				if (Size < Length)
					push_back(i);
				else
					return;
		}


		TY_& operator [](const size_t index)
		{
			return Elements[index];
		}

		THIS_TYPE& operator+=(const TY_ in)
		{
			push_back(in);
		}

		THIS_TYPE& operator+=(const THIS_TYPE& in)
		{
			for (const auto& i : in)
			{
				if (!full())
					return this;

				push_back(i);
			}
			return *this;
		}

		THIS_TYPE& operator = (THIS_TYPE& in)
		{
			if (in.size() < size())
			{
				for (size_t I = 0; I < size(); ++I)
					pop_back();

				for (auto& I : in)
					push_back(I);
			}
			return *this;
		}

		const TY_& operator [](size_t index) const
		{
			return Elements[index];
		}

		TY_& at(const size_t index)
		{
			return Elements[index];
		}

		const TY_& at(const size_t index) const
		{
			return Elements[index];
		}


		TY_& back()
		{
			return Elements[size() - 1];
		}

		void push_back(const TY_& in)
		{
			if (!full())
			{
				new(&Elements[Size]) TY_(in);
				Size++;
			}
		}

		iterator begin()
		{
			return Elements;
		}

		iterator end()
		{
			return &Elements[Size];
		}

		reverse_iterator rbegin()
		{
			return &Elements[Size - 1];
		}

		reverse_iterator rend()
		{
			return Elements - 1;
		}

		iterator erase(iterator i, iterator _end)
		{
			size_t count = _end - i;
			while (count)
			{
				if (count && i < end()) {
					i.I->~TY_();
					*i++ = back();
					pop_back();
					count--;
				} else {
					pop_back();
					count--;
				}
			}
			return i;
		}

		iterator erase(iterator i)
		{
			i->~TY_();
			*i++ = back();
			--Size;

			return i;
		}

		const TY_* begin() const
		{
			return Elements;
		}

		const TY_* end() const
		{
			return &Elements[Size];
		}

		void pop_back()
		{
			if (!Size)
				return;
			Size--;
			(&Elements[Size])->~TY_();
		}

		TY_ front()
		{
			return Elements[0];
		}

		size_t size()
		{
			return Size;
		}

		size_t	max_length()
		{
			return Length;
		}

		bool full() const
		{
			return !(Size < Length);
		}

		bool empty() const
		{
			return (Size == 0);
		}

		void clear()
		{
			auto BeforeSize = Size;
			for (size_t I = 0; I < BeforeSize; ++I)
				pop_back();
		}

		void SetFull()
		{
			while (!full())
			{
				new(&Elements[Size]) TY_();
				++Size;
			}
		}

		template<typename Pre_>
		static void Sort(iterator begin, iterator end, Pre_ PD) { std::sort(begin.I, end.I, PD); }
		static void Sort(iterator begin, iterator end) { std::sort(begin.I, end.I); }
		void Sort() { std::sort(begin().I, end().I); }

		template<typename ALLOC>
		static fixed_vector& Create(size_t size, ALLOC* Allocator)
		{
			char* Buffer = (char*)Allocator->malloc(sizeof(THIS_TYPE) + sizeof(TY_) * size);
			fixed_vector<TY_>* newVector = new(Buffer) fixed_vector<TY_>(size);
			return *newVector;
		}

		static fixed_vector& Create_Aligned(size_t size, iAllocator* Allocator, size_t A = 0x10)
		{
			char* Buffer = (char*)Allocator->_aligned_malloc(sizeof(THIS_TYPE) + sizeof(TY_) * size);
			fixed_vector<TY_>* newVector = new(Buffer) fixed_vector<TY_>(size);
			return *newVector;
		}

	private:
		
		uint64_t Size;		// Used Counter
		uint64_t Length;	// Element[] size
		TY_      Elements[];
	};

	template<typename TY_Allocator, typename TY_>
	fixed_vector<TY_>* ExpandFixedBuffer(TY_Allocator* Memory, fixed_vector<TY_>* in) {
		auto NewBuffer = &TempResourceList::Create_Aligned(in->size() * 2, Memory);
		RawMove(*NewBuffer, *in);
		Memory->free(in);

		return NewBuffer;
	}

	template<typename TY_>
	static void RawMove(fixed_vector<TY_>& lhs, fixed_vector<TY_>& rhs)	{
		if (rhs.size() < lhs.max_length())
			memcpy(lhs.begin(), rhs.begin(), sizeof(TY_) * rhs.size());
	}

}

#endif