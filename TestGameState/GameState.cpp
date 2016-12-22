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
//		(DONE) Scene Loading
//		Config loading system?
//
//	Graphics:
//		(DONE) Basic Gui rendering methods (Draw Rect, etc)
//		Multi-threaded Texture Uploads
//		Terrain Rendering
//			Texture Splatting
//			Height Mapping
//			Geometry Generation - partially complete
//		Occlusion Culling
//		Animation State Machine
//		(DONE/PARTLY) 3rd Person Camera Handler
//		(DONE) Object -> Bone Attachment
//		(DONE) Texture Loading
//		(DONE) Simple Window Utilities
//		(DONE) Simple Window Elements
//		(DONE) Deferred Rendering
//		(DONE) Forward Rendering
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
//		(DONE) Statics
//		(DONE) TriMesh Statics
//	Network:
//		Client:
//		Server:
//
//	Tools:
//		(DONE) Meta-Data for Resource Compiling
//

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif

#include "..\Application\GameMemory.h"
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
using FlexKit::LineSet;

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
	float  dr	= FlexKit::pi/2;	// Rotation Rate;
	float  dm	= 10;				// Movement Rate;
	float  r	= 3;				// Radius
	float  h	= 10;				// Height
	float3 POS	= {0, 0, 0};		// Foot Position
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
	int2	dPos			= {0, 0};
	float2	Position		= {0, 0};
	float2	NormalizedPos	= {0, 0};

	bool LeftButtonPressed	= false;
	bool Enabled			= false;
};


void UpdateMouseCameraController(MouseCameraController* out, SceneNodes* Nodes, int2 MouseState)
{
	out->Pitch	= max	(min(out->Pitch + float(MouseState[1]) / 2, 25), -45);
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


	if (Actor->BPC.FloorContact)
		IS->Inertia.y = 0;
	else
		IS->Inertia += dt * G;
	
	if (IS->Inertia.magnitudesquared() > 0.0001f)
		return IS->Inertia;
	else
		return {0.0f, 0.0f, 0.0f};
}


