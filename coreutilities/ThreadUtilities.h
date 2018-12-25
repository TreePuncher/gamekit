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
#include <iostream>

#define MAXTHREADCOUNT 3

#pragma warning ( disable : 4251 )



namespace FlexKit
{
	using std::atomic;
	using std::atomic_bool;

	class ThreadManager;

	typedef std::function<void()> OnCompletionEvent;


	/************************************************************************************************/


	class iWork : public DequeNode_MT
	{
	public:
		iWork & operator = (iWork& rhs) = delete;

		//iWork(iAllocator* Memory) : 
		//	Watchers{Memory}	{}

		iWork(iAllocator* Memory) {}

		//~iWork()					{ Watchers.Release();}

		virtual void Run() {}
		virtual void Release() {}

		void NotifyWatchers()
		{
			if (!Watchers.size())
				return;

			size_t end = Watchers.size();
			for (size_t itr = 0; itr < end; ++itr) {
				test++;
				Watchers[itr]();
			}
		}

		void Subscribe(OnCompletionEvent CallMeLater) 
		{ 
			Watchers.push_back(CallMeLater);
		}

		operator iWork* () { return this; }

	private:
		atomic_int					test = 0;
		bool						notifiedWatchers = false;
		//Vector<OnCompletionEvent>	Watchers;
		static_vector<OnCompletionEvent>	Watchers;
	};


	/************************************************************************************************/


	typedef	Deque_MT<iWork>	WorkQueue;

	class WorkerThread : public DequeNode_MT
	{
	public:
		WorkerThread(iAllocator* ThreadMemory = nullptr) :
			Thread		{ [this] {Run(); } },
			Running		{ false },
			Quit		{ false },
			Allocator	{ ThreadMemory }
		{}

		bool AddItem(iWork* Work);
		void Shutdown();
		void Run();
		bool IsRunning();
		void Wake();

		iWork* Steal();

		static ThreadManager*	Manager;

	private:
		std::condition_variable	CV;
		std::atomic_bool		Running;
		std::atomic_bool		Quit;
		std::thread				Thread;

		WorkQueue				WorkList;

		iAllocator*				Allocator;
		std::atomic_bool		pad;
	};


	/************************************************************************************************/


	template<typename TY_FN>
	class LambdaWork : public iWork
	{
	public:
		LambdaWork(TY_FN& FNIN, iAllocator* Memory = FlexKit::SystemAllocator) :
			iWork{ Memory },
			Callback{ FNIN } {}

		void Run()
		{
			Callback();
		}

		TY_FN Callback;
	};

	template<typename TY_FN>
	LambdaWork<TY_FN> CreateLambdaWork(TY_FN FNIN, iAllocator* Memory = FlexKit::SystemAllocator)
	{
		return LambdaWork<TY_FN>(FNIN, Memory);
	}

	template<typename TY_FN>
	iWork& CreateLambdaWork_New(
		TY_FN FNIN, 
		iAllocator* Memory_1 = FlexKit::SystemAllocator,
		iAllocator* Memory_2 = FlexKit::SystemAllocator)
	{
		return Memory_1->allocate<LambdaWork<TY_FN>>(FNIN, Memory_2);
	}

	/************************************************************************************************/


	class ThreadManager
	{
	public:
		ThreadManager(size_t ThreadCount = 4, iAllocator* memory = FlexKit::SystemAllocator) :
			Threads				{},
			Memory				{memory},
			WorkingThreadCount	{0},
			WorkerCount			{ThreadCount}
		{
			WorkerThread::Manager = this;

			for (size_t I = 0; I < ThreadCount; ++I)
				Threads.push_back(memory->allocate<WorkerThread>(memory));
		}


		void Release()
		{
			WaitForWorkersToComplete();

			SendShutdown();

			WaitForShutdown();

			while (!Threads.empty())
			{
				WorkerThread* Worker = nullptr;
				if (Threads.try_pop_back(Worker))
					Memory->free(Worker);
			}

		}


		void SendShutdown()
		{
			for (auto& I : Threads)
				I.Shutdown();
		}


