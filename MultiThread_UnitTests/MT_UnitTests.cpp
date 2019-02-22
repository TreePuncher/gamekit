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
	static std::atomic_int readyCount = 0;

	TEST_CLASS(MultiThreadUnitTests)
	{
	public:

		class TestClass : public FlexKit::DequeNode_MT
		{
		public:
			TestClass(size_t X = 0) :
				x{ X } 
			{
				SetX(x);
			}


			~TestClass() // Destructor
			{
			}


			TestClass(TestClass&& in) = default;
			TestClass(TestClass& in) 
				: TestClass(in.x) // Copy Constructor
			{}

			void SetX(int new_x)
			{
				x = new_x;
				std::stringstream SS;
				SS << "int: " << new_x;
				Message = SS.str();
			}

			void print()
			{
				Logger::WriteMessage(Message.c_str());
			}

			std::string Message;
			size_t x;
		};

		template<class TY>
		using Deque			= FlexKit::Deque_MT<TY>;


		// Tests if removing from ends will cause obvious failures
		TEST_METHOD(Deque_BasicFunctionsTest)
		{
			try
			{
				TestClass N1(1);
				TestClass N2(2);
				TestClass N3(3);
				TestClass N4(4);

				Deque<TestClass> deque;

				// Test One
				{
					TestClass* E;
					auto res = deque.try_pop_front(E);
					if (res != false)
						Assert::IsTrue(false, L"Container returned a non-existing element!\n");

					deque.push_back(&N1);
					res = deque.try_pop_front(E);

					if (res != true) // Container should be return one element
						Assert::IsTrue(false, L"Container failed to pop element!\n");


					res = deque.try_pop_front(E);
					if (res != false) // Container should be empty
						Assert::IsTrue(false, L"Container failed to pop element!\n");
				}

				deque.push_front(N1);
				deque.push_front(N2);
				deque.push_front(N3);
				deque.push_front(N4);

				while (!deque.empty())
				{
					TestClass* E_ptr = nullptr;
					if (deque.try_pop_front(E_ptr))
						E_ptr->print();
				}

				deque.push_back	(N1);
				deque.push_back	(N2);
				deque.push_back	(N3);
				deque.push_back	(N4);

				while (!deque.empty())
				{
					TestClass* E_ptr = nullptr;
					if (deque.try_pop_back(E_ptr))
						E_ptr->print();
				}
			}
			catch (...)
			{
				Assert::IsTrue(false);
			}

			Assert::IsTrue(true);
		}

		static const size_t testSize	= 1000u;
		static const size_t passCount	= 100;

		TEST_METHOD(Deque_MultiReaderMultiWriterTest)
		{
			FlexKit::ThreadManager Threads{ 6 };

			for (size_t i = 0; i < passCount; ++i)
			{
				Deque<TestClass> Queue;

				std::condition_variable CV;

				Logger::WriteMessage(L"Setting up test\n");

				class AddThread : public FlexKit::iWork
				{
				public:
					AddThread(
						size_t						IN_N,
						std::condition_variable&	IN_CV,
						Deque<TestClass>&			IN_Queue) :
						iWork	{ FlexKit::SystemAllocator	},
						CV		{ IN_CV						},
						Queue	{ IN_Queue					},
						N		{ IN_N						}
					{
					}


					void Run() override
					{
						Test.reserve(testSize);

						Logger::WriteMessage(L"Thread Creating number Set\n");

						for (size_t itr = 0; itr < testSize; ++itr)
							Test.push_back(itr + N);

						Logger::WriteMessage(L"Thread Finished Creating number Set\n");

						std::mutex			M;
						std::unique_lock	UL{ M };

						readyCount++;
						CV.wait(UL);

						size_t i = 0;

						for (auto& I : Test)
						{
							bool front_or_back = (i % 2) == 0;

							if (front_or_back)
								Queue.push_back(&I);
							else
								Queue.push_front(&I);

							i++;
						}
					}

					const size_t				N;
					std::condition_variable&	CV;
					Deque<TestClass>&			Queue;
					std::vector<TestClass>		Test;

				}	threadGroup1[] = {
					{ 0	* testSize, CV, Queue },
					{ 1	* testSize, CV, Queue },
					{ 2	* testSize, CV, Queue },
					{ 3	* testSize, CV, Queue }};


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
						Ints.reserve(testSize);
						Nodes.reserve(testSize);
					}


					void Run() override
					{
						std::mutex			M;
						std::unique_lock	UL{ M };

						readyCount++;
						CV.wait(UL);

						int n = std::random_device{}() % 159;

						try
						{
							while(true)
							{
								do
								{
									TestClass* E_ptr = nullptr;

									bool front_or_back	= (n % 2)	== 0;
									bool push_or_pull	= (n % 13)	== 0;


									if (push_or_pull && Nodes.size())
									{
										auto node = Nodes.back();
										Nodes.pop_back();

										Queue.push_back(node);
									}
									else
									{
										bool success = false;

										if (front_or_back)
											success = Queue.try_pop_back(E_ptr);
										else
											success = Queue.try_pop_front(E_ptr);

										if (success)
										{
											Assert::IsTrue(E_ptr != nullptr, L"Returned Invalid Value!\n");
											Nodes.push_back(E_ptr);
										}
									}

									n++;
								} while (!Queue.empty());


								for (auto& node : Nodes)
									Ints.push_back(node->x);

								if (Queue.empty())
									return;
							}
						}
						catch (...) 
						{
							Assert::IsTrue(false, L"Crash Detected\n");
						}
					}

					std::condition_variable&	CV;
					Deque<TestClass>&			Queue;
					std::vector<int>			Ints;
					std::vector<TestClass*>		Nodes;
				}	Thread5{ CV, Queue },
					Thread6{ CV, Queue },
					Thread7{ CV, Queue },
					Thread8{ CV, Queue };

				Logger::WriteMessage(L"Beginning tests\n");

				Logger::WriteMessage("Begin stage 1\n");
				Logger::WriteMessage("Preparing...\n");

				for(auto& work : threadGroup1)
					Threads.AddWork(&work);

				{
					FlexKit::WorkBarrier barrier{ Threads };
					readyCount = 0;

					for (auto& work : threadGroup1)
						barrier.AddDependentWork(&work);

					while(readyCount != 4)
						Sleep(1);

					Logger::WriteMessage("Starting Test\n");

					CV.notify_all();
					readyCount = 0;

					barrier.Wait();
					Logger::WriteMessage("Completed Test\n");
				}

				Logger::WriteMessage("Verifying stage 1\n");

				// Verify Contents 1
				{
					std::vector<int> Sorted;
					for (auto& element : Queue)
						Sorted.push_back(element.x);

					std::sort(Sorted.begin(), Sorted.end());

					size_t Last = 0;
					for (size_t idx = 1; idx < Sorted.size(); idx++)
					{
						if (Sorted[idx] - Last > 1)
						{
							for (size_t i = idx - 10; i <= idx; ++i)
							{
								char Temp[50];
								itoa(Sorted[i], Temp, 10);
								Logger::WriteMessage(Temp);
								Logger::WriteMessage("\n");
							}

							Assert::IsTrue(false, L"Missing Element Detected\n");
						}

						if (Sorted[idx] - Last == 0) 
						{
							for (size_t i = idx - 10; i <= idx; ++i)
							{
								char Temp[50];
								itoa(Sorted[i], Temp, 10);
								Logger::WriteMessage(Temp);
								Logger::WriteMessage("\n");
							}

							Assert::IsTrue(false, L"Double Element Detected\n");
						}

						Last = Sorted[idx];
					}
				}

				Logger::WriteMessage("Stage 1 Complete\n");
				Logger::WriteMessage("Begin stage 2\n");
				Logger::WriteMessage("Preparing...\n");

				Threads.AddWork(&Thread5);
				Threads.AddWork(&Thread6);
				Threads.AddWork(&Thread7);
				Threads.AddWork(&Thread8);

				{
					FlexKit::WorkBarrier barrier{ Threads };
					barrier.AddDependentWork(&Thread5);
					barrier.AddDependentWork(&Thread6);
					barrier.AddDependentWork(&Thread7);
					barrier.AddDependentWork(&Thread8);

					while(readyCount != 4)
						Sleep(1);

					Logger::WriteMessage("Starting Test\n");

					CV.notify_all();
					readyCount = 0;

					barrier.Wait();
					Logger::WriteMessage("Completed Test\n");
				}


				std::vector<size_t> Sorted;

				Logger::WriteMessage("verifying stage 2\n");
				// Verify Contents 2
				for (auto& I : Thread5.Ints)
					Sorted.push_back(I);
				for (auto& I : Thread6.Ints)
					Sorted.push_back(I);
				for (auto& I : Thread7.Ints)
					Sorted.push_back(I);
				for (auto& I : Thread8.Ints)
					Sorted.push_back(I);

				Assert::IsTrue(Sorted.size() == testSize * 4, L"Missing Element Detected\n");
				std::sort(Sorted.begin(), Sorted.end());


				size_t Last = 0;
				for (size_t idx = 1; idx < Sorted.size(); idx++)
				{
					if (Sorted[idx] - Last > 1)
					{
						for (size_t i = idx - 10; i <= idx; ++i)
						{
							char Temp[50];
							itoa(Sorted[i], Temp, 10);
							std::cout << (Temp) << "\n";
							Logger::WriteMessage("\n");
						}

						Assert::IsTrue(false, L"Missing Element Detected\n");
					}

					if (Sorted[idx] - Last == 0)
					{
						for (size_t i = idx - 10; i <= idx; ++i)
						{
							char Temp[50];
							itoa(Sorted[i], Temp, 10);
							Logger::WriteMessage(Temp);
							Logger::WriteMessage("\n");
						}

						Assert::IsTrue(false, L"Double Element Detected\n");
					}

					Last = Sorted[idx];
				}

				Assert::IsTrue(Queue.empty(), L"Queue is not empty\n");
				Logger::WriteMessage("Test pass Successful\n");
			}
			Logger::WriteMessage("all passes Successful\n");

			Threads.Release();
		}

	};
}