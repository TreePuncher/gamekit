#pragma once

#include "buildsettings.h"
#include "Assets.h"
#include "GraphicsComponents.h"
#include "Materials.h"

#include "AnimationUtilities.h" 
#include "graphics.h"
#include "CoreSceneObjects.h"
#include "defaultpipelinestates.h"
#include "RuntimeComponentIDs.h"

#include "FrameGraph.h"

namespace FlexKit
{
	// Handles
	using BrushHandle		= Handle_t<32, BrushComponentID>;
	using LightHandle		= Handle_t<32, GetTypeGUID(PointLight)>;
	using LightShadowHandle	= Handle_t<32, PointLightShadowMapID>;
	using SceneHandle		= Handle_t<32, GetTypeGUID(SceneID)>;
	using VisibilityHandle	= Handle_t<32, GetTypeGUID(BrushID)>;


	//Forward Declarations 
	struct PoseState;

	typedef Handle_t<16, GetCRCGUID(SceneEntity)> SceneEntityHandle;
	typedef size_t SpotLightHandle;
	typedef Pair<bool, int64_t> GSPlayAnimation_RES;

	class  Scene;
	struct SceneNodeComponentSystem;

	enum class QuadTreeNodeLocation: int
	{
		UpperRight,
		UpperLeft,
		LowerLeft,
		LowerRight
	};

	const float MinNodeSize = 1;


	struct BrushComponentEventHandler
	{
		BrushComponentEventHandler(RenderSystem& IN_renderSystem) : renderSystem{ IN_renderSystem }{}

		Brush OnCreate(GameObject& gameObject, Brush args)
		{
			return args;
		}


		void OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);

