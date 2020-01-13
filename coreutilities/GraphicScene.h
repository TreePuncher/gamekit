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

#ifndef GraphicScene_H
#define GraphicScene_H

#include "..\buildsettings.h"
#include "..\coreutilities\Assets.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\AnimationUtilities.h" 
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\CoreSceneObjects.h"
#include "..\graphicsutilities\defaultpipelinestates.h"

namespace FlexKit
{
	//Forward Declarations 
	struct DrawablePoseState;

	typedef Handle_t<16, GetCRCGUID(SceneEntity)> SceneEntityHandle;
	typedef size_t SpotLightHandle;
	typedef Pair<bool, int64_t> GSPlayAnimation_RES;

	class  GraphicScene;
	struct SceneNodeComponentSystem;

	enum class QuadTreeNodeLocation: int
	{
		UpperRight,
		UpperLeft,
		LowerLeft,
		LowerRight
	};

	const float MinNodeSize = 1;


	/************************************************************************************************/


	constexpr ComponentID DrawableComponentID	= GetTypeGUID(DrawableID);
	using DrawableHandle						= Handle_t <32, DrawableComponentID>;

    struct DrawableComponentEventHandler
    {
        DrawableComponentEventHandler(RenderSystem& IN_renderSystem) : renderSystem{ IN_renderSystem }{}

        void OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);