float3 UpdateGameActor(float3 dV, GraphicScene* Scene, double dt, GameActor* ActorState, NodeHandle Target)
{ 
	float3 FinalDelta = dV;
	FinalDelta *= ActorState->BPC.FloorContact ? float3(1, 0, 1) : float3(1);

	if (FinalDelta.magnitude() > 0.01f)
	{
		float3 OldP = GetActorPosition(ActorState);
		TranslateActor(FinalDelta, ActorState, dt);
		float3 NewP = GetActorPosition(ActorState);
		SetPositionW( Scene->SN, Target, NewP);
		Scene->GetDrawable(ActorState->Drawable).Dirty = true;

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


void ClearMouseButtonStates(MouseInputState* State)
{
	State->LeftButtonPressed = false;
}


void UpdateMouseInput(MouseInputState* State, RenderWindow* Window)
{
	using FlexKit::int2;

	if ( !State->Enabled )
		return;

	if ( GetForegroundWindow() == Window->hWindow )
	{
		State->dPos		 = GetMousedPos(Window);
		State->Position.x -= State->dPos[0];
		State->Position.y += State->dPos[1];

		State->Position[0] = max(0, min(State->Position[0], Window->WH[0]));
		State->Position[1] = max(0, min(State->Position[1], Window->WH[1]));

		State->NormalizedPos[0] = max(0, min(State->Position[0] / Window->WH[0], 1));
		State->NormalizedPos[1] = max(0, min(State->Position[1] / Window->WH[1], 1));

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
	Shader PS = LoadShader("DebugTerrainPaint",	"DebugTerrainPaint",	"ps_5_0", "assets\\pshader.hlsl");

	// Create Pipeline State Objects
	{
		// Simulation Step
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {}; {
				Desc.CS				= CS;
				Desc.Flags			= D3D12_PIPELINE_STATE_FLAG_NONE;
				Desc.NodeMask		= 0;
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
				Depth_Desc.DepthFunc	= D3D12_COMPARISON_FUNC::D3D12_COMPARISON_FUNC_GREATER_EQUAL;
				Depth_Desc.DepthEnable	= false;
			}

			static_vector<D3D12_INPUT_ELEMENT_DESC> InputElements = {
				{ "POSITION",	0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};



			D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {}; {
				Desc.pRootSignature = RS->Library.RS4CBVs4SRVs;
				Desc.VS = VS;
				Desc.HS = HS;
				Desc.DS = DS;
				//Desc.GS						= GS; // Skipping for Now
				Desc.PS = PS;
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

	Destroy(CS);
	Destroy(VS);
	Destroy(HS);
	Destroy(DS);
	Destroy(PS);
}


/************************************************************************************************/


void CleanupHairRender(HairRender* Out)
{
	//Out->Draw.PSO->Release();
	//Out->Simulate.PSO->Release();
}


/************************************************************************************************/


bool InitiateTextureVTable(RenderSystem* RS, TextureVTable_Desc Desc, TextureVTable* Out)
{
	FK_ASSERT(Out != nullptr ,				"INVALID ARGUEMENT!");
	FK_ASSERT(RS != nullptr,				"INVALID ARGUEMENT!");
	FK_ASSERT(Desc.Memory != nullptr,		"INVALID ARGUEMENT!");
	FK_ASSERT(Desc.PageSize.Product() > 0,	"INVALID ARGUEMENT!");
	FK_ASSERT(Desc.PageCount.Product() > 0, "INVALID ARGUEMENT!");
	FK_ASSERT(Desc.Resources != nullptr,	"INVALID ARGUEMENT!");

	D3D12_RESOURCE_DESC	Resource_DESC = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, Desc.PageSize[0] * Desc.PageCount[0], Desc.PageSize[1] * Desc.PageCount[1]);

	D3D12_HEAP_PROPERTIES HEAP_Props = {}; {
		HEAP_Props.CPUPageProperty			= D3D12_CPU_PAGE_PROPERTY::D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HEAP_Props.Type						= D3D12_HEAP_TYPE_DEFAULT;
		HEAP_Props.MemoryPoolPreference		= D3D12_MEMORY_POOL::D3D12_MEMORY_POOL_UNKNOWN;
		HEAP_Props.CreationNodeMask			= 0;
		HEAP_Props.VisibleNodeMask			= 0;
	}

	ID3D12Resource* Resource = nullptr;
	HRESULT HR = RS->pDevice->CreateCommittedResource(&HEAP_Props, D3D12_HEAP_FLAG_NONE, &Resource_DESC, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Resource));
	// TODO: Resource Eviction on creation failure
	CheckHR(HR, ASSERTONFAIL("FAILED TO CREATE RESOURCE!"));

	size_t PageCount = Desc.PageCount.Product();
	Out->PageTable = (TextureVTable::TableEntry*)(Desc.Memory->_aligned_malloc( sizeof(TextureVTable::TableEntry) * PageCount) );
	Out->PageTableDimensions = Desc.PageCount;

	for (size_t I = 0; I < PageCount; ++I)
		Out->PageTable[I] = { INVALIDHANDLE, { 0.0f, 0.0f } };

	Texture2D_Desc TextureDesc = {};
	TextureDesc.CV			   = false;
	TextureDesc.MipLevels	   = 0;
	TextureDesc.UAV			   = false;
	TextureDesc.Read		   = false;
	TextureDesc.Write		   = true;
	TextureDesc.Width		   = Desc.PageCount[0] * Desc.PageSize[0];
	TextureDesc.Height		   = Desc.PageCount[1] * Desc.PageSize[1];
	TextureDesc.Format		   = FORMAT_2D::R8G8B8A8_UNORM;
	
	auto TextureMemory = CreateTexture2D(RS, &TextureDesc);
	SETDEBUGNAME(TextureMemory, "TextureMemory");
	
	Out->Memory					= Desc.Memory;
	Out->TextureMemory			= TextureMemory;
	Out->TextureTable.Allocator	= Desc.Memory;
	Out->TextureSets.Allocator	= Desc.Memory;

	return true;
}


/************************************************************************************************/


void CleanUpTextureVTable(TextureVTable* Table)
{
	Table->Constants.Release();
	Table->ReadBackBuffer.Release();
	Table->TextureMemory->Release();
	Table->Memory->_aligned_free(Table->PageTable);

	for (auto E : Table->TextureSets)
		ReleaseTextureSet(E, Table->Memory);

	Table->TextureTable.Release();

	for (auto E : Table->PageTables)
		E->Release();

	Table->PageTables.Release();
}


/************************************************************************************************/


bool isTextureSetLoaded(GUID_t ID, TextureVTable* Table) 
{
	for (auto I : Table->TextureTable) 
		if (I.ResourceID == ID) 
			return true;

	return false;
}


/************************************************************************************************/


void AddTextureResource(RenderSystem* RS, TextureVTable* Table, GUID_t TextureSet)
{
	if (!isTextureSetLoaded(TextureSet, Table)) 
	{
		auto Set = LoadTextureSet(Table->ResourceTable, TextureSet, Table->Memory);

		for ( auto I : Set->TextureGuids) 
			if(I != INVALIDHANDLE)
				Table->TextureTable.push_back({INVALIDHANDLE, I});
	}
}


/************************************************************************************************/


void FindUsedTexturePages(RenderSystem* RS, TextureVTable* Table, GeometryTable* GT, Camera* C, PVS* PVS) 
{
	// Setup Pipeline State
	auto CL				= GetCommandList_1					(RS);
	auto FrameResources	= GetCurrentFrameResources			(RS);

	auto DescPOSGPU		= GetDescTableCurrentPosition_GPU	(RS); // _Ptr to Beginning of Heap On GPU
	auto DescPOS		= ReserveDescHeap					(RS, 6);
	auto DescriptorHeap	= GetCurrentDescriptorTable			(RS);

	auto RTVPOSCPU		= GetRTVTableCurrentPosition_CPU	(RS); // _Ptr to Current POS On RTV heap on CPU
	auto RTVPOS			= ReserveRTVHeap					(RS, 6);
	auto RTVHeap		= GetCurrentRTVTable				(RS);

	auto DSVPOSCPU		= GetDSVTableCurrentPosition_CPU	(RS); // _Ptr to Current POS On DSV heap on CPU
	auto DSVPOS			= ReserveDSVHeap					(RS, 1);
	auto DSVHeap		= GetCurrentRTVTable				(RS);

	CL->SetGraphicsRootSignature			(RS->Library.RS2UAVs4SRVs4CBs);
	CL->SetPipelineState					(Table->UpdatePSO);
	CL->SetGraphicsRootConstantBufferView	(VTP_CameraConstants, C->Buffer->GetGPUVirtualAddress());


	for (auto P : *PVS)
	{
		Drawable* E = P;

		if (E->Posed || !E->Textured)
			continue;

		TriMeshHandle CurrentHandle	= E->MeshHandle;

		if (CurrentHandle == INVALIDMESHHANDLE)
			continue;

		TriMesh*	CurrentMesh			= GetMesh(GT, CurrentHandle);
		TextureSet* CurrentTextureSet	= E->Textures;

		size_t IBIndex	  = CurrentMesh->VertexBuffer.MD.IndexBuffer_Index;
		size_t IndexCount = CurrentMesh->IndexCount;

		if (CurrentTextureSet == nullptr)
			continue;

		/*
			typedef struct D3D12_INDEX_BUFFER_VIEW
			{
			D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
			UINT SizeInBytes;
			DXGI_FORMAT Format;
			} 	D3D12_INDEX_BUFFER_VIEW;
		*/

		D3D12_INDEX_BUFFER_VIEW	IndexView = {
			GetBuffer(CurrentMesh, IBIndex)->GetGPUVirtualAddress(),
			UINT(IndexCount * 32),
			DXGI_FORMAT::DXGI_FORMAT_R32_UINT,
		};

		static_vector<D3D12_VERTEX_BUFFER_VIEW> VBViews;
		FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_POSITION,	CurrentMesh, VBViews));
		FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_UV,			CurrentMesh, VBViews));
		FK_ASSERT(AddVertexBuffer(VERTEXBUFFER_TYPE::VERTEXBUFFER_TYPE_NORMAL,		CurrentMesh, VBViews));

		CL->SetGraphicsRootConstantBufferView	(VTP_ShadingConstants, E->VConstants->GetGPUVirtualAddress());
		CL->IASetIndexBuffer					(&IndexView);
		CL->IASetVertexBuffers					(0, VBViews.size(), VBViews.begin());
		CL->DrawIndexedInstanced				(IndexCount, 1, 0, 0, 0);
	}
}


