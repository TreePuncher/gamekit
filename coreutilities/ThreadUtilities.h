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

	class ThreadManager;

	typedef std::function<void()> OnCompletionEvent;


	class iWork
	{
	public:
		iWork & operator = (iWork& rhs) = delete;

		iWork(iAllocator* Memory) : Watchers{Memory}	{}
		~iWork()					{ Watchers.Release();}

		virtual void Run() {}

		void NotifyWatchers()
		{
			for (auto& Watcher : Watchers)
				Watcher();
		}

		void Subscribe(OnCompletionEvent CallMeLater) 
		{ 
			Watchers.push_back(CallMeLater);
		}

		operator iWork* () { return this; }

	private:

		Vector<OnCompletionEvent> Watchers;
	};

	struct WorkContainer
	{
		WorkContainer(iWork* IN_ptr) :
			ptr{ IN_ptr } {}

		iWork* operator -> ()
		{
			return ptr;
		}

		iWork* ptr;
	};

	typedef Deque_MT<WorkContainer>::Element_TY WorkItem;
	typedef	Deque_MT<WorkContainer>				WorkQueue;

	//__declspec(align(64))
	class WorkerThread
	{
	public:
		WorkerThread(iAllocator* ThreadMemory = nullptr) :
			Thread		{ [this] {Run(); } },
			Running		{ false },
			Quit		{ false },
			Allocator	{ ThreadMemory }
		{}

		bool AddItem(WorkItem* Work);
		void Shutdown();
		void Run();
		bool IsRunning();
		void Wake();

		//iWork* Steal();

		static ThreadManager*	Manager;

	private:
		std::condition_variable	CV;
		std::atomic_bool		Running;
		std::atomic_bool		Quit;
		std::thread				Thread;

		WorkQueue				WorkList;

		iAllocator*				Allocator;
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
	WorkItem& CreateLambdaWork_New(
		TY_FN FNIN, 
		iAllocator* Memory_1 = FlexKit::SystemAllocator,
		iAllocator* Memory_2 = FlexKit::SystemAllocator)
	{
		return Memory_1->allocate<WorkItem>(Memory_1->allocate<LambdaWork<TY_FN>>(FNIN, Memory_2));
	}

	/************************************************************************************************/


	class ThreadManager
	{
	public:
		ThreadManager(size_t ThreadCount = 4, iAllocator* memory = FlexKit::SystemAllocator) :
			Threads				{},
			Memory				{memory},
			WorkingThreadCount	{0}
		{
			WorkerThread::Manager = this;

			for (size_t I = 0; I < ThreadCount; ++I)
				Threads.push_back(memory->allocate<Deque_MT<WorkerThread>::Element_TY>(memory));
		}


		void Release()
		{
			WaitForWorkersToComplete();

			SendShutdown();

			WaitForShutdown();
		}


		void SendShutdown()
		{
			for (auto& I : Threads)
				I.Shutdown();
		}


		void AddWork(WorkItem* NewWork)
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
				Work.push_back(*NewWork);
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
					WorkItem* WorkItem = nullptr;
					if (Work.try_pop_front(&WorkItem))
					{
						WorkItem->ptr->Run();
						WorkItem->ptr->NotifyWatchers();
					}
				}
			}
		}


		void RotateThreads()
		{
			Deque_MT<WorkerThread>::Element_TY* Ptr = nullptr;

			auto success = Threads.try_pop_front(&Ptr);

			if (success)
				Threads.push_back(*Ptr);
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


	private:

		Deque_MT<WorkerThread>		Threads;
		Deque_MT<WorkContainer>		Work; // for the case of a single thread, work is pushed here and process on Wait for workers to complete


		std::condition_variable		CV;
		std::atomic_int				WorkingThreadCount;

		iAllocator*		Memory;
	};


	/************************************************************************************************/


	class WorkBarrier
	{
	public:
		WorkBarrier(
			iAllocator* Memory = FlexKit::SystemAllocator) :
				PostEvents{ Memory }
		{
		}

		~WorkBarrier() {}

		WorkBarrier(const WorkBarrier&)					= delete;
		WorkBarrier& operator = (const WorkBarrier&)	= delete;

		void AddDependentWork		(iWork* Work);
		void AddOnCompletionEvent	(OnCompletionEvent Callback);
		void Wait					();

	private:
		ThreadManager*				Threads;
		Vector<OnCompletionEvent>	PostEvents;

		std::condition_variable		CV;
		std::atomic_int				TaskInProgress;
	};


	/************************************************************************************************/
}	// namespace FlexKit

#endif