        RenderSystem& renderSystem;
    };

	using DrawableComponent = BasicComponent_t<Drawable, DrawableHandle, DrawableComponentID, DrawableComponentEventHandler>;


	class DrawableView : public ComponentView_t<DrawableComponent>
	{
	public:
		DrawableView(TriMeshHandle	triMesh, NodeHandle node)
		{
			GetComponent()[drawable].MeshHandle = triMesh;
			GetComponent()[drawable].Node		= node;
		}

		TriMeshHandle GetTriMesh()
		{
			return GetComponent()[drawable].MeshHandle;
		}

        Drawable& GetDrawable()
        {
            return GetComponent()[drawable];
        }

		operator Drawable& ()
		{
			return GetComponent()[drawable];
		}

		BoundingSphere GetBoundingSphere()
		{
			auto meshHandle = GetComponent()[drawable].MeshHandle;
			auto mesh		= GetMeshResource(meshHandle);

			return mesh->BS;
		}

		DrawableHandle	drawable = GetComponent().Create(Drawable{});
	};


	TriMeshHandle GetTriMesh(GameObject& go)
	{
		return Apply(go,
			[&](DrawableView& drawable)
			{
				return drawable.GetTriMesh();
			}, 
			[]() -> TriMeshHandle
			{
				return TriMeshHandle{ InvalidHandle_t };
			});
	}


    /************************************************************************************************/


    void SetMaterialParams(GameObject& go, float3 albedo, float kS, float IOR, float anisotropic, float roughness, float metallic)
    {
        return Apply(go,
            [&](DrawableView& drawable)
            {
                drawable.GetDrawable().MatProperties.albedo         = albedo;
                drawable.GetDrawable().MatProperties.kS             = kS;
                drawable.GetDrawable().MatProperties.IOR            = IOR;
                drawable.GetDrawable().MatProperties.anisotropic    = anisotropic;
                drawable.GetDrawable().MatProperties.roughness      = roughness;
                drawable.GetDrawable().MatProperties.metallic       = metallic;
            });
    }

	/************************************************************************************************/


    class PointLightEventHandler
    {
    public:
        static void OnCreateView(GameObject& gameObject, const std::byte* buffer, const size_t bufferSize, iAllocator* allocator);
    };

	constexpr ComponentID PointLightComponentID	= GetTypeGUID(PointLightID);
	using PointLightHandle						= Handle_t <32, GetTypeGUID(PointLightID)>;
	using PointLightComponent					= BasicComponent_t<PointLight, PointLightHandle, PointLightComponentID, PointLightEventHandler>;


	class PointLightView : public ComponentView_t<PointLightComponent>
	{
	public:
		PointLightView(float3 color, float intensity, NodeHandle node) : light{ GetComponent().Create({}) } 
		{
			auto& poingLight        = GetComponent()[light];
			poingLight.K			= color;
			poingLight.I			= intensity;
			poingLight.R			= intensity;
			poingLight.Position		= node;
		}

		float GetRadius()
		{
			return GetComponent()[light].I;
		}

		void SetNode(NodeHandle node)
		{
			GetComponent()[light].Position = node;
		}


		operator PointLightHandle () { return light; }

		PointLightHandle	light;
	};


	/************************************************************************************************/


	constexpr ComponentID SceneVisibilityComponentID	= GetTypeGUID(SceneVisibilityComponentID);
	using VisibilityHandle								= Handle_t <32, GetTypeGUID(DrawableID)>;
	using SceneHandle									= Handle_t <32, GetTypeGUID(SceneID)>;

	struct VisibilityFields
	{
		GameObject*		entity		= nullptr;	// to get parents in raycast results
		SceneHandle		scene		= InvalidHandle_t;
		NodeHandle		node		= InvalidHandle_t;

		bool			visable		= true;
		bool			rayVisible	= false;
		bool			transparent = false;

		BoundingSphere	boundingSphere = { 0, 0, 0, 0 }; // model space
	};

	using SceneVisibilityComponent = BasicComponent_t<VisibilityFields, VisibilityHandle, SceneVisibilityComponentID>;

	class SceneVisibilityView : public ComponentView_t<SceneVisibilityComponent>
	{
	public:
		SceneVisibilityView(GameObject& go, NodeHandle node, SceneHandle scene) :
			visibility	{ GetComponent().Create(VisibilityFields{}) }
		{
			auto& vis_ref	= GetComponent()[visibility];
			vis_ref.entity	= &go;
			vis_ref.node	= node;
			vis_ref.scene	= scene;
		}


		void SetBoundingSphere(BoundingSphere boundingSphere)
		{
			GetComponent()[visibility].boundingSphere = boundingSphere;
		}

        void SetVisable(bool v)
        {
            GetComponent()[visibility].visable = v;
        }

		operator VisibilityHandle() { return visibility; }

		VisibilityHandle visibility;
	};


	/************************************************************************************************/


	inline void SetBoundingSphereFromMesh(GameObject& go)
	{
		Apply(
			go,
			[](	SceneVisibilityView&	visibility,
				DrawableView&			drawable)
			{
				visibility.SetBoundingSphere(drawable.GetBoundingSphere());
			});
	}


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

	/************************************************************************************************/


	struct QuadTreeNode
	{
		QuadTreeNode(iAllocator* in_allocator) : 
			allocator{ in_allocator } {}

		~QuadTreeNode()
		{

		}

		void Clear()
		{
			for (auto child : ChildNodes) 
			{
				child->Clear();
				child->Contents.clear();
				allocator->free(child);
			}

			ChildNodes.clear();
			Contents.clear();
		}

		void AddEntity		(VisibilityHandle visable);
		void RemoveEntity	(VisibilityHandle visable);
		
		enum SphereTestRes
		{
			Partially	= 2,
			Fully		= 3, 
			Failed		= 0
		};

		SphereTestRes IsSphereInside(float r, float3 pos);

		// requires SceneVisibilityComponent
		void UpdateBounds();
		void ExpandNode(iAllocator* allocator);

		float4								GetArea() const;

		float2								lowerLeft;
		float2								upperRight;
		float2								centerPoint;

		static_vector<VisibilityHandle, 4>	Contents;
		static_vector<QuadTreeNode*,4>		ChildNodes;

		iAllocator*							allocator;
	};


	struct QuadTree
	{
		QuadTree(SceneHandle in_scene, float2 AreaDimensions, iAllocator* in_allocator) :
			allocator	{ in_allocator	},
			scene		{ in_scene		},
			root		{ in_allocator	}
		{
			area					= float2{0, 0};

			root.upperRight = AreaDimensions;
			root.lowerLeft	= AreaDimensions * -1;
		}

		void clear();
		
		void AddEntity		(VisibilityHandle handle);
		void RemoveEntity	(VisibilityHandle handle);

		void Rebuild		(GraphicScene& parentScene);

		float4		GetArea();

		UpdateTask& Update(FlexKit::UpdateDispatcher& dispatcher, GraphicScene* parentScene, UpdateTask& transformDependency);

		const SceneHandle			scene;
		const size_t				NodeSize = 16;


		size_t						RebuildPeriod	= 10;
		size_t						RebuildCounter	= 10;

		float2						area;
		QuadTreeNode				root;
		iAllocator*					allocator;
	};


	void UpdateQuadTree		( QuadTreeNode* Node, GraphicScene* Scene );

	
	/************************************************************************************************/

	struct PointLightGather
	{
		Vector<PointLightHandle>	pointLights;
		GraphicScene*				scene;
		StackAllocator				temp;
	};


    using PointLightGatherTask = UpdateTaskTyped<PointLightGather>;

	class GraphicScene
	{
	public:
		GraphicScene(iAllocator*		in_allocator ) :
				allocator					{ in_allocator							},
				HandleTable					{ in_allocator							},
				sceneID						{ rand()								},
				sceneManagement				{ sceneID, {1024, 1024}, in_allocator	},
				sceneEntities				{ in_allocator							} {}
				
		~GraphicScene()
		{
			ClearScene();
		}

		void				AddGameObject	(GameObject& go, NodeHandle node);
		void				RemoveEntity	(GameObject& go);

		void				ClearScene			();

		Drawable&	        SetNode(SceneEntityHandle EHandle, NodeHandle Node);

		Vector<PointLightHandle>    FindPointLights(const Frustum& f, iAllocator* tempMemory) const;


        PointLightGatherTask&	    GetPointLights(UpdateDispatcher& disatcher, iAllocator* tempMemory);
		size_t					    GetPointLightCount();

        auto begin()    { return sceneEntities.begin(); }
        auto end()      { return sceneEntities.end(); }

		const SceneHandle					sceneID;

		HandleUtilities::HandleTable<SceneEntityHandle, 16> HandleTable;
			
		Vector<VisibilityHandle>			sceneEntities;
		QuadTree							sceneManagement;
		iAllocator*							allocator;

		operator GraphicScene* () { return this; }
	};


    /************************************************************************************************/


    std::pair<GameObject*, bool> FindGameObject(GraphicScene& scene, const char* id);


	/************************************************************************************************/


	struct GetPVSTaskData
	{
		CameraHandle	camera;
		GraphicScene*	scene; // Source Scene
		StackAllocator	taskMemory;
		PVS				solid;
		PVS				transparent;

		UpdateTask*		task;

		operator UpdateTask*() { return task; }
	};


	/************************************************************************************************/


    using GatherTask = UpdateTaskTyped<GetPVSTaskData>;

    FLEXKITAPI void DEBUG_ListSceneObjects(GraphicScene& scene);


	FLEXKITAPI void UpdateGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void UpdateAnimationsGraphicScene	(GraphicScene* SM, double dt);
	FLEXKITAPI void UpdateGraphicScenePoseTransform	(GraphicScene* SM );
	FLEXKITAPI void UpdateShadowCasters				(GraphicScene* SM);

	FLEXKITAPI void	        GatherScene(GraphicScene* SM, CameraHandle C, PVS* __restrict out, PVS* __restrict T_out);
    FLEXKITAPI GatherTask&  GatherScene(UpdateDispatcher& dispatcher, GraphicScene* scene, CameraHandle C, iAllocator* allocator);


	FLEXKITAPI void ReleaseGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void BindJoint						(GraphicScene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode);

	FLEXKITAPI bool LoadScene(RenderSystem* RS, GUID_t Guid,			GraphicScene& GS_out, iAllocator* allocator, iAllocator* Temp);
	FLEXKITAPI bool LoadScene(RenderSystem* RS, const char* LevelName,	GraphicScene& GS_out, iAllocator* allocator, iAllocator* Temp);




    /************************************************************************************************/


    FLEXKITAPI void SetVisable(GameObject&, bool);


	/************************************************************************************************/


	inline void Release(DrawablePoseState* EPS, iAllocator* allocator);


	/************************************************************************************************/


	inline void DEBUG_DrawQuadTree(
		GraphicScene&			scene, 
		UpdateDispatcher&		dispatcher,
		UpdateTask&				sceneUpdate,
		FrameGraph&				graph, 
		CameraHandle			camera,
		VertexBufferHandle		vertexBuffer, 
		ConstantBufferHandle	constantBuffer,
		ResourceHandle			renderTarget,
		iAllocator*				tempMemory)
	{
		Vector<FlexKit::Rectangle> rects{ tempMemory };

		const float4 colors[] =
		{
			{0, 0, 1, 1},
			{0, 1, 1, 1},
			{1, 1, 0, 1},
			{1, 0, 0, 1}
		};

		const auto	area		= scene.sceneManagement.GetArea();
		const auto	areaLL		= (area.y <  area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
		const auto	areaUR		= (area.y >= area.z) ? float2{ area.x, area.y } : float2{ area.z, area.w };
		const auto	areaSpan	= (areaUR - areaLL);
		int			itr			= 0;

		auto drawNode = [&](auto& self, const QuadTreeNode& Node, const int depth) -> void
		{
			auto nodeArea = Node.GetArea();

			//const auto ll = (float2{ nodeArea.x, nodeArea.y } - areaLL) / areaSpan;
			//const auto ur = (float2{ nodeArea.z, nodeArea.w } - areaLL) / areaSpan;

			const auto ll = float2{nodeArea.x, nodeArea.y};// lower left
			const auto ur = float2{nodeArea.z, nodeArea.w};// upper right

			FlexKit::Rectangle rect;
			rect.Color		= colors[depth % 4];
			rect.Position	= ll;
			rect.WH			= ur - ll;
			rects.push_back(rect);

			for (auto& child : Node.ChildNodes)
				self(self, *child, depth + 1 + itr++);
		};

		drawNode(drawNode, scene.sceneManagement.root, 0);

		DrawWireframeRectangle_Desc drawDesc =
		{
			renderTarget,
			vertexBuffer,
			constantBuffer,
			camera,
			DRAW_LINE3D_PSO
		};

		WireframeRectangleList(
			graph,
			drawDesc,
			rects,
			tempMemory);
	}


    /************************************************************************************************/


    struct _PointLightShadowCaster
    {
        PointLightHandle    pointLight;
        NodeHandle          node;
    };


    constexpr ComponentID PointLightShadowCasterID = GetTypeGUID(PointLighShadowCaster);

    using PointLightShadowCasterHandle  = Handle_t<32, PointLightShadowCasterID>;
    using PointLightShadowCaster        = BasicComponent_t<_PointLightShadowCaster, PointLightShadowCasterHandle, PointLightShadowCasterID>;
    using PointLightShadowCasterView    = PointLightShadowCaster::View;


}	/************************************************************************************************/

#endif
