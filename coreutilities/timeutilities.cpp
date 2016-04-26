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

#include "..\pch.h"
#include "timeutilities.h"
#include <windows.h>

//**********************************************************************

namespace FlexKit
{
	Timer::Timer( TIMER_CALLBACKFN* Callback, int64_t Duration, bool repeat )
	{
		mCallback = Callback;
		mDuration = Duration;
		mRepeat = repeat;
		mTimeElapsed = 0;
	}
	//**********************************************************************
	Timer::~Timer()
	{}
	//**********************************************************************
	bool Timer::Update( int64_t mdT )
	{
		mTimeElapsed += mdT;
		if( mTimeElapsed >= mDuration )
		{
			mCallback( this );

			if( mRepeat )
			{
				mTimeElapsed = 0;
				return false;
			}
			return true;
		}
		return false;
	}
	//**********************************************************************
	void Timer::reset()
	{
		mTimeElapsed = 0;
	}
	//**********************************************************************
	void Timer::SetDuration( int64_t Duration )
	{
		mDuration = Duration;
	}
	//**********************************************************************
	Time::Time( void )
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&mFrequency);
		mStartCount			= 0;
		mEndCount			= 0;
		mdT				= 0;
		mSampleCount		= 0;
		mTimeSinceLastFrame	= 0;
		mIndex				= 0;
		mdAverage			= 0;

		mFrameStart		= 0;
		mFrameEnd		= 0;

		for( auto& sample : mTimesamples )
			sample = 0;
	}
//**********************************************************************
	void Time::AddTimer( Timer* Timer_ptr )
	{
		auto itr = mTimers.begin();
		while( itr != mTimers.end() )
		{
			if( *itr == Timer_ptr )
				return;
			++itr;
		}
		mTimers.push_back( Timer_ptr );
	}
//**********************************************************************
	void Time::RemoveTimer( Timer* Timer_ptr )
	{
		auto itr = mTimers.begin();
		while( itr != mTimers.end() )
		{
			if( *itr == Timer_ptr )
			{
				*itr = 0;
			}
			itr++;
		}
	}
//**********************************************************************
	void Time::Update()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&mFrequency);
		QueryPerformanceCounter((LARGE_INTEGER*)&mFrameEnd);
		int64_t delta2 = mFrameEnd - mFrameStart;

		UpdateTimers();
		auto index = mIndex%TimeSampleCount;
		mTimesamples[mIndex%TimeSampleCount] = delta2;
		
		mIndex++;
		mSampleCount++;

		double total_Time = 0;
		for( auto sample : mTimesamples )
			total_Time += sample;

		if( TimeSampleCount <= mSampleCount )
			mSampleCount = 30;

		mdAverage = (total_Time / (mSampleCount) )/mFrequency;
	}
//**********************************************************************
	void Time::UpdateTimers()
	{
		auto itr = mTimers.begin();
		while( mTimers.size() > 0 && itr != mTimers.end() )
		{
			bool skipIncrement = false;

			// return true if it loops else it deletes the Timer
			if( !(*itr)->Update( mdT ) )
			{
				auto _ptr = *itr;

				if( *itr == mTimers.front() )
					skipIncrement = true;

				*itr = 0;
				delete _ptr;

				if( !skipIncrement )
					itr = mTimers.begin();
			}
			if( !skipIncrement )
				itr++;
		}
	}

//**********************************************************************
	inline void Time::PrimeLoop() 
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&mStartCount);
	}
	//**********************************************************************
	inline void Time::Before()
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&mStartCount);
		mFrameStart = mStartCount;
	}
	//**********************************************************************

	inline void Time::After() 
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&mEndCount);
		mFrameEnd = mEndCount;
	}
	//**********************************************************************
	double Time::GetTimeSinceLastFrame()
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&mFrequency);
		QueryPerformanceCounter((LARGE_INTEGER*)&mEndCount);
		int64_t delta2 = mEndCount - mStartCount;
		auto Temp = ((double)delta2) / ((double)mFrequency);
		return Temp;
	}
	//**********************************************************************
	int64_t Time::GetTimeSinceLastFrame64()
	{
		return mdT;
	}
	//**********************************************************************
	int64_t Time::GetTimeResolution()
	{
		return mFrequency;
	}
	//**********************************************************************
	float Time::GetSystemTime()
	{
		return (float)GetTickCount64();
	}
	//**********************************************************************
	double Time::GetAveragedFrameTime()
	{
		return mdAverage;
	}
	//**********************************************************************
	int64_t Time::DoubletoUINT64( double Seconds )
	{
		int64_t intSeconds = Seconds;
		return intSeconds * mFrequency;
	}
	//**********************************************************************
	void Time::Clear()
	{
		memset(mTimesamples, 0,		sizeof(mTimesamples));
		
		mTimers.clear();
		mSampleCount			= 0;
		mIndex					= 0;
		mdAverage				= 0.0f;
		mTimeSinceLastFrame		= 0.0f;
	}
}