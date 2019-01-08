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


#include "..\buildsettings.h"
#include "..\coreutilities\Resources.h"
#include "physicsutilities.h"
#include <PhysX_sdk/physx/include/PxFoundation.h>
#include <PhysX_sdk/physx/include/PxPhysics.h>
#include <PhysX_sdk/physx/include/extensions/PxExtensionsAPI.h>

using namespace physx;

/************************************************************************************************/

namespace FlexKit
{
	/************************************************************************************************/


	FLEXKITINTERNAL physx::PxTriangleMesh*	GetCollider(PhysicsSystem* PS, ColliderHandle CHandle);


	/************************************************************************************************/


	void InitiatePhysics(PhysicsSystem* Physics, uint32_t CoreCount, iAllocator* allocator)
	{
#ifdef _DEBUG
		bool recordMemoryAllocations = true;
#else
		bool recordMemoryAllocations = false;
#endif
		Physics->Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		if (!Physics->Foundation)
			FK_ASSERT(0); // Failed to init


#if USING(PHYSX_PVD)
		if (Physics->RemoteDebuggerEnabled)
		{
			physx::PxPvd*				pvd			= physx::PxCreatePvd(*Physics->Foundation);
			physx::PxPvdTransport*		transport	= physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

			pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

			Physics->VisualDebugger				= pvd;
			Physics->VisualDebuggerConnection	= transport;
		}
		else
		{
		}
#endif

		Physics->Physx = PxCreatePhysics(PX_PHYSICS_VERSION, *Physics->Foundation, physx::PxTolerancesScale(), recordMemoryAllocations);

		if (!Physics->Physx)
			FK_ASSERT(0); // Failed to init
		if (!PxInitExtensions(*Physics->Physx, nullptr))
			FK_ASSERT(0);


#ifdef USING(PHYSX_PVD)
		Physics->RemoteDebuggerEnabled = true;
#else
		Physics->RemoteDebuggerEnabled = false;
#endif



		Physics->CPUDispatcher = physx::PxDefaultCpuDispatcherCreate(CoreCount);

		// Create Default Material
		auto DefaultMaterial = Physics->Physx->createMaterial(0.5f, 0.5f, .1f);
		if (!DefaultMaterial)
			assert(0);

		Physics->DefaultMaterial									= DefaultMaterial;

		Physics->Colliders.FreeSlots.Allocator						= allocator;
		Physics->Colliders.TriMeshColliders.Allocator				= allocator;
		Physics->Colliders.TriMeshColliderTable.FreeList.Allocator	= allocator;
		Physics->Colliders.TriMeshColliderTable.Indexes.Allocator	= allocator;
	}


	/************************************************************************************************/

	/*
	size_t CreateCubeActor(physx::PxMaterial* material, PScene* scene, float l, float3 initialP, FlexKit::Quaternion initialQ, float3 InitialV)
	{
		physx::PxVec3 pV;
		pV.x = initialP.x;
		pV.y = initialP.y;
		pV.z = initialP.z;

		physx::PxQuat pQ;
		pQ.x = initialQ.x;
		pQ.y = initialQ.y;
		pQ.z = initialQ.z;
		pQ.w = initialQ.w;

		physx::PxRigidDynamic* Cube = scene->Scene->getPhysics().createRigidDynamic(physx::PxTransform(pV, pQ));
		physx::PxShape* Shape = Cube->createShape(physx::PxBoxGeometry(l, l, l), *material);
		Cube->setLinearVelocity({ InitialV.x, InitialV.y, InitialV.z }, true);
		Cube->setOwnerClient(scene->CID);

		//scene->Colliders.push_back({Cube, NodeHandle() });
		//scene->Scene->addActor(*Cube);

		return -1;//scene->Colliders.size() - 1;
	}
	*/

	/************************************************************************************************/

	/*
	size_t CreatePlaneCollider(physx::PxMaterial* material, PScene* scene, SceneNodes* SN, NodeHandle Nodes)
	{
		auto Static = physx::PxCreateStatic(scene->Scene->getPhysics(), { 0, -0.5f, 0 }, PxBoxGeometry(50000.0f, 1.0f, 50000.0f), *material);

		//scene->Scene->addActor(*Static);
		//scene->Colliders.push_back({ Static, Nodes });

		return -1;//scene->Colliders.size() - 1;
	}
	*/

