/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "PlayState.h"
#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\PipelineState.h"
#include "..\graphicsutilities\ImageUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\PipelineState.h"


#define DEBUGCAMERA

#pragma comment(lib, "fmod64_vc.lib")



/************************************************************************************************/


void Draw3DGrid(
	FrameGraph&				FrameGraph,
	const size_t			ColumnCount,
	const size_t			RowCount,
	const float2			GridWH,
	const float4			GridColor,
	TextureHandle			RenderTarget,
	TextureHandle			DepthBuffer,
	VertexBufferHandle		VertexBuffer,
	ConstantBufferHandle	Constants,
	CameraHandle			Camera,
	iAllocator*				TempMem)
{
	LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	//  Vertical Lines on ground
	for (size_t I = 1; I < RowCount; ++I)
		Lines.push_back(
			{ { RStep  * I * GridWH.x, 0, 0 },
			GridColor,
			{ RStep  * I * GridWH.x, 0, GridWH.y },
			GridColor });


	// Horizontal lines on ground
	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 1; I < ColumnCount; ++I)
		Lines.push_back(
			{ { 0,			0, CStep  * I * GridWH.y },
			GridColor,
			{ GridWH.x,		0, CStep  * I * GridWH.y },
			GridColor });

	struct DrawGrid
	{
		FrameResourceHandle		RenderTarget;
		FrameResourceHandle		DepthBuffer;

		size_t					VertexBufferOffset;
		size_t					VertexCount;
		VertexBufferHandle		VertexBuffer;
		ConstantBufferHandle	CB;

		size_t CameraConstantsOffset;
		size_t LocalConstantsOffset;

		FlexKit::DesciptorHeap	Heap; // Null Filled
	};

	struct VertexLayout
	{
		float4 POS;
		float2 UV;
		float4 Color;
	};

	FrameGraph.AddNode<DrawGrid>(0,
		[&](FrameGraphNodeBuilder& Builder, auto& Data)
	{
		Data.RenderTarget	= Builder.WriteRenderTarget(FrameGraph.Resources.RenderSystem->GetTag(RenderTarget));
		Data.DepthBuffer	= Builder.WriteDepthBuffer(FrameGraph.Resources.RenderSystem->GetTag(DepthBuffer));
		Data.CB				= Constants;

		Data.CameraConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
		PushConstantBufferData(
			GetCameraConstantBuffer(Camera),
			Constants,
			FrameGraph.Resources);

		Data.Heap.Init(
			FrameGraph.Resources.RenderSystem,
			FrameGraph.Resources.RenderSystem->Library.RS4CBVs4SRVs.GetDescHeap(0),
			TempMem);
		Data.Heap.NullFill(FrameGraph.Resources.RenderSystem);

		Drawable::VConsantsLayout DrawableConstants;
		DrawableConstants.Transform = DirectX::XMMatrixIdentity();

		Data.LocalConstantsOffset = BeginNewConstantBuffer(Constants, FrameGraph.Resources);
		PushConstantBufferData(
			DrawableConstants,
			Constants,
			FrameGraph.Resources);

		Data.VertexBuffer = VertexBuffer;
		Data.VertexBufferOffset = FrameGraph.Resources.GetVertexBufferOffset(VertexBuffer);

		// Fill Vertex Buffer Section
		for (auto& LineSegment : Lines)
		{
			VertexLayout Vertex;
			Vertex.POS = float4(LineSegment.A, 1);
			Vertex.Color = float4(LineSegment.AColour, 1) * float4 { 1.0f, 0.0f, 0.0f, 1.0f };
			Vertex.UV = { 0.0f, 0.0f };

			PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);

			Vertex.POS = float4(LineSegment.B, 1);
			Vertex.Color = float4(LineSegment.BColour, 1) * float4 { 0.0f, 1.0f, 0.0f, 1.0f };
			Vertex.UV = { 1.0f, 1.0f };

			PushVertex(Vertex, VertexBuffer, FrameGraph.Resources);
		}

		Data.VertexCount = Lines.size() * 2;

	},
		[](auto& Data, const FrameResources& Resources, Context* Ctx)
	{
		Ctx->SetRootSignature(Resources.RenderSystem->Library.RS4CBVs4SRVs);
		Ctx->SetPipelineState(Resources.GetPipelineState(DRAW_LINE3D_PSO));

		Ctx->SetScissorAndViewports({ Resources.GetRenderTarget(Data.RenderTarget) });
		Ctx->SetRenderTargets(
			{ (DescHeapPOS)Resources.GetRenderTargetObject(Data.RenderTarget) }, false,
			(DescHeapPOS)Resources.GetRenderTargetObject(Data.DepthBuffer));

		Ctx->SetPrimitiveTopology(EInputTopology::EIT_LINE);
		Ctx->SetVertexBuffers(VertexBufferList{ { Data.VertexBuffer, sizeof(VertexLayout), (UINT)Data.VertexBufferOffset } });

		Ctx->SetGraphicsDescriptorTable(0, Data.Heap);
		Ctx->SetGraphicsConstantBufferView(1, Data.CB, Data.CameraConstantsOffset);
		Ctx->SetGraphicsConstantBufferView(2, Data.CB, Data.LocalConstantsOffset);

		Ctx->Draw(Data.VertexCount, 0);
	});
}



