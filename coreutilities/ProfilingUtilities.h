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

#ifndef PROFILINGUTILITIES
#define PROFILINGUTILITIES

#include "..\buildsettings.h"
#include <Windows.h>
#include <chrono>


/************************************************************************************************/


namespace FlexKit
{
	enum PROFILE_IDs
	{
		PROFILE_ANIMATIONUPDATE,
		PROFILE_TRANSFORMUPDATE,
		PROFILE_ENTITYUPDATE,
		PROFILE_FRAME,
		PROFILE_SUBMISSION,
		PROFILE_PRESENT,
		PROFILE_SORTING,
		PROFILE_ID_COUNT
	};


	enum COUNTER_IDs
	{
		COUNTER_DRAWCALL,
		COUNTER_INDEXEDDRAWCALL,
		COUNTER_DRAWCALL_SECOND,
		COUNTER_INDEXEDDRAWCALL_SECOND,
		COUNTER_DRAWCALL_LASTSECOND,
		COUNTER_INDEXEDDRAWCALL_LASTSECOND,
		COUNTER_OBJECTSDRAWN_FRAME,
		COUNTER_OBJECTSDRAWN_LASTFRAME,
		COUNTER_OBJECTSDRAWN_SECOND,
		COUNTER_OBJECTSDRAWN_LASTSECOND,
		COUNTER_FRAMECOUNTER,
		COUNTER_AVERAGEANIMATIONTIME,
		COUNTER_AVERAGEFRAMETIME,
		COUNTER_AVERAGESORTTIME,
		COUNTER_AVERAGEENTITYUPDATETIME,
		COUNTER_AVERAGEPRESENT,
		COUNTER_AVERAGETRANSFORMUPDATETIME,
		COUNTER_ACC_ANIMTIME,
		COUNTER_ACC_FRAMETIME,
		COUNTER_ACC_SORTTIME,
		COUNTER_ACC_ENTITYUPDATETIME,
		COUNTER_ACC_PRESENT,
		COUNTER_ACC_TRANSFORMUPDATETIME,
		COUNTER_ENTITYUPDATETIME,
		COUNTER_COUNT
	};


	struct FrameStats
	{
		size_t	DrawCallsLastSecond	= 0;
		size_t	DrawCallsLastFrame	= 0;
		double	PresentTime			= 0.0;
		double	AverageFPSTime		= 0.0;
		double	AverageSortTime		= 0.0;
		double	EntUpdateTime		= 0.0;
	};

	
	struct EngineMemory_DEBUG
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


    template<typename TY>
    FLEXKITAPI decltype(auto) _TimeBlock(TY& function, const char* id)
    {
        std::chrono::system_clock Clock;
        auto Before = Clock.now();

        EXITSCOPE(
            auto After = Clock.now();
        auto Duration = chrono::duration_cast<chrono::microseconds>(After - Before);
        FK_LOG_9("Function %s executed in %umicroseconds.", id, Duration.count()); );

        return function();
    }


    #define TIMEBLOCK(A, B) _TimeBlock([&]{ return A; }, B)


	FLEXKITAPI void		InitDebug		(EngineMemory_DEBUG* _ptr);
	FLEXKITAPI void		SetDebugMemory	(EngineMemory_DEBUG* _ptr);
	FLEXKITAPI void		LogFrameStats	(FrameStats Stats);
	FLEXKITAPI void		AddCounter		(uint16_t Id, size_t N);
	FLEXKITAPI void		IncCounter		(uint16_t Id);
	FLEXKITAPI size_t	GetCounter		(uint16_t Id);
	FLEXKITAPI void		ResetCounter	(uint16_t Id);
	FLEXKITAPI void		ProfileBegin	(uint16_t Id);
	FLEXKITAPI void		ProfileEnd		(uint16_t Id);
	FLEXKITAPI size_t	GetDuration		(uint16_t Id);

	FLEXKITAPI FrameStats	GetStats		();
	FLEXKITAPI void			UpdateFPSStats	();
}


/************************************************************************************************/

#endif // ! PROFILINGUTILITIES