	/************************************************************************************************/


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

	/************************************************************************************************/


	ColliderHandle AddCollider(TriMeshColliderList* ColliderList, TriangleCollider Collider)
	{
		auto NewHandle = ColliderList->TriMeshColliderTable.GetNewHandle();

		if (!ColliderList->FreeSlots.size())
		{
			size_t Index = ColliderList->TriMeshColliders.size();
			ColliderList->TriMeshColliderTable[NewHandle]	= Index;
			ColliderList->TriMeshColliders.push_back		(Collider);
		}
		else
		{
			size_t Index = ColliderList->FreeSlots.back();
			ColliderList->FreeSlots.pop_back();
			ColliderList->TriMeshColliderTable[NewHandle]	= Index;
			ColliderList->TriMeshColliders[Index]			= Collider;
		}

		return NewHandle;
	}



	void ReleaseAllColliders(TriMeshColliderList* ColliderList)
	{
		for (auto C : ColliderList->TriMeshColliders){
			C.Mesh->release();
		}

		ColliderList->FreeSlots.Release();
		ColliderList->TriMeshColliders.Release();
		ColliderList->TriMeshColliderTable.Release();
	}


	/************************************************************************************************/


	/*
	size_t CreatePlaneCollider(physx::PxMaterial* material, PScene* scene)
	{
		auto Static = physx::PxCreateStatic(scene->Scene->getPhysics(), { 0, -0.5f, 0 }, PxBoxGeometry(50000.0f, 1.0f, 50000.0f), *material);

		scene->Scene->addActor(*Static);
		//scene->Colliders.push_back({ Static });

		return -1;//scene->Colliders.size() - 1;
	}
	*/

	/************************************************************************************************/

#pragma warning(disable : 4309)

	ColliderHandle LoadTriMeshCollider(FlexKit::PhysicsSystem* PS, GUID_t Guid)
	{
		if (isResourceAvailable(Guid))
		{
			auto RHandle = LoadGameResource(Guid);
			auto Resource = (ColliderResourceBlob*)GetResource(RHandle);
			size_t ResourceSize = Resource->ResourceSize - sizeof(ColliderResourceBlob);

			PxDefaultMemoryInputData readBuffer(Resource->Buffer, ResourceSize);
			auto Mesh = PS->Physx->createTriangleMesh(readBuffer);

			FreeResource(RHandle);

			return AddCollider(&PS->Colliders, {Mesh, 1});
		}

		return static_cast<ColliderHandle>(INVALIDHANDLE);
	}


	physx::PxHeightField*	LoadHeightFieldCollider(PhysicsSystem* PS, GUID_t Guid)
	{
		if (isResourceAvailable(Guid))
		{
			auto RHandle		= LoadGameResource(Guid);
			auto Resource		= (ColliderResourceBlob*)GetResource(RHandle);
			size_t ResourceSize = Resource->ResourceSize - sizeof(ColliderResourceBlob);

			PxDefaultMemoryInputData readBuffer(Resource->Buffer, ResourceSize);
			auto HeightField = PS->Physx->createHeightField(readBuffer);

			FreeResource(RHandle);
			return HeightField;
			//return AddCollider(&PS->Colliders, { Mesh, 1 });
		}
		return nullptr;
		//return static_cast<ColliderHandle>(INVALIDHANDLE);
	}


	/************************************************************************************************/


	physx::PxTriangleMesh*	GetCollider(PhysicsSystem* PS, ColliderHandle CHandle)
	{
		return PS->Colliders.TriMeshColliders[PS->Colliders.TriMeshColliderTable[CHandle]].Mesh;
	}


	/************************************************************************************************/

	
	void AddRef(PhysicsSystem* PS, ColliderHandle CHandle)
	{
		PS->Colliders.TriMeshColliders[PS->Colliders.TriMeshColliderTable[CHandle]].RefCount++;
	}
	
	
	/************************************************************************************************/


	void Release(PhysicsSystem* PS, ColliderHandle CHandle)
	{
		size_t RefCount = --PS->Colliders.TriMeshColliders[PS->Colliders.TriMeshColliderTable[CHandle]].RefCount;
		if (RefCount == 0)
		{
			size_t Index = PS->Colliders.TriMeshColliderTable[CHandle];
			PS->Colliders.FreeSlots.push_back( PS->Colliders.TriMeshColliderTable[CHandle]);
			PS->Colliders.TriMeshColliderTable.RemoveHandle(CHandle);
			PS->Colliders.TriMeshColliders[Index].Mesh->release();
		}
	}


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


