#include "Scene.h"
#include "GraphicsComponents.h"
#include "intersection.h"

#include "AnimationRuntimeUtilities.H"
#include "Components.h"
#include "componentBlobs.h"
#include "ProfilingUtilities.h"
#include "SceneLoadingContext.h"
#include "KeyValueIds.h"

#include <cmath>

namespace FlexKit
{	/************************************************************************************************/


	std::optional<GameObject*> FindGameObject(Scene& scene, const char* id)
	{
		auto& visableComponent = SceneVisibilityComponent::GetComponent();

		for (auto& visable : scene)
		{
			auto& gameObject = *visableComponent[visable].entity;
			auto res = Apply(
				gameObject,
				[&]
				(
					StringIDView& ID
				)
				{
					auto goID = ID.GetString();
					return strncmp(goID, id, 64) == 0;
				},
				[]
				{ return false; });

			if (res)
				return &gameObject;
		}

		return {};
	}


	void DEBUG_ListSceneObjects(Scene& scene)
	{
		auto& visableComponent = SceneVisibilityComponent::GetComponent();

		for (auto& visable : scene)
		{
			auto& gameObject = *visableComponent[visable].entity;
			Apply(
				gameObject,
				[&]
				(
					StringIDView& ID
				)
				{
					std::cout << ID.GetString() << '\n';
				});
		}
	}


	/************************************************************************************************/


	void SetVisable(GameObject& gameObject, bool v)
	{
		Apply(
			gameObject,
			[&]( SceneVisibilityView& visable )
			{
				visable.SetVisable(v);
			});
	}


	/************************************************************************************************/


	BrushView::BrushView(GameObject& gameObject)
	{
		auto node = GetSceneNode(gameObject);

		GetComponent()[brush].Node = node != InvalidHandle ? node : GetZeroedNode();
	}


	/************************************************************************************************/


	BrushView::BrushView(GameObject& gameObject, TriMeshHandle	triMesh)
	{
		auto node = GetSceneNode(gameObject);

		auto& meshes = GetComponent()[brush].meshes;

		GetComponent()[brush].meshes.push_back(triMesh);
		GetComponent()[brush].Node = node != InvalidHandle ? node : GetZeroedNode();
	}


	/************************************************************************************************/


	BrushView::~BrushView()
	{
		auto& brush_ref = GetComponent()[brush];
		ReleaseMesh(brush_ref.Occluder);

		for (auto& mesh : brush_ref.meshes)
			ReleaseMesh(mesh);

		GetComponent().Remove(brush);
	}


	/************************************************************************************************/


	std::span<TriMeshHandle> BrushView::GetMeshes() const noexcept
	{
		return GetComponent()[brush].meshes;
	}


	/************************************************************************************************/


	MaterialHandle BrushView::GetMaterial() noexcept
	{
		return GetComponent()[brush].material;
	}


	/************************************************************************************************/


	void BrushView::SetMaterial(MaterialHandle material) noexcept
	{
		GetComponent()[brush].material = material;
	}


	/************************************************************************************************/


	BrushView::operator Brush& () noexcept
	{
		return GetComponent()[brush];
	}


	/************************************************************************************************/


	AABB BrushView::GetAABB() const noexcept
	{
		auto meshes = GetMeshes();

		if (meshes.empty())
			return {};
		else
		{
			AABB aabb{};

			for (const auto mesh : meshes)
				aabb += GetMeshResource(mesh)->AABB;

			return aabb;
		}
	}


	/************************************************************************************************/


	BoundingSphere BrushView::GetBoundingSphere() const noexcept
	{
		auto meshes = GetMeshes();

		if (meshes.empty())
			return {};
		else
		{
			BoundingSphere BS = GetMeshResource(meshes.front())->BS;

			for (const auto mesh : std::span(meshes.begin() + 1, meshes.end()))
			{
				const auto meshBS = GetMeshResource(mesh)->BS;
				const auto center1 = BS.xyz();
				const auto center2 = meshBS.xyz();
				const auto midpoint = (center1 + center2) / 2.0f;

				const auto r1 = (midpoint - center1).magnitude() + BS.w;
				const auto r2 = (midpoint - center2).magnitude() + meshBS.w;

				BS = float4(midpoint, Max(r1, r2));
			}
			return BS;// BoundingSphere{ aabb.MidPoint(), aabb.Span().magnitude() / 4.0f };
		}
	}


	/************************************************************************************************/


	void BrushView::SetTransparent(const bool transparent) noexcept
	{
		GetComponent()[brush].Transparent = transparent;
	}


	/************************************************************************************************/


	void BrushView::PushMesh(const TriMeshHandle mesh) noexcept
	{
		auto& meshes = GetComponent()[brush].meshes;
		meshes.push_back(mesh);

		AddRef(mesh);
	}


	/************************************************************************************************/


	void BrushView::SetOptionalAllocator(iAllocator& allocator) noexcept
	{
		auto& meshes = GetComponent()[brush].meshes;

		Vector<TriMeshHandle, 16, uint8_t>	newContainer{ allocator };
		newContainer += meshes;

		meshes = std::move(newContainer);
	}


	/************************************************************************************************/


	void BrushView::RemoveMesh(const TriMeshHandle mesh) noexcept
	{
		auto& meshes = GetComponent()[brush].meshes;

		if(auto itr = std::ranges::find(meshes, mesh); itr != meshes.end())
			meshes.remove_unstable(itr);

		ReleaseMesh(mesh);
	}


	/************************************************************************************************/


	bool BrushView::GetTransparent() const noexcept
	{
		return GetComponent()[brush].Transparent;
	}


	/************************************************************************************************/

