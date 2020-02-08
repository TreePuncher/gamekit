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


#include "..\buildsettings.h"
#include "..\coreutilities\Assets.h"
#include "physicsutilities.h"
#include <PhysX_sdk/physx/include/PxFoundation.h>
#include <PhysX_sdk/physx/include/PxPhysics.h>
#include <PhysX_sdk/physx/include/extensions/PxExtensionsAPI.h>

using namespace physx;


/************************************************************************************************/


namespace FlexKit
{	/************************************************************************************************/


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
			}

			visualDebugger				= pvd;
			visualDebuggerConnection	= transport;
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
		desc.gravity		= physx::PxVec3(0.0f, -9.81f, 0.0f);
		desc.filterShader	= physx::PxDefaultSimulationFilterShader;
		desc.cpuDispatcher	= &dispatcher;

		auto pScene			= physxAPI->createScene(desc);
		auto Idx			= scenes.emplace_back(pScene, *this, allocator);

        /*
        pScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        pScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);
        pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
        */

		return PhysXSceneHandle(Idx);
	}


	/************************************************************************************************/


    StaticBodyHandle PhysXComponent::CreateStaticCollider(PhysXSceneHandle sceneHndl, PxShapeHandle shape, float3 pos, Quaternion q)
	{
        auto& scene = GetScene_ref(sceneHndl);
		return scene.CreateStaticCollider(shapes[shape], pos, q);
	}


	/************************************************************************************************/


    RigidBodyHandle PhysXComponent::CreateRigidBodyCollider(PhysXSceneHandle sceneHndl, PxShapeHandle shape, float3 pos, Quaternion q)
	{
        auto& scene = GetScene_ref(sceneHndl);
		return scene.CreateRigidBodyCollider(shapes[shape], pos, q);
	}


    /************************************************************************************************/


    PxShapeHandle  PhysXComponent::CreateCubeShape(const float3 dimensions)
    {
        auto shape = physxAPI->createShape(
            physx::PxBoxGeometry(dimensions.x, dimensions.y, dimensions.z),
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


	void PhysXComponent::Simulate(double dt)
	{
		for (PhysXScene& scene : scenes)
			scene.Update(dt);
	}


	/************************************************************************************************/

/*
	physx::PxFilterFlags Test(
		physx::PxFilterObjectAttributes attributes0,
		physx::PxFilterData filterData0,
		physx::PxFilterObjectAttributes attributes1,
		physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags,
		const byte* constantBlock,
		physx::PxU32 constantBlockSize)
	{
		std::cout << "FILTERING";

		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}
*/


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


	/*
	size_t CreateStaticActor(PhysicsSystem* PS, FlexKit::PScene* Scene, ColliderHandle CHandle, float3 POS, Quaternion Q)
	{
		PxTriangleMeshGeometry triGeom;
		PxRigidStatic* Static	= PS->Physx->createRigidStatic(PxTransform(POS.x, POS.y, POS.z, PxQuat(Q.x, Q.y, Q.z, Q.w)));
		triGeom.triangleMesh	= GetCollider(PS, CHandle);

		auto Shape = Static->createShape(triGeom, *PS->DefaultMaterial, PxTransform(0, 0, 0));

		Scene->Scene->addActor(*Static);
		//Scene->Colliders.push_back({ Static });

		AddRef(PS, CHandle);

		return -1; //Scene->Colliders.size() - 1;
	}
	*/


	/************************************************************************************************/

	/*
	size_t CreateSphereActor(physx::PxMaterial* material, PScene* scene, float3 initialP, FlexKit::Quaternion initialQ, float3 InitialV)
	{
		physx::PxRigidDynamic* sphere = scene->Scene->getPhysics().createRigidDynamic(physx::PxTransform(initialP.x, initialP.y, initialP.z, { initialQ.x, initialQ.y, initialQ.z, initialQ.w }));
		physx::PxShape* Shape = sphere->createShape(physx::PxSphereGeometry(1), *material);

		PActor	MappingData;

		sphere->setLinearVelocity(physx::PxVec3(InitialV.x, InitialV.y, InitialV.z), true);
		sphere->setOwnerClient(scene->CID);
		scene->Scene->addActor(*sphere);
		//scene->Colliders.push_back({ sphere, NodeHandle(-1), MappingData});
		//sphere->userData = &scene->Colliders.back().ExtraData;

		return -1;//scene->Colliders.size() - 1;
	}
	(/

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


    void PhysXComponent::Update(double dt)
    {
        const auto updatePeriod = 1.0 / updateFrequency;

        while(acc > updatePeriod)
        {
            Simulate(dt);
            acc -= updatePeriod;
        }

        acc += dt;
    }


	/************************************************************************************************/

	/*
	void Initiate(CapsuleCharacterController* out, PScene* Scene, PhysicsSystem* PS, CapsuleCharacterController_DESC& Desc)
	{
		PxCapsuleControllerDesc CCDesc;
		CCDesc.material             = PS->DefaultMaterial;
		CCDesc.radius               = Desc.r;
		CCDesc.height               = Desc.h;
		CCDesc.contactOffset        = 0.01f;
		CCDesc.position             = { 0.0f, Desc.h / 2, 0.0f };
		CCDesc.climbingMode         = PxCapsuleClimbingMode::eEASY;
		//CCDesc.upDirection = PxVec3(0, 0, 1);

		auto NewCharacterController = Scene->ControllerManager->createController(CCDesc);
		out->Controller             = NewCharacterController;
		out->Controller->setFootPosition({ Desc.FootPos.x, Desc.FootPos.y, Desc.FootPos.z });
		out->FloorContact           = false;
		out->CeilingContact         = false;
	}
	*/

	/************************************************************************************************/

	/*
	void CharacterControllerSystem::Initiate(SceneNodeComponentSystem* nodes, iAllocator* Memory, physx::PxScene* Scene)
	{
		Nodes					= nodes;
		Controllers.Allocator	= Memory;
		ControllerManager		= PxCreateControllerManager(*Scene);
		Handles.Initiate(Memory);
	}


	/************************************************************************************************\/


	void CharacterControllerSystem::UpdateSystem(double dT)
	{
		for (auto& C : Controllers)
		{
			physx::PxControllerFilters Filter;
			if (C.Delta.magnitudesquared() > 0.001f)
			{
				if(C.Delta.magnitudesquared() < 10.0f)
				{
					auto Flags = C.Controller->move(
						{ C.Delta.x, C.Delta.y, C.Delta.z },
						0.01f,
						dT,
						Filter);

					C.CeilingContact	= Flags & physx::PxControllerCollisionFlag::eCOLLISION_UP;
					C.FloorContact		= Flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN;

					auto NewPosition	= C.Controller->getFootPosition();
					auto NewPositionFK	= { NewPosition.x, NewPosition.y, NewPosition.z };
					C.CurrentPosition	= NewPositionFK;

					Nodes->SetPositionL(C.Node, NewPositionFK);
				}
				else
				{
					auto Position		= GetPositionL(*Nodes, C.Node);
					C.CurrentPosition	= Position;
					C.Controller->setFootPosition({ Position.x, Position.y, Position.z });
				}

				C.Delta = 0;
			}
		}
	}


	/************************************************************************************************\/


	ComponentHandle CharacterControllerSystem::CreateCapsuleController(PhysicsSystem* PS, physx::PxScene* Scene, physx::PxMaterial* Mat,float R, float H, float3 InitialPosition, Quaternion Q)
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

		ComponentHandle NewHandle = Handles.GetNewHandle();
		Handles[NewHandle] = Controllers.size() - 1;
		return NewHandle;
	}


	/************************************************************************************************\/


	void ReleaseCapsule(CapsuleCharacterController* Capsule)
	{
		Capsule->Controller->release();
	}


	/************************************************************************************************\/
	*/


	void PhysXScene::Update(double dT)
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
					UpdateColliders();
					updateColliders = false;

					T -= stepSize;
				}
				else
					return;
			}
		} while ((T > stepSize));
	}


	/************************************************************************************************/


	void PhysXScene::UpdateColliders()
	{
		staticColliders.UpdateColliders();
		rbColliders.UpdateColliders();
	}


	/************************************************************************************************/


	StaticBodyHandle	PhysXScene::CreateStaticCollider(Shape shape, float3 initialPosition, Quaternion initialQ)
	{
		physx::PxTransform pxInitialPose =
			physx::PxTransform{ PxMat44(PxIdentity) };
		
		pxInitialPose.q = physx::PxQuat{ initialQ.x, initialQ.y, initialQ.z, initialQ.w};
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
}// Namespace FlexKit
