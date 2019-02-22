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
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\graphicScene.h"
#include "..\coreutilities\ThreadUtilities.h"

#include "PxPhysicsAPI.h"
#include <PhysX_sdk/physx/include/characterkinematic/PxController.h>
#include <PhysX_sdk/physx/include/extensions/PxDefaultAllocator.h>
#include <PhysX_sdk/physx/include/pvd/PxPvd.h>
#include <PhysX_sdk/physx/include/pvd/PxPvdTransport.h>
#include <PhysX_sdk/physx/include/characterkinematic/PxControllerManager.h>

#ifdef _DEBUG
#pragma comment(lib,	"LowLevel_static_64.lib"				)
#pragma comment(lib,	"PhysX_64.lib"							)
#pragma comment(lib,	"PhysXFoundation_64.lib"				)
#pragma comment(lib,	"PhysXCommon_64.lib"					)
#pragma comment(lib,	"PhysXExtensions_static_64.lib"			)
#pragma comment(lib,	"PhysXPvdSDK_static_64.lib"				)
#pragma comment(lib,	"PhysXCharacterKinematic_static_64.lib"	)
#else
#pragma comment(lib,	"LowLevel_static_64.lib"			)
#pragma comment(lib,	"PhysX_64.lib"						)
#pragma comment(lib,	"PhysXFoundation_64.lib"			)
#pragma comment(lib,	"PhysXCommon_64.lib"				)
#pragma comment(lib,	"PhysXExtensions_static_64.lib"		)
#pragma comment(lib,	"PhysXPvdSDK_static_64.lib"			)
#endif

namespace FlexKit
{
	// Forward Declarations
	class PhysicsScene;
	class PhysicsSystem;
	class StaticColliderSystem;

	static physx::PxDefaultErrorCallback	gDefaultErrorCallback;
	static physx::PxDefaultAllocator		gDefaultAllocatorCallback;

	typedef void(*FNPSCENECALLBACK_POSTUPDATE)(void*);
	typedef void(*FNPSCENECALLBACK_PREUPDATE) (void*);


	/************************************************************************************************/


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

	*/

	

	typedef Handle_t<16, GetCRCGUID(StaticColliderHandle)>	StaticColliderEntityHandle;
	typedef Handle_t<16, GetCRCGUID(rbColliderHandle)>		rbColliderEntityHandle;
	typedef Handle_t<16, GetCRCGUID(PhysicsScene)>			PhysicsSceneHandle;

	using StaticColliderHandle	= Pair<PhysicsSceneHandle, StaticColliderEntityHandle>;
	using rbColliderHandle		= Pair<PhysicsSceneHandle, rbColliderEntityHandle>;


	/************************************************************************************************/


	class StaticColliderSystem
	{
	public:
		StaticColliderSystem(PhysicsScene* IN_scene, iAllocator* IN_memory) :
			parentScene	{ IN_scene	},
			colliders	{ IN_memory }
		{
		}

		// NON COPYABLE
		StaticColliderSystem				(const StaticColliderSystem&) = delete;
		StaticColliderSystem& operator =	(const StaticColliderSystem&) = delete;

		// MOVEABLE
		StaticColliderSystem(StaticColliderSystem&& IN_staticColliders) : 
			colliders{ std::move(IN_staticColliders.colliders) }
		{
			parentScene = IN_staticColliders.parentScene;

			IN_staticColliders.parentScene = nullptr;
		}


		void Release()
		{
			for (auto& collider : colliders)
				collider.staticActor->release();

			colliders.clear();
		}


		void UpdateColliders()
		{
			for (auto& collider : colliders) 
			{
				physx::PxTransform pose = collider.staticActor->getGlobalPose();
				Quaternion	orientation		= Quaternion{pose.q.x, pose.q.y, pose.q.z, pose.q.w};
				float3		position		= float3(pose.p.x, pose.p.y, pose.p.z);

				FlexKit::SetPositionW	(collider.sceneNode, position);
				FlexKit::SetOrientation	(collider.sceneNode, orientation);
			}
		}


