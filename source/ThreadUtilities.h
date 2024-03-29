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

#ifdef _WIN32
#pragma once
#endif

#ifndef CTHREAD_H
#define CTHREAD_H

	// includes
#include "buildsettings.h"
#include "containers.h"
#include "memoryutilities.h"
#include "ProfilingUtilities.h"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <stdint.h>
#include <thread>
#include <utility>

#define MAXTHREADCOUNT 8


namespace FlexKit
{
	using std::atomic;
	using std::atomic_bool;
	using std::chrono::nanoseconds;
	using std::chrono::microseconds;
	using std::chrono::milliseconds;

	using namespace std::chrono_literals;

	class ThreadManager;


	/************************************************************************************************/


	typedef TypeErasedCallable<void ()> OnCompletionEvent;


	/************************************************************************************************/


	class iWork
	{
	public:
		iWork(iAllocator* Memory = nullptr) : _debugID{ "UNIDENTIFIED!" } { }

		virtual ~iWork() {}

		iWork& operator = (const iWork&)    = delete;
		iWork& operator = (iWork&&)         = delete;

		iWork(const iWork&) = delete;
		iWork(iWork&&)      = delete;

		void DoWork(iAllocator& threadLocalAllocator)
		{
			Run(threadLocalAllocator);
			completed = true;

			ReleaseAndNotifyWatchers();
		}

		template<typename TY_Callable>
		void Subscribe(TY_Callable&& subscriber)
		{
			if (completed)
				subscriber();
			else
				subscribers.emplace_back(subscriber);
		}


		operator iWork* () { return this; }

		const char*         _debugID;

	protected:
		virtual void Run(iAllocator& threadLocalAllocatoDDr) { FK_ASSERT(0); }
		virtual void Release() {}

		void ReleaseAndNotifyWatchers()
		{
			static_vector<OnCompletionEvent, 8>	tempSubs{ std::move(subscribers) };

			Release();

			for (auto& sub : tempSubs)
				sub();
		}

	private:
		static_vector<OnCompletionEvent, 8>	subscribers;
		std::atomic_bool					completed = false;
	};


	struct DependencyBarrier
	{
		DependencyBarrier() = default;

		DependencyBarrier& operator =	(const DependencyBarrier& rhs) = delete;
		DependencyBarrier				(const DependencyBarrier& rhs) = delete;

		DependencyBarrier& operator =	(DependencyBarrier&& rhs) = delete;
		DependencyBarrier				(DependencyBarrier&& rhs) = delete;

		void AddInput(iWork* work);
		void operator() ();
		void Increment() { dependencyCounter.fetch_add(1); }

		void Start(iWork& IN_work);

		std::atomic_uint64_t dependencyCounter = 0;
		iWork* work	= nullptr;
	};


	/************************************************************************************************/


	template<typename TY, size_t Alignment = sizeof(TY)>
	struct alignas(Alignment) alignedAtomicWrapper
	{
		std::atomic<TY> element;

		operator TY ()
		{
			return element.load();
		}


		alignedAtomicWrapper& operator = (const TY& rhs)
		{
			element.store(rhs);
			return *this;
		}


		alignedAtomicWrapper& operator = (const alignedAtomicWrapper& rhs) = default;


		template<typename = std::enable_if_t<std::is_pointer_v<TY>>>
		TY operator -> ()
		{
			return element.load();
		}


		template<typename = std::enable_if_t<std::is_pointer_v<TY>>>
		TY operator * ()
		{
			return element.load();
		}
	};


	/************************************************************************************************/


	template<typename TY_E>
	class alignas(64) CircularStealingQueue
	{
	public:
		using ElementType = alignedAtomicWrapper<TY_E>;

		CircularStealingQueue(iAllocator* IN_allocator, const size_t initialReservation = 256) noexcept :
			queueArraySize	{ initialReservation	},
			queue			{ nullptr				},
			frontCounter	{ 0						},
			backCounter	    { 0						},
			allocator       { IN_allocator          }
		{
			if(initialReservation)
				queue = (ElementType*)allocator->_aligned_malloc(sizeof(ElementType) * initialReservation);
		}

