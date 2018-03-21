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

#include "..\pch.h"
#include "timeutilities.h"
#include "ThreadUtilities.h"
#include <atomic>
#include <chrono>

using std::mutex;

namespace FlexKit
{
	bool WorkerThread::AddItem(iWork* Item)
	{
		if (!Running || Quit)
			return false;

		std::unique_lock<std::mutex> Lock(AddLock);

		WorkList.push_back(Item);
		CV.notify_all();

		return true;
	}


	void WorkerThread::Shutdown()
	{
		Quit = true;
		CV.notify_all();
	}


	void WorkerThread::Run()
	{
		Running.store(true);

		EXITSCOPE({
			Running.store(false);
		});


		while (true)
		{
			std::unique_lock<std::mutex> Lock(CBLock);
			CV.wait(Lock);

			while (WorkList.size())
			{
				auto WorkItem = WorkList.front();
				WorkList.pop_front();

				if (WorkItem) {
					Manager->IncrementActiveWorkerCount();

					WorkItem->Run();
					WorkItem->NotifyWatchers();

					Manager->DecrementActiveWorkerCount();
				}
			}

			if (!WorkList.size() && Quit)
				return;
		}
	}

	bool WorkerThread::IsRunning()
	{
		return Running.load();
	}

	ThreadManager* WorkerThread::Manager = nullptr;
}