		struct StaticColliderObject
		{
			NodeHandle				sceneNode;
			physx::PxRigidStatic*	staticActor;
			physx::PxShape*			shape;
		};


		StaticColliderObject GetAPIObject(StaticColliderEntityHandle collider)
		{
			return colliders[collider];
		}

		Vector<StaticColliderObject>	colliders;
		PhysicsScene*					parentScene;
	};


	class RigidBodyColliderSystem
	{
	public:
		RigidBodyColliderSystem(PhysicsScene* IN_scene, iAllocator* IN_memory) :
			parentScene	{ IN_scene },
			colliders	{ IN_memory }
		{
		}


		void Release()
		{
			for (auto& collider : colliders)
				collider.dynamicActor->release();

			colliders.clear();
		}


		void UpdateColliders()
		{
			for (auto& collider : colliders) 
			{
				if (collider.dynamicActor->isSleeping())
					continue;

				physx::PxTransform pose		= collider.dynamicActor->getGlobalPose();
				Quaternion	orientation		= Quaternion{pose.q.x, pose.q.y, pose.q.z, pose.q.w};
				float3		position		= float3(pose.p.x, pose.p.y, pose.p.z);

				FlexKit::SetPositionW	(collider.sceneNode, position);
				FlexKit::SetOrientation	(collider.sceneNode, orientation);
			}
		}


		struct rbColliderObject
		{
			NodeHandle				sceneNode;
			physx::PxRigidDynamic*	dynamicActor;
			physx::PxShape*			shape;
		};


		rbColliderObject GetAPIObject(rbColliderEntityHandle collider)
		{
			return colliders[collider];
		}

		Vector<rbColliderObject>	colliders;
		PhysicsScene*				parentScene;
	};


	class PhysicsScene
	{
	public:
		PhysicsScene(physx::PxScene* IN_scene, PhysicsSystem* IN_system, iAllocator* IN_memory) :
			scene			{ IN_scene			},
			system			{ IN_system			},
			memory			{ IN_memory			},
			staticColliders	{ this, IN_memory	},
			rbColliders		{ this, IN_memory	}
		{
			FK_ASSERT(scene && system && memory, "INVALID ARGUEMENT");

			if (!scene)
				FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

			updateColliders		= false;
			CID					= scene->createClient();
		}


		PhysicsScene(PhysicsScene&& IN_scene) :
			staticColliders		{ std::move(IN_scene.staticColliders)	},
			rbColliders			{ std::move(IN_scene.rbColliders)		},
			scene				{ IN_scene.scene						},
			controllerManager	{ IN_scene.controllerManager			},
			CID					{ IN_scene.CID							},
			system				{ IN_scene.system						},
			memory				{ IN_scene.memory						}
		{
			staticColliders.parentScene = this;
		}


		~PhysicsScene()
		{
			Release();
		}


		void Release()
		{
			while (updateColliders)
			{
				if (scene->checkResults())
				{
					updateColliders = false;

					if (scene->fetchResults(true)) {
						UpdateColliders();
					}
				}
			}


			staticColliders.Release();
			rbColliders.Release();

			if(scene)
				scene->release();
			if(controllerManager)
				controllerManager->release();

			scene				= nullptr;
			controllerManager	= nullptr;
		}


		void Update(double dT);
		void UpdateColliders		();

		void DebugDraw				(FrameGraph* FGraph, iAllocator* TempMemory);

		StaticColliderEntityHandle	CreateStaticBoxCollider		(float3 dimensions, float3 initialPosition, Quaternion initialQ);
		rbColliderEntityHandle		CreateRigidBodyBoxCollider	(float3 dimensions, float3 initialPosition, Quaternion initialQ, NodeHandle node);

		void						SetPosition	(rbColliderEntityHandle, float3 xyz);
		void						SetMass(rbColliderEntityHandle, float m);


	private:
		physx::PxScene*				scene				= nullptr;
		physx::PxControllerManager*	controllerManager	= nullptr;
		physx::PxClientID			CID;

		double	stepSize = 1.0 / 60.0;
		double	T;
		bool	updateColliders;