		~CircularStealingQueue()
		{
			Release();
		}


		CircularStealingQueue               (const CircularStealingQueue&) = delete;
		CircularStealingQueue& 	operator =	(const CircularStealingQueue&) = delete;


		[[nodiscard]] std::optional<TY_E> pop_back() noexcept // FILO
		{
			const auto back  = backCounter.fetch_sub(1, std::memory_order_acq_rel) - 1;
			const auto front = frontCounter.load();

			if (front <= back)
			{
				auto job = *queue[back % queueArraySize];

				if (front != back)
					return job;
				else
				{   // last job in queue, potential race with a steal
					auto expected   = front;
					const auto res  = frontCounter.compare_exchange_strong(expected, expected + 1);
					backCounter.store(front + 1);

					if (res)
						return { job };
					else
						return {};
				}
			}
			else
			{
				backCounter.store(front);
				return {};
			}
		}


		bool Steal(TY_E& out) noexcept // FIFO
		{
			auto front  = frontCounter.load(std::memory_order_acquire);
			auto back   = backCounter.load(std::memory_order_acquire);

			if (front < back)
			{
				auto job = *queue[front % queueArraySize];

				if (frontCounter.compare_exchange_strong(front, front + 1))
				{
					out = job;
					return true;
				}
				else
					return false;
			}
			else
				return false;
		}

	
		void push_back(TY_E element) noexcept
		{
			if (queueArraySize < size() + 1)
				_Expand();

			queue[backCounter % queueArraySize] = element;

			std::atomic_thread_fence(std::memory_order_release);

			backCounter.fetch_add(1, std::memory_order_acq_rel); // publish push
		}


		bool empty() const noexcept
		{
			return size() == 0;
		}


		size_t size() const noexcept
		{
			return (size_t)(backCounter - frontCounter);
		}


		void Release()
		{
			if (queue)
				allocator->_aligned_free(queue);

			queue = nullptr;
		}

	private:
		void _Expand() noexcept
		{
			const auto	arraySize	= queueArraySize * 2;
			auto		newArray	= (ElementType*)allocator->_aligned_malloc(sizeof(ElementType) * arraySize);

			const int64_t   begin	= frontCounter;
			const int64_t   end	    = backCounter;
			const auto      count	= size_t(end - begin);

			FK_ASSERT(queueArraySize >= count);

			for (uint64_t idx = 0; idx < count; ++idx)
			{
				const auto oldIdx = (begin + idx) % queueArraySize;
				const auto newIdx = (begin + idx) % arraySize;
				newArray[newIdx]  = *queue[oldIdx];
			}

			auto tmp_ptr    = queue;
			queue			= newArray;
			queueArraySize	= arraySize;

			allocator->_aligned_free(tmp_ptr);
		}


		void _Contract() noexcept
		{
			const auto newSize	= (size_t)std::pow(2, log2(size()) + 1);
			const auto newArray	= new TY_E[newSize];
		}

		size_t			                    queueArraySize  = 0;
		ElementType*                        queue           = nullptr;
		iAllocator*                         allocator       = nullptr;

		alignas(64) std::atomic_int64_t     backCounter    = 0;
		alignas(64) std::atomic_int64_t     frontCounter   = 0;
	};


	class _WorkerThread;



	class _BackgrounWorkQueue
	{
	public:
		_BackgrounWorkQueue(iAllocator* allocator);

		void Shutdown();

		void PushWork(iWork& work);

		void                            Run();
		CircularStealingQueue<iWork*>&  GetQueue();
		bool                            Running();

	private:

		Vector<iWork*>                  workList;

		std::mutex                      lock;
		std::atomic_bool                running = false;
		std::condition_variable         cv;
		std::thread                     backgroundThread;

		CircularStealingQueue<iWork*>   queue;
	};


	class _WorkerThread
	{
	public:
		_WorkerThread(iAllocator* ThreadMemory = SystemAllocator) :
			Running		{ false				},
			Quit		{ false				},
			hasJob		{ false				},
			workQueue   { ThreadMemory      },
			Allocator	{ ThreadMemory		} {}

