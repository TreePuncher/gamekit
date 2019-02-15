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

			//thirdPersonCamera.HandleEvents(evt);
		});

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Core, UpdateDispatcher& dispatcher, double dT)
{
	//debugCamera.Yaw(dT * pi/8);

	auto transformTask			= QueueTransformUpdateTask	(dispatcher);
	auto cameraUpdate			= QueueCameraUpdate			(dispatcher, transformTask);
	auto sceneUpdate			= scene.Update				(dispatcher, transformTask);
	auto orbitUpdate			= QueueOrbitCameraUpdateTask(dispatcher, transformTask, cameraUpdate, debugCamera, framework->MouseState, dT);

	//auto cameraRigUpdateTask	= UpdateThirdPersonRig		(dispatcher, thirdPersonCamera, *transformTask, *cameraUpdate, dT);

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


bool PlayState::Draw(EngineCore* core, UpdateDispatcher& dispatcher, double dt, FrameGraph& frameGraph)
{
	frameGraph.Resources.AddDepthBuffer(base.depthBuffer);


	FlexKit::WorldRender_Targets targets = {
		GetCurrentBackBuffer(&core->Window),
		base.depthBuffer
	};

	PVS	solidDrawables			(core->GetTempMemory());
	PVS	transparentDrawables	(core->GetTempMemory());

	ClearVertexBuffer	(frameGraph, base.vertexBuffer);
	ClearVertexBuffer	(frameGraph, base.textBuffer);

	ClearBackBuffer		(frameGraph, 0.0f);
	ClearDepthBuffer	(frameGraph, base.depthBuffer, 1.0f);

	CameraHandle activeCamera	= (CameraHandle)debugCamera;
	auto cameraConstants		= GetCameraConstantBuffer(activeCamera);

	GetGraphicScenePVS(scene, activeCamera, &solidDrawables, &transparentDrawables);

	LighBufferDebugDraw debugDraw;
	debugDraw.constantBuffer = base.constantBuffer;
	debugDraw.renderTarget   = targets.RenderTarget;
	debugDraw.vertexBuffer	 = base.vertexBuffer;

	base.render.updateLightBuffers(activeCamera, scene, frameGraph, core->GetTempMemory());
	base.render.DefaultRender(solidDrawables, activeCamera, targets, frameGraph, SceneDescription{ scene.GetPointLightCount() }, core->GetTempMemory());

	FlexKit::DrawCollection_Desc DrawCollectionDesc{};

	DrawCollectionDesc.ConstantData[0]		= reinterpret_cast<char*>(&cameraConstants);
	DrawCollectionDesc.ConstantsSize[0]		= sizeof(cameraConstants);
	DrawCollectionDesc.ConstantData[1]		= reinterpret_cast<char*>(&cameraConstants);
	DrawCollectionDesc.ConstantsSize[1]		= sizeof(cameraConstants);
	DrawCollectionDesc.DepthBuffer			= targets.DepthTarget;
	DrawCollectionDesc.RenderTarget			= targets.RenderTarget;
	DrawCollectionDesc.VertexBuffer			= base.vertexBuffer;
	DrawCollectionDesc.Mesh					= LightModel;
	DrawCollectionDesc.Constants			= base.constantBuffer;
	DrawCollectionDesc.PSO					= FORWARDDRAWINSTANCED;
	DrawCollectionDesc.InstanceElementSize	= sizeof(float4x4);

	auto f		= GetFrustum(activeCamera);
	auto lights = scene.FindPointLights(f, core->GetTempMemory());

	if(false)
	{
		DrawCollection(
				frameGraph, 
				lights,
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