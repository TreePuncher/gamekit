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
//		Basic Gui rendering methods (Draw Rect, etc)
//		Multi-threaded Texture Uploads
//		Terrain Rendering
//			Texture Splatting
//			Height Mapping
//			Geometry Generation - partially complete
//		Occlusion Culling
//		Animation State Machine
//		3rd Person Camera Handler
//		Object -> Bone Attachment
//		Particles
//		Static Mesh Batcher
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
#include "..\coreutilities\DungeonGen.h"
#include "..\coreutilities\DungeonGen.cpp"
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

struct TestSceneStats
{
	float		T;
	uint32_t	AvgObjectDrawnPerFrame;
};

void ResetStats(TestSceneStats* Stats)
{
	Stats->AvgObjectDrawnPerFrame = 0;
	Stats->T = 0;
}

struct Scalp
{
	ID3D12Resource*			Constants;
	FrameBufferedResource	State[2];

	// Hair Initial State
	struct Strand
	{
		struct{
			float	xyz[3];
		}Points[64];
	};

	size_t				CurrentBuffer;
	DynArray<Strand>	Hairs;
};


struct HairRender
{
	struct
	{
		ID3D12PipelineState* PSO;
	}Simulate;

	struct
	{
		ID3D12PipelineState* PSO;
	}Draw;

	Shader VS;
	Shader HS;
	Shader DS;
	Shader GS;
	Shader PS;
	Shader CS;
};


/************************************************************************************************/


void AddTestStrand(Scalp* Out)
{
	Scalp::Strand TestStrand;
	for (auto& p : TestStrand.Points)
	{
		p.xyz[0] = 0.0f;
		p.xyz[1] = 1.0f;
		p.xyz[2] = 0.0f;
	}

	Out->Hairs.push_back(TestStrand);
}


/************************************************************************************************/


void InitiateScalp(RenderSystem* RS, Resources* Assets, ResourceHandle RHandle, Scalp* Out, iAllocator* allocator)
{
	/*
	auto Resource1			= FlexKit::CreateShaderResource(RS, 2048);
	auto Resource2			= FlexKit::CreateShaderResource(RS, 2048);
	auto Resource3			= FlexKit::CreateShaderResource(RS, 2048);
	*/
	Out->Hairs.Allocator	= allocator;

	//AddTestStrand(Out);
}


void CleanupScalp(Scalp* S)
{
	S->Constants->Release();
	S->Hairs.Release();
	S->State[0].Release();
	S->State[1].Release();
}


/************************************************************************************************/


