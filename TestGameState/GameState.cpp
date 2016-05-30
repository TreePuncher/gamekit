/**********************************************************************

Copyright (c) 2015 Robert May

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

// TODO's
//	Gameplay:
//		Entity Model
//	Sound:
//	
//	Generic:
//		Scene Loading
//		Config loading system?
//
//	Graphics:
//		Basic Gui rendering methods (Draw Rect, Draw Line, etc)
//		Texture Loading
//		Terrain Rendering
//			Texture Splatting
//			Height Mapping
//			Geometry Generation - partially complete
//		Occlusion Culling
//		Animation State Machine
//		3rd Person Camera Handler
//		Object -> Bone Attachment
//		Particles
//
//		Bugs:
//			TextRendering Bug, Certain Characters do not get Spaces Correctly
//
//	AI:
//		Path Finding
//		State Handling
//
//	Physics:
//		Statics
// 
//	Network:
//		Client:
//		Server:
//
//	Tools:
//		Meta-Data for Resource Compiling
//

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif

#include "..\Application\GameMemory.h"
#include "..\Application\ResourceUtilities.h"
#include "..\Application\ResourceUtilities.cpp"
#include "..\Application\GameUtilities.cpp"
#include "..\graphicsutilities\TerrainRendering.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\Resources.h"
#include "..\coreutilities\AllSourceFiles.cpp"

#include <iostream>

/*
#ifdef _DEBUG
#pragma comment(lib, "FlexKitd.lib")
#else
#pragma comment(lib, "FlexKit.lib")
#endif
*/
/************************************************************************************************/


using FlexKit::float3;
using FlexKit::Quaternion;
using FlexKit::Line3DPass;

/************************************************************************************************/


#include <..\physx\Include\characterkinematic\PxController.h>
#include <..\physx\Include\characterkinematic\PxCapsuleController.h>

class CCHitReport : public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
{
public:

	virtual void onShapeHit(const physx::PxControllerShapeHit& hit)
	{
	}

	virtual void onControllerHit(const physx::PxControllersHit& hit)
	{
	}

	virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit)
	{
	}

	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape&, const physx::PxActor&)
	{
		return physx::PxControllerBehaviorFlags(0);
	}

	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController&)
	{
		return physx::PxControllerBehaviorFlags(0);
	}

	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle&)
	{
		return physx::PxControllerBehaviorFlags(0);
	}

protected:
};

struct CapsuleCharacterController
{
	physx::PxControllerFilters  characterControllerFilters;
	physx::PxFilterData			characterFilterData;
	physx::PxController*		Controller;

	bool						FloorContact;
	bool						CeilingContact;

	CCHitReport					ReportCallback;
};

struct CapsuleCharacterController_DESC
{
	float r;
	float h;
	float3 FootPos;
};


using namespace physx;


/************************************************************************************************/


void Initiate(CapsuleCharacterController* out, PScene* Scene, PhysicsSystem* PS, CapsuleCharacterController_DESC& Desc)
{
	PxCapsuleControllerDesc CCDesc;
	CCDesc.material       = PS->DefaultMaterial;
	CCDesc.radius	      = Desc.r;
	CCDesc.height	      = Desc.h;
	CCDesc.contactOffset  = 0.1f;
	CCDesc.position       = { 0.0f, Desc.h / 2, 0.0f };
	CCDesc.climbingMode	  = PxCapsuleClimbingMode::eEASY;

	auto NewCharacterController = Scene->ControllerManager->createController(CCDesc);
	out->Controller				= NewCharacterController;
	out->Controller->setFootPosition({Desc.FootPos.x, Desc.FootPos.y, Desc.FootPos.z});
	out->FloorContact			= false;
	out->CeilingContact			= false;
}


/************************************************************************************************/


struct KeyState
{
	bool Forward;
	bool Backward;
	bool Left;
	bool Right;
	bool Jump;
	bool MouseEnable;
};

struct GameActor
{
	EntityHandle				Drawable;
	float						Health;

	CapsuleCharacterController	BPC;
};
typedef size_t ActorHandle;

struct PlayerController
{
	NodeHandle			PitchNode;
	NodeHandle			Node;
	NodeHandle			YawNode;
	NodeHandle			ModelNode;

	float3				dV;
	float				r;// Rotation Rate;
	float				m;// Movement Rate;
	float				HalfHeight;
};