/************************************************************************************************/


void UpdateTextureResources(RenderSystem* RS, TextureVTable* Table, GeometryTable* GT, Camera* C, PVS* PVS)
{
	FindUsedTexturePages(RS, Table, GT, C, PVS); // Queue for Next Frame
}


/************************************************************************************************/


void UploadTextureResources(RenderSystem* RS, TextureVTable* Out)
{
	// ReadBack Results

}


/************************************************************************************************/


struct GUICallbackArgs
{
	SimpleWindow*	Window;
	TextArea*		Text;
};

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
	GUIElementHandle		Slider;
	DAConditionHandle		Condition1;
	DAConditionHandle		Condition2;

	Texture2D			TestTexture;
	DungeonGenerator	Dungeon;
	SimpleWindow		Window;

	AnimationStateMachine	ASM;
	GUICallbackArgs			SliderArgs;
};


/************************************************************************************************/


struct GameState
{
	Scene TestScene;

	FontAsset*	Font;
	TextArea	Text;

	TextureSet*		Set1;
	TextureVTable	TextureState;

	DeferredPass		DeferredPass;
	ForwardPass			ForwardPass;
	ShadowMapPass		ShadowMapPass;

	GUIRender			GUIRender;

	LineSet	Lines;

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
	Texture2D			ShadowMap;

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

	EAG_WALL_NS             = 512,
	EAG_WALL_NS_COLLIDER    = 513,
	EAG_WALL_EW             = 514,
	EAG_WALL_EW_COLLIDER    = 515,

	EAG_DOOR_N              = 516,
	EAG_DOOR_N_COLLIDER     = 517,
	EAG_DOOR_S              = 518,
	EAG_DOOR_S_COLLIDER     = 519,
	EAG_DOOR_E              = 520,
	EAG_DOOR_E_COLLIDER     = 521,
	EAG_DOOR_W              = 522,
	EAG_DOOR_W_COLLIDER     = 523,

