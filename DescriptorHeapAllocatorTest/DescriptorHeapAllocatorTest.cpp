#include "DescriptorHeapAllocatorTest.h"


/************************************************************************************************/


DescriptorAllocatorTest::DescriptorAllocatorTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState			{ IN_framework },
	constantBuffer			{ IN_framework.GetRenderSystem().CreateConstantBuffer(16 * MEGABYTE, false) },
	cameras					{ IN_framework.core.GetBlockMemory() },
	runOnceQueue			{ IN_framework.core.GetBlockMemory() },
	sortingRootSignature	{ IN_framework.core.GetBlockMemory() },
	gpuAllocator			{ IN_framework.core.RenderSystem, 128 * MEGABYTE, 64 * KILOBYTE, FlexKit::DeviceHeapFlags::UAVBuffer, IN_framework.core.GetBlockMemory() }
{
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Sorting Test - WIP");
}


/************************************************************************************************/


DescriptorAllocatorTest::~DescriptorAllocatorTest()
{
	sortingRootSignature.Release();
}


/************************************************************************************************/


FlexKit::UpdateTask* DescriptorAllocatorTest::Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT)
{
	FlexKit::UpdateInput();

	return nullptr;
}


/************************************************************************************************/


FlexKit::UpdateTask* DescriptorAllocatorTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph)
{
	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer());
	PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void DescriptorAllocatorTest::PostDrawUpdate(FlexKit::EngineCore&, double dT)
{
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


bool DescriptorAllocatorTest::EventHandler(FlexKit::Event evt)
{
	return false;
}


/************************************************************************************************/