struct InertiaState
{
	float3	Inertia;
	float	Mass;
	float	Drag;
};


/************************************************************************************************/


struct TestPlayer_DESC
{
	float  dr	= FlexKit::pi/2;  // Rotation Rate;
	float  dm	= 10;  // Movement Rate;
	float  r	= 3;   // Radius
	float  h	= 10;   // Height
	float3 POS	= {0, 0, 0}; // Foot Position
};


/************************************************************************************************/


struct MouseCameraController
{
	NodeHandle	PitchOut;
	NodeHandle	YawOut;
	float		Pitch;
	float		Yaw;
	float		PitchMax;
	Camera*		Camera;
};


/************************************************************************************************/


void InitateMouseCameraController(
	float P, float Y, float P_Max, 
	Camera* Cam, 
	NodeHandle PN, NodeHandle YN, 
	MouseCameraController* MCC)
{
	MCC->Camera   = Cam;
	MCC->Pitch    = 0;
	MCC->Yaw      = 0;
	MCC->PitchOut = PN;
	MCC->YawOut   = YN;
	MCC->PitchMax = 75.0f;
}


/************************************************************************************************/


void InitiateGameActor(float r, float h, float3 InitialPosition,  FlexKit::PScene* S, FlexKit::PhysicsSystem* PS, EntityHandle Drawable, GameActor* out)
{
	out->Drawable			= Drawable;
	Initiate(&out->BPC, S, PS, CapsuleCharacterController_DESC{r, h, InitialPosition});
}


/************************************************************************************************/


void BindActorToPlayerController(GameActor* Actor, PlayerController* Controller, GraphicScene* GS )
{
	GS->SetNode(Actor->Drawable, Controller->ModelNode);
}


/************************************************************************************************/


void InitiateEntityController(PlayerController* p, SceneNodes* Nodes, TestPlayer_DESC& Desc = TestPlayer_DESC())
{
	NodeHandle PitchNode	= GetZeroedNode(Nodes);
	NodeHandle Node			= GetZeroedNode(Nodes);
	NodeHandle YawNode		= GetZeroedNode(Nodes);
	NodeHandle ModelNode	= GetZeroedNode(Nodes);

	SetParentNode(Nodes, Node,    ModelNode);
	SetParentNode(Nodes, YawNode, PitchNode);
	SetParentNode(Nodes, Node,		YawNode);

	p->dV			  = {0.0f, 0.0f, 0.0f};
	p->PitchNode	  = PitchNode;
	p->YawNode		  = YawNode;
	p->Node			  = Node;
	p->ModelNode	  = ModelNode;
	p->r			  = FlexKit::pi/2;
	p->m			  = 10;

	p->HalfHeight	  = Desc.h / 2 + Desc.r;
}


/************************************************************************************************/


void TranslateActor(float3 pos, GameActor* Actor, double dt)
{
	auto flags = Actor->BPC.Controller->move({pos.x, pos.y, pos.z}, 0.2, dt, PxControllerFilters());
	Actor->BPC.FloorContact   = flags & PxControllerCollisionFlag::eCOLLISION_DOWN;
	Actor->BPC.CeilingContact = flags & PxControllerCollisionFlag::eCOLLISION_UP;
}

float3 GetActorPosition(GameActor* Actor)
{
	auto Pos = Actor->BPC.Controller->getFootPosition();
	return {Pos.x, Pos.y, Pos.z};
}


/************************************************************************************************/


struct MouseInputState
{
	int2 dPos		={0, 0};
	bool Enabled	= false;
};


void UpdateMouseCameraController(MouseCameraController* out, SceneNodes* Nodes, int2 MouseState)
{
	out->Pitch	= max(min	(out->Pitch + float(MouseState[1]) / 2, 75), -45);
	out->Yaw	= fmodf	(out->Yaw + float(MouseState[0])/ 2, 360);

	Quaternion Yaw(0, out->Yaw, 0);
	Quaternion Pitch(out->Pitch, 0, 0);

	SetOrientationL(Nodes, out->PitchOut, Pitch);
	SetOrientationL(Nodes, out->YawOut, Yaw);
}


