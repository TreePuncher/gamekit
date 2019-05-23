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
#include <random>

#define MAXTHREADCOUNT 3

#pragma warning ( disable : 4251 )



namespace FlexKit
{
	using std::atomic;
	using std::atomic_bool;

	class ThreadManager;

	typedef std::function<void()> OnCompletionEvent;


	/************************************************************************************************/


	class iWork
	{
	public:
		iWork & operator = (iWork& rhs) = delete;

		iWork(iAllocator* Memory) :
			_debugID	{ "UNIDENTIFIED!" }
			//: subscribers{Memory}
		{
			//subscribers.reserve(8);
		}


		virtual ~iWork()					
		{ 
			//subscribers.Release();
		}


		virtual void Run()		{ FK_ASSERT(0); }
		virtual void Release()	= 0;


		void NotifyWatchers()
		{
			for (auto& watcher : subscribers)
			{
				if (!watcher)
					FK_ASSERT(0);
				watcher();
			}

			subscribers.clear();
			notifiedSubs = true;
		}


		void Subscribe(OnCompletionEvent subscriber) 
		{ 
			if (!subscriber)
				FK_ASSERT(0);
			subscribers.push_back(subscriber);
		}


		operator iWork* () { return this; }

		bool						scheduled = false;

		void SetDebugID(const char* IN_debugID)
		{
			_debugID = IN_debugID;
		}

	protected:
		const char*							_debugID;

	private:
		bool								notifiedSubs = false;
		static_vector<OnCompletionEvent, 8>	subscribers;
	};


	/************************************************************************************************/


	class _WorkerThread
	{
	public:
		_WorkerThread(iAllocator* ThreadMemory = SystemAllocator) :
			Thread		{ [this] { _Run(); }	},
			Running		{ false				},
			Quit		{ false				},
			hasJob		{ false				},
			Allocator	{ ThreadMemory		} {}


		void Shutdown()	noexcept;
		void Wake()		noexcept;

		bool IsRunning()	noexcept;
		bool HasJob()		noexcept;
		bool hasWork()		noexcept;

		bool	AddItem(iWork* Work)	noexcept;
		iWork*	Steal()					noexcept;

		static ThreadManager*	Manager;

	private:
		void _Run();

		std::condition_variable	CV;
		std::atomic_bool		Running;
		std::atomic_bool		hasJob;
		std::atomic_bool		Quit;
		std::atomic_int			workCount = 0;

		iAllocator*				Allocator;

		std::mutex					exclusive;
		CircularBuffer<iWork*, 32>	workList;

		std::thread					Thread;

		size_t tasksCompleted = 0;
	};


	using WorkerList	= IntrusiveLinkedList<_WorkerThread>;
	using WorkerThread	= WorkerList::TY_Element;


	/************************************************************************************************/


	template<typename TY_FN>
	class LambdaWork : public iWork
	{
	public:
		LambdaWork(TY_FN& FNIN, iAllocator* Memory = FlexKit::SystemAllocator) noexcept :
			iWork		{ Memory },
			Callback	{ FNIN } {}

		void Release() noexcept
		{

		}

		void Run()
		{
			Callback();
		}

		TY_FN Callback;
	};

	template<typename TY_FN>
	LambdaWork<TY_FN> CreateLambdaWork(TY_FN FNIN, iAllocator* Memory = FlexKit::SystemAllocator) noexcept
	{
		return LambdaWork<TY_FN>(FNIN, Memory);
	}

	template<typename TY_FN>
	iWork& CreateLambdaWork_New(
		TY_FN FNIN, 
		iAllocator* Memory_1,
		iAllocator* Memory_2)
	{
		return Memory_1->allocate<LambdaWork<TY_FN>>(FNIN, Memory_2);
	}

	template<typename TY_FN>
	iWork& CreateLambdaWork_New(
		TY_FN&		FNIN,
		iAllocator* allocator = SystemAllocator)
	{
		return CreateLambdaWork_New(FNIN, allocator, allocator);
	}

	/************************************************************************************************/


	class ThreadManager
	{
	public:
		ThreadManager(size_t ThreadCount = 6, iAllocator* IN_allocator = SystemAllocator) :
			threads				{ },
			allocator			{ IN_allocator		},
			workingThreadCount	{ 0					},
			workerCount			{ ThreadCount		}
		{
			WorkerThread::Manager = this;

			for (size_t I = 0; I < ThreadCount; ++I)
				threads.push_back(allocator->allocate<WorkerThread>(allocator));
		}


		void Release()
		{
			WaitForWorkersToComplete();

			SendShutdown();

			WaitForShutdown();

			while (!threads.empty())
			{
				WorkerThread* worker = nullptr;
				if (threads.try_pop_back(worker))
					allocator->free(worker);
			}

		}


		void SendShutdown()
		{
			for (auto& I : threads)
				I.Shutdown();
		}