		~_WorkerThread()
		{
			workQueue.Release();

			if (Thread.joinable())
				Thread.join();
		}

		void Shutdown()	noexcept;
		void Wake()		noexcept;

		void Start() noexcept;

		bool IsRunning()	noexcept;
		bool HasJob()		noexcept;
		bool hasWork()		noexcept;

		bool	AddItem(iWork* Work)	noexcept;
		iWork*	Steal()					noexcept;

		auto&   GetQueue() { return workQueue; }

		static ThreadManager*	Manager;

	private:
		void _Run();

		std::condition_variable	CV;
		std::atomic_bool		Running;
		std::atomic_bool		hasJob;
		std::atomic_bool		Quit;
		std::atomic_int			workCount = 0;

		iAllocator*				Allocator;

		std::mutex					    exclusive;
		CircularStealingQueue<iWork*>   workQueue;
		std::thread					    Thread;
	};


	/************************************************************************************************/


	FLEXKITAPI extern void PushToLocalQueue(iWork& work);

	FLEXKITAPI CircularStealingQueue<iWork*>&    _GetThreadLocalQueue();
	FLEXKITAPI void                              _SetThreadLocalQueue(CircularStealingQueue<iWork*>& localQueue);

	FLEXKITAPI inline _WorkerThread&                    GetLocalThread();

	inline thread_local static_vector<std::unique_ptr<StackAllocator>> localAllocators;

	FLEXKITAPI inline void RunTask(iWork& work)
	{
		std::unique_ptr<StackAllocator> allocator;

		if (localAllocators.empty())
		{
			allocator = std::make_unique<StackAllocator>();
			allocator->Init((byte*)malloc(16 * MEGABYTE), 16 * MEGABYTE);
		}
		else
		{
			allocator = std::move(localAllocators.back());
			localAllocators.pop_back();
		}

		work.DoWork(*allocator);

		allocator->clear();

		localAllocators.emplace_back(std::move(allocator));
	}

	using WorkerList	= IntrusiveLinkedList<_WorkerThread>;
	using WorkerThread	= WorkerList::TY_Element;


	/************************************************************************************************/


	template<typename TY_FN>
	class LambdaWork : public iWork
	{
	public:
		LambdaWork(TY_FN FNIN, iAllocator* IN_allocator = FlexKit::SystemAllocator) noexcept :
			iWork		{ IN_allocator	},
			allocator	{ IN_allocator	},
			Callback	{ FNIN			} {}

		void Release() noexcept
		{
			auto temp = allocator;
			this->~LambdaWork();
			temp->free(this);
		}

		void Run(iAllocator& allocator) override
		{
			ProfileFunction();

			Callback(allocator);
		}

		TY_FN		Callback;
		iAllocator* allocator;
	};


	template<typename TY_FN>
	[[nodiscard]] iWork& CreateWorkItem(
		TY_FN		FNIN, 
		iAllocator* objectAllocator,
		iAllocator* workitemAllocator)
	{
		return objectAllocator->allocate<LambdaWork<TY_FN>>(FNIN, workitemAllocator);
	}

	template<typename TY_FN>
	[[nodiscard]] iWork& CreateWorkItem(
		TY_FN		FNIN,
		iAllocator* allocator = SystemAllocator)
	{
		return CreateWorkItem(FNIN, allocator, allocator);
	}


	/************************************************************************************************/


	class ThreadManager
	{
	public:
		ThreadManager(const size_t ThreadCount = 2, iAllocator* IN_allocator = SystemAllocator);
		~ThreadManager() { Release(); }

		void Release();


		void SendShutdown();

		void AddWork(iWork* newWork) noexcept;
		void AddBackgroundWork(iWork& newWork) noexcept;

		void WaitForWorkersToComplete(iAllocator& tempAllocator);
		void WaitForShutdown() noexcept;


		void IncrementActiveWorkerCount() noexcept;
		void DecrementActiveWorkerCount() noexcept;

		void WaitForWork() noexcept;

		iWork* FindWork(bool stealBackground = false);

		size_t GetThreadCount() const noexcept;

