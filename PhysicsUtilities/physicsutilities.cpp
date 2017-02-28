/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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
		Physics->ProfileZoneManager = &physx::PxProfileZoneManager::createProfileZoneManager(Physics->Foundation);
		if (!Physics->ProfileZoneManager)
			FK_ASSERT(0); // Failed to init
		Physics->Physx = PxCreatePhysics(PX_PHYSICS_VERSION, *Physics->Foundation, physx::PxTolerancesScale(), recordMemoryAllocations, Physics->ProfileZoneManager);
		if (!Physics->Physx)
			FK_ASSERT(0); // Failed to init
		if (!PxInitExtensions(*Physics->Physx))
			FK_ASSERT(0);


#ifdef PHYSXREMOTEDB
		Physics->RemoteDebuggerEnabled = true;
#else
		Physics->RemoteDebuggerEnabled = false;
#endif


		if (Physics->RemoteDebuggerEnabled)
		{
			if (Physics->Physx->getPvdConnectionManager())
			{
				const char*     pvd_host_ip = "127.0.0.1";
				int             port        = 5425;
				unsigned int    timeout     = 100;

				physx::PxVisualDebuggerConnectionFlags connectionFlags = physx::PxVisualDebuggerExt::getAllConnectionFlags();
				Physics->VisualDebuggerConnection = physx::PxVisualDebuggerExt::createConnection(Physics->Physx->getPvdConnectionManager(),
																								 pvd_host_ip, port, timeout, connectionFlags);

				if (!Physics->VisualDebuggerConnection) {
					Physics->RemoteDebuggerEnabled = false; // No remote Debugger Available
					std::cout << "PVD failed to Connect!\n";
				}

				Physics->Physx->getVisualDebugger()->setVisualizeConstraints(true);
				Physics->Physx->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
				Physics->Physx->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
			} else
			{
				std::cout << "PVD Disabled!\n";
				Physics->RemoteDebuggerEnabled = false;
			}
		}
		else
		{
			std::cout << "PVD Disabled!\n";
		}

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


	void UpdateColliders(PScene* scn, FlexKit::SceneNodes* nodes)
	{
		if (scn->UpdateColliders)
		{
			if (!scn->Scene->checkResults())
				return;

			scn->UpdateColliders = false;
			scn->Scene->fetchResults();

			for (auto c : scn->Colliders)
			{
				if (c.Actor->isRigidBody())
				{
					physx::PxRigidDynamic* o = static_cast<physx::PxRigidDynamic*>(c.Actor);
					auto pose = o->getGlobalPose();

					FlexKit::LT_Entry LT(FlexKit::GetLocal(nodes, c.Node));
					LT.T.m128_f32[0] = pose.p.x;
					LT.T.m128_f32[1] = pose.p.y;
					LT.T.m128_f32[2] = pose.p.z;

					LT.R.m128_f32[0] = pose.q.x;
					LT.R.m128_f32[1] = pose.q.y;
					LT.R.m128_f32[2] = pose.q.z;
					LT.R.m128_f32[3] = pose.q.w;

					FlexKit::SetLocal(nodes, c.Node, &LT);
				}
			}
		}
	}


	/************************************************************************************************/


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

		scene->Colliders.push_back({Cube, NodeHandle() });
		scene->Scene->addActor(*Cube);

		return scene->Colliders.size() - 1;
	}


	/************************************************************************************************/


	size_t CreatePlaneCollider(physx::PxMaterial* material, PScene* scene, SceneNodes* SN, NodeHandle Nodes)
	{
		auto Static = physx::PxCreateStatic(scene->Scene->getPhysics(), { 0, -0.5f, 0 }, PxBoxGeometry(50000.0f, 1.0f, 50000.0f), *material);

		scene->Scene->addActor(*Static);
		scene->Colliders.push_back({ Static, Nodes });

		return scene->Colliders.size() - 1;
	}

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

	void InitiateScene(PhysicsSystem* System, PScene* Scn, iAllocator* allocator)
	{
		physx::PxSceneDesc desc(System->Physx->getTolerancesScale());
		desc.gravity		= physx::PxVec3(0.0f, -9.81f, 0.0f);
		desc.filterShader	= physx::PxDefaultSimulationFilterShader;
		desc.cpuDispatcher	= System->CPUDispatcher;

		Scn->Colliders.Allocator = allocator;

		//if (!desc.gpuDispatcher && game->Physics->GpuContextManger)
		//{
		//	desc.gpuDispatcher = game->Physics->GpuContextManger->getGpuDispatcher();
		//}

		Scn->Scene = System->Physx->createScene(desc);

		if (!Scn->Scene)
			FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

		Scn->UpdateColliders = false;

		physx::PxClientID CID = Scn->Scene->createClient();
		Scn->CID = CID;

		Scn->ControllerManager = PxCreateControllerManager(*Scn->Scene);
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


	size_t CreatePlaneCollider(physx::PxMaterial* material, PScene* scene)
	{
		auto Static = physx::PxCreateStatic(scene->Scene->getPhysics(), { 0, -0.5f, 0 }, PxBoxGeometry(50000.0f, 1.0f, 50000.0f), *material);

		scene->Scene->addActor(*Static);
		scene->Colliders.push_back({ Static });

		return scene->Colliders.size() - 1;
	}


	/************************************************************************************************/

#pragma warning(disable : 4309)

	ColliderHandle LoadTriMeshCollider(FlexKit::PhysicsSystem* PS, Resources* RM, GUID_t Guid)
	{
		if (isResourceAvailable(RM, Guid))
		{
			auto RHandle = LoadGameResource(RM, Guid);
			auto Resource = (ColliderResourceBlob*)GetResource(RM, RHandle);
			size_t ResourceSize = Resource->ResourceSize - sizeof(ColliderResourceBlob);

			PxDefaultMemoryInputData readBuffer(Resource->Buffer, ResourceSize);
			auto Mesh = PS->Physx->createTriangleMesh(readBuffer);

			FreeResource(RM, RHandle);

			return AddCollider(&PS->Colliders, {Mesh, 1});
		}

		return static_cast<ColliderHandle>(INVALIDHANDLE);
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


	size_t CreateStaticActor(PhysicsSystem* PS, FlexKit::PScene* Scene, ColliderHandle CHandle, float3 POS, Quaternion Q)
	{
		PxTriangleMeshGeometry triGeom;
		PxRigidStatic* Static	= PS->Physx->createRigidStatic(PxTransform(POS.x, POS.y, POS.z, PxQuat(Q.x, Q.y, Q.z, Q.w)));
		triGeom.triangleMesh	= GetCollider(PS, CHandle);

		auto Shape = Static->createShape(triGeom, *PS->DefaultMaterial, PxTransform(0, 0, 0));

		Scene->Scene->addActor(*Static);
		Scene->Colliders.push_back({ Static });

		AddRef(PS, CHandle);

		return Scene->Colliders.size() - 1;
	}


	/************************************************************************************************/


	size_t CreateSphereActor(physx::PxMaterial* material, PScene* scene, float3 initialP, FlexKit::Quaternion initialQ, float3 InitialV)
	{
		physx::PxRigidDynamic* sphere = scene->Scene->getPhysics().createRigidDynamic(physx::PxTransform(initialP.x, initialP.y, initialP.z, { initialQ.x, initialQ.y, initialQ.z, initialQ.w }));
		physx::PxShape* Shape = sphere->createShape(physx::PxSphereGeometry(1), *material);

		PActor	MappingData;

		sphere->setLinearVelocity(physx::PxVec3(InitialV.x, InitialV.y, InitialV.z), true);
		sphere->setOwnerClient(scene->CID);
		scene->Scene->addActor(*sphere);
		scene->Colliders.push_back({ sphere, NodeHandle(-1), MappingData});
		sphere->userData = &scene->Colliders.back().ExtraData;

		return scene->Colliders.size() - 1;
	}


	/************************************************************************************************/


	void UpdateScene(PScene* Scn, double dt, FNPSCENECALLBACK_POSTUPDATE PreUpdate, FNPSCENECALLBACK_PREUPDATE PostUpdate, void* P)
	{
		if (!Scn->Scene)
			return;

		const double StepSize = 1 / 60.0f;
		Scn->T += dt;

		while (Scn->T > StepSize)
		{
			Scn->T -= StepSize;
			if (!Scn->Scene->checkResults() && !Scn->UpdateColliders)
			{
				if(PreUpdate)	PreUpdate(P);
				Scn->Scene->simulate(StepSize);
				Scn->UpdateColliders = true;
				if(PostUpdate)	PostUpdate(P);
			}
		}
	}


	/************************************************************************************************/


	void ReleasePhysics(PhysicsSystem* Physics)
	{
		Physics->CPUDispatcher->release();// Physx
		Physics->DefaultMaterial->release();
		if (Physics->RemoteDebuggerEnabled)	Physics->VisualDebuggerConnection->release();
		PxCloseExtensions();
		if (Physics->Physx)					Physics->Physx->release();
		if (Physics->ProfileZoneManager)	Physics->ProfileZoneManager->release();
		if (Physics->Foundation)			Physics->Foundation->release();
	}


	/************************************************************************************************/


	void ReleaseScene(PScene* Scn, PhysicsSystem* PS)
	{
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
	}


	/************************************************************************************************/


	void MakeCube(CubeDesc& cdesc, SceneNodes* Nodes, PScene* scene, physx::PxMaterial* Material, Drawable* E, NodeHandle node, float3 initialP, FlexKit::Quaternion initialQ)
	{
		auto index                                 = CreateCubeActor(Material, scene, cdesc.r, initialP, initialQ);
		scene->Colliders[index].Node               = node;
		scene->Colliders[index].Actor->userData	   = E;
		E->Node                                    = node;

		ZeroNode(Nodes, node);
	}


	/************************************************************************************************/


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


	void ReleaseCapsule(CapsuleCharacterController* Capsule)
	{
		Capsule->Controller->release();
	}


	/************************************************************************************************/
}// Namespace FlexKit