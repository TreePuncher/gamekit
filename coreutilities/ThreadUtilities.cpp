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


    FLEXKITAPI void PushToLocalQueue(iWork& work)
    {
        localWorkQueue->push_back(&work);
    }


    FLEXKITAPI CircularStealingQueue<iWork*>& _GetThreadLocalQueue()
    {
        return *localWorkQueue;
    }


    FLEXKITAPI void _SetThreadLocalQueue(CircularStealingQueue<iWork*>& localQueue)
    {
        localWorkQueue = &localQueue;
    }


    FLEXKITAPI _WorkerThread& GetLocalThread()
    {
        return *_localThread;
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
            for(size_t I = 0; I < 10; I++)
			{
				EXITSCOPE({
					Manager->DecrementActiveWorkerCount();
					});

				    Manager->IncrementActiveWorkerCount();

                    if (auto workItem = Manager->FindWork(true); workItem)
                    {
                        I = 0;
                        hasJob.store(true, std::memory_order_release);

                        RunTask(*workItem);

                        hasJob.store(false, std::memory_order_release);
                    }
                    else if(I > 1) // Reduce thread contention a little
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
                    localWorkQueue  = &workQueue;
                    _localThread    = this;

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

        iWork* work = nullptr;
		localWorkQueue->Steal(work);

        return work;
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

        cv.notify_one();
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
        inProgress = true;

		work.Subscribe(
			[&]
			{
				const auto prev = tasksInProgress.fetch_sub(1, std::memory_order::memory_order_seq_cst); FK_ASSERT(prev > 0);

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
        ProfileFunction();

		do
		{
			if (!inProgress.load(std::memory_order::acquire))
				return;
		} while (true);
	}


	/************************************************************************************************/


	void WorkBarrier::Join()
	{
        ProfileFunction();

        if (!inProgress.load(std::memory_order::acquire))
            return;

		do
		{
			if (!inProgress.load(std::memory_order::acquire))
				return;

			for(auto work = threads.FindWork(); work; work = threads.FindWork())
                RunTask(*work);

		} while (true);

        Reset();
	}


    /************************************************************************************************/


    void WorkBarrier::JoinLocal()
    {
        ProfileFunction();

        if (!inProgress.load(std::memory_order::acquire))
            return;

		while(inProgress.load(std::memory_order::acquire))
        {
            ProfileFunction(Joined);

            auto work = localWorkQueue->pop_back().value_or(nullptr);
            if (work)
                RunTask(*work);
            else break;
		}

        Wait();
        Reset();
    }


    /************************************************************************************************/


    void WorkBarrier::Reset()
    {
        tasksInProgress = 0;
        inProgress      = false;
    }


    /************************************************************************************************/


    ThreadManager::ThreadManager(const size_t threadCount, iAllocator* IN_allocator) :
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


	void ThreadManager::AddWork(iWork* newWork) noexcept
	{
        PushToLocalQueue(*newWork);

		workerWait.notify_one();
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
                RunTask(*workItem);
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

        workerWait.wait_for(l, 5ms);
	}


    /************************************************************************************************/


	iWork* ThreadManager::FindWork(bool stealBackground)
	{
        auto& localWorkQueue = _GetThreadLocalQueue();

        if (auto res = localWorkQueue.pop_back(); res)
            return res.value();

        thread_local static const size_t startingPoint = randomDevice();

        for (size_t I = 0; I < workQueues.size(); ++I)
        {
            const size_t idx = (I + startingPoint) % workQueues.size();

            iWork* work = nullptr;

            if (auto res = workQueues[idx]->Steal(work); res)
                return work;
        }

        if (stealBackground)
        {
            iWork* work = nullptr;
            backgroundQueue.GetQueue().Steal(work);
            return work;
        }
        else
            return nullptr;
	}


    /************************************************************************************************/


    size_t ThreadManager::GetThreadCount() const noexcept
	{
		return workerCount;
	}


    /************************************************************************************************/


	ThreadManager* WorkerThread::Manager = nullptr;


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
