/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef CTHREAD_H
#define CTHREAD_H

	// includes
#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <functional>
#include <stdint.h>

#define MAXTHREADCOUNT 3

#pragma warning ( disable : 4251 )

namespace FlexKit
{
	using std::atomic;
	using std::atomic_bool;

	class Thread;
	class TaskRendezvous;
	class ThreadScheduler;

	template<typename Ty_1, unsigned int MAX = 16>
	class SRSW_Queue
	{
	public:
		SRSW_Queue()
		{
			ZeroMemory( m_Waiting, sizeof( m_Waiting ) );

			m_writer_index = 0;
			m_reader_index = 0;
		}
		/************************************************************************************************/
		bool Push( Ty_1& in )
		{
			if( IsFull() )
				return false;
	
			unsigned int index = m_writer_index;
			m_Waiting[index % MAX] = in;
	
			_WriteBarrier();
			m_writer_index = index + 1;
	
			return true;
		}
		/************************************************************************************************/
		bool Pop( Ty_1& out )
		{
			if( !IsEmpty() )
			{
				unsigned int index = m_reader_index;
				out = m_Waiting[ m_reader_index % MAX ];
				m_reader_index = index + 1;
	
				return true;
			}
			return false;
		}
	private:
		/************************************************************************************************/
		bool IsEmpty()
		{
			return ( m_writer_index == m_reader_index );
		}
		/************************************************************************************************/
		bool IsFull()
		{
			return ( m_writer_index - m_reader_index  == MAX );
		}	 
		/************************************************************************************************/
	
	private:
		__declspec(align(4))	std::atomic_char32_t m_writer_index;
		__declspec(align(4))	std::atomic_char32_t m_reader_index;
	
		Ty_1 m_Waiting[MAX]; // Buffer 
	};

	class FLEXKITAPI ThreadTask
	{
	public:
		ThreadTask() { mDelete = false; }

		virtual ~ThreadTask(){}
		virtual void Run()	{}
		//
		

		bool mDelete;
	};

	//-----------------------------------------------------------------------------------------

	typedef std::function<void ()> Task_Function_Def;

	class LambdaTask : public ThreadTask
	{
	public:
		LambdaTask( Task_Function_Def lambda_func ) : m_Lambda( lambda_func ) {}

	private:
		LambdaTask( const LambdaTask *);

		void				Run()
		{ 
			if( m_Lambda )	m_Lambda(); 
			delete this;
		}

		Task_Function_Def	m_Lambda;
	};



	__declspec(align(64))
	class ThreadWorker
	{
	public:
		ThreadWorker(iAllocator* Memory) : JobList(Memory)
		{
			Thread = std::thread([&]() {_Run(); });
		}

		void Initate()
		{
		}


		void Release()
		{

		}

		void Shutdown()
		{

		}

		void AddJob(ThreadTask* Worker)
		{
			JobList.push_back(Worker);
		}

		bool isRunning()
		{
			return false;
		}

		size_t QueueSize()
		{
			return JobList.size();
		}

	private:

		void _Run()
		{

		}

		std::thread	Thread;

		SL_list<ThreadTask*>	JobList;
		std::mutex				Mutex;
		std::condition_variable	Condition;
	};


	//-----------------------------------------------------------------------------------------


	class ThreadManager
	{
	public:
		ThreadManager(iAllocator* memory = nullptr, size_t ThreadCount = -1) :
			Workers(memory),
			Memory(memory)
		{
			if (ThreadCount == -1)
				ThreadCount = std::thread::hardware_concurrency() - 1;

			for (size_t I = 0; I < ThreadCount; ++I)
				Workers.push_back(&Memory->allocate_aligned<ThreadWorker, 64>(Memory));
		}


		void AddTask(ThreadTask* Task)
		{
			std::sort(Workers.begin(), Workers.end(), [&](ThreadWorker* LHS, ThreadWorker* RHS) {
				return LHS->QueueSize() < RHS->QueueSize();
			});
		}


		void Release()
		{
			for (auto& Worker : Workers)
				Worker->Shutdown();

			for (bool ThreadRunning = false; ThreadRunning;)
			{
				for (auto& Worker : Workers)
					ThreadRunning |= !Worker->isRunning();
			}

			for (auto Worker : Workers)
				Memory->free(Worker);

			Workers.Release();
		}


	private:
		Vector<ThreadWorker*>	Workers;
		iAllocator*				Memory;
	};


	const unsigned MAXQUEUEDTASKS = 1000;
	const unsigned MaxThreads = MAXTHREADCOUNT;


	//-----------------------------------------------------------------------------------------



#define MAXTASKCOUNT	100
	struct TaskProxy;

	struct TaskProxy : public ThreadTask
	{
		void Run();

		TaskRendezvous*		mRend;
		atomic<bool>		mRunning;
		ThreadTask*			mTask;

		TaskProxy()
		{
			mRend = nullptr;
			mTask = nullptr;
		}
		TaskProxy( const TaskProxy& in )
		{
			bool Val = in.mRunning;
			mRunning = Val;
		}
	};

	class FLEXKITAPI TaskRendezvous
	{
	public:
		TaskRendezvous();
		~TaskRendezvous();

		bool AddTask(ThreadTask* _ptr);
		bool TasksAreStillRunning();
		void WaitforRendezvous();

		ThreadTask*	_GetNextTask();

	private:
		TaskRendezvous( const TaskRendezvous & ) = delete;

		std::mutex					mReadlock;
		std::mutex					mWritelock;
		list_t<ThreadTask*>			mTasksWaiting;
		static_vector<TaskProxy,16>	mProxyPool;
	};


	class FLEXKITAPI JobGroup
	{
	public:
		bool AddTask(ThreadTask* _ptr);

		static_vector<TaskProxy, 16> Workers;
	};


	//-----------------------------------------------------------------------------------------
#define BEGINTHREADEDSECTION FlexKit::AddTask( new FlexKit::LambdaTask( [&]() {
#define ENDTHREADEDSECTION } ) );

#define Begin_TASKCREATION new FlexKit::LambdaTask( [&]() {
#define End_TASKCREATION } )
}

#endif
