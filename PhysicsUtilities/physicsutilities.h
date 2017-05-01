/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

#ifndef PHYSICSUTILITIES_H
#define PHYSICSUTILITIES_H

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"

#include "PxPhysicsAPI.h"

#include <typeinfo>

#ifdef _DEBUG
#pragma comment(lib,	"LowLevelDebug.lib"						)
#pragma comment(lib,	"LowLevelClothDEBUG.lib"				)
#pragma comment(lib,	"PhysX3DEBUG_x64.lib"					)
#pragma comment(lib,	"PhysX3CharacterKinematicDebug_x64.lib"	)
#pragma comment(lib,	"PhysX3CommonDEBUG_x64.lib"				)
#pragma comment(lib,	"PhysX3ExtensionsDEBUG.lib"				)
#pragma comment(lib,	"PhysXProfileSDKDEBUG.lib"				)
#pragma comment(lib,	"PhysXVisualDebuggerSDKDEBUG.lib"		)
#pragma comment(lib,	"legacy_stdio_definitions.lib"			)
#pragma comment(lib,	"SceneQueryDEBUG.lib"					)
#pragma comment(lib,	"SimulationControllerDEBUG.lib"			)

#else
#pragma comment(lib,	"LowLevel.lib"						)
#pragma comment(lib,	"LowLevelCloth.lib"					)
#pragma comment(lib,	"PhysX3_x64.lib"					)
#pragma comment(lib,	"PhysX3CharacterKinematic_x64.lib"	)
#pragma comment(lib,	"PhysX3Common_x64.lib"				)
#pragma comment(lib,	"PhysX3Cooking_x64.lib"				)
#pragma comment(lib,	"PhysX3Extensions.lib"				)
#pragma comment(lib,	"PhysXProfileSDK.lib"				)
#pragma comment(lib,	"PhysX3Vehicle.lib"					)
#pragma comment(lib,	"PhysXVisualDebuggerSDK.lib"		)
#pragma comment(lib,	"legacy_stdio_definitions.lib"		)
#endif

namespace FlexKit
{
	static physx::PxDefaultErrorCallback	gDefaultErrorCallback;
	static physx::PxDefaultAllocator		gDefaultAllocatorCallback;

	typedef void(*FNPSCENECALLBACK_POSTUPDATE)(void*);
	typedef void(*FNPSCENECALLBACK_PREUPDATE) (void*);


	/************************************************************************************************/


	struct TriangleCollider
	{
		physx::PxTriangleMesh*	Mesh;
		size_t					RefCount;
	};


	/************************************************************************************************/


	typedef Handle_t<16>	 ColliderHandle;

	struct TriMeshColliderList
	{
		DynArray<TriangleCollider>						TriMeshColliders;
		DynArray<size_t>								FreeSlots;
		HandleUtilities::HandleTable<ColliderHandle>	TriMeshColliderTable;
	};

	struct PhysicsSystem
	{
		physx::PxFoundation*			Foundation;
		physx::PxProfileZoneManager*	ProfileZoneManager;
		physx::PxPhysics*				Physx;
		physx::PxCooking*				Oven;

		physx::PxDefaultCpuDispatcher*	CPUDispatcher;

		bool								RemoteDebuggerEnabled;
		physx::PxVisualDebugger*			VisualDebugger;
		physx::PxVisualDebuggerConnection*	VisualDebuggerConnection;
		physx::PxGpuDispatcher*				GPUDispatcher;
		physx::PxMaterial*					DefaultMaterial;

		TriMeshColliderList Colliders;
	};



	/************************************************************************************************/


	struct PActor
	{
		enum EACTORTYPE
		{
			EA_TRIMESH,
			EA_PRIMITIVE
		}type;

		void*			_ptr;
		ColliderHandle	CHandle;
	};


	struct Collider
	{
		physx::PxActor* Actor;
		NodeHandle		Node;

		PActor	ExtraData;
	};


	struct PRagdoll
	{

	};


	struct SceneDesc
	{
		physx::PxTolerancesScale		tolerances;
		physx::PxDefaultCpuDispatcher*	CPUDispatcher;
	};


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


	/************************************************************************************************/


