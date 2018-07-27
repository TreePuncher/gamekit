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

#include "stdafx.h"
#include "GameMemory.h"

namespace ProfilingUtilities
{
	struct EngineMemoryDEBUG
	{
		size_t		FrameCount;
		FrameStats	LastSecond[60];
		bool		PrintStats;

		struct PROFILETIMINGS
		{
			LARGE_INTEGER	Begin;
			LARGE_INTEGER	End;
			LARGE_INTEGER	Freq;
			LARGE_INTEGER	Elapsed;
			size_t			Duration;
		}Timings[PROFILE_IDs::PROFILE_ID_COUNT];

		size_t Counters[COUNTER_IDs::COUNTER_COUNT];
	};

#if USING(DEBUG_PROFILING)
	static EngineMemoryDEBUG DEBUGMEMORY;
#endif
	void InitDebug()
	{
#if USING(DEBUG_PROFILING)
		DEBUGMEMORY.FrameCount = 0;
		DEBUGMEMORY.PrintStats = false;

		for (auto& T : DEBUGMEMORY.Timings)
		{
			T.Begin     = {};
			T.End	    = {};
			T.Freq	    = {};
			T.Duration	= {};
			T.Elapsed	= {};
		}
#endif
	}

	void Log_FrameStats(FrameStats Stats)
	{
#if USING(DEBUG_PROFILING)
#endif
	}

	void TogglePrintStats()
	{
#if USING(DEBUG_PROFILING)
		DEBUGMEMORY.PrintStats = !DEBUGMEMORY.PrintStats;
#endif
	}

	void ProfileBegin(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		QueryPerformanceFrequency(&DEBUGMEMORY.Timings[ID].Freq);
		QueryPerformanceCounter(&DEBUGMEMORY.Timings[ID].Begin);
#endif
	}


	void ProfileEnd(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		LARGE_INTEGER End;
		QueryPerformanceCounter(&End);
		DEBUGMEMORY.Timings[ID].End				 = End;
		DEBUGMEMORY.Timings[ID].Elapsed.QuadPart = DEBUGMEMORY.Timings[ID].End.QuadPart - DEBUGMEMORY.Timings[ID].Begin.QuadPart;
		DEBUGMEMORY.Timings[ID].Elapsed.QuadPart *= 1000000;
		DEBUGMEMORY.Timings[ID].Elapsed.QuadPart /= DEBUGMEMORY.Timings[ID].Freq.QuadPart;
#endif
	}

	inline size_t GetDuration(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		return DEBUGMEMORY.Timings[ID].Elapsed.QuadPart;
#else
		return 0;
#endif
	}

	void IncCounter(uint16_t Id)
	{
#if USING(DEBUG_PROFILING)
		++DEBUGMEMORY.Counters[Id];
#endif
	}

	void AddCounter(uint16_t Id, size_t N)
	{
#if USING(DEBUG_PROFILING)
		DEBUGMEMORY.Counters[Id] += N;
#endif
	}

	void SetCounter(uint16_t Id, size_t N)
	{
#if USING(DEBUG_PROFILING)
		DEBUGMEMORY.Counters[Id] = N;
#endif
	}

	size_t GetCounter(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		return DEBUGMEMORY.Counters[ID];
#endif
	}

	void ResetCounter(uint16_t ID)
	{
#if USING(DEBUG_PROFILING)
		DEBUGMEMORY.Counters[ID] = 0;
#endif
	}

