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



/************************************************************************************************/


PlayState::PlayState(GameFramework* framework) :
	FrameworkState	(framework),
	Input			(Framework),
	TPC				(Framework, Input, Framework->Core->Cameras),
	OrbitCameras	(Framework, Input),
	Render			(
		 Framework->Core->GetTempMemory(), 
		 Framework->Core->RenderSystem, 
		 Framework->Core->Geometry),
	Scene		(
		 Framework->Core->RenderSystem, 
		&Framework->Core->Assets, 
		 Framework->Core->Nodes, 
		 Framework->Core->Geometry, 
		 Framework->Core->GetBlockMemory(), 
		 Framework->Core->GetTempMemory()),
	Drawables	(&Scene, Framework->Core->Nodes),
	Lights		(&Scene, Framework->Core->Nodes),
	Physics		(&Framework->Core->Physics, Framework->Core->Nodes, Framework->Core->GetBlockMemory()),
	Test		(0),
	VertexBuffer	(Framework->Core->RenderSystem.CreateVertexBuffer(8096 * 4, false)),
	ConstantBuffer	(Framework->Core->RenderSystem.CreateConstantBuffer(8096, false))
{
	Framework->ActivePhysicsScene	= &Physics;
	Framework->ActiveScene			= &Scene;

	bool res = LoadScene(
		Framework->Core->RenderSystem, 
		Framework->Core->Nodes, 
		&Framework->Core->Assets, 
		&Framework->Core->Geometry, 
		201, 
		&Scene, 
		Framework->Core->GetTempMemory());

	InitiateGameObject(
		Player,
			Physics.CreateCharacterController({0, 10, 0}, 5, 5),
			//CreateThirdPersonCamera(&State->TPC, Framework->ActiveCamera));
			CreateCameraComponent(Framework->Core->Cameras, GetWindowAspectRatio(Framework->Core), 0.01f, 10000.0f, InvalidComponentHandle),
			CreateOrbitCamera(OrbitCameras, &Framework->Core->Cameras, 100.0f));

	SetWorldPosition(Player, { 0, 15, 60 });
	OffsetYawNode	(Player, { 0.0f, 5, 0.0f });
	SetCameraOffset	(Player, {0, 30, 60});


	InitiateGameObject(	FloorObject,
		Physics.CreateStaticBoxCollider({ 1000, 3, 1000 }, { 0, -3, 0 }));

	DepthBuffer = (Framework->Core->RenderSystem.CreateDepthBuffer({ 1920, 1080 }, true));
	Framework->Core->RenderSystem.SetTag(DepthBuffer, GetCRCGUID(DEPTHBUFFER));
}


/************************************************************************************************/


bool PlayState::EventHandler(Event evt)
{
	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				Input.KeyState.Shield   = true;
				break;
			case KC_W:
				Input.KeyState.Forward  = true;
				break;
			case KC_S:
				Input.KeyState.Backward = true;
				break;
			case KC_A:
				Input.KeyState.Left     = true;
				break;
			case KC_D:
				Input.KeyState.Right    = true;
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_R:
				//this->Framework->Core->RenderSystem.QueuePSOLoad(FORWARDDRAW);
				break;
			case KC_E:
				Input.KeyState.Shield   = false;
				break;
			case KC_W:
				Input.KeyState.Forward  = false;
				break;
			case KC_S:
				Input.KeyState.Backward = false;
				break;
			case KC_A:
				Input.KeyState.Left     = false;
				break;
			case KC_D: 
				Input.KeyState.Right    = false;
				break;
			}
		}	break;
		}
	}
	return true;
}


/************************************************************************************************/

float3 CameraPOS = {8000,1000,8000};

