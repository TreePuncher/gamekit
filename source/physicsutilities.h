/**********************************************************************

Copyright (c) 2015 - 2021 Robert May

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
#include "Scene.h"
#include "mathUtils.h"
#include "memoryutilities.h"
#include "threadUtilities.h"
#include "ResourceHandles.h"

#include <PxPhysicsAPI.h>
#include <characterkinematic/PxController.h>
#include <extensions/PxDefaultAllocator.h>
#include <pvd/PxPvd.h>
#include <pvd/PxPvdTransport.h>
#include <characterkinematic/PxControllerManager.h>
#include <PxQueryReport.h>

#ifdef _DEBUG
#pragma comment(lib,	"LowLevel_static_64.lib"				)
#pragma comment(lib,	"PhysX_64.lib"							)
#pragma comment(lib,	"PhysXFoundation_64.lib"				)
#pragma comment(lib,	"PhysXCommon_64.lib"					)
#pragma comment(lib,	"PhysXExtensions_static_64.lib"			)
#pragma comment(lib,	"PhysXPvdSDK_static_64.lib"				)
#pragma comment(lib,	"PhysXCharacterKinematic_static_64.lib"	)
#pragma comment(lib,    "PhysXCooking_64.lib")  
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
	class PhysicsLayer;
	class PhysXComponent;
	class StaticColliderSystem;

	static physx::PxDefaultErrorCallback	gDefaultErrorCallback;
	static physx::PxDefaultAllocator		gDefaultAllocatorCallback;

	typedef void(*FNPSCENECALLBACK_POSTUPDATE)(void*);
	typedef void(*FNPSCENECALLBACK_PREUPDATE) (void*);


	/************************************************************************************************/


	inline float3                   pxVec3ToFloat3(const physx::PxExtendedVec3 v)    { return { (float)v.x, (float)v.y, (float)v.z }; }
	inline float3                   pxVec3ToFloat3(const physx::PxVec3 v)            { return { v.x, v.y, v.z }; }
	inline physx::PxVec3            Float3TopxVec3(const float3 v)                   { return { v.x, v.y, v.z }; }
	inline physx::PxExtendedVec3    Float3TopxVec3Ext(const float3 v)                { return { v.x, v.y, v.z }; }

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


	struct Shape
	{
		physx::PxShape* _ptr;

		operator bool() const noexcept { return _ptr != nullptr; }
	};


	/************************************************************************************************/


	class StaticColliderSystem
	{
	public:
		StaticColliderSystem(PhysicsLayer& IN_layer, iAllocator* IN_memory) :
			handles		{ IN_memory },
			layer		{ &IN_layer },
			colliders	{ IN_memory },
			dirtyFlags	{ IN_memory, 128 } {}


		~StaticColliderSystem()
		{
			Release();
		}

		// NON COPYABLE
		StaticColliderSystem				(const StaticColliderSystem&) = delete;
		StaticColliderSystem& operator =	(const StaticColliderSystem&) = delete;

		// MOVEABLE
		StaticColliderSystem(StaticColliderSystem&& IN_staticColliders) noexcept;

		void Release();

		void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_iAllocator = nullptr);

		auto& operator[] (const StaticBodyHandle collider)
		{
			return colliders[handles[collider]];
		}


		struct StaticColliderObject
		{
			NodeHandle				node;
			StaticBodyHandle		handle;
			physx::PxRigidStatic*	actor;
			GameObject*				gameObject;
			void*					User;
		};


		StaticBodyHandle		AddCollider(const StaticColliderObject& object, bool initialDirtyFlag);
		StaticColliderObject	GetAPIObject(StaticBodyHandle collider);

		using HandleTable = FlexKit::HandleUtilities::HandleTable<StaticBodyHandle>;

		Vector<StaticColliderObject>	colliders;
		Vector<bool>					dirtyFlags;
		HandleTable						handles;
		PhysicsLayer*					layer;
	};


	/************************************************************************************************/


	class RigidBodyColliderSystem
	{
	public:
		RigidBodyColliderSystem(PhysicsLayer& IN_layer, iAllocator* IN_memory) :
			layer       { IN_layer  },
			colliders	{ IN_memory } {}


		// NON COPYABLE
		RigidBodyColliderSystem             (const RigidBodyColliderSystem&) = delete;
		RigidBodyColliderSystem& operator =	(const RigidBodyColliderSystem&) = delete;

		// MOVEABLE
		RigidBodyColliderSystem(RigidBodyColliderSystem&& IN_staticColliders);


		void Release();

		void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);


		struct rbColliderObject
		{
			NodeHandle				node;
			physx::PxRigidDynamic*	actor;
			void*                   User;
		};


		rbColliderObject& operator[] (const RigidBodyHandle collider);
		rbColliderObject GetAPIObject(const RigidBodyHandle collider);

		Vector<rbColliderObject>	colliders;
		PhysicsLayer&				layer;
	};


	/************************************************************************************************/

	using PhysicsLayerUpdateCallback = FlexKit::TypeErasedCallable<void (WorkBarrier&, iAllocator&, double), 64>;

	enum class PhysicsDebugOverlayMode
	{
		Wireframe,
		Solid,
		Disabled
	};

	class PhysicsLayer
	{
	public:
		PhysicsLayer(physx::PxScene* IN_scene, PhysXComponent& IN_system, iAllocator* IN_memory);
		PhysicsLayer(PhysicsLayer&& IN_scene);

		~PhysicsLayer();


		// uncopyable
		PhysicsLayer(const PhysicsLayer& scene)                 = delete;
		PhysicsLayer& operator = (const PhysicsLayer& scene)    = delete;


		void Release();


		RigidBodyColliderSystem::rbColliderObject&  operator[] (const RigidBodyHandle collider);
		StaticColliderSystem::StaticColliderObject& operator[] (const StaticBodyHandle collider);
		

		void Update(const double dT, WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);
		void UpdateColliders(WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);
		void CallUserUpdates(WorkBarrier& barrier, iAllocator& allocator, double dT);

		void UpdateDebugGeometry();

		[[nodiscard]] StaticBodyHandle  CreateStaticCollider    (GameObject* go, const float3 initialPosition, const Quaternion initialQ);
		[[nodiscard]] RigidBodyHandle	CreateRigidBodyCollider (GameObject* go, const float3 initialPosition, const Quaternion initialQ);

		void ReleaseCollider(const StaticBodyHandle);
		void ReleaseCollider(const RigidBodyHandle);

		void ApplyForce(const RigidBodyHandle, const float3 xyz);

		void SetPosition	        (const RigidBodyHandle, float3 xyz);
		void SetMass                (const RigidBodyHandle, float m);
		void SetRigidBodyPosition   (const RigidBodyHandle, const float3 xyz);

		void AddUpdateCallback(PhysicsLayerUpdateCallback&& callback);

		struct RayCastHit
		{
			const float     distance;
			const float3    normal;
		};

		template<typename FN_callable>
		struct HitCallback : public physx::PxRaycastCallback
		{
			HitCallback(FN_callable callable) :
				physx::PxRaycastCallback{ nullptr, 0 },
				hitHandler{ callable }{}

			physx::PxAgain processTouches(const physx::PxRaycastHit* hit, physx::PxU32 nbHits) final
			{
				return hitHandler(
					RayCastHit{
						.distance   = (float)hit->distance,
						.normal     = pxVec3ToFloat3(hit->normal) } );
			}

			void finalizeQuery() final override
			{
				if(hasBlock)
					hitHandler(
						RayCastHit{
							.distance   = (float)block.distance,
							.normal     = pxVec3ToFloat3(block.normal) });
			}

			FN_callable hitHandler;
		};

		template<typename TY_HITFN>
		bool RayCast(const float3 origin, const float3 ray, const float maxDistance, TY_HITFN OnHit)
		{
			const auto pxOrigin = Float3TopxVec3(origin);
			const auto pxRay    = Float3TopxVec3(ray);

			HitCallback<TY_HITFN> hitCallback{ OnHit };

			return scene->raycast(pxOrigin, pxRay, maxDistance, hitCallback);
		}

		physx::PxControllerManager& GetCharacterController() { return *controllerManager; }


		physx::PxScene*				scene				= nullptr;
		physx::PxControllerManager*	controllerManager	= nullptr;
		physx::PxClientID			CID;

		double	stepSize	= 1.0 / 120.0f;
		double	T			= 0.0;
		bool	updateColliders;


		struct DebugVertex
		{
			FlexKit::float4 position;
			FlexKit::float4 color;
			FlexKit::float2 UV;
		};


		Vector<PhysicsLayerUpdateCallback>  userCallback;
		StaticColliderSystem		        staticColliders;
		RigidBodyColliderSystem		        rbColliders;

		PhysXComponent&				system;

		FNPSCENECALLBACK_POSTUPDATE PreUpdate;
		FNPSCENECALLBACK_PREUPDATE	PostUpdate;
		void*						User;

		Vector<DebugVertex>			debugGeometry;
		size_t						debugTriCount	= 0;
		size_t						debugLineCount	= 0;

		PhysicsDebugOverlayMode		debugMode;
		iAllocator*					memory;
	};


	/************************************************************************************************/


	constexpr ComponentID PhysXComponentID = GetTypeGUID(PhysXComponentID);

	FLEXKITAPI class PhysXComponent : public FlexKit::Component<PhysXComponent, PhysXComponentID>
	{
	public:
		PhysXComponent(ThreadManager& threads, iAllocator* allocator);
		~PhysXComponent();

		void							Release();
		void							ReleaseScene				(LayerHandle);

		UpdateTask&						Update(UpdateDispatcher&, const double dt);
		void							Simulate( const double dt, WorkBarrier* barrier = nullptr, iAllocator* temp_allocator = nullptr);

		[[nodiscard]] LayerHandle		CreateLayer(bool DebugVis = false);
		[[nodiscard]] StaticBodyHandle	CreateStaticCollider	(GameObject* go, const LayerHandle, const float3 pos = { 0, 0, 0 }, const Quaternion q = { 0, 0, 0, 1 });
		[[nodiscard]] RigidBodyHandle	CreateRigidBodyCollider	(GameObject* go, const LayerHandle, const float3 pos = { 0, 0, 0 }, const Quaternion q = { 0, 0, 0, 1 });

		[[nodiscard]] Shape				CreateCubeShape			(const float3 dimensions);
		[[nodiscard]] Shape				CreateSphereShape		(const float radius);
		[[nodiscard]] Shape				CreateCapsuleShape		(const float radius, const float halfRadius);


		PhysicsLayer&					GetLayer_ref(LayerHandle handle);
		physx::PxMaterial*				GetDefaultMaterial() const { return defaultMaterial;}

		physx::PxCooking*				GetCooker	();
		Shape							CookMesh	(float3* geometry, size_t geometrySize, uint32_t* indices, size_t indexCount);
		Blob							CookMesh2	(float3* geometry, size_t geometrySize, uint32_t* indices, size_t indexCount);

		Shape							LoadTriMeshShape(const Blob&);
		physx::PxPhysics*				GetAPI() { return physxAPI; }

		template<typename TY>
		PhysicsLayer&	GetOwningLayer(TY handle)
		{
			return scenes[GetByType<LayerHandle>(handle)];
		}

		operator PhysXComponent* () { return this; }

	private:
		physx::PxFoundation*			foundation;
		physx::PxPhysics*				physxAPI;

		double							acc = 0.0;

		physx::PxCooking*				cooker				= nullptr;
		physx::PxCudaContextManager*	cudaContextmanager	= nullptr;

		bool							updateColliders;
		physx::PxPvd*					visualDebugger;
		physx::PxPvdTransport*			visualDebuggerConnection;
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
					ProfileFunction();

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
				threads.AddWork(newTask);
			}


			uint32_t getWorkerCount() const
			{
				return (uint32_t)threads.GetThreadCount();
			}



			operator ThreadManager& () { return threads; }

		private:
			ThreadManager&		threads;
			iAllocator*			allocator;
		} dispatcher;


		Vector<Shape>				shapes;

		Vector<PhysicsLayer>		scenes;
		iAllocator*					allocator;
		ThreadManager&				threads;

		friend PhysicsLayer;
	};


	/************************************************************************************************/


	constexpr ComponentID StaticBodyComponentID  = GetTypeGUID(StaticBodyComponentID);

	enum class StaticBodyType
	{
		TriangleMesh,
		Cube,
		Capsule,
		Sphere,
		BoundingVolume
	};

	struct CubeCollider
	{
		float dimensions[3];
	};

	struct SphereCollider
	{
		float radius;
	};

	struct CapsuleCollider
	{
		float radius;
		float height;
	};

	struct BoudingVolumeCollider
	{
		StaticBodyType subType; // Must be a Cube, Capsule, or Sphere
	};

	struct StaticBodyShape
	{
		StaticBodyShape() = default;

		StaticBodyType  type;

		union
		{
			uint64_t				triMeshResource;
			CubeCollider			cube;
			SphereCollider			sphere;
			CapsuleCollider			capsule;
			BoudingVolumeCollider	bv;
		};

		float3          position;
		Quaternion      orientation;

		void Serialize(auto& ar)
		{
			ar& position;
			ar& orientation;
			ar& cube;
			ar& type;
		}
	};

	struct StaticBodyHeaderBlob
	{
		FlexKit::ComponentBlock::Header  componentHeader = {
			0,
			FlexKit::EntityComponentBlock,
			sizeof(FlexKit::ComponentBlock::Header),
			FlexKit::StaticBodyComponentID
		};

		uint32_t shapeCount;
	};


	class StaticBodyComponent : public Component<StaticBodyComponent, StaticBodyComponentID>
	{
	public:
		StaticBodyComponent(PhysXComponent& IN_physx) :
			physx{ IN_physx } {}

		StaticBodyHandle Create(GameObject* gameObject, LayerHandle layer, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });

		void AddComponentView(GameObject& GO, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;
		void Remove(LayerHandle layer, StaticBodyHandle sb) noexcept;

		auto& GetLayer(LayerHandle layer)
		{
			return physx.GetLayer_ref(layer);
		}

		using OnConstructionHandler = TypeErasedCallable<void(GameObject&, const char* userData_ptr, const size_t userData_size, ValueMap)>;


		static void SetOnConstructionHandler(const auto& callback)	{ onConstruction = callback; }
		static void ClearCallback()									{ onConstruction.Release(); }

	private:
		PhysXComponent&                         physx;
		inline static OnConstructionHandler     onConstruction;
	};


	/************************************************************************************************/


	class StaticBodyView : public ComponentView_t<StaticBodyComponent>
	{
	public:
		StaticBodyView(GameObject& gameObject, StaticBodyHandle IN_staticBody, LayerHandle IN_layer);
		StaticBodyView(GameObject& gameObject, LayerHandle IN_layer, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });

		~StaticBodyView();

		NodeHandle  GetNode() const;
		GameObject& GetGameObject() const;

		void AddShape(Shape shape);
		void RemoveShape(uint32_t);
		void RemoveAll();

		physx::PxShape*	GetShape(size_t);
		size_t			GetShapeCount() const noexcept;

		StaticColliderSystem::StaticColliderObject* operator -> ();

		void    SetUserData(void*) noexcept;
		void*   GetUserData() noexcept;

		const LayerHandle       layer;
		const StaticBodyHandle  staticBody;
	};


	/************************************************************************************************/


	NodeHandle	GetStaticBodyNode				(GameObject& GO);
	void		StaticBodySetWorldPosition		(GameObject& GO, const float3 xyz);
	void		StaticBodySetScale				(GameObject& GO, const float3 xyz);
	void		StaticBodySetWorldOrientation	(GameObject& GO, const Quaternion xyz);


	/************************************************************************************************/


	constexpr ComponentID RigidBodyComponentID = GetTypeGUID(RigicBodyComponentID);

	class RigidBodyView;

	class RigidBodyComponent : public Component<RigidBodyComponent, RigidBodyComponentID >
	{
	public:
		RigidBodyComponent(PhysXComponent& IN_physx);

		RigidBodyHandle CreateRigidBody(GameObject* gameObject, LayerHandle layer, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 });
		void            AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override;

		void            Remove(RigidBodyView& rigidBody);
		PhysicsLayer&   GetLayer(LayerHandle layer);

	private:
		PhysXComponent& physx;
	};


	/************************************************************************************************/


	class RigidBodyView : public ComponentView_t<RigidBodyComponent>
	{
	public:
		RigidBodyView(GameObject& gameObject, RigidBodyHandle IN_rigidBody, LayerHandle IN_layer) :
			staticBody	{ IN_rigidBody },
			layer		{ IN_layer      } {}

		RigidBodyView(GameObject& gameObject, LayerHandle IN_layer, float3 pos = { 0, 0, 0 }, Quaternion q = { 0, 0, 0, 1 }) :
			staticBody	{ GetComponent().CreateRigidBody(&gameObject, IN_layer, pos, q) },
			layer		{ IN_layer } {}

		NodeHandle		GetNode() const;
		void			AddShape(Shape shape);

		const LayerHandle		layer;
		const RigidBodyHandle	staticBody;
	};


	NodeHandle	GetRigidBodyNode(GameObject& GO);
	void		SetRigidBodyPosition(GameObject& GO, const float3 worldPOS);
	void		ApplyForce(GameObject& GO, const float3 force);



	/************************************************************************************************/


	constexpr ComponentID CharacterControllerComponentID = GetTypeGUID(CharacterControllerComponentID);
	using CharacterControllerHandle = Handle_t<32, CharacterControllerComponentID>;

	struct CharacterController
	{
		CharacterControllerHandle	handle;
		NodeHandle					node;
		GameObject*					gameObject;
		LayerHandle					layer;

		physx::PxController*		controller;

		float						focusHeight		= 4.0f;
		float						cameraDistance	= 10.0f;
		float2						mouseMoved		= { 0, 0 };
		double						updateTimer		= 0;
	};

	
	class CharacterControllerComponent : public Component<CharacterControllerComponent, CharacterControllerComponentID>
	{
	public:
		CharacterControllerComponent(PhysXComponent& IN_physx, iAllocator* allocator) :
			physx       { IN_physx  },
			handles     { allocator },
			controllers { allocator } {}


		CharacterControllerHandle Create(const LayerHandle layer, GameObject& gameObject, const NodeHandle node = GetZeroedNode(), const float3 initialPosition = {}, const float R = 1, const float H = 1)
		{
			auto& manager = physx.GetLayer_ref(layer).GetCharacterController();

			if (!gameObject.hasView(TransformComponentID))
				gameObject.AddView<SceneNodeView<>>(node);

			SetPositionW(node, initialPosition + float3{0, H / 2, 0});

			physx::PxCapsuleControllerDesc CCDesc;
			CCDesc.material			= physx.GetDefaultMaterial();
			CCDesc.radius			= R;
			CCDesc.height			= H;
			CCDesc.contactOffset	= 0.1f;
			CCDesc.position			= { initialPosition.x, initialPosition.y, initialPosition.z };
			CCDesc.climbingMode		= physx::PxCapsuleClimbingMode::eEASY;
			CCDesc.stepOffset		= 0.2f;

			auto controller = manager.createController(CCDesc);

			auto newHandle	= handles.GetNewHandle();
			auto idx		= controllers.push_back(
				CharacterController{
					newHandle,
					node,
					&gameObject,
					layer,
					controller
				});

			controller->setUserData(&gameObject);

			handles[newHandle] = (index_t)idx;

			return newHandle;
		}


		void AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
		{
			FK_ASSERT(0);
		}


		void Remove(CharacterControllerHandle handle)
		{
			const auto idx				= handles[handle];
			auto& characterController	= controllers[idx];

			characterController.controller->release();
			characterController = controllers.back();

			controllers.pop_back();

			handles[characterController.handle] = idx;
		}


		auto& GetLayer(LayerHandle layer)
		{
			return physx.GetLayer_ref(layer);
		}


		CharacterController& operator[] (CharacterControllerHandle controller)
		{
			return controllers[controller];
		}

	private:

		HandleUtilities::HandleTable<CharacterControllerHandle>	handles;
		Vector<CharacterController>								controllers;
		PhysXComponent&											physx;
	};


	/************************************************************************************************/


	class CharacterControllerView : public ComponentView_t<CharacterControllerComponent>
	{
	public:
		CharacterControllerView(
			GameObject&		gameObject,
			LayerHandle		layer,
			const float3	initialPosition	= { 0, 0, 0 },
			NodeHandle		node			= GetZeroedNode(),
			const float		R				= 1.0f,
			const float		H				= 1.0f) :
				controller{ GetComponent().Create(layer, gameObject, node, initialPosition) }
		{
			auto& controller_ref = GetComponent()[controller];

			controller_ref.controller->getActor()->userData = &gameObject;
			controller_ref.controller->setUserData(&gameObject);
		}


		CharacterControllerView(
			GameObject&                 gameObject,
			CharacterControllerHandle   IN_controller) :
				controller  { IN_controller } {}


		~CharacterControllerView()
		{
			GetComponent().Remove(controller);
		}


		NodeHandle GetNode() const
		{
			return GetComponent()[controller].node;
		}


		float3 GetPosition()
		{
			auto& ref = GetComponent()[controller];
			
			return pxVec3ToFloat3(ref.controller->getPosition());
		}


		void SetPosition(const float3 xyz)
		{
			auto& ref = GetComponent()[controller];
			ref.controller->setFootPosition(Float3TopxVec3Ext(xyz));
			SetPositionW(ref.node, xyz);
		}


		void SetOrientation(const Quaternion q)
		{
			auto& ref = GetComponent()[controller];
			FlexKit::SetOrientation(ref.node, q);
		}


		uint32_t Move(const float3 v, double dt, physx::PxControllerFilters& filters)
		{
			auto& ref = GetComponent()[controller];

			return ref.controller->move({ v.x, v.y, v.z }, 0.01f, (float)dt, filters);
		}


		CharacterControllerHandle controller;
	};


	NodeHandle					GetControllerNode			(GameObject&);
	CharacterControllerHandle	GetControllerHandle			(GameObject&);
	float3						GetControllerPosition		(GameObject&);
	void						SetControllerPosition		(GameObject&, const float3);
	void						SetControllerOrientation	(GameObject&, const Quaternion);
	void						MoveController				(GameObject&, const float3&, physx::PxControllerFilters& filters, double dt);


	/************************************************************************************************/


	constexpr ComponentID CameraControllerComponentID   = GetTypeGUID(ThirdPersonCameraComponentID);
	using CameraControllerHandle                        = Handle_t<32, CameraControllerComponentID>;


	struct ThirdPersonCameraFrameState
	{
		float3 Position;
		float  yaw;
		float  pitch;
		float  roll;
	};


	struct ThirdPersonCamera
	{
		ThirdPersonCamera(
			CharacterControllerHandle	IN_controller		= InvalidHandle,
			NodeHandle					IN_node				= GetZeroedNode(),
			CameraHandle				IN_camera			= CameraComponent::GetComponent().CreateCamera(),
			float3						initialPos			= { 0, 0, 0 },
			const float					initialMovementSpeed= 50);

		static physx::PxFilterFlags Test(
			physx::PxFilterObjectAttributes	attributes0,
			physx::PxFilterData				filterData0,
			physx::PxFilterObjectAttributes	attributes1,
			physx::PxFilterData				filterData1,
			physx::PxPairFlags&				pairFlags,
			const byte*						constantBlock,
			physx::PxU32					constantBlockSize);


		ThirdPersonCamera(const ThirdPersonCamera&) = default;

		struct KeyStates
		{
			float x;
			float y;
		};

		Quaternion	GetOrientation()	const;
		float3		GetForwardVector()	const;
		float3		GetRightVector()	const;

		void SetRotation(const float3 xyz);

		void Rotate		(const float3 xyz);
		void Yaw		(const float theta);
		void Pitch		(const float theta);
		void Roll		(const float theta);
		void SetPosition(const float3 xyz);

		float3 GetHeadPosition() const;


		void UpdateCharacter(const float2 mouseInput, const double dt);
		void UpdateCharacter(const float2 mouseInput, const KeyStates& keyState, const double dt);

		void UpdateCamera(const float2 mouseInput, const KeyStates& keyState, const double dt);


		ThirdPersonCameraFrameState GetFrameState() const
		{
			ThirdPersonCameraFrameState out;

			out.pitch	= pitch;
			out.yaw		= yaw;
			out.roll	= roll;

			return out;
		}

		CharacterControllerHandle	controller	= InvalidHandle;
		CameraHandle				camera		= InvalidHandle;

		NodeHandle objectNode	= InvalidHandle;
		NodeHandle cameraNode	= InvalidHandle;

		NodeHandle yawNode		= InvalidHandle;
		NodeHandle pitchNode	= InvalidHandle;
		NodeHandle rollNode		= InvalidHandle;


		float3			cameraPosition  = float3(0, 0, 0);

		float			pitch	= 0; // parented to yaw
		float			roll	= 0; // Parented to pitch
		float			yaw		= 0; 

		float3			velocity		= 0;
		float			acceleration	= 100;
		float			drag			= 5.0f;
		float			moveRate		= 50;
		float			gravity			= 9.8f;

		bool			floorContact = false;

		KeyStates keyStates;
	};


	/************************************************************************************************/

	struct ThirdPersonEventHandler
	{
		void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
		{
		}
	};

	using CameraControllerComponent		= BasicComponent_t<ThirdPersonCamera, CameraControllerHandle, CameraControllerComponentID, ThirdPersonEventHandler>;
	using CameraControllerView			= CameraControllerComponent::View;


	CameraHandle	GetCameraControllerCamera(GameObject& GO);
	GameObject&		CreateThirdPersonCameraController(GameObject& gameObject, LayerHandle layer, iAllocator& allocator, const float R = 1, const float H = 1);

	float3			GetCameraControllerHeadPosition(GameObject& GO);
	float3			GetCameraControllerForwardVector(GameObject& GO);
	Quaternion		GetCameraControllerOrientation(GameObject& GO);
	NodeHandle		GetCameraControllerNode(GameObject& GO);

	float3			GetCameraControllerModelPosition(GameObject& GO);
	Quaternion		GetCameraControllerModelOrientation(GameObject& GO);

	void			YawCameraController(GameObject& GO, float rad);

	void			SetCameraControllerCameraHeightOffset(GameObject& GO, const float offset);
	void			SetCameraControllerCameraBackOffset(GameObject& GO, const float offset);


	void			UpdateThirdPersonCameraControllers(const float2& mouseInput, const double dT);
	UpdateTask&		QueueThirdPersonCameraControllers(UpdateDispatcher& dispatcher, float2 mouseInput, const double dT);


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


	bool HandleEvents(GameObject& GO, Event evt);


	/************************************************************************************************/

	//FLEXKITAPI ColliderHandle			LoadTriMeshCollider		(PhysicsSystem* PS, GUID_t Guid);
	FLEXKITAPI physx::PxHeightField*	LoadHeightFieldCollider	(PhysXComponent& PX, GUID_t Guid);


}	/************************************************************************************************/


#endif//PHYSICSUTILITIES_H