		void AddWork(iWork* NewWork, iAllocator* Allocator = SystemAllocator)
		{
			if (!Threads.empty())
			{
				bool success = false;
				do {
					success = Threads.begin()->AddItem(NewWork);
					RotateThreads();
				} while (!success);
			}
			else
			{
				Work.push_back(NewWork);
			}
		}


		void WaitForWorkersToComplete()
		{
			if (!Threads.empty())
			{
				std::mutex						M;
				std::unique_lock<std::mutex>	Lock(M);

				CV.wait(Lock, [this] { return WorkingThreadCount <= 0; });
			}
			else
			{
				while(!Work.empty())
				{
					iWork* WorkItem = nullptr;
					if (Work.try_pop_front(WorkItem))
					{
						WorkItem->Run();
						WorkItem->NotifyWatchers();
					}
				}
			}
		}


		void RotateThreads()
		{
			WorkerThread* Ptr = nullptr;

			auto success = Threads.try_pop_front(Ptr);

			if (success)
				Threads.push_back(Ptr);
		}


		void WaitForShutdown() // TODO: this does not work!
		{
			bool ThreadRunning = false;
			do
			{
				ThreadRunning = false;
				for (auto& I : Threads) {
					ThreadRunning |= I.IsRunning();
					I.Shutdown();
				}
			} while (ThreadRunning);
		}


		void IncrementActiveWorkerCount()
		{
			WorkingThreadCount++;
			CV.notify_all();
		}


		void DecrementActiveWorkerCount()
		{
			WorkingThreadCount--;
			CV.notify_all();
		}

		iWork* StealSomeWork()
		{
			if (!Threads.empty())
				return nullptr;

			iWork* StolenWork = Threads.begin()->Steal();
			RotateThreads();

			return StolenWork;
		}

		size_t GetThreadCount() const
		{
			return WorkerCount;
		}


	private:

		Deque_MT<WorkerThread>		Threads;
		Deque_MT<iWork>				Work; // for the case of a single thread, work is pushed here and process on Wait for workers to complete


		std::condition_variable		CV;
		std::atomic_int				WorkingThreadCount;

		const size_t	WorkerCount;
		iAllocator*		Memory;
	};


	/************************************************************************************************/


	class WorkBarrier
	{
	public:
		WorkBarrier(
			iAllocator* Memory = FlexKit::SystemAllocator) :
				PostEvents{ Memory }
		{}

		~WorkBarrier() {}

		WorkBarrier(const WorkBarrier&)					= delete;
		WorkBarrier& operator = (const WorkBarrier&)	= delete;

		int  GetDependentCount		(){ return TasksInProgress; }
		void AddDependentWork		(iWork* Work);
		void AddOnCompletionEvent	(OnCompletionEvent Callback);
		void Wait					();

	private:
		Vector<OnCompletionEvent>	PostEvents;

		std::condition_variable		CV;
		std::atomic_int				TasksInProgress	= 0;
	};


	/************************************************************************************************/


	class WorkDependencyWait
	{
	public:
		WorkDependencyWait(iWork& IN_work, ThreadManager* IN_threads, iAllocator* IN_allocator = SystemAllocator) :
			work			{ IN_work		},
			allocator		{ IN_allocator	},
			threads			{ IN_threads	},
			dependentCount	{ 0 }{}

		WorkDependencyWait				(const WorkDependencyWait&) = delete;
		WorkDependencyWait& operator =	(const WorkDependencyWait&) = delete;


		void AddDependency(iWork& waitsOn)
		{
			dependentCount++;

			waitsOn.Subscribe(
				[this]() {
					dependentCount--;
					if (!dependentCount && scheduleLock.try_lock()) // Leave locked!
						threads->AddWork(&work, allocator);
					});
		}

		iWork&						work;
		ThreadManager*				threads;
		iAllocator*					allocator;
		std::atomic_int				dependentCount;
		std::mutex					scheduleLock;
		Vector<OnCompletionEvent>	PostEvents;
	};


	/************************************************************************************************/
}	// namespace FlexKit

#endif