bool PlayState::Update(EngineCore* Engine, double dT)
{
	//printfloat3(GetWorldPosition(Player));
	//printf("\n");

	//float MovementFactor			= 50.0f;

	//ThisState->Model.PlayerInputs[0].FrameID++;
	//ThisState->Model.PlayerInputs[0].MouseInput		= { HorizontalMouseMovement, VerticalMouseMovement };
	//ThisState->Model.PlayerInputs[0].KeyboardInput	= ThisState->Input;

	Input.Update(dT, Framework->MouseState, Framework );
	//Yaw(Player, dT * Framework->MouseState.Normalized_dPos[0]);

	double T = Framework->TimeRunning;
	double CosT = (float)cos(T);
	double SinT = (float)sin(T);

	float Begin	= 0.0f;
	float End	= 60.0f;
	float IaR	= 10000 * (1 + (float)cos(T * 6)) / 2;
	
	//Translate(ThisState->Player, float3{ 0, 100, 0 } * dT);
	
	const float MoveRate = 10;


	//SetPositionW(Framework->Core->Nodes, Framework->DebugCamera.Node, CameraPOS);
	//Yaw(Framework->Core->Nodes, Framework->DebugCamera.Node, pi * dT);
	/*

#if 0
	if (Input.KeyState.Forward)
		//Translate(Player, Forward * dT * MoveRate);
		Translate(Player, float3{0, float(dT) * MoveRate, 0});
#else

	//auto Forward	= GetForwardVector(Player);
	//auto Left		= GetLeftVector(Player);

	auto Q			= GetCameraOrientation(Player);
	auto Forward	= Q * float3( 0,  0, -1);
	auto Left		= Q * float3( -1,  0,  0);

	if (Input.KeyState.Forward)
		Translate(Player, Forward * dT * MoveRate);
	if (Input.KeyState.Backward)
		Translate(Player, Forward * dT * -MoveRate);
	if (Input.KeyState.Left)
		Translate(Player, Left * dT * MoveRate);
	if (Input.KeyState.Right)
		Translate(Player, Left * dT * -MoveRate);
#endif

	//Translate(Player, dT * float3{0, -98.0f, 0});
	*/
	//Yaw(Player, pi * dT/ 2);

	OrbitCameras.Update(dT);
	Physics.UpdateSystem(dT);
	TPC.Update(dT);

	//UpdateTransforms(Engine->Nodes);
	//Engine->Cameras.Update(0.0);

	return false;
}


/************************************************************************************************/

bool PlayState::DebugDraw(EngineCore* Core, double dT)
{
	//PushCapsule_Wireframe(Framework->Immediate, Core->GetTempMemory(), GetWorldPosition(Player), 5, 10, GREEN);
	//Physics.DebugDraw	(Framework->Immediate, Core->GetTempMemory());
	//Drawables.DrawDebug	(Framework->Immediate, Core->Nodes, Core->GetTempMemory());

	return true;
}


/************************************************************************************************/


bool PlayState::PreDrawUpdate(EngineCore* Core, double dT)
{
	Physics.UpdateSystem_PreDraw(dT);

	if(Framework->DrawPhysicsDebug)
	{
		//auto PlayerPOS = ThisState->Model.Players[0].PlayerCTR.Pos;
	}

	//ThisState->Model.UpdateAnimations(ThisState->Framework, DT);

#if 0

	//UpdateCamera(Framework->Core->RenderSystem, Framework->Core->Nodes, &Framework->DebugCamera, dT);
	//auto Q = GetOrientation(Framework->Core->Nodes, Framework->DebugCamera.Node);

	LineSegments Lines(Engine->TempAllocator);
	LineSegment Line;
	Line.A       = float3(0, 0, 0) + CameraPOS;
	Line.B       = float3(0, 20, 0) + CameraPOS;
	Line.AColour = float3(1, 1, 0);
	Line.BColour = float3(1, 1, 0);
	Lines.push_back(Line);
	
	Line.B       = Q * float3(0, 0, -10000) + CameraPOS;
	Lines.push_back(Line);

	PushLineSet3D(Framework->Immediate, Lines);

#endif

#if 0
	// Ray Cast Tests

	double T	= Framework->TimeRunning;
	double CosT = (float)cos(T);
	double SinT = (float)sin(T);

	float Begin = 00.0f;
	float End	= 60.0f;
	float IaR	= 10000 * (1 + (float)cos(T * 6)) / 2;

	Quaternion Q2	= GetCameraOrientation(Player);
	float3 Origin	= GetWorldPosition(Player);
	float3 D		= Q2 * float3(0, 0, -1);
	float3 Color	= RED;

	RaySet Rays(Engine->GetTempMemory());
	Ray R = {Origin, D};
	Rays.push_back(R);
	auto results = Drawables.RayCastOBB(Rays, Engine->GetBlockMemory(), Engine->Nodes);

	if (results.size()) {
		Color = BLUE;
		
		Yaw(TestObject, dT * pi);
		SetDrawableColor(TestObject, Grey(CosT));
		//SetWorldPosition(ThisState->TestObject, float3( 100.0f * CosT, 60.0f, SinT * 100));

		SetLightRadius(TestObject, 100 + IaR);
		SetLightIntensity(TestObject, 100 + IaR);
		SetWorldPosition(TestObject, Origin + results.back().D * D);
		SetVisibility(TestObject, true);
	}
	else
	{
		SetLightIntensity(TestObject, 0);
		SetVisibility(TestObject, false);
	}

	LineSegment Line2;
	Line2.A       = Origin + (Q2 * float3(10, 0, 0));
	Line2.AColour = Color;
	Line2.B       = Origin + D * 1000;
	Line2.BColour = Color;
	LineSegments Lines2(Engine->GetTempMemory());
	Lines2.push_back(Line2);

	PushLineSet3D(Framework->Immediate, Lines2);

#endif

	return false;
}


