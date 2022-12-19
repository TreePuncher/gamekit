#pragma once

#include <Application.h>
#include <graphics.h>
#include <Win32Graphics.h>
#include <filesystem>
#include <expected>
#include <mutex>


class DescriptorAllocatorTest : public FlexKit::FrameworkState
{
public:
	DescriptorAllocatorTest(FlexKit::GameFramework& IN_framework);
	~DescriptorAllocatorTest() final;

	FlexKit::UpdateTask* Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT) final;
	FlexKit::UpdateTask* Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph) final;

	void PostDrawUpdate(FlexKit::EngineCore&, double dT) final;
	bool EventHandler(FlexKit::Event evt) final;

	FlexKit::MemoryPoolAllocator		gpuAllocator;
	FlexKit::CameraComponent			cameras;
	FlexKit::SceneNodeComponent			sceneNodes;

	bool							pause = false;
	bool							debugVis = false;

	FlexKit::RootSignature			sortingRootSignature;
	FlexKit::Win32RenderWindow		renderWindow;

	FlexKit::ConstantBufferHandle	constantBuffer;

	FlexKit::RunOnceQueue<void(FlexKit::UpdateDispatcher&, FlexKit::FrameGraph&)>	runOnceQueue;
};
