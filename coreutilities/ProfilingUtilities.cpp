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

#include "ProfilingUtilities.h"

#include <Windows.h>
#include <iostream>

namespace FlexKit
{
	static EngineMemory_DEBUG* gMemory = nullptr;

	void InitDebug(EngineMemory_DEBUG* out)
	{
#if USING(DEBUG_PROFILING)
		gMemory = out;
		gMemory->FrameCount = 0;
		gMemory->PrintStats = false;

		for (auto& T : gMemory->Timings)
		{
			T.Begin     = {};
			T.End	    = {};
			T.Freq	    = {};
			T.Duration	= {};
			T.Elapsed	= {};
		}
#endif
	}

	void SetDebugMemory(EngineMemory_DEBUG* _ptr)
	{
		gMemory = _ptr;
	}


	void LogFrameStats(FrameStats Stats)
	{
#if USING(DEBUG_PROFILING)
#endif
	}

	void ProfileBegin(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		QueryPerformanceFrequency	(&gMemory->Timings[ID].Freq);
		QueryPerformanceCounter		(&gMemory->Timings[ID].Begin);
#endif
	}


	void ProfileEnd(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		LARGE_INTEGER End;
		QueryPerformanceCounter(&End);
		gMemory->Timings[ID].End			  = End;
		gMemory->Timings[ID].Elapsed.QuadPart = gMemory->Timings[ID].End.QuadPart - gMemory->Timings[ID].Begin.QuadPart;
		gMemory->Timings[ID].Elapsed.QuadPart *= 1000000;
		gMemory->Timings[ID].Elapsed.QuadPart /= gMemory->Timings[ID].Freq.QuadPart;
#endif
	}

	inline size_t GetDuration(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		return gMemory->Timings[ID].Elapsed.QuadPart;
#else
		return 0;
#endif
	}

	void IncCounter(uint16_t Id)
	{
#if USING(DEBUG_PROFILING)
		++gMemory->Counters[Id];
#endif
	}

	void AddCounter(uint16_t Id, size_t N)
	{
#if USING(DEBUG_PROFILING)
		gMemory->Counters[Id] += N;
#endif
	}

	void SetCounter(uint16_t Id, size_t N)
	{
#if USING(DEBUG_PROFILING)
		gMemory->Counters[Id] = N;
#endif
	}

	size_t GetCounter(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		return gMemory->Counters[ID];
#endif
	}

	void ResetCounter(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		gMemory->Counters[ID] = 0;
#endif
	}

	FrameStats GetStats()
	{
		FrameStats out;
		out.DrawCallsLastSecond		= GetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND);
		out.DrawCallsLastFrame		= GetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND)		/ 600;
		out.AverageFPSTime			= double(GetCounter(COUNTER_AVERAGEFRAMETIME))			/ 1000.0;
		out.AverageSortTime			= double(GetCounter(COUNTER_AVERAGESORTTIME))			/ 1000.0;
		out.EntUpdateTime			= double(GetCounter(COUNTER_AVERAGEENTITYUPDATETIME))	/ 1000.0;

		return out;
	}

	void AddTimerToCounter(COUNTER_IDs CID, PROFILE_IDs PID)
	{
		AddCounter(CID,		GetDuration(PID));
	}

	void UpdateTimerAverage(COUNTER_IDs CID_AVE, COUNTER_IDs CID_ACC, size_t SampleCount)
	{
		SetCounter	(CID_AVE, GetCounter(CID_ACC)/SampleCount);
		ResetCounter(CID_ACC);
	}


	void UpdateFPSStats()
	{
#if USING(DEBUG_PROFILING)
		gMemory->FrameCount++;

		IncCounter(COUNTER_FRAMECOUNTER);

		AddCounter(COUNTER_DRAWCALL_SECOND,			GetCounter(COUNTER_DRAWCALL));
		AddCounter(COUNTER_INDEXEDDRAWCALL_SECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL));

		AddTimerToCounter(COUNTER_ACC_ANIMTIME,			PROFILE_ANIMATIONUPDATE);
		AddTimerToCounter(COUNTER_ACC_ENTITYUPDATETIME, PROFILE_ENTITYUPDATE);
		AddTimerToCounter(COUNTER_ACC_FRAMETIME,		PROFILE_FRAME);
		AddTimerToCounter(COUNTER_ACC_PRESENT,			PROFILE_PRESENT);
		AddTimerToCounter(COUNTER_ACC_SORTTIME,			PROFILE_SORTING);

		ResetCounter(COUNTER_DRAWCALL);
		ResetCounter(COUNTER_INDEXEDDRAWCALL);
		ResetCounter(COUNTER_OBJECTSDRAWN_FRAME);

		if (gMemory->FrameCount >= 60 )
		{
			gMemory->FrameCount = 0;
			UpdateTimerAverage( COUNTER_AVERAGEPRESENT,				COUNTER_ACC_PRESENT,			60);
			UpdateTimerAverage( COUNTER_AVERAGEFRAMETIME,			COUNTER_ACC_FRAMETIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGEANIMATIONTIME,		COUNTER_ACC_ANIMTIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGESORTTIME,			COUNTER_ACC_SORTTIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGEENTITYUPDATETIME,	COUNTER_ACC_ENTITYUPDATETIME,	60);


			SetCounter(COUNTER_DRAWCALL_LASTSECOND,			GetCounter(COUNTER_DRAWCALL_SECOND));
			SetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL_SECOND));

			ResetCounter(COUNTER_DRAWCALL_SECOND);
			ResetCounter(COUNTER_INDEXEDDRAWCALL_SECOND);
		}
#endif
	}
}