		auto GetThreadsBegin()	const noexcept { return threads.begin(); }
		auto GetThreadsEnd()	const noexcept { return threads.end(); }

	private:
		WorkerList				threads;
		_BackgrounWorkQueue		backgroundQueue;

		std::condition_variable		CV;
		std::condition_variable		workerWait;
		std::atomic_int				workingThreadCount;
		std::default_random_engine	randomDevice;

		const size_t				workerCount;
		iAllocator*					allocator;
		std::mutex					exclusive;

		CircularStealingQueue<iWork*>	mainThreadQueue;

		Vector<CircularStealingQueue<iWork*>*>			workQueues;
		StackAllocator									localAllocator;
		std::unique_ptr<std::array<byte, MEGABYTE * 16>> buffer;
	};


	/************************************************************************************************/


	class WorkBarrier
	{
	public:
		WorkBarrier(ThreadManager& IN_threads, iAllocator* allocator = FlexKit::SystemAllocator);
		~WorkBarrier();

		WorkBarrier(const WorkBarrier&)					= delete;
		WorkBarrier& operator = (const WorkBarrier&)	= delete;

		size_t	GetPendingWorkCount() const	{ return tasksInProgress; }
		void	AddWork					(iWork& Work);
		void	AddWork					(WorkBarrier& barrier);
		void	AddOnCompletionEvent	(OnCompletionEvent Callback);
		void	Wait					();

		void	Join		();
		void	JoinLocal	();

		void 	Reset();

	private:

		void OnEnd();

		std::atomic_int		tasksInProgress = 0;
		std::atomic_int		tasksScheduled	= 0;
		std::atomic_bool	joined			= false;

		ThreadManager&				threads;
		Vector<OnCompletionEvent>	PostEvents;
	};


	/************************************************************************************************/



	// Thread safe lazy object constructor, returns callable that returns the same object everytime, for every calling thread.  Will block during construction.
	template<typename TY, typename FN_Constructor> 
	[[nodiscard]] auto MakeLazyObject(iAllocator* allocator, FN_Constructor FN_Construct)
	{
		struct _State
		{
			TY			constructable;
			atomic_bool ready = false;
			std::mutex  m;

		};

		return
			[FN_Construct = FN_Construct, _state = MakeSharedRef<_State>(allocator)](auto&& ... args) mutable -> TY&
			{
				auto& state = _state.Get();

				if (!state.ready.load(std::memory_order_relaxed))
				{
					if (state.m.try_lock())
					{
						state.constructable	= FN_Construct(std::forward<decltype(args)>(args)...);
						state.ready.store(true, std::memory_order_release);
						state.m.unlock();
					}
					else
						while(!state.ready);
				}

				return state.constructable;
			};
	}


	/************************************************************************************************/


	template<typename TY_OP>
	class SynchronizedOperation
	{
	public:
		SynchronizedOperation(TY_OP IN_operation) : operation{ IN_operation} {}

		// No Copy
		SynchronizedOperation(const SynchronizedOperation& rhs) = delete;
		SynchronizedOperation& operator = (const SynchronizedOperation& rhs) = delete;


		SynchronizedOperation(SynchronizedOperation&& rhs) :
			operation		{ std::move(rhs.operation) },
			criticalSection	{ std::move(rhs.criticalSection) } {}


		SynchronizedOperation& operator = (SynchronizedOperation&& rhs)
		{
			std::scoped_lock<std::mutex> lock{ rhs.criticalSection };
			operation = std::move(rhs.operation);
		}

		template<typename ... TY_ARGS>
		decltype(auto) operator ()(TY_ARGS&& ... args)
		{
			const auto lock = std::scoped_lock( criticalSection );
			return operation(std::forward<TY_ARGS>(args)...);
		}


	private:
		TY_OP		operation;
		std::mutex	criticalSection;
	};

	template<typename TY_OP>
	decltype(auto) MakeSynchonized(TY_OP operation, iAllocator* allocator)
	{
		return MakeSharedRef<SynchronizedOperation<TY_OP>>(allocator, operation);
	}


