#include "stdafx.h"
///**********************************************************************
//
//Copyright (c) 2015 - 2016 Robert May
//
//Permission is hereby granted, free of charge, to any person obtaining a
//copy of this software and associated documentation files (the "Software"),
//to deal in the Software without restriction, including without limitation
//the rights to use, copy, modify, merge, publish, distribute, sublicense,
//and/or sell copies of the Software, and to permit persons to whom the
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//**********************************************************************/
//
//// Todos
//// TODO(RM) World utilities
//// TODO(RM) Material Sorting
//// TODO(RM) Different Rendering Modes
//
//#include "TestMode.h"
//#include "GameUtilities.h"
//#include "ResourceUtilities.h"
//
//#include "..\coreutilities\DungeonGen.h"
//#include "..\coreutilities\intersection.h"
//#include "..\coreutilities\GraphicScene.h"
//#include "..\graphicsutilities\AnimationUtilities.h"
//#include "..\graphicsutilities\TerrainRendering.h"
//
//#include <DirectXMath.h>
//
//// Usings
//using namespace FlexKit;
//using FlexKit::Quaternion;
//using FlexKit::TranslateLocal;
//using FlexKit::UpdateInput;
//using FlexKit::Yaw;
//
//// DirectX Usings
//using DirectX::XMMATRIX;
//
//#define SKINLOADING ON
//#define ANIMATIOLOADING ON
//#define SKELETONLOADING ON
//
///************************************************************************************************/
//
///*
//void TestMode_HandleEvents(const FlexKit::Event& in, void* _ptr)
//{
//	auto Engine = reinterpret_cast<EngineMemory*>(_ptr);
//	
//	switch (in.mType)
//	{
//	case FlexKit::Event::Input:
//		switch(in.InputSource)
//		{
//			case FlexKit::Event::Keyboard:
//			switch (in.Action)
//			{
//				case FlexKit::Event::Release:
//					switch (in.mData1.mKC[0])
//					{
//						case FlexKit::KC_SPACE:
//							Engine->KeyState.SpaceBar = false;
//							break;
//						case FlexKit::KC_Y:
//							Engine->ToggleMouse = !Engine->ToggleMouse;
//							if (Engine->ToggleMouse )
//							{
//								auto POS = Engine->Window.WindowCenterPosition;
//								POINT CENTER = { ( long )POS[ 0 ], ( long )POS[ 1 ] };
//								ClientToScreen(Engine->Window.hWindow, &CENTER );
//								SetCursorPos( CENTER.x, CENTER.y );
//								ShowCursor( FALSE );
//							} else
//								ShowCursor( TRUE );
//							break;
//						default:
//							break;
//					}
//					break;
//				case FlexKit::Event::Pressed:
//					switch (in.mData1.mKC[0])
//					{
//					case FlexKit::KC_SPACE:
//						Engine->KeyState.SpaceBar = true;
//						break;
//					case FlexKit::KC_T:
//						//std::cout << "W KEY PRESSED\n";
//						Engine->SplitCount = ++Engine->SplitCount % 10;
//						std::cout << "SplitCount: " << Engine->SplitCount << "\n";
//						break;
//					case FlexKit::KC_L:
//					{
//	#if USING( EDITSHADERCONTINUE )
//						// Reload Compute Shader
//						bool res = false;
//						{
//							FlexKit::ShaderDesc SDesc;
//							strcpy(SDesc.entry, "cmain");
//							strcpy(SDesc.ID, "DeferredShader");
//							strcpy(SDesc.shaderVersion, "cs_5_0");
//							Shader CShader;
//
//							{
//								printf("LoadingShader - Compute Shader Deferred Shader -\n");
//								res = FlexKit::LoadComputeShaderFromFile(Engine->RenderSystem, "assets\\cshader.hlsl", &SDesc, &CShader);
//								if (!res)
//								{
//									std::cout << "Failed to Load\n try again\n";
//								}
//								else
//								{
//									std::cout << "Swapping Shader\n";
//									FlexKit::Destroy(Engine->GB.Shade);
//									Engine->GB.Shade = CShader;
//								}
//							}
//						}
//					}
//	#endif
//					break;
//					case FlexKit::KC_U:
//					{
//					}
//					break;
//					case FlexKit::KC_P:
//					{
//	#if USING( EDITSHADERCONTINUE )
//						// Reload Compute Shader
//						bool res = false;
//						{
//							FlexKit::ShaderDesc SDesc;
//							strcpy(SDesc.entry,			"VMainVertexPallet");
//							strcpy(SDesc.shaderVersion, "vs_5_0");
//							FlexKit::Shader NewShader;
//
//							{
//								printf("LoadingShader - Vertex Shader Deferred Shader -\n");
//								res = FlexKit::LoadVertexShaderFromFile(Engine->RenderSystem, "assets\\vshader.hlsl", &SDesc, &NewShader);
//								if (!res)
//								{
//									std::cout << "Failed to Load\n try again\n";
//								}
//								else
//								{
//									std::cout << "Swapping Shader\n";
//									FlexKit::Destroy(Engine->Materials.GetShader(Engine->ShaderHandles.VertexPaletteSkinning));
//									Engine->Materials.SetShader(Engine->ShaderHandles.VertexPaletteSkinning, NewShader);
//									Engine->VertexPalletSkinning = Engine->Materials.GetShader(Engine->ShaderHandles.VertexPaletteSkinning);
//								}
//							}
//						}
//					}
//	#endif
//					break;
//					default:
//						break;
//					}
//				default:
//					break;
//			}
//			break;
//			case FlexKit::Event::Mouse:
//				//if (Engine->Level && Engine->MouseHandler)
//				//	Engine->MouseHandler(Engine, in);
//			break;
//		}
//	default:
//		break;
//	}
//}
//*/
//
///************************************************************************************************/
//
//
//struct FrameStats
//{
//	double T		= 0;
//	double FPSTimer	= 0;
//
//	size_t FrameCount	= 0;
//	size_t FPScounter	= 0;
//	size_t LastFPSCount	= 0;
//};
//
//
///************************************************************************************************/
//
//
//void UpdateFrameStats(FrameStats* fs, double dt)
//{
//	fs->T += dt;
//	fs->FPSTimer += dt;
//	fs->FrameCount++;
//	fs->FPScounter++;
//
//	if (fs->FPSTimer >= 1.0f)
//	{
//		fs->FPSTimer = 0;
//		fs->LastFPSCount = fs->FPScounter;
//		fs->FPScounter = 0;
//
//		//printf("FPS: %u\n", fs->LastFPSCount);
//	}
//}
//
//
///************************************************************************************************/
//
//
//struct DebugStructs
//{
//	double		ta = 0;
//	FrameStats	fs = {};
//
//	void Update(EngineMemory* Game, double dt){UpdateFrameStats(&fs, dt);}
//};
//
//
///************************************************************************************************/
//#include <..\physx\Include\characterkinematic\PxController.h>
//#include <..\physx\Include\characterkinematic\PxCapsuleController.h>
//
//class CCHitReport : public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
//{
//public:
//
//	virtual void onShapeHit(const physx::PxControllerShapeHit& hit)
//	{
//	}
//
//	virtual void onControllerHit(const physx::PxControllersHit& hit)
//	{
//	}
//
//	virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit)
//	{
//	}
//
//	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape&, const physx::PxActor&)
//	{
//		return physx::PxControllerBehaviorFlags(0);
//	}
//
//	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController&)
//	{
//		return physx::PxControllerBehaviorFlags(0);
//	}
//
//	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle&)
//	{
//		return physx::PxControllerBehaviorFlags(0);
//	}
//
//protected:
//};
//
//struct CapsuleCharacterController
//{
//	physx::PxControllerFilters  characterControllerFilters;
//	physx::PxFilterData			characterFilterData;
//	physx::PxController*		Controller;
//	CCHitReport					ReportCallback;
//};
//
//struct CapsuleCharacterController_DESC
//{
//	float r;
//	float h;
//	float3 FootPos;
//};
//
//
//using namespace physx;
//
//
///************************************************************************************************/
//
//
//void Initiate(CapsuleCharacterController* out, PScene* Scene, PhysicsSystem* PS, CapsuleCharacterController_DESC& Desc)
//{
//	PxCapsuleControllerDesc CCDesc;
//	CCDesc.material       = PS->DefaultMaterial;
//	CCDesc.radius	      = Desc.r;
//	CCDesc.height	      = Desc.h;
//	CCDesc.contactOffset  = 0.1f;
//	CCDesc.position       = { 0.0f, Desc.h / 2, 0.0f };
//	//CCDesc.reportCallback = &out->ReportCallback;
//	CCDesc.climbingMode			= PxCapsuleClimbingMode::eEASY;
//
//	auto NewCharacterController = Scene->ControllerManager->createController(CCDesc);
//	out->Controller				= NewCharacterController;
//	out->Controller->setFootPosition({Desc.FootPos.x, Desc.FootPos.y, Desc.FootPos.z});
//}
//
//struct TestPlayer
//{
//	NodeHandle			PitchNode;
//	NodeHandle			Node;
//	NodeHandle			YawNode;
//	NodeHandle			ModelNode;
//
//	float3				dV;
//	float				r;// Rotation Rate;
//	float				m;// Movement Rate;
//	float				HalfHeight;
//	bool				FloorContact;
//	bool				CeilingContact;
//	size_t				EntityHandle;
//	CapsuleCharacterController	BPC;
//};
//
//struct TestPlayer_DESC
//{
//	float  dr	= FlexKit::pi/2;  // Rotation Rate;
//	float  dm	= 10;  // Movement Rate;
//	float  r	= 3;   // Radius
//	float  h	= 10;   // Height
//	float3 POS	= {0, 0, 0}; // Foot Position
//};
//
//void InitialiseTestPlayer(TestPlayer* p, SceneNodes* Nodes, PScene* S, PhysicsSystem* PS, TestPlayer_DESC& Desc = TestPlayer_DESC())
//{
//	NodeHandle PitchNode	= GetZeroedNode(Nodes);
//	NodeHandle Node			= GetZeroedNode(Nodes);
//	NodeHandle YawNode		= GetZeroedNode(Nodes);
//	NodeHandle ModelNode	= GetZeroedNode(Nodes);
//
//	SetParentNode(Nodes, Node,    ModelNode);
//	SetParentNode(Nodes, YawNode, PitchNode);
//	SetParentNode(Nodes, Node,		YawNode);
//
//	p->dV			  = {0.0f, 0.0f, 0.0f};
//	p->PitchNode	  = PitchNode;
//	p->YawNode		  = YawNode;
//	p->Node			  = Node;
//	p->ModelNode	  = ModelNode;
//	p->r			  = FlexKit::pi/2;
//	p->m			  = 10;
//	p->FloorContact   = false;
//	p->CeilingContact = false;
//	p->HalfHeight	  = Desc.h / 2 + Desc.r;
//
//	Initiate(&p->BPC, S, PS, CapsuleCharacterController_DESC{Desc.r, Desc.h, Desc.POS});
//}
//
//
///************************************************************************************************/
//
//
//void TranslatePlayer(float3 pos, TestPlayer* P)
//{
//	auto flags = P->BPC.Controller->move({pos.x, pos.y, pos.z}, 0.1, 1.0f/60.0f, PxControllerFilters());
//	P->FloorContact   = flags & PxControllerCollisionFlag::eCOLLISION_DOWN;
//	P->CeilingContact = flags & PxControllerCollisionFlag::eCOLLISION_UP;
//}
//
//
///************************************************************************************************/
//
//
//void UpdateTestPlayerMovement(TestPlayer* p, sKeyState k, SceneNodes* Nodes, double dt, float c = 0.2f, float3 G ={ 0, -9.8f, 0})
//{
//	using FlexKit::Yaw;
//	using FlexKit::Pitch;
//	using FlexKit::Roll;
//
//	if (k.R) Pitch(Nodes, p->PitchNode, (dt * p->r));
//	if (k.F) Pitch(Nodes, p->PitchNode, (-dt * p->r));
//
//	if (k.Q) Yaw(Nodes, p->YawNode, (dt * p->r));
//	if (k.E) Yaw(Nodes, p->YawNode, (-dt * p->r));
//
//	Quaternion Q;
//	GetOrientation(Nodes, p->YawNode, &Q );
//	float3 dm = {0, 0, 0};
//
//	if (p->FloorContact)
//	{
//		p->dV.y = 0;
//
//		if (k.W)
//		{
//			float3 forward ={ 0, 0,  -1 };
//			float3 d = forward;
//			dm += d;
//		}
//
//		if (k.S)
//		{
//			float3 backward ={ 0, 0, 1 };
//			float3 d = backward;
//			dm += d;
//		}
//
//		if (k.A)
//		{
//			float3 Left ={ -1, 0, 0 };
//			float3 d = Left;
//			dm += d;
//		}
//
//		if (k.D)
//		{
//			float3 Right ={ 1, 0, 0 };
//			float3 d = Right;
//			dm += d;
//		}
//	}
//
//	if (p->FloorContact && k.SpaceBar) {
//		dm += {0, 5, 0};
//	}
//
//	if (p->CeilingContact)
//		p->dV.y = 0;
//
//	p->dV += p->FloorContact ? -1.0f * p->dV * c : G * dt;
//
//	if (dm.magnitudesquared() > .1)
//		p->dV += Q * dm;
//
//	if (p->dV.magnitude() > .1) 
//	{
//		PxFilterData characterFilterData;
//		characterFilterData.word0 = 0;
//
//		PxControllerFilters characterControllerFilters;
//		characterControllerFilters.mFilterData = &characterFilterData;
//
//		float3 Delta = p->dV * dt *  p->m;
//		TranslatePlayer(Delta, p);
//
//		auto NewPosition = p->BPC.Controller->getFootPosition();
//		SetPositionW(Nodes, p->Node, {NewPosition.x, NewPosition.y, NewPosition.z});
//	}
//
//	if (dm.magnitudesquared() > 0)
//	{
//		float3 Dir  = Quaternion(0, 90, 0) * (float3(1, 0, 1) * p->dV * float3(-1, 0, -1)).normalize();
//		float3 SV   = Dir.cross(float3{0, 1, 0});
//		float3 UpV	= SV.cross(Dir);
//
//		auto Q2 = Vector2Quaternion(Dir, UpV, SV);
//		Q2.x = 0;
//		Q2.z = 0;
//
//		SetOrientation(Nodes, p->ModelNode, Q2);
//		// Animation Code Here
//	}
//}
//
//
///************************************************************************************************/
//
//
//using TextUtilities::ClearText;
//using TextUtilities::CreateTextObject;
//using TextUtilities::FontAsset;
//using TextUtilities::PrintText;
//using TextUtilities::TextArea;
//
//void HandleMouseInput( EngineMemory* memory );
//
//struct HZB
//{
//	typedef Handle_t<16> VisibilityHandle;
//
//	void Initiate(RenderSystem* RS, size_t x, size_t y)
//	{
//	}
//
//	void AddOccluderGeometry(TriMesh* mesh)
//	{
//	}
//
//	VisibilityHandle AddOccluder(NodeHandle, size_t meshID)
//	{
//	}
//
//	void DownSample(RenderSystem* RS)
//	{
//	}
//
//	void DrawOccluder(RenderSystem* RS)
//	{
//	}
//
//	void SampleBuffer(RenderSystem* RS)
//	{
//	}
//
//	void Draw(RenderSystem* RS)
//	{
//		DrawOccluder(RS);
//		DownSample(RS);
//		SampleBuffer(RS);
//	}
//
//	void PostDraw(EngineMemory* Engine)
//	{
//
//	}
//
//
//	void PullResults()
//	{
//	}
//
//	bool isVisiable(VisibilityHandle hndl)
//	{
//		return true;
//	}
//		
//	ID3D11Buffer*				DepthBuffer;
//	ID3D11ShaderResourceView*	DepthRV;
//	ID3D11SamplerState*			DepthSampler;
//	ID3D11RenderTargetView*		DepthRTV;
//
//	struct GeometryTable
//	{
//		size_t			meshID;
//		ID3D11Buffer*	Transforms;
//		NodeHandle*		Handles;
//		TriMesh*		OccluderGeometry;
//	}Table;
//
//	ShaderSetHandle	FillDepthBufferShaderSet;
//	ShaderHandle	Sample;
//	//ShaderHandle	DownSample;
//};
//
//
//
//
//
///************************************************************************************************/
//
//
//float4x4 GetTransform(Camera* C)
//{
//	return float4x4::Identity();
//}
//
//
///************************************************************************************************/
//
//using FlexKit::GraphicScene;
//using FlexKit::Landscape;
//
//struct LevelLayout
//{
//	Resources					Assets;
//	size_t						PointLightCount;
//	size_t						SpotLightCount;
//	size_t						TerainSplitCount;
//
//	PScene			S;
//	GraphicScene	SM;
//	Landscape		Terrain;
//	FrameStats		T;
//
//	SceneNodes*		Nodes;
//	TestPlayer		Player1;
//	Camera*			PlayerCamera;
//	Camera			TestCamera;
//	size_t			lightEntity;
//	int2			MouseMove;
//
//	size_t TestMesh;
//	size_t Arrow;
//	size_t P1;
//
//	ShaderHandle	PShader;
//	ShaderTable*	Shaders;
//
//	FlexKit::EventNotifier<>::Subscriber sub;
//
//
//	/************************************************************************************************/
//
//
//	struct DebugDrawSkeleton
//	{
//		VertexBuffer			VB;
//		FlexKit::ConstantBuffer	CB;
//		ShaderSetHandle			SSHndl;
//
//		struct VBLayout
//		{
//
//		};
//
//		struct CBLayout
//		{
//
//		};
//	};
//
//
//	/************************************************************************************************/
//
//
//	DebugDrawSkeleton	DDSkel;
//	FontAsset			Font;
//	TextArea			TextBlock;
//	
//	
//	/************************************************************************************************/
//
//
//	void PushSkeleton( DebugDrawSkeleton* SK, FlexKit::Skeleton* S, FlexKit::NodeHandle hndl )
//	{
//	}
//
//
//	/************************************************************************************************/
//
//
//	void DrawSkeleton_DEBUG( DebugDrawSkeleton * SK, RenderSystem* RS)
//	{
//	}
//
//	
//	/************************************************************************************************/
//
//
//	inline Quaternion PointAt2(float3 A, float3 B, float3 Up = {0, 1, 0})
//	{
//		float3 V	= float3{ B - A }.normalize();
//		float3 Dir	= {V.z, V.y, -V.x};
//		float3 Left	= Dir.cross(Up);
//		float3 Up2	= Left.cross(Dir);
//
//		return Vector2Quaternion(Dir.normalize(), Up2.normalize(), Left.normalize());
//	}
//
//
//	/************************************************************************************************/
//
//
//	void Update(EngineMemory* Engine, double dt)
//	{
//		//Yaw(Nodes, SM.GetDrawable(TestMesh).Node, pi/4 * dt);
//		FlexKit::WT_Entry WT;
//		FlexKit::GetWT(Engine->Nodes, Engine->PlayerCam.Node, &WT);
//
//		UpdateFrameStats(&T, dt);
//
//		int2 MouseMove = Engine->dMousePOS;
//		Yaw(Engine->Nodes, Player1.YawNode, Engine->MouseMovementFactor * dt * MouseMove[0]);
//		Pitch(Engine->Nodes, Player1.PitchNode, Engine->MouseMovementFactor * dt * MouseMove[1]);
//		Engine->dMousePOS ={ 0, 0 };
//
//		UpdateTestPlayerMovement(&Player1, Engine->KeyState, Engine->Nodes, dt);
//		/*
//
//		char* STR = (char*)Engine->TempAllocator.malloc(1025);
//		memset(STR, ' ', 1024);
//		STR[1024] = '\0';
//
//		TerainSplitCount = 8;
//		Yaw(Nodes, TestCamera.Node, pi/4 * dt);
//
//		float3 P1POS	= FlexKit::GetPositionW(Nodes, Player1.Node);
//		{
//			float3 ArrowPOS = SM.GetEntityPosition(Arrow);
//			float3 Dir = ArrowPOS - P1POS;
//
//
//			if (Dir.magnitudesquared() > 1)
//			{
//				auto Q = PointAt2(float3(1, 0, 1) * ArrowPOS, float3(1, 0, 1) * P1POS);
//				SetOrientation(Nodes, SM.GetDrawable(Arrow).Node, Q);
//			}
//
//		}
//
//		sprintf_s(STR, 1024,
//				  "Draw Call Last Second: %i\nAve Draw Calls Per Second: %i\nAve FrameTime: %fms\nAve Sorting Time: %fms\nAve Drawable Update Time: %fms\nAve Present Time: %fms\nAve Animation UpdateTime: %fms\nUpdateTime: %fms\n"
//				  "PlayerPOS{%f, %f, %f}\n",
//				    (int)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_INDEXEDDRAWCALL_LASTSECOND),
//				    (int)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_INDEXEDDRAWCALL_LASTSECOND) / 60,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEFRAMETIME) / 1000.0f,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGESORTTIME) / 1000.0f,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEENTITYUPDATETIME) / 1000.0f,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEPRESENT) / 1000.0f,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEANIMATIONTIME) / 1000.0f,
//				  (float)ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEFRAMETIME) / 1000.0 - float(ProfilingUtilities::GetCounter(ProfilingUtilities::COUNTER_AVERAGEPRESENT) / 1000.0),
//				  P1POS.x, P1POS.y, P1POS.z );
//
//		ClearText(&TextBlock);
//		PrintText(&TextBlock, STR);
//		ProfilingUtilities::IncCounter(ProfilingUtilities::COUNTER_INDEXEDDRAWCALL);
//		*/
//	}
//
//
//	PScene* GetPhysicsScene() { return &S; }
//
//
//	/************************************************************************************************/
//
//
//	static void PrePhysicsUpdate(void* p)
//	{
//
//	}
//
//
//	/************************************************************************************************/
//
//
//	static void PostPhysicsUpdate(void* p)
//	{
//
//	}
//
//
//	/************************************************************************************************/
//
//	void CreateScene(EngineMemory* Engine)
//	{	
//		auto defaultMat  = Engine->BuiltInMaterials.DefaultMaterial;
//
//		// Setup Player
//		InitialiseTestPlayer(&Player1, Engine->Nodes, &S, &Engine->Physics);
//		SetParentNode(Nodes, Player1.PitchNode, Engine->PlayerCam.Node);
//	
//		TranslateLocal(Engine->Nodes, Player1.PitchNode,		{00.0f, 20.0f, 00.0f});
//		TranslateLocal(Engine->Nodes, Engine->PlayerCam.Node,	{00.0f, 00.0f, 40.0f});
//		TranslatePlayer({0.0f, 10.0f, 0.0f}, &Player1);
//	
//		FlexKit::InitiateCamera(Engine->RenderSystem, Nodes, &TestCamera, 1);
//		TestCamera.Node = GetZeroedNode(Nodes);
//		Yaw(Nodes, TestCamera.Node, pi/4);
//
//		CreatePlaneCollider(Engine->Physics.DefaultMaterial, &S);
//		P1 = SM.CreateDrawableAndSetMesh(defaultMat, "PlayerMesh");
//		auto Floor = SM.CreateDrawableAndSetMesh(defaultMat, "Floor");
//		//SM.EntityEnablePosing(P1);
//		//SM.EntityPlayAnimation(P1, "ANIMATION");
//		SM.SetNode(P1, Player1.ModelNode).SetAlbedo({ 0.5f, 0.75f, 10.0f, 0.05f }).SetSpecular({1, 1, .5, 0});
//
//		SM.GetDrawable(Floor).SetAlbedo({ 1.0f, 0.25f, 0.25f, 0.2f }).SetSpecular({1.0f, 1.0f, 1.0f, 0});
//		
//		Arrow = SM.CreateDrawableAndSetMesh(defaultMat, "Arrow");
//		
//		auto Light1	= SM.CreateDrawableAndSetMesh(defaultMat, "Light");
//		auto Light2	= SM.CreateDrawableAndSetMesh(defaultMat, "Light");
//		auto Light3	= SM.CreateDrawableAndSetMesh(defaultMat, "Light");
//
//		SM.TranslateEntity_WT(Arrow,  {   0, 20, 0});
//		SM.TranslateEntity_WT(Light1, {   0, 50, 0});
//		SM.TranslateEntity_WT(Light2, { 100, 50, 0});
//		SM.TranslateEntity_WT(Light3, {-100, 50, 0});
//
//		float I = 10000;
//		//auto PL1 = SM.AddPointLight({   0, 50, 0 },	{ 1, 1, 1 }, I, I);
//		//auto PL2 = SM.AddPointLight({ 100, 50, 0 }, { 1, 1, 1 }, I, I);
//		//auto PL3 = SM.AddPointLight({-100, 50, 0 }, { 1, 1, 1 }, I, I);
//		//SM.AddPointLight({0, 500, 0 }, { 1, 1, 1 }, 1000, 1000);
//
//		//SM.SetLightNodeHandle(PL1, SM.GetDrawable(Light1).Node);
//		//SM.SetLightNodeHandle(PL2, SM.GetDrawable(Light2).Node);
//		//SM.SetLightNodeHandle(PL3, SM.GetDrawable(Light3).Node);
//
//		lightEntity = Light1;
//
//		//CreateTerrain(Engine->RenderSystem, &Terrain, GetZeroedNode(Nodes));
//	}
//	
//
//	/************************************************************************************************/
//
//
//	void UpdateAnimations(RenderSystem* RS, StackAllocator* TempMemory, double dt)
//	{
//		//UpdateAnimationsGraphicScene(&SM, nullptr, dt);
//	}
//
//
//	/************************************************************************************************/
//
//
//	void PreDraw(EngineMemory* Engine, double dt)
//	{
//		float3 POS = FlexKit::GetPositionW(Nodes, Player1.Node);
//		SM.SetPositionEntity_WT(lightEntity, {POS.x, 30, POS.z});
//
//		//UpdateGraphicScene_PreDraw(&SM);
//		//PointLightCount	= SM.PLights.size();
//		//SpotLightCount	= SM.SPLights.size();
//
//		//UpdateCamera(Engine->RenderSystem, Nodes, &TestCamera, 0, 0, 0);
//		UpdateGraphicScene(&SM, PlayerCamera, Engine->PVS_);
//		//UpdateLandscape(Engine->RenderSystem, &Terrain, &Engine->Nodes, PlayerCamera);
//	}
//
//
//	/************************************************************************************************/
//
//	struct TerrainDrawParams
//	{
//		Shader PixelShader;
//	};
//
//	static void FinalDraw(Context* RS, void* _ptr)
//	{
//		TerrainDrawParams* Params = (TerrainDrawParams*)_ptr;
//		SetShader(*RS, Params->PixelShader);
//		DrawAuto(*RS);
//	}
//
//	void Draw(RenderSystem* RS, Camera* C, FlexKit::ShaderTable* M)
//	{
//		TerrainDrawParams Params;
//		Params.PixelShader = Shaders->GetPixelShader(PShader);
//		//DrawLandscape(RS->ContextState, &Terrain, C, TerainSplitCount, &FinalDraw, &Params);
//		//DrawSkeleton_DEBUG(&DDSkel, RS);
//	}
//
//
//	/************************************************************************************************/
//	
//
//	void PostDraw(EngineMemory* Engine)
//	{
//		//DrawTextArea(&Font, &TextBlock, &Engine->TempAllocator, Engine->RenderSystem, Engine->RenderSystem, &Engine->Materials, &Engine->Window);
//	}
//
//
//	/************************************************************************************************/
//
//	void CleanUp(EngineMemory* Engine)
//	{
//		FreeFontAsset(&Font);
//		//CleanUpTerrain(Nodes, &Terrain);
//		//CleanUpTextArea(&TextBlock, &Engine->BlockAllocator);
//		CleanUpScene(&S);
//		CleanUpGraphicScene(&SM);
//
//		FreeAllResources(&Assets);
//		FreeAllResourceFiles(&Assets);
//	}
//
//};
//
//#pragma comment(lib, "User32.lib")
//
//
///************************************************************************************************/
//
//
//void ProcessDungeonBlock(byte* _ptr, DungeonGenerator::Tile in[3][3], uint2 POS, uint2 BPOS, DungeonGenerator* D)
//{
//	LevelLayout* level = (LevelLayout*)_ptr;
//	if (in[1][1] == DungeonGenerator::FLOOR_Dungeon || in[1][1] == DungeonGenerator::Corridor)
//	{
//		//auto node = FlexKit::GetZeroedNode(level->Nodes);
//		//SetPositionW(level->Nodes, node, DungeonCordToTranslatePosition(D, POS, 2));
//		//level->Scene.CreateDrawable(node, 0);
//		//printf("Floor Found: Position %u, %u\n", POS[0], POS[1]);
//	}
//}
//
//
///************************************************************************************************/
//
//
//struct FontRender
//{
//	FontAsset*	Font;
//};
//
//
///************************************************************************************************/
//
//
//void MoveFontAsset(FontAsset* S, FontAsset* D, BlockAllocator* Mem)
//{
//	*D = *S;
//	if (S->KerningTableSize && Mem)
//	{
//		// TODO(RM): HANDLE THIS CASE
//	} else {
//		D->KerningTable		= nullptr;
//		D->KerningTableSize = 0;
//	}
//}
//
//
///************************************************************************************************/
//
//
//struct SubDivTriMesh_DESC
//{
//	TriMesh*	SourceMesh;
//};
//
///*
//bool CreateSubDivTriMesh(RenderSystem* RS , SubDivTriMesh_DESC* Desc, FlexKit::TriMesh* Out)
//{
//	OpenSubdiv::Far::TopologyDescriptor	Descriptor;
//	Descriptor.vertIndicesPerFace;
//
//	return false;
//}
//
//*/
//LevelLayout* CreateTestLevel(EngineMemory* Engine)
//{
//	// level Initiation
//	LevelLayout* level	 = (LevelLayout*)Engine->LevelAllocator._aligned_malloc(sizeof(LevelLayout), 64);
//	level->Nodes		 =  Engine->Nodes;
//	level->Shaders		 = &Engine->Materials;
//	level->PShader		 =  Engine->ShaderHandles.PixelShaderDebug;
//	level->PlayerCamera  = &Engine->PlayerCam;
//	InitiateScene(&Engine->Physics, &level->S);
//
//	/*
//	auto Font = TextUtilities::LoadFontAsset( "assets\\textures\\", "fontTest.fnt", Engine->RenderSystem, &Engine->TempAllocator, &Engine->LevelAllocator );
//	if ((size_t)Font > 0) {
//		MoveFontAsset(GetByType<FontAsset*>(Font), &level->Font, &Engine->BlockAllocator);
//		Engine->TempAllocator.clear();
//	}
//	*/
//
//	level->Assets.ResourceMemory = &Engine->BlockAllocator;
//	AddResourceFile("assets\\ResourceFile.gameres", &level->Assets);
//	InitiateGraphicScene(&level->SM, Engine->RenderSystem, &Engine->Materials, &level->Assets, level->Nodes, &Engine->BlockAllocator, &Engine->TempAllocator);
//	level->PointLightCount	= 0;
//
//	// Text Render Initiation
//	if(0)
//	{
//		float2 ScreenPOS;// 0 - 1 Space
//		float2 WH; // Text Width Height
//		TextUtilities::TextArea_Desc Desc;
//		Desc.GShader	= Engine->ShaderHandles.GTextRendering;
//		Desc.PShader	= Engine->ShaderHandles.PTextRendering;
//		Desc.VShader	= Engine->ShaderHandles.VTextRendering;
//		Desc.POS		= {   0,   0 };		
//		Desc.TextWH		= {  10,  20 };
//		Desc.WH			= { 800, 600 };
//
//		level->TextBlock = CreateTextObject(Engine->RenderSystem, &Engine->BlockAllocator, &Desc, &Engine->Materials);// Setups a 2D Surface for Drawing Text into
//		PrintText(&level->TextBlock, "THIS IS A TEST\nNEWLINE!\nA LINE THAT IS TOO BIG TO FIT ON A SINGLE LINE THAT SHOULD OVERFLOW THE EDGE ONTO THE NEXT LINES UNDER!!!!!!!! qwertyuiopasdfghjklzxcvbnm~!@#$%^&*()");
//	}
//
//	// Hook Input
//	level->sub.Notify	= &TestMode_HandleEvents;
//	level->sub._ptr		= Engine;
//
//	Engine->Window.Handler.Subscribe(level->sub);
//	Engine->Level = level;
//
//	level->CreateScene(Engine);
//
//	return level;
//}
//
//
///************************************************************************************************/
//
//#include "..\coreutilities\Signal.h"
//
//void TestMode( EngineMemory* Engine )
//{
//	InitEngine(Engine);
//	//GameLoop(Engine, CreateTestLevel( Engine ));
//}
//
//
///************************************************************************************************/