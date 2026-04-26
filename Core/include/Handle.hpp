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

#pragma once

#include "BuildSettings.hpp"
#include "Containers.hpp"
#include "Type.hpp"

#include <stdint.h>


namespace FlexKit
{
	typedef uint32_t index_t;

	struct _InvalidHandle_t {} inline static const InvalidHandle;

	template<size_t HandleSize = 32, size_t ID = (size_t)-1, typename InternalType = uint32_t>
	class Handle_t
	{
	public:
		constexpr static const size_t GetHandleSize() { return HandleSize; }

		constexpr Handle_t()									= default;
		constexpr Handle_t(			Handle_t&	in)	noexcept	= default;
		constexpr Handle_t(const	Handle_t&	in)	noexcept	= default;
		constexpr Handle_t(			Handle_t&&	in)	noexcept	= default;

		template<typename TY> requires std::is_integral_v<TY>
		constexpr Handle_t(const TY in)					noexcept : INDEX{ (InternalType)in } {}
		constexpr Handle_t(_InvalidHandle_t)
		{
			INDEX = (HandleSize >= (sizeof(InternalType) * 8) ? -1 : (InternalType(1) << (HandleSize - 1)) - 1) & (0xffffffffffffffff);
		}

		//operator uint32_t() const { return to_uint(); }

		bool				operator ==	(const Handle_t in) const
		{
#if USING( DEBUGHANDLES )
			if (TYPE == in.TYPE && FLAGS == in.FLAGS && INDEX == in.INDEX)
				return true;
#else
			if (INDEX == in.INDEX)
				return true;
#endif
			return false;
		}

		bool operator !=	(const Handle_t in) const
		{
			return !(*this == in);
		}

		const InternalType	to_uint() const
		{
			return INDEX;
		}


		Handle_t& operator =	(const	Handle_t& in)	noexcept = default;
		Handle_t& operator =	(Handle_t&& in)			noexcept = default;

		Handle_t<HandleSize, ID> operator = (_InvalidHandle_t) noexcept
		{
			INDEX = (HandleSize >= (sizeof(InternalType) * 8) ? -1 : (InternalType(1) << (HandleSize - 1)) - 1) & (0xffffffffffffffff);
			return {};
		}

		template<typename TY> requires std::is_integral_v<TY>
		bool operator == (TY x)
		{
			return x == INDEX;
		}

		bool operator == (_InvalidHandle_t)
		{
			return (*this == Handle_t{ InvalidHandle });
		}

		bool operator != (_InvalidHandle_t handle)
		{
			return !(*this == Handle_t{ InvalidHandle });
		}

		operator InternalType(){ return INDEX; }


		InternalType   INDEX : HandleSize;
#if USING( DEBUGHANDLES )
		unsigned int	TYPE		: 28            = 0;
		unsigned int	FLAGS		: 4             = 0;
#endif

		operator InternalType () const { return INDEX; }

		enum HANDLE_FLAGS
		{
			HF_ERROR = 0x0001,
			HF_FREE	 = 0x0002,
			HF_USED	 = 0x0004
		};
	};


	template<size_t ID>
	class Handle_t<64, ID, uint64_t>
	{
	public:
		constexpr static const size_t GetHandleSize() { return 64; }

		constexpr Handle_t()									= default;
		constexpr Handle_t(			Handle_t&	in)	noexcept	= default;
		constexpr Handle_t(const	Handle_t&	in)	noexcept	= default;
		constexpr Handle_t(			Handle_t&&	in)	noexcept	= default;

		template<typename TY> requires std::is_integral_v<TY>
		constexpr Handle_t(const TY in)					noexcept : INDEX{ (uint64_t)in } {}
		constexpr Handle_t(_InvalidHandle_t)
		{
			INDEX = 0xffffffffffffffff;
		}

		bool				operator ==	(const Handle_t in) const
		{
#if USING( DEBUGHANDLES )
			if (TYPE == in.TYPE && FLAGS == in.FLAGS && INDEX == in.INDEX)
				return true;
#else
			if (INDEX == in.INDEX)
				return true;
#endif
			return false;
		}

		bool operator !=	(const Handle_t in) const
		{
			return !(*this == in);
		}

		const uint64_t	to_uint() const
		{
			return INDEX;
		}


		Handle_t& operator =	(const	Handle_t& in)	noexcept = default;
		Handle_t& operator =	(Handle_t&& in)			noexcept = default;

