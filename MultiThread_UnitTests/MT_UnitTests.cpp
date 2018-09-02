#include "stdafx.h"
#include <iostream>
#include "CppUnitTest.h"

#include "..\coreutilities\ThreadUtilities.cpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{		
	TEST_CLASS(MultiThreadUnitTests)
	{
	public:

		class TestClass
		{
		public:
			TestClass(int X = 0) :
				x{ X } 
			{
				SetX(x);
				//Logger::WriteMessage( "TestClass(int)\n" );
			}


			~TestClass() // Destructor
			{
				//Logger::WriteMessage( "~TestClass()\n" );
			}


			TestClass(TestClass&& in) = default;

			/*
			TestClass(TestClass&& in) 
				: TestClass(in.x) // Move Constructor
			{
				Logger::WriteMessage( "TestClass(TestClass&& rhs)\n" );
			}
			*/

			TestClass(TestClass& in) 
				: TestClass(in.x) // Copy Constructor
			{
				Logger::WriteMessage( "TestClass(TestClass& rhs)\n" );
			}

			void SetX(int new_x)
			{
				x = new_x;
				std::stringstream SS;
				SS << "int: " << new_x;
				Message = SS.str();
			}

			void print()
			{
				
				//Logger::WriteMessage("print(TestClass& rhs)\n");
				Logger::WriteMessage(Message.c_str());

				//std::cout << x << "\n";
			}

			std::string Message;
			int x;
		};

		template<class TY>
		using Deque			= FlexKit::Deque_MT<TY>;
		using ElementType	= Deque<TestClass>::Element_TY;


		// Tests if removing from ends will cause obvious failures
		TEST_METHOD(Deque_BasicFunctionsTest)
		{
			std::condition_variable		CV;
			std::condition_variable		WorkStartBarrier;
			std::atomic_int64_t			WorkersAwake;
			const size_t				ThreadCount = 1;

			try
			{
				ElementType N1(1);
				ElementType N2(2);
				ElementType N3(3);
				ElementType N4(4);

				Deque<TestClass> deque;

				// Test One
				{
					ElementType* E;
					auto res = deque.try_pop_front(&E);
					if (res != false)
						throw;

					deque.push_back(N1);
					res = deque.try_pop_front(&E);

					if (res != true) // Container should be return one element
						throw;

					res = deque.try_pop_front(&E);
					if (res != false) // Container should be empty
						throw;
				}

				deque.push_front(N1);
				deque.push_back	(N2);
				deque.push_front(N3);
				deque.push_back	(N4);

				while (!deque.empty())
				{
					ElementType* E_ptr = nullptr;
					if (deque.try_pop_front(&E_ptr))
						E_ptr->print();
				}
			}
			catch (...)
			{
				Assert::IsTrue(false);
			}

			Assert::IsTrue(true);
		}


		TEST_METHOD(Deque_SingleReaderSingleWriterTest)
		{
			Deque<TestClass> Queue;

			FlexKit::ThreadManager Threads{8};

			std::condition_variable CV;

			class AddThread : public FlexKit::iWork
			{
			public:
				AddThread(
						int							N,
						std::condition_variable&	IN_CV,
						Deque<TestClass>&			IN_Queue) :
					iWork		{ FlexKit::SystemAllocator },
					CV			{ IN_CV },
					Queue		{ IN_Queue }
				{
					size_t itr = 0;
					for (size_t itr = 0; itr < 4000; ++itr)
						Test.push_back(itr + N);
				}


				void Run() override
				{
					std::mutex			M;
					std::unique_lock	UL{ M };
					CV.wait(UL);

					for(auto& I : Test)
						Queue.push_back(I);

					Logger::WriteMessage("Hello World from AddThread\n");
					std::cout << "Hello World!\n";
				}

				std::condition_variable&		CV;
				Deque<TestClass>&				Queue;
				std::vector<Deque<TestClass>::Element_TY>	Test;

			}Thread1{ 0, CV, Queue }, Thread2{ 4000, CV, Queue }, Thread3{ 8000, CV, Queue }, Thread4{ 12000, CV, Queue };

			class PopThread: public FlexKit::iWork
			{
			public:
				PopThread(
						std::condition_variable&	IN_CV,
						Deque<TestClass>&			IN_Queue) :
					iWork	{ FlexKit::SystemAllocator },
					CV		{ IN_CV		},
					Queue	{ IN_Queue	}
				{}


				void Run() override
				{
					std::mutex			M;
					std::unique_lock	UL{ M };
					CV.wait(UL);

					Sleep(5);

					while (!Queue.empty())
					{
						ElementType* E_ptr = nullptr;
						if (Queue.try_pop_front(&E_ptr))
							E_ptr->print();
					}

					//Logger::WriteMessage("Hello World from PopThread\n");
					//std::cout << "Hello World!\n";
				}

				std::condition_variable&	CV;
				Deque<TestClass>&			Queue;
			}Thread5{ CV, Queue }, Thread6{ CV, Queue }, Thread7{ CV, Queue }, Thread8{ CV, Queue };

			
			FlexKit::WorkItem Work1{ &Thread1 };
			FlexKit::WorkItem Work2{ &Thread2 };
			FlexKit::WorkItem Work3{ &Thread3 };
			FlexKit::WorkItem Work4{ &Thread4 };
			FlexKit::WorkItem Work5{ &Thread5 };
			FlexKit::WorkItem Work6{ &Thread6 };
			FlexKit::WorkItem Work7{ &Thread7 };
			FlexKit::WorkItem Work8{ &Thread8 };

			Deque<TestClass>::Element_TY	Test{ 4000 };
			Test.SetX(4001);


			Threads.AddWork(&Work1);
			Threads.AddWork(&Work2);
			Threads.AddWork(&Work3);
			Threads.AddWork(&Work4);
			Threads.AddWork(&Work5);
			Threads.AddWork(&Work6);
			Threads.AddWork(&Work7);
			Threads.AddWork(&Work8);


			Logger::WriteMessage("Waiting 1 second.");

			Sleep(1000);

			Queue.push_back(Test);
			CV.notify_all();

			Sleep(1000);

			Threads.Release();

			Logger::WriteMessage("Test Complete");
			Assert::IsTrue(Queue.empty());
		}

	};
}