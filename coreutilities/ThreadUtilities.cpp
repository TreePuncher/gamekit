#ifndef THREADUTILITIES_H
#define THREADUTILITIES_H

#include "..\pch.h"
#include "timeutilities.h"
#include "ThreadUtilities.h"
#include <atomic>
#include <chrono>

using std::mutex;

namespace FlexKit
{   /************************************************************************************************/


    thread_local CircularStealingQueue<iWork*>* localWorkQueue  = nullptr;
    thread_local _WorkerThread*                 _localThread    = nullptr;
    thread_local iAllocator*                    _localAllocator = nullptr;


    FLEXKITAPI inline void PushToLocalQueue(iWork& work)
    {
        localWorkQueue->push_back(&work);
    }


    FLEXKITAPI inline CircularStealingQueue<iWork*>& _GetThreadLocalQueue()
    {
        return *localWorkQueue;
    }


    FLEXKITAPI inline void _SetThreadLocalQueue(CircularStealingQueue<iWork*>& localQueue)
    {
        localWorkQueue = &localQueue;
    }


    FLEXKITAPI inline _WorkerThread& GetLocalThread()
    {
        return *_localThread;
    }



    FLEXKITAPI inline iAllocator& GetThreadLocalAllocator()
    {
        return *_localAllocator;
    }

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

                    RunTask(*work, localAllocator);

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

                    if (auto workItem = Manager->FindWork(true); workItem)
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

                            if (duration > std::chrono::nanoseconds(1000))
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


    /************************************************************************************************/


    void _WorkerThread::Start() noexcept
    {
        Thread = std::move(
            std::thread([&]
                {
                    localWorkQueue = &workQueue;
                    _localThread = this;

                    buffer = std::make_unique<std::array<byte, MEGABYTE * 16>>();
                    localAllocator.Init(buffer->data(), MEGABYTE * 16);

                    _Run();
                }));
    }


    /************************************************************************************************/


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


    _BackgrounWorkQueue::_BackgrounWorkQueue(iAllocator* allocator) :
        queue{ allocator },
        workList{ allocator }
    {
        backgroundThread = std::thread{
            [&]
            {
                localWorkQueue = &queue;
                running = true;

                Run();
            } };
    }



    void _BackgrounWorkQueue::Shutdown()
    {
        running = false;
        cv.notify_all();

        if(backgroundThread.joinable())
            backgroundThread.join();
    }


    void _BackgrounWorkQueue::PushWork(iWork& work)
    {
        std::scoped_lock localLock{ lock };
        workList.push_back(work);

        cv.notify_all();
    }


    void _BackgrounWorkQueue::Run()
    {
        while (running)
        {
            if (workList.size())
            {
                std::scoped_lock localLock{ lock };
                while (workList.size())
                    queue.push_back(workList.pop_back());
            }
            else
            {
                std::mutex m;
                std::unique_lock ul{ m };

                cv.wait(ul);
            }
        }
    }


    CircularStealingQueue<iWork*>& _BackgrounWorkQueue::GetQueue()
    {
        return queue;
    }


    bool _BackgrounWorkQueue::Running()
    {
        return running;
    }


    /************************************************************************************************/



