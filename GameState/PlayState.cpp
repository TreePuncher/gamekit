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
#include "..\graphicsutilities\ImageUtilities.h"
#include "..\coreutilities\GraphicsComponents.h"

/************************************************************************************************/


bool PlayEventHandler(SubState* StateMemory, Event evt)
{
	PlayState* ThisState = (PlayState*)StateMemory;

	if (evt.InputSource == Event::Keyboard)
	{
		switch (evt.Action)
		{
		case Event::Pressed:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				ThisState->Input.KeyState.Shield   = true;
				break;
			case KC_W:
				ThisState->Input.KeyState.Forward  = true;
				break;
			case KC_S:
				ThisState->Input.KeyState.Backward = true;
				break;
			case KC_A:
				ThisState->Input.KeyState.Left     = true;
				break;
			case KC_D:
				ThisState->Input.KeyState.Right    = true;
				break;
			}
		}	break;
		case Event::Release:
		{
			switch (evt.mData1.mKC[0])
			{
			case KC_E:
				ThisState->Input.KeyState.Shield   = false;
				break;
			case KC_W:
				ThisState->Input.KeyState.Forward  = false;
				break;
			case KC_S:
				ThisState->Input.KeyState.Backward = false;
				break;
			case KC_A:
				ThisState->Input.KeyState.Left     = false;
				break;
			case KC_D: 
				ThisState->Input.KeyState.Right    = false;
				break;
			}
		}	break;
		}
	}
	return true;
}


/************************************************************************************************/


bool PlayUpdate(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (PlayState*)StateMemory;

	float MovementFactor			= 50.0f;

	//ThisState->Model.PlayerInputs[0].FrameID++;
	//ThisState->Model.PlayerInputs[0].MouseInput		= { HorizontalMouseMovement, VerticalMouseMovement };
	//ThisState->Model.PlayerInputs[0].KeyboardInput	= ThisState->Input;

	ThisState->Input.Update(dT, ThisState->Framework->MouseState, StateMemory->Framework );

	double T = ThisState->Framework->TimeRunning;
	double CosT = (float)cos(T);
	double SinT = (float)sin(T);

	float Begin	= 0.0f;
	float End	= 60.0f;
	float IaR	= 10000 * (1 + (float)cos(T * 6)) / 2;

	Yaw(ThisState->TestObject, dT * pi);
	SetDrawableColor(ThisState->TestObject, Grey(CosT));
	SetWorldPosition(ThisState->TestObject, float3( 100.0f * CosT, 60.0f, SinT * 100));

	SetLightRadius		(ThisState->TestObject, 100 + IaR);
	SetLightIntensity	(ThisState->TestObject, 100 + IaR);
	
	ThisState->OrbitCameras.Update(dT);
	ThisState->Physics.UpdateSystem(dT);

	return false;
}


/************************************************************************************************/


bool PreDrawUpdate(SubState* StateMemory, EngineMemory* Engine, double dT)
{
	auto ThisState = (PlayState*)StateMemory;
	ThisState->Physics.UpdateSystem_PreDraw(dT);

	if(ThisState->Framework->DrawPhysicsDebug)
	{
		//auto PlayerPOS = ThisState->Model.Players[0].PlayerCTR.Pos;
		//PushCapsule_Wireframe(&StateMemory->Framework->Immediate, Engine->TempAllocator, { PlayerPOS.x, PlayerPOS.y, PlayerPOS.z }, 5, 10, GREEN);
	}


	//ThisState->Model.UpdateAnimations(ThisState->Framework, DT);

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
	GScene.ClearScene();

	ReleaseGraphicScene(&GScene);
}


/************************************************************************************************/


void ReleasePlayState(SubState* StateMemory)
{
	auto ThisState = (PlayState*)StateMemory;
	ThisState->Framework->Engine->BlockAllocator.Delete(ThisState);
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

PlayState* CreatePlayState(EngineMemory* Engine, GameFramework* Framework)
{
	PlayState* State = nullptr;

	State						 = &Engine->BlockAllocator.allocate_aligned<PlayState>();
	State->VTable.PreDrawUpdate  = PreDrawUpdate;
	State->VTable.Update         = PlayUpdate;
	State->VTable.EventHandler   = PlayEventHandler;
	State->VTable.PostDrawUpdate = nullptr;
	State->VTable.Release        = ReleasePlayState;
	State->Framework			 = Framework;

	InitiateGraphicScene(&State->GScene, Engine->RenderSystem, &Engine->Assets, Engine->Nodes, Engine->Geometry, Engine->BlockAllocator, Engine->TempAllocator);
	State->Drawables.InitiateSystem(&State->GScene, Engine->Nodes);
	State->Lights.InitiateSystem(&State->GScene, Engine->Nodes);
	State->Physics.InitiateSystem(&Engine->Physics, Engine->Nodes, Engine->BlockAllocator);

	State->Input.Initiate(Framework);
	State->OrbitCameras.Initiate(Framework, State->Input);


	Framework->ActivePhysicsScene	= &State->Physics;
	Framework->ActiveScene			= &State->GScene;

	//FK_ASSERT(LoadScene(Engine->RenderSystem, Engine->Nodes, &Engine->Assets, &Engine->Geometry, 201, &State->GScene, Engine->TempAllocator), "FAILED TO LOAD!\n");

	GameObject<> Test;

	InitiateGameObject(
		State->TestObject,
			CreateEnityComponent(&State->Drawables, "Flower"),
			CreateLightComponent(&State->Lights));

	SetLightColor(State->TestObject, RED);

#if 0
	InitiateGameObject( 
		State->FloorObject,
			State->Physics.CreateStaticBoxCollider({10000, 1, 10000}, {0, -0.5, 0}));
#endif

	for(size_t I = 0; I < 10; ++I){
		InitiateGameObject( 
			State->CubeObjects[I],
				CreateEnityComponent(&State->Drawables, "Flower"),
				//CreateLightComponent(&State->Lights, {1, -1, -1}, 1, 1000),
				State->Physics.CreateCubeComponent(
					{ 0, 20.0f + 10.0f * I, 0}, 
					{ 0, 10, 0}, 1));
	}

	InitiateGameObject(
		State->Player,
			State->Physics.CreateCharacterController({0, 10, 0}, 5, 0.01),
			CreateOrbitCamera(State->OrbitCameras, Framework->ActiveCamera));

	Translate(State->Player, {0, 10, -10});
	Yaw(State->Player, pi);
	PushRegion(&State->Framework->Landscape, { { 0, 0, 0, 16384 },{}, 0,{ 0.0f, 0.0f },{ 1.0f, 1.0f } });

	auto CubeHandle = CreateCube(State->Framework->Engine->RenderSystem, State->Framework->Engine->Geometry, State->Framework->Engine->BlockAllocator, 100, 1234);

	return State;
}


/************************************************************************************************/