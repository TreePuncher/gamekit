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

#ifndef GraphicScene_H
#define GraphicScene_H

#include "..\buildsettings.h"
#include "..\coreutilities\Resources.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\graphicsutilities\AnimationUtilities.h" 
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\CoreSceneObjects.h"

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

	struct QuadTreeNode
	{
		void Clear(iAllocator* memory)
		{
			for (auto child : ChildNodes) 
			{
				child->Clear(memory);
				child->Contents.clear();
				memory->free(child);
			}

			ChildNodes.clear();
			Contents.clear();
		}

		void AddChild(SceneEntityHandle, GraphicScene& parentScene, iAllocator* memory);
		
		enum SphereTestRes
		{
			Partially	= 2,
			Fully		= 3, 
			Failed		= 0
		};

		SphereTestRes IsSphereInside(float r, float3 pos);

		void UpdateBounds(GraphicScene& parentScene);
		void ExpandNode(iAllocator* allocator);

		float4								GetArea() const;

		float2								lowerLeft;
		float2								upperRight;
		float2								centerPoint;

		static_vector<SceneEntityHandle, 4>	Contents;
		static_vector<QuadTreeNode*,4>		ChildNodes;
	};


	struct QuadTree
	{
		void Initiate(float2 AreaDimensions, iAllocator* memory);
		void Release();
		
		void CreateNode		(SceneEntityHandle Handle, GraphicScene& parentScene);
		void ReleaseNode	(SceneEntityHandle Handle);

		void Rebuild		(GraphicScene& parentScene);

		float4		GetArea();

		UpdateTask* Update(FlexKit::UpdateDispatcher& dispatcher, GraphicScene* parentScene, UpdateTask* transformDependency);

		const size_t NodeSize = 16;


		size_t						RebuildPeriod	= 10;
		size_t						RebuildCounter	= 10;

		float2						area;
		QuadTreeNode				root;
		Vector<SceneEntityHandle>	allEntities;
		iAllocator*					allocator;

		
	};


	void UpdateQuadTree		( QuadTreeNode* Node, GraphicScene* Scene );


	
	/************************************************************************************************/


	class GraphicScene
	{
	public:
		GraphicScene(
			RenderSystem*	IN_RS,
			iAllocator*		memory, 
			iAllocator*		tempmemory) :
				Memory						{memory},
				TempMemory					{tempmemory},
				HandleTable					{memory},
				DrawableHandles				{memory},
				Drawables					{memory},
				DrawableVisibility			{memory},
				DrawableRayVisibility		{memory},
				//SpotLightCasters			{memory},
				TaggedJoints				{memory},
				RS							{IN_RS},
				_PVS						{tempmemory}
		{
			using FlexKit::CreateSpotLightList;
			using FlexKit::CreatePointLightList;
			using FlexKit::PointLightListDesc;

			FlexKit::PointLightListDesc Desc;
			Desc.MaxLightCount = 512;

			CreatePointLightList(&PLights, Desc, Memory);
			CreateSpotLightList	(&SPLights, Memory);

			SceneManagement.Initiate({1024, 1024}, Memory);
		}

		~GraphicScene()
		{
			ClearScene();
		}

		SceneEntityHandle CreateDrawable	(TriMeshHandle mesh = InvalidHandle_t, NodeHandle node = InvalidHandle_t);
		void		 RemoveEntity	(SceneEntityHandle E );
		void		 ClearScene		();

		Drawable&		GetDrawable			(SceneEntityHandle EHandle );
		BoundingSphere	GetBoundingSphere	(SceneEntityHandle EHandle);

		Skeleton*	GetSkeleton			(SceneEntityHandle EHandle ) { return FlexKit::GetSkeleton(Drawables.at(HandleTable[EHandle]).MeshHandle);		}
		float4		GetMaterialColour	(SceneEntityHandle EHandle ) { return Drawables.at(HandleTable[EHandle]).MatProperties.Albedo;					}
		float4		GetMaterialSpec		(SceneEntityHandle EHandle ) { return Drawables.at(HandleTable[EHandle]).MatProperties.Spec;					}
		NodeHandle	GetNode				(SceneEntityHandle EHandle ) { return Drawables.at(HandleTable[EHandle]).Node;									}
		float3		GetEntityPosition	(SceneEntityHandle EHandle );

		bool isEntitySkeletonAvailable			(SceneEntityHandle EHandle );
		bool EntityEnablePosing					(SceneEntityHandle EHandle );

		inline DrawableAnimationState*	GetEntityAnimationState(SceneEntityHandle EHandle) { return Drawables.at(EHandle).AnimationState; }

		GSPlayAnimation_RES EntityPlayAnimation	(SceneEntityHandle EHandle, const char*	Animation,	float w = 1.0f, bool Loop = false );
		GSPlayAnimation_RES EntityPlayAnimation	(SceneEntityHandle EHandle, GUID_t		Guid,		float w = 1.0f, bool Loop = false );
		size_t				GetEntityAnimationPlayCount	(SceneEntityHandle EHandle );

		inline void SetVisability		(SceneEntityHandle EHandle, bool Visable = true )		{ DrawableVisibility.at(EHandle) = Visable; }
		inline bool GetVisability		(SceneEntityHandle EHandle )							{ return DrawableVisibility.at(EHandle);	}

		inline void SetRayVisability	(SceneEntityHandle EHandle, bool Visable = true )		{ DrawableRayVisibility.at(EHandle) = Visable; }
		inline bool GetRayVisability	(SceneEntityHandle EHandle )							{ return DrawableRayVisibility.at(EHandle);	}

		inline void				SetMeshHandle(SceneEntityHandle EHandle, TriMeshHandle M)		{ Drawables.at(EHandle).MeshHandle = M;		}
		inline TriMeshHandle	GetMeshHandle(SceneEntityHandle EHandle)							{ return Drawables.at(EHandle).MeshHandle;	}

		inline void				SetTextureSet		(SceneEntityHandle EHandle, TextureSet* Set)	{ Drawables.at(EHandle).Textures = Set; Drawables.at(EHandle).Textured = true; }
		inline TextureSet*		GetTextureSet		(SceneEntityHandle EHandle)					{ return Drawables.at(EHandle).Textures; }

		inline void SetMaterialParams	(SceneEntityHandle EHandle, float4 RGBA = float4(1.0f, 1.0f, 1.0f, 0.5f), float4 Spec = float4(1.0f, 1.0f, 1.0f, 0.5f))
		{
			Drawables.at(EHandle).MatProperties.Albedo		 = RGBA;
			Drawables.at(EHandle).MatProperties.Spec		 = Spec;
		}

		Drawable& SetNode(SceneEntityHandle EHandle, NodeHandle Node);

		SceneEntityHandle CreateSceneEntityAndSetMesh(GUID_t Mesh,		NodeHandle node = InvalidHandle_t);
		SceneEntityHandle CreateSceneEntityAndSetMesh(const char* Mesh, NodeHandle node = InvalidHandle_t);
		
		LightHandle	AddPointLight(float3 Color, float3 POS, float I = 10, float R = 10);
		LightHandle	AddPointLight(float3 Color, NodeHandle, float I = 10, float R = 10);

		void EnableShadowCasting(SpotLightHandle SpotLight);

		SpotLightHandle AddSpotLight (NodeHandle		Node,	float3 Color, float3 Dir, float t = pi / 4, float I = 10.0f, float R = 10.0f);
		SpotLightHandle AddSpotLight (float3			POS,	float3 Color, float3 Dir, float t = pi / 4, float I = 10, float R = 10);

		void _PushEntity(Drawable E, SceneEntityHandle Handle);
		void SetLightNodeHandle		(SpotLightHandle Handle, NodeHandle Node	);

		Quaternion GetOrientation	(SceneEntityHandle Handle );

		UpdateTask* Update(FlexKit::UpdateDispatcher& Dispatcher, UpdateTask* transformDependency);


		void SetDirty(SceneEntityHandle handle)
		{}

		struct TaggedJoint
		{
			SceneEntityHandle	Source;
			JointHandle			Joint;
			NodeHandle			Target;
		};

		iAllocator*					TempMemory;
		iAllocator*					Memory;
		RenderSystem*				RS;
		PVS							_PVS;

		HandleUtilities::HandleTable<SceneEntityHandle, 16> HandleTable;

		//Vector<SpotLightShadowCaster>		SpotLightCasters;
		Vector<Drawable>					Drawables;
		Vector<bool>						DrawableVisibility;
		Vector<bool>						DrawableRayVisibility;
		Vector<SceneEntityHandle>			DrawableHandles;
		Vector<TaggedJoint>					TaggedJoints;

		PointLightList	PLights;
		SpotLightList	SPLights;

		QuadTree	SceneManagement;

		operator GraphicScene* () { return this; }
	};


	/************************************************************************************************/


	FLEXKITAPI void UpdateGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void UpdateAnimationsGraphicScene	(GraphicScene* SM, double dt);
	FLEXKITAPI void UpdateGraphicScenePoseTransform	(GraphicScene* SM );
	FLEXKITAPI void GetGraphicScenePVS				(GraphicScene* SM, CameraHandle C, PVS* __restrict out, PVS* __restrict T_out);
	FLEXKITAPI void UploadGraphicScene				(GraphicScene* SM, PVS* , PVS*);
	FLEXKITAPI void UpdateShadowCasters				(GraphicScene* SM);

	FLEXKITAPI void ReleaseGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void BindJoint						(GraphicScene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode);

	FLEXKITAPI bool LoadScene (RenderSystem* RS, GUID_t Guid, GraphicScene* GS_out, iAllocator* Temp);
	FLEXKITAPI bool LoadScene (RenderSystem* RS, const char* LevelName, GraphicScene* GS_out, iAllocator* Temp);


	/************************************************************************************************/


	inline void Release(DrawablePoseState* EPS, iAllocator* allocator);


	/************************************************************************************************/


	class DrawableBehavior;

	template<typename TY>
	struct SceneNodeBehaviorOverrides
	{
		using Parent_TY = TY;

		template<typename ... discard>
		static void SetDirty(TY* drawable) 
		{
			std::cout << "SceneNodeMoved!\n";
			drawable->ParentScene->SetDirty(drawable->Entity);
		}
	};

	class DrawableBehavior : public SceneNodeBehavior<SceneNodeBehaviorOverrides<DrawableBehavior>>
	{
	public:
		DrawableBehavior(GraphicScene* IN_ParentScene = nullptr, SceneEntityHandle Handle = InvalidHandle_t) :
			SceneNodeBehavior	{ InvalidHandle_t },
			ParentScene			{IN_ParentScene},
			Entity				{Handle}	{}


		void		SetNode(NodeHandle Handle) 
		{
			ParentScene->SetNode(Entity, Handle);
		}


		NodeHandle	GetNode() 
		{ 
			return ParentScene->GetNode(Entity); 
		}


		GraphicScene*		ParentScene;
		SceneEntityHandle	Entity;
	};


}	/************************************************************************************************/

#endif