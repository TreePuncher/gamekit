#pragma once
/**********************************************************************
 
Copyright (c) 2014-2017 Robert May
 
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
 
#include <algorithm>
#include <initializer_list>

namespace FlexKit
{
// Like Vector but of static size
// Over-Flow Safe
// Element Position Not Guaranteed 

	template< typename TY_, unsigned int TSIZE = 16 >
	class static_vector
	{
	public:
		class iterator_t
		{
		public:
			iterator_t(TY_* i) : I(i){}
			
			TY_& operator *(){return *I;};
			TY_* operator ->(){return I;};

			iterator_t operator ++ () { ++I; return *this; }
			iterator_t operator -- () { --I; return *this; }
			TY_* operator ++ (int) { I++; return I - 1; }
			TY_* operator -- (int) { I++; return I - 1; }
			size_t operator - (iterator_t i) { return I - i.I; }


			operator TY_* () {return I;}

			TY_* I;
		};
		typedef typename iterator_t iterator;

		class reverse_iterator_t
		{
		public:
			reverse_iterator_t(TY_* i) : I(i){}
			
			inline TY_& operator *(){return *I;};
			inline TY_* operator ->(){return I;};

			inline void operator ++ (){--I;}
			inline void operator -- (){++I;}
			inline TY_* operator ++ (int){return I--;}
			inline TY_* operator -- (int){return I++;}

			inline operator TY_* () {return I;}

			TY_* I;
		};
		typedef typename reverse_iterator_t reverse_iterator;

		static_vector()
		{
#ifdef _DEBUG
			if( sizeof( TY_ ) == sizeof( byte* ) )
				memset( Elements, 0, sizeof( Elements ) );
#endif
			Size = 0;
		}

		static_vector(std::initializer_list<TY_> list) : static_vector()
		{
			for( auto e : list )
				push_back( e );
		}
		template< unsigned int RHSIZE = 10 >
		static_vector(const static_vector<TY_, RHSIZE>& in)
		{
			Size = 0;
 
			for (auto i : in)
				if (Size != TSIZE)
					push_back(i);
				else
					return;
		}
 
		operator TY_* () { return Elements; }
 
		TY_& operator [](size_t index)
		{
			return Elements[index];
		}

		const TY_& operator [](const size_t index) const
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

		static_vector& operator+=(const TY_ in)
		{
			push_back(in);
		}
 
		template< unsigned int RHSIZE>
		static_vector& operator+=(const static_vector<TY_, RHSIZE>& in)
		{
			for (const auto& i : in)
			{
				if (full())
					return *this;
 
				push_back(i);
			}
			return *this;
		}
 
		
		TY_& back()
		{
			return Elements[size() - 1];
		}
 
		void push_back( const TY_& in )
		{
			if (!full())
			{
				new(&Elements[Size]) TY_( in );
				Size++;
			}
		}

		void push_back(TY_&& in)
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
				if (count && i < end()){
					i.I->~TY_();
					*i++ = back();
					pop_back();
					count--;
				}
				else {
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
 
		size_t size() const
		{
			return Size;
		}
 
		bool full() const
		{
			return !(Size < TSIZE);
		}
 
		bool empty() const
		{
			return (Size == 0);
		}
 
		void clear()
		{
			for (const auto& I : Elements)
				pop_back();
		}
 
		void resize(size_t NewSize)
		{
			if (NewSize > size())
			{
				size_t end = NewSize - size();
				for (size_t I = 0; I < end; ++I) {
					pop_back();
				}
			}
			else if (NewSize < size())
			{
				size_t end = size() - NewSize;
				for (size_t I = 0; I < end; ++I) {
					new(Elements + (I + size())) TY_();
				}
			}

			Size = NewSize;
		}

		void SetFull()
		{
			while( !full() )
			{
				new(&Elements[Size]) TY_();
				++Size;
			}
		}
		
		// USE WITH CAUTION!
		template<typename TY_OUT>
		static_vector<TY_OUT*>& _PTR_Cast()
		{
			return *((static_vector<TY_OUT*>*)(this));
		}

		static void Sort(iterator begin, iterator end)
		{
			std::sort(begin.I, end.I);
		}

		template<typename Pre_>
		static void Sort(iterator begin, iterator end, Pre_ PD)
		{
			std::sort(begin.I, end.I, PD);
		}
		
	private:
		size_t          Size;							// Used Counter
		char			Padding[0x10 - sizeof(size_t)];	// 8-Byte Pad to keep 16byte Alignment of Elements

		TY_             Elements[TSIZE];
	};
}