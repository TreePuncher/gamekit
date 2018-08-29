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
	
			//_WriteBarrier();
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

		bool AddItem(iWork* Item);
		void Shutdown();
		void Run();
		bool IsRunning();

		static ThreadManager*	Manager;

	private:
		std::condition_variable	CV;
		std::atomic_bool		Running;
		std::atomic_bool		Quit;
		std::deque<iWork*>		WorkList;
		std::mutex				CBLock;
		std::mutex				AddLock;
		std::thread				Thread;

		iAllocator*				Allocator;
	};


	/************************************************************************************************/


	class WorkerList
	{
	public:
		struct Element
		{
			Element*		_Prev = nullptr;
			Element*		_Next = nullptr;
			WorkerThread	Thread;
		};

		struct ElementIterator
		{
			ElementIterator(Element* i) : I{ i } {}

			ElementIterator operator ++()
			{
				if (I->_Next)
					I = I->_Next;
				else
					I = nullptr;

				return *this;
			}

			ElementIterator operator --()
			{
				if (I->_Prev)
					I = I->_Prev;
				else
					I = nullptr;

				return *this;
			}

			bool operator == (const ElementIterator& rhs) const
			{
				return (rhs.I == I);
			}

			bool operator != (const ElementIterator& rhs) const
			{
				return !(rhs == *this);
			}

			WorkerThread& operator* ()
			{
				return I->Thread;
			}

			WorkerThread* operator -> ()
			{
				return &I->Thread;
			}


			Element* I;
		};


		WorkerList(iAllocator* Memory, const size_t IN_ThreadCount = 2) :
			ThreadCount	{ IN_ThreadCount },
			Allocator	{ Memory },
			Begin		{ nullptr },
			End			{ nullptr }
		{
		}


		ElementIterator begin()
		{
			return { Begin };
		}


		ElementIterator end()
		{
			return { nullptr };
		}


		void Rotate()
		{
			MoveToBack(begin());
		}


		void MoveToBack(ElementIterator Itr)
		{
			std::unique_lock<std::mutex> Lock(M);

			if (Begin == Itr.I)
				Begin = Itr.I->_Next;

			if (Itr.I->_Prev)
				Itr.I->_Prev->_Next = Itr.I->_Next;

			if (Itr.I->_Next)
				Itr.I->_Next->_Prev = Itr.I->_Prev;

			if (End)
			{
				Itr.I->_Prev = End;
				End->_Next = Itr.I;
				Itr.I->_Next = nullptr;
			}

			End = Itr.I;
		}


		void AddThread(iAllocator*)
		{
			std::unique_lock<std::mutex> Lock(M);

			auto NewThread = new Element;

			if (End)
			{
				NewThread->_Prev = End;
				End->_Next = NewThread;
			}

			if (!Begin)
				Begin = NewThread;

			End = NewThread;
		}


	private:
		Element * Begin;
		Element*	End;
		size_t		ThreadCount;
		iAllocator*	Allocator;
		std::mutex	M;
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
	LambdaWork<TY_FN>& CreateLambdaWork_New(
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
		ThreadManager(iAllocator* memory, size_t ThreadCount = 4) :
			Threads(memory),
			Memory(memory)
		{
			WorkerThread::Manager = this;

			for (size_t I = 0; I < ThreadCount; ++I)
				Threads.AddThread(memory);
		}


		void Release()
		{
			WaitForWorkersToComplete();

			for (auto& I : Threads)
				I.Shutdown();

			WaitForShutdown();
		}


		/*
		template<typename TY_FN>
		void AddWork(TY_FN FN, iAllocator* Memory = nullptr)
		{
			LambdaWork<TY_FN>* Item = &Memory->allocate<LambdaWork<TY_FN>>(FN, Memory);

			AddWork(static_cast<iWork*>(Item));
		}
		*/

		void AddWork(iWork* Work)
		{
			bool success = false;
			do {
				success = Threads.begin()->AddItem(Work);
				Threads.Rotate();
			} while (!success);
		}


		void WaitForWorkersToComplete()
		{
			std::mutex						M;
			std::unique_lock<std::mutex>	Lock(M);

			CV.wait(Lock, [this]{ return WorkingThreadCount <= 0; });
		}


		void WaitForShutdown()
		{
			bool ThreadRunning = false;
			do
			{
				ThreadRunning = false;
				for (auto& I : Threads)
					ThreadRunning |= I.IsRunning();
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
		WorkerList					Threads;

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
