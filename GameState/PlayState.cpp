#include "PlayState.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\PipelineState.h"
#include "..\graphicsutilities\TextureUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


#define DEBUGCAMERA


/************************************************************************************************/


PlayState::PlayState(
		GameFramework*			IN_Framework,
		BaseState*				IN_base) : 
	FrameworkState		{ IN_Framework						},

	/*
	ui					{ FlexKit::GuiSystem_Desc{}, framework->core->GetBlockMemory()	},
	uiMainGrid			{ nullptr														},
	uiSubGrid_1			{ nullptr														},
	uiSubGrid_2			{ nullptr														},
	*/

	frameID				{ 0 },
	sound				{ framework->core->Threads, framework->core->GetBlockMemory()	},
	render				{ IN_base->render												},
	base				{ *IN_base														},
	eventMap			{ framework->core->GetBlockMemory()								},
	debugCameraInputMap	{ framework->core->GetBlockMemory()								},
	debugEventsInputMap	{ framework->core->GetBlockMemory()								},

	scene				{	framework->core->RenderSystem,
							framework->core->GetBlockMemory(),
							framework->core->GetTempMemory()	},

	debugCamera			{ }
	//thirdPersonCamera	{ CreateCharacterRig(GetMesh(GetRenderSystem(), "Flower"), &scene) }
{
	debugCamera.SetCameraAspectRatio(GetWindowAspectRatio(framework->core));
	debugCamera.TranslateWorld({0, 100, 0});

	LoadScene(IN_Framework->core, &scene, "TestScene");

	LightModel = GetMesh(GetRenderSystem(), "LightModel");

	eventMap.MapKeyToEvent(KEYCODES::KC_W, ThirdPersonCameraRig::Forward		);
	eventMap.MapKeyToEvent(KEYCODES::KC_A, ThirdPersonCameraRig::Left			);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, ThirdPersonCameraRig::Backward		);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, ThirdPersonCameraRig::Right			);
	eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward	);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward	);
	eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft		);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight	);


	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_C, DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA		);
	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_X, DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY	);


	framework->core->RenderSystem.PipelineStates.RegisterPSOLoader	(DRAW_SPRITE_TEXT_PSO, { nullptr, LoadSpriteTextPSO });
	framework->core->RenderSystem.PipelineStates.QueuePSOLoad		(DRAW_SPRITE_TEXT_PSO, framework->core->GetBlockMemory());

	/*
	uiMainGrid	= &ui.CreateGrid(nullptr);
	uiMainGrid->SetGridDimensions({ 3, 3 });

	uiMainGrid->WH = { 0.5f, 1.0f  };
	uiMainGrid->XY = { 0.25f, 0.0f };

	uiSubGrid_1		= &ui.CreateGrid(uiMainGrid, { 0, 0 });
	uiSubGrid_1->SetGridDimensions({ 2, 2 });
	uiSubGrid_1->WH = { 1.0f, 1.0f };
	uiSubGrid_1->XY = { 0.0f, 0.0f };
	
	uiSubGrid_2 = &ui.CreateGrid(uiMainGrid, {2, 2});
	uiSubGrid_2->SetGridDimensions({ 5, 5 });
	uiSubGrid_2->WH = { 1.0f, 1.0f };
	uiSubGrid_2->XY = { 0.0f, 0.0f };
	
	auto& Btn = ui.CreateButton(uiSubGrid_2, {0, 0});
	*/
}


/************************************************************************************************/


PlayState::~PlayState()
{
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	eventMap.Handle(
		evt, 
		[&](auto& evt)
		{
			debugCamera.HandleEvent(evt);
		});

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Core, UpdateDispatcher& dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool PlayState::DebugDraw(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	return true;
}


/************************************************************************************************/


bool PlayState::PreDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	return false;
}


/************************************************************************************************/