	FLEXKITAPI void AddRef(PhysicsSystem* PS, ColliderHandle);
	FLEXKITAPI void Release(PhysicsSystem* PS, ColliderHandle);

	/*
	FLEXKITAPI size_t			CreateCubeActor			(physx::PxMaterial* material, PScene* scene, float l, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV ={ 0, 0, 0 });
	FLEXKITAPI size_t			CreatePlaneCollider		(physx::PxMaterial* material, PScene* scene);
	FLEXKITAPI size_t			CreateSphereActor		(physx::PxMaterial* material, PScene* scene, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV ={ 0, 0, 0 });
	FLEXKITAPI ColliderHandle	LoadTriMeshCollider		(PhysicsSystem* PS, Resources* RM, GUID_t Guid);
	FLEXKITAPI size_t			CreateStaticActor		(PhysicsSystem* PS, PScene* Scene, NodeHandle Node, float3 POS = { 0, 0, 0 }, Quaternion Q = Quaternion::Identity());


	FLEXKITAPI void Initiate		(CapsuleCharacterController* out, PScene* Scene, PhysicsSystem* PS, CapsuleCharacterController_DESC& Desc);
	FLEXKITAPI void ReleaseCapsule	(CapsuleCharacterController* Capsule);
	FLEXKITAPI void	ReleaseScene	(PScene* mat);
	FLEXKITAPI void	InitiateScene	(PhysicsSystem* System, PScene* scn, iAllocator* allocator);



	FLEXKITAPI void	MakeCube		(CubeDesc& cdesc, SceneNodes* Nodes, PScene* scene, physx::PxMaterial* Material, Drawable* E, NodeHandle node, float3 initialP ={ 0, 0, 0 }, FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity());


	FLEXKITAPI void	UpdateScene		(PScene* scn, double dt, FNPSCENECALLBACK_POSTUPDATE, FNPSCENECALLBACK_PREUPDATE, void* P);
	FLEXKITAPI void	UpdateColliders	(PScene* scn, FlexKit::SceneNodes* nodes);

	*/

	FLEXKITAPI void	InitiatePhysics(PhysicsSystem* Physics, uint32_t CoreCount, iAllocator* allocator);
	FLEXKITAPI void	ReleasePhysics	(PhysicsSystem* Physics);


	/************************************************************************************************/


	struct ColliderBaseSystem : FlexKit::ComponentSystemInterface
	{
		void Initiate(
			SceneNodeComponentSystem* nodes,
			iAllocator* Memory) 
		{
			Nodes				= nodes;
			Colliders.Allocator = Memory;
		}

		void Release()
		{
			for (auto C : Colliders) 
			{
				if (C.Actor) 
					C.Actor->release();
			}
		}


		void ReleaseHandle(ComponentHandle Handle)
		{
		}


		struct Collider
		{
			Collider() :
				Actor		(nullptr),
				Node		(NodeHandle(-1)),
				Material	(nullptr) {}

			Collider(NodeHandle node, physx::PxMaterial* Mat = nullptr, physx::PxActor* actor = nullptr) :
				Actor(actor),
				Material(Mat),
				Node(node) {}

			physx::PxActor*		Actor;
			physx::PxMaterial*	Material;
			NodeHandle			Node;
		};

		ComponentHandle CreateCollider(
			physx::PxActor*		actor,
			physx::PxMaterial*	mat
		)
		{
			ComponentHandle Handle(Colliders.size());
			
			Colliders.push_back(Collider(Nodes->GetNewNode(), mat, actor));

			return Handle;
		}


		void UpdateSystem()
		{
			for (auto c : Colliders)
			{
				if (c.Actor->isRigidBody())
				{
					physx::PxRigidDynamic* o = static_cast<physx::PxRigidDynamic*>(c.Actor);
					auto pose = o->getGlobalPose();

					FlexKit::LT_Entry LT(FlexKit::GetLocal(*Nodes, c.Node));
					LT.T.m128_f32[0] = pose.p.x;
					LT.T.m128_f32[1] = pose.p.y;
					LT.T.m128_f32[2] = pose.p.z;

					LT.R.m128_f32[0] = pose.q.x;
					LT.R.m128_f32[1] = pose.q.y;
					LT.R.m128_f32[2] = pose.q.z;
					LT.R.m128_f32[3] = pose.q.w;

					FlexKit::SetLocal(*Nodes, c.Node, &LT);
				}
			}
		}