	void WorkBarrier::AddWork(iWork& work)
	{
		++tasksInProgress;

		work.Subscribe(
			[&]
			{
				const auto prev = tasksInProgress.fetch_sub(1); FK_ASSERT(prev > 0);

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
        if (!tasksInProgress)
            return;

		do
		{
			if (!inProgress)
				return;

			for(auto work = threads.FindWork(); work; work = threads.FindWork())
                RunTask(*work, GetThreadLocalAllocator());

		} while (true);

        Reset();
	}


    /************************************************************************************************/


    void WorkBarrier::JoinLocal()
    {
        if (!tasksInProgress)
            return;

		do
		{
			if (!inProgress)
				return;

			while(true)
            {
                auto work = localWorkQueue->pop_back().value_or(nullptr);
                if (work)
                    RunTask(*work, GetThreadLocalAllocator());
                else
                    break;
			}
		} while (true);

        Reset();
    }


    /************************************************************************************************/


    void WorkBarrier::Reset()
    {
        tasksInProgress = 0;
        inProgress      = true;
    }


    /************************************************************************************************/


    ThreadManager::ThreadManager(const uint32_t threadCount, iAllocator* IN_allocator) :
		threads				{ },
		allocator			{ IN_allocator		},
		workingThreadCount	{ 0					},
		workerCount			{ threadCount       },
		workQueues          { IN_allocator      },
        mainThreadQueue     { IN_allocator      },
        backgroundQueue     { IN_allocator      }
	{
        FK_ASSERT(WorkerThread::Manager == nullptr);

		WorkerThread::Manager = this;
        _SetThreadLocalQueue(mainThreadQueue);
		workQueues.push_back(&mainThreadQueue);

        buffer = std::make_unique<std::array<byte, MEGABYTE * 16>>();
        localAllocator.Init(buffer->data(), MEGABYTE * 16);
        _localAllocator = localAllocator;

		for (size_t I = 0; I < workerCount; ++I)
        {
			auto& thread = allocator->allocate<WorkerThread>(allocator);
			threads.push_back(thread);
			workQueues.push_back(&thread.GetQueue());
		}

        for (auto& thread : threads)
            thread.Start();
	}


    /************************************************************************************************/


	void ThreadManager::Release()
	{
		WaitForWorkersToComplete(localAllocator);

		SendShutdown();

		WaitForShutdown();

		while (!threads.empty())
		{
			WorkerThread* worker = nullptr;
			if (threads.try_pop_back(worker))
				allocator->release(worker);
		}

        backgroundQueue.Shutdown();
	}


    /************************************************************************************************/


	void ThreadManager::SendShutdown()
	{
		for (auto& I : threads)
			I.Shutdown();
	}


    /************************************************************************************************/


	void ThreadManager::AddWork(iWork* newWork, iAllocator* Allocator) noexcept
	{
        PushToLocalQueue(*newWork);
		workerWait.notify_all();
	}


    /************************************************************************************************/


    void ThreadManager::AddBackgroundWork(iWork& newWork) noexcept
    {
        backgroundQueue.PushWork(newWork);
    }


    /************************************************************************************************/


	void ThreadManager::WaitForWorkersToComplete(iAllocator& tempAllocator)
	{
        auto& localWorkQueue = _GetThreadLocalQueue();
		while (!localWorkQueue.empty())
		{
			if (auto workItem = FindWork(true); workItem)
                RunTask(*workItem, localAllocator);
		}
		while (workingThreadCount > 0);
	}


    /************************************************************************************************/


	void ThreadManager::WaitForShutdown() noexcept // TODO: this does not work!
	{
		bool threadRunnings = false;

		do
		{
			threadRunnings = false;
			for (auto& I : threads) {
				threadRunnings |= I.IsRunning();
				I.Shutdown();
			}

			workerWait.notify_all();
		} while (threadRunnings);
	}


    /************************************************************************************************/


	void ThreadManager::IncrementActiveWorkerCount() noexcept
	{
		workingThreadCount++;
		CV.notify_all();
	}


    /************************************************************************************************/


	void ThreadManager::DecrementActiveWorkerCount() noexcept
	{
		workingThreadCount--;
		CV.notify_all();
	}


    /************************************************************************************************/


	void ThreadManager::WaitForWork() noexcept
	{
        std::mutex m;
        std::unique_lock l{ m };

        workerWait.wait_for(l, 1ms);
	}


    /************************************************************************************************/


	iWork* ThreadManager::FindWork(bool stealBackground)
	{
        auto& localWorkQueue = _GetThreadLocalQueue();

        if (auto res = localWorkQueue.pop_back(); res)
            return res.value();

        const size_t startingPoint = randomDevice();
        for (size_t I = 0; I < workQueues.size(); ++I)
        {
            const size_t idx = (I + startingPoint) % workQueues.size();
            if (auto res = workQueues[idx]->Steal(); res)
                return res.value();
        }

        return  stealBackground ?
            backgroundQueue.GetQueue().Steal().value_or(nullptr) :
            nullptr;
	}


    /************************************************************************************************/


	uint32_t ThreadManager::GetThreadCount() const noexcept
	{
		return workerCount;
	}


    /************************************************************************************************/


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
