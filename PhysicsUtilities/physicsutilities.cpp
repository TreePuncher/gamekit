#include "buildsettings.h"
#include "Assets.h"
#include "physicsutilities.h"
#include <PxFoundation.h>
#include <PxPhysics.h>
#include <extensions/PxExtensionsAPI.h>

using namespace physx;


namespace FlexKit
{	/************************************************************************************************/


    void RigidBodyColliderSystem::Release()
    {
        for (auto& collider : colliders)
            collider.actor->release();

        colliders.clear();
    }


    /************************************************************************************************/


    void RigidBodyColliderSystem::UpdateColliders(WorkBarrier* barrier, iAllocator* temp_allocator)
    {
        auto _updateColliders = [&](auto itr, const auto end)
        {
            ProfileFunction();

            for (; itr < end; itr++)
            {
                auto& collider = *itr;
                if (collider.actor->isSleeping())
                    continue;

                physx::PxTransform  pose = collider.actor->getGlobalPose();
                Quaternion	        orientation = Quaternion{ pose.q.x, pose.q.y, pose.q.z, pose.q.w };
                float3		        position = float3{ pose.p.x, pose.p.y, pose.p.z };

                SetPositionW(collider.node, position);
                SetOrientation(collider.node, orientation);
            }
        };


        if (barrier)
        {
            for (auto itr = colliders.begin(); itr < colliders.end(); itr += 512)
            {
                const auto remaining = (colliders.end() - itr);
                const auto enditr = remaining >= 512 ? itr + 512 : colliders.end();

                auto& workitem = CreateWorkItem(
                    [&, itr, enditr](iAllocator&)
                    {
                        _updateColliders(itr, enditr);
                    },  temp_allocator);

                barrier->AddWork(workitem);
                PushToLocalQueue(workitem);
            }
        }

        else
            _updateColliders(colliders.begin(), colliders.end());
    }


    /************************************************************************************************/


    RigidBodyColliderSystem::rbColliderObject& RigidBodyColliderSystem::operator[] (const RigidBodyHandle collider)
    {
        return colliders[collider];
    }


    /************************************************************************************************/


    RigidBodyColliderSystem::rbColliderObject RigidBodyColliderSystem::GetAPIObject(const RigidBodyHandle collider)
    {
        return colliders[collider];
    }


    /************************************************************************************************/


    PhysXComponent::PhysXComponent(ThreadManager& IN_threads, iAllocator* IN_allocator) :
		threads		{ IN_threads				},
		scenes		{ IN_allocator				},
        shapes      { IN_allocator              },
		allocator	{ IN_allocator				},
		dispatcher	{ IN_threads, IN_allocator	}
	{
#ifdef _DEBUG
		bool recordMemoryAllocations = false;
#else
		bool recordMemoryAllocations = false;
#endif

		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		if (!foundation)
			FK_ASSERT(0); // Failed to init

        PxCudaContextManagerDesc cudaDesc;

        cudaContextmanager = nullptr; // PxCreateCudaContextManager(*foundation, cudaDesc);

#if USING(PHYSX_PVD)
		if (remoteDebuggerEnabled || true)
		{
			physx::PxPvd*			pvd			= physx::PxCreatePvd(*foundation);
			physx::PxPvdTransport*	transport	= physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

			bool res = pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

			if (!res) {
				pvd->release();
				transport->release();
				FK_LOG_WARNING("FAILED TO CONNECT TO PHYSX REMOVE DEBUGGER");
                visualDebugger              = nullptr;
                visualDebuggerConnection    = nullptr;

			}
            else
            {
                visualDebugger              = pvd;
                visualDebuggerConnection    = transport;
            }
		}
		else
		{
            visualDebugger              = nullptr;
            visualDebuggerConnection    = nullptr;
		}
#endif
        physxAPI = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), recordMemoryAllocations, visualDebugger);

		// TODO: gracefull handling of errors maybe?
		if (!physxAPI)
			FK_ASSERT(0); 
		if (!PxInitExtensions(*physxAPI, nullptr))
			FK_ASSERT(0);

#ifdef USING(PHYSX_PVD)
		remoteDebuggerEnabled = visualDebugger != nullptr;
#else
		remoteDebuggerEnabled = false;
