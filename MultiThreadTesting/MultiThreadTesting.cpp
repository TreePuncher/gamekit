// MultiThreadTesting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>


#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\ThreadUtilities.h"
#include "..\coreutilities\ThreadUtilities.cpp"


#include <Windows.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>


std::condition_variable CV;
std::condition_variable WorkStartBarrier;
std::atomic_int64_t		WorkersAwake;
std::atomic_int64_t		x			= 0;
const size_t			threadCount	= 1;


void Test()
{
	std::mutex						M;
	std::unique_lock<std::mutex>	Lock(M);

	CV.wait(Lock);

	std::cout << "working! \n";

	for (
			size_t i = 0; 
			i < 100000u / threadCount; 
			++i
		) 
		++x;
}


/************************************************************************************************/


namespace FlexKit
{	/************************************************************************************************/

	// Intrusive, Thread-Safe, Double Linked List
	template<typename TY>
	class Deque_MT
	{
	public:
		/************************************************************************************************/


		template<typename TY>
		class Element : public TY
		{
		public:
			typedef Element<TY>						ThisType;
			typedef std::function<void (ThisType*)> DeleterFN;

			ThisType& operator = (const ThisType& ) = delete;
			
			bool operator == (const ThisType& rhs)	{ return this == &rhs; }

			template<typename ... ARGS_TY>
			Element(DeleterFN FN, ARGS_TY&& ... args) :
				TY				( args... ),
				Deleter			{ FN },

				Prev			{ nullptr },
				Next			{ nullptr }{}


			~Element()
			{
				Deleter(this);
			}


			ThisType*	Prev;
			ThisType*	Next;
			DeleterFN	Deleter;
		};


		typedef Element<TY> Element_TY;


		class Iterator
		{
		public:
			Iterator(Element_TY* _ptr) : I{ _ptr } {}

			TY operator ++ (int) 
			{ 
				I = I->Next ? I->Next : nullptr;	
				return *static_cast<TY*>(I); 
			}
			TY operator -- (int) 
			{ 
				I = I->Prev ? I->Prev : nullptr;	
				return *static_cast<TY*>(I); 
			}

			void operator ++ () 
			{ 
				I = I->Next ? I->Next : nullptr; 
			}
			void operator -- () 
			{ 
				I = I->Prev ? I->Prev : nullptr; 
			}


			bool operator !=(const Iterator& rhs) { return I != &rhs; }

			Element_TY& operator* ()				{ return *I; }
			Element_TY* operator-> ()				{ return *I; }

			const Element_TY* operator& () const	{ return I; }
		private:

			Element_TY* I;
		};

		/************************************************************************************************/


		Deque_MT()
		{}


		/************************************************************************************************/


		~Deque_MT()
		{
			for (auto& Itr : *this)
				Itr.Deleter(&Itr);
		}


		/************************************************************************************************/


		Iterator begin()	{ return First; }
		Iterator end()		{ return nullptr; }


		/************************************************************************************************/


		void push_back(Element_TY& E)
		{
			std::unique_lock<std::mutex> Lock(RearLock);

			if (ElementCount <= 2)
			{
				std::unique_lock<std::mutex> Lock(FrontLock);

				++ElementCount;

				if (!First)
					First = &E;

				if (Last)
					Last->Next = &E;

				E.Prev = Last;
				Last = &E;
			}
			else
			{
				++ElementCount;

				if (!First)
					First = &E;

				if (Last)
					Last->Next = &E;

				E.Prev = Last;
				Last = &E;
			}
		}


		/************************************************************************************************/


		void push_front(Element_TY& E)
		{
			std::unique_lock<std::mutex> Lock(FrontLock);

			if (ElementCount <= 2)
			{
				std::unique_lock<std::mutex> Lock(RearLock);

				++ElementCount;
				
				if(First)
					First->Prev = &E;

				if (!Last)
					Last = &E;

				E.Next = First;
				First = &E;
			}
			else
			{
				++ElementCount;

				if (First)
					First->Prev = &E;

				if (!Last)
					Last = &E;

				E.Next = First;
				First = &E;
			}
		}


