#pragma once
#ifndef static_vector_h
#define static_vector_h

/**********************************************************************
 
Copyright (c) 2014-2018 Robert May
 
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

	template<typename TY_, unsigned int TSIZE = 16>
	class static_vector
	{
	public:
		using value_type    = TY_;
		using iterator      = TY_*;

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

			bool operator == (const iterator_t rhs) const { return rhs.I == I; }
			bool operator != (const iterator_t rhs) const { return !(*this == rhs); }

			operator TY_* () {return I;}

			TY_* I;
		};

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


		static_vector() = default;


		static_vector(const std::initializer_list<TY_> list) : static_vector()
		{
			for( auto e : list )
				push_back( e );
		}


		static_vector(size_t initialSize) : static_vector()
		{
			for (size_t I = 0; I < initialSize; I++)
				emplace_back();
		}


		template<unsigned int RHSIZE = 10> requires(std::is_trivially_copyable_v<TY_>)
		static_vector(const static_vector<TY_, RHSIZE>& in)
		{
			Size = in.size();
			memcpy(buffer, in.data(), in.size() * sizeof(TY_));
		}


		template<unsigned int RHSIZE = 10> requires(std::is_trivially_copyable_v<TY_>)
		static_vector(static_vector<TY_, RHSIZE>&& in)
		{
			Size = in.size();
			memcpy(buffer, in.data(), in.size() * sizeof(TY_));

			in.Size = 0;
		}


		template<unsigned int RHSIZE = 10> requires(!std::is_trivially_copyable_v<TY_>)
		static_vector(static_vector<TY_, RHSIZE>&& in)
		{
			for (auto&& i : in)
				if (Size != TSIZE)
					emplace_back(std::move(i));
				else
					return;

			in.Size = 0;
		}


		template<unsigned int RHSIZE = 10>
		static_vector(const static_vector<TY_, RHSIZE>& in)
		{
			clear();

			for (auto i : in)
				if (Size != TSIZE)
					push_back(i);
				else
					return;
		}


		~static_vector()
		{
			if constexpr (!std::is_trivially_destructible_v<TY_>)
			{
				while (!empty())
					pop_back();
			}
		}

		static_vector& operator = (const TY_& in)
		{
			FK_ASSERT(TSIZE > in.size());

			if constexpr (std::is_trivially_copyable_v<TY_>)
			{
				Size = in.size();

				memcpy(buffer, in.buffer, in.size() * sizeof(TY_));
			}
			else
			{
				Size = 0;

				for (auto i : in)
					if (Size != TSIZE)
						push_back(i);
					else
						break;
			}

			return *this;
		}


		operator TY_* () { return reinterpret_cast<TY_*>(buffer); }


		TY_* at_ptr(size_t idx)
		{
			return reinterpret_cast<TY_*>(buffer) + idx;
		}

		const TY_* at_ptr(size_t idx) const
		{
			return reinterpret_cast<const TY_*>(buffer) + idx;
		}


		TY_& at(const size_t index)
		{
			return *at_ptr(index);
		}


		const TY_& at(const size_t index) const
		{
			return *at_ptr(index);
		}


		TY_& operator [](size_t index)
		{
			return at(index);
		}

		const TY_& operator [](size_t index) const
		{
			return at(index);
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
			return at(Size - 1);
		}


		const TY_& back() const
		{
			return at(Size - 1);
		}


		size_t push_back( const TY_& in )
		{
			if (!full())
			{
				new(at_ptr(Size)) TY_( in );
				return Size++;
			}

			return -1;
		}


		template<typename ... TY_ARGS>
		void emplace_back(TY_ARGS&& ... in_args)
		{
			if (!full())
			{
				new(at_ptr(Size)) TY_{ std::forward<TY_ARGS>(in_args)...};
				Size++;
			}
		}


		TY_* begin()
		{
			return at_ptr(0);
		}

 
		TY_* end()
		{
			return at_ptr(Size);
		}


		reverse_iterator rbegin()
		{
			return at_ptr(Size - 1);
		}


		reverse_iterator rend()
		{
			return at_ptr(Size - 1);
		}


		void remove_unstable(iterator i)
		{
			if (i == end())
				return;

			*i = back();
			pop_back();
		}

		iterator erase(iterator i, iterator _end)
		{
			size_t count = _end - i;
			while (count)
			{
				if (count && i < end()){
					i->~value_type();
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
			pop_back();

			return i;
		}
 
		const TY_* begin() const
		{
			return at_ptr(0);
		}
 
		const TY_* end() const
		{
			return at_ptr(Size);
		}
 
		void pop_back()
		{
			if (!Size)
				return;
			Size--;
			(at_ptr(Size))->~TY_();
		}
 
		TY_& front()
		{
			return at(0);
		}

		const TY_& front() const noexcept
		{
			return at(0);
		}

		size_t size() const noexcept
		{
			return Size;
		}

		size_t ByteSize() const noexcept
		{
			return sizeof(buffer);
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
			if constexpr (std::is_trivial_v<TY_>)
			{
				Size = 0;
			}
			else
			{
				while(!empty())
					pop_back();
			}
		}

		TY_* data()
		{
			return reinterpret_cast<TY_*>(buffer);
		}

		const TY_* data() const
		{
			return reinterpret_cast<const TY_*>(buffer);
		}
 
		void resize(size_t NewSize)
		{
			if (NewSize < size())
			{
				const size_t end = size() - NewSize;

				for (size_t I = 0; I < end; ++I)
					pop_back();
			}
			else if (NewSize > size())
			{
				const size_t end = NewSize - size();

				for (size_t I = size(); I < end; ++I)
					new(at_ptr(I + size())) TY_();
			}

			Size = NewSize;
		}

		void SetFull()
		{
			while( !full() )
			{
				new(at_ptr(Size)) TY_();
				++Size;
			}
		}
		
		// USE WITH CAUTION!
		template<typename TY_OUT>
		static_vector<TY_OUT*>& _PTR_Cast()
		{
			return *((static_vector<TY_OUT*>*)(this));
		}

		template<typename Pre_>
		static void Sort(iterator begin, iterator end, Pre_ PD)
		{
			std::sort(begin, end, PD);
		}
		
		typedef TY_ TYPE;

		size_t  Size = 0;                       // Used Counter
		char	Padding[0x10 - sizeof(size_t)];	// 8-Byte Pad to keep 16byte Alignment of Elements
		char    buffer[TSIZE * sizeof(TY_)];
	};
}

#endif