/************************************************************************************************/


PlayState::~PlayState()
{
	FloorObject.Release();
	TestObject.Release();

	for(auto& GO : CubeObjects)
		GO.Release();

	Player.Release();
	TestObject.Release();

	Physics.Release();

	Framework->Core->RenderSystem.ReleaseVB(VertexBuffer);
	Framework->Core->RenderSystem.ReleaseCB(ConstantBuffer);

	ReleaseGraphicScene(&Scene);
	Framework->Core->GetBlockMemory().free(this);
}


/************************************************************************************************/


bool PlayState::Draw(EngineCore* Core, double dt, FrameGraph& FrameGraph)
{
	FrameGraph.Resources.AddDepthBuffer(DepthBuffer);
	WorldRender_Targets Targets = {
		GetCurrentBackBuffer(&Core->Window),
		DepthBuffer
	};

	PVS	Drawables_Solid(Core->GetTempMemory());
	PVS	Drawables_Transparent(Core->GetTempMemory());

	GetGraphicScenePVS(Scene, GetCamera_ptr(Player), &Drawables_Solid, &Drawables_Transparent);

	ClearBackBuffer		(FrameGraph, 0.0f);
	ClearVertexBuffer	(FrameGraph, VertexBuffer);

	Render.DefaultRender(Drawables_Solid, GetCamera_ref(Player), Core->Nodes, Targets, FrameGraph, Core->GetTempMemory());

	//DrawShapes(EPIPELINESTATES::Draw_PSO, FrameGraph, VertexBuffer, ConstantBuffer, GetCurrentBackBuffer(&Core->Window), Core->GetTempMemory(),
	//	RectangleShape	({0.01f, 0.01f}, { 0.98f, 0.98f }, float4(0.1f, 0.1f, 0.1f, 0.0f)),
	//	CircleShape		({0.5f, 0.5f},	 0.2f, float4(1.0f,0.0f,0.0f,1.0f) ),
	//	CircleShape		({0.5f, 0.5f},	 0.1f, float4(1.0f,1.0f,1.0f,1.0f) ));



	PresentBackBuffer	(FrameGraph, &Core->Window);

	return true;
}


/************************************************************************************************/