void InitiateHairRender(RenderSystem* RS, DepthBuffer* DB, HairRender* Out)
{
	Shader CS = LoadShader("ComputeMain",		"DeferredShader",		"cs_5_0", "assets\\HairSimulation.hlsl");
	Shader VS = LoadShader("VPassThrough",		"HairVPassThrough",		"vs_5_0", "assets\\HairRendering.hlsl");
	Shader HS = LoadShader("HShader",			"HShader",				"hs_5_0", "assets\\HairRendering.hlsl");
	Shader DS = LoadShader("DShader",			"DShader",				"ds_5_0", "assets\\HairRendering.hlsl");
	Shader PS = LoadShader("DebugTerrainPaint",	"DebugTerrainPaint",	"ps_5_0", "assets\\Pshader.hlsl");

	Out->CS = CS;
	Out->VS = VS;
	Out->HS = HS;
	Out->DS = DS;
	Out->PS = PS;

	// Create Pipeline State Objects
	{
		// Simulation Step
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {}; {
				Desc.CS = { CS.Blob->GetBufferPointer(), CS.Blob->GetBufferSize() };
				Desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
				Desc.NodeMask = 0;
				Desc.pRootSignature = RS->Library.RS2UAVs4SRVs4CBs;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR = RS->pDevice->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&PSO));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE HAIR SIMULATION STATE OBJECT!"));

			Out->Simulate.PSO = PSO;
		}

		// Render Step
		{

			D3D12_RASTERIZER_DESC		Rast_Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); {
			}

			D3D12_DEPTH_STENCIL_DESC	Depth_Desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); {
				Depth_Desc.DepthFunc = D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				Depth_Desc.DepthEnable = false;
			}

			static_vector<D3D12_INPUT_ELEMENT_DESC> InputElements = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};


			D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {}; {
				Desc.pRootSignature = RS->Library.RS4CBVs4SRVs;
				Desc.VS = { VS.Blob->GetBufferPointer(), VS.Blob->GetBufferSize() };
				Desc.HS = { HS.Blob->GetBufferPointer(), HS.Blob->GetBufferSize() };
				Desc.DS = { DS.Blob->GetBufferPointer(), DS.Blob->GetBufferSize() };
				//Desc.GS						= { GeometryShader.Blob->GetBufferPointer(),	GeometryShader.Blob->GetBufferSize() }; // Skipping for Now
				Desc.PS = { PS.Blob->GetBufferPointer(), PS.Blob->GetBufferSize() };
				Desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

				Desc.NodeMask              = 0;
				Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
				Desc.RasterizerState       = Rast_Desc;
				Desc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				Desc.SampleMask            = UINT_MAX;
				Desc.NumRenderTargets      = 1;
				Desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
				Desc.SampleDesc.Count      = 1;
				Desc.SampleDesc.Quality    = 0;

				Desc.DSVFormat         = DB->Buffer->Format;
				Desc.InputLayout       = { InputElements.begin(), (UINT)InputElements.size() };
				Desc.DepthStencilState = Depth_Desc;
			}

			ID3D12PipelineState* PSO = nullptr;
			HRESULT HR = RS->pDevice->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(&PSO));
			CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE HAIR RENDERING STATE OBJECT!"));

			Out->Draw.PSO = PSO;
		}
	}

}


/************************************************************************************************/


void CleanupHairRender(HairRender* Out)
{
	Out->CS.Blob->Release();
	Out->DS.Blob->Release();
	//Out->GS.Blob->Release();
	Out->HS.Blob->Release();
	Out->PS.Blob->Release();
	Out->VS.Blob->Release();

	Out->Draw.PSO->Release();
	Out->Simulate.PSO->Release();
}


/************************************************************************************************/


struct Scene
{
	EntityHandle			PlayerModel;
	EntityHandle			TestModel;
	JointHandle				Joint;
	Camera					PlayerCam;
	MouseCameraController	PlayerCameraController;
	GameActor				PlayerActor;
	PlayerController		PlayerController;
	InertiaState			PlayerInertia;
	Scalp					TestScalp;
	float					T;


	DungeonGenerator	Dungeon;
};


/************************************************************************************************/


struct GameState
{
	Scene TestScene;

	TextUtilities::TextRender	TextRender;
	TextUtilities::FontAsset*	Font;
	TextUtilities::TextArea		Text;

	TextureSet*	Set1;

	DeferredPass		DeferredPass;
	ForwardPass			ForwardPass;

	GUIRender			GUIRender;

	LineDrawState		LineDrawState;
	Line3DPass			LinePass;

	StaticMeshBatcher	StaticMeshBatcher;

	MouseInputState		Mouse;
	float				MouseMovementFactor;
	double				PhysicsUpdateTimer;

	Landscape			Landscape;
	Camera*				ActiveCamera;
	RenderWindow*		ActiveWindow;
	DepthBuffer*		DepthBuffer;
	float4				ClearColor;
	
	GeometryTable*		GT;
	SceneNodes*			Nodes;
	PScene				PScene;
	GraphicScene		GScene;

	Texture2D			Albedo;

	KeyState	Keys;
	bool		Quit;
	bool		DoDeferredShading;