	void ReleasePhysics(PhysicsSystem* Physics)
	{
		if (!Physics)
			return;

		Physics->CPUDispatcher->release();// Physx
		Physics->DefaultMaterial->release();
		PxCloseExtensions();

		if (Physics->Physx)					Physics->Physx->release();
		if (Physics->Foundation)			Physics->Foundation->release();

		Physics = nullptr;
	}


	/************************************************************************************************/


	//void ReleaseScene(PScene* Scn, PhysicsSystem* PS)
	//{
		/*
		for (auto c : Scn->Colliders) {
			auto SceneData = (PActor*)c.Actor->userData;
			if (SceneData)
			{
				switch (SceneData->type)
				{
				case PActor::EA_PRIMITIVE:
					// NOTHING TO BE DONE
					break;
				case PActor::EA_TRIMESH:
					Release(PS, SceneData->CHandle);
					break;
				}
			}
			if(c.Actor) c.Actor->release();
		}

		Scn->Scene->fetchResults(true);
		Scn->ControllerManager->purgeControllers();
		Scn->ControllerManager->release();
		Scn->Colliders.Release();
		Scn->Scene->release();
		*/
	//}


	/************************************************************************************************/

	/*
	void MakeCube(CubeDesc& cdesc, SceneNodes* Nodes, PScene* scene, physx::PxMaterial* Material, Drawable* E, NodeHandle node, float3 initialP, FlexKit::Quaternion initialQ)
	{
		auto index                                 = CreateCubeActor(Material, scene, cdesc.r, initialP, initialQ);
		//scene->Colliders[index].Node               = node;
		//scene->Colliders[index].Actor->userData	   = E;
		E->Node                                    = node;

		ZeroNode(Nodes, node);
	}
	*/

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


	void PhysicsComponentSystem::UpdateSystem(double dT)
	{
		if (!Scene)
			return;

		const double StepSize = 1 / 60.0f;
		T += dT;

		CharacterControllers.UpdateSystem(dT);

		while (T > StepSize)
		{
			T -= StepSize;
			if (!Scene->checkResults() && !UpdateColliders)
			{
				if (PreUpdate)	PreUpdate(User);
				Scene->simulate(StepSize);
				UpdateColliders	= true;
				if (PostUpdate)	PostUpdate(User);
			}
		}

		if (UpdateColliders) 
		{
			if (!Scene->checkResults())
				return;

			Scene->fetchResults();
			Base.UpdateSystem();
			UpdateColliders = false;
		}
	}


	/************************************************************************************************\/


	void PhysicsComponentSystem::UpdateSystem_PreDraw(double dT)
	{
	}


	void PhysicsComponentSystem::DebugDraw(FlexKit::FrameGraph* FGraph, iAllocator* TempMemory)
	{
		CharacterControllers.DebugDraw(FGraph, TempMemory);
		//CubeColliders.DrawDebug(FR);
		//StaticBoxColliders.DrawDebug(FR);
	}


	/************************************************************************************************\/


	void PhysicsComponentSystem::Release()
	{
		CubeColliders.Release();
		StaticBoxColliders.Release();
		Base.Release();

		Scene->fetchResults(true);
		ControllerManager->purgeControllers();
		ControllerManager->release();
		Scene->release();
	}


	/************************************************************************************************\/


	StaticBoxColliderArgs	PhysicsComponentSystem::CreateStaticBoxCollider(float3 XYZ, float3 Pos)
	{
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


	/************************************************************************************************\/


	CubeColliderArgs PhysicsComponentSystem::CreateCubeComponent(float3 InitialP, float3 InitialV, float l, Quaternion Q)
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


	/************************************************************************************************\/


	CapsuleCharacterArgs PhysicsComponentSystem::CreateCharacterController(float3 InitialP, float R, float H, Quaternion Q )
	{
		auto CapsuleCharacter = CharacterControllers.CreateCapsuleController(System, Scene, System->DefaultMaterial, R, H, InitialP, Q);

		CapsuleCharacterArgs Args;
		Args.CapsuleController = CapsuleCharacter;
		Args.System = &CharacterControllers;

		return Args;
	}


	/************************************************************************************************/
}// Namespace FlexKit