TriMeshHandle CreateCube(RenderSystem* RS, GeometryTable* GT, iAllocator* Memory, float R, GUID_t MeshID)
{
	FlexKit::VertexBufferView* Views[3];

	// Index Buffer
	{
		Views[0] = FlexKit::CreateVertexBufferView((byte*)Memory->_aligned_malloc(4096), 4096);
		Views[0]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_INDEX, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R16);

		for(uint16_t I = 0; I < 36; ++I)
			Views[0]->Push(I);

		FK_ASSERT( Views[0]->End() );
	}

	// Vertex Buffer
	{
		Views[1] = FlexKit::CreateVertexBufferView((byte*)Memory->_aligned_malloc(4096), 4096);
		Views[1]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32);

		float3 TopFarLeft	= { -R,  R, -R };
		float3 TopFarRight  = { -R,  R, -R };
		float3 TopNearLeft  = { -R,  R,  R };
		float3 TopNearRight = {  R,  R,  R };

		float3 BottomFarLeft   = { -R,  -R, -R };
		float3 BottomFarRight  = { -R,  -R, -R };
		float3 BottomNearLeft  = { -R,  -R,  R };
		float3 BottomNearRight = {  R,  -R,  R };

		// Top Plane
		Views[1]->Push(TopFarLeft); 
		Views[1]->Push(TopFarRight);
		Views[1]->Push(TopNearRight);

		Views[1]->Push(TopNearRight);
		Views[1]->Push(TopNearLeft);
		Views[1]->Push(TopFarLeft);


		// Bottom Plane
		Views[1]->Push(BottomFarLeft);
		Views[1]->Push(BottomNearRight);
		Views[1]->Push(BottomFarRight);

		Views[1]->Push(BottomNearRight);
		Views[1]->Push(BottomFarLeft);
		Views[1]->Push(BottomNearLeft);

		// Left Plane
		Views[1]->Push(TopFarLeft);
		Views[1]->Push(TopNearLeft);
		Views[1]->Push(BottomNearLeft);

		Views[1]->Push(TopFarLeft);
		Views[1]->Push(BottomNearLeft);
		Views[1]->Push(BottomFarLeft);
		
		// Right Plane
		Views[1]->Push(TopFarRight);
		Views[1]->Push(BottomNearRight);
		Views[1]->Push(TopNearRight);

		Views[1]->Push(TopFarRight);
		Views[1]->Push(BottomFarRight);
		Views[1]->Push(TopNearRight);

		// Near Plane
		Views[1]->Push(TopNearLeft);
		Views[1]->Push(TopNearRight);
		Views[1]->Push(BottomNearRight);

		Views[1]->Push(TopNearLeft);
		Views[1]->Push(BottomNearRight);
		Views[1]->Push(BottomNearLeft);

		// FarPlane
		Views[1]->Push(TopFarLeft);
		Views[1]->Push(TopFarRight);
		Views[1]->Push(BottomFarRight);
		
		Views[1]->Push(TopFarLeft);
		Views[1]->Push(BottomFarRight);
		Views[1]->Push(BottomFarLeft);

		FK_ASSERT( Views[1]->End() );
	}
	// Normal Buffer
	{
		Views[2] = FlexKit::CreateVertexBufferView((byte*)Memory->_aligned_malloc(4096), 4096);
		Views[2]->Begin(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL, VERTEXBUFFER_FORMAT::VERTEXBUFFER_FORMAT_R32G32B32);

		float3 TopPlane		= {  0,  1,  0 };
		float3 BottomPlane  = {  0, -1,  0 };
		float3 LeftPlane	= { -1,  0,  0 };
		float3 RightPlane	= {  1,  0,  1 };
		float3 NearPlane	= {  0,  0,  1 };
		float3 FarPlane		= {  0,  0, -1 };

		// Top Plane
		Views[2]->Push(TopPlane);
		Views[2]->Push(TopPlane);
		Views[2]->Push(TopPlane);

		Views[2]->Push(TopPlane);
		Views[2]->Push(TopPlane);
		Views[2]->Push(TopPlane);

		// Bottom Plane
		Views[2]->Push(BottomPlane);
		Views[2]->Push(BottomPlane);
		Views[2]->Push(BottomPlane);

		Views[2]->Push(BottomPlane);
		Views[2]->Push(BottomPlane);
		Views[2]->Push(BottomPlane);

		// Left Plane
		Views[2]->Push(LeftPlane);
		Views[2]->Push(LeftPlane);
		Views[2]->Push(LeftPlane);

		Views[2]->Push(LeftPlane);
		Views[2]->Push(LeftPlane);
		Views[2]->Push(LeftPlane);

		// Right Plane
		Views[2]->Push(RightPlane);
		Views[2]->Push(RightPlane);
		Views[2]->Push(RightPlane);

		Views[2]->Push(RightPlane);
		Views[2]->Push(RightPlane);
		Views[2]->Push(RightPlane);

		// Near Plane
		Views[2]->Push(NearPlane);
		Views[2]->Push(NearPlane);
		Views[2]->Push(NearPlane);

		Views[2]->Push(NearPlane);
		Views[2]->Push(NearPlane);
		Views[2]->Push(NearPlane);

		// FarPlane
		Views[2]->Push(FarPlane);
		Views[2]->Push(FarPlane);
		Views[2]->Push(FarPlane);

		Views[2]->Push(FarPlane);
		Views[2]->Push(FarPlane);
		Views[2]->Push(FarPlane);

		FK_ASSERT( Views[2]->End() );
	}

	Mesh_Description Desc;
	Desc.Buffers		= Views;
	Desc.BufferCount	= 3;
	Desc.IndexBuffer	= 0;

	auto MeshHandle		= BuildMesh(RS, GT, &Desc, MeshID);

	for(auto V : Views)
		Memory->_aligned_free(V);

	return MeshHandle;
}