float3 UpdateController(PlayerController p, KeyState k, SceneNodes* Nodes)
{
	using FlexKit::Yaw;
	using FlexKit::Pitch;
	using FlexKit::Roll;

	float3 dm(0);

	if (k.Forward){
		float3 forward ={ 0, 0,  -1 };
		float3 d = forward;
		dm += d;
	}

	if (k.Backward){
		float3 backward ={ 0, 0, 1 };
		float3 d = backward;
		dm += d;
	}

	if (k.Left){
		float3 Left ={ -1, 0, 0 };
		float3 d = Left;
		dm += d;
	}

	if (k.Right){
		float3 Right ={ 1, 0, 0 };
		float3 d = Right;
		dm += d;
	}

	Quaternion Q;
	GetOrientation(Nodes, p.YawNode, &Q);
	if (dm.magnitudesquared() > 0)
		dm.normalize();

	return Q * dm;
}


float3 UpdateActorInertia(InertiaState* IS, double dt, GameActor* Actor, float3 D, float c = 0.1f, float3 G ={ 0, -9.8f, 0})
{
	IS->Inertia = (IS->Inertia - IS->Inertia * c) + D * dt * 10;

	if (D.magnitude() < 0.001)
		IS->Inertia = 0;
	if (Actor->BPC.FloorContact)
		IS->Inertia.y = 0;
	else
		IS->Inertia += dt * G;
	
	return IS->Inertia;
}

float3 UpdateGameActor(float3 dV, GraphicScene* Scene, double dt, GameActor* ActorState, NodeHandle Target)
{ 
	float3 FinalDelta = dV;
	FinalDelta *= ActorState->BPC.FloorContact ? float3(1, 0, 1) : float3(1);

	if (FinalDelta.magnitude() > 0)
	{
		float3 OldP = GetActorPosition(ActorState);
		TranslateActor(FinalDelta, ActorState, dt);
		float3 NewP = GetActorPosition(ActorState);
		SetPositionW( Scene->SN, Target, NewP);
		Scene->GetEntity(ActorState->Drawable).Dirty = true;

		float3 Dir  = Quaternion(0, 90, 0) * (float3(1, 0, 1) * FinalDelta * float3(-1, 0, -1)).normal();
		float3 SV   = Dir.cross(float3{0, 1, 0});
		float3 UpV	= SV.cross(Dir);
	
		auto Q2 = Vector2Quaternion(Dir, UpV, SV);
		Q2.x = 0;
		Q2.z = 0;
	
		SetOrientation(Scene->SN, Scene->GetNode(ActorState->Drawable), Q2);

		return (NewP - OldP);
	}

	return (0);
}


/************************************************************************************************/


void UpdateMouseInput(MouseInputState* State, RenderWindow* Window)
{
	using FlexKit::int2;

	if ( !State->Enabled )
		return;

	if ( GetForegroundWindow() == Window->hWindow )
	{
		State->dPos = GetMousedPos(Window);
		SetSystemCursorToWindowCenter(Window);
	}
	else
	{
		ShowCursor( true );
		State->Enabled = false;
	}
}


/************************************************************************************************/


struct Scene
{
	EntityHandle			PlayerModel;
	JointHandle				Joint;
	Camera					PlayerCam;
	MouseCameraController	PlayerCameraController;
	GameActor				PlayerActor;
	PlayerController		PlayerController;
	InertiaState			PlayerInertia;

	float T;
};

struct GameState
{
	Scene TestScene;

	TextUtilities::TextRender	TextRender;
	TextUtilities::FontAsset*	Font;
	TextUtilities::TextArea		Text;

	DeferredPass		DeferredPass;
	ForwardPass			ForwardPass;
	Line3DPass			LineDrawPass;

	MouseInputState		Mouse;
	float				MouseMovementFactor;
	double				PhysicsUpdateTimer;

	Landscape			Landscape;
	Camera*				ActiveCamera;
	RenderWindow*		ActiveWindow;
	DepthBuffer*		DepthBuffer;
	float4				ClearColor;
		
	SceneNodes*			Nodes;
	PScene				PScene;
	GraphicScene		GScene;

	KeyState	Keys;
	bool		Quit;
	bool		DoDeferredShading;
};


/************************************************************************************************/

#include "..\Application\GameUtilities.h"
#include "..\graphicsutilities\TerrainRendering.h"
#include "..\graphicsutilities\graphics.h"
#include "..\PhysicsUtilities\physicsutilities.h"

/************************************************************************************************/
using namespace FlexKit;