	PointLight::~PointLight()
	{
		if (shadowState)
			shadowState->Release();
	}


	/************************************************************************************************/


	PointLight::PointLight(PointLight&& rhs)
	{
		K = rhs.K;
		I = rhs.I;
		R = rhs.R;

		Position    = rhs.Position;
		shadowMap   = rhs.shadowMap;

		forceDisableShadowMapping   = rhs.forceDisableShadowMapping;
		state                       = rhs.state;
		shadowState                 = rhs.shadowState;

		rhs.shadowMap       = InvalidHandle;
		rhs.shadowState     = nullptr;
	}


	/************************************************************************************************/


	PointLight& PointLight::operator = (PointLight&& rhs)
	{
		K = rhs.K;
		I = rhs.I;
		R = rhs.R;

		Position    = rhs.Position;
		shadowMap   = rhs.shadowMap;

		forceDisableShadowMapping   = rhs.forceDisableShadowMapping;
		state                       = rhs.state;
		shadowState                 = rhs.shadowState;

		rhs.shadowMap       = InvalidHandle;
		rhs.shadowState     = nullptr;

		return *this;
	}


	PointLightView::PointLightView(GameObject& gameObject, float3 color, float intensity, float radius, NodeHandle node) : light{ GetComponent().Create() }
	{
		auto& poingLight        = GetComponent()[light];
		poingLight.K			= color;
		poingLight.I			= intensity;
		poingLight.R			= radius;
		poingLight.Position		= node != InvalidHandle ? node : FlexKit::GetSceneNode(gameObject);
	}

	float PointLightView::GetRadius() const noexcept
	{
		return GetComponent()[light].R;
	}

	void PointLightView::SetRadius(float r) noexcept
	{
		GetComponent()[light].R = Max(r, 0.1f);
	}

	float PointLightView::GetIntensity()
	{
		return GetComponent()[light].I;
	}

	float3 PointLightView::GetK()
	{
		return GetComponent()[light].K;
	}

	void PointLightView::SetK(float3 color)
	{
		GetComponent()[light].K = color;
	}

	void PointLightView::SetIntensity(float I)
	{
		GetComponent()[light].I = I;
	}

	void PointLightView::SetNode(NodeHandle node) const noexcept
	{
		GetComponent()[light].Position = node;
	}

	NodeHandle PointLightView::GetNode() const noexcept
	{
		return GetComponent()[light].Position;
	}


	/************************************************************************************************/


	PointLightHandle GetPointLight(GameObject& go)
	{
		return Apply(
			go,
			[](PointLightView& pointLight) -> PointLightHandle
			{
				return pointLight;
			},
			[]() -> PointLightHandle
			{
				return InvalidHandle;
			});
	}


	/************************************************************************************************/


	BoundingSphere GetVisibilityBoundingSphere(GameObject& go)
	{
		return Apply(
			go,
			[&](SceneVisibilityView& visibility) -> BoundingSphere
			{
				return visibility.GetBoundingSphere();
			},
			[]() -> BoundingSphere { return {}; });
	}


	/************************************************************************************************/


	void SetBoundingSphereFromMesh(GameObject& go)
	{
		Apply(
			go,
			[&](SceneVisibilityView&	visibility,
				BrushView&				brush)
			{
				auto boundingSphere = brush.GetBoundingSphere();
				visibility.SetBoundingSphere(boundingSphere);
			});
	}


	/************************************************************************************************/


	void SetBoundingSphereRadius(GameObject& go, const float radius)
	{
		Apply(
			go,
			[&](SceneVisibilityView& visibility)
			{
				visibility.SetBoundingSphere(BoundingSphere{ 0, 0, 0, radius });
			});
	}


	/************************************************************************************************/


	void SetBoundingSphere(GameObject& go, const BoundingSphere bs)
	{
		Apply(
			go,
			[&](SceneVisibilityView& visibility)
			{
				visibility.SetBoundingSphere(bs);
			});
	}



	/************************************************************************************************/


	void SetBoundingSphereFromLight(GameObject& go)
	{
		Apply(
			go,
			[](	SceneVisibilityView&    visibility,
				PointLightView&	        pointLight)
			{
				visibility.SetBoundingSphere({ 0, 0, 0, pointLight.GetRadius() });
			});
	}


	/************************************************************************************************/


	void SetTransparent(GameObject& go, const bool tranparent)
	{
		Apply(
			go,
			[&](SceneVisibilityView& visibility)
			{
				visibility.SetTransparency(tranparent);
			});
	}


	/************************************************************************************************/

	
	BoundingSphere GetBoundingSphereFromMesh(GameObject& go)
	{
		const auto scale = GetScale(go).Max();

		return Apply(
			go,
			[&](BrushView& brushView)	{ return brushView.GetBoundingSphere(); },
			[]							{ return BoundingSphere{ 0 }; });
	}


	/************************************************************************************************/


	AABB GetAABBFromMesh(GameObject& go)
	{
		return Apply(
			go,
			[&](BrushView& brushView)	{ return brushView.GetAABB(); },
			[]()						{ return AABB{}; });
	}


	/************************************************************************************************/


	BoundingSphere GetBoundingSphere(GameObject& go)
	{
		return Apply(
			go,
			[&](SceneVisibilityView& visibility)
			{
				auto pos    = GetWorldPosition(go);
				auto scale  = GetScale(go).Max();
				auto bs     = visibility.GetBoundingSphere();

				return BoundingSphere{ bs.xyz() + pos, bs.w * scale };
			},
			[]()
			{
				return BoundingSphere{ 0, 0, 0, 0 };
			});
	}


	/************************************************************************************************/


