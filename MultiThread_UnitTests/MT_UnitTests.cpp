/**********************************************************************

Copyright (c) 2015 - 2018 Robert May

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
				//Logger::WriteMessage( "TestClass(TestClass& rhs)\n" );
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
						Assert::IsTrue(false, L"Container returned a non-existing element!");

					deque.push_back(N1);
					res = deque.try_pop_front(&E);

					if (res != true) // Container should be return one element
						Assert::IsTrue(false, L"Container failed to pop element!");


					res = deque.try_pop_front(&E);
					if (res != false) // Container should be empty
						Assert::IsTrue(false, L"Container failed to pop element!");
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

				deque.push_front(N1);
				deque.push_back(N2);
				deque.push_front(N3);
				deque.push_back(N4);

				while (!deque.empty())
				{
					ElementType* E_ptr = nullptr;
					if (deque.try_pop_back(&E_ptr))
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

			FlexKit::ThreadManager Threads{ 8 };

			std::condition_variable CV;

			class AddThread : public FlexKit::iWork
			{
			public:
				AddThread(
					int							N,
					std::condition_variable&	IN_CV,
					Deque<TestClass>&			IN_Queue) :
					iWork{ FlexKit::SystemAllocator },
					CV{ IN_CV },
					Queue{ IN_Queue }
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



					int n = 0;

					for (auto& I : Test)
					{
						bool front_or_back = (n % 2) == 0;

						if (front_or_back)
							Queue.push_back(I);
						else
							Queue.push_front(I);

						n++;
					}
				}

				std::condition_variable&		CV;
				Deque<TestClass>&				Queue;
				std::vector<Deque<TestClass>::Element_TY>	Test;

			}Thread1{ 0, CV, Queue }, Thread2{ 4000, CV, Queue }, Thread3{ 8000, CV, Queue }, Thread4{ 12000, CV, Queue };


			class PopThread : public FlexKit::iWork
			{
			public:
				PopThread(
					std::condition_variable&	IN_CV,
					Deque<TestClass>&			IN_Queue ) :
					iWork{ FlexKit::SystemAllocator },
					CV{ IN_CV },
					Queue{ IN_Queue }
				{
					Ints.reserve(4000);
				}


				void Run() override
				{
					std::mutex			M;
					std::unique_lock	UL{ M };
					CV.wait(UL);

					Sleep(5);

					int n = 0;
					while (!Queue.empty())
					{
						ElementType* E_ptr = nullptr;

						bool front_or_back = (n % 2) == 0;
						bool success = false;

						if (front_or_back)
							success = Queue.try_pop_back(&E_ptr);
						else
							success = Queue.try_pop_front(&E_ptr);

						if (success)
							Ints.push_back(E_ptr->x);

						n++;
					}
				}

				std::condition_variable&	CV;
				Deque<TestClass>&			Queue;
				std::vector<int>			Ints;
			}	Thread5{ CV, Queue },
				Thread6{ CV, Queue },
				Thread7{ CV, Queue },
				Thread8{ CV, Queue };


			FlexKit::WorkItem Work1{ &Thread1 };
			FlexKit::WorkItem Work2{ &Thread2 };
			FlexKit::WorkItem Work3{ &Thread3 };
			FlexKit::WorkItem Work4{ &Thread4 };
			FlexKit::WorkItem Work5{ &Thread5 };
			FlexKit::WorkItem Work6{ &Thread6 };
			FlexKit::WorkItem Work7{ &Thread7 };
			FlexKit::WorkItem Work8{ &Thread8 };

			Threads.AddWork(&Work1);
			Threads.AddWork(&Work2);
			Threads.AddWork(&Work3);
			Threads.AddWork(&Work4);

			CV.notify_all();
			Sleep(1000);

			Threads.AddWork(&Work5);
			Threads.AddWork(&Work6);
			Threads.AddWork(&Work7);
			Threads.AddWork(&Work8);


			Sleep(1000);

			CV.notify_all();

			Sleep(1000);

			Threads.Release();

			std::vector<int> Sorted;

			for (auto& I : Thread5.Ints)
				Sorted.push_back(I);
			for (auto& I : Thread6.Ints)
				Sorted.push_back(I);
			for (auto& I : Thread7.Ints)
				Sorted.push_back(I);
			for (auto& I : Thread8.Ints)
				Sorted.push_back(I);

			std::sort(Sorted.begin(), Sorted.end());

			int Last = 0;
			for (int idx = 1; idx < Sorted.size(); idx++)
			{
				if (Sorted[idx] - Last > 1)
					Assert::IsTrue(false, L"Missing Element Detected");

				if (Sorted[idx] - Last == 0)
					Assert::IsTrue(false, L"Double Element Detected");

				Last = Sorted[idx];
			}

			for (auto I : Sorted)
			{
				char Temp[50];
				itoa(I, Temp, 10);

				Logger::WriteMessage(Temp);
			}

			//Logger::WriteMessage("Test Complete");
			Assert::IsTrue(Queue.empty(), L"Queue is not empty");
		}

	};
}