		/************************************************************************************************/


		Element_TY& pop_front()
		{
			std::unique_lock<std::mutex> Lock{ FrontLock };
			auto Element = First;
			First = First->Next;
			First->Prev = nullptr;
			ElementCount--;

			return *Element;
		}


		/************************************************************************************************/


		bool try_pop_front(Element_TY*& Out)
		{
			if (ElementCount == 0)
				return false;

			auto res = FrontLock.try_lock();
			if (!res)
				return false;

			ElementCount--;
			auto Element = First;
			First = First->Next;

			if(First)
				First->Prev = nullptr;

			Out = Element;

			FrontLock.unlock();
			return true;
		}


		/************************************************************************************************/

	private:
		std::mutex				FrontLock;
		std::mutex				RearLock;
		std::atomic_int64_t		ElementCount = 0;

		Element_TY* First	= nullptr;
		Element_TY* Last	= nullptr;
	};


}	/************************************************************************************************/


template<class TY>
using Deque = FlexKit::Deque_MT<TY>;


bool DeQue_Test()
{
	class TestClass
	{
	public:
		TestClass(int X) :
			x{ X + 1 } {}

		~TestClass()
		{
			std::cout << "Test Deleting!\n";
		}

		void print()
		{
			std::cout << x << "\n";
		}

		int x;
	};

	{
		using ElementType = Deque<TestClass>::Element_TY;
		auto Deleter = [](ElementType*) {std::cout << "Deleting Node!\n"; };

		ElementType N1(Deleter, 1);
		ElementType N2(Deleter, 2);
		ElementType N3(Deleter, 3);
		ElementType N4(Deleter, 4);

		Deque<TestClass> deque;

		// Test One
		{
			ElementType* E;
			auto res = deque.try_pop_front(E);
			if (res != false)
				return false;

			deque.push_back(N1);
			res = deque.try_pop_front(E);

			if (res != true)
				return false;
		}

		deque.push_front(N1);
		deque.push_front(N2);
		deque.push_front(N3);
		deque.push_front(N4);

		for (auto& I : deque)
			I.print();

		auto Temp = deque.pop_front();

		return true;
	}
}


/************************************************************************************************/


bool Thread_Test()
{
	FlexKit::ThreadManager Manager(FlexKit::SystemAllocator, threadCount);
	FlexKit::WorkBarrier   WorkBarrier;

	for (auto I = 0; I < threadCount; ++I) {
		auto Time = (rand() % 5) * 1000;
		auto& Work = FlexKit::CreateLambdaWork_New(
			[Time]()
		{
			++WorkersAwake;
			WorkStartBarrier.notify_all();
			Test();
		});

		WorkBarrier.AddDependentWork(Work);
		Manager.AddWork(Work);
	}


	auto& Work = FlexKit::CreateLambdaWork_New(
		[]()
		{
			std::cout << "Sleeping for " << 1000 << " MS\n";
			Sleep(1000);
		});


	WorkBarrier.AddOnCompletionEvent([] { std::cout << "Work Done1\n"; });
	WorkBarrier.AddOnCompletionEvent([] { std::cout << "Work Done2\n"; });
	WorkBarrier.AddOnCompletionEvent([&WorkBarrier, &Work]
		{
			WorkBarrier.AddDependentWork(Work);
		});

	std::mutex M;
	std::unique_lock<std::mutex> Lock(M);

	WorkStartBarrier.wait(Lock, [] { return WorkersAwake == threadCount; });
	CV.notify_all();
	WorkBarrier.Wait();
	WorkBarrier.Wait();

	Manager.Release();
	std::cout << x << '\n';

	return true;
}


/************************************************************************************************/


int main()
{//
	if (!DeQue_Test())	return -1;
	if (!Thread_Test())	return -1;

    return 0;
}


/************************************************************************************************/