void DrawGame(
	double					dt,
	float					AspectRatio,
	Game&					Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	TextureHandle			DepthBuffer,
	CameraHandle			Camera,
	iAllocator*				TempMem)
{
	const size_t ColumnCount = Grid.WH[1];
	const size_t RowCount 	 = Grid.WH[0];
	const float2 GridWH		 = { 50.0, 50.0 };
	const float4 GridColor	 = { 1, 1, 1, 1 };


	struct InstanceConstantLayout
	{
		float4x4 WorldTransform;
	};

	Drawable::VConsantsLayout Constants_1 =
	{
		Drawable::MaterialProperties(),
		DirectX::XMMatrixIdentity()
	};

	auto CameraConstants = GetCameraConstantBuffer(Camera);

	DrawCollection_Desc DrawDesc =
	{
		TriMeshHandle(0),
		RenderTarget,
		DepthBuffer,
		VertexBuffer,
		ConstantBuffer,

		FORWARDDRAWINSTANCED,

		sizeof(InstanceConstantLayout),

		{
			sizeof(CameraConstants),
			sizeof(Drawable::VConsantsLayout)
		},

		{
			(char*)&CameraConstants,
			(char*)&Constants_1
		}
	};


	Draw3DGrid(
		FrameGraph,
		ColumnCount,
		RowCount,
		GridWH,
		GridColor,
		RenderTarget,
		DepthBuffer,
		VertexBuffer,
		ConstantBuffer,
		Camera,
		TempMem);

	DrawCollection(
		FrameGraph,
		Grid.Players,
		[&](auto& Player) -> InstanceConstantLayout
		{
			float3 Position = {
				(Player.XY[0] + Player.Offset.x) * GridWH.x / ColumnCount,
				0.0,
				(Player.XY[1] + Player.Offset.y) * GridWH.y / RowCount };

			auto Transform = TranslationMatrix(Position);

			return  { Transform };
		},
		DrawDesc,
		TempMem
		);

	DrawCollection(
		FrameGraph,
		Grid.Objects,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0]) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1]) * GridWH.y / RowCount };

			auto Transform = TranslationMatrix(Position);

			return  { Transform };
		},
		DrawDesc,
		TempMem
	);

	DrawCollection(
		FrameGraph,
		Grid.Spaces,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0]) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1]) * GridWH.y / RowCount };

			return  { TranslationMatrix(Position) };
		},
		DrawDesc,
		TempMem
	);

	DrawCollection(
		FrameGraph,
		Grid.Bombs,
		[&](auto& Entity) -> InstanceConstantLayout
		{
			float3 Position = {
				(Entity.XY[0] + Entity.Offset.x) * GridWH.x / ColumnCount,
				0.0,
				(Entity.XY[1] + Entity.Offset.y) * GridWH.y / RowCount };

			return  { TranslationMatrix(Position) };
		},
		DrawDesc,
		TempMem
	);
}



/************************************************************************************************/


FrameSnapshot::FrameSnapshot(Game* IN_Game, EventList* IN_FrameEvents, size_t IN_FrameID, iAllocator* IN_Memory) :
	FrameID{ IN_FrameID },
	FrameCopy{ IN_Memory },
	Memory{ IN_Memory },
	FrameEvents{ IN_Memory }
{
	if (!IN_Memory)
		return;

	FrameEvents = *IN_FrameEvents;
	FrameCopy = *IN_Game;
}


/************************************************************************************************/


FrameSnapshot::~FrameSnapshot()
{
	FrameCopy.Release();
}


/************************************************************************************************/


void FrameSnapshot::Restore(Game* out)
{
}


/************************************************************************************************/