	HairRender		HairRender;
	TestSceneStats	Stats;
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

void HandleMouseEvents	(const Event& e, GameState* GS){}
void SetActiveCamera	(GameState* State, Camera* _ptr){State->ActiveCamera = _ptr;}

enum ETILEASSET_GUIDS
{
	EAG_BEGIN = 500,
	EAG_CORRIDOR_NS                 = 500,
	EAG_CORRIDOR_NS_COLLIDER        = 501,
	EAG_CORRIDOR_EW                 = 502,
	EAG_CORRIDOR_EW_COLLIDER        = 503,
	EAG_CORRIDOR_CORNER_NE          = 504,
	EAG_CORRIDOR_CORNER_NE_COLLIDER = 505,
	EAG_CORRIDOR_CORNER_NW          = 506,
	EAG_CORRIDOR_CORNER_NW_COLLIDER = 507,
	EAG_CORRIDOR_CORNER_SE          = 508,
	EAG_CORRIDOR_CORNER_SE_COLLIDER = 509,
	EAG_CORRIDOR_CORNER_SW          = 510,
	EAG_CORRIDOR_CORNER_SW_COLLIDER = 511,

	EAG_WALL_NS                     = 512,
	EAG_WALL_NS_COLLIDER            = 513,
	EAG_WALL_EW                     = 514,
	EAG_WALL_EW_COLLIDER            = 515,

	EAG_DOOR_N                  = 516,
	EAG_DOOR_N_COLLIDER         = 517,
	EAG_DOOR_S                  = 518,
	EAG_DOOR_S_COLLIDER         = 519,
	EAG_DOOR_E                  = 520,
	EAG_DOOR_E_COLLIDER         = 521,
	EAG_DOOR_W                  = 522,
	EAG_DOOR_W_COLLIDER         = 523,

	EAG_DEADEND_N					= 524,
	EAG_DEADEND_N_COLLIDER			= 525,
	EAG_DEADEND_S					= 526,
	EAG_DEADEND_S_COLLIDER			= 527,
	EAG_DEADEND_E					= 528,
	EAG_DEADEND_E_COLLIDER			= 529,
	EAG_DEADEND_W					= 530,
	EAG_DEADEND_W_COLLIDER			= 531,

	EAG_END
};


enum ECOLLIDERASSETS
{
	ECA_DEADEND_N_COLLIDER,
	ECA_DEADEND_S_COLLIDER,
	ECA_DEADEND_E_COLLIDER,
	ECA_DEADEND_W_COLLIDER,

	ECA_CORNER_NE_COLLIDER,
	ECA_CORNER_NW_COLLIDER,
	ECA_CORNER_SE_COLLIDER,
	ECA_CORNER_SW_COLLIDER,

	ECA_HALLWAY_NS_COLLIDER,
	ECA_HALLWAY_EW_COLLIDER,

	ECA_COUNT
};


enum ETILEASSET
{
	EAT_CORRIDOR_NS,
	EAT_CORRIDOR_EW,
	EAT_CORRIDOR_CORNER_NE,
	EAT_CORRIDOR_CORNER_NW,
	EAT_CORRIDOR_CORNER_SE,
	EAT_CORRIDOR_CORNER_SW,

	EAT_WALL_NS,
	EAT_WALL_EW,

	EAT_DOORWARD_N,
	EAT_DOORWARD_S,
	EAT_DOORWARD_E,
	EAT_DOORWARD_W,

	EAT_COUNT
};


struct ProcessDungonTileArgs
{
	GraphicScene*	GS;
	PScene*			PS;
	PhysicsSystem*	Physics;
	RenderSystem*	RS;
	GeometryTable*	GT;
	Resources*		Assets;

	ColliderHandle	Colliders[ECA_COUNT];
};


void LoadColliders(PhysicsSystem* PS, Resources* Assets, ProcessDungonTileArgs* Out)
{
	Out->Colliders[ECA_DEADEND_N_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_DEADEND_N_COLLIDER);
	Out->Colliders[ECA_DEADEND_S_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_DEADEND_S_COLLIDER);
	Out->Colliders[ECA_DEADEND_E_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_DEADEND_E_COLLIDER);
	Out->Colliders[ECA_DEADEND_W_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_DEADEND_W_COLLIDER);
	Out->Colliders[ECA_CORNER_NE_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_CORNER_NE_COLLIDER);
	Out->Colliders[ECA_CORNER_NW_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_CORNER_NW_COLLIDER);
	Out->Colliders[ECA_CORNER_SE_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_CORNER_SE_COLLIDER);
	Out->Colliders[ECA_CORNER_SW_COLLIDER]  = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_CORNER_SW_COLLIDER);
	Out->Colliders[ECA_HALLWAY_NS_COLLIDER] = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_NS_COLLIDER);
	Out->Colliders[ECA_HALLWAY_EW_COLLIDER] = LoadTriMeshCollider(PS, Assets, EAG_CORRIDOR_EW_COLLIDER);
}