		RenderSystem& renderSystem;
	};

	using BrushComponent = BasicComponent_t<Brush, BrushHandle, BrushComponentID, BrushComponentEventHandler>;

	class BrushView : public ComponentView_t<BrushComponent>
	{
	public:
		BrushView(GameObject& gameObject);
		BrushView(GameObject& gameObject, TriMeshHandle	triMesh);
		~BrushView();

		std::span<TriMeshHandle> GetMeshes() const noexcept;

		auto& GetBrush(this auto&& self) noexcept
		{
			if constexpr (std::is_const_v<decltype(self)>)
				return std::add_const_t<decltype(GetComponent()[self.brush])>(GetComponent()[self.brush]);
			else
				return std::forward<decltype(GetComponent()[self.brush])>(GetComponent()[self.brush]);
		}

		MaterialHandle GetMaterial() noexcept;

		void SetMaterial(MaterialHandle material) noexcept;

		operator Brush& () noexcept;


		AABB GetAABB() const noexcept;

		BoundingSphere	GetBoundingSphere() const noexcept;

		bool			GetTransparent() const noexcept;
		void			SetTransparent(const bool transparent) noexcept;

		void			PushMesh(const TriMeshHandle mesh) noexcept;
		void			RemoveMesh(const TriMeshHandle mesh) noexcept;


		// Brush::Meshes has internal space for 16 meshes.
		// When more than 16 meshes are expected,
		// set the optional allocator to allow for expansion
		// Brush::Meshes can store up to 256 meshes. 
		void SetOptionalAllocator(iAllocator& allocator) noexcept;

		BrushHandle	brush;
	};


	/************************************************************************************************/


	std::span<TriMeshHandle>	GetTriMesh(GameObject& go) noexcept;
	void						SetTriMesh(GameObject& go, TriMeshHandle triMesh, uint32_t offset = 0) noexcept;
	Brush*						GetBrush(GameObject& go) noexcept;

	void ToggleSkinned(GameObject& go, bool enabled);


	/************************************************************************************************/


	enum LightStateFlags
	{
		Dirty = 0x00,
		Clean = 0x01,
		Unused = 0x02,
	};


	struct CubeMapState
	{
		CubeMapState(iAllocator* IN_allocator) :
			allocator		{ IN_allocator },
			visableObjects	{ IN_allocator } {}

		void Release()
		{
			visableObjects.Release();
			allocator->free(this);
		}

		uint						shadowMapSize = 1;
		Vector<VisibilityHandle>	visableObjects;
		iAllocator*					allocator;
	};


	enum class LightType
	{
		PointLight,
		SpotLight,
		Direction,
		PointLightNoShadows
	};

	struct Light
	{
		Light() = default;
		~Light();

		Light(Light&& rhs);

		Light& operator = (Light&& rhs);
		Light& operator = (const Light& rhs) = delete;

		uint3 GetExtra() const;

		LightType			type;
		bool				enabled						= true;
		bool				forceDisableShadowMapping	= false;
		LightStateFlags		state;
		NodeHandle			node			= InvalidHandle;
		CubeMapState*		shadowState		= nullptr;
		ResourceHandle		shadowMap		= InvalidHandle;

		float3		K;
		float		I, R;
		float		innerAngle = (float)pi / 16.0f;
		float		outerAngle = (float)pi / 8.0f;
	};


	class LightFactory
	{
	public:
		static Light	OnCreate(GameObject& gameObject);
		static void		OnCreateView(GameObject& gameObject, ValueMap user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
	};

	using LightComponent	= BasicComponent_t<Light, LightHandle, LightComponentID, LightFactory>;

	class LightView : public ComponentView_t<LightComponent>
	{
	public:
		LightView(GameObject& gameObject, float3 color = { 1, 1, 1 }, float intensity = 100, float radius = 100, NodeHandle node = InvalidHandle, bool triggered = false);


		float3		GetK();
		float		GetIntensity();
		NodeHandle	GetNode() const noexcept;
		float		GetRadius() const noexcept;

		void		SetK			(float3 color);
		void		SetIntensity	(float I);
		void		SetNode			(NodeHandle node) const noexcept;
		void		SetRadius		(float r) noexcept;
		void		SetOuterRadius	(float r) noexcept;

		void		SetType(LightType) noexcept;
		LightType	GetType() const noexcept;

		Light*	operator -> (this auto& self) noexcept  { return &GetComponent()[self.light]; }

		operator LightHandle () { return light; }

		LightHandle	light;
	};


	LightHandle GetLight(GameObject& go);


	template<IsConstCharStar ... TY>
	struct LightQuery
	{
		using Type		= LightView&;
		using ValueType	= LightView;
		static constexpr bool IsConst() { return false; }

		bool		IsValid		(const LightView&)				{ return true; }
		bool		Available	(const GameObject& gameObject)	{ return gameObject.hasView(LightComponentID); }
		LightView&	GetValue	(GameObject& gameObject)		{ return GetView<LightView>(gameObject); }
	};


	/************************************************************************************************/


	struct VisibilityFields
	{
		GameObject*		entity		= nullptr;	// to get parents in raycast results
		SceneHandle		scene		= InvalidHandle;
		NodeHandle		node		= InvalidHandle;

		bool			visable		= true;
		bool			rayVisible	= false;
		bool			transparent = false;

		BoundingSphere	boundingSphere = { 0, 0, 0, 0 }; // model space

		AABB			GetAABB() const
		{
			const auto posW = GetPositionW(node);
			return { posW - boundingSphere.w, posW + boundingSphere.w };
		}
	};


	class SceneVisibilityComponent : public Component<SceneVisibilityComponent, SceneVisibilityComponentID>
	{
	public:
		template<typename ... TY_args>
		SceneVisibilityComponent(iAllocator* allocator, TY_args&&... args) :
			elements		{  allocator },
			handles			{  allocator } {}


		SceneVisibilityComponent(iAllocator* allocator) :
			elements		{  allocator },
			handles			{  allocator } {}


		~SceneVisibilityComponent()
		{
			elements.Release();
		}


		struct ElementData
		{
			VisibilityHandle handle;
			VisibilityFields componentData;
		};


		VisibilityHandle Create(const VisibilityFields& initial)
		{
			auto handle		= handles.GetNewHandle();
			handles[handle] = (index_t)elements.push_back({ handle, initial });

			return handle;
		}


		VisibilityHandle Create(VisibilityFields&& initial)
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)elements.emplace_back(handle, initial);

			return handle;
		}


		VisibilityHandle Create()
		{
			auto handle = handles.GetNewHandle();
			handles[handle] = (index_t)elements.emplace_back(handle);

			return handle;
		}


		void AddComponentView(GameObject& GO, ValueMap values, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator) override
		{
		}


		void FreeComponentView(void* _ptr) final
		{
			static_cast<View*>(_ptr)->Release();
		}


		void Remove(VisibilityHandle handle)
		{
			auto lastElement			= std::move(elements.back());
			elements[handles[handle]]	= std::move(lastElement);
			elements.pop_back();

			handles[lastElement.handle] = handles[handle];
			handles.RemoveHandle(handle);
		}


		Vector<VisibilityFields> GetElements_copy(iAllocator* tempMemory) const
		{
			Vector<VisibilityFields>	out{ tempMemory };

			for (auto& I : elements)
				out.push_back({ I.componentData });

			return out;
		}


		auto& operator[] (VisibilityHandle handle)
		{
			return elements[handles[handle]].componentData;
		}

		auto operator[] (VisibilityHandle handle) const
		{
			return elements[handles[handle]].componentData;
		}

		auto begin()
		{
			return elements.begin();
		}

		auto end()
		{
			return elements.end();
		}

		
		class View : public ComponentView_t<SceneVisibilityComponent>
		{
		public:
			View(GameObject& go, const NodeHandle node, const SceneHandle scene) :
				visibility	{ GetComponent().Create(VisibilityFields{}) }
			{
				auto& vis_ref	= GetComponent()[visibility];
				vis_ref.entity	= &go;
				vis_ref.node	= node;
				vis_ref.scene	= scene;
			}


			void Release()
			{
				GetComponent().Remove(visibility);
				visibility = InvalidHandle;
			}


			BoundingSphere GetBoundingSphere() const
			{
				return GetComponent()[visibility].boundingSphere;
			}


			void SetBoundingSphere(const BoundingSphere boundingSphere)
			{
				GetComponent()[visibility].boundingSphere = boundingSphere;
			}


			void SetVisable(bool v)
			{
				GetComponent()[visibility].visable = v;
			}


			void SetTransparency(bool t)
			{
				GetComponent()[visibility].transparent = t;
			}

			operator VisibilityHandle() { return visibility; }

			VisibilityHandle visibility;
		};

		HandleUtilities::HandleTable<VisibilityHandle>	handles;
		Vector<ElementData>								elements;
	};


	using SceneVisibilityView = SceneVisibilityComponent::View;


	/************************************************************************************************/


	AABB			GetVisibilityAABB(GameObject& go);
	void			SetBoundingSphereFromMesh(GameObject& go);
	void			SetBoundingSphereRadius(GameObject& go, const float radius);
	void			SetBoundingSphere(GameObject& go, const BoundingSphere);
	void			SetBoundingSphereFromLight(GameObject& go);
	void			SetTransparent(GameObject& go, const bool tranparent);

	AABB			GetAABBFromMesh(GameObject& go);
	BoundingSphere	GetBoundingSphereFromMesh(GameObject& go);
	BoundingSphere	GetBoundingSphere(GameObject& go);


	/************************************************************************************************/


	struct LightGather
	{
		Vector<LightHandle>	lights;
		const Scene*		scene;
	};

	struct LightShadowGather
	{
		Vector<LightHandle>	lights;
		const Scene*		scene;
	};

	struct alignas(64) SceneBVH
	{
		struct BVHElement
		{
			uint32_t			ID;
			VisibilityHandle	handle;

			friend bool operator > (const BVHElement& lhs, const BVHElement& rhs) { return lhs.ID > rhs.ID; }
			friend bool operator < (const BVHElement& lhs, const BVHElement& rhs) { return lhs.ID < rhs.ID; }
		};


		struct BVHNode {
			AABB boundingVolume;

			uint16_t	children	= 0;
			uint8_t		count		= 0;
			bool		Leaf		= false;
		};


		SceneBVH() = default;

		SceneBVH(iAllocator& allocator) :
			elements	{ &allocator },
			nodes		{ &allocator },
			allocator	{ &allocator } {}

		SceneBVH(SceneBVH&& rhs)					= default;
		SceneBVH(const SceneBVH&)					= default;
		SceneBVH& operator = (SceneBVH&& rhs)		= default;
		SceneBVH& operator = (const SceneBVH& rhs)	= default;

		static SceneBVH Build(const Scene& scene, iAllocator& allocator);

		void Release()
		{
			allocator->release(this);
		}

		template<typename TY_BV, typename TY_FN_OnIntersection>
		void TraverseBVHNode(const BVHNode& node, const TY_BV& bv, TY_FN_OnIntersection& IntersectionHandler) const
		{
			static auto& visabilityComponent = SceneVisibilityComponent::GetComponent();

			if (!Intersects(bv, node.boundingVolume))
				return;

			if (node.Leaf)
			{
				const auto end = node.children + node.count;

				for (auto childIdx = node.children; childIdx < end; ++childIdx)
				{
					const auto	child			= elements[childIdx];
					const auto&	visibleObject	= visabilityComponent[child.handle];
					const AABB	aabb			= visibleObject.GetAABB();

					if (auto res = Intersects(bv, aabb); res)
					{
						const auto	gameObject = visibleObject.entity;
						IntersectionHandler(child.handle, res, gameObject);
					}
				}
			}
			else
			{
				const auto end = node.children + node.count;

				for (auto child = node.children; child < end; ++child)
					TraverseBVHNode(nodes[child], bv, IntersectionHandler);
			}
		}


		template<typename TY_BV, typename TY_FN_OnIntersection>
		void Traverse(const TY_BV& bv, TY_FN_OnIntersection IntersectionHandler) const
		{
			if(nodes.size())
				TraverseBVHNode(nodes[root], bv, IntersectionHandler);
		}

		bool Valid() const
		{
			return nodes.size();
		}

		SceneBVH Copy(iAllocator& dest) const
		{
			SceneBVH copy{ dest };

			copy.elements	= elements.Copy(dest);
			copy.nodes		= nodes.Copy(dest);
			copy.root		= root;

			return copy;
		}

		void Clear()
		{
			elements.clear();
			nodes.clear();
			root = 0;
		}

		Vector<BVHElement>		elements;
		Vector<BVHNode>			nodes;
		uint16_t				root		= 0;
		iAllocator*				allocator	= nullptr;
	};


	struct SceneBVHBuild
	{
		SceneBVH* bvh;
	};


	struct LightUpdate_DATA
	{
		Vector<LightHandle> dirtyList;
	};

	using LightGatherTask			= UpdateTaskTyped<LightGather>;
	using LightShadowGatherTask		= UpdateTaskTyped<LightShadowGather>;
	using BuildBVHTask				= UpdateTaskTyped<SceneBVHBuild>;
	using PointLightUpdate			= UpdateTaskTyped<LightUpdate_DATA>;

	struct ComputeLod_RES
	{
		uint32_t    requestedLOD;
		uint32_t    recommendedLOD;
	};

	ComputeLod_RES ComputeLOD(const Brush& b, const float3 CameraPosition, const float maxZ);

	void PushPV(GameObject&, const Brush& b, PVS& pvs, const float3 CameraPosition, float maxZ = 10'000.0f);

	struct SceneRayCastResult
	{
		VisibilityHandle	visibileObject;
		float				d;
		GameObject*			gameObject	= nullptr;
	};


	template<typename ... TY_Queries>
	using QueryResultObject = decltype(FlexKit::Query(std::declval<GameObject&>(), TY_Queries{}...));

	class Scene
	{
	public:
		Scene(iAllocator* in_allocator = SystemAllocator) :
				allocator					{ in_allocator		},
				HandleTable					{ in_allocator		},
				sceneID						{ (size_t)rand()	},
				ownedGameObjects			{ in_allocator		},
				sceneEntities				{ in_allocator		} {}
				
		~Scene()
		{
			ClearScene();
		}

		void				AddGameObject	(GameObject& go, NodeHandle node);
		void				AddGameObject	(GameObject& go);
		void				RemoveEntity	(GameObject& go);

		void				ClearScene();

		Vector<LightHandle>	FindLights(const Frustum& f, iAllocator* tempMemory) const;

		BuildBVHTask&				UpdateSceneBVH(UpdateDispatcher&, UpdateTask& transformDependency, iAllocator* persistent);
		LightGatherTask&			GetLights(UpdateDispatcher&, iAllocator* tempMemory) const;
		size_t						GetLightCount();


		LightShadowGatherTask&		GetVisableLights(UpdateDispatcher&, CameraHandle, BuildBVHTask&, iAllocator* tempMemory) const;
		PointLightUpdate&			UpdateLights(UpdateDispatcher&, BuildBVHTask&, LightShadowGatherTask&, iAllocator* temporaryMemory, iAllocator* persistentMemory) const;

		Vector<SceneRayCastResult>	RayCast(FlexKit::Ray v, iAllocator& allocator = SystemAllocator) const;

		template<typename ... TY_Queries>
		[[nodiscard]] auto Query(iAllocator& allocator, TY_Queries ... queries)
		{
			auto& visables = SceneVisibilityComponent::GetComponent();

			using Optional_ty = decltype(FlexKit::Query(std::declval<GameObject&>(), queries...));
			Vector<Optional_ty> results{ &allocator };

			for (auto entity : sceneEntities)
			{
				auto& gameObject = *visables[entity].entity;
				if (auto res = FlexKit::Query(gameObject, queries...); res)
					results.emplace_back(std::move(res));
			}

			return results;
		}

		template<typename ... TY_Queries>
		void Query(auto& outVector, uint32_t max, TY_Queries ... queries)
		{
			auto& visables = SceneVisibilityComponent::GetComponent();

			using Optional_ty = decltype(FlexKit::Query(std::declval<GameObject&>(), queries...));
			Vector<Optional_ty> results{ &allocator };

			uint32_t count = 0;

			for (auto entity : sceneEntities)
			{
				auto& gameObject = *visables[entity].entity;
				if (auto res = FlexKit::Query(gameObject, queries...); res)
				{
					outVector.emplace_back(std::move(res));

					count++;
					if (count >= max)
						return;
				}
			}
		}

		template<typename TY_FN, typename ... TY_Queries>
		void QueryFor(TY_FN FN, const TY_Queries& ... queries)
		{
			auto& visables = SceneVisibilityComponent::GetComponent();

			for (const auto entity : sceneEntities)
			{
				auto& gameObject = *visables[entity].entity;
				if (auto res = FlexKit::Query(gameObject, queries...); res)
					FN(gameObject, res);
			}
		}

		auto begin()	{ return sceneEntities.begin(); }
		auto end()		{ return sceneEntities.end(); }

		const SceneHandle	sceneID;

		HandleUtilities::HandleTable<SceneEntityHandle> HandleTable;

		Vector<GameObject*>				ownedGameObjects;
		Vector<VisibilityHandle>		sceneEntities;
		SceneBVH						bvh;
		iAllocator*						allocator = nullptr;

		operator Scene* () { return this; }
	};


	/************************************************************************************************/


	std::optional<GameObject*> FindGameObject(Scene& scene, const char* id);


	/************************************************************************************************/


	auto FindPass(auto begin, auto end, PassHandle passID)
	{
		if (auto res = std::find_if(begin, end,
			[&](auto& pass) -> bool
			{
				return pass.pass == passID;
			}); res != end)
		{
			return &*res;
		}
		else return (decltype(&*begin))nullptr;
	}


	/************************************************************************************************/


	struct GetPVSTaskData
	{
		CameraHandle	camera;
		Scene*			scene; // Source Scene
		PVS				solid;
		PVS				transparent;

		UpdateTask*		task;
		Vector<PassPVS> passes;

		std::span<const PVEntry> GetPass(PassHandle passID) const
		{
			for (auto& pass : passes)
			{
				if (pass.pass == passID)
					return std::span<const PVEntry>{ pass.pvs };
			}

			return {};
		}

		operator UpdateTask*() { return task; }
	};


	/************************************************************************************************/


	using GatherPassesTask = UpdateTaskTyped<GetPVSTaskData>;

	FLEXKITAPI void DEBUG_ListSceneObjects(Scene& scene);


	FLEXKITAPI void UpdateScene					(Scene* SM);
	FLEXKITAPI void UpdateAnimationsScene		(Scene* SM, double dt);
	FLEXKITAPI void UpdateScenePoseTransform	(Scene* SM );
	FLEXKITAPI void UpdateShadowCasters			(Scene* SM);

	FLEXKITAPI void					GatherScene(Scene* SM, CameraHandle Camera, PVS& solid);
	FLEXKITAPI GatherPassesTask&	GatherScene(UpdateDispatcher& dispatcher, Scene* scene, CameraHandle C, iAllocator& allocator);

	FLEXKITAPI void LoadLodLevels(UpdateDispatcher& dispatcher, GatherPassesTask& PVS, CameraHandle camera, RenderSystem& renderSystem, iAllocator& allocator);

	FLEXKITAPI void ReleaseScene				(Scene* SM);
	FLEXKITAPI void BindJoint					(Scene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode);

	struct SceneLoadingContext;

	FLEXKITAPI bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, GUID_t Guid,				iAllocator* allocator, iAllocator* Temp);
	FLEXKITAPI bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, const char* LevelName,	iAllocator* allocator, iAllocator* Temp);


	/************************************************************************************************/


	FLEXKITAPI void SetVisable(GameObject&, bool);


	/************************************************************************************************/


	inline void Release(PoseState* EPS, iAllocator* allocator);


	/************************************************************************************************/


	struct ShadowCaster
	{
		LightHandle	pointLight	= InvalidHandle;
		NodeHandle			node		= InvalidHandle;
		ResourceHandle		shadowMap	= InvalidHandle;
		float4x4			matrix		= float4x4::Identity();
	};

	using ShadowMapComponent	= BasicComponent_t<ShadowCaster, LightShadowHandle, PointLightShadowMapID>;
	using ShadowMapView			= ShadowMapComponent::View;


	void EnableShadows(GameObject& gameObject);


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
