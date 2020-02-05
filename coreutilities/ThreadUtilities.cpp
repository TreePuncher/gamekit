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


	bool _WorkerThread::AddItem(iWork* Work) noexcept
	{
		std::unique_lock lock{ exclusive };

		workCount++;

		if (Quit)
			return false;

		workQueue.push_back(Work);

		if (!Running)
			CV.notify_all();

		return true;
	}


	/************************************************************************************************/


	void _WorkerThread::Shutdown() noexcept
	{
		Quit = true;
		CV.notify_all();
	}


	/************************************************************************************************/


	void _WorkerThread::_Run()
	{
		Running.store(true);

		EXITSCOPE({
			Running.store(false);
		});

		while (true)
		{
			auto doWork = [&](iWork* work)
			{
				if (work) 
				{
					hasJob.store(true, std::memory_order_release);

					work->Run();
					work->NotifyWatchers();
					work->Release();

					tasksCompleted++;

					hasJob.store(false, std::memory_order_release);

					return true;
				}
				return false;
			};


            for(size_t I = 0; I < 10; I++)
			{
				EXITSCOPE({
					Manager->DecrementActiveWorkerCount();
					});

				    Manager->IncrementActiveWorkerCount();

                    if (auto workItem = Manager->FindWork(); workItem)
                    {
                        I = 0;
                        doWork(workItem);
                    }
                    else if(I > 1)
                    {
                        const auto beginTimePoint = std::chrono::high_resolution_clock::now();

                        while (true)
                        {
                            const auto currentTime    = std::chrono::high_resolution_clock::now();
                            const auto duration       = currentTime - beginTimePoint;

                            if (duration > 1000ns)
                                break;
                        }

                    }
			}

			Manager->WaitForWork();

			if (Quit)
				return;
		}

	}


	/************************************************************************************************/


	bool _WorkerThread::IsRunning() noexcept
	{
		return Running.load(std::memory_order_relaxed);
	}


	bool _WorkerThread::HasJob() noexcept
	{
		return hasJob;
	}


	void _WorkerThread::Wake() noexcept
	{
		CV.notify_all();
	}


	bool _WorkerThread::hasWork() noexcept
	{
		return !localWorkQueue->empty();
	}

	
	/************************************************************************************************/


	iWork* _WorkerThread::Steal() noexcept
	{

		return localWorkQueue->Steal().value_or(nullptr);
	}


	/************************************************************************************************/


	void WorkBarrier::AddWork(iWork* work)
	{
		FK_ASSERT(work != nullptr);

		workList.push_back(work);

		++tasksInProgress;

		work->Subscribe(
			[this]
			{
				auto prev = tasksInProgress.fetch_sub(1); FK_ASSERT(prev > 0);

				if (prev == 1)
					_OnEnd();
			});
	}


	/************************************************************************************************/


	void WorkBarrier::AddOnCompletionEvent(OnCompletionEvent Callback)
	{
		PostEvents.emplace_back(std::move(Callback));
	}


	/************************************************************************************************/


	void WorkBarrier::Wait()
	{
		do
		{
			if (!inProgress)
				return;
		} while (true);
	}


	/************************************************************************************************/


	void WorkBarrier::Join()
	{
		FK_ASSERT(workList.size() >= tasksInProgress);

		if (!workList.size())
			return;

		do
		{
			if (!inProgress)
				return;

			for(auto work = threads.FindWork(); work; work = threads.FindWork())
			{
				work->Run();
				work->NotifyWatchers();
				work->Release();
			}
		} while (true);
	}


	ThreadManager* WorkerThread::Manager = nullptr;
}	/************************************************************************************************/


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

#endif
