#include "PlayState.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\PipelineState.h"
#include "..\graphicsutilities\TextureUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


#define DEBUGCAMERA


/************************************************************************************************/


PlayState::PlayState(
		GameFramework&	IN_framework,
		BaseState&		IN_base) : 
	FrameworkState		{ IN_framework						},

	/*
	ui					{ FlexKit::GuiSystem_Desc{}, framework.core.GetBlockMemory()	},
	uiMainGrid			{ nullptr														},
	uiSubGrid_1			{ nullptr														},
	uiSubGrid_2			{ nullptr														},
	*/

	frameID				{ 0 },
	render				{ IN_base.render												},
	base				{ IN_base														},
	eventMap			{ framework.core.GetBlockMemory()								},
	debugCameraInputMap	{ framework.core.GetBlockMemory()								},
	debugEventsInputMap	{ framework.core.GetBlockMemory()								},

	scene				{ framework.core.GetBlockMemory()								},

	debugCamera			{ }
	//thirdPersonCamera	{ CreateCharacterRig(GetMesh(GetRenderSystem(), "Flower"), &scene) }
{
	debugCamera.SetCameraAspectRatio(GetWindowAspectRatio(framework.core));
	debugCamera.TranslateWorld({0, 100, 0});

	LoadScene(framework.core, scene, "Scene1");
	LightModel = GetMesh(GetRenderSystem(), "LightModel");

	// Print Scene entity list

	auto& visibles = SceneVisibilityComponent::GetComponent();
	for( auto& entity : scene.sceneEntities )
	{
		Apply(
			*visibles[entity].entity, 
			[](StringIDView& id) 
			{
				std::cout << id.GetString() << "\n";
			});
	}

	//eventMap.MapKeyToEvent(KEYCODES::KC_W, ThirdPersonCameraRig::Forward		);
	//eventMap.MapKeyToEvent(KEYCODES::KC_A, ThirdPersonCameraRig::Left			);
	//eventMap.MapKeyToEvent(KEYCODES::KC_S, ThirdPersonCameraRig::Backward		);
	//eventMap.MapKeyToEvent(KEYCODES::KC_D, ThirdPersonCameraRig::Right			);
	eventMap.MapKeyToEvent(KEYCODES::KC_W, OCE_MoveForward	);
	eventMap.MapKeyToEvent(KEYCODES::KC_S, OCE_MoveBackward	);
	eventMap.MapKeyToEvent(KEYCODES::KC_A, OCE_MoveLeft		);
	eventMap.MapKeyToEvent(KEYCODES::KC_D, OCE_MoveRight	);


	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_C, DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA		);
	debugEventsInputMap.MapKeyToEvent(KEYCODES::KC_X, DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY	);


	framework.core.RenderSystem.PipelineStates.RegisterPSOLoader	(DRAW_SPRITE_TEXT_PSO, { nullptr, LoadSpriteTextPSO });
	framework.core.RenderSystem.PipelineStates.QueuePSOLoad		(DRAW_SPRITE_TEXT_PSO, framework.core.GetBlockMemory());

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
	iAllocator* allocator	= base.framework.core.GetBlockMemory();
	auto entities			= scene.sceneEntities;

	for (auto entity : entities)
	{
		auto entityGO = SceneVisibilityView::GetComponent()[entity].entity;
		scene.RemoveEntity(*entityGO);

		entityGO->Release();
		allocator->free(entityGO);
	}
}


/************************************************************************************************/


void PlayState::EventHandler(Event evt)
{
	eventMap.Handle(
		evt, 
		[&](auto& evt)
		{
			debugCamera.HandleEvent(evt);

			//thirdPersonCamera.HandleEvents(evt);
		});
}


/************************************************************************************************/


void PlayState::Update(EngineCore& core, UpdateDispatcher& dispatcher, double dT)
{
	//debugCamera.Yaw(dT * pi/8);
	debugCamera.Update(framework.MouseState, dT);
	//auto cameraRigUpdateTask	= UpdateThirdPersonRig		(dispatcher, thirdPersonCamera, *transformTask, *cameraUpdate, dT);
}


/************************************************************************************************/


void PlayState::DebugDraw(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
}


/************************************************************************************************/


void PlayState::PreDrawUpdate(EngineCore& core, UpdateDispatcher& Dispatcher, double dT)
{
}


/************************************************************************************************/


