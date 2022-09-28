#pragma once
#pragma warning ( disable : 4244 )
#pragma warning ( disable : 4251 )

// Includes
#include "buildsettings.h"
#include "containers.h"
#include <stdint.h>

namespace FlexKit
{
	class Timer;
	typedef void TIMER_CALLBACKFN (Timer*, void* args);

	class Timer
	{
	public:
		Timer(TIMER_CALLBACKFN Callback, void*, int64_t Duration, bool repeat );
		virtual ~Timer();

		inline	void	SetDuration( int64_t Duration );
		inline	void	reset();

		bool Update( int64_t mdT );

		TIMER_CALLBACKFN* mCallback;

		void*	mArgs;
		bool	mRepeat;
		int64_t	mTimeElapsed;
		int64_t	mDuration;
	};

	class Time
	{
	public:
		Time(iAllocator* Alloc);

		void PrimeLoop();
		void Before();
		void After();

		double	GetTimeSinceLastFrame();
		int64_t GetTimeSinceLastFrame64();
		int64_t GetTimeResolution();

		float	GetSystemTime();
		double	GetAveragedFrameTime();

		int64_t DoubletoUINT64( double Seconds );

		void	AddTimer( Timer* Timer );
		void	RemoveTimer( Timer* );
	
		void	Update();
		void	UpdateTimers();

		void	Clear();


		static const size_t	TimeSampleCount = 30;

		uint64_t mStartCount;
		uint64_t mFrameStart;
		uint64_t mFrameEnd;
		uint64_t mEndCount;
		uint64_t mFrequency;
		uint64_t mdT;

		uint64_t mTimesamples[TimeSampleCount];
		size_t	 mSampleCount;
		size_t	 mIndex;

		double	mdAverage;
		double	mTimeSinceLastFrame;

		Vector<Timer*> mTimers;
	};
}


/**********************************************************************

Copyright (c) 2015 - 2022 Robert May

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
