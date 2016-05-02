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
#include "physicsutilities.h"

using namespace physx;

/************************************************************************************************/

namespace FlexKit
{
	void InitiatePhysics(PhysicsSystem* Physics, uint32_t CoreCount)
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

				if (!Physics->VisualDebuggerConnection)
					Physics->RemoteDebuggerEnabled = false; // No remote Debugger Available

				Physics->Physx->getVisualDebugger()->setVisualizeConstraints(true);
				Physics->Physx->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
				Physics->Physx->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
			} else
			{
				Physics->RemoteDebuggerEnabled = false;
			}
		}

		Physics->CPUDispatcher = physx::PxDefaultCpuDispatcherCreate(CoreCount);

		// Create Default Material
		auto DefaultMaterial = Physics->Physx->createMaterial(0.5f, 0.5f, .1f);
		if (!DefaultMaterial)
			assert(0);

		Physics->DefaultMaterial = DefaultMaterial;
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
				if (c.collider->isRigidBody())
				{
					physx::PxRigidDynamic* o = static_cast<physx::PxRigidDynamic*>(c.collider);
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

		physx::PxRigidDynamic* cube = scene->Scene->getPhysics().createRigidDynamic(physx::PxTransform(pV, pQ));
		physx::PxShape* Shape = cube->createShape(physx::PxBoxGeometry(l, l, l), *material);
		cube->setLinearVelocity({ InitialV.x, InitialV.y, InitialV.z }, true);
		cube->setOwnerClient(scene->CID);

		scene->Colliders.push_back(cube);
		scene->Scene->addActor(*cube);

		return scene->Colliders.size() - 1;
	}

	/************************************************************************************************/

	size_t CreatePlaneCollider(physx::PxMaterial* material, PScene* scene)
	{
		//physx::PxRigidStatic* plane = physx::PxCreatePlane(scene->Scene->getPhysics(), physx::PxPlane(0, 1, 0, 0), *material);
		auto Static = physx::PxCreateStatic(scene->Scene->getPhysics(), { 0, -0.5f, 0 }, PxBoxGeometry(500.0f, 1.0f, 500.0f), *material);

		//Static->createShape(PxPlaneGeometry(), *material, {0, 0, 0});
		//plane->setOwnerClient(scene->CID);
		//scene->Scene->addActor(*plane);
		scene->Scene->addActor(*Static);
		//scene->Colliders.push_back(plane);

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

	void InitiateScene(PhysicsSystem* System, PScene* scn)
	{
		physx::PxSceneDesc desc(System->Physx->getTolerancesScale());
		desc.gravity		= physx::PxVec3(0.0f, -9.81f, 0.0f);
		desc.filterShader	= physx::PxDefaultSimulationFilterShader;
		//desc.filterShader	= Test;
		desc.cpuDispatcher	= System->CPUDispatcher;
		//desc.flags =physx::PxSceneFlag::;// |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;
		//customizeSceneDesc(desc);

		//if (!desc.gpuDispatcher && game->Physics->GpuContextManger)
		//{
		//	desc.gpuDispatcher = game->Physics->GpuContextManger->getGpuDispatcher();
		//}

		scn->Scene = System->Physx->createScene(desc);

		if (!scn->Scene)
			FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

		scn->UpdateColliders = false;

		physx::PxClientID CID = scn->Scene->createClient();
		scn->CID = CID;

		scn->ControllerManager = PxCreateControllerManager(*scn->Scene);
	}

	/************************************************************************************************/

	size_t CreateSphereActor(physx::PxMaterial* material, PScene* scene, float3 initialP, FlexKit::Quaternion initialQ, float3 InitialV)
	{
		physx::PxRigidDynamic* sphere = scene->Scene->getPhysics().createRigidDynamic(physx::PxTransform(initialP.x, initialP.y, initialP.z, { initialQ.x, initialQ.y, initialQ.z, initialQ.w }));
		physx::PxShape* Shape = sphere->createShape(physx::PxSphereGeometry(1), *material);

		sphere->setLinearVelocity(physx::PxVec3(InitialV.x, InitialV.y, InitialV.z), true);
		sphere->setOwnerClient(scene->CID);
		scene->Scene->addActor(*sphere);
		scene->Colliders.push_back(sphere);

		return scene->Colliders.size() - 1;
	}

	/************************************************************************************************/

	void UpdateScene(PScene* scn, double dt, FNPSCENECALLBACK_POSTUPDATE PreUpdate, FNPSCENECALLBACK_PREUPDATE PostUpdate, void* P)
	{
		if (!scn->Scene)
			return;

		const double StepSize = 1 / 60.0f;
		scn->T += dt;

		while (scn->T > StepSize)
		{
			scn->T -= StepSize;
			if (!scn->Scene->checkResults() && !scn->UpdateColliders)
			{
				if(PreUpdate)	PreUpdate(P);
				scn->Scene->simulate(StepSize);
				scn->UpdateColliders = true;
				if(PostUpdate)	PostUpdate(P);
			}
		}
	}

	/************************************************************************************************/

	void CleanupPhysics(PhysicsSystem* Physics)
	{
		Physics->CPUDispatcher->release();// Physx
		Physics->DefaultMaterial->release();
		if (Physics->RemoteDebuggerEnabled)	Physics->VisualDebuggerConnection->release();
		PxCloseExtensions();
		if (Physics->Physx)				Physics->Physx->release();
		if (Physics->ProfileZoneManager) Physics->ProfileZoneManager->release();
		if (Physics->Foundation)			Physics->Foundation->release();
	}

	/************************************************************************************************/

	void CleanUpScene(PScene* mat)
	{
		for (auto c : mat->Colliders)
			c.collider->release();
		mat->Scene->release();
	}

	/************************************************************************************************/

	void MakeCube(CubeDesc& cdesc, SceneNodes* Nodes, PScene* scene, physx::PxMaterial* Material, Drawable* E, NodeHandle node, float3 initialP, FlexKit::Quaternion initialQ)
	{
		auto index                                 = CreateCubeActor(Material, scene, cdesc.r, initialP, initialQ);
		scene->Colliders[index].Node               = node;
		scene->Colliders[index].collider->userData = E;
		E->Node                                    = node;

		FlexKit::ZeroNode(Nodes, node);
	}

	/************************************************************************************************/
}// Namespace FlexKit