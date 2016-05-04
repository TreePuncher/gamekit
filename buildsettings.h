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

#ifndef BUILDSETTING_H
#define BUILDSETTING_H

#define ON			+
#define OFF			-
#define USING(x) ((1 x 1)==2) 

#define FASTMATH ON

#ifdef COMPILE64 
#define X64EXE ON
#define X86EXE OFF
#else
#ifdef COMPILE32
#define X64EXE OFF
#define X86EXE ON
#endif
#endif

#define USINGMETACOMP	OFF
#define DLLEXPORT		OFF
#ifdef _DEBUG
#define USESTL				ON
#define PHYSX				ON
#define PHYSXREMOTEDB		ON
#define DEBUGALLOCATOR		OFF
#define DEBUGGRAPHICS		ON
#define DEBUGHANDLES		ON
#define FATALERROR			OFF
#define EDITSHADERCONTINUE	ON
#define STL					OFF
#define DEBUG_PROFILING 	ON
#else
#define USESTL ON
#define PHYSX				ON
#define PHYSXREMOTEDB		ON
#define DEBUGALLOCATOR		OFF
#define DEBUGGRAPHICS		OFF
#define DEBUGHANDLES		OFF
#define FATALERROR			ON
#define EDITSHADERCONTINUE 	ON
#define STL					OFF
#define DEBUG_PROFILING 	ON
#endif

#define ROUT		 &
#define RIN		const&
#define RINOUT		 &

#if USING(DEBUGALLOCATOR)
#include <vld.h>
#endif

#if USING(DLLEXPORT)
#ifdef FLEXKITAPI_COMPILE
#define FLEXKITAPI __declspec(dllexport)
#else
#define FLEXKITAPI __declspec(dllimport)
#endif
#else
#define FLEXKITAPI
#endif


#include <assert.h>
#include <functional>

#ifdef _DEBUG

#define FK_ASSERT1(A) assert(A)
#define FK_ASSERT2(A, B) assert(A)

#else

#define FK_ASSERT1(A) A
#define FK_ASSERT2(A, B) A

#endif

#define GETASSERT(_1, _2, Name, ...)Name
#define FK_ASSERT(...) GETASSERT(__VA_ARGS__, FK_ASSERT2, FK_ASSERT1)(__VA_ARGS__)

#define MERGECOUNT_(a,b)  a##b
#define FINALLABEL_(a) MERGECOUNT_(unique_name_, a)
#define FINALLABEL FINALLABEL_(__LINE__)

struct _FINALLY
{
	_FINALLY(std::function<void ()> INFN) : FN(INFN){}
	~_FINALLY(){FN();}
	std::function<void ()> FN;

private: 
	_FINALLY(const _FINALLY& INFN) {}
};

#define FINALLY _FINALLY FINALLABEL([&]()->void
#define FINALLYOVER );

static const size_t gCORECOUNT	= 4;
static const size_t KILOBYTE	= 1024; 
static const size_t MEGABYTE	= 1024 * KILOBYTE;
static const size_t GIGABYTE	= 1024 * MEGABYTE;

typedef uint8_t byte;
typedef uint64_t ResourceHandle;

#define NOMINMAX

#include <WinSock2.h>
#include <windows.h>
#include <Ws2tcpip.h>

#endif//BUILDSETTING_H