	EAG_DEADEND_N			= 524,
	EAG_DEADEND_N_COLLIDER	= 525,
	EAG_DEADEND_S			= 526,
	EAG_DEADEND_S_COLLIDER	= 527,
	EAG_DEADEND_E			= 528,
	EAG_DEADEND_E_COLLIDER	= 529,
	EAG_DEADEND_W			= 530,
	EAG_DEADEND_W_COLLIDER	= 531,

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
			case 3: // T-Junction
			{

			}
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
			GS->Keys.Forward = false; break;
		case FlexKit::KC_S:
			GS->Keys.Backward = false; break;
		case FlexKit::KC_A:
			GS->Keys.Left = false; break;
		case FlexKit::KC_D:
			GS->Keys.Right = false; break;
		case FlexKit::KC_M:
		{
			SetSystemCursorToWindowCenter(GS->ActiveWindow);
			if (GS->Keys.MouseEnable)
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
			GS->Keys.Forward = true; break;
		case FlexKit::KC_S:
			GS->Keys.Backward = true; break;
		case FlexKit::KC_A:
			GS->Keys.Left = true; break;
		case FlexKit::KC_D:
			GS->Keys.Right = true; break;
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
	switch (e.Action)
	{
	case Event::Moved:
		break;
	case Event::Pressed:
		if (e.mData1.mKC[0] == KC_MOUSELEFT)
			GS->Mouse.LeftButtonPressed = true;

		break;
	case Event::Release:
		break;
	case Event::Resized:
		break;
	default:
		break;
	}
}


void SetActiveCamera(GameState* State, Camera* _ptr) { State->ActiveCamera = _ptr; }

void PrintOnClick(void* _ptr, size_t GUIElement)
{
	GUICallbackArgs* Args = (GUICallbackArgs*)_ptr;
	PrintText(Args->Text, "Clicked BUTTON\n");
	std::cout << "Clicked BUTTON\n";
}

void PrintOnEnter(void* _ptr, size_t GUIElement)
{
	GUICallbackArgs* Args = (GUICallbackArgs*)_ptr;
	PrintText(Args->Text, "ENTERED BUTTON\n");
	std::cout << "ENTERED BUTTON\n";
}

void PrintOnExit(void* _ptr, size_t GUIElement)
{
	GUICallbackArgs* Args = (GUICallbackArgs*)_ptr;
	PrintText(Args->Text, "LEFT BUTTON\n");
	std::cout << "LEFT BUTTON\n";
}

void SliderBarClicked(float r, void* _ptr, size_t GUIElement)
{
	GUICallbackArgs* Args = (GUICallbackArgs*)_ptr;
	PrintText(Args->Text, "Slider Clicked\n");
	std::cout << "Slider Clicked\n";
}

void CreateTestScene(EngineMemory* Engine, GameState* State, Scene* Out)
{
	//auto Head			 = State->GScene.CreateDrawableAndSetMesh("HeadModel");
	auto PlayerModel	 = State->GScene.CreateDrawableAndSetMesh("PlayerModel");
	Out->TestModel		 = State->GScene.CreateDrawableAndSetMesh("Flower");
	Out->PlayerModel	 = PlayerModel;
	Out->SliderArgs.Text = &State->Text;

	//auto HeadNode = State->GScene.GetDrawable(Head).Node;
	//SetFlag(State->Nodes, HeadNode, SceneNodes::StateFlags::SCALE);
	//Scale(State->Nodes, HeadNode, 10);


#if 1
	State->GScene.EntityEnablePosing(PlayerModel);
	//int64_t ID = State->GScene.EntityPlayAnimation(PlayerModel, 3, 1.0f, true);
	//SetAnimationWeight(State->GScene.GetEntityAnimationState(PlayerModel), ID, 0.5f);
	//State->GScene.EntityPlayAnimation(PlayerModel, 4, 0.5f);
	Out->Joint = State->GScene.GetDrawable(PlayerModel).PoseState->Sk->FindJoint("wrist_R");
	DEBUG_PrintSkeletonHierarchy(State->GScene.GetEntitySkeleton(PlayerModel));
	BindJoint(&State->GScene, Out->Joint, PlayerModel, State->GScene.GetNode(Out->TestModel));

	{
		InitiateASM(&Out->ASM, Engine->BlockAllocator, PlayerModel);

		AnimationCondition_Desc  Condition_Desc; {
			Condition_Desc.InputType = ASC_BOOL;
			Condition_Desc.Operation = EASO_TRUE;
		}

		AnimationStateEntry_Desc Forward_Desc;{
			Forward_Desc.Animation			= 3;
			Forward_Desc.EaseOutDuration	= 0.2f;
			Forward_Desc.Out				= EWF_Square;
			Forward_Desc.Loop				= true;
			Forward_Desc.ForceComplete		= false;
		}

		AnimationStateEntry_Desc Backward_Desc;{
			Backward_Desc.Animation	= 3;
			Backward_Desc.Loop		= false;
		}

		auto State1 = DASAddState(Forward_Desc, &Out->ASM);
		auto State2 = DASAddState(Backward_Desc, &Out->ASM);

		Condition_Desc.DrivenState = State1;
		auto Condition1 = DASAddCondition(Condition_Desc, &Out->ASM);
		Out->Condition1 = Condition1;

		Condition_Desc.DrivenState = State2;
		auto Condition2 = DASAddCondition(Condition_Desc, &Out->ASM);
		Out->Condition2 = Condition2;
	}

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
	TranslateLocal	(Engine->Nodes, LightNode,			 {0.0f, 80.0f,  0.0f});

	InitateMouseCameraController(
		0, 0, 75.0f, 
		&Out->PlayerCam, 
		Out->PlayerController.PitchNode, 
		Out->PlayerController.YawNode, 
		&Out->PlayerCameraController);

	Out->PlayerInertia.Drag		= 0.1f;
	Out->PlayerInertia.Inertia	= float3(0);
	Out->T						= 0.0f;

	auto Light = State->GScene.AddSpotLight(float3{0, 80, 0}, { 1, 1, 1 }, { 0.0f, -1.0f, 0.0f }, pi/2, 1000, 1000);
	State->GScene.EnableShadowCasting(Light);

	auto Texture2D		= LoadTextureFromFile("Assets//textures//agdg.dds", Engine->RenderSystem, Engine->BlockAllocator);
	Out->TestTexture	= Texture2D;
	
	SimpleWindow_Desc Desc(0.3f, 0.3f, 3, 3, (float)Engine->Window.WH[0] / (float)Engine->Window.WH[1]);
	InitiateSimpleWindow(Engine->BlockAllocator, &Out->Window, Desc);

#if 0

	GUIList List_Desc;
	List_Desc.Color		= float4(WHITE, 1);
	List_Desc.Position	= { 0, 0 };
	List_Desc.WH		= { 3, 2 };

	auto Column = SimpleWindowAddVerticalList(&Out->Window, List_Desc);

	float2 BorderSize = GetPixelSize(State->ActiveWindow) * float2{ 10.0f, 10.0f };
	SimpleWindowAddDivider(&Out->Window, BorderSize, Column);

	List_Desc.WH		= { 3, 1 };
	List_Desc.Position	= { 0, 1 };
	auto Row = SimpleWindowAddHorizonalList(&Out->Window, List_Desc, Column);

	GUITexturedButton_Desc	TexturedButton;
	TexturedButton.OnEntered_CB = PrintOnEnter;
	TexturedButton.OnExit_CB	= PrintOnExit;
	TexturedButton.OnClicked_CB	= PrintOnClick;
	TexturedButton.CB_Args		= &Out->SliderArgs;

	TexturedButton.SetButtonSizeByPixelSize(GetPixelSize(State->ActiveWindow), { 50, 50 });
	TexturedButton.Texture_Active = TexturedButton.Texture_InActive = &Out->TestTexture;

	GUITextButton_Desc	TextButton_Desc;
	TextButton_Desc.OnEntered_CB = PrintOnEnter;
	TextButton_Desc.OnExit_CB	 = PrintOnExit;
	TextButton_Desc.OnClicked_CB = PrintOnClick;
	TextButton_Desc.CB_Args		 = &Out->SliderArgs;
	TextButton_Desc.Text		 = "This is a Button!";
	TextButton_Desc.Font		 = State->Font;
	TextButton_Desc.WH			 = GetPixelSize(State->ActiveWindow) * float2 { 128.0f, 32.0f };
	TextButton_Desc.WindowSize	 = float2{ float(State->ActiveWindow->WH[0]), float(State->ActiveWindow->WH[1]) };

	GUITextInput_Desc TextInput_Desc;
	TextInput_Desc.SetTextBoxSizeByPixelSize(GetPixelSize(State->ActiveWindow), { 128, 32 });
	TextInput_Desc.Text = nullptr;		// Initial Text
	TextInput_Desc.Font = State->Font;


	SimpleWindowAddTextInput(&Out->Window, TextInput_Desc, Engine->RenderSystem, Column);

	GUISlider_Desc Slider_Desc;
	Slider_Desc.BarRatio		= 0.3f;
	Slider_Desc.WH				= GetPixelSize(State->ActiveWindow) * float2 { 256.0f, 10.0f };
	Slider_Desc.InitialPosition = -10.0f;
	Slider_Desc.OnClicked_CB	= SliderBarClicked;
	Slider_Desc.CB_Args			= &Out->SliderArgs;

	SimpleWindowAddDivider			(&Out->Window, BorderSize, Row);
	
	Out->Slider = SimpleWindowAddHorizontalSlider	(&Out->Window, Slider_Desc, Row);

	SimpleWindowAddDivider(&Out->Window, BorderSize, Row);
	SimpleWindowAddTextButton		(&Out->Window, TextButton_Desc, Engine->RenderSystem, Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize, Row);

	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Column);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Column);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Column);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Column);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Column);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Column);

	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Row);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Row);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Row);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Row);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);
	SimpleWindowAddDivider			(&Out->Window, BorderSize,		Row);
	SimpleWindowAddTexturedButton	(&Out->Window, TexturedButton,	Row);

