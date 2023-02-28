#include "buildsettings.h"
#include "Assets.h"
#include "KeyValueIds.h"
#include "physicsutilities.h"
#include "SceneLoadingContext.h"
#include "TriggerComponent.h"
#include "TriggerSlotIDs.hpp"

#include <any>
#include <PxFoundation.h>
#include <PxPhysics.h>
#include <extensions/PxExtensionsAPI.h>
#include <cooking/PxCooking.h>
#include <fmt/printf.h>

using namespace physx;

namespace FlexKit
{	/************************************************************************************************/


	RigidBodyColliderSystem::RigidBodyColliderSystem(RigidBodyColliderSystem&& IN_staticColliders) :
		layer { IN_staticColliders.layer }
	{
		colliders = std::move(IN_staticColliders.colliders);
	}


	/************************************************************************************************/


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

				physx::PxTransform	pose = collider.actor->getGlobalPose();
				Quaternion			orientation = Quaternion{ pose.q.x, pose.q.y, pose.q.z, pose.q.w };
				float3				position = float3{ pose.p.x, pose.p.y, pose.p.z };

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
		shapes		{ IN_allocator				},
		allocator	{ IN_allocator				},
		dispatcher	{ IN_threads, IN_allocator	}
	{
		scenes.reserve(10);

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


	LayerHandle PhysXComponent::CreateLayer(bool bebugVis)
	{
		physx::PxSceneDesc desc(physxAPI->getTolerancesScale());
		desc.gravity			= physx::PxVec3(0.0f, -9.81f, 0.0f);
		desc.filterShader		= physx::PxDefaultSimulationFilterShader;
		desc.cpuDispatcher		= &dispatcher;
		desc.cudaContextManager	= cudaContextmanager;

		desc.gpuDynamicsConfig.forceStreamCapacity		*= 4;
		desc.gpuDynamicsConfig.patchStreamSize			= 8096 * sizeof(PxContactPatch);

		if(cudaContextmanager != nullptr)
			desc.flags	|= PxSceneFlag::eENABLE_GPU_DYNAMICS;

		auto pScene			= physxAPI->createScene(desc);
		auto idx			= scenes.emplace_back(pScene, *this, allocator);

		if (bebugVis)
		{
			pScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
			pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_EDGES, 1.0f);
			pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
			pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);
			pScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_STATIC, 1.0f);
			pScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);

			scenes[idx].debugMode = PhysicsDebugOverlayMode::Wireframe;
		}

		return LayerHandle(idx);
	}


	/************************************************************************************************/


	StaticBodyHandle PhysXComponent::CreateStaticCollider(
		GameObject*             gameObject,
		const LayerHandle       layerHandle,
		const float3            pos,
		const Quaternion        q)
	{
		auto& layer = GetLayer_ref(layerHandle);
		return layer.CreateStaticCollider(gameObject, pos, q);
	}


	/************************************************************************************************/


	RigidBodyHandle PhysXComponent::CreateRigidBodyCollider(
		GameObject*             gameObject,
		const LayerHandle       layerHandle,
		const float3            pos,
		const Quaternion        q)
	{
		auto& layer = GetLayer_ref(layerHandle);
		return layer.CreateRigidBodyCollider(gameObject, pos, q);
	}


	/************************************************************************************************/


	Shape PhysXComponent::CreateCubeShape(const float3 dimensions)
	{
		auto shape = physxAPI->createShape(
			physx::PxBoxGeometry(dimensions.x, dimensions.y, dimensions.z),
			*defaultMaterial,
			false);

		return { shape };
	}


	/************************************************************************************************/


	Shape PhysXComponent::CreateSphereShape(const float radius)
	{
		auto shape = physxAPI->createShape(
			physx::PxSphereGeometry(radius),
			*defaultMaterial,
			false);

		return { shape };
	}


	/************************************************************************************************/


	Shape PhysXComponent::CreateCapsuleShape(const float radius, const float halfHeight)
	{
		auto shape = physxAPI->createShape(
			physx::PxCapsuleGeometry(radius, halfHeight),
			*defaultMaterial,
			false);

		return { shape };
	}


	/************************************************************************************************/


	PhysicsLayer& PhysXComponent::GetLayer_ref(LayerHandle layer)
	{
		return scenes[layer];
	}


	/************************************************************************************************/


	physx::PxCooking* PhysXComponent::GetCooker()
	{
		if (!cooker)
		{
			physx::PxCookingParams params{ physx::PxTolerancesScale() };

			params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
			params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

			cooker = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, params);
		}

		return cooker;
	}


	/************************************************************************************************/


	Shape PhysXComponent::CookMesh(float3* geometry, size_t geometrySize, uint32_t* indices, size_t indexCount)
	{
		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count   = geometrySize;
		meshDesc.points.stride  = sizeof(FlexKit::float3);
		meshDesc.points.data    = geometry;

		meshDesc.triangles.count    = indexCount / 3;
		meshDesc.triangles.stride   = 3 * sizeof(uint32_t);
		meshDesc.triangles.data     = indices;

		auto res = GetCooker()->validateTriangleMesh(meshDesc);

		auto triangleMesh   = cooker->createTriangleMesh(meshDesc, physxAPI->getPhysicsInsertionCallback());

		if (!triangleMesh)
			return {};

		auto pxShape        = physxAPI->createShape(PxTriangleMeshGeometry(triangleMesh), *defaultMaterial);

		return { pxShape };
	}


	/************************************************************************************************/


	class BlobStream : public physx::PxOutputStream
	{
	public:
		virtual ~BlobStream() { }

		uint32_t write(const void* src, uint32_t count) override
		{
			blob += Blob{ (char*)src, count };

			return count;
		}

		Blob blob;
	};


	Blob PhysXComponent::CookMesh2(float3* geometry, size_t geometrySize, uint32_t* indices, size_t indexCount)
	{
		physx::PxTriangleMeshDesc meshDesc;
		meshDesc.points.count   = geometrySize;
		meshDesc.points.stride  = 12;
		meshDesc.points.data    = geometry;

		std::vector<uint32_t> reversedIndices;
		reversedIndices.reserve(indexCount);
		for(int I = 0; I < indexCount; I += 3)
		{
			reversedIndices.push_back(indices[I]);
			reversedIndices.push_back(indices[I + 2]);
			reversedIndices.push_back(indices[I + 1]);
		}

		meshDesc.triangles.count	= indexCount / 3;
		meshDesc.triangles.stride	= 3 * sizeof(uint32_t);
		meshDesc.triangles.data		= reversedIndices.data();

		auto oven = GetCooker()->validateTriangleMesh(meshDesc);

		BlobStream ouputStream;
		auto res = cooker->cookTriangleMesh(meshDesc, ouputStream);

		if(res)
			return ouputStream.blob;

		return {};
	}


	/************************************************************************************************/


	class BlobInputStream : public PxInputData
	{
	public:
		BlobInputStream(const Blob& IN_blob) : blob{ IN_blob } {}
		virtual ~BlobInputStream() {}

		uint32_t getLength() const
		{
			return blob.size();
		}

		uint32_t read(void* dest, uint32_t count) override
		{
			memcpy(dest, blob.buffer.data() + offset, count);
			offset += count;

			return count;
		}

		void seek(uint32_t newOffset) override
		{
			offset = newOffset;
		}

		uint32_t tell() const override
		{
			return offset;
		}

		size_t offset = 0;
		const Blob& blob;
	};

	Shape PhysXComponent::LoadTriMeshShape(const Blob& blob)
	{
		BlobInputStream stream{ blob };

		auto triangleMesh = physxAPI->createTriangleMesh(stream);

		if (!triangleMesh)
			return { nullptr };

		auto pxShape = physxAPI->createShape(PxTriangleMeshGeometry(triangleMesh), *defaultMaterial);

		Shape shape;
		shape._ptr = pxShape;

		return shape;
	}


	/************************************************************************************************/


	void PhysXComponent::Simulate(double dt, WorkBarrier* barrier, iAllocator* temp_allocator)
	{
		for (PhysicsLayer& scene : scenes)
			if(!scene.paused)
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


	void PhysXComponent::ReleaseScene(LayerHandle handle)
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
				ProfileFunction();

				WorkBarrier barrier{ threads, &threadAllocator };
				Simulate(dt, &barrier, &threadAllocator);
				barrier.JoinLocal(); // Only works on work in local thread queue, to avoid increasing latency doing other tasks before continuing
			});
	}


	/************************************************************************************************/


	StaticColliderSystem::StaticColliderSystem(StaticColliderSystem&& IN_staticColliders) noexcept :
		colliders{ std::move(IN_staticColliders.colliders) }
	{
		layer = IN_staticColliders.layer;

		IN_staticColliders.layer = nullptr;
	}


	void StaticColliderSystem::Release()
	{
		//for (auto& collider : colliders)
		//	collider.actor->release();

		colliders.Release();
		dirtyFlags.Release();
	}


	void StaticColliderSystem::UpdateColliders(WorkBarrier* barrier, iAllocator* temp_iAllocator)
	{
		for (size_t itr = 0; itr < dirtyFlags.size(); ++itr) 
		{
			if (dirtyFlags[itr])
			{
				auto& collider = colliders[itr];
				const physx::PxTransform pose   = collider.actor->getGlobalPose();
				const Quaternion	orientation	= Quaternion{ pose.q.x, pose.q.y, pose.q.z, pose.q.w };
				const float3		position	= float3{ pose.p.x, pose.p.y, pose.p.z };

				SetPositionW	(collider.node, position);
				SetOrientation	(collider.node, orientation);

				dirtyFlags[itr] = false;
			}
		}
	}


	/************************************************************************************************/


	StaticBodyHandle StaticColliderSystem::AddCollider(const StaticColliderObject& object, bool initialDirtyFlag)
	{
		dirtyFlags.push_back(initialDirtyFlag);

		const auto idx				= colliders.push_back(object);
		const auto handle			= handles.GetNewHandle();
		colliders[handle].handle	= handle;
		handles[handle]				= idx;

		return handle;
	}


	StaticColliderSystem::StaticColliderObject StaticColliderSystem::GetAPIObject(StaticBodyHandle collider)
	{
		return colliders[collider];
	}


	/************************************************************************************************/


	void PhysicsLayer::Release()
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


	/************************************************************************************************/

	RigidBodyColliderSystem::rbColliderObject& PhysicsLayer::operator[] (const RigidBodyHandle collider)
	{
		return rbColliders[collider];
	}


	/************************************************************************************************/


	StaticColliderSystem::StaticColliderObject& PhysicsLayer::operator[] (const StaticBodyHandle collider)
	{
		return staticColliders[collider];
	}


	/************************************************************************************************/


	PhysicsLayer::PhysicsLayer(physx::PxScene* IN_scene, PhysXComponent& IN_system, iAllocator* IN_memory) noexcept :
		scene			    { IN_scene			},
		system			    { IN_system			},
		memory			    { IN_memory			},
		staticColliders	    { *this, IN_memory	},
		rbColliders		    { *this, IN_memory	},
		debugGeometry       { IN_memory },
		userCallback        { IN_memory },
		controllerManager   { PxCreateControllerManager(*IN_scene) }
	{
		FK_ASSERT(scene && memory, "INVALID ARGUEMENT");

		if (!scene)
			FK_ASSERT(0, "FAILED TO CREATE PSCENE!");

		updateColliders		= false;
		CID					= scene->createClient();
	}


	/************************************************************************************************/


	PhysicsLayer::PhysicsLayer(PhysicsLayer&& IN_scene) noexcept :
		staticColliders		{ std::move(IN_scene.staticColliders)	},
		rbColliders			{ std::move(IN_scene.rbColliders)		},
		scene				{ IN_scene.scene						},
		controllerManager	{ IN_scene.controllerManager			},
		CID					{ IN_scene.CID							},
		system				{ IN_scene.system						},
		memory				{ IN_scene.memory						},
		debugGeometry       { std::move(IN_scene.debugGeometry)     }
	{
		staticColliders.layer = this;
	}


	/************************************************************************************************/


	PhysicsLayer::~PhysicsLayer()
	{
		Release();
	}


	/************************************************************************************************/


	void PhysicsLayer::UpdateDebugGeometry()
	{
		auto& renderBuffer = scene->getRenderBuffer();

		debugGeometry.clear();

		DebugVertex v;

		auto triangleCount = renderBuffer.getNbTriangles();
		for (uint32_t i = 0; i < triangleCount; i++)
		{
			const auto& tri = renderBuffer.getTriangles()[i];

			v.position.x	= tri.pos0.x;
			v.position.y	= tri.pos0.y;
			v.position.z	= tri.pos0.z;
			v.position.w	= 1.0f;
			v.color			= float4((tri.color0 & (0xf << 0)) / 255.0f, (tri.color0 & (0xf << 8)) / 255.0f, (tri.color0 & (0xf << 16)) / 255.0f, 1.0f);
			debugGeometry.push_back(v);

			v.position.x	= tri.pos1.x;
			v.position.y	= tri.pos1.y;
			v.position.z	= tri.pos1.z;
			v.position.w	= 1.0f;
			v.color			= float4((tri.color1 & (0xf << 0)) / 255.0f, (tri.color1 & (0xf << 8)) / 255.0f, (tri.color1 & (0xf << 16)) / 255.0f, 1.0f);
			debugGeometry.push_back(v);

			v.position.x	= tri.pos2.x;
			v.position.y	= tri.pos2.y;
			v.position.z	= tri.pos2.z;
			v.position.w	= 1.0f;
			v.color			= float4((tri.color2 & (0xf << 0)) / 255.0f, (tri.color2 & (0xf << 8)) / 255.0f, (tri.color2 & (0xf << 16)) / 255.0f, 1.0f);
			debugGeometry.push_back(v);
		}

		debugTriCount = debugGeometry.size();

		auto lineCount = renderBuffer.getNbLines();
		for (uint32_t i = 0; i < lineCount; i++)
		{
			const auto& line = renderBuffer.getLines()[i];

			v.position.x = line.pos0.x;
			v.position.y = line.pos0.y;
			v.position.z = line.pos0.z;
			v.position.w = 1.0f;
			v.color		 = float4((line.color0 & (0xf << 0)) / 255.0f, (line.color0 & (0xf << 8)) / 255.0f, (line.color0 & (0xf << 16)) / 255.0f, 1.0f);
			debugGeometry.push_back(v);

			v.position.x = line.pos1.x;
			v.position.y = line.pos1.y;
			v.position.z = line.pos1.z;
			v.position.w = 1.0f;
			v.color		 = float4((line.color1 & (0xf << 0)) / 255.0f, (line.color1 & (0xf << 8)) / 255.0f, (line.color1 & (0xf << 16)) / 255.0f, 1.0f);

			debugGeometry.push_back(v);
		}

		debugLineCount	= debugGeometry.size() - debugTriCount;
	}


	/************************************************************************************************/


	void PhysicsLayer::Update(double dT, WorkBarrier* barrier, iAllocator* temp_allocator)
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

					CallUserUpdates(*barrier, *temp_allocator, stepSize);

					UpdateDebugGeometry();

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


	void PhysicsLayer::CallUserUpdates(WorkBarrier& barrier, iAllocator& allocator, double dT)
	{
		for (auto& callback : userCallback)
			callback(barrier, allocator, dT);
	}


	/************************************************************************************************/


	void PhysicsLayer::UpdateColliders(WorkBarrier* barrier, iAllocator* temp_allocator)
	{
		staticColliders.UpdateColliders(barrier, temp_allocator);
		rbColliders.UpdateColliders(barrier, temp_allocator);
	}


	/************************************************************************************************/


	StaticBodyHandle PhysicsLayer::CreateStaticCollider(GameObject* gameObject, float3 initialPosition, Quaternion initialQ)
	{
		physx::PxTransform pxInitialPose =
			physx::PxTransform{ PxMat44(PxIdentity) };
		
		pxInitialPose.q = physx::PxQuat{ initialQ.x, initialQ.y, initialQ.z, -initialQ.w};
		pxInitialPose.p = physx::PxVec3{ initialPosition.x, initialPosition.y, initialPosition.z };

		auto rigidStaticActor	= system.physxAPI->createRigidStatic(pxInitialPose);
		rigidStaticActor->userData = gameObject;

		auto node = GetSceneNode(*gameObject);

		if (node == InvalidHandle)
		{
			node = GetZeroedNode();
			auto& sceneNode			= gameObject->AddView<SceneNodeView>(node);
		}

		auto& triggerView	= gameObject->AddView<TriggerView>();
		auto& sceneNodeView = gameObject->AddView<SceneNodeView>();

		sceneNodeView.triggerEnable = true;

		triggerView->CreateSlot(ChangePositionStaticBodySlot,
			[gameObject](void* _ptr, uint64_t typeID)
			{
				FK_ASSERT(typeID == GetTypeGUID(float3));

				auto& xyz = *(float3*)_ptr;

				StaticBodySetWorldPosition(*gameObject, xyz);
			});

		triggerView->CreateSlot(ChangeOrientationStaticBodySlot,
			[gameObject](void* _ptr, uint64_t typeID)
			{
				FK_ASSERT(typeID == GetTypeGUID(Quaternion));

				auto& q = *(Quaternion*)(_ptr);

				StaticBodySetWorldOrientation(*gameObject, q.Conjugate());
			});

		triggerView->Connect(TranslationSignalID,		ChangePositionStaticBodySlot);
		triggerView->Connect(SetOrientationmSignalID,	ChangeOrientationStaticBodySlot);
		triggerView->Connect(SetTransformSignalID,		ChangePositionStaticBodySlot,
			[gameObject](void* _ptr, uint64_t typeID)
			{
				StaticBodySetWorldPosition(*gameObject,		GetLocalPosition(*gameObject));
				StaticBodySetWorldOrientation(*gameObject,	GetOrientationLocal(*gameObject).Conjugate());
			});

		auto handle = staticColliders.AddCollider(
			{	.node		= node,
				.handle		= InvalidHandle,
				.actor		= rigidStaticActor,
				.gameObject	= gameObject
			}, true);

		scene->addActor(*rigidStaticActor);

		return handle;
	}


	/************************************************************************************************/


	RigidBodyHandle PhysicsLayer::CreateRigidBodyCollider(GameObject* gameObject, float3 initialPosition, Quaternion initialQ)
	{
		auto node = GetZeroedNode();
		SetOrientation	(node, initialQ);
		SetPositionW	(node, initialPosition);

		physx::PxTransform pxInitialPose = physx::PxTransform{ PxMat44(PxIdentity) };

		pxInitialPose.q = physx::PxQuat{ initialQ.x, initialQ.y, initialQ.z, initialQ.w };
		pxInitialPose.p = physx::PxVec3{ initialPosition.x, initialPosition.y, initialPosition.z };

		PxRigidDynamic* rigidBodyActor = system.physxAPI->createRigidDynamic(pxInitialPose);
		rigidBodyActor->userData = gameObject;


		size_t handleIdx = rbColliders.colliders.push_back({	node,
																rigidBodyActor });
		rigidBodyActor->setMass(1.0f);
		scene->addActor(*rigidBodyActor);

		return RigidBodyHandle{ static_cast<unsigned int>(handleIdx) };
	}


	/************************************************************************************************/


	void PhysicsLayer::ReleaseCollider(RigidBodyHandle handle)
	{
		scene->removeActor(*rbColliders.colliders[handle].actor);
	}


	/************************************************************************************************/


	void PhysicsLayer::ReleaseCollider(StaticBodyHandle handle)
	{
		scene->removeActor(*staticColliders.colliders[handle].actor);

		staticColliders.colliders.remove_unstable(staticColliders.colliders.begin() + handle);
	}


	/************************************************************************************************/


	void PhysicsLayer::SetPosition(RigidBodyHandle collider, float3 xyz)
	{
		auto& colliderIMPL	= rbColliders.colliders[collider.INDEX];
		auto pose			= colliderIMPL.actor->getGlobalPose();

		pose.p = {xyz.x, xyz.y, xyz.z};
		colliderIMPL.actor->setGlobalPose(pose);
	}


	/************************************************************************************************/


	void PhysicsLayer::SetMass(RigidBodyHandle collider, float m)
	{
		auto& colliderIMPL = rbColliders.colliders[collider.INDEX];
		colliderIMPL.actor->setMass(m);
	}


	/************************************************************************************************/


	void PhysicsLayer::ApplyForce(RigidBodyHandle rbHandle, float3 xyz)
	{
		auto actor = rbColliders[rbHandle].actor;
		actor->addForce({ xyz.x, xyz.y, xyz.z }, PxForceMode::eIMPULSE);
	}


	/************************************************************************************************/


	void PhysicsLayer::SetRigidBodyPosition(RigidBodyHandle rbHandle, const float3 xyz)
	{
		auto actor  = rbColliders[rbHandle].actor;
		auto pose   = actor->getGlobalPose();
		pose.p      = { xyz.x, xyz.y, xyz.z };

		rbColliders[rbHandle].actor->setGlobalPose(pose);
	}


	/************************************************************************************************/


	void PhysicsLayer::AddUpdateCallback(PhysicsLayerUpdateCallback&& callback)
	{
		userCallback.emplace_back(std::move(callback));
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
		return Apply(GO, [](CameraView& cameraView)
			{
				return cameraView.camera;
			},
			[]
			{
				return (CameraHandle)InvalidHandle;
			});
	}


	bool HandleTPCEvents(GameObject& GO, Event evt)
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
						keyStates.y	= state ? 1 : 0;
						return true;
					case TPC_MoveBackward:
						keyStates.y = state ? -1 : 0;
						return true;
					case TPC_MoveLeft:
						keyStates.x = state ? -1 : 0;
						return true;
					case TPC_MoveRight:
						keyStates.x = state ? 1 : 0;
						return true;
					case TPC_MoveUp:
						return true;
					case TPC_MoveDown:
						return true;
					}
				}
				return false;
			}, []() -> bool { return false; });
	}


	/************************************************************************************************/


	CameraControllerView& CreateThirdPersonCameraController(GameObject& gameObject, LayerHandle layer, iAllocator& allocator, const float R, const float H)
	{
		auto& characterController = gameObject.AddView<CharacterControllerView>(layer, float3{ 0, 10, 0 });

		auto& cameraController = gameObject.AddView<CameraControllerView>(
			CameraControllerComponent::GetComponent().Create(
				ThirdPersonCamera(
					GetControllerHandle(gameObject),
					GetControllerNode(gameObject))));

		auto& cameraView = gameObject.AddView<CameraView>(cameraController.GetData().camera);

		return cameraController;
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
				return InvalidHandle;
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


	void UpdateThirdPersonCameraControllers(const float2& mouseInput, const double dT)
	{
		for (auto& controller : CameraControllerComponent::GetComponent())
		{
			controller.componentData.UpdateCharacter(mouseInput, dT);
			controller.componentData.UpdateCamera(mouseInput, { mouseInput.x, mouseInput.y }, dT);
		}
	}


	/************************************************************************************************/


	UpdateTask& QueueThirdPersonCameraControllers(UpdateDispatcher& dispatcher, float2 mouseInput, const double dT)
	{
		struct TPC_Update {};

		return dispatcher.Add<TPC_Update>(
			[&](auto& builder, auto& data){},
			[mouseInput, dT](TPC_Update& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				UpdateThirdPersonCameraControllers(mouseInput, dT);
			});
	}


	/************************************************************************************************/


	NodeHandle RigidBodyView::GetNode() const
	{
		return GetComponent().GetLayer(layer)[staticBody].node;
	}

	void RigidBodyView::AddShape(Shape shape)
	{
		GetComponent().GetLayer(layer)[staticBody].actor->attachShape(*shape._ptr);
	}


	NodeHandle GetRigidBodyNode(GameObject& gameObject)
	{
		return Apply(gameObject,
			[](RigidBodyView& staticBody) -> NodeHandle
			{
				return staticBody.GetNode();
			},
			[] { return (NodeHandle)InvalidHandle; });
	}


	/************************************************************************************************/


	void SetRigidBodyPosition(GameObject& gameObject, const float3 worldPOS)
	{
		Apply(gameObject, [&](RigidBodyView& rigidBody)
			{
				PhysXComponent::GetComponent().GetLayer_ref(rigidBody.layer).SetRigidBodyPosition(rigidBody.staticBody, worldPOS);
			});
	}


	/************************************************************************************************/


	void ApplyForce(GameObject& gameObject, const float3 force)
	{
		Apply(gameObject, [&](RigidBodyView& rigidBody)
			{
				PhysXComponent::GetComponent().GetLayer_ref(rigidBody.layer).ApplyForce(rigidBody.staticBody, force);
			});
	}


	/************************************************************************************************/


	NodeHandle GetStaticBodyNode(GameObject& gameObject)
	{
		return Apply(gameObject,
			[](StaticBodyView& staticBody) -> NodeHandle
			{
				return staticBody.GetNode();
			},
			[] { return (NodeHandle)InvalidHandle; });
	}


	/************************************************************************************************/


	void StaticBodySetWorldPosition(GameObject& gameObject, const float3 xyz)
	{
		Apply(gameObject,
			[&](StaticBodyView& staticBody)
			{
				auto actor				= staticBody->actor;
				PxTransform transform	= actor->getGlobalPose();
				transform.p				= physx::PxVec3{ xyz.x, xyz.y, xyz.z };
				actor->setGlobalPose(transform);
			});
	}


	/************************************************************************************************/


	void StaticBodySetScale(GameObject& gameObject, const float3 xyz)
	{
		Apply(gameObject,
			[&](StaticBodyView& staticBody)
			{
				auto actor = staticBody->actor;
				PxMeshScale scale{ PxVec3{xyz.x, xyz.y, xyz.z } };

				PxShape* shape;
				actor->getShapes(&shape, 1);

				if (shape)
				{
					auto geometry   = shape->getGeometry();
					auto& mesh  = geometry.triangleMesh();

					if (!shape->isExclusive())
					{
						mesh.meshFlags;
						mesh.scale = scale;

						actor->detachShape(*shape);
						shape = PhysXComponent::GetComponent().GetAPI()->createShape(geometry.any(), *PhysXComponent::GetComponent().GetDefaultMaterial(), true);
						actor->attachShape(*shape);
					}

					else
						mesh.scale = scale;

					FK_ASSERT(mesh.isValid());
				}

				//transform               = physx::PxVec3{ xyz.x, xyz.y, xyz.z };
				//actor->setGlobalPose(transform);
				SetFlag(staticBody->node, SceneNodes::SCALE);
			});
	}


	/************************************************************************************************/


	void StaticBodySetWorldOrientation(GameObject& gameObject, const Quaternion q)
	{
		Apply(gameObject,
			[&](StaticBodyView& staticBody)
			{
				auto actor = staticBody->actor;
				PxTransform transform = actor->getGlobalPose();
				transform.q = physx::PxQuat{ q.x, q.y, q.z, -q.w };
				actor->setGlobalPose(transform);
				transform = actor->getGlobalPose();

			});
	}


	/************************************************************************************************/


	NodeHandle GetControllerNode(GameObject& gameObject)
	{
		return Apply(gameObject, [](CharacterControllerView& controller)
			{
				return controller.GetNode();
			},
			[]
			{
				FK_LOG_WARNING("Failed to retrieve character controller node");
				return (NodeHandle)InvalidHandle;
			});
	}


	CharacterControllerHandle GetControllerHandle(GameObject& gameObject)
	{
		return Apply(gameObject, [](CharacterControllerView& controller)
			{
				return controller.controller;
			},
			[]
			{
				FK_LOG_WARNING("Failed to retrieve character controller handle");
				return (CharacterControllerHandle)InvalidHandle;
			});
	}


	float3 GetControllerPosition(GameObject& gameObject)
	{
		return Apply(gameObject, [&](CharacterControllerView& controller)
			{
				return controller.GetPosition();
			}, [] { return float3{ 0, 0, 0 };  });
	}


	void SetControllerPosition(GameObject& gameObject, const float3 xyz)
	{
		Apply(gameObject, [&](CharacterControllerView& controller)
			{
				controller.SetPosition(xyz);
			});
	}


	void SetControllerOrientation(GameObject& gameObject, const Quaternion q)
	{
		Apply(gameObject, [&](CharacterControllerView& controller)
			{
				controller.SetOrientation(q);
			});
	}


	void MoveController(GameObject& gameObject, const float3& v, physx::PxControllerFilters& filters, double dt)
	{
		Apply(gameObject, [&](CharacterControllerView& controller)
			{
				controller.Move(v, dt, filters);
			});
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
		physx::PxFilterObjectAttributes	attributes0,
		physx::PxFilterData				filterData0,
		physx::PxFilterObjectAttributes	attributes1,
		physx::PxFilterData				filterData1,
		physx::PxPairFlags&				pairFlags,
		const byte*						constantBlock,
		physx::PxU32					constantBlockSize)
	{
		pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;
		return physx::PxFilterFlag::eDEFAULT;
	}


	/************************************************************************************************/


	void ThirdPersonCamera::SetRotation(const float3 xyz)
	{
		if (xyz[0] != 0.0f)
			FlexKit::SetOrientationL(pitchNode, Quaternion{ RadToDegree(xyz[0]), 0.0f, 0.0f });
		

		if (xyz[1] != 0.0f)
			FlexKit::SetOrientationL(yawNode, Quaternion{ 0.0f, RadToDegree(xyz[1]), 0.0f });


		if (xyz[2] != 0.0f)
			FlexKit::SetOrientationL(rollNode, Quaternion{ 0.0f, 0.0f, RadToDegree(xyz[2]) });

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


	void ThirdPersonCamera::UpdateCharacter(const float2 mouseInput, const double dt)
	{
		UpdateCharacter(mouseInput, keyStates, dt);
	}


	void ThirdPersonCamera::UpdateCharacter(const float2 mouseInput, const ThirdPersonCamera::KeyStates& keyStates, const double dt)
	{
		auto& controllerImpl = CharacterControllerComponent::GetComponent()[controller];

		controllerImpl.updateTimer += dt;
		controllerImpl.mouseMoved += mouseInput;

		const float focusHeight     = controllerImpl.focusHeight;
		const float cameraDistance  = controllerImpl.cameraDistance;

		const double deltaTime = dt;
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

			movementVector += keyStates.x * right;
			movementVector += keyStates.y * forward;

			float3 newVelocity = velocity;

			if (movementVector.magnitudeSq() > 0.05f)
			{
				movementVector.normalize();
				movementVector *= float2(keyStates.x, keyStates.y).Magnitude();

				velocity += movementVector * acceleration * (float)deltaTime;
			}
			
			if (velocity.magnitudeSq() < 0.1f || velocity.isNaN())
				velocity = 0.0f;

			auto controller = controllerImpl.controller;

			PxControllerState state;
			controller->getState(state);

			if(!floorContact)
				velocity += -up * gravity;

			velocity -= velocity * drag * (float)deltaTime;

			const auto  desiredMove    = velocity * (float)deltaTime;
			const auto& pxPrevPos      = controller->getPosition();
			const auto  prevPos        = float3{ (float)pxPrevPos.x, (float)pxPrevPos.y, (float)pxPrevPos.z };

			physx::PxControllerFilters filters;
			auto collision = controller->move(
				{   desiredMove.x,
					desiredMove.y,
					desiredMove.z },
				0.001f,
				dt,
				filters);

			floorContact = PxControllerCollisionFlag::eCOLLISION_DOWN & collision;

			const auto      pxPostPos   = controller->getFootPosition();
			const float3    postPos     = pxVec3ToFloat3(pxPostPos);
			const auto      deltaPos    = prevPos - postPos;

			if (desiredMove.magnitudeSq() * 0.5f >= deltaPos.magnitude())
				velocity = 0.0f;

			FlexKit::SetPositionW(controllerImpl.node, pxVec3ToFloat3(controller->getPosition()));
		}
	}


	/************************************************************************************************/


	void ThirdPersonCamera::UpdateCamera(const float2 mouseInput, const KeyStates& keyState, const double dt)
	{
		auto& controllerImpl = CharacterControllerComponent::GetComponent()[controller];

		const float focusHeight		= controllerImpl.focusHeight;
		const float cameraDistance	= controllerImpl.cameraDistance;

		
		const float3 footPosition	= pxVec3ToFloat3(controllerImpl.controller->getFootPosition());

		auto& layer = PhysXComponent::GetComponent().GetLayer_ref(controllerImpl.layer);

		const float3 forward	{ (GetForwardVector() * float3(1, 0, 1)).normal() };
		const float3 right		{ GetRightVector() };
		const float3 up			{ 0, 1, 0 };

		const float3 origin1	= footPosition + up * 3 + 2.0f * -forward;
		const float3 ray1		= -forward.normal();

		float cameraZ = cameraDistance;

		layer.RayCast(origin1, ray1, 10,
			[&](auto hit)
			{
				cameraZ = Min(hit.distance + 1.0f, cameraDistance);
				return false;
			});

		const float3 origin2	= footPosition - forward * cameraZ + up * 10.0f;
		const float3 ray2		= (-up).normal();
		float cameraMinY		= 0;

		layer.RayCast(origin2, ray2, 100,
			[&](auto hit)
			{
				cameraMinY = hit.distance - origin2.y + 1;
				return false;
			});

		const auto cameraY		= Max(focusHeight - std::tanf(pitch) * cameraZ, cameraMinY);

		SetPositionW(objectNode, footPosition);

		const auto position				= footPosition - forward * cameraZ + up * cameraY;
		const auto newCameraPosition	= lerp(position, cameraPosition, 0.65f);
		cameraPosition					= newCameraPosition;

		SetPositionW(yawNode, cameraPosition);

		CameraComponent::GetComponent().MarkDirty(camera);
	}


	/************************************************************************************************/


	StaticBodyHandle StaticBodyComponent::Create(GameObject* gameObject, LayerHandle layer, float3 pos, Quaternion q)
	{
		auto res = physx.CreateStaticCollider(gameObject, layer, pos, q);

		if (onConstruction)
			onConstruction(*gameObject, nullptr, 0, ValueMap{});

		return res;
	}


	void StaticBodyComponent::AddComponentView(GameObject& gameObject, ValueMap userValues, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		if (userValues.empty())
			return;

		StaticBodyHeaderBlob header;
		memcpy(&header, (void*)buffer, sizeof(header));

		LayerHandle* layer = FindValue<LayerHandle*>(userValues, PhysicsLayerKID).value_or(nullptr);

		auto& staticBody	= gameObject.hasView(StaticBodyComponentID) ? static_cast<StaticBodyView&>(*gameObject.GetView(StaticBodyComponentID)) : gameObject.AddView<StaticBodyView>(*layer);

		auto currentPOS		= GetLocalPosition(gameObject);
		auto currentQ		= GetOrientationLocal(gameObject);
		auto globalPose		= staticBody->actor->getGlobalPose();

		globalPose.p = PxVec3{
			currentPOS.x,
			currentPOS.y,
			currentPOS.z };

		globalPose.q = PxQuat{
			currentQ.x,
			currentQ.y,
			currentQ.z,
			-currentQ.w };

		staticBody->actor->setGlobalPose(globalPose);

		for (size_t I = 0; I < header.shapeCount; I++)
		{
			StaticBodyShape triMeshShape;
			memcpy(&triMeshShape, buffer + sizeof(header) + I * sizeof(triMeshShape), sizeof(triMeshShape));

			Shape shape;
			auto& physX = PhysXComponent::GetComponent();

			switch (triMeshShape.type)
			{
			case StaticBodyType::Cube:
			{
				shape = physX.CreateCubeShape(float3{ triMeshShape.cube.dimensions[0], triMeshShape.cube.dimensions[1], triMeshShape.cube.dimensions[2] });
			}   break;
			case StaticBodyType::Capsule:
			{
				shape = physX.CreateCapsuleShape(triMeshShape.capsule.radius, triMeshShape.capsule.height);
			}   break;
			case StaticBodyType::Sphere:
			{
				shape = physX.CreateSphereShape(triMeshShape.sphere.radius);
			}   break;
			case StaticBodyType::TriangleMesh:
			{
				auto assetHandle    = LoadGameAsset(triMeshShape.triMeshResource);
				Resource* resource  = GetAsset(assetHandle);

				Blob triMeshBlob{ ((char*)resource) + sizeof(Resource), resource->ResourceSize - sizeof(Resource) };
				shape = physX.LoadTriMeshShape(triMeshBlob);
			}	break;
			case StaticBodyType::BoundingVolume:
			{
				auto boundingVolume = GetBoundingSphere(gameObject);
				switch (triMeshShape.bv.subType)
				{
				case StaticBodyType::Cube:
					shape = physX.CreateSphereShape(boundingVolume.w);
					break;
				case StaticBodyType::Capsule:
					shape = physX.CreateCapsuleShape(boundingVolume.w, 0);
					break;
				case StaticBodyType::Sphere:
					shape = physX.CreateSphereShape(boundingVolume.w);
					break;
				default:
					FK_LOG_WARNING("Incorrect argument found in staticbodyComponent entity data.");
					break;
				}
			}	break;
			default:
				break;
			};

			auto localPose = shape._ptr->getLocalPose();

			localPose.p = PxVec3{
				triMeshShape.position.x,
				triMeshShape.position.y,
				triMeshShape.position.z };

			localPose.q = PxQuat{
				triMeshShape.orientation.x,
				triMeshShape.orientation.y,
				triMeshShape.orientation.z,
				triMeshShape.orientation.w };

			shape._ptr->setLocalPose(localPose);

			staticBody.AddShape(shape);
		}

		if (onConstruction)
			onConstruction(gameObject, (const char*)buffer, bufferSize, userValues);
	}


	void StaticBodyComponent::Remove(LayerHandle layer, StaticBodyHandle sb) noexcept
	{
		auto& layer_ref		= GetComponent().GetLayer_ref(layer);
		auto& staticBody	= layer_ref[sb];
		staticBody.actor->release();
		layer_ref.ReleaseCollider(sb);
	}


	/************************************************************************************************/


	StaticBodyView::StaticBodyView(GameObject& gameObject, StaticBodyHandle IN_staticBody, LayerHandle IN_layer) :
			staticBody	{ IN_staticBody },
			layer		{ IN_layer } {}


	StaticBodyView::StaticBodyView(GameObject& gameObject, LayerHandle IN_layer, float3 pos, Quaternion q) :
			staticBody	{ GetComponent().Create(&gameObject, layer, pos, q) },
			layer		{ IN_layer } {}


	StaticBodyView::~StaticBodyView()
	{
		RemoveAll();
		GetComponent().Remove(layer, staticBody);
	}


	NodeHandle StaticBodyView::GetNode() const
	{
		return GetComponent().GetLayer_ref(layer)[staticBody].node;
	}


	GameObject& StaticBodyView::GetGameObject() const
	{
		return *GetComponent().GetLayer_ref(layer)[staticBody].gameObject;
	}


	void StaticBodyView::AddShape(Shape shape)
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		staticBody_data.actor->attachShape(*shape._ptr);
	}


	void StaticBodyView::RemoveShape(uint32_t idx)
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		if (idx >= staticBody_data.actor->getNbShapes())
			return;

		PxShape* shape = nullptr;

		staticBody_data.actor->getShapes(&shape, sizeof(PxShape*), idx);
		staticBody_data.actor->detachShape(*shape);
	}


	void StaticBodyView::RemoveAll()
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		PxShape* shape = nullptr;

		while (staticBody_data.actor->getNbShapes() > 0)
		{
			staticBody_data.actor->getShapes(&shape, sizeof(PxShape*), 0);
			staticBody_data.actor->detachShape(*shape);
		}
	}


	physx::PxShape* StaticBodyView::GetShape(size_t idx)
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		physx::PxShape* shape;
		staticBody_data.actor->getShapes(&shape, sizeof(shape), idx);

		return shape;
	}


	size_t StaticBodyView::GetShapeCount() const noexcept
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		return staticBody_data.actor->getNbShapes();
	}


	StaticColliderSystem::StaticColliderObject* StaticBodyView::operator -> ()
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];

		return &staticBody_data;
	};


	void StaticBodyView::SetUserData(void* _ptr) noexcept
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];
		staticBody_data.User = _ptr;
	}


	void* StaticBodyView::GetUserData() noexcept
	{
		auto& staticBody_data = GetComponent().GetLayer_ref(layer)[staticBody];
		return staticBody_data.User;
	}


	/************************************************************************************************/


	RigidBodyComponent::RigidBodyComponent(PhysXComponent& IN_physx) :
		physx{ IN_physx } {}


	RigidBodyHandle RigidBodyComponent::CreateRigidBody(GameObject* gameObject, LayerHandle layer, float3 pos, Quaternion q)
	{
		return physx.CreateRigidBodyCollider(gameObject, layer, pos, q);
	}

	void RigidBodyComponent::AddComponentView(GameObject& GO, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		FK_ASSERT(0);
	}

	void RigidBodyComponent::Remove(RigidBodyView& rigidBody)
	{

	}

	PhysicsLayer& RigidBodyComponent::GetLayer(LayerHandle layer)
	{
		return physx.GetLayer_ref(layer);
	}


	/**********************************************************************

	Copyright (c) 2015 - 2022 Robert May

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