		NodeHandle GetNode(ComponentHandle C)
		{
			return Colliders[C].Node;
		}

		SceneNodeComponentSystem*	Nodes;
		DynArray<Collider>			Colliders;
		DynArray<Collider>			FreeColliders;
	};

	const uint32_t ColliderComponentID = GetTypeGUID(ColliderSystem);

	/************************************************************************************************/


	struct CharacterControllerSystem : public FlexKit::ComponentSystemInterface
	{
		struct CharacterController
		{
			float3					CurrentPosition = 0;
			float3					Delta			= 0;
			NodeHandle				Node			= NodeHandle(-1);
			physx::PxController*	Controller		= nullptr;
			bool					FloorContact	= false;
			bool					CeilingContact	= false;
			bool					SideContact		= false;
		};


		void Initiate(SceneNodeComponentSystem* nodes, iAllocator* Memory, physx::PxScene* Scene)
		{
			Nodes					= nodes;
			Controllers.Allocator	= Memory;
			ControllerManager		= PxCreateControllerManager(*Scene);
		}


		void UpdateSystem(double dT)
		{
			for (auto& C : Controllers)
			{
				physx::PxControllerFilters Filter;
				if (C.Delta.magnitudesquared() > 0.001f)
				{
					C.Controller->move(
					{ C.Delta.x, C.Delta.y, C.Delta.z },
						0.01f,
						dT,
						Filter);

					C.Delta = 0;

					auto NewPosition = C.Controller->getFootPosition();
					auto NewPositionFK = { NewPosition.x, NewPosition.y, NewPosition.z };

					C.CurrentPosition = NewPositionFK;

					printfloat3(C.CurrentPosition);
					printf("\n");

					Nodes->SetPositionL(C.Node, NewPositionFK);
				}
			}
		}


		void Release()
		{
		}


		void ReleaseHandle(ComponentHandle Handle)
		{
		}


		void HandleEvent(ComponentHandle Handle, ComponentType EventSource, ComponentSystemInterface* System, EventTypeID ID, GameObjectInterface* GO) final
		{
			if (EventSource == TransformComponentID && ID == GetCRCGUID(POSITION))
			{
				auto CurrentPosition	= Controllers[Handle].CurrentPosition;
				auto Position			= Nodes->GetPositionL(Controllers[Handle].Node);
				auto DeltaPosition		= Position - CurrentPosition;

				Controllers[Handle].Delta += DeltaPosition;
			}
		}

		void		SetNodeHandle(ComponentHandle Handle, NodeHandle Node)
		{
			Nodes->ReleaseHandle(Controllers[Handle].Node);
			Controllers[Handle].Node = Node;
		}
		NodeHandle	GetNodeHandle(ComponentHandle Handle)
		{
			return Controllers[Handle].Node;
		}


		ComponentHandle CreateCapsuleController(PhysicsSystem* PS, physx::PxScene* Scene, physx::PxMaterial* Mat,float R, float H, float3 InitialPosition = 0, Quaternion Q = Quaternion())
		{
			physx::PxCapsuleControllerDesc CCDesc;
			CCDesc.material             = Mat;
			CCDesc.radius               = R;
			CCDesc.height               = H;
			CCDesc.contactOffset        = 0.01f;
			CCDesc.position             = { 0.0f, H / 2, 0.0f };
			CCDesc.climbingMode         = physx::PxCapsuleClimbingMode::eEASY;
			//CCDesc.upDirection = PxVec3(0, 0, 1);

			CharacterController NewController;

			auto NewCharacterController = ControllerManager->createController(CCDesc);
			NewController.Controller	= NewCharacterController;
			NewController.Node			= Nodes->GetNewNode();
			NewCharacterController->setFootPosition({ InitialPosition.x, InitialPosition.y, InitialPosition.z });

			Controllers.push_back(NewController);

			return ComponentHandle(Controllers.size() - 1);
		}

