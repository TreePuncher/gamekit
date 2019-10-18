#pragma once

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

#include "..\buildsettings.h"
#include "..\coreutilities\type.h"
#include <algorithm>
#include <functional>

namespace FlexKit
{
	using FlexKit::Type_t;


	template<typename ... FNDef>
	class Signal
	{
	public:
		class Slots
		{
		public:
			~Slots()
			{
				for( auto Entry : m_Signals )
					Entry.Ptr->Disconnect( this, Entry.ID );
			}

			void release( Signal* Ptr )
			{
#if USING(STL)
				//m_Signals.erase( std::remove_if( m_Signals.begin(), m_Signals.end(), [=]( const Signal_Entry& in ) -> bool
				//{
				//	return ( in.Ptr == Ptr );
				//} ), m_Signals.end() );
#endif
				auto i	 = m_Signals.begin();
				auto end = m_Signals.end();
				for (; i < end; ++i)
				{
					if ((*i).Ptr == Ptr)
					{
						(*i).Ptr->Disconnect(this, (*i).ID);
						break;
					}
				}

				if (i != m_Signals.end())
					m_Signals.erase(i);
			}

			void NotifyOnDestruction( Signal* _ptr, unsigned int id )
			{
				m_Signals.push_back( Signal_Entry( _ptr, id ) );
			}
		private:

			struct Signal_Entry
			{
				Signal_Entry() {}
				Signal_Entry( Signal* signal_in, unsigned int id ) : ID( id ), Ptr( signal_in ) {}
				Signal*								Ptr;
				unsigned int						ID;
			};
			static_vector<Signal_Entry>	m_Signals;
		};

		typedef void   (FN)(Slots*, FNDef...);
		typedef FN*		FN_PTR;

	private:
		struct Slot_Entry
		{
			Slot_Entry() {}
			Slot_Entry(FN_PTR in, unsigned int id, Slots* Ptr_in ) : FN{ in }, ID{ id }, Ptr{ Ptr_in } {}

			FN_PTR			FN;
			Slots*			Ptr;
			unsigned int	ID;
		};

	public:
		Signal() {}
		~Signal( void )
		{
			DiconnectAll();
		}

		void operator()()
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN(slot.Ptr);
			}
		}
		template< typename Ty_1 >
		void operator()( Ty_1 in )
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN(slot.Ptr, in1 );
			}
		}
		template< typename Ty_1, typename Ty_2 >
		void operator()( Ty_1 in_1, Ty_2 in_2 )
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN(slot.Ptr, in1, in_2 );
			}
		}

		template< typename Ty_1, typename Ty_2, typename Ty_3 >
		void operator()( Ty_1 in_1, Ty_2 in_2, Ty_3 in_3 )
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN(slot.Ptr, in1, in_2, in_3 );
			}
		}

		template< typename Ty_1, typename Ty_2, typename Ty_3, typename Ty_4 >
		void operator()( Ty_1 in_1, Ty_2 in_2, Ty_3 in_3, Ty_4 in_4 )
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN( slot.Ptr, in1, in_2, in_3, in_4 );
			}
		}

		template< typename Ty_1, typename Ty_2, typename Ty_3, typename Ty_4, typename Ty_5 >
		void operator()( Ty_1 in_1, Ty_2 in_2, Ty_3 in_3, Ty_4 in_4, Ty_5 in_5 )
		{
			for( auto slot : mOutputSlots )
			{
				slot.FN( slot.Ptr, in1, in_2, in_3, in_4, in_5 );
			}
		}

		void Connect( Slots* _ptr, FN_PTR FN_In )
		{
			_ptr->NotifyOnDestruction( this, mOutputSlots.size() );
			mOutputSlots.push_back( Slot_Entry(FN_In, mOutputSlots.size(), _ptr ) );
		}

		void Disconnect( Slots* _ptr, unsigned int ID )
		{
			auto end = mOutputSlots.end();

			for( auto itr = mOutputSlots.begin(); itr != end; itr++ )
			{
				auto entry = *itr;
				if( entry.Ptr == _ptr && entry.ID == ID )
				{
					mOutputSlots.erase( itr );
					break;
				}
			}
		}

		void DiconnectAll()
		{
			for( auto entry : mOutputSlots )
				entry.Ptr->release( this );

			mOutputSlots.clear();
		}

	private:
#ifdef _DEBUG
		bool	mVerify;
		Type_t	mSignature;
#endif
		static_vector<Slot_Entry> mOutputSlots;
	};
}