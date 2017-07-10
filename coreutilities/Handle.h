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

#pragma once

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\type.h"

#include <stdint.h>


namespace FlexKit
{
	typedef uint32_t index_t;

	template< typename int HandleSize = 32 >
	class Handle_t
	{
	public:
		Handle_t()
		{
#if USING( DEBUGHANDLES )
			INDEX = 0XFFFFFFFF;
			TYPE = 0XFFFF;
			FLAGS = HANDLE_FLAGS::HF_ERROR;
#endif
		}
		Handle_t(unsigned int index, unsigned int type, unsigned int flags)
			: INDEX(index)
#if USING( DEBUGHANDLES )
			, TYPE(type)
			, FLAGS(flags)
#endif
		{}

		Handle_t(const Handle_t<HandleSize>& in)
		{
			INDEX = in.INDEX;
#if USING( DEBUGHANDLES )
			TYPE = in.TYPE;
			FLAGS = in.FLAGS;
#endif
		}

		explicit Handle_t(unsigned int in)
		{
#if USING( DEBUGHANDLES )
			TYPE = 0XFFFF;
			FLAGS = HANDLE_FLAGS::HF_ERROR;
#endif
			INDEX = in;
		}

		operator uint32_t&() { return INDEX; }
		operator uint32_t() const { return this->to_uint(); }
		bool				operator ==	(const Handle_t<HandleSize> in) const
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

		bool operator !=	(const Handle_t<HandleSize> in) const
		{
			return !(*this == in);
		}

		const uint32_t	to_uint() const
		{
			return INDEX;
		}

		operator uint32_t(){ return INDEX; }
		unsigned int	INDEX		: HandleSize;
#if USING( DEBUGHANDLES )
		unsigned int	TYPE		: 16;
		unsigned int	FLAGS		: 16;
#endif

		operator size_t () { return INDEX; }

		enum HANDLE_FLAGS
		{
			HF_ERROR = 0x0001,
			HF_FREE	 = 0x0002,
			HF_USED	 = 0x0004
		};
	};

	/************************************************************************************************/

	template<int Size>
	inline bool operator < ( const Handle_t<Size>& lhs,  const Handle_t<Size>& rhs )
	{
		return lhs.to_uint() < rhs.to_uint();
	}

	/************************************************************************************************/
	typedef Handle_t<32>					Handle;
	typedef list_t<Handle>					Handle_list;
	typedef vector_t<Handle>				Handle_vector;
	typedef map_t<Handle, Handle_vector>	Handle_child_map;
	
	template<size_t SIZE>	using Handle_static_vector = static_vector<Handle, SIZE>;

	namespace HandleUtilities
	{
		// TODO RESORT HANDLES AFTER FREE LIST FILLS

		template<typename HANDLE, size_t SIZE = 128>
		struct HandleTable
		{
			HandleTable( const Type_t type = 0x00, iAllocator* Memory = nullptr ) : mType( type ), FreeList(Memory), Indexes(Memory) {}

			void Initiate( iAllocator* Memory )
			{
				FreeList.Allocator = Memory;
				Indexes.Allocator = Memory;
			}

			inline index_t&	operator[] ( const HANDLE in )
			{
				#ifdef _DEBUG
				FlexKit::HandleUtilities::CheckType( in, mType );
				#endif
				return Indexes[ in.INDEX ];
			}

			inline index_t	operator[] ( const HANDLE in ) const	{return Indexes[ in.INDEX ];}

			inline index_t&	Get( const HANDLE in )					{return Indexes[ in.INDEX ];}
			inline index_t	Get( const HANDLE in ) const			{return Indexes[ in.INDEX ];}

			inline HANDLE	GetNewHandle()
			{
				HANDLE NewHandle( 0, mType, FlexKit::Handle::HF_USED );

				if( FreeList.size() )
				{
					NewHandle.INDEX = FreeList.back();
					FreeList.pop_back();
				}
				else
				{
					NewHandle.INDEX = Indexes.size();
					Indexes.push_back( 0 );
				}

				return NewHandle;
			}

			inline void	Clear()
			{
				FreeList.clear();
				Indexes.clear();
			}

			inline bool	Has( Handle find_this_Handle ) const
			{
				if( std::find( Indexes.begin(), Indexes.end(), find_this_Handle.INDEX ) !=
					Indexes.end() )
					return true;
				return false;
			}

			inline void	RemoveHandle( HANDLE in )
			{
				if( in.INDEX < Indexes.size() )
					FreeList.push_back( Indexes[in.INDEX] );
				else
					FK_ASSERT( 0 );
			}

			inline size_t size()
			{
				return Indexes.size();
			}

			inline size_t size() const
			{
				return Indexes.size();
			}

			typedef typename static_vector<index_t, SIZE>::iterator iterator;
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

				return HANDLE(-1);
			}

			HandleTable( const HandleTable<HANDLE,SIZE>& in )	= delete;	// Do not allow Table copying
			HandleTable& operator = ( const HandleTable& rhs )	= delete;	// Do not allow Table copying

			Vector<index_t> FreeList;
			Vector<index_t> Indexes;

			void Release()
			{
				FreeList.Release();
				Indexes.Release();
			}

			const FlexKit::Type_t mType;
		};

		template<int Size>
		inline void CheckType( Handle_t<Size> hdnl_in, FlexKit::Type_t type_in )
		{
#if USING( DEBUGHANDLES )
			FK_ASSERT(type_in == hdnl_in.TYPE);
#endif
		}
		/************************************************************************************************/
	}

	typedef Handle_t<16> EntityHandle;
	//typedef size_t EntityHandle;

}