	void BrushComponentEventHandler::OnCreateView(GameObject& gameObject, ValueMap, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		auto node = GetSceneNode(gameObject);
		if (node == InvalidHandle)
			node = GetZeroedNode();

		if (!gameObject.hasView(TransformComponentID))
			gameObject.AddView<SceneNodeView<>>(node);

		BrushComponentBlob brushComponent;
		memcpy(&brushComponent, buffer, sizeof(BrushComponentBlob));

		BrushView* brush = nullptr;
		if (!gameObject.hasView(FlexKit::BrushComponentID))
			brush = &gameObject.AddView<BrushView>();
		else
			brush = (BrushView*)gameObject.GetView(FlexKit::BrushComponentID);

		for(uint I = 0; I < brushComponent.meshCount; I++)
		{
			GUID_t guid;
			memcpy(&guid, buffer + sizeof(BrushComponentBlob) + sizeof(GUID_t) * I, sizeof(GUID_t));
			auto [triMesh, loaded] = FindMesh(guid);

			if (!loaded)
				triMesh = LoadTriMeshIntoTable(renderSystem.GetImmediateUploadQueue(), guid);

			if (triMesh == InvalidHandle)
				return;

			brush->PushMesh(triMesh);
		}

		SetBoundingSphereFromMesh(gameObject);
	}