size_t CountNeighbor(DungeonGenerator::Tile in[3][3])
{
	int C = 0;

	if (in[0][1] != DungeonGenerator::Empty) C++; // Top
	if (in[2][1] != DungeonGenerator::Empty) C++; // Bottom
	if (in[1][0] != DungeonGenerator::Empty) C++; // left
	if (in[1][2] != DungeonGenerator::Empty) C++; // Right

	return C;
}


void ProcessDungeonTile(byte* _ptr, DungeonGenerator::Tile in[3][3], uint2 POS, uint2 BPOS, DungeonGenerator* D)
{
	ProcessDungonTileArgs* args = (ProcessDungonTileArgs*)_ptr;

	char NorthernBlock  = in[1][1];
	char SouthernBlock  = in[2][1];
	char EasternBlock   = in[1][2];
	char WesternBlock   = in[1][0];
	char CenterBlock    = in[1][1];
	char NeighborCount	= CountNeighbor(in);

	switch (in[1][1])
	{
		case DungeonGenerator::Tile::Connector:
		{
			int X = 0;
		}	break;
		case DungeonGenerator::Tile::Corridor:
		{	
			// Straight
			ETILEASSET_GUIDS	Visual = EAG_END;
			ColliderHandle		Collider = ColliderHandle(-1);

			switch (NeighborCount)
			{
			case 1: // Dead End
			{
				{
					ETILEASSET_GUIDS	Visual;
					ColliderHandle		Collider;
					if (NorthernBlock == DungeonGenerator::Tile::Corridor)
					{
						Visual		= ETILEASSET_GUIDS::EAG_DEADEND_N;
						Collider	= args->Colliders[ECA_DEADEND_N_COLLIDER];
					}
					else if (SouthernBlock == DungeonGenerator::Tile::Corridor)
					{
						Visual		= ETILEASSET_GUIDS::EAG_DEADEND_S;
						Collider	= args->Colliders[ECA_DEADEND_S_COLLIDER];
					}
					else if (EasternBlock == DungeonGenerator::Tile::Corridor)
					{
						Visual		= ETILEASSET_GUIDS::EAG_DEADEND_E;
						Collider	= args->Colliders[ECA_DEADEND_E_COLLIDER];
					}
					else if (WesternBlock == DungeonGenerator::Tile::Corridor)
					{
						Visual		= ETILEASSET_GUIDS::EAG_DEADEND_W;
						Collider	= args->Colliders[ECA_DEADEND_W_COLLIDER];
					}
				}	break;
			}
			case 2: // Straight or Corner
			{
				if (NorthernBlock == DungeonGenerator::Tile::Corridor && SouthernBlock == DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_CORRIDOR_EW;
					Collider	= args->Colliders[ECA_HALLWAY_EW_COLLIDER];
				}
				else if (WesternBlock == DungeonGenerator::Tile::Corridor && EasternBlock == DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_CORRIDOR_NS;
					Collider	= args->Colliders[ECA_HALLWAY_NS_COLLIDER];
				}
				// Corners
				else if (NorthernBlock == DungeonGenerator::Tile::Corridor && EasternBlock == DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_END;
					Collider	= args->Colliders[ECA_HALLWAY_NS_COLLIDER];
				}
				else if (NorthernBlock == DungeonGenerator::Tile::Corridor && WesternBlock == DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_END;
					Collider	= args->Colliders[ECA_HALLWAY_NS_COLLIDER];
				}
				else if (SouthernBlock == DungeonGenerator::Tile::Corridor && EasternBlock== DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_END;
					Collider	= args->Colliders[ECA_HALLWAY_NS_COLLIDER];
				}
				else if (SouthernBlock == DungeonGenerator::Tile::Corridor && WesternBlock == DungeonGenerator::Tile::Corridor)
				{
					Visual		= ETILEASSET_GUIDS::EAG_END;
					Collider	= args->Colliders[ECA_HALLWAY_NS_COLLIDER];
				}

				
			}	break;
			default:
				return;
			}

			if (Visual == EAG_END)
				return;

			float3 Position = float3(40, 0, 40) * float3(POS[1], 0, POS[0]);
			auto Model		= args->GS->CreateDrawableAndSetMesh(Visual);
			args->GS->SetPositionEntity_WT(Model, Position);
			CreateStaticActor(args->Physics, args->PS, Collider, Position);
		}	break;
		case DungeonGenerator::Tile::DoorWay:
		{	
			int X = 0;
		}	break;
		case DungeonGenerator::Tile::FLOOR_Dungeon:
		{	
			int X = 0;
		}	break;
		default:
		break;
	}
}


