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

#include "buildsettings.h"
#include "components.h"
#include "containers.h"
#include "Events.h"
#include "graphicScene.h"
#include "mathUtils.h"
#include "memoryutilities.h"
#include "threadUtilities.h"

//#include <PxPhysicsAPI.h>
#include <physx/PxPhysicsAPI.h>
#include <physx/characterkinematic/PxController.h>
#include <physx/extensions/PxDefaultAllocator.h>
#include <physx/pvd/PxPvd.h>
#include <physx/pvd/PxPvdTransport.h>
#include <physx/characterkinematic/PxControllerManager.h>

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
#pragma comment(lib,	"PhysXCharacterKinematic_static_64.lib"	)
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


		void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_iAllocator = nullptr)
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


        auto& operator[] (const StaticBodyHandle collider)
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


        void Release();

        void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);


		struct rbColliderObject
		{
			NodeHandle				node;
			physx::PxRigidDynamic*	actor;
		};


        rbColliderObject& operator[] (const RigidBodyHandle collider);
        rbColliderObject GetAPIObject(const RigidBodyHandle collider);

		Vector<rbColliderObject>	colliders;
        PhysXScene&				    parentScene;
	};


    /************************************************************************************************/


	class PhysXScene
	{
	public:
        PhysXScene(physx::PxScene* IN_scene, PhysXComponent& IN_system, iAllocator* IN_memory) :
			scene			    { IN_scene			},
			system			    { IN_system			},
			memory			    { IN_memory			},
			staticColliders	    { *this, IN_memory	},
			rbColliders		    { *this, IN_memory	},
            controllerManager   { PxCreateControllerManager(*IN_scene) }
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

            if (controllerManager)
                controllerManager->release();
			if(scene)
				scene->release();

			scene				= nullptr;
			controllerManager	= nullptr;
		}


        auto& operator[] (const RigidBodyHandle collider)
        {
            return rbColliders[collider];
        }


        auto& operator[] (const StaticBodyHandle collider)
        {
            return staticColliders[collider];
        }


		void Update(const double dT, WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);
		void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);

		void DebugDraw				(FrameGraph* FGraph, iAllocator* TempMemory);

        [[nodiscard]] StaticBodyHandle  CreateStaticCollider    (const Shape shape, const float3 initialPosition, const Quaternion initialQ);
        [[nodiscard]] RigidBodyHandle	CreateRigidBodyCollider (const Shape shape, const float3 initialPosition, const Quaternion initialQ);


        void ReleaseCollider(const StaticBodyHandle);
        void ReleaseCollider(const RigidBodyHandle);

        void ApplyForce(const RigidBodyHandle, const float3 xyz);

		void SetPosition	        (const RigidBodyHandle, float3 xyz);
		void SetMass                (const RigidBodyHandle, float m);
        void SetRigidBodyPosition   (const RigidBodyHandle, const float3 xyz);

        physx::PxControllerManager& GetCharacterController() { return *controllerManager; }


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

        void                            Update(const double dt, iAllocator* temp_allocator);
		void							Simulate(const double dt, WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);

        [[nodiscard]] PhysXSceneHandle	CreateScene();
        [[nodiscard]] StaticBodyHandle	CreateStaticCollider	(const PhysXSceneHandle, const PxShapeHandle shape, const float3 pos = { 0, 0, 0 }, const Quaternion q = { 0, 0, 0, 1 });
        [[nodiscard]] RigidBodyHandle	CreateRigidBodyCollider	(const PhysXSceneHandle, const PxShapeHandle shape, const float3 pos = { 0, 0, 0 }, const Quaternion q = { 0, 0, 0, 1 });
        [[nodiscard]] PxShapeHandle     CreateCubeShape         (const float3 dimensions);

        PhysXScene&                     GetScene_ref(PhysXSceneHandle handle);
        physx::PxMaterial*              GetDefaultMaterial() const { return defaultMaterial;}


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


				void Run(iAllocator& allocator) final override
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



            operator ThreadManager& () { return threads; }

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


    constexpr ComponentID StaticBodyComponentID  = GetTypeGUID(StaticBodyComponentID);

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


    inline NodeHandle GetStaticBodyNode(GameObject& GO);


	/************************************************************************************************/


    constexpr ComponentID RigicBodyComponentID = GetTypeGUID(RigicBodyComponentID);

    class RigidBodyView;

    class RigidBodyComponent : public Component<RigidBodyComponent, RigicBodyComponentID>
    {
    public:
        RigidBodyComponent(PhysXComponent& IN_physx) :
            physx{ IN_physx } {}


        RigidBodyView CreateRigidBody(PhysXSceneHandle IN_scene);


        void AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
        {
            FK_ASSERT(0);
        }


        void Remove(RigidBodyView& rigidBody);

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
        RigidBodyView(RigidBodyHandle IN_staticBody, PhysXSceneHandle IN_scene) :
            staticBody  { IN_staticBody },
            scene       { IN_scene} {}

        NodeHandle GetNode() const;

        const PhysXSceneHandle    scene;
        const RigidBodyHandle     staticBody;
    };


    NodeHandle  GetRigidBodyNode(GameObject& GO);
    void        SetRigidBodyPosition(GameObject& GO, const float3 worldPOS);
    void        ApplyForce(GameObject& GO, const float3 force);



    /************************************************************************************************/


    constexpr ComponentID CharacterControllerComponentID = GetTypeGUID(CharacterControllerComponentID);
    using CharacterControllerHandle = Handle_t<32, CharacterControllerComponentID>;

    struct CharacterController
    {
        NodeHandle                node;
        PhysXSceneHandle          scene;

        physx::PxController*      controller;

        float2                    mouseMoved = { 0, 0 };
        double                    updateTimer = 0;
    };


    class CharacterControllerComponent : public Component<CharacterControllerComponent, CharacterControllerComponentID>
    {
    public:
        CharacterControllerComponent(PhysXComponent& IN_physx, iAllocator* allocator) :
            physx       { IN_physx  },
            controllers { allocator } {}


        CharacterControllerHandle Create(const PhysXSceneHandle scene, const NodeHandle node = GetZeroedNode(), const float3 initialPosition = {}, const float R = 1, const float H = 1)
        {
            auto& manager = physx.GetScene_ref(scene).GetCharacterController();

            SetPositionW(node, initialPosition + float3{0, H / 2, 0});

            physx::PxCapsuleControllerDesc CCDesc;
            CCDesc.material         = physx.GetDefaultMaterial();
            CCDesc.radius           = 1;
            CCDesc.height           = 1;
            CCDesc.contactOffset    = 0.01f;
            CCDesc.position         = { initialPosition.x, initialPosition.y, initialPosition.z };
            CCDesc.climbingMode     = physx::PxCapsuleClimbingMode::eEASY;

            auto controller = manager.createController(CCDesc);

            auto idx = controllers.push_back(
                CharacterController{
                    node,
                    scene,
                    controller
                });

            return CharacterControllerHandle{ idx };
        }


        void AddComponentView(GameObject& GO, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
        {
            FK_ASSERT(0);
        }


        void Remove()
        {
            //instance.scene.ReleaseCollider(instance.handle);
        }


        auto& GetScene(PhysXSceneHandle sceneHandle)
        {
            return physx.GetScene_ref(sceneHandle);
        }


        CharacterController& operator[] (CharacterControllerHandle controller)
        {
            return controllers[controller];
        }

    private:

        Vector<CharacterController> controllers;
        PhysXComponent&             physx;
    };


    /************************************************************************************************/


    class CharacterControllerView : public ComponentView_t<CharacterControllerComponent>
    {
    public:
        CharacterControllerView(
            PhysXSceneHandle scene,
            const float3    initialPosition = { 0, 0, 0 },
            NodeHandle      node            = GetZeroedNode(),
            const float     R               = 1.0f,
            const float     H               = 1.0f) :
                controller{ GetComponent().Create(scene, node, initialPosition) } {}


        CharacterControllerView(CharacterControllerHandle IN_controller) :
            controller  { IN_controller } {}


        NodeHandle GetNode() const
        {
            return GetComponent()[controller].node;
        }

        struct MoveFilter {};

        void SetPosition(const float3 xyz)
        {
            auto& ref = GetComponent()[controller];
            ref.controller->setPosition({ xyz.x, xyz.y, xyz.z });
            SetPositionW(ref.node, xyz);
        }


        CharacterControllerHandle controller;
    };


    NodeHandle GetControllerNode(GameObject& GO)
    {
        return Apply(GO, [](CharacterControllerView& controller)
            {
                return controller.GetNode();
            },
            []
            {
                return (NodeHandle)InvalidHandle_t;
            });
    }


    CharacterControllerHandle GetControllerHandle(GameObject& GO)
    {
        return Apply(GO, [](CharacterControllerView& controller)
            {
                return controller.controller;
            },
            []
            {
                return (CharacterControllerHandle)InvalidHandle_t;
            });
    }
    
    void SetControllerPosition(GameObject& GO, const float3 xyz)
    {
        Apply(GO, [&](CharacterControllerView& controller)
            {
                controller.SetPosition(xyz);
            });
    }

    /************************************************************************************************/


    constexpr ComponentID CameraControllerComponentID   = GetTypeGUID(ThirdPersonCameraComponentID);
    using CameraControllerHandle                        = Handle_t<32, CameraControllerComponentID>;


    struct ThirdPersonCamera
    {
        ThirdPersonCamera(
            CharacterControllerHandle   IN_controller        = InvalidHandle_t,
            NodeHandle                  IN_node              = GetZeroedNode(),
            CameraHandle                IN_camera            = CameraComponent::GetComponent().CreateCamera(),
            float3                      initialPos           = { 0, 0, 0 },
            const float                 initialMovementSpeed = 500);

        static physx::PxFilterFlags Test(
            physx::PxFilterObjectAttributes attributes0,
            physx::PxFilterData filterData0,
            physx::PxFilterObjectAttributes attributes1,
            physx::PxFilterData filterData1,
            physx::PxPairFlags& pairFlags,
            const byte* constantBlock,
            physx::PxU32 constantBlockSize);


        ThirdPersonCamera(const ThirdPersonCamera&) = default;

        Quaternion	GetOrientation()     const;
        float3      GetForwardVector()   const;
        float3      GetRightVector()     const;

        void Rotate (const float3 xyz);
        void Yaw    (const float theta);
        void Pitch  (const float theta);
        void Roll   (const float theta);


        float3 GetHeadPosition() const;

        void Update(const float2 mouseInput, const double dt);

        CharacterControllerHandle   controller    = InvalidHandle_t;
        CameraHandle                camera        = InvalidHandle_t;

        NodeHandle cameraNode   = InvalidHandle_t;
        NodeHandle yawNode      = InvalidHandle_t;
        NodeHandle pitchNode    = InvalidHandle_t;
        NodeHandle rollNode     = InvalidHandle_t;

        float3          velocity     = 0;
        float           acceleration = 500;
        float           drag         = 5.0;
        float			moveRate     = 100;

        struct KeyStates
        {
            bool forward  = false;
            bool backward = false;
            bool left     = false;
            bool right    = false;
            bool up       = false;
            bool down     = false;

            bool KeyPressed()
            {
                return forward | backward | left | right | up | down;
            }
        }keyStates;
    };


    /************************************************************************************************/


    using CameraControllerComponent     = BasicComponent_t<ThirdPersonCamera, CameraControllerHandle, CameraControllerComponentID>;
    using CameraControllerView          = CameraControllerComponent::View;


    CameraHandle    GetCameraControllerCamera(GameObject& GO);
    GameObject&     CreateThirdPersonCameraController(PhysXSceneHandle scene, iAllocator* allocator, const float R = 1, const float H = 1);
    auto&           UpdateThirdPersonCameraControllers(UpdateDispatcher& dispatcher, float2 mouseInput, const double dT);

    float3          GetCameraControllerHeadPosition(GameObject& GO);
    float3          GetCameraControllerForwardVector(GameObject& GO);


    /************************************************************************************************/


    enum ThirdPersonCameraEvents
    {
        TPC_MoveForward,
        TPC_MoveBackward,
        TPC_MoveLeft,
        TPC_MoveRight,
        TPC_MoveUp,
        TPC_MoveDown,
    };


    void HandleEvents(GameObject& GO, Event evt);


    /************************************************************************************************/

	//FLEXKITAPI ColliderHandle			LoadTriMeshCollider		(PhysicsSystem* PS, GUID_t Guid);
	FLEXKITAPI physx::PxHeightField*	LoadHeightFieldCollider	(PhysXComponent& PX, GUID_t Guid);


}	/************************************************************************************************/


#endif//PHYSICSUTILITIES_H
