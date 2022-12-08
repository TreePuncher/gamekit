#include "pch.h"
#include "TextureStreamingTest.h"


/************************************************************************************************/


TextureStreamingTest::TextureStreamingTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState		{ IN_framework },
	brushes				{ framework.core.GetBlockMemory(), framework.GetRenderSystem() },
	cameras				{ framework.core.GetBlockMemory() },
	sceneNodes			{},
	materials			{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	visibilityComponent	{ framework.core.GetBlockMemory() },
	pointLights			{ framework.core.GetBlockMemory() },

	renderer				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	textureStreamingEngine	{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	renderWindow	{},
	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(64 * MEGABYTE) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(64 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() }
{
	auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 });

	if (res)
		renderWindow = std::move(res.value());

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Texture Streaming");
}

/************************************************************************************************/


TextureStreamingTest::~TextureStreamingTest()
{
	renderWindow.Release();
	framework.GetRenderSystem().ReleaseVB(vertexBuffer);
	framework.GetRenderSystem().ReleaseCB(constantBuffer);
}


/************************************************************************************************/


FlexKit::UpdateTask* TextureStreamingTest::Update(FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT)
{
	FlexKit::UpdateInput();

	return nullptr;
}


/************************************************************************************************/


FlexKit::UpdateTask* TextureStreamingTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore&, FlexKit::UpdateDispatcher&, double dT, FlexKit::FrameGraph& frameGraph)
{
	ClearBackBuffer(frameGraph, renderWindow.GetBackBuffer(), { 1, 0, 1, 0 });
	PresentBackBuffer(frameGraph, renderWindow);

	return nullptr;
}


/************************************************************************************************/


void TextureStreamingTest::PostDrawUpdate(FlexKit::EngineCore&, double dT)
{
	renderWindow.Present(1, 0);
}


/************************************************************************************************/


bool TextureStreamingTest::EventHandler(FlexKit::Event evt)
{
	return false;
}