PlayState::PlayState(
		GameFramework*			IN_Framework,
		BaseState*				Base) : 
	FrameworkState		{ IN_Framework						},
	FrameEvents			{ Framework->Core->GetBlockMemory() },
	FrameID				{ 0 },
	Sound				{ Framework->Core->Threads	},
	Render				{ &Base->Render				},
	LocalGame			{ Framework->Core->GetBlockMemory() },
	Player1_Handler		{ LocalGame, Framework->Core->GetBlockMemory() },
	Player2_Handler		{ LocalGame, Framework->Core->GetBlockMemory() },
	GameInPlay			{ true					},
	UseDebugCamera		{ false					},

	ConstantBuffer		{ Base->ConstantBuffer	}, 
	DepthBuffer			{ Base->DepthBuffer		},
	TextBuffer			{ Base->TextBuffer		},
	VertexBuffer		{ Base->VertexBuffer	},

	EventMap			{ Framework->Core->GetBlockMemory() },
	DebugCameraInputMap	{ Framework->Core->GetBlockMemory() },
	DebugEventsInputMap	{ Framework->Core->GetBlockMemory() },
	Puppet				{ CreatePlayerPuppet(Scene)			},
	Scene
						{
							Framework->Core->RenderSystem,
							Framework->Core->GetBlockMemory(),
							Framework->Core->GetTempMemory()
						}
{
	Player1_Handler.SetActive(LocalGame.CreatePlayer({ 11, 11 }));
	Player1_Handler.SetPlayerCameraAspectRatio(GetWindowAspectRatio(Framework->Core));

	LocalGame.CreateGridObject({ 10, 5 });

	OrbitCamera.SetCameraAspectRatio(GetWindowAspectRatio(Framework->Core));
	OrbitCamera.TranslateWorld({ 0, 2, -5 });
	OrbitCamera.Yaw(pi);


	EventMap.MapKeyToEvent(KEYCODES::KC_W, PLAYER_EVENTS::PLAYER_UP);
	EventMap.MapKeyToEvent(KEYCODES::KC_A, PLAYER_EVENTS::PLAYER_LEFT);
	EventMap.MapKeyToEvent(KEYCODES::KC_S, PLAYER_EVENTS::PLAYER_DOWN);
	EventMap.MapKeyToEvent(KEYCODES::KC_D, PLAYER_EVENTS::PLAYER_RIGHT);
	EventMap.MapKeyToEvent(KEYCODES::KC_Q, PLAYER_EVENTS::PLAYER_ROTATE_LEFT);
	EventMap.MapKeyToEvent(KEYCODES::KC_E, PLAYER_EVENTS::PLAYER_ROTATE_RIGHT);
	EventMap.MapKeyToEvent(KEYCODES::KC_LEFTSHIFT, PLAYER_EVENTS::PLAYER_HOLD);
	EventMap.MapKeyToEvent(KEYCODES::KC_SPACE, PLAYER_EVENTS::PLAYER_ACTION1);

	// Debug Orbit Camera
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_I, PLAYER_EVENTS::DEBUG_PLAYER_UP);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_J, PLAYER_EVENTS::DEBUG_PLAYER_LEFT);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_K, PLAYER_EVENTS::DEBUG_PLAYER_DOWN);
	DebugCameraInputMap.MapKeyToEvent(KEYCODES::KC_L, PLAYER_EVENTS::DEBUG_PLAYER_RIGHT);


	// Debug Events, are not stored in Frame cache
	DebugEventsInputMap.MapKeyToEvent(KEYCODES::KC_C, DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA);
	DebugEventsInputMap.MapKeyToEvent(KEYCODES::KC_X, DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY);

	Framework->Core->RenderSystem.PipelineStates.RegisterPSOLoader(DRAW_SPRITE_TEXT_PSO, LoadSpriteTextPSO);
	Framework->Core->RenderSystem.PipelineStates.QueuePSOLoad(DRAW_SPRITE_TEXT_PSO);
}


/************************************************************************************************/


PlayState::~PlayState()
{
	// TODO: Make this not stoopid 
	Framework->Core->GetBlockMemory().free(this);
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	Event Remapped;
	if (DebugEventsInputMap.Map(evt, Remapped))
	{
		if (Remapped.InputSource == Event::Keyboard &&
			Remapped.Action		== Event::Pressed)
		{
			switch ((DEBUG_EVENTS)Remapped.mData1.mINT[0])
			{
			case DEBUG_EVENTS::TOGGLE_DEBUG_CAMERA:
				UseDebugCamera	= !UseDebugCamera;
				break;
			case DEBUG_EVENTS::TOGGLE_DEBUG_OVERLAY:
				Framework->DrawDebug = !Framework->DrawDebug;
				break;
			default:
				break;
			}
		}
	}
	else
		FrameEvents.push_back(evt);

	return true;
}


/************************************************************************************************/