	/************************************************************************************************/


	template<typename ITERATOR_TY, typename FN_TY>
	void Parallel_For(
		ThreadManager&	threads,
		iAllocator&		allocator,
		ITERATOR_TY		begin,
		ITERATOR_TY		end,
		const size_t	blockSize,
		FN_TY			task)
	{
		ProfileFunction();

		const size_t taskCount = std::distance(begin, end);

		const size_t temp			= taskCount / blockSize;
		const size_t temp2			= taskCount % blockSize != 0;
		const size_t threadCount	= Max(temp + temp2, 1);



		struct Task : public iWork
		{
			using ITERATOR = ITERATOR_TY;

			ITERATOR begin   = 0;
			ITERATOR end     = 0;

			FN_TY* task_FN;

			Task() : iWork{ nullptr } {}

			Task(ITERATOR IN_begin, ITERATOR IN_end, FN_TY* IN_FN) :
					iWork		{ nullptr	},
					begin		{ IN_begin	},
					end			{ IN_end	},
					task_FN		{ IN_FN		}
			{
				int x = 0;
			}

			~Task(){}

			void Run(iAllocator& threadLocalAllocator) final
			{
				ProfileFunction();

				for(auto I = begin; I < end; I++)
					(*task_FN)(*I, threadLocalAllocator);
			}

			void Release() final
			{
			}
		};

		if (threadCount > 1)
		{
			WorkBarrier barrier{threads, &allocator};
			Vector<Task*> tasks{&allocator, threadCount};

			for (size_t I = 0; I < threadCount; ++I)
			{
				auto workItem =
					&allocator.allocate<Task>(
						begin + I * blockSize,
						begin + Min((I + 1) * blockSize, taskCount),
						&task);

				tasks.emplace_back(workItem);
				barrier.AddWork(*workItem);
			}

			for (auto& task : tasks)
				threads.AddWork(task);

			barrier.Join();
		}
		else
			std::for_each(begin, end,
				[&](auto& element)
				{
					task(element, allocator);
				});
	}


		template<typename ITERATOR_TY, typename FN_TY>
	void Parallel_For2(
		ThreadManager&	threads,
		iAllocator&		allocator,
		ITERATOR_TY		begin,
		ITERATOR_TY		end,
		const size_t	blockSize,
		FN_TY			task)
	{
		ProfileFunction();

		const size_t taskCount = std::distance(begin, end);

		const size_t temp			= taskCount / blockSize;
		const size_t temp2			= taskCount % blockSize != 0;
		const size_t threadCount	= Max(temp + temp2, 1);

		struct Task : public iWork
		{
			using ITERATOR = ITERATOR_TY;

			ITERATOR    begin   = 0;
			ITERATOR    end     = 0;
			size_t      dispatchID;

			FN_TY* task_FN;

			Task() : iWork{ nullptr } {}

			Task(ITERATOR IN_begin, ITERATOR IN_end, size_t IN_dispatchID, FN_TY* IN_FN) :
					iWork		{ nullptr		},
					begin		{ IN_begin		},
					end			{ IN_end		},
					task_FN		{ IN_FN			},
					dispatchID	{ IN_dispatchID	} {}

			~Task(){}

			void Run(iAllocator& threadLocalAllocator) final
			{
				ProfileFunction();

				(*task_FN)(begin, end, dispatchID, threadLocalAllocator);
			}

			void Release() final
			{
			}
		};

		if (threadCount > 1)
		{
			WorkBarrier barrier{threads, &allocator};
			Vector<Task*> tasks{&allocator, threadCount};

			for (size_t I = 0; I < threadCount; ++I)
			{
				auto workItem =
					&allocator.allocate<Task>(
						begin + I * blockSize,
						begin + Min((I + 1) * blockSize, taskCount),
						I,
						&task);

				tasks.emplace_back(workItem);
				barrier.AddWork(*workItem);
			}

			for (auto& task : tasks)
				threads.AddWork(task);

			barrier.Join();
		}
		else
			task(begin, end, 0, allocator);
	}


	/************************************************************************************************/
}	// namespace FlexKit

#endif