	FrameStats GetStats()
	{
		FrameStats out;
		out.DrawCallsLastSecond		= GetCounter( COUNTER_INDEXEDDRAWCALL_LASTSECOND );
		out.DrawCallsLastFrame		= GetCounter( COUNTER_INDEXEDDRAWCALL_LASTSECOND )		/ 600;
		out.AverageFPSTime			= double(GetCounter( COUNTER_AVERAGEFRAMETIME ))		/ 1000.0;
		out.AverageSortTime			= double(GetCounter( COUNTER_AVERAGESORTTIME ))			/ 1000.0;
		out.EntUpdateTime			= double(GetCounter( COUNTER_AVERAGEENTITYUPDATETIME))	/ 1000.0;

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
		DEBUGMEMORY.FrameCount++;

		IncCounter		(COUNTER_FRAMECOUNTER);

		AddCounter(COUNTER_DRAWCALL_SECOND,			GetCounter(COUNTER_DRAWCALL));
		AddCounter(COUNTER_INDEXEDDRAWCALL_SECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL));

		AddTimerToCounter(COUNTER_ACC_ANIMTIME,			PROFILE_ANIMATIONUPDATE	);
		AddTimerToCounter(COUNTER_ACC_ENTITYUPDATETIME, PROFILE_ENTITYUPDATE	);
		AddTimerToCounter(COUNTER_ACC_FRAMETIME,		PROFILE_FRAME			);
		AddTimerToCounter(COUNTER_ACC_PRESENT,			PROFILE_PRESENT			);
		AddTimerToCounter(COUNTER_ACC_SORTTIME,			PROFILE_SORTING			);

		ResetCounter(COUNTER_DRAWCALL);
		ResetCounter(COUNTER_INDEXEDDRAWCALL);
		ResetCounter(COUNTER_OBJECTSDRAWN_FRAME);

		if (DEBUGMEMORY.FrameCount >= 60 )
		{
			DEBUGMEMORY.FrameCount = 0;
			UpdateTimerAverage( COUNTER_AVERAGEPRESENT,				COUNTER_ACC_PRESENT,			60);
			UpdateTimerAverage( COUNTER_AVERAGEFRAMETIME,			COUNTER_ACC_FRAMETIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGEANIMATIONTIME,		COUNTER_ACC_ANIMTIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGESORTTIME,			COUNTER_ACC_SORTTIME,			60);
			UpdateTimerAverage( COUNTER_AVERAGEENTITYUPDATETIME,	COUNTER_ACC_ENTITYUPDATETIME,	60);


			SetCounter(COUNTER_DRAWCALL_LASTSECOND,			GetCounter(COUNTER_DRAWCALL_SECOND)			);
			SetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND,	GetCounter(COUNTER_INDEXEDDRAWCALL_SECOND)	);

			if (DEBUGMEMORY.PrintStats)
			{
				std::cout << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
				std::cout << "DRAW CALLS LAST SECOND: "		<<			GetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND)			<< "\n";
				std::cout << "Ave Draw Calls Per Second: "	<<			GetCounter(COUNTER_INDEXEDDRAWCALL_LASTSECOND)/60		<< "\n";
				std::cout << "Ave FrameTime: "				<< double(	GetCounter(COUNTER_AVERAGEFRAMETIME))/1000.0			<< " Ms\n";
				std::cout << "Ave Sorting Time: "			<< double(	GetCounter(COUNTER_AVERAGESORTTIME))/1000.0				<< " Ms\n";
				std::cout << "Ave Entity Update Time: "		<< double(	GetCounter(COUNTER_AVERAGEENTITYUPDATETIME))/1000.0		<< " Ms\n";
				std::cout << "Ave Present Time: "			<< double(	GetCounter(COUNTER_AVERAGEPRESENT))/1000.0				<< " Ms\n";
				std::cout << "Ave Animation UpdateTime: "	<< double(	GetCounter(COUNTER_AVERAGEANIMATIONTIME))/1000.0		<< " Ms\n";
				std::cout << "UpdateTime: "					<< double(	GetCounter(COUNTER_AVERAGEPRESENT))/1000.0	- double(	GetCounter(COUNTER_AVERAGEPRESENT))/1000.0			<< " Ms\n";
			}

			ResetCounter(COUNTER_DRAWCALL_SECOND);
			ResetCounter(COUNTER_INDEXEDDRAWCALL_SECOND);
		}
#endif
	}
}
