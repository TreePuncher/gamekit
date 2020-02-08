/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
#include "..\coreutilities\components.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\graphicScene.h"
#include "..\coreutilities\mathUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\coreutilities\threadUtilities.h"

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
	class PhysXScene;
	class PhysXComponent;
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

	const uint32_t CharacterControllerComponentID = GetTypeGUID(CharacterControllerSystem);

	/************************************************************************************************\/


	bool GetFloorContact(ComponentListInterface* GO, bool& out)
	{
		auto C = FindComponent(GO, CharacterControllerComponentID);
		if (C) {
			auto System = (CharacterControllerSystem*)C->ComponentSystem;
			out = System->Controllers[C->ComponentHandle].FloorContact;
			return true;
		}
		else
			return false;
	}

	*/


    /************************************************************************************************/


	typedef Handle_t<16, GetCRCGUID(StaticBodyHandle)>	StaticBodyHandle;
	typedef Handle_t<16, GetCRCGUID(RigidBodyHandle)>	RigidBodyHandle;
	typedef Handle_t<16, GetCRCGUID(PhysXSceneHandle)>	PhysXSceneHandle;
    typedef Handle_t<16, GetCRCGUID(PxShapeHandle)>		PxShapeHandle;


	/************************************************************************************************/


    struct Shape
    {
        physx::PxShape* _ptr;
    };


    /************************************************************************************************/


	class StaticColliderSystem
	{
	public:
		StaticColliderSystem(PhysXScene& IN_scene, iAllocator* IN_memory) :
			parentScene	{ &IN_scene	},
			colliders	{ IN_memory },
            dirtyFlags  { IN_memory, 128 }
		{
		}


        ~StaticColliderSystem()
        {
            Release();
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
				collider.actor->release();

			colliders.Release();
            dirtyFlags.Release();
		}


		void UpdateColliders()
		{
			for (size_t itr = 0; itr < dirtyFlags.size(); ++itr) 
			{
                if (dirtyFlags[itr])
                {
                    auto& collider = colliders[itr];
				    physx::PxTransform pose     = collider.actor->getGlobalPose();
				    Quaternion	orientation		= Quaternion{pose.q.x, pose.q.y, pose.q.z, pose.q.w};
				    float3		position		= float3(pose.p.x, pose.p.y, pose.p.z);

				    FlexKit::SetPositionW	(collider.node, position);
				    FlexKit::SetOrientation	(collider.node, orientation);

                    dirtyFlags[itr] = false;
                }
			}
		}


        auto& operator[] (StaticBodyHandle collider)
        {
            return colliders[collider];
        }

		struct StaticColliderObject
		{
			NodeHandle				node;
			physx::PxRigidStatic*	actor;
			physx::PxShape*			shape;
		};


        size_t push_back(const StaticColliderObject& object, bool initialDirtyFlag)
        {
            dirtyFlags.push_back(initialDirtyFlag);
            return colliders.push_back(object);
        }


		StaticColliderObject GetAPIObject(StaticBodyHandle collider)
		{
			return colliders[collider];
		}


        Vector<StaticColliderObject>	colliders;
        Vector<bool>	                dirtyFlags;
        PhysXScene*				        parentScene;
	};


    /************************************************************************************************/


	class RigidBodyColliderSystem
	{
	public:
		RigidBodyColliderSystem(PhysXScene& IN_scene, iAllocator* IN_memory) :
			parentScene	{ IN_scene  },
			colliders	{ IN_memory } {}


		void Release()
		{
			for (auto& collider : colliders)
				collider.actor->release();

			colliders.clear();
		}


		void UpdateColliders()
		{
			for (auto& collider : colliders) 
			{
				if (collider.actor->isSleeping())
					continue;

				physx::PxTransform  pose		 = collider.actor->getGlobalPose();
				Quaternion	        orientation	 = Quaternion{pose.q.x, pose.q.y, pose.q.z, pose.q.w};
                float3		        position     = float3{ pose.p.x, pose.p.y, pose.p.z };

				SetPositionW	(collider.node, position);
				SetOrientation	(collider.node, orientation);
			}
		}


		struct rbColliderObject
		{
			NodeHandle				node;
			physx::PxRigidDynamic*	actor;
		};


        rbColliderObject& operator[] (RigidBodyHandle collider)
        {
            return colliders[collider];
        }


		rbColliderObject GetAPIObject(RigidBodyHandle collider)
		{
			return colliders[collider];
		}

		Vector<rbColliderObject>	colliders;
        PhysXScene&				    parentScene;
	};


	class PhysXScene
	{
	public:
        PhysXScene(physx::PxScene* IN_scene, PhysXComponent& IN_system, iAllocator* IN_memory) :
			scene			{ IN_scene			},
			system			{ IN_system			},
			memory			{ IN_memory			},
			staticColliders	{ *this, IN_memory	},
			rbColliders		{ *this, IN_memory	}
		{
			FK_ASSERT(scene && memory, "INVALID ARGUEMENT");

			if (!scene)
				FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

			updateColliders		= false;
			CID					= scene->createClient();
		}


        PhysXScene(PhysXScene&& IN_scene) :
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


		~PhysXScene()
		{
			Release();
		}


        // uncopyable
        PhysXScene(const PhysXScene& scene)                 = delete;
        PhysXScene& operator = (const PhysXScene& scene)    = delete;


		void Release()
		{
            // Drain updates first
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


        auto& operator[] (RigidBodyHandle collider)
        {
            return rbColliders[collider];
        }


        auto& operator[] (StaticBodyHandle collider)
        {
            return staticColliders[collider];
        }


		void Update(double dT);
		void UpdateColliders		();

		void DebugDraw				(FrameGraph* FGraph, iAllocator* TempMemory);

        PxShapeHandle               CreateCubeShape(const float3 dimensions);

        StaticBodyHandle	        CreateStaticCollider    (Shape shape, float3 initialPosition, Quaternion initialQ);
        RigidBodyHandle		        CreateRigidBodyCollider (Shape shape, float3 initialPosition, Quaternion initialQ);

        void                        ReleaseCollider(StaticBodyHandle);
        void                        ReleaseCollider(RigidBodyHandle);

		void						SetPosition	(RigidBodyHandle, float3 xyz);
		void						SetMass     (RigidBodyHandle, float m);


	private:
		physx::PxScene*				scene				= nullptr;
		physx::PxControllerManager*	controllerManager	= nullptr;
		physx::PxClientID			CID;

		double	stepSize	= 1.0 / 60.0;
		double	T			= 0.0;
		bool	updateColliders;

		StaticColliderSystem		staticColliders;
		RigidBodyColliderSystem		rbColliders;

        PhysXComponent&				system;

		FNPSCENECALLBACK_POSTUPDATE PreUpdate;
		FNPSCENECALLBACK_PREUPDATE	PostUpdate;
		void*						User;

		iAllocator*					memory;
	};


	/************************************************************************************************/


    constexpr ComponentID PhysXComponentID = GetTypeGUID(RigidBodyComponentID);

	FLEXKITAPI class PhysXComponent : public FlexKit::Component<PhysXComponent, PhysXComponentID>
	{
	public:
        PhysXComponent(ThreadManager& threads, iAllocator* allocator);
		~PhysXComponent();

		void							Release();
		void							ReleaseScene				(PhysXSceneHandle);

        void                            Update(double dt);
		void							Simulate					(double dt);

		PhysXSceneHandle				CreateScene();
        StaticBodyHandle			    CreateStaticCollider	(PhysXSceneHandle, PxShapeHandle shape, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });
        RigidBodyHandle				    CreateRigidBodyCollider	(PhysXSceneHandle, PxShapeHandle shape, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });

        PxShapeHandle                   CreateCubeShape(const float3 dimensions);

        PhysXScene&                     GetScene_ref(PhysXSceneHandle handle);

		template<typename TY>
		PhysXScene&	GetOwningScene(TY handle)
		{
			return scenes[GetByType<PhysicsSceneHandle>(handle)];
		}

		operator PhysXComponent* () { return this; }

	private:
		physx::PxFoundation*			foundation;
		physx::PxPhysics*				physxAPI;

        double                          acc             = 0.0;
        size_t                          updateFrequency = 30;

		//physx::PxProfileZoneManager*	ProfileZoneManager;
		//physx::PxCooking*				Oven;


		bool							updateColliders;
		physx::PxPvd*					visualDebugger;
		physx::PxPvdTransport*			visualDebuggerConnection;
		//physx::PxGpuDispatcher*			GPUDispatcher;
		physx::PxMaterial*				defaultMaterial;

		bool							remoteDebuggerEnabled = true;


		class CpuDispatcher : public physx::PxCpuDispatcher
		{
		public:
			CpuDispatcher(ThreadManager& IN_threads, iAllocator* persistent_allocator = FlexKit::SystemAllocator) :
				threads		{ IN_threads			},
				allocator	{ persistent_allocator	} {}

			~CpuDispatcher() {}


			class PhysXTask : public iWork
			{
			public:
				PhysXTask(physx::PxBaseTask& IN_task, iAllocator* IN_memory = FlexKit::SystemAllocator) :
					iWork		{ IN_memory	},
					allocator	{ IN_memory	},
					task		{ IN_task	}
                {
                    _debugID = "PhysX Task";
                }


				virtual ~PhysXTask() final {}


				void Run() final override
				{
					task.run();
					task.release();
				}


				void Release() final override
				{
					allocator->release_aligned(this);
				}

			private:
				physx::PxBaseTask&	task;
				iAllocator*			allocator;
			};


			void submitTask(physx::PxBaseTask& pxTask)
			{
				auto& newTask = allocator->allocate_aligned<PhysXTask>(pxTask, allocator);
				threads.AddWork(newTask, allocator);
			}


			uint32_t getWorkerCount() const
			{
				return threads.GetThreadCount();
			}


		private:
			ThreadManager&		threads;
			iAllocator*			allocator;
		} dispatcher;


        Vector<Shape>               shapes;

		Vector<PhysXScene>			scenes;
		iAllocator*					allocator;
		ThreadManager&				threads;

		friend PhysXScene;
	};


    /************************************************************************************************/


    constexpr ComponentID StaticBodyComponentID  = GetTypeGUID(RigidBodyComponentID);

    class StaticBodyComponent : public Component<StaticBodyComponent, StaticBodyComponentID>
    {
    public:
        StaticBodyComponent(PhysXComponent& IN_physx) :
            physx{ IN_physx }
        {

        }


        void AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
        {
        }


        void Remove()
        {
            //instance.scene.ReleaseCollider(instance.handle);
        }


        auto& GetScene(PhysXSceneHandle sceneHandle)
        {
            return physx.GetScene_ref(sceneHandle);
        }


    private:
        PhysXComponent& physx;
    };


    /************************************************************************************************/


    class StaticBodyView : public ComponentView_t<StaticBodyComponent>
    {
    public:
        StaticBodyView(StaticBodyHandle IN_staticBody, PhysXSceneHandle IN_handle) :
            staticBody  { IN_staticBody },
            scene       { IN_handle     } {}

        NodeHandle GetNode() const
        {
            return GetComponent().GetScene(scene)[staticBody].node;
        }

        const PhysXSceneHandle    scene;
        const StaticBodyHandle    staticBody;
    };


    /************************************************************************************************/


    inline NodeHandle GetStaticBodyNode(GameObject& GO)
    {
        return Apply(GO,
            [](StaticBodyView& staticBody) -> NodeHandle
            {
                return staticBody.GetNode();
            },
            [] { return (NodeHandle)InvalidHandle_t; });
    }


	/************************************************************************************************/


    constexpr ComponentID RigicBodyComponentID = GetTypeGUID(RigicBodyComponentID);

    class RigidBodyComponent : public Component<RigidBodyComponent, RigicBodyComponentID>
    {
    public:
        RigidBodyComponent(PhysXComponent& IN_physx) :
            physx{ IN_physx }
        {

        }


        void AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
        {
        }


        void Remove()
        {
            //instance.scene.ReleaseCollider(instance.handle);
        }


        auto& GetScene(PhysXSceneHandle sceneHandle)
        {
            return physx.GetScene_ref(sceneHandle);
        }


    private:
        PhysXComponent& physx;
    };


    /************************************************************************************************/


    class RigidBodyView : public ComponentView_t<RigidBodyComponent>
    {
    public:
        RigidBodyView(RigidBodyHandle IN_staticBody, PhysXSceneHandle IN_handle) :
            staticBody{ IN_staticBody },
            scene{ IN_handle } {}

        NodeHandle GetNode() const
        {
            return GetComponent().GetScene(scene)[staticBody].node;
        }

        const PhysXSceneHandle    scene;
        const RigidBodyHandle     staticBody;
    };


    inline NodeHandle GetRigidBodyNode(GameObject& GO)
    {
        return Apply(GO,
            [](RigidBodyView& staticBody) -> NodeHandle
            {
                return staticBody.GetNode();
            },
            [] { return (NodeHandle)InvalidHandle_t; });
    }


    /************************************************************************************************/


	//FLEXKITAPI ColliderHandle			LoadTriMeshCollider		(PhysicsSystem* PS, GUID_t Guid);
	FLEXKITAPI physx::PxHeightField*	LoadHeightFieldCollider	(PhysXComponent& PX, GUID_t Guid);


}	/************************************************************************************************/


#endif//PHYSICSUTILITIES_H