void HandleKeyEvents(const Event& e, GameState* GS)
{
	switch (e.Action)
	{
	case Event::Release:
			switch (e.mData1.mKC[0])
			{
			case FlexKit::KC_ESC:
				GS->Quit = true;
			case FlexKit::KC_W:
				GS->Keys.Forward  = false; break;
			case FlexKit::KC_S:
				GS->Keys.Backward = false; break;
			case FlexKit::KC_A:
				GS->Keys.Left     = false; break;
			case FlexKit::KC_D:
				GS->Keys.Right    = false; break;
			case FlexKit::KC_M:
			{
				SetSystemCursorToWindowCenter(GS->ActiveWindow);
				if(GS->Keys.MouseEnable)
					ShowSystemCursor(GS->ActiveWindow);
					else
					HideSystemCursor(GS->ActiveWindow);

				GS->Keys.MouseEnable = !GS->Keys.MouseEnable; 
			}	break;
			default:
				break;
			}
		break;
	case Event::Pressed:
			switch (e.mData1.mKC[0])
			{
			case FlexKit::KC_W:
				GS->Keys.Forward  = true; break;
			case FlexKit::KC_S:
				GS->Keys.Backward = true; break;
			case FlexKit::KC_A:
				GS->Keys.Left     = true; break;
			case FlexKit::KC_D:
				GS->Keys.Right    = true; break;
			case FlexKit::KC_M:
			default:
				break;
			}
		break;

	default:
		break;

	}
}

void HandleMouseEvents(const Event& e, GameState* GS)
{
}

void SetActiveCamera(GameState* State, Camera* _ptr){State->ActiveCamera = _ptr;}

