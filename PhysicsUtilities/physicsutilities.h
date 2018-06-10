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
#include <physx\Include\characterkinematic\PxController.h>
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
		Vector<TriangleCollider>						TriMeshColliders;
		Vector<size_t>									FreeSlots;
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

		operator PhysicsSystem* () { return this; }
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

	FLEXKITAPI ColliderHandle			LoadTriMeshCollider			(PhysicsSystem* PS, Resources* RM, GUID_t Guid);
	FLEXKITAPI physx::PxHeightField*	LoadHeightFieldCollider		(PhysicsSystem* PS, Resources* RM, GUID_t Guid);

	/*
	FLEXKITAPI size_t			CreateCubeActor			(physx::PxMaterial* material, PScene* scene, float l, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV ={ 0, 0, 0 });
	FLEXKITAPI size_t			CreatePlaneCollider		(physx::PxMaterial* material, PScene* scene);
	FLEXKITAPI size_t			CreateSphereActor		(physx::PxMaterial* material, PScene* scene, float3 initialP = float3(), FlexKit::Quaternion initialQ = FlexKit::Quaternion::Identity(), float3 InitialV ={ 0, 0, 0 });
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

	/*

	struct ColliderBaseSystem
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

			Colliders.Release();
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

					LT_Entry LT(GetLocal(*Nodes, c.Node));
					LT.T = float3		(pose.p.x, pose.p.y, pose.p.z );
					LT.R = Quaternion	(pose.q.x, pose.q.y, pose.q.z, pose.q.w);

					SetLocal(*Nodes, c.Node, &LT);
				}
			}
		}


		NodeHandle GetNode(ComponentHandle C)
		{
			return Colliders[C].Node;
		}


		SceneNodeComponentSystem*	Nodes;
		Vector<Collider>			Colliders;
		Vector<Collider>			FreeColliders;
	};
		*/

	//const uint32_t ColliderComponentID = GetTypeGUID(ColliderSystem);

	/************************************************************************************************/

	/*
	struct CharacterControllerSystem : public FlexKit::ComponentSystemInterface
	{
		struct CharacterController
		{
			float3					CurrentPosition = 0;
			float3					Delta			= 0;
			float					Height			= 0.0f;
			float					Radius			= 0.0f;
			NodeHandle				Node			= NodeHandle(-1);
			physx::PxController*	Controller		= nullptr;
			bool					FloorContact	= false;
			bool					CeilingContact	= false;
			bool					SideContact		= false;
		};


		void Initiate		(SceneNodeComponentSystem* nodes, iAllocator* Memory, physx::PxScene* Scene);
		void UpdateSystem	(double dT);

		void DebugDraw		(FrameGraph* FGraph,  iAllocator* TempMemory)
		{
			for (auto& C : Controllers)
			{
				FK_ASSERT(0, "Not Implemented!");
				auto Pos = C.Controller->getFootPosition();
				//PushCapsule_Wireframe(FR, TempMemory, {Pos.x, Pos.y, Pos.z}, C.Radius, C.Height, GREEN);
			}
		}

		void Release()
		{
			ControllerManager->release();
		}


		void ReleaseHandle(ComponentHandle Handle)
		{
			Controllers[Handles[Handle]].Controller->release();

			if(Handles[Handle] != Controllers.size() - 1)
			{
				auto Temp		= Controllers.back();
				auto SwapHandle = Handles.find(Controllers.size() - 1);

				if(SwapHandle.INDEX)
				{
					Controllers.pop_back();
					Controllers[SwapHandle] = Temp;

					Handles.Indexes[SwapHandle] = Handles[Handle];
				}
			}
		}


		void HandleEvent(ComponentHandle Handle, ComponentType EventSource, ComponentSystemInterface* System, EventTypeID ID, ComponentListInterface* GO) final
		{
			if (EventSource == TransformComponentID && ID == GetCRCGUID(POSITION))
			{
				auto Index				= Handles[Handle];
				auto CurrentPosition	= Controllers[Index].CurrentPosition + Controllers[Index].Delta;
				auto Position			= Nodes->GetPositionL(Controllers[Index].Node);
				auto DeltaPosition		= Position - CurrentPosition;

				Controllers[Index].Delta += DeltaPosition;
			}
		}

		
		void SetNodeHandle(ComponentHandle Handle, NodeHandle Node)
		{
			auto Index = Handles[Handle];
			Nodes->ReleaseHandle(Controllers[Index].Node);
			Controllers[Index].Node = Node;
		}


		NodeHandle	GetNodeHandle(ComponentHandle Handle)
		{
			return Controllers[Handles[Handle]].Node;
		}


		ComponentHandle CreateCapsuleController(PhysicsSystem* PS, physx::PxScene* Scene, physx::PxMaterial* Mat,float R, float H, float3 InitialPosition = 0, Quaternion Q = Quaternion());

		HandleUtilities::HandleTable<ComponentHandle>	Handles;
		Vector<CharacterController>						Controllers;
		physx::PxControllerManager*						ControllerManager;
		SceneNodeComponentSystem*						Nodes;
	};

	const uint32_t CharacterControllerSystemID = GetTypeGUID(CharacterControllerSystem);

	/************************************************************************************************\/


	bool GetFloorContact(ComponentListInterface* GO, bool& out)
	{
		auto C = FindComponent(GO, CharacterControllerSystemID);
		if (C) {
			auto System = (CharacterControllerSystem*)C->ComponentSystem;
			out = System->Controllers[C->ComponentHandle].FloorContact;
			return true;
		}
		else
			return false;
	}


	/************************************************************************************************\/


	struct CapsuleCharacterArgs
	{
		ComponentHandle				CapsuleController;
		CharacterControllerSystem*	System;
	};


	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, CapsuleCharacterArgs& Args)
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


	/************************************************************************************************\/


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
			Colliders.Release();
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
		Vector<CubeCollider>	Colliders;
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
	void CreateComponent(ComponentList<SIZE>& GO, StaticBoxColliderArgs& Args)
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
			Colliders.Release();
		}

		void DebugDraw(FrameGraph* FGraph, iAllocator* TempMemory)
		{
			FK_ASSERT(0, "NOT IMPLEMENTED!");

			for (auto& C : Colliders)
			{
				auto pose = C.Cube->getGlobalPose();

				float3		Pos(pose.p.x, pose.p.y, pose.p.z );
				Quaternion	Q(pose.q.x, pose.q.y, pose.q.z, pose.q.w);
			}
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
		Vector<CubeCollider>	Colliders;
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
	void CreateComponent(ComponentList<SIZE>& GO, CubeColliderArgs& Args)
	{
		GO.AddComponent(Component(Args.Base,	Args.ColliderHandle,	ColliderComponentID));
		GO.AddComponent(Component(Args.System,	Args.CubeHandle,		CubeColliderComponentID));

		auto TransformComponent = (FlexKit::TansformComponent*)FindComponent(GO, FlexKit::TransformComponentID);

		if (TransformComponent) {
			auto ColliderNode = Args.Base->GetNode(Args.ColliderHandle);
			SetParent(GO, ColliderNode);
		}
	}

	struct PhysicsComponentSystem
	{
		PhysicsComponentSystem(PhysicsSystem* system, SceneNodeComponentSystem* nodes, iAllocator* memory) :
			System	(system),
			Nodes	(nodes),
			Memory	(memory)
		{
			FK_ASSERT(System && Nodes && Memory, "INVALID ARGUEMENT");

			physx::PxSceneDesc desc(System->Physx->getTolerancesScale());
			desc.gravity		= physx::PxVec3(0.0f, -9.81f, 0.0f);
			desc.filterShader	= physx::PxDefaultSimulationFilterShader;
			desc.cpuDispatcher	= System->CPUDispatcher;

			//Scn->Colliders.Allocator = allocator;

			//if (!desc.gpuDispatcher && game->Physics->GpuContextManger)
			//{
			//	desc.gpuDispatcher = game->Physics->GpuContextManger->getGpuDispatcher();
			//}

			Scene = System->Physx->createScene(desc);

			if (!Scene)
				FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

			UpdateColliders = false;

			physx::PxClientID CID = Scene->createClient();
			CID = CID;

			ControllerManager = PxCreateControllerManager(*Scene);

			CharacterControllers.Initiate(Nodes, memory, Scene);
			CubeColliders.Initiate(Memory);
			StaticBoxColliders.Initate(Memory);
			Base.Initiate(nodes, Memory);
		}

		void UpdateSystem			(double dT);
		void UpdateSystem_PreDraw	(double dT);

		void DebugDraw				(FlexKit::FrameGraph* FGraph, iAllocator* TempMemory);


		void Release				();

		StaticBoxColliderArgs	CreateStaticBoxCollider		( float3 XYZ = 1, float3 Pos = 0 );
		CubeColliderArgs		CreateCubeComponent			( float3 InitialP = 0, float3 InitialV = 0, float l = 10, Quaternion Q = Quaternion(0, 0, 0, 1) );
		CapsuleCharacterArgs	CreateCharacterController	( float3 InitialP = 0, float R = 5, float H = 5, Quaternion Q = Quaternion() );

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

*/

}	/************************************************************************************************/


#endif//PHYSICSUTILITIES_H