#endif


		// Create Default Material
		defaultMaterial = physxAPI->createMaterial(0.5f, 0.5f, .1f);
		updateColliders	= false;
	}


	/************************************************************************************************/


	PhysXSceneHandle PhysXComponent::CreateScene()
	{
		physx::PxSceneDesc desc(physxAPI->getTolerancesScale());
		desc.gravity		     = physx::PxVec3(0.0f, -9.81f, 0.0f);
		desc.filterShader	     = physx::PxDefaultSimulationFilterShader;
		desc.cpuDispatcher	     = &dispatcher;
        desc.cudaContextManager  = cudaContextmanager;

        desc.gpuDynamicsConfig.forceStreamCapacity      *= 4;
        desc.gpuDynamicsConfig.patchStreamSize          = 8096 * sizeof(PxContactPatch);

        if(cudaContextmanager != nullptr)
            desc.flags              |= PxSceneFlag::eENABLE_GPU_DYNAMICS;

		auto pScene			= physxAPI->createScene(desc);
		auto Idx			= scenes.emplace_back(pScene, *this, allocator);

		return PhysXSceneHandle(Idx);
	}


	/************************************************************************************************/


    StaticBodyHandle PhysXComponent::CreateStaticCollider(
        const PhysXSceneHandle  sceneHndl,
        const PxShapeHandle     shape,
        const float3            pos,
        const Quaternion        q)
	{
        auto& scene = GetScene_ref(sceneHndl);
		return scene.CreateStaticCollider(shapes[shape], pos, q);
	}


	/************************************************************************************************/


    RigidBodyHandle PhysXComponent::CreateRigidBodyCollider(
        const PhysXSceneHandle  sceneHndl,
        const PxShapeHandle     shape,
        const float3            pos,
        const Quaternion        q)
	{
        auto& scene = GetScene_ref(sceneHndl);
		return scene.CreateRigidBodyCollider(shapes[shape], pos, q);
	}


    /************************************************************************************************/


    PxShapeHandle   PhysXComponent::CreateCubeShape(const float3 dimensions)
    {
        auto shape = physxAPI->createShape(
            physx::PxBoxGeometry(dimensions.x, dimensions.y, dimensions.z),
            *defaultMaterial,
            false);

        return PxShapeHandle{ shapes.push_back({ shape }) };
    }


    /************************************************************************************************/


    PxShapeHandle   PhysXComponent::CreateSphereShape(const float radius)
    {
        auto shape = physxAPI->createShape(
            physx::PxSphereGeometry(radius),
            *defaultMaterial,
            false);

        return PxShapeHandle{ shapes.push_back({ shape }) };
    }



    /************************************************************************************************/


    PhysXScene& PhysXComponent::GetScene_ref(PhysXSceneHandle handle)
    {
        return scenes[handle];
    }


	/************************************************************************************************/


	void PhysXComponent::Simulate(double dt, WorkBarrier* barrier, iAllocator* temp_allocator)
	{
		for (PhysXScene& scene : scenes)
			scene.Update(dt, barrier, temp_allocator);
	}



	/************************************************************************************************/



	/*
	ColliderHandle LoadTriMeshCollider(FlexKit::PhysicsSystem* PS, GUID_t Guid)
	{
		if (isAssetAvailable(Guid))
		{
			auto RHandle		= LoadGameAsset(Guid);
			auto Resource		= (ColliderResourceBlob*)GetAsset(RHandle);
			size_t ResourceSize = Resource->ResourceSize - sizeof(ColliderResourceBlob);

			PxDefaultMemoryInputData readBuffer(Resource->Buffer, ResourceSize);
			auto Mesh = PS->Physx->createTriangleMesh(readBuffer);

			FreeAsset(RHandle);

			return AddCollider(&PS->Colliders, {Mesh, 1});
		}

		return static_cast<ColliderHandle>(INVALIDHANDLE);
	}
	*/

	/*
	physx::PxHeightField*	LoadHeightFieldCollider(PhysicsSystem* PS, GUID_t Guid)
	{
		if (isAssetAvailable(Guid))
		{
			auto RHandle		= LoadGameAsset(Guid);
			auto Resource		= (ColliderResourceBlob*)GetAsset(RHandle);
			size_t ResourceSize = Resource->ResourceSize - sizeof(ColliderResourceBlob);

			//PxDefaultMemoryInputData readBuffer(Resource->Buffer, ResourceSize);
			//auto HeightField	= PS->physxAPI->createHeightField(readBuffer);

			FreeAsset(RHandle);
			return nullptr;
			//return HeightField;
			//return AddCollider(&PS->Colliders, { Mesh, 1 });
		}
		return nullptr;
		//return static_cast<ColliderHandle>(INVALIDHANDLE);
	}
	*/


    /************************************************************************************************/


	PhysXComponent::~PhysXComponent()
	{
		Release();
	}


	/************************************************************************************************/


	void PhysXComponent::Release()
	{
		for (auto& scene : scenes)
			scene.Release();

		if(defaultMaterial) defaultMaterial->release();

		PxCloseExtensions();

		if (physxAPI)	physxAPI->release();
		if (foundation)	foundation->release();

		for (auto& scene : scenes)
			scene.Release();

		defaultMaterial = nullptr;
		physxAPI		= nullptr;
		foundation		= nullptr;
	}


	/************************************************************************************************/


	void PhysXComponent::ReleaseScene(PhysXSceneHandle handle)
	{
		scenes[handle.INDEX].Release();
	}


    /************************************************************************************************/


    UpdateTask& PhysXComponent::Update(UpdateDispatcher& dispatcher, double dt)
    {
        struct PhysXUpdate
        {

        };
        return dispatcher.Add<PhysXUpdate>(
            [&](UpdateDispatcher::UpdateBuilder& Builder, auto& Data)
            {

            },
            [this, dt = dt, &dispatcher]
            (auto& Data, iAllocator& threadAllocator)
            {
                const auto updatePeriod = 1.0 / updateFrequency;

                while (acc > updatePeriod)
                {
                    WorkBarrier barrier{ threads, &threadAllocator };
                    Simulate(dt, &barrier, &threadAllocator);
                    barrier.JoinLocal(); // Only works on work in local thread queue, to avoid increasing latency doing other tasks before continuing
                    acc -= updatePeriod;
                }

                acc += dt;
            });
    }


    /************************************************************************************************/


	void PhysXScene::Update(double dT, WorkBarrier* barrier, iAllocator* temp_allocator)
	{
		EXITSCOPE( T += dT; );

		do
		{
			if (T >= stepSize && !updateColliders)
			{
				scene->simulate(stepSize);
				updateColliders = true;
			}


			if (updateColliders)
			{
				if (scene->fetchResults())
				{
					UpdateColliders(barrier, temp_allocator);
					updateColliders = false;

                    if (barrier)
                        barrier->JoinLocal();


					T -= stepSize;
				}
				else
					return;
			}
		} while ((T > stepSize));
	}


	/************************************************************************************************/


	void PhysXScene::UpdateColliders(WorkBarrier* barrier, iAllocator* temp_allocator)
	{
		staticColliders.UpdateColliders(barrier, temp_allocator);
		rbColliders.UpdateColliders(barrier, temp_allocator);
	}


	/************************************************************************************************/


	StaticBodyHandle PhysXScene::CreateStaticCollider(Shape shape, float3 initialPosition, Quaternion initialQ)
	{
		physx::PxTransform pxInitialPose =
			physx::PxTransform{ PxMat44(PxIdentity) };
		
		pxInitialPose.q = physx::PxQuat{ initialQ.x, initialQ.y, initialQ.z, -initialQ.w};
		pxInitialPose.p = physx::PxVec3{ initialPosition.x, initialPosition.y, initialPosition.z };

		auto rigidStaticActor	= system.physxAPI->createRigidStatic(pxInitialPose);

        rigidStaticActor->attachShape(*shape._ptr);

		size_t handleIdx		= staticColliders.push_back({ GetZeroedNode(), rigidStaticActor }, true);

		scene->addActor(*rigidStaticActor);

		return StaticBodyHandle{ static_cast<unsigned int>(handleIdx) };
	}


	/************************************************************************************************/


    RigidBodyHandle PhysXScene::CreateRigidBodyCollider(Shape shape, float3 initialPosition, Quaternion initialQ)
	{
        auto node = GetZeroedNode();
		SetOrientation	(node, initialQ);
		SetPositionW	(node, initialPosition);

		physx::PxTransform pxInitialPose = physx::PxTransform{ PxMat44(PxIdentity) };

		pxInitialPose.q = physx::PxQuat{ initialQ.x, initialQ.y, initialQ.z, initialQ.w };
		pxInitialPose.p = physx::PxVec3{ initialPosition.x, initialPosition.y, initialPosition.z };

		PxRigidDynamic* rigidBodyActor = system.physxAPI->createRigidDynamic(pxInitialPose);


        rigidBodyActor->attachShape(*shape._ptr);

		size_t handleIdx = rbColliders.colliders.push_back({	node,
																rigidBodyActor });

		rigidBodyActor->setMass(1.0f);
		scene->addActor(*rigidBodyActor);

		return RigidBodyHandle{ static_cast<unsigned int>(handleIdx) };
	}


    /************************************************************************************************/


    void PhysXScene::ReleaseCollider(RigidBodyHandle handle)
    {
        scene->removeActor(*rbColliders.colliders[handle].actor);
    }


    /************************************************************************************************/


    void PhysXScene::ReleaseCollider(StaticBodyHandle handle)
    {
        scene->removeActor(*staticColliders.colliders[handle].actor);
    }


	/************************************************************************************************/


	void PhysXScene::SetPosition(RigidBodyHandle collider, float3 xyz)
	{
		auto& colliderIMPL	= rbColliders.colliders[collider.INDEX];
		auto pose			= colliderIMPL.actor->getGlobalPose();

		pose.p = {xyz.x, xyz.y, xyz.z};
		colliderIMPL.actor->setGlobalPose(pose);
	}


	/************************************************************************************************/


	void PhysXScene::SetMass(RigidBodyHandle collider, float m)
	{
		auto& colliderIMPL = rbColliders.colliders[collider.INDEX];
		colliderIMPL.actor->setMass(m);
	}


    /************************************************************************************************/


    void PhysXScene::ApplyForce(RigidBodyHandle rbHandle, float3 xyz)
    {
        auto actor = rbColliders[rbHandle].actor;
        actor->addForce({ xyz.x, xyz.y, xyz.z }, PxForceMode::eIMPULSE);
    }


    /************************************************************************************************/


    void PhysXScene::SetRigidBodyPosition(RigidBodyHandle rbHandle, const float3 xyz)
    {
        auto actor  = rbColliders[rbHandle].actor;
        auto pose   = actor->getGlobalPose();
        pose.p      = { xyz.x, xyz.y, xyz.z };

        rbColliders[rbHandle].actor->setGlobalPose(pose);
    }


    /************************************************************************************************/


    Quaternion	ThirdPersonCamera::GetOrientation() const
    {
        return FlexKit::GetOrientation(pitchNode);
    }


    /************************************************************************************************/


    float3 ThirdPersonCamera::GetForwardVector() const
    {
        Quaternion Q = GetOrientation();
        return -(Q * float3(0, 0, 1)).normal();
    }


    /************************************************************************************************/


    float3 ThirdPersonCamera::GetRightVector() const
    {
        Quaternion Q = GetOrientation();
        return -(Q * float3(-1, 0, 0)).normal();
    }


    /************************************************************************************************/


    CameraHandle GetCameraControllerCamera(GameObject& GO)
    {
        return Apply(GO, [](CameraControllerView& cameraController)
            {
                return cameraController.GetData().camera;
            },
            []
            {
                return (CameraHandle)InvalidHandle_t;
            });
    }


    bool HandleEvents(GameObject& GO, Event evt)
    {
        return Apply(GO,
            [&](CameraControllerView& cameraController)
            {
                auto& keyStates = cameraController.GetData().keyStates;

                if (evt.InputSource == FlexKit::Event::Keyboard)
		        {
			        bool state = evt.Action == Event::Pressed ? true : false;

			        switch (evt.mData1.mINT[0])
			        {
			        case TPC_MoveForward:
				        keyStates.forward	= state;
                        return true;
			        case TPC_MoveBackward:
				        keyStates.backward	= state;
                        return true;
			        case TPC_MoveLeft:
				        keyStates.left		= state;
                        return true;
			        case TPC_MoveRight:
				        keyStates.right		= state;
                        return true;
                    case TPC_MoveUp:
                        keyStates.up        = state;
                        return true;
                    case TPC_MoveDown:
                        keyStates.down      = state;
                        return true;
			        }
		        }
                return false;
            }, []() -> bool { return false; });
    }


    /************************************************************************************************/


    GameObject& CreateThirdPersonCameraController(GameObject& gameObject, PhysXSceneHandle scene, iAllocator& allocator, const float R, const float H)
    {
        gameObject.AddView<CharacterControllerView>(scene, &gameObject, float3{ 0, 10, 0 });

        gameObject.AddView<CameraControllerView>(
            CameraControllerComponent::GetComponent().Create(
                ThirdPersonCamera(
                    GetControllerHandle(gameObject),
                    GetControllerNode(gameObject))));

        return gameObject;
    }


    /************************************************************************************************/


    float3 GetCameraControllerHeadPosition(GameObject& GO)
    {
        return Apply(GO, [](CameraControllerView& cameraController)
            {
                return cameraController.GetData().GetHeadPosition();
            },
            []
            {
                return float3(0, 0, 0);
            });
    }



    /************************************************************************************************/


    float3 GetCameraControllerModelPosition(GameObject& GO)
    {
        return Apply(GO, [](CameraControllerView& cameraController)
            {
                return GetPositionW(cameraController.GetData().objectNode);
            },
            []
            {
                return float3(0, 0, 0);
            });
    }


    /************************************************************************************************/


    Quaternion GetCameraControllerModelOrientation(GameObject& GO)
    {
        return Apply(GO, [](CameraControllerView& cameraController)
            {
                return GetOrientation(cameraController.GetData().objectNode);
            },
            []
            {
                return Quaternion{};
            });
    }


    /************************************************************************************************/


    float3 GetCameraControllerForwardVector(GameObject& GO)
    {
        return Apply(GO, [](CameraControllerView& cameraController)
            {
                return cameraController.GetData().GetForwardVector();
            },
            []
            {
                return float3(0, 0, 0);
            });
    }


    /************************************************************************************************/


    Quaternion GetCameraControllerOrientation(GameObject& GO)
    {
        return Apply(GO,
            [](CameraControllerView& cameraController)
            {
                return GetOrientation(cameraController.GetData().cameraNode);
            },
            []
            {
                return Quaternion(0, 0, 0, 1);
            });
    }


    /************************************************************************************************/


    NodeHandle GetCameraControllerNode(GameObject& GO)
    {
        return Apply(GO,
            [](CameraControllerView& cameraController)
            {
                return cameraController.GetData().objectNode;
            },
            [] () -> NodeHandle
            {
                return InvalidHandle_t;
            });
    }


    void YawCameraController(GameObject& GO, float rad)
    {
        Apply(GO,
            [&](CameraControllerView& view)
            {
                view.GetData().Yaw(rad);
            });
    }

    void PitchCameraController(GameObject& GO, float rad)
    {
        Apply(GO,
            [&](CameraControllerView& view)
            {
                view.GetData().Pitch(rad);
            });
    }

    void RollCameraController(GameObject& GO, float rad)
    {
        Apply(GO,
            [&](CameraControllerView& view)
            {
                view.GetData().Roll(rad);
            });
    }

    /************************************************************************************************/


    void SetCameraControllerPosition(GameObject& GO, const float3 pos)
    {
        Apply(GO, [&](CameraControllerView& cameraController)
            {
                cameraController.GetData().SetPosition(pos);
            });
    }


    /************************************************************************************************/


    void SetCameraControllerCameraHeightOffset(GameObject& GO, const float offset)
    {
        Apply(GO, [&](CameraControllerView& cameraController)
            {
                auto& controller = cameraController.GetData();

                TranslateLocal(controller.pitchNode, float3{ 0, offset, 0 });
            });
    }


    /************************************************************************************************/


    void SetCameraControllerCameraBackOffset(GameObject& GO, const float offset)
    {
        Apply(GO, [&](CameraControllerView& cameraController)
            {
                auto& controller = cameraController.GetData();

                TranslateLocal(controller.pitchNode, float3{ 0, 0, offset });
            });
    }


    /************************************************************************************************/


    NodeHandle RigidBodyView::GetNode() const
    {
        return GetComponent().GetScene(scene)[staticBody].node;
    }


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


    void SetRigidBodyPosition(GameObject& GO, const float3 worldPOS)
    {
        Apply(GO, [&](RigidBodyView& rigidBody)
            {
                PhysXComponent::GetComponent().GetScene_ref(rigidBody.scene).SetRigidBodyPosition(rigidBody.staticBody, worldPOS);
            });
    }


    /************************************************************************************************/


    void ApplyForce(GameObject& GO, const float3 force)
    {
        Apply(GO, [&](RigidBodyView& rigidBody)
            {
                PhysXComponent::GetComponent().GetScene_ref(rigidBody.scene).ApplyForce(rigidBody.staticBody, force);
            });
    }


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


    ThirdPersonCamera::ThirdPersonCamera(
        const CharacterControllerHandle     IN_controller,
        const NodeHandle                    IN_node,
        const CameraHandle                  IN_camera,
        const float3                        initialPos,
        const float                         initialMovementSpeed) :
            controller  { IN_controller },
            camera      { IN_camera     },
            yawNode     { IN_node       },

            cameraNode  { GetZeroedNode() },
            objectNode  { GetZeroedNode() },

            pitchNode   { GetZeroedNode() },
            rollNode    { GetZeroedNode() },
            moveRate    { initialMovementSpeed }
    {
        SetParentNode(rollNode, cameraNode);
        SetParentNode(pitchNode, rollNode);
        SetParentNode(yawNode, pitchNode);

        auto& cameras = CameraComponent::GetComponent();
        ReleaseNode(cameras.GetCameraNode(camera));

        cameras.SetCameraNode(camera, cameraNode);
        cameras.MarkDirty(camera);

        TranslateWorld(yawNode, initialPos);
    }


    /************************************************************************************************/


    physx::PxFilterFlags ThirdPersonCamera::Test(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData             filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData             filterData1,
        physx::PxPairFlags&             pairFlags,
        const byte*                     constantBlock,
        physx::PxU32                    constantBlockSize)
    {
        std::cout << "FILTERING";

        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
        return physx::PxFilterFlag::eDEFAULT;
    }


    /************************************************************************************************/


    void ThirdPersonCamera::SetRotation(const float3 xyz)
    {

        if (xyz[0] != 0.0f)
            FlexKit::SetOrientationL(pitchNode, Quaternion{ RadToDegree(xyz[0]), 0, 0 });
        

        if (xyz[1] != 0.0f)
            FlexKit::SetOrientationL(yawNode, Quaternion{ 0, RadToDegree(xyz[1]), 0 });


        if (xyz[2] != 0.0f)
            FlexKit::SetOrientationL(rollNode, Quaternion{ 0, 0, RadToDegree(xyz[2]) });

        CameraComponent::GetComponent().MarkDirty(camera);
    }


    /************************************************************************************************/


    void ThirdPersonCamera::Rotate(float3 xyz)
    {
        if (xyz[0] != 0.0f)
            FlexKit::Pitch(pitchNode, xyz[0]);

        if (xyz[1] != 0.0f)
            FlexKit::Yaw(yawNode, xyz[1]);

        if (xyz[2] != 0.0f)
            FlexKit::Roll(rollNode, xyz[2]);

        CameraComponent::GetComponent().MarkDirty(camera);
    }


    /************************************************************************************************/


    void ThirdPersonCamera::Yaw(float Theta)
    {
        yaw += RadToDegree(Theta);
    }


    /************************************************************************************************/


    void ThirdPersonCamera::Pitch(float Theta)
    {
        pitch += RadToDegree(Theta);
    }


    /************************************************************************************************/


    void ThirdPersonCamera::Roll(float Theta)
    {
        roll += RadToDegree(Theta);
    }


    /************************************************************************************************/


    void ThirdPersonCamera::SetPosition(const float3 xyz)
    {
        auto& controllerImpl = CharacterControllerComponent::GetComponent()[controller];
        controllerImpl.controller->setPosition({ (float)xyz.x, (float)xyz.y, (float)xyz.z });
        FlexKit::SetPositionL(yawNode, xyz);
    }


    /************************************************************************************************/


    float3 ThirdPersonCamera::GetHeadPosition() const
    {
        auto& controllerImpl = CharacterControllerComponent::GetComponent()[controller];
        auto pxPos = controllerImpl.controller->getPosition();
        return { (float)pxPos.x, (float)pxPos.y , (float)pxPos.z };
    }


    /************************************************************************************************/


    void ThirdPersonCamera::Update(const float2 mouseInput, const double dt)
    {
        Update(mouseInput, keyStates, dt);
    }


    void ThirdPersonCamera::Update(const float2 mouseInput, const ThirdPersonCamera::KeyStates& keyStates, const double dt)
    {
        auto& controllerImpl = CharacterControllerComponent::GetComponent()[controller];

        controllerImpl.updateTimer += dt;
        controllerImpl.mouseMoved += mouseInput;

        const float focusHeight     = controllerImpl.focusHeight;
        const float cameraDistance  = controllerImpl.cameraDistance;

        const double deltaTime = 1.0 / 60.0f;
        while(controllerImpl.updateTimer >= deltaTime)
        {
            controllerImpl.updateTimer -= deltaTime;

            yaw     += controllerImpl.mouseMoved[0] * deltaTime * pi * 50;
            pitch   += controllerImpl.mouseMoved[1] * deltaTime * pi * 50;

            yaw     = fmod(yaw, DegreetoRad(360.0f));
            pitch   = clamp(DegreetoRad(-85.0f), pitch, DegreetoRad(75.0f));

            controllerImpl.mouseMoved = { 0.0f, 0.0f };

            SetRotation({ pitch, yaw, roll });

            float3 movementVector   { 0 };
            const float3 forward    { (GetForwardVector() * float3(1, 0, 1)).normal() };
            const float3 right      { GetRightVector() };
            const float3 up         { 0, 1, 0 };

            movementVector += keyStates.forward     ? forward   : float3::Zero();
            movementVector -= keyStates.backward    ? forward   : float3::Zero();
            movementVector += keyStates.right       ? right     : float3::Zero();
            movementVector -= keyStates.left        ? right     : float3::Zero();
            movementVector += keyStates.up          ? up        : float3::Zero();

            if (keyStates.down)
                movementVector -= up;

            if (keyStates.KeyPressed())
            {
                movementVector.normalize();
                velocity += movementVector * acceleration * (float)deltaTime;
            }
            
            if (velocity.magnitudeSq() < 0.1f || velocity.isNaN())
                velocity = 0.0f;

            PxControllerState state;
            controllerImpl.controller->getState(state);

            if(!floorContact)
                velocity += -up * gravity;

            velocity -= velocity * drag * (float)deltaTime;

            const auto  desiredMove    = velocity * (float)deltaTime;
            const auto& pxPrevPos      = controllerImpl.controller->getPosition();
            const auto  prevPos        = float3{ (float)pxPrevPos.x, (float)pxPrevPos.y, (float)pxPrevPos.z };

            physx::PxControllerFilters filters;
            auto collision = controllerImpl.controller->move(
                {   desiredMove.x,
                    desiredMove.y,
                    desiredMove.z },
                0.001f,
                dt,
                filters);

            floorContact = PxControllerCollisionFlag::eCOLLISION_DOWN & collision;

            const auto      pxPostPos   = controllerImpl.controller->getPosition();
            const float3    postPos     = pxVec3ToFloat3(pxPostPos);
            const auto      deltaPos    = prevPos - postPos;

            if (desiredMove.magnitudeSq() * 0.5f >= deltaPos.magnitude())
                velocity = 0;

            const float3 footPosition   = pxVec3ToFloat3(controllerImpl.controller->getFootPosition());

            auto& scene = PhysXComponent::GetComponent().GetScene_ref(controllerImpl.scene);

            const float3 origin1    = footPosition + up * 3 + 2.0f * -forward;
            const float3 ray1       = -forward.normal();

            float cameraZ = cameraDistance;

            scene.RayCast(origin1, ray1, 10,
                [&](auto hit)
                {
                    cameraZ = Min(hit.distance + 1.0f, cameraDistance);
                    return false;
                });

            const float3 origin2 = footPosition - forward * cameraZ + up * 10.0f;
            const float3 ray2    = (-up).normal();

            float cameraMinY     = 0;
            scene.RayCast(origin2, ray2, 100,
                [&](auto hit)
                {
                    cameraMinY = hit.distance - origin2.y + 1;
                    return false;
                });

            const auto cameraY      = Max(focusHeight - std::tanf(pitch) * cameraZ, cameraMinY);

            SetPositionW(objectNode, footPosition);

            const auto position             = footPosition - forward * cameraZ + up * cameraY;
            const auto newCameraPosition    = lerp(position, cameraPosition, 0.65f);
            cameraPosition                  = newCameraPosition;

            SetPositionW(yawNode, cameraPosition);

            CameraComponent::GetComponent().MarkDirty(camera);
        }
    }


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




	/************************************************************************************************/
}// Namespace FlexKit
