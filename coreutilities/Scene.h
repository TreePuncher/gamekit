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

#ifndef Scene_H
#define Scene_H

#include "buildsettings.h"
#include "Assets.h"
#include "GraphicsComponents.h"
#include "Materials.h"

#include "AnimationUtilities.h" 
#include "graphics.h"
#include "CoreSceneObjects.h"
#include "defaultpipelinestates.h"

#include "FrameGraph.h"

namespace FlexKit
{
    // IDs
    constexpr ComponentID BrushComponentID          = GetTypeGUID(Brush);
    constexpr ComponentID PointLightShadowMapID     = GetTypeGUID(PointLighShadowCaster);

    // Handles
    using BrushHandle               = Handle_t<32, BrushComponentID>;
    using PointLightHandle          = Handle_t<32, GetTypeGUID(PointLight)>;
    using PointLightShadowHandle    = Handle_t<32, PointLightShadowMapID>;
    using SceneHandle               = Handle_t<32, GetTypeGUID(SceneID)>;
    using VisibilityHandle          = Handle_t<32, GetTypeGUID(BrushID)>;


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

        void OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);

        RenderSystem& renderSystem;
    };

	using BrushComponent = BasicComponent_t<Brush, BrushHandle, BrushComponentID, BrushComponentEventHandler>;

	class BrushView : public ComponentView_t<BrushComponent>
	{
	public:
		BrushView(GameObject& gameObject, TriMeshHandle	triMesh, NodeHandle node)
		{
			GetComponent()[brush].MeshHandle    = triMesh;
			GetComponent()[brush].Node		    = node;
		}

		TriMeshHandle GetTriMesh()
		{
			return GetComponent()[brush].MeshHandle;
		}

        Brush& GetBrush()
        {
            return GetComponent()[brush];
        }

		operator Brush& ()
		{
			return GetComponent()[brush];
		}

		BoundingSphere GetBoundingSphere()
		{
			auto meshHandle = GetComponent()[brush].MeshHandle;
			auto mesh		= GetMeshResource(meshHandle);

			return mesh->BS;
		}

        void SetTransparent(const bool transparent)
        {
            GetComponent()[brush].Transparent = transparent;
        }

        bool GetTransparent() const
        {
            return GetComponent()[brush].Transparent;
        }


		BrushHandle	brush = GetComponent().Create(Brush{});
	};


    /************************************************************************************************/


    TriMeshHandle GetTriMesh(GameObject& go);

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
            allocator       { IN_allocator },
            visableObjects  { IN_allocator } {}

        void Release()
        {
            visableObjects.Release();
            allocator->free(this);
        }

        uint                        shadowMapSize = 1;
        Vector<VisibilityHandle>    visableObjects;
        iAllocator*                 allocator;
    };


    struct PointLight
    {
        PointLight() = default;

        ~PointLight()
        {
            if (shadowState)
                shadowState->Release();
        }

        PointLight(PointLight&& rhs)
        {
            K = rhs.K;
            I = rhs.I;
            R = rhs.R;

            Position    = rhs.Position;
            shadowMap   = rhs.shadowMap;

            forceDisableShadowMapping   = rhs.forceDisableShadowMapping;
            state                       = rhs.state;
            shadowState                 = rhs.shadowState;

            rhs.shadowMap       = InvalidHandle_t;
            rhs.shadowState     = nullptr;
        }

        PointLight& operator = (PointLight&& rhs)
        {
            K = rhs.K;
            I = rhs.I;
            R = rhs.R;

            Position    = rhs.Position;
            shadowMap   = rhs.shadowMap;

            forceDisableShadowMapping   = rhs.forceDisableShadowMapping;
            state                       = rhs.state;
            shadowState                 = rhs.shadowState;

            rhs.shadowMap       = InvalidHandle_t;
            rhs.shadowState     = nullptr;

            return *this;
        }

        float3 K;
        float I, R;

        NodeHandle      Position;
        ResourceHandle  shadowMap = InvalidHandle_t;

        bool                forceDisableShadowMapping = false;
        LightStateFlags     state;
        CubeMapState*       shadowState = nullptr;
    };


    class PointLightEventHandler
    {
    public:
        static void OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
    };

	constexpr ComponentID PointLightComponentID	= GetTypeGUID(PointLight);
	using PointLightComponent					= BasicComponent_t<PointLight, PointLightHandle, PointLightComponentID, PointLightEventHandler>;


	class PointLightView : public ComponentView_t<PointLightComponent>
	{
	public:
		PointLightView(GameObject& gameObject, float3 color, float intensity, float radius, NodeHandle node) : light{ GetComponent().Create() }
		{
			auto& poingLight        = GetComponent()[light];
			poingLight.K			= color;
			poingLight.I			= intensity;
			poingLight.R			= radius;
			poingLight.Position		= node;
		}

		float GetRadius() const noexcept
		{
			return GetComponent()[light].R;
		}

        void SetRadius(float r) noexcept
        {
            GetComponent()[light].R = r;
        }

        float GetIntensity()
        {
            return GetComponent()[light].I;
        }

        void SetIntensity(float I)
        {
            GetComponent()[light].I = I;
        }

		void SetNode(NodeHandle node) const noexcept
		{
			GetComponent()[light].Position = node;
		}

        NodeHandle GetNode() const noexcept
        {
            return GetComponent()[light].Position;
        }


		operator PointLightHandle () { return light; }

		PointLightHandle	light;
	};


    inline PointLightHandle GetPointLight(GameObject& go)
    {
        return Apply(
            go,
            [](PointLightView& pointLight) -> PointLightHandle
            {
                return pointLight;
            },
            []() -> PointLightHandle
            {
                return InvalidHandle_t;
            });
    }


	/************************************************************************************************/


	constexpr ComponentID SceneVisibilityComponentID	= GetTypeGUID(SceneVisibility);

	struct VisibilityFields
	{
		GameObject*		entity		= nullptr;	// to get parents in raycast results
		SceneHandle		scene		= InvalidHandle_t;
		NodeHandle		node		= InvalidHandle_t;

		bool			visable		= true;
		bool			rayVisible	= false;
		bool			transparent = false;

		BoundingSphere	boundingSphere = { 0, 0, 0, 0 }; // model space

        AABB            GetAABB() const
        {
            const auto posW = GetPositionW(node);
            return { posW - boundingSphere.w, posW + boundingSphere.w };
        }
	};

	using SceneVisibilityComponent = BasicComponent_t<VisibilityFields, VisibilityHandle, SceneVisibilityComponentID>;

	class SceneVisibilityView : public ComponentView_t<SceneVisibilityComponent>
	{
	public:
		SceneVisibilityView(GameObject& go, const NodeHandle node, const SceneHandle scene) :
			visibility	{ GetComponent().Create(VisibilityFields{}) }
		{
			auto& vis_ref	= GetComponent()[visibility];
			vis_ref.entity	= &go;
			vis_ref.node	= node;
			vis_ref.scene	= scene;
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


	/************************************************************************************************/


	inline void SetBoundingSphereFromMesh(GameObject& go)
	{
        const auto Scale = GetScale(go);

        float rScale = 1;
        for (size_t I = 0; I < 3; ++I)
            rScale = Max(rScale, Scale[I]);

		Apply(
			go,
			[&]( SceneVisibilityView&	visibility,
			    BrushView&			    brush)
			{
                auto boundingSphere = brush.GetBoundingSphere();
                boundingSphere.w   *= rScale;

				visibility.SetBoundingSphere(boundingSphere);
			});
	}


    /************************************************************************************************/


    inline void SetBoundingSphereRadius(GameObject& go, const float radius)
    {
        Apply(
            go,
            [&](SceneVisibilityView& visibility)
            {
                visibility.SetBoundingSphere(BoundingSphere{ 0, 0, 0, radius });
            });
    }


    /************************************************************************************************/


	inline void SetBoundingSphereFromLight(GameObject& go)
	{
		Apply(
			go,
			[](	SceneVisibilityView&    visibility,
				PointLightView&	        pointLight)
			{
				visibility.SetBoundingSphere({ 0, 0, 0, pointLight.GetRadius() });
			});
	}

    inline void SetTransparent(GameObject& go, const bool tranparent)
    {
        Apply(
            go,
            [&](SceneVisibilityView& visibility)
            {
                visibility.SetTransparency(tranparent);
            });
    }


    /************************************************************************************************/


    inline BoundingSphere GetBoundingSphereFromMesh(GameObject& go)
    {
        const auto Scale = GetScale(go);

        float rScale = 1;
        for (size_t I = 0; I < 3; ++I)
            rScale = Max(rScale, Scale[I]);

        return Apply(
            go,
            [&](SceneVisibilityView&    visibility,
                BrushView&              brushView)
            {
                auto boundingSphere = brushView.GetBoundingSphere();
                auto pos            = GetPositionW(brushView.GetBrush().Node);

                return BoundingSphere{ pos, boundingSphere.w * rScale };
            },
            []()
            {
                return BoundingSphere{ 0, 0, 0, 0 };
            });
    }

    inline BoundingSphere GetBoundingSphere(GameObject& go)
    {
        const auto Scale = GetScale(go);

        float rScale = 1;
        for (size_t I = 0; I < 3; ++I)
            rScale = Max(rScale, Scale[I]);

        return Apply(
            go,
            [&](SceneVisibilityView&    visibility)
            {
                return visibility.GetBoundingSphere();
            },
            []()
            {
                return BoundingSphere{ 0, 0, 0, 0 };
            });
    }


	/************************************************************************************************/


	struct PointLightGather
	{
		Vector<PointLightHandle>	pointLights;
		const Scene*			    scene;
	};

    struct PointLightShadowGather
    {
        Vector<PointLightHandle>	pointLightShadows;
        const Scene*                scene;
    };

    struct alignas(64) SceneBVH
    {
        struct BVHElement
        {
            uint32_t            ID;
            VisibilityHandle    handle;

            friend bool operator > (const BVHElement& lhs, const BVHElement& rhs)
            {
                return lhs.ID > rhs.ID;
            }

            friend bool operator < (const BVHElement& lhs, const BVHElement& rhs)
            {
                return lhs.ID < rhs.ID;
            }
        };


        struct BVHNode {
            AABB boundingVolume;

            uint16_t    children    = 0;
            uint8_t     count       = 0;
            bool        Leaf        = false;
        };


        SceneBVH() = default;

        SceneBVH(iAllocator& allocator) :
            elements    { &allocator },
            nodes       { &allocator },
            allocator   { &allocator } {}

        SceneBVH(SceneBVH&& rhs)                    = default;
        SceneBVH(const SceneBVH& )                  = default;
        SceneBVH& operator = (SceneBVH&& rhs)       = default;
        SceneBVH& operator = (const SceneBVH& rhs)  = default;

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
                    const auto child    = elements[childIdx];
                    const auto aabb     = visabilityComponent[child.handle].GetAABB();

                    if (auto res = Intersects(bv, aabb); res)
                        IntersectionHandler(child.handle, res);
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

            copy.elements   = elements.Copy(dest);
            copy.nodes      = nodes.Copy(dest);
            copy.root       = root;

            return copy;
        }

        Vector<BVHElement>    elements;
        Vector<BVHNode>       nodes;
        uint16_t              root      = 0;
        iAllocator*           allocator = nullptr;
    };


    struct SceneBVHBuild
    {
        SceneBVH* bvh;
    };


    struct PointLightUpdate_DATA
    {
        Vector<PointLightHandle> dirtyList;
    };

    using PointLightGatherTask          = UpdateTaskTyped<PointLightGather>;
    using PointLightShadowGatherTask    = UpdateTaskTyped<PointLightShadowGather>;
    using BuildBVHTask                  = UpdateTaskTyped<SceneBVHBuild>;
    using PointLightUpdate              = UpdateTaskTyped<PointLightUpdate_DATA>;

    struct ComputeLod_RES
    {
        uint32_t    requestedLOD;
        uint32_t    recommendedLOD;
    };

    ComputeLod_RES ComputeLOD(const Brush& e, const float3 CameraPosition, const float maxZ);

    void PushPV(const Brush& e, PVS& pvs, const float3 CameraPosition, float maxZ = 10'000.0f);

    struct SceneRayCastResult
    {
        VisibilityHandle    visibileObject;
        float               d;
    };


	class Scene
	{
	public:
		Scene(iAllocator* in_allocator = SystemAllocator) :
				allocator					{ in_allocator	},
				HandleTable					{ in_allocator	},
				sceneID						{ rand()		},
                ownedGameObjects            { in_allocator  },
				sceneEntities				{ in_allocator	} {}
				
		~Scene()
		{
			ClearScene();
		}

		void				AddGameObject	(GameObject& go, NodeHandle node);
		void				RemoveEntity	(GameObject& go);

		void				ClearScene			();

		Vector<PointLightHandle>    FindPointLights(const Frustum& f, iAllocator* tempMemory) const;

        BuildBVHTask&               UpdateSceneBVH(UpdateDispatcher&, UpdateTask& transformDependency, iAllocator* persistent);
        PointLightGatherTask&	    GetPointLights(UpdateDispatcher&, iAllocator* tempMemory) const;
		size_t					    GetPointLightCount();


        PointLightShadowGatherTask& GetVisableLights(UpdateDispatcher&, CameraHandle, BuildBVHTask&, iAllocator* tempMemory) const;
        PointLightUpdate&           UpdatePointLights(UpdateDispatcher&, BuildBVHTask&, PointLightShadowGatherTask&, iAllocator* temporaryMemory, iAllocator* persistentMemory) const;

        Vector<SceneRayCastResult>    RayCast(FlexKit::Ray v, iAllocator& allocator) const;

        auto begin()    { return sceneEntities.begin(); }
        auto end()      { return sceneEntities.end(); }

		const SceneHandle					sceneID;

		HandleUtilities::HandleTable<SceneEntityHandle> HandleTable;

        Vector<GameObject*>		        ownedGameObjects;
		Vector<VisibilityHandle>		sceneEntities;
        SceneBVH                        bvh;
		iAllocator*						allocator       = nullptr;

		operator Scene* () { return this; }
	};


    /************************************************************************************************/


    std::pair<GameObject*, bool> FindGameObject(Scene& scene, const char* id);


	/************************************************************************************************/


    struct PassPVS
    {
        PassHandle  pass;
        PVS         pvs;
    };

    

	struct GetPVSTaskData
	{
		CameraHandle	camera;
		Scene*	        scene; // Source Scene
		PVS				solid;
		PVS				transparent;

		UpdateTask*		task;
        Vector<PassPVS> passes;

		operator UpdateTask*() { return task; }
	};


	/************************************************************************************************/


    using GatherPassesTask = UpdateTaskTyped<GetPVSTaskData>;

    FLEXKITAPI void DEBUG_ListSceneObjects(Scene& scene);


	FLEXKITAPI void UpdateScene				    (Scene* SM);
	FLEXKITAPI void UpdateAnimationsScene	    (Scene* SM, double dt);
	FLEXKITAPI void UpdateScenePoseTransform	(Scene* SM );
	FLEXKITAPI void UpdateShadowCasters			(Scene* SM);

    FLEXKITAPI void                 GatherScene(Scene* SM, CameraHandle Camera, PVS& solid);
    FLEXKITAPI GatherPassesTask&    GatherScene(UpdateDispatcher& dispatcher, Scene* scene, CameraHandle C, iAllocator& allocator);

    FLEXKITAPI void LoadLodLevels(UpdateDispatcher& dispatcher, GatherPassesTask& PVS, CameraHandle camera, RenderSystem& renderSystem, iAllocator& allocator);

	FLEXKITAPI void ReleaseScene				(Scene* SM);
	FLEXKITAPI void BindJoint					(Scene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode);

	FLEXKITAPI bool LoadScene(RenderSystem* RS, GUID_t Guid,			Scene& GS_out, iAllocator* allocator, iAllocator* Temp);
	FLEXKITAPI bool LoadScene(RenderSystem* RS, const char* LevelName,	Scene& GS_out, iAllocator* allocator, iAllocator* Temp);


    /************************************************************************************************/


    FLEXKITAPI void SetVisable(GameObject&, bool);


	/************************************************************************************************/


	inline void Release(PoseState* EPS, iAllocator* allocator);


    /************************************************************************************************/


    struct _PointLightShadowCaster
    {
        PointLightHandle    pointLight  = InvalidHandle_t;
        NodeHandle          node        = InvalidHandle_t;
        ResourceHandle      shadowMap   = InvalidHandle_t;
        float4x4            matrix      = float4x4::Identity();
    };

    using PointLightShadowMap       = BasicComponent_t<_PointLightShadowCaster, PointLightShadowHandle, PointLightShadowMapID>;
    using PointLightShadowMapView   = PointLightShadowMap::View;


    inline void EnablePointLightShadows(GameObject& gameObject)
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

#endif