		void AddWork(iWork* newWork, iAllocator* Allocator = SystemAllocator) noexcept
		{
			FK_ASSERT(newWork->scheduled == false);

			if (!threads.empty())
			{
				size_t	startPoint = randomDevice();

				auto _AddWork = [](iWork* newWork, auto& thread)
				{

					if (thread->AddItem(newWork))// committed to adding work here
					{
						newWork->scheduled = true;
						return true;
					}
					return false;
				};

				for(size_t itr = 0; itr < workerCount; ++itr)
				{
					size_t	threadidx	= (startPoint + itr) % workerCount;
					auto& thread		= threads.begin() + threadidx;

					if (!thread->IsRunning())
					{
						if(_AddWork(newWork, thread))
							return;
					}
				}

				for (size_t itr = 0; itr < workerCount; ++itr)
				{
					size_t	threadidx = (startPoint + itr) % workerCount;
					auto& thread = threads.begin() + threadidx;

					if (!thread->HasJob())
					{
						if(_AddWork(newWork, thread))
							return;
					}
				}

				for(size_t I = 0; true; I = (I + 1) % workerCount)
				{
					if (_AddWork(newWork, threads.begin() + ((I + startPoint - 1) % workerCount )))
						return;
				}
			}
			else
			{
				std::scoped_lock lock{ exclusive };
				workList.push_back(newWork);
			}
		}


		void WaitForWorkersToComplete()
		{
			if (!threads.empty())
			{
				std::mutex						M;
				std::unique_lock<std::mutex>	lock(M);

				CV.wait(lock, [this] { return workingThreadCount <= 0; });
			}
			else
			{
				while(!workList.empty())
				{
					std::scoped_lock lock{ exclusive };

					if (auto workItem = workList.pop_front(); workItem != nullptr)
					{
						workItem->Run();
						workItem->NotifyWatchers();
						workItem->Release();
					}
				}
			}
		}

		/*
		void RotateThreads() noexcept
		{
			WorkerThread* Ptr = nullptr;

			auto success = threads.try_pop_front(Ptr);

			if (success) {
				threads.push_back(Ptr);
				Ptr->Wake();
			}
		}
		*/


		void WaitForShutdown() noexcept // TODO: this does not work!
		{
			bool threadRunnings = false;
			do
			{
				threadRunnings = false;
				for (auto& I : threads) {
					threadRunnings |= I.IsRunning();
					I.Shutdown();
				}
			} while (threadRunnings);
		}


		void IncrementActiveWorkerCount() noexcept
		{
			workingThreadCount++;
			CV.notify_all();
		}


		void DecrementActiveWorkerCount() noexcept
		{
			workingThreadCount--;
			CV.notify_all();
		}


		iWork* StealSomeWork()
		{
			if (threads.empty()) 
			{
				if (!workList.size())
				{
					std::scoped_lock lock{ exclusive };
					return workList.pop_front();
				}
				return nullptr;
			}

			for(auto& thread : threads)
			{
				if (auto res = thread.Steal())
					return res;
			}

			return nullptr;
		}


		size_t GetThreadCount() const
		{
			return workerCount;
		}


		auto GetThreadsBegin()	{ return threads.begin(); }
		auto GetThreadsEnd()	{ return threads.end(); }

	private:
		WorkerList	threads;

		std::condition_variable		CV;
		std::atomic_int				workingThreadCount;
		std::default_random_engine	randomDevice;

		const size_t				workerCount;
		iAllocator*					allocator;
		std::mutex					exclusive;
		CircularBuffer<iWork*, 128>	workList; // for the case of a single thread, work is pushed here and process on Wait for workers to complete
	};


	/************************************************************************************************/


	class WorkBarrier
	{
	public:
		WorkBarrier(
			ThreadManager&	IN_threads,
			iAllocator*		Memory = FlexKit::SystemAllocator) :
				PostEvents	{ Memory		},
				threads		{ IN_threads	} {}

		~WorkBarrier() {}

		WorkBarrier(const WorkBarrier&)					= delete;
		WorkBarrier& operator = (const WorkBarrier&)	= delete;

		int  GetDependentCount		() { return TasksInProgress; }
		void AddDependentWork		(iWork* Work);
		void AddOnCompletionEvent	(OnCompletionEvent Callback);
		void Wait					();
		void Join					();

	private:
		ThreadManager&				threads;
		Vector<OnCompletionEvent>	PostEvents;

		std::condition_variable		CV;
		std::atomic_int				TasksInProgress	= 0;
		std::mutex					m;
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
				[&]() {
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


	// Thread safe lazy object constructor, returns callable that returns the same object everytime, for every calling thread.  Will block during construction.
	template<typename TY, typename FN_Constructor>
	auto MakeLazyObject(iAllocator* allocator, FN_Constructor FN_Construct)
	{
		struct _State
		{
			atomic_bool inProgress	= false;
			atomic_bool ready		= false;

			std::condition_variable		cv;
			TY							constructable;
		};

		auto lazyConstructor = [FN_Construct, _state = MakeSharedRef<_State>(allocator)](auto&& ... args) mutable
		{
			if (!_state.Get().ready)
			{
				if (!_state.Get().inProgress)
				{
					_state.Get().inProgress		= true;

					_state.Get().constructable	= FN_Construct(std::forward<decltype(args)>(args)...);

					_state.Get().inProgress		= false;
					_state.Get().ready			= true;
					_state.Get().cv.notify_all();
				}
				else
				{
					std::mutex m;
					std::unique_lock ul{ m };

					_state.Get().cv.wait(ul, [&]() -> bool { return _state.Get().ready; });
				}
			}

			return _state.Get().constructable;
		};

		return lazyConstructor;
	}


	/************************************************************************************************/
}	// namespace FlexKit

#endif