void PlayState::Draw(EngineCore& core, UpdateDispatcher& dispatcher, double dT, FrameGraph& frameGraph)
{
	frameGraph.Resources.AddDepthBuffer(base.depthBuffer);

	CameraHandle activeCamera = debugCamera;

	auto& transforms		= QueueTransformUpdateTask	(dispatcher);
	auto& cameras			= CameraComponent::GetComponent().QueueCameraUpdate(dispatcher);
	auto& orbitUpdate		= QueueOrbitCameraUpdateTask(dispatcher, transforms, cameras, debugCamera, framework.MouseState, dT);
	auto& cameraConstants	= MakeHeapCopy				(Camera::ConstantBuffer{}, core.GetTempMemory());
	auto& PVS				= GatherScene               (dispatcher, scene, activeCamera, core.GetTempMemory());
	auto& textureStreams	= base.streamingEngine.update(dispatcher);
    auto& renderSystem      = core.RenderSystem;

	WorldRender_Targets targets = {
		core.Window.backBuffer,
		base.depthBuffer
	};

	LighBufferDebugDraw debugDraw;
	debugDraw.constantBuffer = base.constantBuffer;
	debugDraw.renderTarget   = targets.RenderTarget;
	debugDraw.vertexBuffer	 = base.vertexBuffer;

    const SceneDescription sceneDesc = {
        scene.GetPointLights(dispatcher, core.GetTempMemory()),
        transforms,
        cameras,
        PVS
    };

	base.render.UpdateLightBuffers(dispatcher, frameGraph, activeCamera, scene, sceneDesc, core.GetTempMemory(), &debugDraw);

	ClearVertexBuffer(frameGraph, base.vertexBuffer);
	ClearVertexBuffer(frameGraph, base.textBuffer);

	ClearBackBuffer(frameGraph, targets.RenderTarget, 0.0f);
	ClearDepthBuffer(frameGraph, base.depthBuffer, 1.0f);

    FK_ASSERT(0);
	//base.render.RenderDrawabledPBR_ForwardPLUS(dispatcher, frameGraph, PVS.GetData().solid, activeCamera, targets, sceneDesc, core.GetTempMemory());

	FlexKit::DrawCollection_Desc DrawCollectionDesc{};
	DrawCollectionDesc.DepthBuffer			= targets.DepthTarget;
	DrawCollectionDesc.RenderTarget			= targets.RenderTarget;
	DrawCollectionDesc.instanceBuffer		= base.vertexBuffer;
	DrawCollectionDesc.Mesh					= LightModel;
	DrawCollectionDesc.constantBuffer		= base.constantBuffer;

	if(false)
	{
		auto cameraConstanstSize = sizeof(decltype(GetCameraConstants(activeCamera)));

        // Draw objects to represent point lights
		DrawCollection(
				frameGraph, 
				{},
				{	{ 0, (char*)&cameraConstants, cameraConstanstSize },
					{ 1, (char*)&cameraConstants, cameraConstanstSize } },
				{ },
				[&, activeCamera](auto& data)
				{
					cameraConstants = GetCameraConstants(activeCamera); // update buffer with recent data
				},
				[this, &core, activeCamera]()
				{ 
					return scene.FindPointLights(GetFrustum(activeCamera), core.GetTempMemory());
				},
				[&, &lights = PointLightComponent::GetComponent()](auto& light) -> float4x4
				{
					const float3 pos    = GetPositionW(lights[light].Position);
					const float scale   = 10;
					XMMATRIX m          =
                        DirectX::XMMatrixAffineTransformation(
						    { scale, scale, scale, 1},
						    { 0, 0, 0, 0 }, 
						    { 0, 0, 0, 1 }, 
						    { pos.x, pos.y, pos.z });

					return XMMatrixToFloat4x4(&m);
				},
				[=, heap = DescriptorHeap{
                                renderSystem,
                                renderSystem.Library.RS6CBVs4SRVs.GetDescHeap(0),
                                core.GetTempMemory() }
                ]
                (auto& defaultResources, auto& resources, Context* ctx)  // Do not setup render targets!
				{
                    ctx->SetGraphicsDescriptorTable(0, heap);
					ctx->SetRootSignature(resources.renderSystem.Library.RS6CBVs4SRVs);
					ctx->SetPipelineState(resources.GetPipelineState(FORWARDDRAWINSTANCED));
					ctx->SetPrimitiveTopology(EInputTopology::EIT_TRIANGLELIST);
				},
				DrawCollectionDesc,
				core.GetTempMemory());
	}
}


/************************************************************************************************/


void PlayState::PostDrawUpdate(EngineCore& core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& frameGraph)
{
	if (framework.drawDebug)
		framework.DrawDebugHUD(dT, base.textBuffer, frameGraph);

	PresentBackBuffer(frameGraph, &core.Window);
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