#endif

	//LoadScene(Engine->RenderSystem, Engine->Nodes, &Engine->Assets, &Engine->Geometry, 200, &State->GScene, Engine->TempAllocator);

#if 0 
	srand(123);
	Out->Dungeon.Generate();
	auto temp = Out->Dungeon.Tiles;

	ProcessDungonTileArgs Args;
	Args.Assets  = &Engine->Assets;
	Args.GS      = &State->GScene;
	Args.GT      = &Engine->Geometry;
	Args.RS      = &Engine->RenderSystem;
	Args.PS		 = &State->PScene;
	Args.Physics = &Engine->Physics;

	LoadColliders(&Engine->Physics, &Engine->Assets, &Args);
	Out->Dungeon.DungeonScanCallBack(ProcessDungeonTile, (byte*)&Args);
#endif
	//InitiateScalp(Engine->RenderSystem, &Engine->Assets, INVALIDHANDLE, &Out->TestScalp, Engine->BlockAllocator);
}


void UpdateTestScene(Scene* TestScene,  GameState* State, double dt, iAllocator* TempMem)
{
	ClearLineSet(&State->Lines);

	float3 InputMovement	= UpdateController(TestScene->PlayerController, State->Keys, State->Nodes);
	float3 Inertia			= UpdateActorInertia(&TestScene->PlayerInertia, dt, &TestScene->PlayerActor, InputMovement);

	TestScene->T += dt;

	Draw_TEXTURED_RECT TexturedRect;
	TexturedRect.BLeft	       = { 0.9f, 0.9f - 0.05625f };
	TexturedRect.TRight        = { 1.0f, 1.0f };
	TexturedRect.Color         = float4(WHITE, 1.0f);
	TexturedRect.TextureHandle = &TestScene->TestTexture;

	PushRect(State->GUIRender, TexturedRect);

	UpdateGameActor				(Inertia, &State->GScene, dt, &TestScene->PlayerActor, TestScene->PlayerController.Node);
	UpdateMouseCameraController	(&TestScene->PlayerCameraController, State->Nodes, State->Mouse.dPos);

	PushLineSet(State->GUIRender, { &State->Lines, State->ActiveCamera });

	SimpleWindowInput Input;
	Input.MousePosition			 = State->Mouse.NormalizedPos;
	Input.LeftMouseButtonPressed = State->Mouse.LeftButtonPressed;

	if (Input.LeftMouseButtonPressed)
		PrintText(&State->Text, "MouseButtonPressed!\n");

	if (State->Keys.Forward || State->Keys.Backward || State->Keys.Left || State->Keys.Right)
		ASSetBool(TestScene->Condition1, true, &TestScene->ASM);
	else
		ASSetBool(TestScene->Condition1, false, &TestScene->ASM);

	UpdateSimpleWindow(&Input, &TestScene->Window);

	//SetSliderPosition((1 + sin(TestScene->T)) / 2, TestScene->Slider, &TestScene->Window);

	DrawSimpleWindow(Input, &TestScene->Window, &State->GUIRender);
}