void CreateTestScene(EngineMemory* Engine, GameState* State, Scene* Out)
{
	auto Head			= State->GScene.CreateDrawableAndSetMesh("HeadModel");
	auto PlayerModel	= State->GScene.CreateDrawableAndSetMesh("PlayerModel");
	Out->TestModel		= State->GScene.CreateDrawableAndSetMesh("Flower");
	Out->PlayerModel	= PlayerModel;

	auto HeadNode = State->GScene.GetEntity(Head).Node;
	SetFlag(State->Nodes, HeadNode, SceneNodes::StateFlags::SCALE);
	Scale(State->Nodes, HeadNode, 10);

#if 1
	State->GScene.EntityEnablePosing(PlayerModel);
	State->GScene.EntityPlayAnimation(PlayerModel, 3, 1.1f);
	State->GScene.EntityPlayAnimation(PlayerModel, 4, 0.5f);
	Out->Joint = State->GScene.GetEntity(PlayerModel).PoseState->Sk->FindJoint("wrist_R");
	DEBUG_PrintSkeletonHierarchy(State->GScene.GetEntitySkeleton(PlayerModel));
	TagJoint(&State->GScene, Out->Joint, PlayerModel, State->GScene.GetNode(Out->TestModel));
#endif

	State->GScene.Yaw				(PlayerModel, pi / 2);
	State->GScene.SetMaterialParams	(PlayerModel, { 0.1f, 0.2f, 0.5f , 0.8f }, { 0.5f, 0.5f, 0.5f , 0.0f });
	State->DepthBuffer	= &Engine->DepthBuffer;

	TextureSet* Textures = LoadTextureSet(&Engine->Assets, 100, Engine->BlockAllocator);
	UploadTextureSet(Engine->RenderSystem, Textures, Engine->BlockAllocator);
	State->Set1 = Textures;

	uint2	WindowRect	= Engine->Window.WH;
	float	Aspect		= (float)WindowRect[0] / (float)WindowRect[1];
	InitiateCamera	(Engine->RenderSystem, State->Nodes, &Out->PlayerCam, Aspect, 0.01f, 10000.0f, true);
	SetActiveCamera	(State, &Out->PlayerCam);

	InitiateGameActor(5, 10, {0, 0, 0}, &State->PScene, &Engine->Physics, PlayerModel, &Out->PlayerActor);
	InitiateEntityController(&Out->PlayerController, &Engine->Nodes);

	BindActorToPlayerController(&Out->PlayerActor, &Out->PlayerController, &State->GScene);
	State->GScene.Yaw(PlayerModel, pi );

	NodeHandle LightNode = GetZeroedNode(&Engine->Nodes);

	SetParentNode	(State->Nodes,	Out->PlayerController.PitchNode, Out->PlayerCam.Node);
	SetParentNode	(State->Nodes,	Out->PlayerController.ModelNode, LightNode);

	TranslateLocal	(Engine->Nodes, Out->PlayerCam.Node, {0.0f, 20.0f, 00.0f});
	TranslateLocal	(Engine->Nodes, Out->PlayerCam.Node, {0.0f, 00.0f, 40.0f});
	TranslateLocal	(Engine->Nodes, LightNode,			 {0.0f, 40.0f, 0.0f });

	InitateMouseCameraController(
		0, 0, 75.0f, 
		&Out->PlayerCam, 
		Out->PlayerController.PitchNode, 
		Out->PlayerController.YawNode, 
		&Out->PlayerCameraController);

	Out->PlayerInertia.Drag		= 0.1f;
	Out->PlayerInertia.Inertia	= float3(0);
	Out->T						= 0.0f;

	State->GScene.AddPointLight({1, 1, 1}, LightNode, 1000, 1000);
	//LoadScene(Engine->RenderSystem, Engine->Nodes, &Engine->Assets, &Engine->Geometry, 200, &State->GScene, Engine->TempAllocator);
	
	/*
	//srand(123);
	//Out->Dungeon.Generate();
	//auto temp = Out->Dungeon.Tiles;

	ProcessDungonTileArgs Args;
	Args.Assets  = &Engine->Assets;
	Args.GS      = &State->GScene;
	Args.GT      = &Engine->Geometry;
	Args.RS      = &Engine->RenderSystem;
	Args.PS		 = &State->PScene;
	Args.Physics = &Engine->Physics;

	//LoadColliders(&Engine->Physics, &Engine->Assets, &Args);
	//Out->Dungeon.DungeonScanCallBack(ProcessDungeonTile, (byte*)&Args);
	*/

	InitiateScalp(Engine->RenderSystem, &Engine->Assets, INVALIDHANDLE, &Out->TestScalp, Engine->BlockAllocator);
}