		Handle_t operator	= (_InvalidHandle_t) noexcept
		{
			INDEX = 0xffffffffffffffff;
			return {};
		}

		template<typename TY> requires std::is_integral_v<TY>
		bool operator == (TY x)
		{
			return x == INDEX;
		}

		bool operator == (_InvalidHandle_t)
		{
			return (*this == Handle_t{ InvalidHandle });
		}

		bool operator != (_InvalidHandle_t handle)
		{
			return !(*this == Handle_t{ InvalidHandle });
		}

		operator uint64_t(){ return INDEX; }


		uint64_t   INDEX;
#if USING( DEBUGHANDLES )
		unsigned int	TYPE		: 28            = 0;
		unsigned int	FLAGS		: 4             = 0;
#endif

		operator uint64_t () const { return INDEX; }

		enum HANDLE_FLAGS
		{
			HF_ERROR = 0x0001,
			HF_FREE	 = 0x0002,
			HF_USED	 = 0x0004
		};
	};

	/************************************************************************************************/


	template<
		typename TY_HANDLE_OUT,
		typename TY_HANDLE_IN>
		TY_HANDLE_OUT handle_cast(TY_HANDLE_IN in)
	{
		static_assert(TY_HANDLE_OUT::GetHandleSize() == TY_HANDLE_IN::GetHandleSize(), "Handles must be equal size!");

		return TY_HANDLE_OUT{ in.INDEX };
	}



	/************************************************************************************************/


	typedef Handle_t<32>					Handle;
	template<size_t SIZE>	using Handle_static_vector = static_vector<Handle, SIZE>;

	namespace HandleUtilities
	{
		// TODO RESORT HANDLES AFTER FREE LIST FILLS

		template<typename HANDLE>
		struct HandleTable
		{
			HandleTable(iAllocator* Memory = nullptr, const Type_t type = 0x00 ) : Indexes(Memory), mType( type ) {}

			void Initiate( iAllocator* Memory )
			{
				FreeList            = -1;
				Indexes.Allocator   = Memory;
			}

			index_t&	operator[] ( const HANDLE in )
			{
				return Indexes[ in.INDEX ];
			}

			index_t	operator[] ( const HANDLE in ) const	{return Indexes[ in.INDEX ];}

			index_t&	Get( const HANDLE in )					{return Indexes[ in.INDEX ];}
			index_t		Get( const HANDLE in ) const			{return Indexes[ in.INDEX ];}

			[[nodiscard]]
		    HANDLE GetNewHandle(index_t initialIndex = -1)
			{
				if (FreeList != -1) {
					const auto idx = FreeList;
					FreeList = Indexes[idx];
					Indexes[idx] = initialIndex;

					return { (index_t)idx };
				}

				return { (index_t)Indexes.push_back(initialIndex) };
			}

			void Clear()
			{
				Indexes.clear();
			}

			bool Has( Handle find_this_Handle ) const
			{
				if( std::find( Indexes.begin(), Indexes.end(), find_this_Handle.INDEX ) !=
					Indexes.end() )
					return true;
				return false;
			}

			void RemoveHandle( HANDLE in )
			{
				Indexes[in] = FreeList;
				FreeList    = (index_t)in;
			}

			size_t size()
			{
				return Indexes.size();
			}

			size_t size() const
			{
				return Indexes.size();
			}

			typedef typename static_vector<index_t>::iterator iterator;

			iterator	begin()
			{
				return Indexes.begin();
			}

			iterator	end()
			{
				return Indexes.end();
			}

			HANDLE find(size_t idx)
			{
				for (size_t I = 0; I < Indexes.size(); ++I)
					if(Indexes[I] == idx)
						return HANDLE(I);

				return InvalidHandle;
			}

			bool IsValid(HANDLE handle) const
			{
				return ((handle.INDEX < -1) && (Indexes[handle.INDEX] != (index_t)0xffffffffffffffff));
			}

			HandleTable( const HandleTable<HANDLE>& in )				= delete;	// Do not allow Table copying
			HandleTable& operator = ( const HandleTable<HANDLE>& rhs )	= delete;	// Do not allow Table copying

			index_t         FreeList = -1;
			Vector<index_t> Indexes;

			void Release()
			{
				Indexes.Release();
			}

			const Type_t mType;
		};


		/************************************************************************************************/
	}

	typedef Handle_t<16> EntityHandle;
	//typedef size_t EntityHandle;

}