bool PlayState::Update(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT)
{
	FrameCache.push_back(FrameSnapshot(&LocalGame, &FrameEvents, ++FrameID, Core->GetBlockMemory()));

	for (auto& evt : FrameEvents)
	{
		Event Remapped;
		if (Player1_Handler.Enabled && EventMap.Map(evt, Remapped))
			Player1_Handler.Handle(Remapped);

		//if (Player2_Handler.Enabled)
		//	Player2_Handler.Handle(evt);
		if (DebugCameraInputMap.Map(evt, Remapped))
			OrbitCamera.EventHandler(Remapped);
	}

	FrameEvents.clear();

	// Check if any players Died
	for (auto& P : LocalGame.Players)
		if (LocalGame.IsCellDestroyed(P.XY))
			Framework->PopState();

	LocalGame.Update(dT, Core->GetTempMemory());

	if (Player1_Handler.Enabled)
		Player1_Handler.Update(dT);

	if (Player2_Handler.Enabled)
		Player2_Handler.Update(dT);


	float HorizontalMouseMovement	= float(Framework->MouseState.dPos[0]) / GetWindowWH(Framework->Core)[0];
	float VerticalMouseMovement		= float(Framework->MouseState.dPos[1]) / GetWindowWH(Framework->Core)[1];

	Framework->MouseState.Normalized_dPos = { HorizontalMouseMovement, VerticalMouseMovement };

	OrbitCamera.Yaw(Framework->MouseState.Normalized_dPos[0]);
	OrbitCamera.Pitch(Framework->MouseState.Normalized_dPos[1]);

	OrbitCamera.Update(dT);
	Puppet.Update(dT);

	QueueSoundUpdate(Dispatcher, &Sound);
	auto TransformTask = QueueTransformUpdateTask(Dispatcher);
	QueueCameraUpdate(Dispatcher, TransformTask);

	return false;
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


bool PlayState::Draw(EngineCore* Core, UpdateDispatcher& Dispatcher, double dt, FrameGraph& FrameGraph)
{
	FrameGraph.Resources.AddDepthBuffer(DepthBuffer);

	WorldRender_Targets Targets = {
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer
	};

	PVS	Drawables_Solid(Core->GetTempMemory());
	PVS	Drawables_Transparent(Core->GetTempMemory());

	ClearVertexBuffer(FrameGraph, VertexBuffer);
	ClearVertexBuffer(FrameGraph, TextBuffer);

	ClearBackBuffer(FrameGraph, 0.0f);
	ClearDepthBuffer(FrameGraph, DepthBuffer, 1.0f);


#ifndef DEBUGCAMERA
#if 1
	DrawGameGrid_Debug(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		Core->GetTempMemory()
	);
#else
	DrawGame(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer,
		OrbitCamera,
		Core->GetTempMemory());
#endif
#else
	PVS Drawables{ Core->GetTempMemory() };
	PVS TransparentDrawables{ Core->GetTempMemory() };

	CameraHandle ActiveCamera = UseDebugCamera ? (CameraHandle)OrbitCamera : (CameraHandle)Player1_Handler.GameCamera;

	GetGraphicScenePVS(Scene, ActiveCamera, &Drawables, &TransparentDrawables);


	if(Framework->DrawDebug)
	{
		DrawGameGrid_Debug(
			dt,
			GetWindowAspectRatio(Core),
			LocalGame,
			FrameGraph,
			ConstantBuffer,
			VertexBuffer,
			GetCurrentBackBuffer(&Core->Window),
			Core->GetTempMemory()
		);
	}

	DrawGame(
		dt,
		GetWindowAspectRatio(Core),
		LocalGame,
		FrameGraph,
		ConstantBuffer,
		VertexBuffer,
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer,
		ActiveCamera,
		Core->GetTempMemory());


	Render->DefaultRender(
		Drawables,
		ActiveCamera,
		Targets,
		FrameGraph,
		Core->GetTempMemory());

#endif


	return true;
}


/************************************************************************************************/


bool PlayState::PostDrawUpdate(EngineCore* Core, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph)
{
	if (Framework->DrawDebugStats)
		Framework->DrawDebugHUD(dT, TextBuffer, Graph);

	PresentBackBuffer(Graph, &Core->Window);
	return true;
}


/************************************************************************************************/


void PlayState::BindPlayer1()
{

}


/************************************************************************************************/


void PlayState::BindPlayer2()
{

}


/************************************************************************************************/


void PlayState::ReleasePlayer1()
{
}


/************************************************************************************************/


void PlayState::ReleasePlayer2()
{

}


/************************************************************************************************/
