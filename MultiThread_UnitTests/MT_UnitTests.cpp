#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{		
	TEST_CLASS(MultiThreadUnitTests)
	{
	public:

		class TestClass
		{
		public:
			TestClass(int X) :
				x{ X } 
			{
				Logger::WriteMessage( "TestClass(int)\n" );
			}


			~TestClass() // Destructor
			{
				Logger::WriteMessage( "~TestClass()\n" );
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

			void print()
			{
				std::cout << x << "\n";
			}

			const int x;
		};

		template<class TY>
		using Deque			= FlexKit::Deque_MT<TY>;
		using ElementType	= Deque<TestClass>::Element_TY;


		// Tests if removing from ends will cause obvious failures
		TEST_METHOD(BasicTestDequeContainer)
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

					if (res != true)
						throw;
				}

				deque.push_front(N1);
				deque.push_front(N2);
				deque.push_front(N3);
				deque.push_front(N4);

				auto& Temp = deque.pop_front();
			}
			catch (...)
			{
				Assert::IsTrue(false);
			}

			Assert::IsTrue(true);
		}
	};
}