void CreateIntersectionTest(PlayState* State, FlexKit::GameFramework* Framework)
{
	FK_ASSERT(LoadScene(Framework->Core->RenderSystem, Framework->Core->Nodes, &Framework->Core->Assets, &Framework->Core->Geometry, 201, &State->Scene, Framework->Core->GetTempMemory()), "FAILED TO LOAD!\n");

	InitiateGameObject( 
		State->FloorObject,
			State->Physics.CreateStaticBoxCollider({1000, 3, 1000}, {0, -3, 0}));

	//InitiateGameObject(
	//	State->Player,
	//		State->Physics.CreateCharacterController({0, 10, 0}, 10, 5.0f),
	//		CreateCameraComponent(Framework->Core->Cameras, GetWindowAspectRatio(Framework->Core), 0.01f, 10000.0f, InvalidComponentHandle),
			//CreateThirdPersonCamera(&State->TPC));
	//		CreateOrbitCamera(State->OrbitCameras, , 10));

	//Yaw(State->Player, pi/100);


	SetWorldPosition(State->Player, {0, 30, 50});
	//OffsetYawNode(State->Player, { 0.0f, 5, 0.0f });
	//SetCameraOffset(State->Player, {0, 30, 30});

	for(size_t I = 0; I < 0; ++I){
		InitiateGameObject( 
			State->CubeObjects[I],
				CreateEnityComponent(&State->Drawables, "Flower"),
				//CreateLightComponent(&State->Lights, {1, -1, -1}, 1, 1000),
				State->Physics.CreateCubeComponent(
					{ 0, 10.0f * I, 0}, 
					{ 0, 10, 0}, 1));

		SetRayVisibility(State->CubeObjects[I], true);
	}

	InitiateGameObject(
		State->TestObject,
			CreateEnityComponent(&State->Drawables, "Flower"),
			CreateLightComponent(&State->Lights));

	SetLightColor(State->TestObject, RED);
	SetLightIntensity(State->TestObject, 0);
	SetVisibility(State->TestObject, false);
}


/************************************************************************************************/


void CreateTerrainTest(PlayState* State, FlexKit::GameFramework* Framework)
{
	for(size_t I = 0; I < 10; ++I){
		InitiateGameObject( 
			State->CubeObjects[I],
				CreateEnityComponent(&State->Drawables, "Flower"),
				//CreateLightComponent(&State->Lights, {1, -1, -1}, 1, 1000),
				State->Physics.CreateCubeComponent(
					{ 0, 1000.0f + 10.0f * I, 0}, 
					{ 0, 10, 0}, 1));

		SetRayVisibility(State->CubeObjects[I], true);
	}

	InitiateGameObject(
		State->Player,
			State->Physics.CreateCharacterController({0, 10, 0}, 5, 5),
			//CreateThirdPersonCamera(&State->TPC, Framework->ActiveCamera));
			CreateCameraComponent(Framework->Core->Cameras, GetWindowAspectRatio(Framework->Core), 0.01f, 10000.0f, InvalidComponentHandle),
			CreateOrbitCamera(State->OrbitCameras, &Framework->Core->Cameras, 10000.0f));

	Translate		(State->Player, {0, 10000, -10});
	SetCameraOffset	(State->Player, { 0, 15, 10 });

	//auto CubeHandle = CreateCube(State->Framework->Core->RenderSystem, State->Framework->Core->Geometry, State->Framework->Core->GetBlockMemory(), 100, 1234);

	auto HF = LoadHeightFieldCollider(&Framework->Core->Physics, &Framework->Core->Assets, 10601);

	PxHeightFieldGeometry	hfGeom(HF, PxMeshGeometryFlags(), 4096.0f/32767.0f , 8, 8);
	PxTransform				HFPose(PxVec3(-4096, 0, -4096));
	auto					aHeightFieldActor = Framework->Core->Physics.Physx->createRigidStatic(HFPose);

	PxShape* aHeightFieldShape = aHeightFieldActor->createShape(hfGeom, &Framework->Core->Physics.DefaultMaterial, 1);
	State->Physics.Scene->addActor(*aHeightFieldActor);

	FK_ASSERT(0);
	//PushRegion(&State->Framework->Landscape, { { 0, 0, 0, 16384 },{}, 0,{ 0.0f, 0.0f },{ 1.0f, 1.0f } });
}


/************************************************************************************************/
