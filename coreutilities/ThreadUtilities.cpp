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
#include "ThreadUtilities.h"
#include <atomic>
#include <chrono>

using std::mutex;

namespace FlexKit
{
	size_t						gThreadCount = 0;
	list_t<ThreadTask*>			gTaskBoard;
	std::mutex					gQueueWriteLock;
	vector_t<Thread*>			gThreadpool;

	void			AddTask( ThreadTask* _ptr );
	ThreadTask*		GetNextTask();
	void			InitThreads( size_t threadcount );
	bool			WakeNextThread();

	class Thread
	{
		public:
			Thread() : mTask{nullptr}, mThread{}
			{
				mSpin	 = true;
				mRunning = false;
			}
			
			
			void Begin()
			{
				mLock.lock();
				mThread = std::move( std::thread( &Thread::operator(), this ) );
				mLock.unlock();
			}


			void operator()()
			{
				mLock.lock();
				mRunning = true;
				while( mRunning || mSpin )
				{
					auto task = mTask.load();
					if( task != nullptr )
					{
						task->Run();
						//if( task->mDelete )
						//	delete task;
						mTask.store(nullptr);
					}
					ThreadTask* NextTask = GetNextTask();
					mTask = NextTask;

					if( !NextTask )
					{	
						std::this_thread::sleep_for(std::chrono::microseconds(1));
						if(!mSpin)
						goto END;
					}
				}
				END:
				mThread.detach();
				mRunning = false;
				mLock.unlock();
			}
			
			
			void SetSpin( bool spin )
			{
				mSpin = spin;
			}
			
			
			bool SetTask( ThreadTask* Task )
			{
				if( !mRunning )
				{
					// Queued
					mTask.store( Task );
					return true;
				}
				return false;
			}


			bool Running()
			{
				return mRunning;
			}


		std::atomic_bool			mRunning;
		std::atomic_bool			mSpin;
		std::mutex					mLock;
		std::atomic<ThreadTask*>	mTask;
		std::thread					mThread;
	};

	ThreadTask*	GetNextTask()
	{
		gQueueWriteLock.lock();
		ThreadTask* task = nullptr;
		if( gTaskBoard.size() )
		{
			task = gTaskBoard.front();
			gTaskBoard.pop_front();
		}
		gQueueWriteLock.unlock();
		return task;
	}


	void InitThreads( size_t threadcount )
	{
		gThreadCount = threadcount;
		for( int itr = threadcount; itr-->0; )
		{
			gThreadpool.push_back( new Thread());
			gThreadpool.back()->Begin();
		}

	}


	bool WakeNextThread()
	{
		for( auto thread : gThreadpool )
			if( !thread->Running() )
			{
				thread->SetSpin(true);
				thread->Begin();
				return true;
			}
		return false;
	}



	void AddTask( ThreadTask* _ptr )
	{
		if( !gThreadCount )
		{
			_ptr->Run();
			return;
		}
		gQueueWriteLock.lock();
		gTaskBoard.push_back(_ptr);
		gQueueWriteLock.unlock();
	}


	void TaskProxy::Run()
	{
		while( mRunning )
		{
			auto task = mTask;
			if( task != nullptr )
			{
				task->Run();
				if( task->mDelete )
					delete task;
				mTask = nullptr;
			}
			ThreadTask* NextTask = mRend->_GetNextTask();
			mTask = NextTask;

			if( NextTask == nullptr )
				goto END;
		}
	END:
		mRunning = false;
	}


	TaskRendezvous::TaskRendezvous()
	{
		for( auto itr = gThreadCount; itr-- > 1; )
		{
			mProxyPool.push_back( TaskProxy() );
			mProxyPool.back().mRend = this;

		}
	}


	TaskRendezvous::~TaskRendezvous()
	{
		WaitforRendezvous(); 
	}


	bool TaskRendezvous::AddTask( ThreadTask* _ptr )
	{
		if( !gThreadCount )
		{
			_ptr->Run();
			if( _ptr->mDelete )
				delete _ptr;
			return true;
		}
		mWritelock.lock();
		mTasksWaiting.push_back( _ptr );
		mWritelock.unlock();
		return true;
	}


	ThreadTask*	TaskRendezvous::_GetNextTask()
	{
		
		mWritelock.lock();
		ThreadTask* task = nullptr;
		if( mTasksWaiting.size() )
		{
			task = mTasksWaiting.front();
			mTasksWaiting.pop_front();
		}
		mWritelock.unlock();
		return task;
	}


	bool TaskRendezvous::TasksAreStillRunning()
	{
		size_t itr = 0;
		size_t size = mProxyPool.size();
		while( itr++ < size )
			if( mProxyPool[itr].mRunning )
				return true;
		return false;
	}


	void TaskRendezvous::WaitforRendezvous()
	{
		while( mTasksWaiting.size() )
		{
			ThreadTask* task = _GetNextTask();
			if( task )
				task->Run();
		}
		bool debug = TasksAreStillRunning();
		while( debug )
			std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}