bool PlayState::Draw(EngineCore* core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

	CameraHandle activeCamera = (CameraHandle)debugCamera;

	auto transforms				= QueueTransformUpdateTask	(dispatcher);
	auto cameras				= QueueCameraUpdate			(dispatcher, transforms);
	auto sceneUpdate			= scene.Update				(dispatcher, transforms);
	auto orbitUpdate			= QueueOrbitCameraUpdateTask(dispatcher, transforms, cameras, debugCamera, framework->MouseState, dT);
	auto& PVS					= GetGraphicScenePVSTask	(dispatcher, *sceneUpdate, scene, activeCamera, core->GetTempMemory());
	//auto cameraRigUpdateTask	= UpdateThirdPersonRig		(dispatcher, thirdPersonCamera, *transforms, *cameras, dT);
	auto& cameraConstants		= MakeHeapCopy				(GetCameraConstantBuffer(activeCamera), core->GetTempMemory());

	FlexKit::WorldRender_Targets targets = {
		GetCurrentBackBuffer(&core->Window),
		base.depthBuffer
	};

	ClearVertexBuffer	(frameGraph, base.vertexBuffer);
	ClearVertexBuffer	(frameGraph, base.textBuffer);

	ClearBackBuffer		(frameGraph, 0.0f);
	ClearDepthBuffer	(frameGraph, base.depthBuffer, 1.0f);


	LighBufferDebugDraw debugDraw;
	debugDraw.constantBuffer = base.constantBuffer;
	debugDraw.renderTarget   = targets.RenderTarget;
	debugDraw.vertexBuffer	 = base.vertexBuffer;

	SceneDescription sceneDesc;
	sceneDesc.pointLightCount	= scene.GetPointLightCount();
	sceneDesc.transforms		= transforms;
	sceneDesc.cameras			= cameras;
	sceneDesc.PVS				= PVS;

	base.render.updateLightBuffers				(dispatcher, activeCamera, scene, frameGraph, sceneDesc, core->GetTempMemory());
	base.render.RenderDrawabledPBR_ForwardPLUS	(dispatcher, PVS.solid, activeCamera, targets, frameGraph, sceneDesc, core->GetTempMemory());

	FlexKit::DrawCollection_Desc DrawCollectionDesc{};
	DrawCollectionDesc.DepthBuffer			= targets.DepthTarget;
	DrawCollectionDesc.RenderTarget			= targets.RenderTarget;
	DrawCollectionDesc.instanceBuffer		= base.vertexBuffer;
	DrawCollectionDesc.Mesh					= LightModel;
	DrawCollectionDesc.constantBuffer		= base.constantBuffer;
	DrawCollectionDesc.PSO					= FORWARDDRAWINSTANCED;
	DrawCollectionDesc.InstanceElementSize	= sizeof(float4x4);


	if(true)
	{
		auto cameraConstanstSize = sizeof(decltype(GetCameraConstantBuffer(activeCamera)));

		DrawCollection(
				frameGraph, 
				{ sceneUpdate },
				{	{ 0, (char*)&cameraConstants, cameraConstanstSize },
					{ 0, (char*)&cameraConstants, cameraConstanstSize } },
				{ },
				[&, activeCamera](auto& data)
				{
					cameraConstants = GetCameraConstantBuffer(activeCamera); // update buffer with recent data
				},
				[this, core, activeCamera]()
				{ 
					return scene.FindPointLights(GetFrustum(activeCamera), core->GetTempMemory());
				},
				[&](auto& light) -> float4x4
				{
					XMMATRIX m;
					GetTransform(scene.GetPointLightNode(light), &m);
					return float4x4{ XMMatrixToFloat4x4(&m) }.Transpose();
				},
				DrawCollectionDesc,
				core->GetTempMemory());
	}


	return true;
}


/************************************************************************************************/


bool PlayState::PostDrawUpdate(EngineCore* core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	if (framework->drawDebug)
		framework->DrawDebugHUD(dT, base.textBuffer, frameGraph);

	PresentBackBuffer(frameGraph, &core->Window);

	return true;
}


/**********************************************************************

Copyright (c) 2019 Robert May

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