		StaticColliderSystem		staticColliders;
		RigidBodyColliderSystem		rbColliders;

		PhysicsSystem*				system;

		FNPSCENECALLBACK_POSTUPDATE PreUpdate;
		FNPSCENECALLBACK_PREUPDATE	PostUpdate;
		void*						User;

		iAllocator*					memory;
	};


	/************************************************************************************************/


	FLEXKITAPI class PhysicsSystem
	{
	public:
		PhysicsSystem(ThreadManager& threads, iAllocator* allocator);
		~PhysicsSystem();

		void							Release();
		void							ReleaseScene				(PhysicsSceneHandle);

		void							Simulate					(double dt);

		PhysicsSceneHandle				CreateScene();
		StaticColliderHandle			CreateStaticBoxCollider		(PhysicsSceneHandle, float3 xyz = { 10, 10, 10 }, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });

		rbColliderHandle				CreateRigidBodyBoxCollider	(PhysicsSceneHandle, float3 xyz, float3 pos, Quaternion q, NodeHandle node);

		template<typename TY>
		PhysicsScene*					GetOwningScene(TY handle)
		{
			return &scenes[GetByType<PhysicsSceneHandle>(handle)];
		}

		operator PhysicsSystem* () { return this; }

	private:
		physx::PxFoundation*			foundation;
		physx::PxPhysics*				physxAPI;
		//physx::PxProfileZoneManager*	ProfileZoneManager;
		//physx::PxCooking*				Oven;


		bool							updateColliders;
		physx::PxPvd*					visualDebugger;
		physx::PxPvdTransport*			visualDebuggerConnection;
		physx::PxGpuDispatcher*			GPUDispatcher;
		physx::PxMaterial*				defaultMaterial;

		bool							remoteDebuggerEnabled;


		class CpuDispatcher : 
			public physx::PxCpuDispatcher
		{
		public:
			CpuDispatcher(ThreadManager& IN_threads, iAllocator* persistent_memory = FlexKit::SystemAllocator) :
				threads		{ IN_threads		},
				freeList	{ persistent_memory },
				memory		{ persistent_memory }{}

			~CpuDispatcher(){}


			class PhysXTask : public iWork
			{
			public:
				PhysXTask(physx::PxBaseTask& IN_task, iAllocator* IN_memory = FlexKit::SystemAllocator) :
					iWork	{ IN_memory	},
					memory	{ IN_memory	},
					task	{ IN_task	} {}


				virtual ~PhysXTask() 
				{
				}


				void Run()		override 
				{
					task.run();
					task.release();
				}


				void Release()	override 
				{
					if (memory)
					{
						this->~PhysXTask();
						memory->_aligned_free(this);
						//delete this;
						memory = nullptr;
					}
					else
						FK_LOG_ERROR("DOUBLE FREE DETECTTED!");
				}

			private:
				physx::PxBaseTask&	task;
				iAllocator*			memory;
			};


			void submitTask(physx::PxBaseTask& pxTask)
			{
				jobsInProgress++;
				auto& newTask = memory->allocate_aligned<PhysXTask>(pxTask, memory);
				//auto newTask = new PhysXTask(pxTask, memory);
				newTask.Subscribe([&] {jobsInProgress--; });

				FK_ASSERT(newTask != nullptr);

				threads.AddWork(newTask, memory);
			}


			uint32_t getWorkerCount() const
			{
				return threads.GetThreadCount();
			}


		private:
			atomic_int			jobsInProgress = 0;
			ThreadManager&		threads;
			Vector<PhysXTask*>	freeList; // TODO: thread safe memory releasing!
			iAllocator*			memory;
		} dispatcher;

		physx::PxDefaultCpuDispatcher*	pxDispatcher;

