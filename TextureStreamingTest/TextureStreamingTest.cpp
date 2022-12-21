#include "pch.h"
#include "TextureStreamingTest.h"
#include <SceneLoadingContext.h>

using namespace FlexKit;


/************************************************************************************************/


TextureStreamingTest::TextureStreamingTest(FlexKit::GameFramework& IN_framework) :
	FrameworkState		{ IN_framework },

	animators				{ framework.core.GetBlockMemory() },
	brushes					{ framework.core.GetBlockMemory(), framework.GetRenderSystem() },
	cameras					{ framework.core.GetBlockMemory() },
	sceneNodes				{},
	materials				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	visibilityComponent		{ framework.core.GetBlockMemory() },
	pointLights				{ framework.core.GetBlockMemory() },
	orbitCameras			{ framework.core.GetBlockMemory() },
	pointLightShadowMaps	{ framework.core.GetBlockMemory() },
	ikComponent				{ framework.core.GetBlockMemory() },
	skeletons				{ framework.core.GetBlockMemory() },

	physx					{ framework.core.Threads, framework.core.GetBlockMemory() },
	rigidBodies				{ physx },
	staticBodies			{ physx },

	renderer				{ framework.GetRenderSystem(), textureStreamingEngine, framework.core.GetBlockMemory() },
	textureStreamingEngine	{ framework.GetRenderSystem(), framework.core.GetBlockMemory() },

	gbuffer			{ { 1920, 1080 }, framework.GetRenderSystem() },
	depthBuffer		{ framework.GetRenderSystem(), { 1920, 1080 } },
	renderWindow	{},


	constantBuffer	{ framework.GetRenderSystem().CreateConstantBuffer(64 * MEGABYTE, false) },
	vertexBuffer	{ framework.GetRenderSystem().CreateVertexBuffer(64 * MEGABYTE, false) },
	runOnceQueue	{ framework.core.GetBlockMemory() },
	scene			{ framework.core.GetBlockMemory() }

{	// Setup Window and input
	if (auto res = CreateWin32RenderWindow(framework.GetRenderSystem(), { .height = 1080, .width = 1920 }); res)
		renderWindow = std::move(res.value());

	FlexKit::EventNotifier<>::Subscriber sub;
	sub.Notify = &FlexKit::EventsWrapper;
	sub._ptr = &framework;

	renderWindow.Handler->Subscribe(sub);
	renderWindow.SetWindowTitle("Texture Streaming");

	// Load Test Scene
	layer = physx.CreateLayer(false);

	SceneLoadingContext loadCtx{
		.scene = scene,
		.layer = layer,
		.nodes = Vector<FlexKit::NodeHandle>{ framework.core.GetBlockMemory() }
	};

	AddAssetFile(R"(assets\TextureStreaming.gameres)");
	auto loadSuccess = LoadScene(framework.core, loadCtx, 1234);

	// Setup Camera
	activeCamera = cameras.CreateCamera((float)pi / 4.0f, renderWindow.GetAspectRatio());
	SetCameraNode(activeCamera, GetZeroedNode());
	TranslateWorld(GetCameraNode(activeCamera), { 0, 3, 0 });
}

/************************************************************************************************/


TextureStreamingTest::~TextureStreamingTest()
{
	scene.ClearScene();

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


FlexKit::UpdateTask* TextureStreamingTest::Draw(FlexKit::UpdateTask* update, FlexKit::EngineCore& core, FlexKit::UpdateDispatcher& dispatcher, double dT, FlexKit::FrameGraph& frameGraph)
{
	frameGraph.AddOutput(renderWindow.GetBackBuffer());

	ClearDepthBuffer(frameGraph, depthBuffer.Get(), 1.0f);

	FlexKit::WorldRender_Targets targets{
		.RenderTarget = renderWindow.GetBackBuffer(),
		.DepthTarget = depthBuffer,
	};
	ReserveConstantBufferFunction	reserveCB = FlexKit::CreateConstantBufferReserveObject(constantBuffer, core.RenderSystem, core.GetTempMemory());
	ReserveVertexBufferFunction		reserveVB = FlexKit::CreateVertexBufferReserveObject(vertexBuffer, core.RenderSystem, core.GetTempMemory());

	Yaw(GetCameraNode(activeCamera), pi * dT / 3.0f);
	cameras.MarkDirty(activeCamera);

	static double T = 0.0;
	T += dT;

	auto& transformUpdate	= FlexKit::QueueTransformUpdateTask(dispatcher);
	auto& cameraUpdate		= cameras.QueueCameraUpdate(dispatcher);

	cameraUpdate.AddInput(transformUpdate);

	FlexKit::DrawSceneDescription drawSceneDesc{
		.camera = activeCamera,
		.scene	= scene,
		.dt		= dT,
		.t		= T,

		.gbuffer = gbuffer,

		.reserveVB = reserveVB, 
		.reserveCB = reserveCB, 

		.transformDependency	= transformUpdate,
		.cameraDependency		= cameraUpdate
	};

	auto res = renderer.DrawScene(
		dispatcher,
		frameGraph,
		drawSceneDesc,
		targets,
		core.GetBlockMemory(),
		core.GetTempMemoryMT()
	);

	/*
	struct TestPass
	{
		FrameResourceHandle renderTarget;
		FrameResourceHandle depthTarget;
	};

	auto getPass =
		[&passTable = res.passes.GetData()](PassHandle pass) -> const PassPVS*
		{
			return passTable.GetPass(pass).value_or(nullptr);
		};

	frameGraph.AddTaskDependency(res.passes);

	PassDescription<TestPass> pass =
	{
		.materialPassID		= GBufferPassID,
		.sharedData			= TestPass{},
		.getPass			= getPass,
	};

	auto setupFn = [&](FrameGraphNodeBuilder& builder, TestPass& data)
	{
		data.renderTarget	= builder.PixelShaderResource(targets.RenderTarget);
		data.depthTarget	= builder.DepthTarget(targets.DepthTarget.Get());
	};

	auto drawFN = [](auto begin, auto end, auto& ctx)
	{
	};
	*/
	//auto& newPass = frameGraph.AddPass(pass, setupFn, drawFN);
	
	textureStreamingEngine.TextureFeedbackPass(dispatcher, frameGraph, activeCamera, { 256, 256 }, res.passes, res.skinnedDraws, reserveCB, reserveVB);

	FlexKit::PresentBackBuffer(frameGraph, renderWindow);

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


/**********************************************************************

Copyright (c) 2014-2022 Robert May

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