void CreateTestScene(EngineMemory* Engine, GameState* State, Scene* Out)
{
	auto PlayerModel	= State->GScene.CreateDrawableAndSetMesh(Engine->BuiltInMaterials.DefaultMaterial, "PlayerModel");
	Out->PlayerModel	= PlayerModel;

	State->GScene.EntityEnablePosing(PlayerModel);
	State->GScene.EntityPlayAnimation(PlayerModel, "ANIMATION1", 0.5f);
	Out->Joint = State->GScene.GetEntity(PlayerModel).PoseState->Sk->FindJoint("Chest");

	PrintSkeletonHierarchy(State->GScene.GetEntity(PlayerModel).Mesh->Skeleton);

	State->GScene.Yaw					(PlayerModel, pi / 2);
	State->GScene.SetMaterialParams		(PlayerModel, { 0.1f, 0.2f, 0.5f , 0.8f }, { 0.5f, 0.5f, 0.5f , 0.0f });
	State->DepthBuffer	= &Engine->DepthBuffer;

	for (uint32_t I = 0; I < 0; ++I) {
		for (uint32_t II = 0; II < 100; ++II) {
			auto Floor = State->GScene.CreateDrawableAndSetMesh(Engine->BuiltInMaterials.DefaultMaterial, "FloorTile");
			State->GScene.TranslateEntity_WT(Floor, { 10 * 40 - 40.0f * I, 0, 10 * 40 -  40.0f * II });
			State->GScene.SetMaterialParams(Floor,  { 0.3f, 0.3f, 0.3f , 0.2f }, { 0.5f, 0.5f, 0.5f , 0.0f });
		}
	}

	auto	WindowRect	= Engine->Window.WH;
	float	Aspect		= (float)WindowRect[0] / (float)WindowRect[1];
	InitiateCamera	(Engine->RenderSystem, State->Nodes, &Out->PlayerCam, Aspect, 0.01f, 10000.0f, true);
	SetActiveCamera	(State, &Out->PlayerCam);

	InitiateGameActor(5, 10, {0, 0, 0}, &State->PScene, &Engine->Physics, PlayerModel, &Out->PlayerActor);
	InitiateEntityController(&Out->PlayerController, &Engine->Nodes);

	BindActorToPlayerController(&Out->PlayerActor, &Out->PlayerController, &State->GScene);
	State->GScene.Yaw(PlayerModel, pi );

	SetParentNode	(State->Nodes,	Out->PlayerController.PitchNode, Out->PlayerCam.Node);
	TranslateLocal	(Engine->Nodes, Out->PlayerCam.Node, {0.0f, 20.0f, 00.0f});
	TranslateLocal	(Engine->Nodes, Out->PlayerCam.Node, {0.0f, 00.0f, 40.0f});

	State->GScene.AddPointLight({  0, 10,  0 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({ 40, 10,  0 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({-40, 10,  0 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({ 40, 10, 40 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({ 40, 10,-40 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({-40, 10, 40 }, { 1, 1, 1 }, 1000, 1000);
	State->GScene.AddPointLight({-40, 10,-40 }, { 1, 1, 1 }, 1000, 1000);

	InitateMouseCameraController(
		0, 0, 75.0f, 
		&Out->PlayerCam, 
		Out->PlayerController.PitchNode, 
		Out->PlayerController.YawNode, 
		&Out->PlayerCameraController);

	Out->PlayerInertia.Drag = 0.1f;
	Out->PlayerInertia.Inertia = float3(0);
	Out->T = 0;
}


void UpdateTestScene(Scene* TestScene,  GameState* State, double dt)
{
	float3 InputMovement	= UpdateController(TestScene->PlayerController, State->Keys, State->Nodes);
	float3 Inertia			= UpdateActorInertia(&TestScene->PlayerInertia, dt, &TestScene->PlayerActor, InputMovement);

	auto PoseState = State->GScene.GetEntity(TestScene->PlayerModel).PoseState;
	//TranslateJoint(PoseState, TestScene->Joint, {0, 0, (float)dt });
	TestScene->T += dt;

	AddLineSegment(&State->LineDrawPass, {{0, 0,  0}, { 0, 50, 0}});
	AddLineSegment(&State->LineDrawPass, {{10, 0, 0}, {10, 50, 0}});
	AddLineSegment(&State->LineDrawPass, {{20, 0, 0}, {20, 50, 0}});
	AddLineSegment(&State->LineDrawPass, {{30, 0, 0}, {30, 50, 0}});
	AddLineSegment(&State->LineDrawPass, {{40, 0, 0}, {40, 50, 0}});

	UpdateGameActor(Inertia, &State->GScene, dt, &TestScene->PlayerActor, TestScene->PlayerController.Node);
	UpdateMouseCameraController(&TestScene->PlayerCameraController, State->Nodes, State->Mouse.dPos);
}

void KeyEventsWrapper(const Event& in, void* _ptr)
{ 
	switch (in.InputSource)
	{
	case Event::Keyboard:
		HandleKeyEvents(in, reinterpret_cast<GameState*>(_ptr));
	}
}

void MouseEventsWrapper(const Event& in, void* _ptr)
{ 
	switch (in.InputSource)
	{
	case Event::Mouse:
		HandleMouseEvents(in, reinterpret_cast<GameState*>(_ptr));
	}
}

extern "C"
{
	GAMESTATEAPI GameState* InitiateGameState(EngineMemory* Engine)
	{
		GameState& State = Engine->BlockAllocator.allocate_aligned<GameState>();

		FlexKit::AddResourceFile("assets\\ResourceFile.gameres", &Engine->Assets);
		State.ClearColor         = { 0.0f, 0.2f, 0.4f, 1.0f };
		State.Nodes		         = &Engine->Nodes;
		State.Quit		         = false;
		State.PhysicsUpdateTimer = 0.0f;
		State.Mouse				 = MouseInputState();
		State.DoDeferredShading  = true;
		State.ActiveWindow		 = &Engine->Window;

		ForwardPass_DESC FP_Desc{&Engine->DepthBuffer, &Engine->Window};
		DeferredPassDesc DP_Desc{&Engine->DepthBuffer, &Engine->Window, nullptr };

		InitiateForwardPass		(Engine->RenderSystem, &FP_Desc, &State.ForwardPass);
		InitiateDeferredPass	(Engine->RenderSystem, &DP_Desc, &State.DeferredPass);
		InitiateScene			(&Engine->Physics, &State.PScene);
		InitiateGraphicScene	(&State.GScene, Engine->RenderSystem, &Engine->Materials, &Engine->Assets, &Engine->Nodes, Engine->BlockAllocator, Engine->TempAllocator);
		InitiateTextRender		(Engine->RenderSystem, &State.TextRender);
		InitiateSegmentPass		(Engine->RenderSystem, Engine->BlockAllocator, &State.LineDrawPass);

		{
			Landscape_Desc Land_Desc = { 
				State.DeferredPass.Filling.NoTexture.Blob->GetBufferPointer(), 
				State.DeferredPass.Filling.NoTexture.Blob->GetBufferSize() 
			};

			InitiateLandscape		(Engine->RenderSystem, GetZeroedNode(&Engine->Nodes), &Land_Desc, Engine->BlockAllocator, &State.Landscape);
			PushRegion				(&State.Landscape, {{0, 0, 0, 2048}, {}, 0});
			UploadLandscape			(Engine->RenderSystem, &State.Landscape, nullptr, nullptr, true, false);
		}

		auto WH = State.ActiveWindow->WH;
		TextUtilities::TextArea_Desc TA_Desc = { { 0, 0 },{ float(WH[0]), float(WH[1]) },{ 32, 32 } };

		State.Font = TextUtilities::LoadFontAsset("assets\\fonts\\", "fontTest.fnt", Engine->RenderSystem, Engine->TempAllocator, Engine->BlockAllocator);
		State.Text = TextUtilities::CreateTextObject(&Engine->RenderSystem, Engine->BlockAllocator, &TA_Desc);

		TextUtilities::PrintText(&State.Text, "TESTING!\n");

		CreatePlaneCollider		(Engine->Physics.DefaultMaterial, &State.PScene);
		CreateTestScene			(Engine, &State, &State.TestScene);

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify = &KeyEventsWrapper;
		sub._ptr   = &State;
		Engine->Window.Handler.Subscribe(sub);

		UploadResources(&Engine->RenderSystem);// Uploads fresh Resources to GPU

		return &State;
	}

	void CleanUpState(GameState* Scene, EngineMemory* Engine)
	{
		TextUtilities::FreeFontAsset(Scene->Font);
		TextUtilities::CleanUpTextArea(&Scene->Text, Engine->BlockAllocator);

		CleanUpTerrain(Scene->Nodes, &Scene->Landscape);
		Engine->BlockAllocator.free(Scene->Font);
	}


	GAMESTATEAPI void Update(EngineMemory* Engine, GameState* State, double dt)
	{
		Engine->End = State->Quit;
		State->Mouse.Enabled = State->Keys.MouseEnable;

		{
			UpdateScene		(&State->PScene, 1.0f/60.0f, nullptr, nullptr, nullptr );
			UpdateColliders	(&State->PScene, &Engine->Nodes);
		}
	}


	GAMESTATEAPI void UpdateFixed(EngineMemory* Engine, double dt, GameState* State)
	{
		UpdateMouseInput(&State->Mouse, &Engine->Window);
		UpdateTestScene(&State->TestScene, State, dt);
		UpdateGraphicScene(&State->GScene);
	}


	GAMESTATEAPI void UpdateAnimations(RenderSystem* RS, iAllocator* TempMemory, double dt, GameState* _ptr)
	{
		UpdateAnimationsGraphicScene(&_ptr->GScene, dt);
	}


	GAMESTATEAPI void UpdatePreDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameState* State)
	{
		UpdateTransforms(State->Nodes);
		UpdateCamera(Engine->RenderSystem, State->Nodes, State->ActiveCamera, dt);
	}


	GAMESTATEAPI void Draw(RenderSystem* RS, iAllocator* TempMemory, FlexKit::ShaderTable* M, GameState* State)
	{
		using TextUtilities::DrawTextArea;

		auto PVS			= TempMemory->allocate_aligned<FlexKit::PVS>();
		auto Transparent	= TempMemory->allocate_aligned<FlexKit::PVS>();

		GetGraphicScenePVS(&State->GScene, State->ActiveCamera, &PVS, &Transparent);

		SortPVS(State->Nodes, &PVS, State->ActiveCamera);
		SortPVSTransparent(State->Nodes, &Transparent, State->ActiveCamera);

		BeginPass(RS, State->ActiveWindow);
		auto CL = GetCurrentCommandList(RS);

		// Do Uploads
		{
			DeferredPass_Parameters	DPP;
			DPP.PointLightCount = State->GScene.PLights.size();
			DPP.SpotLightCount  = State->GScene.SPLights.size();

			UploadPoses(RS, &PVS, TempMemory);
			UploadLineSegments(RS, &State->LineDrawPass);
			UploadDeferredPassConstants(RS, &DPP, {0.1f, 0.1f, 0.1f, 0}, &State->DeferredPass);
			UploadCamera(RS, State->Nodes, State->ActiveCamera, State->GScene.PLights.size(), State->GScene.SPLights.size(), 0.0f);
			UploadGraphicScene(&State->GScene, &PVS, &Transparent);
			UploadLandscape(RS, &State->Landscape, State->Nodes, State->ActiveCamera, false, true);
		}

		// Submission
		{
			ClearBackBuffer	(RS, CL, State->ActiveWindow, {0, 0, 0, 0});
			ClearDepthBuffer(RS, CL, State->DepthBuffer, 0.0f, 0, State->DeferredPass.CurrentBuffer);

			if (State->DoDeferredShading)
			{
				DoDeferredPass(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, &State->GScene.SPLights);
				DrawLandscape(RS, &State->Landscape, 15, State->ActiveCamera);
				ShadeDeferredPass(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, &State->GScene.SPLights);
				DoForwardPass(&Transparent, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights);// Transparent Objects
			}
			else
			{
				DoForwardPass(&PVS, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights);
			}
		}

		Draw3DLineSegments(RS, &State->LineDrawPass, State->ActiveCamera, State->ActiveWindow);
		DrawTextArea(&State->TextRender, State->Font, &State->Text, TempMemory, RS, State->ActiveWindow);
		
		EndPass(CL, RS, State->ActiveWindow);
	}


	GAMESTATEAPI void PostDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameState* _ptr)
	{
		PresentWindow(&Engine->Window, Engine->RenderSystem);
	}


	GAMESTATEAPI void CleanUp(EngineMemory* Engine, GameState* _ptr)
	{
		WaitforGPU(&Engine->RenderSystem);

		Engine->BlockAllocator.free(_ptr);
		
		FreeAllResourceFiles	(&Engine->Assets);
		FreeAllResources		(&Engine->Assets);

		CleanUpLineDrawPass		(&_ptr->LineDrawPass);
		CleanUpState			(_ptr, Engine);
		CleanUpCamera			(_ptr->Nodes, _ptr->ActiveCamera);
		CleanUpTextRender		(&_ptr->TextRender);
		CleanupForwardPass		(&_ptr->ForwardPass);
		CleanupDeferredPass		(&_ptr->DeferredPass);
		CleanUpGraphicScene		(&_ptr->GScene);
		CleanUpScene			(&_ptr->PScene);
		CleanUpEngine			(Engine);
	}


	GAMESTATEAPI void PostPhysicsUpdate(GameState*)
	{

	}


	GAMESTATEAPI void PrePhysicsUpdate(GameState*)
	{

	}

	struct 
	{
		typedef GameState*	(*InitiateGameStateFN)	(EngineMemory* Engine);
		typedef void		(*UpdateFixedIMPL)		(EngineMemory* Engine,	double dt, GameState* _ptr);
		typedef void		(*UpdateIMPL)			(EngineMemory* Engine,	GameState* _ptr, double dt);
		typedef void		(*UpdateAnimationsFN)	(RenderSystem* RS,		iAllocator* TempMemory, double dt, GameState* _ptr);
		typedef void		(*UpdatePreDrawFN)		(EngineMemory* Engine,	iAllocator* TempMemory, double dt, GameState* _ptr);
		typedef void		(*DrawFN)				(RenderSystem* RS,		iAllocator* TempMemory, FlexKit::ShaderTable* M, GameState* _ptr);
		typedef void		(*PostDrawFN)			(EngineMemory* Engine,	iAllocator* TempMemory, double dt, GameState* _ptr);
		typedef void		(*CleanUpFN)			(EngineMemory* Engine,	GameState* _ptr);
		typedef void		(*PostPhysicsUpdate)	(GameState*);
		typedef void		(*PrePhysicsUpdate)		(GameState*);

		InitiateGameStateFN		Init;
		InitiateEngineFN		InitEngine;
		UpdateIMPL				Update;
		UpdateFixedIMPL			UpdateFixed;
		UpdateAnimationsFN		UpdateAnimations;
		UpdatePreDrawFN			UpdatePreDraw;
		DrawFN					Draw;
		PostDrawFN				PostDraw;
		CleanUpFN				CleanUp;
	}Table;


	GAMESTATEAPI void GetStateTable(CodeTable* out)
	{
		Table.Init				= &InitiateGameState;
		Table.InitEngine		= &InitEngine;
		Table.Update			= &Update;
		Table.UpdateFixed		= &UpdateFixed;
		Table.UpdateAnimations	= &UpdateAnimations;
		Table.UpdatePreDraw		= &UpdatePreDraw;
		Table.Draw				= &Draw;
		Table.PostDraw			= &PostDraw;
		Table.CleanUp			= &CleanUp;

		*out = *(CodeTable*)(&Table);
	}
}