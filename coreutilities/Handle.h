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

#include "buildsettings.h"
#include "containers.h"
#include "type.h"

#include <stdint.h>


namespace FlexKit
{
	typedef uint32_t index_t;

	struct _InvalidHandle_t {} inline static const InvalidHandle_t;

	template<size_t HandleSize = 32, size_t ID = (size_t)-1>
	class Handle_t
	{
	public:
		typedef Handle_t<HandleSize, ID> THISTYPE_t;
		constexpr static const size_t GetHandleSize() { return HandleSize; }

		constexpr Handle_t()
		{
#if USING( DEBUGHANDLES )
			INDEX = 0XFFFFFFFF;
			TYPE = 0XFFFF;
			FLAGS = HANDLE_FLAGS::HF_ERROR;
#endif
		}
		constexpr Handle_t(unsigned int index, unsigned int type, unsigned int flags)
			: INDEX(index)
#if USING( DEBUGHANDLES )
			, TYPE(type)
			, FLAGS(flags)
#endif
		{}

		constexpr Handle_t(const Handle_t<HandleSize>& in)
		{
			INDEX = in.INDEX;
#if USING( DEBUGHANDLES )
			TYPE = in.TYPE;
			FLAGS = in.FLAGS;
#endif
		}

		constexpr explicit Handle_t(size_t in) :
            INDEX{ (unsigned int)in }
		{
#if USING( DEBUGHANDLES )
			TYPE = 0XFFFF;
			FLAGS = HANDLE_FLAGS::HF_ERROR;
#endif
		}

        constexpr Handle_t(_InvalidHandle_t)
		{
			INDEX = 0XFFFFFFFF;
		}


		//operator uint32_t() const { return to_uint(); }

		bool				operator ==	(const THISTYPE_t in) const
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

		bool operator !=	(const THISTYPE_t in) const
		{
			return !(*this == in);
		}

		const uint32_t	to_uint() const
		{
			return INDEX;
		}


		Handle_t<HandleSize, ID> operator = (_InvalidHandle_t)
		{
			INDEX = 0XFFFFFFFF;
			return {};
		}
		

		bool operator == (_InvalidHandle_t)
		{
			return (*this == THISTYPE_t(InvalidHandle_t));
		}

		bool operator != (_InvalidHandle_t handle)
		{
			return !(*this == handle);
		}

		operator uint32_t(){ return INDEX; }

        //unsigned int    INDEX		: HandleSize    = 0xffffffff;
        unsigned int    INDEX		: HandleSize;
#if USING( DEBUGHANDLES )
		unsigned int	TYPE		: 28            = 0;
		unsigned int	FLAGS		: 4             = 0;
#endif

		operator size_t () const { return INDEX; }

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
			HandleTable(iAllocator* Memory = nullptr, const Type_t type = 0x00 ) : mType( type ), Indexes(Memory) {}

			void Initiate( iAllocator* Memory )
			{
                FreeList            = -1;
				Indexes.Allocator   = Memory;
			}

			inline index_t&	operator[] ( const HANDLE in )
			{
				#ifdef _DEBUG
				//HandleUtilities::CheckType(in, mType);
				#endif
				return Indexes[ in.INDEX ];
			}

			inline index_t	operator[] ( const HANDLE in ) const	{return Indexes[ in.INDEX ];}

			inline index_t&	Get( const HANDLE in )					{return Indexes[ in.INDEX ];}
			inline index_t	Get( const HANDLE in ) const			{return Indexes[ in.INDEX ];}

			inline HANDLE	GetNewHandle()
			{
                if (FreeList != -1) {
                    const auto idx = FreeList;
                    FreeList = Indexes[idx];

                    return { (index_t)idx, mType, FlexKit::Handle::HF_USED };
                }

                return { (index_t)Indexes.push_back(-1), mType, FlexKit::Handle::HF_USED };
			}

			inline void	Clear()
			{
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
                Indexes[in] = FreeList;
                FreeList    = in;
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
