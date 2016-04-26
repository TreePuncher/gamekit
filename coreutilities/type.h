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

#ifdef _WIN32
#pragma once
#endif

#ifndef TYPEID_H
#define TYPEID_H

#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4305 )
#pragma warning ( disable : 4309 )

// Include
#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"

/****************************************************************************************************/
namespace FlexKit
{
	/****************************************************************************************************/
	// TODO: Replace this all with Meta Compiler
	extern FLEXKITAPI const unsigned short NULLTYPE_INDEX;

	typedef short Type_t;
	const Type_t Invalid_Type_ID = 0xffff;

	FLEXKITAPI Type_t		GetNextTypeID();

	/****************************************************************************************************/

	template<typename>
	Type_t GetTypeID()
	{
		static Type_t ThisType = GetNextTypeID();
		return ThisType;
	}

}
/****************************************************************************************************/

#if USING(USINGMETACOMP)
#define GetTID(Type)
#else
#define GetTID(Type) FlexKit::TypeID::GetTypeID<Type>()
#endif

#endif