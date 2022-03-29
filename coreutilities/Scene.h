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
#include "RuntimeComponentIDs.h"

#include "FrameGraph.h"

namespace FlexKit
{
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

        void OnCreateView(GameObject& gameObject, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);

        RenderSystem& renderSystem;
    };

	using BrushComponent = BasicComponent_t<Brush, BrushHandle, BrushComponentID, BrushComponentEventHandler>;

	class BrushView : public ComponentView_t<BrushComponent>
	{
	public:
		BrushView(GameObject& gameObject, TriMeshHandle	triMesh)
		{
            auto node = GetSceneNode(gameObject);

			GetComponent()[brush].MeshHandle    = triMesh;
			GetComponent()[brush].Node		    = node != InvalidHandle_t ? node : GetZeroedNode();
		}

        ~BrushView()
        {
            auto& brush_ref = GetComponent()[brush];
            ReleaseMesh(brush_ref.MeshHandle);
            ReleaseMesh(brush_ref.Occluder);

            GetComponent().Remove(brush);
        }

		TriMeshHandle GetTriMesh() noexcept
		{
			return GetComponent()[brush].MeshHandle;
		}

        Brush& GetBrush() noexcept
        {
            return GetComponent()[brush];
        }

        MaterialHandle GetMaterial() noexcept
        {
            return GetComponent()[brush].material;
        }

        void SetMaterial(MaterialHandle material) noexcept
        {
            GetComponent()[brush].material = material;
        }

		operator Brush& () noexcept
		{
			return GetComponent()[brush];
		}

		BoundingSphere GetBoundingSphere() noexcept
		{
			auto meshHandle = GetComponent()[brush].MeshHandle;

            if (meshHandle == InvalidHandle_t)
                return {};
            else
                return GetMeshResource(meshHandle)->BS;
		}

        void SetTransparent(const bool transparent) noexcept
        {
            GetComponent()[brush].Transparent = transparent;
        }

        void SetMesh(TriMeshHandle mesh) noexcept
        {
            auto& currentMesh = GetComponent()[brush].MeshHandle;

            if (mesh == currentMesh)
                return;

            ReleaseMesh(currentMesh);

            AddRef(mesh);
            currentMesh = mesh;
        }

        bool GetTransparent() const noexcept
        {
            return GetComponent()[brush].Transparent;
        }


		BrushHandle	brush = GetComponent().Create(Brush{});
	};


    /************************************************************************************************/


    TriMeshHandle   GetTriMesh(GameObject& go) noexcept;
    Brush*          GetBrush(GameObject& go) noexcept;

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
        ~PointLight();

        PointLight(PointLight&& rhs);

        PointLight& operator = (PointLight&& rhs);
        PointLight& operator = (const PointLight& rhs) = delete;

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
        static void OnCreateView(GameObject& gameObject, void* user_ptr, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
    };

	using PointLightComponent					= BasicComponent_t<PointLight, PointLightHandle, PointLightComponentID, PointLightEventHandler>;

	class PointLightView : public ComponentView_t<PointLightComponent>
	{
	public:
        PointLightView(GameObject& gameObject, float3 color = { 1, 1, 1 }, float intensity = 100, float radius = 100, NodeHandle node = InvalidHandle_t);


        float3      GetK();
        float       GetIntensity();
        NodeHandle  GetNode() const noexcept;
        float       GetRadius() const noexcept;

        void        SetK            (float3 color);
        void        SetIntensity    (float I);
        void        SetNode         (NodeHandle node) const noexcept;
        void        SetRadius       (float r) noexcept;

		operator PointLightHandle () { return light; }

		PointLightHandle	light;
	};


    PointLightHandle GetPointLight(GameObject& go);


	/************************************************************************************************/


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


        ~SceneVisibilityView() override
        {
            GetComponent().Remove(visibility);
            visibility = InvalidHandle_t;
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


    void             SetBoundingSphereFromMesh(GameObject& go);
    void             SetBoundingSphereRadius(GameObject& go, const float radius);
    void             SetBoundingSphereFromLight(GameObject& go);
    void             SetTransparent(GameObject& go, const bool tranparent);
    BoundingSphere   GetBoundingSphereFromMesh(GameObject& go);
    BoundingSphere   GetBoundingSphere(GameObject& go);


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

        void Clear()
        {
            elements.clear();
            nodes.clear();
            root = 0;
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
				allocator					{ in_allocator	    },
				HandleTable					{ in_allocator	    },
				sceneID						{ (size_t)rand()    },
                ownedGameObjects            { in_allocator      },
				sceneEntities				{ in_allocator	    } {}
				
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

        Vector<SceneRayCastResult>    RayCast(FlexKit::Ray v, iAllocator& allocator = SystemAllocator) const;

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


    std::optional<GameObject*> FindGameObject(Scene& scene, const char* id);


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

    struct SceneLoadingContext;

	FLEXKITAPI bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, GUID_t Guid,              iAllocator* allocator, iAllocator* Temp);
	FLEXKITAPI bool LoadScene(RenderSystem* RS, SceneLoadingContext& ctx, const char* LevelName,	iAllocator* allocator, iAllocator* Temp);


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


    void EnablePointLightShadows(GameObject& gameObject);


}	/************************************************************************************************/

#endif