		DynArray<CharacterController>	Controllers;
		physx::PxControllerManager*		ControllerManager;
		SceneNodeComponentSystem*		Nodes;
	};

	const uint32_t CharacterControllerSystemID = GetTypeGUID(CharacterControllerSystem);


	/************************************************************************************************/


	struct CapsuleCharacterArgs
	{
		ComponentHandle				CapsuleController;
		CharacterControllerSystem*	System;
	};


	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, CapsuleCharacterArgs& Args)
	{
		auto C = FindComponent(GO, TransformComponentID);
		if (C)
		{
			auto Transform = (TansformComponent*)C;
			Args.System->SetNodeHandle(Args.CapsuleController, Transform->ComponentHandle);
		}
		else
		{
			auto Handle = Args.System->GetNodeHandle(Args.CapsuleController);
			GO.AddComponent(Component(Args.System->Nodes, Handle, TransformComponentID));
		}

		GO.AddComponent(Component(Args.System, Args.CapsuleController, CharacterControllerSystemID));
	}


	/************************************************************************************************/


	struct StaticCubeColliderSystem : FlexKit::ComponentSystemInterface
	{
		struct CubeCollider
		{
			ComponentHandle	BaseHandle;
			physx::PxRigidStatic* Box;
		};

		void Initate(iAllocator* Memory)
		{
			Colliders.Allocator = Memory;
		}

		void Release()
		{
		}

		void ReleaseHandle(ComponentHandle Handle)
		{

		}

		
		ComponentHandle CreateStaticBoxCollider(ComponentHandle BaseHandle, physx::PxRigidStatic* Box)
		{
			Colliders.push_back({BaseHandle, Box});

			return ComponentHandle(Colliders.size() - 1);
		}

		SceneNodes*				Nodes;
		DynArray<CubeCollider>	Colliders;
	};


	const uint32_t StaticCubeColliderComponentID = GetTypeGUID(StaticCubeColliderComponentID);


	struct StaticBoxColliderArgs
	{
		StaticCubeColliderSystem*	System;
		ColliderBaseSystem*			Base;
		ComponentHandle				ColliderHandle;
		ComponentHandle				BoxColliderHandle;
	};


	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, StaticBoxColliderArgs& Args)
	{
		GO.AddComponent(Component(Args.Base,	Args.ColliderHandle,	ColliderComponentID));
		GO.AddComponent(Component(Args.System,	Args.BoxColliderHandle, StaticCubeColliderComponentID));

		auto TransformComponent = (FlexKit::TansformComponent*)FindComponent(GO, FlexKit::TransformComponentID);

		if (TransformComponent) {
			auto ColliderNode	= Args.Base->GetNode(Args.ColliderHandle);
			SetParent(GO, ColliderNode);
		}
	}

	struct CubeColliderSystem : FlexKit::ComponentSystemInterface
	{
		struct CubeCollider
		{
			ComponentHandle	BaseHandle;
			physx::PxRigidDynamic* Cube;
		};

		void Initiate(iAllocator* Memory)
		{
			Colliders.Allocator = Memory;
		}

		void Release()
		{

		}

		void ReleaseHandle(ComponentHandle Handle)
		{

		}

		ComponentHandle CreateCubeCollider(ComponentHandle BaseHandle, physx::PxRigidDynamic* Box, physx::PxShape* Shape)
		{
			Colliders.push_back({ BaseHandle, Box });

			return ComponentHandle(Colliders.size() - 1);
		}

		SceneNodes*				Nodes;
		DynArray<CubeCollider>	Colliders;
	};

	const uint32_t CubeColliderComponentID = GetTypeGUID(CubeColliderSystem);

	struct CubeColliderArgs
	{
		CubeColliderSystem*			System;
		ColliderBaseSystem*			Base;
		ComponentHandle				ColliderHandle;
		ComponentHandle				CubeHandle;
	};

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, CubeColliderArgs& Args)
	{
		GO.AddComponent(Component(Args.Base,	Args.ColliderHandle,	ColliderComponentID));
		GO.AddComponent(Component(Args.System,	Args.CubeHandle,		CubeColliderComponentID));

		auto TransformComponent = (FlexKit::TansformComponent*)FindComponent(GO, FlexKit::TransformComponentID);

		if (TransformComponent) {
			auto ColliderNode	= Args.Base->GetNode(Args.ColliderHandle);
			SetParent(GO, ColliderNode);
		}
	}

	struct PhysicsComponentSystem
	{
		void InitiateSystem	(PhysicsSystem* System, SceneNodeComponentSystem* Nodes, iAllocator* Memory);
		void UpdateSystem	(double dT);
		void Release		();

		StaticBoxColliderArgs	CreateStaticBoxCollider	(float3 XYZ = 1, float3 Pos = 0) {
			auto Static = physx::PxCreateStatic(
							*System->Physx, 
							{ Pos.x, Pos.y, Pos.z },
							physx::PxBoxGeometry(XYZ.x, XYZ.y, XYZ.z), 
							*System->DefaultMaterial);

			Scene->addActor(*Static);

			auto BaseCollider	= Base.CreateCollider(Static, System->DefaultMaterial);
			auto BoxCollider	= StaticBoxColliders.CreateStaticBoxCollider(BaseCollider, Static);

			SetPositionW(*Nodes, Base.GetNode(BaseCollider), Pos);

			StaticBoxColliderArgs Out;
			Out.Base				= &Base;
			Out.System				= &StaticBoxColliders;
			Out.ColliderHandle		= BaseCollider;
			Out.BoxColliderHandle	= BoxCollider;

			return Out;
		}
		CubeColliderArgs		CreateCubeComponent		(float3 InitialP = 0, float3 InitialV = 0, float l = 10, Quaternion Q = Quaternion(0, 0, 0, 1))
		{
			physx::PxVec3 pV;
			pV.x = InitialP.x;
			pV.y = InitialP.y;
			pV.z = InitialP.z;

			physx::PxQuat pQ;
			pQ.x = Q.x;
			pQ.y = Q.y;
			pQ.z = Q.z;
			pQ.w = Q.w;

			physx::PxRigidDynamic*	Cube	= Scene->getPhysics().createRigidDynamic(physx::PxTransform(pV, pQ));
			physx::PxShape*			Shape	= Cube->createShape(physx::PxBoxGeometry(l, l, l), *System->DefaultMaterial);
			Cube->setLinearVelocity({ InitialV.x, InitialV.y, InitialV.z }, true);
			Cube->setOwnerClient(CID);

			Scene->addActor(*Cube);

			auto BaseCollider	= Base.CreateCollider(Cube, System->DefaultMaterial);
			auto CubeCollider	= CubeColliders.CreateCubeCollider(BaseCollider, Cube, Shape);

			auto Node = Base.GetNode(BaseCollider);
			SetPositionW	(*Nodes, Node, InitialP);
			SetOrientation	(*Nodes, Node, Q);

			CubeColliderArgs Out;
			Out.Base              = &Base;
			Out.System            = &CubeColliders;
			Out.ColliderHandle    = BaseCollider;
			Out.CubeHandle		  = CubeCollider;

			return Out;
		}

		CapsuleCharacterArgs	CreateCharacterController(float3 InitialP = 0, float R = 5, float H = 5, Quaternion Q = Quaternion())
		{
			auto CapsuleCharacter = CharacterControllers.CreateCapsuleController(System, Scene, System->DefaultMaterial, R, H, InitialP, Q);

			CapsuleCharacterArgs Args;
			Args.CapsuleController = CapsuleCharacter;
			Args.System = &CharacterControllers;

			return Args;
		}

		ColliderBaseSystem			Base;
		CubeColliderSystem			CubeColliders;
		StaticCubeColliderSystem	StaticBoxColliders;
		CharacterControllerSystem	CharacterControllers;

		physx::PxScene*				Scene;
		physx::PxControllerManager*	ControllerManager;
		physx::PxClientID			CID;

		double	T;
		bool	UpdateColliders;

		PhysicsSystem*	System;

		SceneNodeComponentSystem*	Nodes;
		iAllocator*					Memory;

		FNPSCENECALLBACK_POSTUPDATE PreUpdate;
		FNPSCENECALLBACK_PREUPDATE	PostUpdate;
		void*						User;
	};

}
#endif//PHYSICSUTILITIES_H