void UpdateTestScene(Scene* TestScene,  GameState* State, double dt, iAllocator* TempMem)
{
	float3 InputMovement	= UpdateController(TestScene->PlayerController, State->Keys, State->Nodes);
	float3 Inertia			= UpdateActorInertia(&TestScene->PlayerInertia, dt, &TestScene->PlayerActor, InputMovement);

	TestScene->T += dt;

	Draw_RECT TestRect;
	TestRect.BLeft  = { 0.0f, 0.5f	};
	TestRect.TRight	= { 0.5, 0.9f };
	TestRect.Color  = float4(Gray((float)sin(TestScene->T * pi)), 1);

	PushRect(State->GUIRender, TestRect);

	UpdateGameActor(Inertia, &State->GScene, dt, &TestScene->PlayerActor, TestScene->PlayerController.Node);
	UpdateMouseCameraController(&TestScene->PlayerCameraController, State->Nodes, State->Mouse.dPos);
}


void UpdateTestScene_PostTransformUpdate(Scene* TestScene, GameState* State, double dt, iAllocator* TempMem)
{
	UpdateGraphicScenePoseTransform(&State->GScene);
	//DEBUG_DrawPoseState(Entity.PoseState, State->Nodes, Entity.Node, &State->LineDrawPass);
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
		
		AddResourceFile("assets\\ResourceFile.gameres", &Engine->Assets);
		State.ClearColor         = { 0.0f, 0.2f, 0.4f, 1.0f };
		State.Nodes		         = &Engine->Nodes;
		State.Quit		         = false;
		State.PhysicsUpdateTimer = 0.0f;
		State.Mouse				 = MouseInputState();
		State.DoDeferredShading  = true;
		State.ActiveWindow		 = &Engine->Window;

		ForwardPass_DESC FP_Desc{&Engine->DepthBuffer, &Engine->Window};
		DeferredPassDesc DP_Desc{&Engine->DepthBuffer, &Engine->Window, nullptr };

		State.GT = &Engine->Geometry;
		ResetStats(&State.Stats);

		InitiateForwardPass		  (Engine->RenderSystem, &FP_Desc, &State.ForwardPass);
		InitiateDeferredPass	  (Engine->RenderSystem, &DP_Desc, &State.DeferredPass);
		InitiateScene			  (&Engine->Physics, &State.PScene, Engine->BlockAllocator);
		InitiateGraphicScene	  (&State.GScene, Engine->RenderSystem, &Engine->Assets, &Engine->Nodes, &Engine->Geometry, Engine->BlockAllocator, Engine->TempAllocator);
		InitiateTextRender		  (Engine->RenderSystem, &State.TextRender);
		InitiateLineDrawState	  (Engine->RenderSystem, Engine->BlockAllocator, &State.LineDrawState);
		InitiateHairRender		  (Engine->RenderSystem, &Engine->DepthBuffer,   &State.HairRender);
		Initiate3DLinePass		  (Engine->RenderSystem, Engine->BlockAllocator, &State.LinePass);
		InitiateRenderGUI		  (Engine->RenderSystem, &State.GUIRender,		Engine->TempAllocator);
		//InitiateStaticMeshBatcher (Engine->RenderSystem, Engine->BlockAllocator, &State.StaticMeshBatcher);

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
		UpdateTestScene(&State->TestScene, State, dt, Engine->TempAllocator);
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
		UpdateTestScene_PostTransformUpdate(&State->TestScene, State, dt, TempMemory);
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

		State->Stats.AvgObjectDrawnPerFrame += PVS.size();
		State->Stats.AvgObjectDrawnPerFrame += Transparent.size();
		State->Stats.T += 1.0f/60.0f;

		if (State->Stats.T > 1.0f)
		{
			char	bar[256];
			sprintf_s(bar, "Objects Drawn per frame: %u \n", State->Stats.AvgObjectDrawnPerFrame/60);
			TextUtilities::ClearText(&State->Text);
			TextUtilities::PrintText(&State->Text, bar);
			ResetStats(&State->Stats);
		}


		// TODO: multi Thread these
		// Do Uploads
		{
			DeferredPass_Parameters	DPP;
			DPP.PointLightCount = State->GScene.PLights.size();
			DPP.SpotLightCount  = State->GScene.SPLights.size();

			UploadGUI(RS, &State->GUIRender);
			UploadPoses(RS, &PVS, State->GT, TempMemory);
			UploadLineSegments(RS, &State->LinePass);
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
				DoDeferredPass(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, nullptr, State->GT);
				DrawLandscape(RS, &State->Landscape, 15, State->ActiveCamera);
				ShadeDeferredPass(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, &State->GScene.PLights, &State->GScene.SPLights);
				DoForwardPass(&Transparent, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, State->GT);// Transparent Objects
			}
			else
			{
				DoForwardPass(&PVS, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, State->GT);
			}
		}

		DrawGUI(RS, CL, &State->GUIRender, State->ActiveWindow);
		Draw3DLineSegments(RS, &State->LineDrawState , &State->LinePass, State->ActiveCamera, State->ActiveWindow);
		DrawTextArea(&State->TextRender, State->Font, &State->Text, TempMemory, RS, State->ActiveWindow);
		
		EndPass(CL, RS, State->ActiveWindow);
	}


	GAMESTATEAPI void PostDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameState* _ptr)
	{
		PresentWindow(&Engine->Window, Engine->RenderSystem);
	}


	GAMESTATEAPI void Cleanup(EngineMemory* Engine, GameState* _ptr)
	{
		for (size_t I = 0; I < 3; ++I) {
			WaitforGPU(&Engine->RenderSystem);
			IncrementRSIndex(Engine->RenderSystem);
		}

		Engine->BlockAllocator.free(_ptr);
		
		FreeAllResourceFiles	(&Engine->Assets);
		FreeAllResources		(&Engine->Assets);

		ReleaseTextureSet(Engine->RenderSystem, _ptr->Set1, Engine->BlockAllocator);

		CleanUpRenderGUI		(&_ptr->GUIRender);
		CleanUpLineDrawPass		(&_ptr->LinePass);
		CleanUpLineDrawState	(&_ptr->LineDrawState);
		CleanUpState			(_ptr, Engine);
		CleanUpCamera			(_ptr->Nodes, _ptr->ActiveCamera);
		CleanUpTextRender		(&_ptr->TextRender);
		CleanupForwardPass		(&_ptr->ForwardPass);
		CleanupDeferredPass		(&_ptr->DeferredPass);
		CleanUpGraphicScene		(&_ptr->GScene);
		CleanupHairRender		(&_ptr->HairRender);
		CleanUpScene			(&_ptr->PScene, &Engine->Physics);
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
		CleanUpFN				Cleanup;
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
		Table.Cleanup			= &Cleanup;

		*out = *(CodeTable*)(&Table);
	}
}