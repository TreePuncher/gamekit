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

#ifndef THREADUTILITIES_H
#define THREADUTILITIES_H

#include "..\pch.h"
#include "timeutilities.h"
#include "ThreadUtilities.h"
#include <atomic>
#include <chrono>

using std::mutex;

namespace FlexKit
{
	/************************************************************************************************/


	bool WorkerThread::AddItem(iWork* Work)
	{
		if (!Running || Quit)
			return false;

		WorkList.push_back(Work);
		CV.notify_all();

		return true;
	}


	/************************************************************************************************/


	void WorkerThread::Shutdown()
	{
		Quit = true;
		CV.notify_all();
	}


	/************************************************************************************************/


	void WorkerThread::Run()
	{
		Running.store(true);

		EXITSCOPE({
			Running.store(false);
		});


		while (true)
		{
			std::mutex	M;
			std::unique_lock<std::mutex> Lock(M);
			CV.wait(Lock);

			{
				EXITSCOPE({
					Manager->DecrementActiveWorkerCount();
				});

				Manager->IncrementActiveWorkerCount();

				while (!WorkList.empty())
				{
					iWork* work;
				
					if (WorkList.try_pop_front(work)) {
						work->Run();
						work->NotifyWatchers();
						work->Release();
					}
				}

				const auto Try_Count = Manager->GetThreadCount();
				for(auto I = 0; I < Try_Count; ++I)
				{
					iWork* work = Manager->StealSomeWork();

					if (work) {
						work->Run();
						work->NotifyWatchers();
						work->Release();
					}
				}

				if (WorkList.empty() && Quit)
					return;
			}
		}

	}


	/************************************************************************************************/


	bool WorkerThread::IsRunning()
	{
		return Running.load();
	}


	void WorkerThread::Wake()
	{
		CV.notify_all();
	}


	/************************************************************************************************/


	iWork* WorkerThread::Steal()
	{
		iWork* work = nullptr;

		return WorkList.try_pop_front(work) ? work : nullptr;
	}

	/************************************************************************************************/


	void WorkBarrier::AddDependentWork(iWork* Work)
	{
		FK_ASSERT(Work != nullptr);

		++TaskInProgress;
		Work->Subscribe(
			[this]
			{
				TaskInProgress--; 
				CV.notify_all(); 
			});
	}


	/************************************************************************************************/


	void WorkBarrier::AddOnCompletionEvent(OnCompletionEvent Callback)
	{
		PostEvents.push_back(Callback);
	}


	/************************************************************************************************/


	void WorkBarrier::Wait()
	{
		std::mutex M;
		std::unique_lock<std::mutex> Lock(M);

		CV.wait(Lock, [this]()->bool
		{
			return TaskInProgress == 0;
		});

		for (auto Evt : PostEvents)
			Evt();
	}


	ThreadManager* WorkerThread::Manager = nullptr;
}	/************************************************************************************************/


#endif