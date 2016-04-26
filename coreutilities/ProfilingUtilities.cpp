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

#include "ProfilingUtilities.h"

#include <Windows.h>
#include <iostream>

namespace FlexKit
{
	void InitDebug(EngineMemory_DEBUG* out)
	{
#if USING(DEBUG_PROFILING)
		out->FrameCount = 0;
		out->PrintStats = false;

		for (auto& T : out->Timings)
		{
			T.Begin     = {};
			T.End	    = {};
			T.Freq	    = {};
			T.Duration	= {};
			T.Elapsed	= {};
		}
#endif
	}

	void LogFrameStats(FrameStats Stats, EngineMemory_DEBUG* _ptr)
	{
#if USING(DEBUG_PROFILING)
#endif
	}

	void ProfileBegin(uint16_t ID, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		QueryPerformanceFrequency(&DMem->Timings[ID].Freq);
		QueryPerformanceCounter(&DMem->Timings[ID].Begin);
#endif
	}


	void ProfileEnd(uint16_t ID, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		LARGE_INTEGER End;
		QueryPerformanceCounter(&End);
		DMem->Timings[ID].End				 = End;
		DMem->Timings[ID].Elapsed.QuadPart = DMem->Timings[ID].End.QuadPart - DMem->Timings[ID].Begin.QuadPart;
		DMem->Timings[ID].Elapsed.QuadPart *= 1000000;
		DMem->Timings[ID].Elapsed.QuadPart /= DMem->Timings[ID].Freq.QuadPart;
#endif
	}

	inline size_t GetDuration(uint16_t ID, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		return DMem->Timings[ID].Elapsed.QuadPart;
#else
		return 0;
#endif
	}

	void IncCounter(uint16_t Id, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		++DMem->Counters[Id];
#endif
	}

	void AddCounter(uint16_t Id, size_t N, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		DMem->Counters[Id] += N;
#endif
	}

	void SetCounter(uint16_t Id, size_t N, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		DMem->Counters[Id] = N;
#endif
	}

	size_t GetCounter(uint16_t ID, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		return DMem->Counters[ID];
#endif
	}

	void ResetCounter(uint16_t ID, EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		DMem->Counters[ID] = 0;
#endif
	}

	FrameStats GetStats(EngineMemory_DEBUG* DMem)
	{
		FrameStats out;
		out.DrawCallsLastSecond		= GetCounter( COUNTER_INDEXEDDRAWCALL_LASTSECOND, DMem );
		out.DrawCallsLastFrame		= GetCounter( COUNTER_INDEXEDDRAWCALL_LASTSECOND, DMem )		/ 600;
		out.AverageFPSTime			= double(GetCounter( COUNTER_AVERAGEFRAMETIME, DMem ))			/ 1000.0;
		out.AverageSortTime			= double(GetCounter( COUNTER_AVERAGESORTTIME, DMem ))			/ 1000.0;
		out.EntUpdateTime			= double(GetCounter( COUNTER_AVERAGEENTITYUPDATETIME, DMem))	/ 1000.0;

		return out;
	}

	void AddTimerToCounter(COUNTER_IDs CID, PROFILE_IDs PID, EngineMemory_DEBUG* DMem)
	{
		AddCounter(CID,		GetDuration(PID, DMem), DMem);
	}

	void UpdateTimerAverage(COUNTER_IDs CID_AVE, COUNTER_IDs CID_ACC, size_t SampleCount, EngineMemory_DEBUG* DMem)
	{
		SetCounter	(CID_AVE, GetCounter(CID_ACC, DMem)/SampleCount, DMem);
		ResetCounter(CID_ACC, DMem);
	}


	void UpdateFPSStats(EngineMemory_DEBUG* DMem)
	{
#if USING(DEBUG_PROFILING)
		DMem->FrameCount++;

		IncCounter(COUNTER_FRAMECOUNTER, DMem);

		AddCounter(COUNTER_DRAWCALL_SECOND,			GetCounter(COUNTER_DRAWCALL, DMem), DMem);
		AddCounter(COUNTER_INDEXEDDRAWCALL_SECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL, DMem), DMem);

		AddTimerToCounter(COUNTER_ACC_ANIMTIME,			PROFILE_ANIMATIONUPDATE,	DMem);
		AddTimerToCounter(COUNTER_ACC_ENTITYUPDATETIME, PROFILE_ENTITYUPDATE,		DMem);
		AddTimerToCounter(COUNTER_ACC_FRAMETIME,		PROFILE_FRAME,				DMem);
		AddTimerToCounter(COUNTER_ACC_PRESENT,			PROFILE_PRESENT,			DMem);
		AddTimerToCounter(COUNTER_ACC_SORTTIME,			PROFILE_SORTING,			DMem);

		ResetCounter(COUNTER_DRAWCALL,				DMem);
		ResetCounter(COUNTER_INDEXEDDRAWCALL,		DMem);
		ResetCounter(COUNTER_OBJECTSDRAWN_FRAME,	DMem);

		if (DMem->FrameCount >= 60 )
		{
			DMem->FrameCount = 0;
			UpdateTimerAverage( COUNTER_AVERAGEPRESENT,				COUNTER_ACC_PRESENT,			60, DMem);
			UpdateTimerAverage( COUNTER_AVERAGEFRAMETIME,			COUNTER_ACC_FRAMETIME,			60, DMem);
			UpdateTimerAverage( COUNTER_AVERAGEANIMATIONTIME,		COUNTER_ACC_ANIMTIME,			60, DMem);
			UpdateTimerAverage( COUNTER_AVERAGESORTTIME,			COUNTER_ACC_SORTTIME,			60, DMem);
			UpdateTimerAverage( COUNTER_AVERAGEENTITYUPDATETIME,	COUNTER_ACC_ENTITYUPDATETIME,	60, DMem);


			SetCounter(COUNTER_DRAWCALL_LASTSECOND,			GetCounter(COUNTER_DRAWCALL_SECOND, DMem),			DMem);
			SetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL_SECOND, DMem),	DMem);

			ResetCounter(COUNTER_DRAWCALL_SECOND,			DMem);
			ResetCounter(COUNTER_INDEXEDDRAWCALL_SECOND,	DMem);
		}
#endif
	}
}
