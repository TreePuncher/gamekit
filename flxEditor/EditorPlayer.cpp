#include <Application.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <SharedEngineMemory.hpp>

#include <scn/scn.h>
#include <string_view>
#include "Win32Graphics.h"

using namespace boost::interprocess;
using namespace FlexKit;

class EditorPlayerState : public FrameworkState
{
public:
	EditorPlayerState(GameFramework& in_framework, HWND hwnd) : FrameworkState{ in_framework }
	{
		renderWindow = std::move(FlexKit::CreateWin32RenderWindowFromHWND(framework.GetRenderSystem(), hwnd).first);

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify	= &FlexKit::EventsWrapper;
		sub._ptr	= &framework;

		renderWindow.Handler->Subscribe(sub);
	}

	UpdateTask* Draw(UpdateTask* update, EngineCore&, UpdateDispatcher&, double dT, FrameGraph& frameGraph)
	{
		frameGraph.AddOutput(renderWindow.GetBackBuffer());

		ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), float4{ (rand() % 1024) / 1024.0f , (rand() % 1024) / 1024.0f, (rand() % 1024) / 1024.0f, 1.0f});

		return nullptr;
	}

	void PostDrawUpdate(EngineCore&, double dT)
	{
		renderWindow.Present(1, 0);
	}

	bool EventHandler(Event evt)
	{
		return false;
	}


	Win32RenderWindow renderWindow;
};

int PlayerMain(int argc, char* argv[])
{
	if (argc < 3)
		return -1;

	try
	{
		shared_memory_object shm_obj(
			open_or_create,
			"shared_memory",
			read_write);

		size_t offset = 0;
		for (int I = 0; I + 1 < argc; I++)
		{
			std::string_view arg{ argv[I] };
			if (arg == "--player")
			{
				auto res = scn::scan(std::string_view{ argv[I + 1] }, "{}", offset);
				if (res)
					break;
			}
		}

		if (offset == 0)
		{
			FK_LOG_ERROR("Invalid Arguments!");
			return -1;
		}

		SharedEngineMemory* shared = GetSharedMemory(shm_obj, offset);

		auto* allocator = FlexKit::CreateEngineMemory();
		EXITSCOPE(ReleaseEngineMemory(allocator));

		CoreOptions options{
			.GPUdebugMode = true
		};

		auto app = std::make_unique<FlexKit::FKApplication>(allocator, options);
		app->PushState<EditorPlayerState>(shared->targetWindow);

		MessageBlob empty{};

		shared->outputQueue.push_front(empty);

		app->GetCore().FPSLimit		= 90;
		app->GetCore().FrameLock	= false;
		app->GetCore().vSync		= true;
		app->Run();
	}
	catch (...)
	{
		return -1;
	}
	return 0;
}