		Vector<PhysicsScene>			scenes;
		iAllocator*						memory;
		ThreadManager&					threads;
		friend PhysicsScene;
	};


	/************************************************************************************************/


	class StaticColliderObjectBehavior :
		public SceneNodeBehavior<>
	{
	public:
		StaticColliderObjectBehavior(NodeHandle node) : 
			SceneNodeBehavior<>(node){}


		PhysicsSystem*			system;
		StaticColliderHandle	handle;
	};



	/************************************************************************************************/

	
	class RigidBodyObjectBehavior
	{
	public:
		RigidBodyObjectBehavior(
			PhysicsSystem* IN_system		= nullptr, 
			rbColliderHandle IN_collider	= { InvalidHandle_t, InvalidHandle_t }) :
				collider	{ IN_collider	},
				system		{ IN_system		}{}

		RigidBodyObjectBehavior(RigidBodyObjectBehavior&& rhs) :
			system		{ rhs.system	},
			collider	{ rhs.collider	}
		{
			rhs.system		= nullptr;
			rhs.collider	= { InvalidHandle_t, InvalidHandle_t };
		}

		RigidBodyObjectBehavior& operator = (RigidBodyObjectBehavior&& rhs)
		{
			system		= rhs.system;
			collider	= rhs.collider;

			rhs.system		= nullptr;
			rhs.collider	= { InvalidHandle_t, InvalidHandle_t };

			return *this;
		}

		// non-copyable
		RigidBodyObjectBehavior				(RigidBodyObjectBehavior&) = delete;
		RigidBodyObjectBehavior& operator = (RigidBodyObjectBehavior&) = delete;


		bool GetSleeping() { return false; }

		PhysicsSystem*		system;
		rbColliderHandle	collider;
	};


	/************************************************************************************************/


	class RigidBodyDrawableBehavior :
		public RigidBodyObjectBehavior
	{
	public:
		RigidBodyDrawableBehavior(
				PhysicsSystem*		system		= nullptr, 
				rbColliderHandle	IN_collider = { InvalidHandle_t, InvalidHandle_t }, 
				GraphicScene*		scene		= nullptr, 
				SceneEntityHandle	entity		= InvalidHandle_t, 
				NodeHandle			IN_node		= InvalidHandle_t) :
			RigidBodyObjectBehavior	{ system, IN_collider		},
			drawable				{ scene, entity, IN_node	}
		{
			drawable.ToggleScaling(true);
		}


		RigidBodyDrawableBehavior(RigidBodyDrawableBehavior&& rhs) : 
			drawable{ std::move(rhs.drawable) }{}


		RigidBodyDrawableBehavior& operator =	(RigidBodyDrawableBehavior&& rhs)
		{
			collider	= rhs.collider;
			system		= rhs.system;
			drawable	= std::move(rhs.drawable);
			
			rhs.collider	= { InvalidHandle_t, InvalidHandle_t };
			rhs.system		= nullptr;

			return *this;
		}

		// non-copyable
		RigidBodyDrawableBehavior				(RigidBodyDrawableBehavior&) = delete;
		RigidBodyDrawableBehavior& operator =	(RigidBodyDrawableBehavior&) = delete;


		void SetPosition(float3 xyz)
		{
			system->GetOwningScene(collider)->SetPosition(collider, xyz);
		}


		void SetMeshScale(float r)
		{
			drawable.Scale(float3{ r, r, r });
		}


		void SetMass(float m)
		{
			system->GetOwningScene(collider)->SetMass(collider, m);

		}


		void SetVisable(bool b)
		{
			drawable.SetVisable(b);
		}

		float3 GetPosition()
		{
			return drawable.GetPosition();
		}

		DrawableBehavior drawable;
	};


	/************************************************************************************************/


	FLEXKITAPI RigidBodyDrawableBehavior CreateRBCube(
		GraphicScene* gScene, 
		TriMeshHandle mesh, 
		PhysicsSystem* physicsSystem, 
		PhysicsSceneHandle pScene, 
		float hR, 
		float3 pos = float3{0, 0, 0});


	/************************************************************************************************/


	//FLEXKITAPI ColliderHandle			LoadTriMeshCollider		(PhysicsSystem* PS, GUID_t Guid);
	FLEXKITAPI physx::PxHeightField*	LoadHeightFieldCollider	(PhysicsSystem* PS, GUID_t Guid);


}	/************************************************************************************************/


#endif//PHYSICSUTILITIES_H