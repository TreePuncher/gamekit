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

#ifndef THREADUTILITIES_H
#define THREADUTILITIES_H

#include "..\pch.h"
#include "timeutilities.h"
#include "ThreadUtilities.h"
#include <atomic>
#include <chrono>

using std::mutex;

namespace FlexKit
{
	/************************************************************************************************/


	bool WorkerThread::AddItem(iWork* Work)
	{
		std::scoped_lock lock{ exclusive };

		if (Quit)
			return false;

		if (workList.full())
			return false;

		auto res = workList.push_back(Work);

		if(res)
			CV.notify_all();

		return res;
	}


	/************************************************************************************************/


	void WorkerThread::Shutdown()
	{
		Quit = true;
		CV.notify_all();
	}


	/************************************************************************************************/


	void WorkerThread::Run()
	{
		Running.store(true);

		EXITSCOPE({
			Running.store(false);
		});


		while (true)
		{
			std::mutex						M;
			std::unique_lock<std::mutex>	lock{ M };

			if (workList.empty() && !Quit) {
				Running.store(false, std::memory_order_release);

				CV.wait(lock,
					[this]() -> bool
					{
						return !workList.empty() || Quit;
					});

				Running.store(true, std::memory_order_release);
			}
			{
				EXITSCOPE({
					Manager->DecrementActiveWorkerCount();
				});

				Manager->IncrementActiveWorkerCount();

				auto doWork = [&](iWork* work)
				{
					if (work) 
					{
						hasJob.store(true, std::memory_order_release);

						work->Run();
						work->Release();
						work->NotifyWatchers();

						hasJob.store(false, std::memory_order_release);

						return true;
					}
					return false;
				};

				auto getWorkItem = [&]()  -> iWork*
				{
					std::scoped_lock lock{ exclusive };
					if (workList.empty())
						return nullptr;

					return workList.pop_front();
				};


				while (!workList.empty())
				{
					auto workItem = getWorkItem();

					if (workItem)
						doWork(workItem);
				}


				auto stealWork = [&](auto& begin, auto& end)
				{
					for (auto worker = begin; workList.empty() && worker != end && worker != nullptr; ++worker)
					{
						if (iWork* work = worker->Steal(); work )
						{
							doWork(work);
							return;
						}
					}
				};


				stealWork(
					Deque_MT<WorkerThread>::Iterator{ this },
					Manager->GetThreadsEnd());

				stealWork(
					Manager->GetThreadsBegin(),
					Deque_MT<WorkerThread>::Iterator{ this });


				if (workList.empty() && Quit)
					return;
			}
		}

	}


	/************************************************************************************************/


	bool WorkerThread::IsRunning()
	{
		return Running.load();
	}


	bool WorkerThread::HasJob()
	{
		return hasJob;
	}


	void WorkerThread::Wake()
	{
		CV.notify_all();
	}


	bool WorkerThread::hasWork()
	{
		return !workList.empty();
	}

	
	/************************************************************************************************/


	iWork* WorkerThread::Steal()
	{

		if (workList.empty())
			return nullptr;

		std::scoped_lock lock{ exclusive };
		return (!workList.empty()) ? workList.pop_back() : nullptr;
	}


	/************************************************************************************************/


	void WorkBarrier::AddDependentWork(iWork* Work)
	{
		FK_ASSERT(Work != nullptr);

		++TasksInProgress;

		Work->Subscribe(
			[&]
			{
				if(--TasksInProgress == 0)
					CV.notify_all();
			});
	}


	/************************************************************************************************/


	void WorkBarrier::AddOnCompletionEvent(OnCompletionEvent Callback)
	{
		PostEvents.push_back(Callback);
	}


	/************************************************************************************************/


	void WorkBarrier::Wait()
	{
		if (TasksInProgress == 0)
			return;

		std::mutex M;
		std::unique_lock<std::mutex> lock(M);

		CV.wait(
			lock, 
			[this]()->bool
			{
				return TasksInProgress.load(std::memory_order_seq_cst) == 0;
			});

		for (auto Evt : PostEvents)
			Evt();
	}


	void WorkBarrier::Join()
	{
		do
		{
			if (TasksInProgress.load(std::memory_order_relaxed) == 0)
				return;

			auto work = threads.StealSomeWork();

			if (work)
			{
				work->Run();
				work->Release();
				work->NotifyWatchers();
			}
		} while (true);
	}



	ThreadManager* WorkerThread::Manager = nullptr;
}	/************************************************************************************************/


#endif