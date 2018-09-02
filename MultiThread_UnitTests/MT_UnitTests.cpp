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
				//Logger::WriteMessage( "TestClass(int)\n" );
			}


			~TestClass() // Destructor
			{
				//Logger::WriteMessage( "~TestClass()\n" );
			}


			TestClass(TestClass&& in) 
				: TestClass(in.x) // Move Constructor
			{
				Logger::WriteMessage( "TestClass(TestClass&& rhs)\n" );
			}


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
				//Logger::WriteMessage(Message.c_str());

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
					auto res = deque.try_pop_front(E);
					if (res != false)
						throw;

					deque.push_back(N1);
					res = deque.try_pop_front(E);

					if (res != true) // Container should be return one element
						throw;

					res = deque.try_pop_front(E);
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
					if (deque.try_pop_front(E_ptr))
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
					for (auto& I : Test)
						I.SetX(N + itr++);
				}


				void Run() override
				{
					std::mutex			M;
					std::unique_lock	UL{ M };
					CV.wait(UL);

					for(auto& I : Test)
						Queue.push_back(I);

					//Logger::WriteMessage("Hello World from AddThread\n");
					//std::cout << "Hello World!\n";
				}

				std::condition_variable&		CV;
				Deque<TestClass>&				Queue;
				Deque<TestClass>::Element_TY	Test[1000];

			}Thread1{ 0, CV, Queue }, Thread2{ 1000, CV, Queue }, Thread3{ 2000, CV, Queue }, Thread4{ 3000, CV, Queue };

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

					Sleep(1);

					while (!Queue.empty())
					{
						ElementType* E_ptr = nullptr;
						if (Queue.try_pop_front(E_ptr))
							E_ptr->print();
					}

					//Logger::WriteMessage("Hello World from PopThread\n");
					//std::cout << "Hello World!\n";
				}

				std::condition_variable&	CV;
				Deque<TestClass>&			Queue;
			}Thread5{ CV, Queue }, Thread6{ CV, Queue }, Thread7{ CV, Queue }, Thread8{ CV, Queue };

			Deque<TestClass>::Element_TY	Test{ 4000 };
			Test.SetX(4001);

			Threads.AddWork(&Thread1);
			Threads.AddWork(&Thread2);
			Threads.AddWork(&Thread3);
			Threads.AddWork(&Thread4);
			Threads.AddWork(&Thread5);
			Threads.AddWork(&Thread6);
			Threads.AddWork(&Thread7);
			Threads.AddWork(&Thread8);


			Logger::WriteMessage("Waiting 1 second.\n");

			Sleep(1000);

			Queue.push_back(Test);
			CV.notify_all();

			Threads.WaitForWorkersToComplete();
			//Threads.WaitForShutdown();

			Sleep(1000);

			Assert::IsTrue(Queue.empty());
		}

	};
}