void UpdateTestScene_PostTransformUpdate(Scene* TestScene, GameState* State, double dt, iAllocator* TempMem)
{
	UpdateGraphicScenePoseTransform(&State->GScene);

	auto& Entity = State->GScene.GetDrawable(TestScene->PlayerModel);
	DEBUG_DrawPoseState		(Entity.PoseState, State->Nodes, Entity.Node, &State->Lines);

	auto C = &State->GScene.SpotLightCasters[0].C;
	auto P = GetPositionW(State->Nodes, C->Node);
	Quaternion Q;
	GetOrientation(State->Nodes, C->Node, &Q);
	DEBUG_DrawCameraFrustum	(&State->Lines, C, P, Q);
}


void KeyEventsWrapper(const Event& in, void* _ptr)
{ 
	switch (in.InputSource)
	{
	case Event::Keyboard:
		HandleKeyEvents(in, reinterpret_cast<GameState*>(_ptr));
	case Event::Mouse:
		break;
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
		//InitiateHairRender		  (Engine->RenderSystem, &Engine->DepthBuffer,   &State.HairRender);
		InitiateLineSet			  (Engine->RenderSystem, Engine->BlockAllocator, &State.Lines);
		InitiateDrawGUI			  (Engine->RenderSystem, &State.GUIRender,		  Engine->TempAllocator);
		InitiateStaticMeshBatcher (Engine->RenderSystem, Engine->BlockAllocator, &State.StaticMeshBatcher);
		InitiateShadowMapPass	  (Engine->RenderSystem, &State.ShadowMapPass);
		InitiateTextureVTable	  (Engine->RenderSystem, {{128, 128}, {32, 32}, &Engine->Assets, Engine->BlockAllocator }, &State.TextureState);

		{
			Landscape_Desc Land_Desc = { 
				State.DeferredPass.Filling.NoTexture.Blob->GetBufferPointer(), 
				State.DeferredPass.Filling.NoTexture.Blob->GetBufferSize() 
			};

			InitiateLandscape		(Engine->RenderSystem, GetZeroedNode(&Engine->Nodes), &Land_Desc, Engine->BlockAllocator, &State.Landscape);
			PushRegion				(&State.Landscape, {{0, 0, 0, 2048}, {}, 0});
			UploadLandscape			(Engine->RenderSystem, &State.Landscape, nullptr, nullptr, true, false);
		}

		State.Font = LoadFontAsset	("assets\\fonts\\", "fontTest.fnt", Engine->RenderSystem, Engine->TempAllocator, Engine->BlockAllocator);

		auto WH = State.ActiveWindow->WH;
		TextArea_Desc TA_Desc = { { 0, 0 },{ float(WH[0]), float(WH[1]) }, { 32, 32 } };

		State.Text = CreateTextArea	(&Engine->RenderSystem, Engine->BlockAllocator, &TA_Desc);

		PrintText(&State.Text, "TEST BUILD: COLLECTING STATS\n");

		CreatePlaneCollider		(Engine->Physics.DefaultMaterial, &State.PScene);
		CreateTestScene			(Engine, &State, &State.TestScene);

		FlexKit::EventNotifier<>::Subscriber sub;
		sub.Notify = &KeyEventsWrapper;
		sub._ptr   = &State;
		Engine->Window.Handler.Subscribe(sub);
		sub.Notify = &MouseEventsWrapper;
		Engine->Window.Handler.Subscribe(sub);

		{
			FlexKit::Texture2D_Desc Desc; {
				Desc.Format			= FlexKit::FORMAT_2D::D32_FLOAT;
				Desc.Width			= 800;
				Desc.Height			= 600;
				Desc.RenderTarget	= false;
				Desc.FLAGS			= SPECIALFLAGS::DEPTHSTENCIL;
				Desc.CV				= 0.0f;
				Desc.initialData	= nullptr;
			}
			State.ShadowMap = CreateTexture2D(Engine->RenderSystem, &Desc);
			int c = 0;
		}

		UploadResources(&Engine->RenderSystem);// Uploads fresh Resources to GPU

		return &State;
	}


	void CleanUpState(GameState* Scene, EngineMemory* Engine)
	{
		FreeFontAsset(Scene->Font);
		CleanUpTextArea(&Scene->Text, Engine->BlockAllocator);
		CleanUpTerrain(Scene->Nodes, &Scene->Landscape);

		Engine->BlockAllocator.free(Scene->Font);
	}


	GAMESTATEAPI void Update(EngineMemory* Engine, GameState* State, double dt)
	{
		Engine->End = State->Quit;
		State->Mouse.Enabled = State->Keys.MouseEnable;

		UpdateScene		(&State->PScene, 1.0f/60.0f, nullptr, nullptr, nullptr );
		UpdateColliders	(&State->PScene, &Engine->Nodes);
	}


	GAMESTATEAPI void UpdateFixed(EngineMemory* Engine, double dt, GameState* State)
	{
		UpdateMouseInput(&State->Mouse, &Engine->Window);
		UpdateTestScene(&State->TestScene, State, dt, Engine->TempAllocator);
		UpdateGraphicScene(&State->GScene);

		UpdateASM(dt, &State->TestScene.ASM, Engine->TempAllocator, Engine->BlockAllocator, &State->GScene);

		ClearMouseButtonStates(&State->Mouse);
	}


	GAMESTATEAPI void UpdateAnimations(RenderSystem* RS, iAllocator* TempMemory, double dt, GameState* _ptr)
	{
		UpdateAnimationsGraphicScene(&_ptr->GScene, dt);
	}


	GAMESTATEAPI void UpdatePreDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameState* State)
	{
		UpdateTransforms	(State->Nodes);
		UpdateCamera		(Engine->RenderSystem, State->Nodes, State->ActiveCamera, dt);
		UpdateTestScene_PostTransformUpdate(&State->TestScene, State, dt, TempMemory);
	}


	GAMESTATEAPI void Draw(RenderSystem* RS, iAllocator* TempMemory, FlexKit::ShaderTable* M, GameState* State)
	{
		BeginSubmission(RS, State->ActiveWindow);

		auto PVS			= TempMemory->allocate_aligned<FlexKit::PVS>();
		auto Transparent	= TempMemory->allocate_aligned<FlexKit::PVS>();
		auto CL				= GetCurrentCommandList(RS);

		GetGraphicScenePVS(&State->GScene, State->ActiveCamera, &PVS, &Transparent);

		SortPVS(State->Nodes, &PVS, State->ActiveCamera);
		SortPVSTransparent(State->Nodes, &Transparent, State->ActiveCamera);


		State->Stats.AvgObjectDrawnPerFrame += PVS.size();
		State->Stats.AvgObjectDrawnPerFrame += Transparent.size();
		State->Stats.T += 1.0f/60.0f;

		if (State->Stats.T > 1.0f)
		{
			char	bar[256];
			sprintf_s(bar, "Objects Drawn per frame: %u \n", State->Stats.AvgObjectDrawnPerFrame/60);
			ClearText(&State->Text);
			PrintText(&State->Text, bar);

			ResetStats(&State->Stats);
		}

		PushText( State->GUIRender, { { 0.0f, 0.0f },{ 1.0f, 1.0f },{ WHITE, 1.0f }, &State->Text, State->Font });

		// TODO: multi Thread these
		// Do Uploads
		{
			DeferredPass_Parameters	DPP;
			DPP.PointLightCount = State->GScene.PLights.size();
			DPP.SpotLightCount  = State->GScene.SPLights.size();

			UploadGUI	(RS, &State->GUIRender, TempMemory, State->ActiveWindow);
			UploadPoses	(RS, &PVS, State->GT, TempMemory);

			UploadLineSegments			(RS, &State->Lines);
			UploadDeferredPassConstants	(RS, &DPP, {0.1f, 0.1f, 0.1f, 0}, &State->DeferredPass);

			UploadCamera			(RS, State->Nodes, State->ActiveCamera, State->GScene.PLights.size(), State->GScene.SPLights.size(), 0.0f, State->ActiveWindow->WH);
			UploadGraphicScene		(&State->GScene, &PVS, &Transparent);
			UploadLandscape			(RS, &State->Landscape, State->Nodes, State->ActiveCamera, false, true);
			UploadTextureResources	(RS, &State->TextureState);
		}


		// Submission
		{
			//SetDepthBuffersWrite(RS, CL, { GetCurrent(State->DepthBuffer) });

			ClearBackBuffer		 (RS, CL, State->ActiveWindow, { 0, 0, 0, 0 });
			ClearDepthBuffers	 (RS, CL, { GetCurrent(State->DepthBuffer) }, DefaultClearDepthValues_0);
			//ClearDepthBuffer(RS, CL, &State->ShadowMap, 0.0f, 0);

			Texture2D BackBuffer = GetBackBufferTexture(State->ActiveWindow);
			SetViewport	(CL, BackBuffer);
			SetScissor	(CL, BackBuffer.WH);

			//UpdateTextureResources(RS, &State->TextureState, State->ActiveCamera, &PVS);
			//UpdateTextureResources(RS, &State->TextureState, State->ActiveCamera, &SortPVSTransparent);

			if (State->DoDeferredShading)
			{
				IncrementDeferredPass (&State->DeferredPass);
				ClearDeferredBuffers  (RS, &State->DeferredPass);

				DoDeferredPass		(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, nullptr, State->GT, &State->TextureState);
				//DrawLandscape		(RS, &State->Landscape, &State->DeferredPass, 15, State->ActiveCamera);

				ShadeDeferredPass	(&PVS, &State->DeferredPass, GetRenderTarget(State->ActiveWindow), RS, State->ActiveCamera, &State->GScene.PLights, &State->GScene.SPLights);
				DoForwardPass		(&Transparent, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, State->GT);// Transparent Objects
			}
			else
				DoForwardPass(&PVS, &State->ForwardPass, RS, State->ActiveCamera, State->ClearColor, &State->GScene.PLights, State->GT);


#if 0
			// Do Shadowing
			for (auto& Caster : State->GScene.SpotLightCasters) {
				auto PVS = TempMemory->allocate_aligned<FlexKit::PVS>();
				GetGraphicScenePVS(&State->GScene, &Caster.C, &PVS, &Transparent);
				RenderShadowMap(RS, &PVS, &Caster, &State->ShadowMap, &State->ShadowMapPass, State->GT);
			}
#endif

			DrawGUI(RS, CL, &State->GUIRender, GetBackBufferTexture(State->ActiveWindow));
			CloseAndSubmit({ CL }, RS, State->ActiveWindow);
		}
	}


	GAMESTATEAPI void PostDraw(EngineMemory* Engine, iAllocator* TempMemory, double dt, GameState* State)
	{
		IncrementCurrent(State->DepthBuffer);

		PresentWindow(&Engine->Window, Engine->RenderSystem);
	}


	GAMESTATEAPI void Cleanup(EngineMemory* Engine, GameState* _ptr)
	{
		for (size_t I = 0; I < 3; ++I) {
			WaitforGPU(&Engine->RenderSystem);
			IncrementRSIndex(Engine->RenderSystem);
		}

		FreeTexture(&_ptr->TestScene.TestTexture);

		Engine->BlockAllocator.free(_ptr);
		
		FreeAllResourceFiles	(&Engine->Assets);
		FreeAllResources		(&Engine->Assets);

		ReleaseTextureSet(_ptr->Set1, Engine->BlockAllocator);

		//CleanUpShadowPass		(&_ptr->ShadowMapPass);
		CleanUpTextureVTable	(&_ptr->TextureState);
		CleanUpSimpleWindow		(&_ptr->TestScene.Window);
		CleanUpDrawGUI			(&_ptr->GUIRender);
		CleanUpLineSet			(&_ptr->Lines);
		CleanUpState			(_ptr, Engine);
		CleanUpCamera			(_ptr->Nodes, _ptr->ActiveCamera);
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