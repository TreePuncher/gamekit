/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
#define PHYSX_PVD			ON
#define DEBUGALLOCATOR		OFF
#define DEBUGGRAPHICS		ON
#define DEBUGSHADERS		ON
#define ENABLEDRED		    OFF
#define DEBUGHANDLES		OFF
#define DEBUGMEMORY			OFF
#define FATALERROR			OFF
#define EDITSHADERCONTINUE	ON
#define STL					OFF
#define DEBUG_PROFILING 	OFF
#define RACKNET				ON
#define RESCOMPILERVERBOSE	ON
#define TOOTLE				OFF
#define AFTERMATH			OFF
#define PIX                 ON
#define ENABLEPROFILER      OFF

#else

#define USESTL ON
#define PHYSX				ON
#define PHYSX_PVD			OFF
#define DEBUGALLOCATOR		OFF
#define DEBUGGRAPHICS		OFF
#define DEBUGSHADERS		OFF
#define ENABLEDRED		    OFF
#define DEBUGHANDLES		OFF
#define DEBUGMEMORY			OFF
#define FATALERROR			ON
#define EDITSHADERCONTINUE 	ON
#define STL					OFF
#define DEBUG_PROFILING 	ON
#define RACKNET				ON
#define RESCOMPILERVERBOSE	ON
#define TOOTLE				OFF
#define AFTERMATH			OFF
#define PIX                 ON
#define ENABLEPROFILER      OFF


#endif

#define ROUT		 &
#define RIN		const&
#define RINOUT		 &

#if USING(DEBUGALLOCATOR)

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

#define FLEXKITINTERNAL

#include <assert.h>
#include <functional>
#include <chrono>

#ifdef _DEBUG

#include "Logging.h"
#define FK_ASSERT1(A) assert(A)
#define FK_ASSERT2(A, B) if(!(A) && false) {FK_LOG_ERROR( B ); assert(false);}

#else


template<typename TY_1>					void Discard(TY_1) {}
template<typename TY_1, typename TY_2>	void Discard(TY_1, TY_2) {}

#define FK_ASSERT1(A) Discard(A)
#define FK_ASSERT2(A, B) Discard(A)

#endif

#define GETASSERT(_1, _2, Name, ...)Name
#define FK_ASSERT(...) GETASSERT(__VA_ARGS__, FK_ASSERT2, FK_ASSERT1)(__VA_ARGS__)

#define MERGECOUNT_(a,b)  a##b
#define FINALLABEL_(a) MERGECOUNT_(Finally_ID_, a)
#define FINALLABEL FINALLABEL_(__LINE__)


#if 0

struct _FINALLY
{
	_FINALLY(std::function<void ()> INFN) : FN(INFN){}
	~_FINALLY(){FN();}
	std::function<void ()> FN;

private: 
	_FINALLY(const _FINALLY& INFN) {}
};

#define FINALLY _FINALLY FINALLABEL([&]()->void {
#define FINALLYOVER });

#else

template<typename TY>
struct FINAL_CONTAINER
{
	template<typename TY_1>
	FINAL_CONTAINER(TY_1 fn) : FN(fn) {}

	~FINAL_CONTAINER()	{ FN();	}

	const TY FN;
};

template<typename FN_TYPE>
auto BuildFinal(FN_TYPE FN) -> FINAL_CONTAINER<decltype(FN)> { return FINAL_CONTAINER<decltype(FN)>(FN); }

#define FINALLY auto FINALLABEL = BuildFinal([&]()->void {
#define FINALLYOVER });

#define FINAL(a) FINALLY a FINALLYOVER

#define EXITSCOPE(a) FINAL(a;)

#endif

#ifdef _DEBUG
#define DEBUGBLOCK(A) A;
#else
#define DEBUGBLOCK(A)
#endif

template<typename TY>
inline void NullCheck(TY* _ptr)
{
#ifdef _DEBUG
	FK_ASSERT(_ptr != nullptr, "NULLCHECK FAILED!");
#endif
}

static const size_t gCORECOUNT	= 4;
static const size_t KILOBYTE	= 1024; 
static const size_t MEGABYTE	= 1024 * KILOBYTE;
static const size_t GIGABYTE	= 1024 * MEGABYTE;

constexpr size_t DefaultWorkerCount = 2; // should be safe for a quad core cpu
const size_t INVALIDHANDLE          = size_t(-1);

namespace FlexKit {
    using byte = uint8_t;

	typedef uint64_t AssetHandle;
	typedef uint64_t GUID_t;
}

#define NOMINMAX

#include <Windows.h>
#include <debugapi.h>


#endif//BUILDSETTING_H