	/************************************************************************************************/

	
	void PointLightEventHandler::OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator)
	{
		PointLightComponentBlob pointLight;

		memcpy(&pointLight, buffer, sizeof(pointLight));

		if(!gameObject.hasView(PointLightComponentID))
			gameObject.AddView<PointLightView>(pointLight.K, pointLight.IR[0], pointLight.IR[1], GetSceneNode(gameObject));
		else
		{
			auto* pointLightView = static_cast<PointLightView*>(gameObject.GetView(PointLightComponentID));
			pointLightView->SetK(pointLight.K);
			pointLightView->SetIntensity(pointLight.IR[0]);
			pointLightView->SetRadius(pointLight.IR[1]);
			pointLightView->SetNode(GetSceneNode(gameObject));
		}

		SetBoundingSphereFromLight(gameObject);

		EnablePointLightShadows(gameObject);
	}


	/************************************************************************************************/


	void Scene::AddGameObject(GameObject& go, NodeHandle node)
	{
		auto& view = go.AddView<SceneVisibilityView>(node, sceneID);
		sceneEntities.push_back(view);
	}

	void Scene::AddGameObject(GameObject& go)
	{
		auto& view = go.AddView<SceneVisibilityView>(GetSceneNode(go), sceneID);
		sceneEntities.push_back(view);
	}


	/************************************************************************************************/



	void Scene::RemoveEntity(GameObject& go)
	{
		Apply(go, 
		[&, allocator = this->allocator](SceneVisibilityView& vis) 
		{
			const auto handle = vis.visibility;
			go.RemoveView(vis);

			sceneEntities.remove_unstable(
				find(sceneEntities, [&](auto i) { return i == handle; }));
		},	[] { });
	}


	/************************************************************************************************/


	void Scene::ClearScene()
	{
		auto&	visables	= SceneVisibilityComponent::GetComponent();
		auto	visableID	= SceneVisibilityComponent::GetComponentID();

		for (auto visHandle : sceneEntities)
		{
			if (!visHandle)
				continue;

			auto entity		= visables[visHandle].entity;
			auto visable	= entity->GetView(visableID);
			entity->RemoveView(visable);
		}
		sceneEntities.clear();

		for (auto* gameObject : ownedGameObjects) {
			gameObject->Release();
			allocator->release(*gameObject);
		}
		ownedGameObjects.clear();

		bvh = SceneBVH(*allocator);
	}


	/************************************************************************************************/


	std::span<TriMeshHandle> GetTriMesh(GameObject& gameObject) noexcept
	{
		return Apply(gameObject,
			[&](BrushView& brush) -> std::span<TriMeshHandle>
			{
				return brush.GetMeshes();
			}, 
			[]() -> std::span<TriMeshHandle>
			{
				return {};
			});
	}


	Brush* GetBrush(GameObject& gameObject) noexcept
	{
		return Apply(gameObject,
			[&](BrushView& brush) -> Brush*
			{
				return &brush.GetBrush();
			},
			[]() -> Brush*
			{
				return nullptr;
			});
	}

	void SetTriMesh(GameObject& gameObject, TriMeshHandle triMesh, uint32_t offset) noexcept
	{
		return Apply(gameObject,
			[&](BrushView& brush)
			{
				brush.GetMeshes()[offset] = triMesh;
			});
	}


	/************************************************************************************************/


	void ToggleSkinned(GameObject& go, bool enabled)
	{
		return Apply(go,
			[&](BrushView& brushView)
			{
				brushView.GetBrush().Skinned = enabled;
			});
	}


	/************************************************************************************************/


	#define ComponentSize 9
	#define ComponentMask (1 << ComponentSize) - 1

	inline uint CreateMortonCode(const uint3 XYZ)
	{
		const uint X = uint(XYZ[0]) & ComponentMask;
		const uint Y = uint(XYZ[1]) & ComponentMask;
		const uint Z = uint(XYZ[2]) & ComponentMask;

		uint mortonCode = 0;

		for (uint I = 0; I < ComponentSize; I++)
		{
			const uint x_bit = X & (1 << I);
			const uint y_bit = Y & (1 << I);
			const uint z_bit = Z & (1 << I);

			const uint XYZ = x_bit << 2 | y_bit << 0 | z_bit << 1;
			//const uint XYZ = x_bit << 0 | y_bit << 1 | z_bit << 2;

			mortonCode |= XYZ << I * 3;
		}

		return mortonCode;
	}


	/************************************************************************************************/


	SceneBVH SceneBVH::Build(const Scene& scene, iAllocator& allocator)
	{
		ProfileFunction();

		auto& visables              = scene.sceneEntities;
		auto& visibilityComponent   = SceneVisibilityComponent::GetComponent();

		auto elements   = Vector<BVHElement>{ &allocator, visables.size() };
		auto nodes      = Vector<BVHNode>{ &allocator, visables.size() * 2 };

		AABB worldAABB;

		for (auto& visable : visables)
			worldAABB += visibilityComponent[visable].GetAABB();

		const float3 worldSpan  = worldAABB.Span();
		const float3 offset     = -worldAABB.Min;

		for (auto& visable : visables)
		{
			const auto POS          = GetPositionW(visibilityComponent[visable].node);
			const auto normalizePOS = (offset + POS) / worldSpan;
			const auto ID           = CreateMortonCode({
										(uint)(normalizePOS.x * ComponentMask),
										(uint)(normalizePOS.y * ComponentMask),
										(uint)(normalizePOS.z * ComponentMask)});

			elements.push_back({ ID, visable });
		}

		std::sort(elements.begin(), elements.end());

		// Phase 1 - Build Leaf Nodes -
		const size_t end            = (size_t)std::ceil(elements.size() / 4.0f);
		const size_t elementCount   = elements.size();

		for (uint16_t I = 0; I < end; ++I)
		{
			BVHNode node;

			const uint16_t begin  = 4 * I;
			const uint16_t end    = (uint16_t)Min(elementCount, 4 * I + 4);

			for (uint16_t II = 4 * I; II < end; II++) {
				const uint16_t idx  = II;
				auto& visable       = elements[idx].handle;
				const auto aabb     = visibilityComponent[visable].GetAABB();
				node.boundingVolume += aabb;
			}

			node.children   = 4 * I;
			node.count      = end - begin;
			node.Leaf       = true;

			nodes.push_back(node);
		}

		// Phase 2 - Build Interior Nodes -
		uint begin = 0;
		while (true)
		{
			const uint end  = (uint)nodes.size();
			const uint temp = (uint)end;

			if (end - begin > 1)
			{
				for (uint I = begin; I < end; I += 4)
				{
					BVHNode node;

					const size_t localEnd = Min(end, I + 4);
					for (size_t II = I; II < localEnd; II++)
						node.boundingVolume += nodes[II].boundingVolume;

					node.children   = I;
					node.count      = uint8_t(localEnd - I);
					node.Leaf       = false;

					nodes.push_back(node);
				}
			}
			else
				break;

			begin = temp;
		}

		SceneBVH BVH{ allocator };
		BVH.elements    = elements.Copy(allocator);
		BVH.nodes       = nodes.Copy(allocator);
		BVH.root        = begin;

		return BVH;
	}


	/************************************************************************************************/


	ComputeLod_RES ComputeLOD(const Brush& brush, const float3 CameraPosition, const float maxZ)
	{
		auto brushPosition		= GetPositionW(brush.Node);
		auto distanceFromView	= (CameraPosition - brushPosition).magnitude();

		auto* mesh						= GetMeshResource(brush.meshes[0]); // TODO: Factor in all meshes
		const auto maxLod				= mesh->lods.size() - 1;
		const auto highestLoadedLod		= mesh->GetHighestLoadedLodIdx();

		auto temp = pow((distanceFromView) / maxZ, 1.0f / 5.0f);

		const uint32_t requestedLodLevel	= temp * maxLod;
		const uint32_t usableLodLevel		= Max(requestedLodLevel, highestLoadedLod);

		return ComputeLod_RES{
			.requestedLOD   = requestedLodLevel,
			.recommendedLOD = usableLodLevel,
		};
	}


	/************************************************************************************************/


	void PushPV(GameObject& gameObject, const Brush& brush, PVS& pvs, const float3 CameraPosition, float maxZ)
	{
		auto brushPosition      = GetPositionW(brush.Node);
		auto distanceFromView   = (CameraPosition - brushPosition).magnitude();

		static_vector<uint8_t> lodLevels;
		for(auto mesh : brush.meshes)
		{
			auto* mesh_ptr = GetMeshResource(mesh);

			const auto maxLod               = mesh_ptr->lods.size() - 1;
			const auto highestLoadedLod     = mesh_ptr->GetHighestLoadedLodIdx();

			/*
			// Alternate, screen space size based LOD selection 
			const auto aabb = mesh->AABB;
			const auto WT   = GetWT(e.Node);

			const auto UpperRight   = WT * aabb.Max;
			const auto LowerLeft    = WT * aabb.Min;

			auto UpperRight_DC    = PV * UpperRight;
			auto LowerLeft_DC     = PV * LowerLeft;

			UpperRight_DC   /= UpperRight_DC.w;
			LowerLeft_DC    /= LowerLeft_DC.w;

			auto screenSpan = UpperRight_DC - LowerLeft;
			auto screenArea = screenSpan.x * screenSpan.y;

			const uint32_t requestedLodLevel    = maxLod - std::ceil(pow(screenArea, 2) * maxLod);
			*/

			const auto normalizedAdjustedDistance = pow((distanceFromView) / maxZ, 1.0f / 5.0f);
			
			const uint32_t requestedLodLevel    = normalizedAdjustedDistance * maxLod;
			lodLevels.push_back(Max(requestedLodLevel, highestLoadedLod));
		}

		pvs.push_back(
			PVEntry{
				.SortID         = CreateSortingID(false, false, (size_t)distanceFromView),
				.brush          = &brush,
				.gameObject     = &gameObject,
				.OcclusionID    = 0,
				.LODlevel       = lodLevels });
	}


	/************************************************************************************************/


	void GatherScene(Scene* SM, CameraHandle Camera, PVS& pvs)
	{
		ProfileFunction();

		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);
		FK_ASSERT(&pvs      != nullptr);


		auto& cameraComponent = CameraComponent::GetComponent();

		const auto CameraNode	= cameraComponent.GetCameraNode(Camera);
		const float3	POS		= GetPositionW(CameraNode);
		const Quaternion Q		= GetOrientation(CameraNode);
		const auto F			= GetFrustum(Camera);
		const auto& Visibles	= SceneVisibilityComponent::GetComponent();

		for(auto handle : SM->sceneEntities)
		{
			const auto potentialVisible = Visibles[handle];

			if(	potentialVisible.visable && 
				potentialVisible.entity->hasView(BrushComponent::GetComponentID()))
			{
				auto Ls	= GetLocalScale		(potentialVisible.node).x;
				auto Pw	= GetPositionW		(potentialVisible.node);
				auto Lq	= GetOrientation	(potentialVisible.node);
				auto BS = BoundingSphere{ 
					Lq * potentialVisible.boundingSphere.xyz() + Pw, 
					Ls * potentialVisible.boundingSphere.w };

				Apply(*potentialVisible.entity,
					[&](BrushView& view)
					{
						const auto& brush = view.GetBrush();

						if (Intersects(F, BS))
							PushPV(*potentialVisible.entity, brush, pvs, POS);
					});
			}
		}
	}


	/************************************************************************************************/


	GatherPassesTask& GatherScene(UpdateDispatcher& dispatcher, Scene* scene, CameraHandle C, iAllocator& allocator)
	{
		auto& task = dispatcher.Add<GetPVSTaskData>(
			[&](auto& builder, auto& data)
			{
				data.scene			= scene;
				data.camera			= C;

				builder.SetDebugString("Gather Scene");
			},
			[&allocator, &threads = *dispatcher.threads](GetPVSTaskData& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				PVS pvs{ &threadAllocator };

				auto activePasses = MaterialComponent::GetComponent().GetActivePasses(threadAllocator);

				GatherScene(data.scene, data.camera, pvs);
				SortPVS(&pvs, &CameraComponent::GetComponent().GetCamera(data.camera));

				Vector<PassPVS> passes{ &allocator };

				for (auto& pass : activePasses)
					passes.emplace_back(pass, &allocator);

				Parallel_For(
					threads, threadAllocator,
					passes.begin(), passes.end(), 2,
					[&](PassPVS& pass, iAllocator& threadAllocator)
					{
						const auto passID = pass.pass;
						auto& materials = MaterialComponent::GetComponent();

						pass.pvs.reserve(128);

						for (auto& visable: pvs)
						{
							const auto passes = materials.GetPasses(visable.brush->material);

							if (std::find(passes.begin(), passes.end(), passID) != passes.end())
								pass.pvs.push_back(visable);
						}
					});

				data.solid  = pvs.Copy(allocator);
				data.passes = std::move(passes);
			});

		return task;
	}


	/************************************************************************************************/


	void LoadLodLevels(UpdateDispatcher& dispatcher, GatherPassesTask& PVS, CameraHandle camera, RenderSystem& renderSystem, iAllocator& allocator)
	{
		struct _ {};

		dispatcher.Add<_>(
			[&](UpdateDispatcher::UpdateBuilder& builder, auto& data)
			{
				builder.AddInput(PVS);
				builder.SetDebugString("Load LODs");
			},
			[&allocator = allocator, &PVS = PVS.GetData().solid, &renderSystem = renderSystem, camera = camera](_& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				//static std::mutex m;

				if (!PVS.size())
					return;

				const float3 cameraPosition = GetPositionW(GetCameraNode(camera));

				bool submit = false;

				CopyContextHandle copyHandle = InvalidHandle;

				for (auto& visable : PVS)
				{
					for (auto mesh : visable.brush->meshes)
					{
						auto res = ComputeLOD(*visable.brush, cameraPosition, 10'000);

						if (res.recommendedLOD != res.requestedLOD)
						{
							auto triMesh = GetMeshResource(mesh);
							auto lodsLoaded = triMesh->GetHighestLoadedLodIdx();

							if (!submit)
							{
								copyHandle = renderSystem.OpenUploadQueue();
								submit = true;
							}

							for (auto itr = lodsLoaded - 1; itr >= res.requestedLOD; itr--)
								LoadLOD(triMesh, itr, renderSystem, copyHandle, allocator);
						}
					}
				}

				if(submit)
					renderSystem.SubmitUploadQueues(SYNC_Graphics, &copyHandle);

				//m.unlock();
			});
	}


	/************************************************************************************************/


	void ReleaseSceneAnimation(AnimationClip* AC, iAllocator* Memory)
	{
		for (size_t II = 0; II < AC->FrameCount; ++II) 
		{
			Memory->free(AC->Frames[II].Joints);
			Memory->free(AC->Frames[II].Poses);
		}

		Memory->free(AC->Frames);
		Memory->free(AC->mID);
	}


	/************************************************************************************************/


	void BindJoint(Scene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode)
	{
		//SM->TaggedJoints.push_back({ Entity, Joint, TargetNode });
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, GUID_t Guid, iAllocator* allocator, iAllocator* temp)
	{
		bool Available = isAssetAvailable(Guid);
		if (Available)
		{
			auto RHandle = LoadGameAsset(Guid);
			auto R		 = GetAsset(RHandle);

			EXITSCOPE(FreeAsset(RHandle));

			if (R != nullptr) {
				SceneResourceBlob* sceneBlob = (SceneResourceBlob*)R;
				char* buffer = (char*)R;

				const auto blockCount = sceneBlob->blockCount;
				
				size_t						offset                  = 0;
				size_t						currentBlock            = 0;
				SceneNodeBlock*				nodeBlock               = nullptr;
				ComponentRequirementBlock*	componentRequirement    = nullptr;

				while (offset < sceneBlob->ResourceSize && currentBlock < blockCount)
				{
					SceneBlock* block = reinterpret_cast<SceneBlock*>(buffer + sizeof(SceneResourceBlob) + offset);
					switch (block->blockType)
					{
						case SceneBlockType::NodeTable:
						{
							// TODO: CRC Check
							// TODO: more error checking
							FK_ASSERT(nodeBlock == nullptr, "Multiple Node Blocks Defined!");
							nodeBlock = reinterpret_cast<SceneNodeBlock*>(block);

							const auto nodeCount = nodeBlock->header.nodeCount;
							ctx.nodes.reserve(nodeCount);


							for (size_t itr = 0; itr < nodeCount; ++itr)
							{
								float3		position;
								Quaternion	orientation;
								float3		scale;

								//auto sceneNode = &nodeBlock->nodes[itr];
								SceneNodeBlock::SceneNode nodeData;
								memcpy(&nodeData, ((char*)block) + sizeof(SceneNodeBlock::Header) + sizeof(SceneNodeBlock::SceneNode) * itr, sizeof(SceneNodeBlock::SceneNode));

								memcpy(&position,		&nodeData.position, sizeof(float3));
								memcpy(&orientation,	&nodeData.orientation, sizeof(orientation));
								memcpy(&scale,			&nodeData.scale, sizeof(float3));

								auto newNode = GetNewNode();
								SetOrientationL	(newNode, orientation);
								SetPositionL	(newNode, position);
								SetScale		(newNode, { scale[0], scale[0], scale[0] }); // Engine only supports uniform scaling, still stores all 3 for future stuff
								SetFlag			(newNode, SceneNodes::StateFlags::SCALE);

								if (nodeData.parent != INVALIDHANDLE)
									SetParentNode(ctx.nodes[nodeData.parent], newNode);

								ctx.nodes.push_back(newNode);
							}
						}	break;
						case SceneBlockType::ComponentRequirementTable:
						case SceneBlockType::EntityBlock:
						{
							FK_ASSERT(nodeBlock != nullptr, "No Node Block defined!");
							//FK_ASSERT(componentRequirement != nullptr, "No Component Requirement Block defined!");

							EntityBlock::Header entityBlock;
							memcpy(&entityBlock, block, sizeof(entityBlock));
							auto& gameObject = allocator->allocate<GameObject>(allocator);
							ctx.scene.ownedGameObjects.push_back(&gameObject);

							size_t itr                  = 0;
							size_t componentOffset      = 0;
							const size_t componentCount = entityBlock.componentCount;

							while(itr < componentCount)
							{
								if (((char*)&entityBlock) + componentOffset >= ((char*)&entityBlock) + entityBlock.blockSize)
									break;

								ComponentBlock::Header component;
								memcpy(&component, (std::byte*)block + sizeof(entityBlock) + componentOffset, sizeof(component));

								const ComponentID ID = component.componentID;
								if (component.blockType != EntityComponentBlock) // malformed blob?
									break;
								else if (ID == SceneNodeView<>::GetComponentID())
								{
									SceneNodeComponentBlob blob;
									memcpy(&blob, (std::byte*)block + sizeof(entityBlock) + componentOffset, sizeof(blob));

									auto node = ctx.nodes[blob.nodeIdx];
									gameObject.AddView<SceneNodeView<>>(node);

									if (!blob.excludeFromScene)
										ctx.scene.AddGameObject(gameObject, node);
								}
								else if (ComponentAvailability(ID) == true)
								{
									static_vector<KeyValuePair> values;
									values.emplace_back(SceneLoadingContextKID, &ctx);

									GetComponent(ID).AddComponentView(
														gameObject,
														std::span{ values },
														(std::byte*)(block) + sizeof(entityBlock) + componentOffset,
														component.blockSize,
														allocator);
								}

								++itr;
								componentOffset += component.blockSize;
								if (itr >= componentCount)
									break;
							}

							SetBoundingSphereFromMesh(gameObject);
						}
						case SceneBlockType::EntityComponentBlock:
							break;
					default:
						break;
					}
					currentBlock++;
					offset += block->blockSize;
				}


				UpdateTransforms();

				return true;
			}
		}


		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, const char* LevelName, iAllocator* allocator, iAllocator* Temp)
	{
		if (isAssetAvailable(LevelName))
		{
			auto RHandle = LoadGameAsset(LevelName);
			auto R = GetAsset(RHandle);

			FINALLY
			{
				FreeAsset(RHandle);
			}
			FINALLYOVER

			return LoadScene(RS, ctx, R->GUID, allocator, Temp);
		}
		return false;
	}


	/************************************************************************************************/


	Vector<PointLightHandle> Scene::FindPointLights(const Frustum &f, iAllocator* tempMemory) const
	{
		Vector<PointLightHandle> lights{tempMemory};

		auto& visables = SceneVisibilityComponent::GetComponent();

		for (auto entity : sceneEntities)
			Apply(*visables[entity].entity,
				[&](PointLightView&			pointLight,
					SceneVisibilityView&	visibility,
					SceneNodeView<>&		sceneNode)
				{
					const auto position	    = sceneNode.GetPosition();
					const auto scale		= sceneNode.GetScale();
					const auto radius		= pointLight.GetRadius();

					if (Intersects(f, BoundingSphere{ position, radius * scale }))
						lights.emplace_back(pointLight);
				});

		return lights;
	}


	/************************************************************************************************/


	BuildBVHTask& Scene::UpdateSceneBVH(UpdateDispatcher& dispatcher, UpdateTask& transformDependency, iAllocator* allocator)
	{
		return dispatcher.Add<SceneBVHBuild>(
			[&](UpdateDispatcher::UpdateBuilder& builder, SceneBVHBuild& data)
			{
				builder.SetDebugString("BVH");
				builder.AddInput(transformDependency);
			},
			[this, allocator = allocator](SceneBVHBuild& data, iAllocator& threadAllocator)
			{
				FK_LOG_9("Build BVH");
				ProfileFunction();

				bvh         = bvh.Build(*this, threadAllocator).Copy(*allocator);
				data.bvh    = &bvh;
			}
		);
	}


	/************************************************************************************************/


	PointLightGatherTask& Scene::GetPointLights(UpdateDispatcher& dispatcher, iAllocator* tempMemory) const
	{
		return dispatcher.Add<PointLightGather>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightGather& data)
			{
				data.pointLights	= Vector<PointLightHandle>{ tempMemory, 64 };
				data.scene			= this;

				builder.SetDebugString("Point Light Gather");
			},
			[this](PointLightGather& data, iAllocator& threadAllocator)
			{
				FK_LOG_9("Point Light Gather");
				ProfileFunction();

				Vector<PointLightHandle> tempList{ &threadAllocator, 64 };
				auto& visables = SceneVisibilityComponent::GetComponent();

				for (auto entity : sceneEntities)
				{

					Apply(*visables[entity].entity,
						[&](PointLightView&         pointLight,
							SceneVisibilityView&    visibility)
						{
							tempList.emplace_back(pointLight);
						});
				}

				data.pointLights = tempList;
			}
		);
	}


	/************************************************************************************************/


	PointLightShadowGatherTask& Scene::GetVisableLights(UpdateDispatcher& dispatcher, CameraHandle camera, BuildBVHTask& bvh, iAllocator* temporaryMemory) const
	{
		return dispatcher.Add<PointLightShadowGather>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightShadowGather& data)
			{
				builder.SetDebugString("Point Light Shadow Gather");
				builder.AddInput(bvh);

				data.pointLightShadows = Vector<PointLightHandle>{ temporaryMemory };
			},
			[this, &bvh = bvh.GetData().bvh, camera = camera](PointLightShadowGather& data, iAllocator& threadAllocator)
			{
				FK_LOG_9("Point Light Shadow Gather");
				ProfileFunction();

				Vector<PointLightHandle> visablePointLights{ &threadAllocator };
				auto& visabilityComponent = SceneVisibilityComponent::GetComponent();

				const auto frustum = GetFrustum(camera);

				auto& lights = PointLightComponent::GetComponent();


				if constexpr (false)
				{
					for (auto& light : lights)
						visablePointLights.push_back(light.handle);
				}
				else
				{
					bvh->Traverse(frustum,
						[&](VisibilityHandle intersector, auto& intersectionResult)
						{
							GameObject& go = *visabilityComponent[intersector].entity;
							Apply(go,
								[&](PointLightView& light)
								{
									visablePointLights.push_back(light);
								}
							);
						});
				}
				data.pointLightShadows = visablePointLights;
			}
		);
	}


	/************************************************************************************************/


	PointLightUpdate& Scene::UpdatePointLights(UpdateDispatcher& dispatcher, BuildBVHTask& bvh, PointLightShadowGatherTask& visablePointLights, iAllocator* temporaryMemory, iAllocator* persistentMemory) const
	{
		class Task : public iWork
		{
		public:
			Task(PointLightHandle IN_light, SceneBVH& IN_bvh, iAllocator& IN_allocator) :
				iWork           {},
				persistentMemory{ IN_allocator },
				lightHandle     { IN_light },
				bvh             { IN_bvh }
			{
				_debugID = "PointLightUpdate_Task";
			}

			void Run(iAllocator& threadLocalAllocator)
			{
				ProfileFunction();

				auto& visables  = SceneVisibilityComponent::GetComponent();
				auto& lights    = PointLightComponent::GetComponent();

				auto& light         = lights[lightHandle];
				const float3 POS    = GetPositionW(light.Position);
				const float  r      = light.R;
				AABB lightAABB{ POS - r, POS + r };

				Vector<VisibilityHandle> PVS{ &threadLocalAllocator };

				bvh.Traverse(lightAABB,
					[&](VisibilityHandle visable, auto& intersionResult)
					{
						PVS.push_back(visable);
					});

				if(const auto flags = GetFlags(light.Position); flags & (SceneNodes::DIRTY | SceneNodes::UPDATED))
					light.state = LightStateFlags::Dirty;

				std::sort(
					std::begin(PVS), std::end(PVS),
					[](const auto& lhs, const auto& rhs)
					{
						return lhs < rhs;
					});

				auto markDirty =
					[&]
					{
						light.state = LightStateFlags::Dirty;
						light.shadowState->visableObjects = PVS;
					};


				if (light.shadowState)
				{   // Compare previousPVS with current PVS to see if shadow map needs update
					auto& previousPVS = light.shadowState->visableObjects;

					if (previousPVS.size() == PVS.size())
					{
						for (size_t I = 0; I < PVS.size(); ++I)
						{
							if (PVS[I] != previousPVS[I])
							{
								markDirty();
								break; // do full update
							}
							const auto flags = GetFlags(visables[PVS[I]].node);

							if (flags & (SceneNodes::UPDATED | SceneNodes::DIRTY))
							{
								markDirty();
								return;
							}
						}
					}
					else
					{
						markDirty();
						return;
					}

					return;
				}

				if (!light.shadowState)
					light.shadowState = &persistentMemory.allocate<CubeMapState>(&persistentMemory);

				light.state = LightStateFlags::Dirty;
				light.shadowState->shadowMapSize = 128;
				light.shadowState->visableObjects = PVS;
			}

			void Release() {}

			iAllocator&             persistentMemory;
			SceneBVH&               bvh;
			const PointLightHandle  lightHandle = InvalidHandle;
		};

		return dispatcher.Add<PointLightUpdate_DATA>(
			[&](UpdateDispatcher::UpdateBuilder& builder, PointLightUpdate_DATA& data)
			{
				builder.SetDebugString("Point Light Shadow Gather");
				builder.AddInput(bvh);
				builder.AddInput(visablePointLights);

				data.dirtyList = Vector<PointLightHandle>{ temporaryMemory };
			},
			[   this,
				&bvh                = bvh.GetData().bvh,
				&visablePointLights = visablePointLights.GetData().pointLightShadows,
				persistentMemory    = persistentMemory,
				&threads            = *dispatcher.threads
			]
			(PointLightUpdate_DATA& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				WorkBarrier     barrier{ threads, &threadAllocator };
				Vector<Task*>   taskList{ &threadAllocator, visablePointLights.size() };

				for (auto visableLight : visablePointLights)
				{
					auto& task = threadAllocator.allocate<Task>(visableLight, *bvh, *persistentMemory);
					barrier.AddWork(task);
					taskList.push_back(&task);
				}

				for (auto& task : taskList)
					threads.AddWork(task);

				barrier.Join();
				
				auto& lights = PointLightComponent::GetComponent();

				for (auto visableLight : visablePointLights)
				{
					auto& light = lights[visableLight];
					if (lights[visableLight].state == LightStateFlags::Dirty && light.shadowState)
						data.dirtyList.push_back(visableLight);
				}
			}
		);
	}


	/************************************************************************************************/


	Vector<SceneRayCastResult> Scene::RayCast(FlexKit::Ray v, iAllocator& allocator) const
	{
		Vector<SceneRayCastResult> results{ &allocator };

		if (!bvh.Valid())
			const_cast<SceneBVH&>(bvh) = bvh.Build(*this, allocator);

		bvh.Traverse(v,
			[&](auto& visable, const auto& intersectionResult)
			{
				results.emplace_back(visable, intersectionResult.value());
			});

		std::sort(
			results.begin(), results.end(),
			[](auto& lhs, auto& rhs)
			{
				return lhs.d < rhs.d;
			});

		return results;
	}


	/************************************************************************************************/


	size_t	Scene::GetPointLightCount()
	{
		auto& visables		= SceneVisibilityComponent::GetComponent();
		size_t lightCount	= 0;

		for (auto entity : sceneEntities)
			Apply(*visables[entity].entity,
				[&](PointLightView& pointLight) { lightCount++; });

		return lightCount;
	}


	/************************************************************************************************/


	Brush::VConstantsLayout Brush::GetConstants() const
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetTransform(Node, &WT);

		Brush::VConstantsLayout	constants;

		auto& materials = MaterialComponent::GetComponent();

		if (material != InvalidHandle)
		{
			const auto albedo       = materials.GetProperty<float4>(material, GetCRCGUID(PBR_ALBEDO)).value_or(float4{ 0.7f, 0.7f, 0.7f, 0.3f });
			const auto specular     = materials.GetProperty<float4>(material, GetCRCGUID(PBR_SPECULAR)).value_or(float4{ 1.0f, 1.0f, 1.0f, 1.0f });
			const auto roughness    = materials.GetProperty<float>(material,  GetCRCGUID(PBR_ROUGHNESS)).value_or(1.0f);
			const auto metal        = materials.GetProperty<float>(material,  GetCRCGUID(PBR_METAL)).value_or(1.0f);

			constants.MP.albedo     = albedo.xyz();
			constants.MP.roughness  = roughness;
			constants.MP.kS         = specular.x;
			constants.MP.metallic   = metal;
		}
		else
		{
			constants.MP.albedo     = float3{ 0.7f, 0.7f, 0.7f };
			constants.MP.roughness  = 0.7f;
			constants.MP.kS         = 1.0f;
			constants.MP.metallic   = 0.0f;
		}

		constants.Transform = XMMatrixToFloat4x4(WT).Transpose();

		if (material != InvalidHandle)
		{
			const auto& textures    = MaterialComponent::GetComponent()[material].Textures;
			constants.textureCount  = (uint32_t)textures.size();

			for (auto& texture : textures)
				constants.textureHandles[std::distance(std::begin(textures), &texture)] = uint4{ 256, 256, texture.to_uint() };
		}
		else
			constants.textureCount = 0;

		return constants;
	}


	/************************************************************************************************/


	void Release(PoseState* EPS, iAllocator* allocator)
	{
		if (EPS->Joints)		allocator->free(EPS->Joints);
		if (EPS->CurrentPose)	allocator->free(EPS->CurrentPose);

		EPS->Joints = nullptr;
		EPS->CurrentPose = nullptr;
	}


	/************************************************************************************************/


	void EnablePointLightShadows(GameObject& gameObject)
	{
		if (!Apply(gameObject,
			[](PointLightShadowMapView& pointLight){ return true; }, []{ return false; }))
		{
			Apply(gameObject,
				[&](PointLightView& pointLight)
				{
					gameObject.AddView<PointLightShadowMapView>(
						_PointLightShadowCaster{
							pointLight,
							pointLight.GetNode()
						}
					);
				});
		}
	}


}	/************************************************************************************************/


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