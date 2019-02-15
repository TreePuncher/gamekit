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

#include "GraphicScene.h"
#include "GraphicsComponents.h"
#include "intersection.h"

#include "..\coreutilities\Components.h"
#include "..\graphicsutilities\AnimationRuntimeUtilities.H"

namespace FlexKit
{
	/************************************************************************************************/


	SceneEntityHandle GraphicScene::CreateDrawable(TriMeshHandle mesh, NodeHandle node)
	{
		SceneEntityHandle Out = HandleTable.GetNewHandle();

		Drawable		D;
		NodeHandle N  = (node == InvalidHandle_t) ? GetZeroedNode() : node;

		D.Node			= N;
		D.MeshHandle	= mesh;
		D.id			= nullptr;

		_PushEntity(D, Out);

		HandleTable[Out] = Drawables.size() - 1;

		if (mesh == InvalidHandle_t || node == InvalidHandle_t)
			SceneManagement.allEntities.push_back(Out);
		else
			SceneManagement.CreateNode(Out, *this);
		
		return Out;
	}


	/************************************************************************************************/


	void GraphicScene::RemoveEntity(SceneEntityHandle E)
	{
		if (E == InvalidHandle_t)
			return;

		ReleaseNode(GetDrawable(E).Node);

		auto& Drawable = GetDrawable(E);
		Drawable.Release();

		ReleaseMesh(RS, Drawable.MeshHandle);

		Drawable.MeshHandle		 = InvalidHandle_t;
		DrawableVisibility[E]	 = false;
		DrawableRayVisibility[E] = false;

		auto Index = HandleTable[E];

		auto TempDrawable			= Drawables.back();
		auto TempVisibility			= DrawableVisibility.back();
		auto TempRayVisilibility	= DrawableRayVisibility.back();
		auto TempDrawableHandle		= DrawableHandles.back();

		auto Index2 = HandleTable[TempDrawableHandle];


		Drawables.pop_back();
		DrawableVisibility.pop_back();
		DrawableRayVisibility.pop_back();
		DrawableHandles.pop_back();

		if (!Drawables.size() || (Index == Drawables.size()))
			return;

		Drawables[Index]				= TempDrawable;
		DrawableVisibility[Index]		= TempVisibility;
		DrawableRayVisibility[Index]	= TempRayVisilibility;
		DrawableHandles[Index]			= TempDrawableHandle;

		HandleTable[TempDrawableHandle] = HandleTable[E];
	}


	/************************************************************************************************/


	void GraphicScene::ClearScene()
	{
		PLights.Release();
		SPLights.Release();


		for (auto& D : this->Drawables)
		{
			if (D.id)
				Memory->free(D.id);
			D.id = nullptr;

			ReleaseNode(D.Node);
			ReleaseMesh(RS, D.MeshHandle);
		}

		SceneManagement.Release();
		Drawables.Release();
		DrawableVisibility.Release();
		DrawableRayVisibility.Release();
		DrawableHandles.Release();
		TaggedJoints.Release();
	}


	/************************************************************************************************/


	Drawable&	GraphicScene::GetDrawable(SceneEntityHandle EHandle) 
	{ 
		return Drawables.at(HandleTable[EHandle]); 
	}


	/************************************************************************************************/


	BoundingSphere	GraphicScene::GetBoundingSphere(SceneEntityHandle EHandle)
	{
		BoundingSphere out;

		float3		position = GetEntityPosition(EHandle);
		Quaternion	orientation = GetOrientation(EHandle);
		auto BS = GetMeshBoundingSphere(GetMeshHandle(EHandle));

		return BoundingSphere{ orientation * position + BS.xyz(), BS.w };
	}


	/************************************************************************************************/


	bool GraphicScene::isEntitySkeletonAvailable(SceneEntityHandle EHandle)
	{
		auto Index = HandleTable[EHandle];
		if (Drawables.at(Index).MeshHandle != InvalidHandle_t)
		{
			auto Mesh		= GetMeshResource(Drawables.at(Index).MeshHandle);
			auto ID			= Mesh->TriMeshID;
			bool Available	= isResourceAvailable(ID);
			return Available;
		}
		return false;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityEnablePosing(SceneEntityHandle EHandle)
	{
		auto Index				= HandleTable[EHandle];
		bool Available			= isEntitySkeletonAvailable(EHandle);
		bool SkeletonAvailable  = false;
		auto MeshHandle			= GetDrawable(EHandle).MeshHandle;

		if (Available) {
			auto Mesh			= GetMeshResource(MeshHandle);
			SkeletonAvailable	= isResourceAvailable(Mesh->TriMeshID);
		}

		bool ret = false;
		if (Available && SkeletonAvailable)
		{
			if(!IsSkeletonLoaded(MeshHandle))
			{
				auto SkeletonGUID	= GetSkeletonGUID(MeshHandle);
				auto Handle			= LoadGameResource(SkeletonGUID);
				auto S				= Resource2Skeleton(Handle, Memory);
				SetSkeleton(MeshHandle, S);
			}

			auto& E = GetDrawable(EHandle);
			E.PoseState	= CreatePoseState(&E, Memory);
			E.Posed		= true;
			ret = true;
		}

		return ret;
	}


	/************************************************************************************************/


	bool LoadAnimation(GraphicScene* GS, SceneEntityHandle EHandle, ResourceHandle RHndl, TriMeshHandle MeshHandle, float w = 1.0f)
	{
		auto Resource = GetResource(RHndl);
		if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
		{
			auto AC = Resource2AnimationClip(Resource, GS->Memory);
			FreeResource(RHndl);// No longer in memory once loaded

			auto mesh				= GetMeshResource(MeshHandle);
			AC.Skeleton				= mesh->Skeleton;
			mesh->AnimationData    |= EAnimationData::EAD_Skin;

			if (AC.Skeleton->Animations)
			{
				auto I = AC.Skeleton->Animations;
				while (I->Next)
					I = I->Next;

				I->Next				= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				I->Next->Clip		= AC;
				I->Next->Memory		= GS->Memory;
				I->Next->Next		= nullptr;
			}
			else
			{
				AC.Skeleton->Animations			= &GS->Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
				AC.Skeleton->Animations->Clip	= AC;
				AC.Skeleton->Animations->Next	= nullptr;
				AC.Skeleton->Animations->Memory = GS->Memory;
			}

			return true;
		}
		return false;
	}


	/************************************************************************************************/


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(SceneEntityHandle EHandle, GUID_t Guid, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded)
		{
			auto S = FlexKit::GetSkeleton(MeshHandle);
			Skeleton::AnimationList* I = S->Animations;
			bool AnimationLoaded = false;

			// Find if Animation is Already Loaded
			{
				
				while (I)
				{
					if (I->Clip.guid == Guid) {
						AnimationLoaded = true;
						break;
					}
					
					I = I->Next;
				}
			}
			if (!AnimationLoaded)
			{
				// Search Resources for Animation
				if (isResourceAvailable(Guid))
				{
					auto RHndl = LoadGameResource(Guid);
					auto Res = LoadAnimation(this, EHandle, RHndl, MeshHandle, W);
					if(!Res)
						return{ false, -1 };
				}
				else
					return{ false, -1 };
			}

			int64_t ID = INVALIDHANDLE;
			auto Res = PlayAnimation(&GetDrawable(EHandle), Guid, Memory, Loop, W, ID);
			if(Res == EPLAY_ANIMATION_RES::EPLAY_SUCCESS)
				return { true, ID };

			return{ false, -1};
		}
		return{ false, -1 };
	}


	/************************************************************************************************/


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(SceneEntityHandle EHandle, const char* Animation, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded && HasAnimationData(MeshHandle))
		{
			// TODO: Needs to Iterate Over Clips
			auto S = FlexKit::GetSkeleton(MeshHandle);
			if (!strcmp(S->Animations->Clip.mID, Animation))
			{
				int64_t AnimationID = -1;
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID))
					return { true, AnimationID };
				return{ false, -1 };
			}
		}

		// Search Resources for Animation
		if(isResourceAvailable(Animation))
		{
			auto RHndl = LoadGameResource(Animation);
			int64_t AnimationID = -1;
			if (LoadAnimation(this, EHandle, RHndl, MeshHandle, W)) {
				if(PlayAnimation(&GetDrawable(EHandle), Animation, Memory, Loop, W, &AnimationID) == EPLAY_SUCCESS)
					return { true, AnimationID };
				return{ false, -1};
			}
			else
				return { false, -1 };
		}
		return { false, -1 };
	}


	/************************************************************************************************/


	size_t	GraphicScene::GetEntityAnimationPlayCount(SceneEntityHandle EHandle)
	{
		size_t Out = 0;
		Out = GetAnimationCount(&Drawables.at(HandleTable[EHandle]));
		return Out;
	}


	/************************************************************************************************/


	Drawable& GraphicScene::SetNode(SceneEntityHandle EHandle, NodeHandle Node)
	{
		FlexKit::ReleaseNode(GetNode(EHandle));
		auto& Drawable = Drawables.at(HandleTable[EHandle]);
		Drawable.Node = Node;
		return Drawable;
	}


	/************************************************************************************************/


	SceneEntityHandle GraphicScene::CreateSceneEntityAndSetMesh(GUID_t Mesh, NodeHandle node)
	{
		auto [Geo, Result]	= FindMesh(Mesh);

		if (!Result)
			Geo	= LoadTriMeshIntoTable(RS, Mesh);

		auto EHandle		= CreateDrawable(Geo, node);
		auto& Drawble       = GetDrawable(EHandle);
		SetVisability(EHandle, true);

		Drawble.Dirty		= true;
		Drawble.Textured	= false;
		Drawble.Textures	= nullptr;

		SetRayVisability(EHandle, true);

		return EHandle;
	}


	/************************************************************************************************/


	SceneEntityHandle GraphicScene::CreateSceneEntityAndSetMesh(const char* Mesh, NodeHandle node)
	{
		auto EHandle = CreateDrawable();

		TriMeshHandle MeshHandle = FindMesh(Mesh);
		if (MeshHandle == InvalidHandle_t)	
			MeshHandle = LoadTriMeshIntoTable(RS, Mesh);

#ifdef _DEBUG
		FK_ASSERT(MeshHandle != InvalidHandle_t, "FAILED TO FIND MESH IN RESOURCES!");
#endif

		auto& Drawble       = GetDrawable(EHandle);
		SetVisability(EHandle, true);

		Drawble.Textures    = nullptr;
		Drawble.MeshHandle  = MeshHandle;
		Drawble.Dirty		= true;
		Drawble.Textured    = false;
		Drawble.Posed		= false;
		Drawble.PoseState   = nullptr;

		return EHandle;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, NodeHandle Node, float I, float R)
	{
		PLights.push_back({Color, I, R, Node});
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(NodeHandle Node, float3 Color, float3 Dir, float t, float I, float R )
	{
		SPLights.push_back({ Color, Dir, I, R, t, Node });
		return PLights.size() - 1;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, float3 POS, float I, float R)
	{
		auto Node = GetNewNode();
		SetPositionW(Node, POS);
		PLights.push_back({ Color, I, R, Node });
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(float3 POS, float3 Color, float3 Dir, float t, float I, float R)
	{
		auto Node = GetNewNode();
		SetPositionW(Node, POS);
		SPLights.push_back({Color, Dir, I, R, t, Node});
		return PLights.size() - 1;
	}

	
	/************************************************************************************************/


	void GraphicScene::EnableShadowCasting(SpotLightHandle SpotLight)
	{
		FK_ASSERT(0, "UNIMPLENTED");
		auto Light = &SPLights[0];
		//SpotLightCasters.push_back({ Camera(), SpotLight});
		//SpotLightCasters.back().C.FOV = RadToDegree(Light->t);
		//InitiateCamera(*SN, &SpotLightCasters.back().C, 1.0f, 15.0f, 100, false);
	}


	/************************************************************************************************/


	void GraphicScene::_PushEntity(Drawable E, SceneEntityHandle Handle)
	{
		Drawables.push_back(E);
		DrawableVisibility.push_back(false);
		DrawableRayVisibility.push_back(false);
		DrawableHandles.push_back(Handle);
	}


	/************************************************************************************************/


	void UpdateQuadTree(QuadTreeNode* Node, GraphicScene* Scene)
	{
		for (const auto D : Scene->Drawables) {
			if (GetFlag(D.Node, SceneNodes::StateFlags::UPDATED))
			{
				/*
				if()
				{	// Moved Branch
					if()
					{	// Space in Node Available
					}
					else
					{	// Node Split Needed
					}
				}
				*/
			}
		}
	}


	/************************************************************************************************/


	void QuadTreeNode::ExpandNode(iAllocator* allocator)
	{
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>());
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>());
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>());
		ChildNodes.push_back(&allocator->allocate_aligned<QuadTreeNode>());

		const auto SpanVector	= upperRight - lowerLeft;
		const auto CenterPoint	= (lowerLeft + upperRight) / 2;

		ChildNodes[(int)QuadTreeNodeLocation::UpperRight]->upperRight	= CenterPoint + SpanVector / 2;
		ChildNodes[(int)QuadTreeNodeLocation::UpperRight]->lowerLeft	= CenterPoint;

		ChildNodes[(int)QuadTreeNodeLocation::UpperLeft]->upperRight	= CenterPoint + (SpanVector / 2) * float2( 0.0f,  1.0f);
		ChildNodes[(int)QuadTreeNodeLocation::UpperLeft]->lowerLeft		= CenterPoint + (SpanVector / 2) * float2(-1.0f,  0.0f);

		ChildNodes[(int)QuadTreeNodeLocation::LowerLeft]->upperRight	= CenterPoint;
		ChildNodes[(int)QuadTreeNodeLocation::LowerLeft]->lowerLeft		= CenterPoint - (SpanVector / 2);

		ChildNodes[(int)QuadTreeNodeLocation::LowerRight]->upperRight	= CenterPoint + (SpanVector / 2) * float2( 1.0f,  0.0f);
		ChildNodes[(int)QuadTreeNodeLocation::LowerRight]->lowerLeft	= CenterPoint + (SpanVector / 2) * float2( 0.0f, -1.0f);

		for (auto& node : ChildNodes)
			node->centerPoint = (node->upperRight + node->lowerLeft) / 2;
	}

	float4 QuadTreeNode::GetArea() const
	{
		const auto areaLL = (lowerLeft.y <  upperRight.y) ? lowerLeft : upperRight;
		const auto areaUR = (lowerLeft.y >= upperRight.y) ? lowerLeft : upperRight;

		return { areaLL, areaUR };
	}

	/************************************************************************************************/


	void QuadTree::Release()
	{
		root.Clear(allocator);
	}


	/************************************************************************************************/


	void QuadTreeNode::AddChild(SceneEntityHandle entityHandle, GraphicScene& parentScene, iAllocator* allocator)
	{
		if (ChildNodes.empty() && !Contents.full())
		{
			Contents.push_back(entityHandle);
		}
		else
		{
			if (ChildNodes.empty()) 
			{
				ExpandNode(allocator);

				for (auto C : Contents)
					AddChild(C, parentScene, allocator);

				Contents.clear();
			}

			// Get Nearest Node

			auto& drawable	= parentScene.GetDrawable(entityHandle);
			auto position	= GetPositionW(drawable.Node);

			float2 pos_f2{position.x, position.z};

			float			l = 1000000000.0f;
			QuadTreeNode*	nearestNode = ChildNodes.front();

			for (auto child : ChildNodes)
			{
				float d = (child->centerPoint - pos_f2).Magnitude();
				if (d < l)
				{
					l = d;
					nearestNode = child;
				}
			}

			nearestNode->AddChild(entityHandle, parentScene, allocator);
		}
	}


	/************************************************************************************************/


	QuadTreeNode::SphereTestRes QuadTreeNode::IsSphereInside(float r, float3 pos)
	{
		float nodeSpan			= (lowerLeft - upperRight).Magnitude();
		float l					= (float2(pos.x, pos.z) - centerPoint).Magnitude();

		bool OutSideNode		= (l > (nodeSpan / 2));

		return OutSideNode ? Failed : Fully;
	}

	/************************************************************************************************/

	void QuadTreeNode::UpdateBounds(GraphicScene& parentScene)
	{
		for (auto child : ChildNodes) 
		{
			child->UpdateBounds(parentScene);

			upperRight.x = (child->upperRight.x > upperRight.x) ? child->upperRight.x : upperRight.x;
			upperRight.y = (child->upperRight.y > upperRight.y) ? child->upperRight.y : upperRight.y;

			lowerLeft.x = (child->lowerLeft.x > lowerLeft.x) ? child->lowerLeft.x : lowerLeft.x;
			lowerLeft.y = (child->lowerLeft.y > lowerLeft.y) ? child->lowerLeft.y : lowerLeft.y;
		}

		for(auto entityHandle : Contents)
		{
			auto& drawable	= parentScene.GetDrawable(entityHandle);
			bool isNullNode = drawable.MeshHandle == InvalidHandle_t;
			auto* triMesh	= isNullNode ? nullptr : GetMeshResource(drawable.MeshHandle);
			auto r			= isNullNode ? 0 : triMesh->Info.r;
			auto max		= isNullNode ? 0 : triMesh->Info.max;
			auto min		= isNullNode ? 0 : triMesh->Info.min;
			//auto offset		= isNullNode ? 0 : triMesh->Info.Offset;
			auto position	= GetPositionW(drawable.Node);

			float2 vectorToNode				= centerPoint - float2{position.x, position.z};
			
			float2 vectorToNodeNormalized	= vectorToNode / vectorToNode.Magnitude();
			
			if ((vectorToNode * vectorToNode).Sum() < 0.00001f)
				vectorToNodeNormalized = float2{ 0, 0 };

			float2 newUR					= centerPoint + vectorToNodeNormalized * -r;

			upperRight.x = (newUR.y > upperRight.y) ? newUR.x : upperRight.x;
			upperRight.y = (newUR.y > upperRight.y) ? newUR.y : upperRight.y;

			lowerLeft.x = (newUR.y < lowerLeft.y) ? newUR.x : lowerLeft.x;
			lowerLeft.y = (newUR.y < lowerLeft.y) ? newUR.y : lowerLeft.y;
		}
	}

	/************************************************************************************************/


	void QuadTree::CreateNode(SceneEntityHandle entityHandle, GraphicScene& parentScene)
	{
		allEntities.push_back(entityHandle);
		//root.AddChild(entityHandle, parentScene, allocator);
		//UpdateBounds(parentScene);
	}


	/************************************************************************************************/


	void QuadTree::ReleaseNode(SceneEntityHandle Handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::Rebuild(GraphicScene& parentScene) 
	{
		RebuildCounter = 0;
		root.Clear(allocator);

		float2 lowerLeft	{ 0, 0 };
		float2 upperRight	{ 0, 0 };
			
		for (auto item : allEntities)
		{
			auto boundingSphere = parentScene.GetBoundingSphere(item);
			float r = boundingSphere.w;

			upperRight.x = boundingSphere.x + r > upperRight.x ? boundingSphere.x + r : upperRight.x;
			upperRight.y = boundingSphere.z + r > upperRight.y ? boundingSphere.z + r : upperRight.y;

			lowerLeft.x = boundingSphere.x - r < lowerLeft.x ? boundingSphere.x - r : lowerLeft.x;
			lowerLeft.y = boundingSphere.z - r < lowerLeft.y ? boundingSphere.z - r : lowerLeft.y;
		}

		auto centerPoint	= (upperRight - lowerLeft) / 2;
		root.centerPoint	= centerPoint;
		root.lowerLeft		= lowerLeft;
		root.upperRight		= upperRight;

		for (auto item : allEntities)
			root.AddChild(item, parentScene, allocator);
	}


	/************************************************************************************************/


	float4	QuadTree::GetArea()
	{
		return root.GetArea();
	}


	/************************************************************************************************/


	UpdateTask* QuadTree::Update(FlexKit::UpdateDispatcher& dispatcher, GraphicScene* parentScene, UpdateTask* transformDependency)
	{
		struct QuadTreeUpdate
		{
			QuadTree*		QTree;
			GraphicScene*	parentScene;
		};

		auto Task = &dispatcher.Add<QuadTreeUpdate>(
			[&](DependencyBuilder& builder, auto& QuadTreeUpdate)
			{
				builder.AddInput(*transformDependency);
				builder.SetDebugString("QuadTree Update!");

				QuadTreeUpdate.QTree		= this;
				QuadTreeUpdate.parentScene	= parentScene;

			},
			[](auto& QuadTreeUpdate)
			{
				//FK_LOG_INFO("QuadTree::Update");

				const int period  = QuadTreeUpdate.QTree->RebuildPeriod;
				const int counter = QuadTreeUpdate.QTree->RebuildCounter;

				//if (period <= counter)
				//	QuadTreeUpdate.QTree->Rebuild(*QuadTreeUpdate.parentScene);
				//else
				//	QuadTreeUpdate.QTree->root.UpdateBounds(*QuadTreeUpdate.parentScene);

				//QuadTreeUpdate.QTree->RebuildCounter++;
			});

		return Task;
	}


	/************************************************************************************************/


	void QuadTree::Initiate(float2 AreaDimensions, iAllocator* memory)
	{
		allocator				= memory;
		allEntities.Allocator	= memory;
		area					= float2{0, 0};

		root.upperRight = AreaDimensions;
		root.lowerLeft	= AreaDimensions * -1;
	}


	/************************************************************************************************/


	void InitiateGraphicScene()
	{
		
	}


	/************************************************************************************************/


	void UpdateAnimationsGraphicScene(GraphicScene* SM, double dt)
	{
		for (auto E : SM->Drawables)
		{
			if (E.Posed) {
				if (E.AnimationState && GetAnimationCount(&E))
					UpdateAnimation(SM->RS, &E, dt, SM->TempMemory);
				else
					ClearAnimationPose(E.PoseState, SM->TempMemory);
			}
		}
	}


	/************************************************************************************************/


	void UpdateGraphicScenePoseTransform(GraphicScene* SM)
	{
		for(auto Tag : SM->TaggedJoints)
		{
			auto Entity = SM->GetDrawable(Tag.Source);

			auto WT		= GetJointPosed_WT(Tag.Joint, Entity.Node, Entity.PoseState);
			auto WT_t	= Float4x4ToXMMATIRX(&WT.Transpose());

			FlexKit::SetWT		(Tag.Target, &WT_t);
			FlexKit::SetFlag	(Tag.Target, SceneNodes::StateFlags::UPDATED);
		}
	}


	/************************************************************************************************/


	void UpdateShadowCasters(GraphicScene* GS)
	{
		//for (auto& Caster : GS->SpotLightCasters)
		//{
		//	UpdateCamera(GS->RS, *GS->SN, &Caster.C, 0.00f);
		//}
	}


	/************************************************************************************************/


	void GetGraphicScenePVS(GraphicScene* SM, CameraHandle Camera, PVS* __restrict out, PVS* __restrict T_out)
	{
		FK_ASSERT(out		!= T_out);
		FK_ASSERT(Camera	!= CameraHandle{(unsigned int)INVALIDHANDLE});
		FK_ASSERT(SM		!= nullptr);
		FK_ASSERT(out		!= nullptr);
		FK_ASSERT(T_out		!= nullptr);


		auto CameraNode = GetCameraNode(Camera);
		float3	POS		= GetPositionW(CameraNode);
		Quaternion Q	= GetOrientation(CameraNode);

		auto F			= GetFrustum(Camera);

		auto End = SM->Drawables.size();
		for (size_t I = 0; I < End; ++I)
		{
			auto &E = SM->Drawables[I];
			auto mesh	= GetMeshResource	(E.MeshHandle);
			auto Ls		= GetLocalScale		(E.Node).x;
			auto Pw		= GetPositionW		(E.Node);
			auto Lq		= GetOrientation	(E.Node);
			auto BS		= mesh->BS;

			BoundingSphere BoundingVolume = float4((Lq * BS.xyz()) + Pw, BS.w * Ls);

			auto DrawableVisibility = SM->DrawableVisibility[I];

			if (DrawableVisibility && mesh && CompareBSAgainstFrustum(&F, BoundingVolume))
			{
				if (!E.Transparent)
					PushPV(&E, out);
				else
					PushPV(&E, T_out);
			}
		}	
	}


	/************************************************************************************************/


	void UploadGraphicScene(GraphicScene* SM, PVS* Dawables, PVS* Transparent_PVS)
	{
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


	void ReleaseGraphicScene(GraphicScene* SM)
	{
		for (auto E : SM->Drawables)
		{
			ReleaseMesh(SM->RS, E.MeshHandle);

			if (E.PoseState) 
			{
				FK_ASSERT(0);
				//Release(E.PoseState);
				//Release(E.PoseState, SM->Memory);
				SM->Memory->_aligned_free(E.PoseState);
				SM->Memory->_aligned_free(E.AnimationState);
			}
		}

		SM->Drawables.Release();
		SM->TaggedJoints.Release();
		SM->PLights.Lights	= nullptr;
		SM->Drawables		= nullptr;
	}


	/************************************************************************************************/


	void BindJoint(GraphicScene* SM, JointHandle Joint, SceneEntityHandle Entity, NodeHandle TargetNode)
	{
		SM->TaggedJoints.push_back({ Entity, Joint, TargetNode });
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, GUID_t Guid, GraphicScene* GS_out, iAllocator* Temp)
	{
		bool Available = isResourceAvailable(Guid);
		if (Available)
		{
			auto RHandle = LoadGameResource(Guid);
			auto R		 = GetResource(RHandle);

			FINALLY
			{
				FreeResource(RHandle);
			}
			FINALLYOVER

			if (R != nullptr) {
				SceneResourceBlob* SceneBlob = (SceneResourceBlob*)R;

				auto& CreatedNodes = 
						Vector<NodeHandle>(
							Temp, 
							SceneBlob->SceneTable.NodeCount);

				{
					CompiledScene::SceneNode* Nodes = (CompiledScene::SceneNode*)(SceneBlob->Buffer + SceneBlob->SceneTable.NodeOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.NodeCount; ++I)
					{
						auto Node		= Nodes[I];
						auto NewNode	= GetNewNode();
						
						SetOrientationL	(NewNode, Node.Q );
						SetPositionL	(NewNode, Node.TS.xyz());
						SetScale		(NewNode, { Node.TS.w, Node.TS.w, Node.TS.w });

						if (Node.Parent != -1)
							SetParentNode(CreatedNodes[Node.Parent], NewNode);

						CreatedNodes.push_back(NewNode);
					}

					UpdateTransforms();
				}

				{
					auto Entities = (CompiledScene::Entity*)(SceneBlob->Buffer + SceneBlob->SceneTable.EntityOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.EntityCount; ++I)
					{
						if (Entities[I].MeshGuid != INVALIDHANDLE) {
							auto node			= CreatedNodes[Entities[I].Node];
							auto Position_DEBUG = GetPositionW(node);

							auto NewEntity = GS_out->CreateSceneEntityAndSetMesh(
								Entities[I].MeshGuid, 
								node);

							size_t idLength = Entities[I].idlength;

							if (idLength)
							{
								const size_t string_offset	= (size_t)Entities[I].id;
								char* stringBuffer			= (char*)GS_out->Memory->malloc(idLength + 1);// Adding 1 byte for null terminator
								stringBuffer[idLength]		= '\0';

								strncpy(stringBuffer, SceneBlob->Buffer + SceneBlob->SceneTable.SceneStringsOffset, idLength);

								GS_out->SetEntityId(NewEntity, stringBuffer);
							}

							//float4 albedo;
							//float4 specular;

							//GS_out->SetMaterialParams(NewEntity, albedo, specular);

							SetFlag(CreatedNodes[Entities[I].Node], SceneNodes::StateFlags::SCALE);
							int x = 0;
						}
					}
				}

				{
					auto Lights = (CompiledScene::PointLight*)(SceneBlob->Buffer + SceneBlob->SceneTable.LightOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.LightCount; ++I)
					{
						auto Light		= Lights[I];
						auto NewEntity	= GS_out->AddPointLight(Light.K, CreatedNodes[Light.Node], Light.I, Light.R);
					}
				}

				//GS_out->SceneManagement.Rebuild(*GS_out);

				return true;
			}
		}


		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, const char* LevelName, GraphicScene* GS_out, iAllocator* Temp)
	{
		if (isResourceAvailable(LevelName))
		{
			auto RHandle = LoadGameResource(LevelName);
			auto R = GetResource(RHandle);

			FINALLY
			{
				FreeResource(RHandle);
			}
			FINALLYOVER

			return LoadScene(RS, R->GUID, GS_out, Temp);
		}
		return false;
	}


	/************************************************************************************************/


	float3 GraphicScene::GetEntityPosition(SceneEntityHandle EHandle)
	{ 
		return GetPositionW(Drawables.at(EHandle).Node); 
	}


	/************************************************************************************************/


	Quaternion GraphicScene::GetOrientation(SceneEntityHandle Handle)
	{
		return FlexKit::GetOrientation(GetNode(Handle));
	}


	/************************************************************************************************/


	UpdateTask* GraphicScene::Update(FlexKit::UpdateDispatcher& Dispatcher, UpdateTask* transformDependency)
	{
		struct SceneUpdateData
		{
			GraphicScene* scene;
		};


		auto& Task1 =
		Dispatcher.Add<SceneUpdateData>(
			[&](DependencyBuilder& builder, SceneUpdateData& data)
			{
				builder.AddInput(*SceneManagement.Update(
													Dispatcher, 
													this, 
													transformDependency));

				builder.SetDebugString("SceneUpdate");

				data.scene = this;
			},
			[](auto& data)
			{
				// Post transform update of scene management structure
				UpdateShadowCasters(data.scene);
			}
		);

		return &Task1;
	}


	/************************************************************************************************/


	float3 GraphicScene::GetPointLightPosition(LightHandle light)
	{
		return GetPositionW(PLights[light].Position);
	}


	/************************************************************************************************/


	NodeHandle GraphicScene::GetPointLightNode(LightHandle light)
	{
		return PLights[light].Position;
	}


	/************************************************************************************************/


	float GraphicScene::GetPointLightRadius(LightHandle light)
	{
		return PLights[light].R;
	}


	/************************************************************************************************/



	size_t GraphicScene::GetPointLightCount()
	{
		return PLights.size();
	}


	/************************************************************************************************/


	Vector<LightHandle> GraphicScene::FindPointLights(const Frustum &f, iAllocator* tempMemory)
	{
		Vector<LightHandle> lights{tempMemory};

		for (unsigned int itr = 0; itr < PLights.size(); ++itr) {
			auto& light = PLights[itr];
			auto Pw		= GetPositionW(light.Position);
			auto Ps		= GetLocalScale(light.Position).x;

			//BoundingSphere BoundingVolume = float4(Pw, light.R * Ps);
			BoundingSphere BoundingVolume = float4(Pw, 1);

			if (CompareBSAgainstFrustum(&f, BoundingVolume))
				lights.emplace_back(LightHandle{ itr });
		}

		return lights;
	}


	/************************************************************************************************/


	void GraphicScene::ListEntities()
	{
		for (auto& d : Drawables)
		{
			if (d.id)
				std::cout << d.id << "\n";
			else
				std::cout << "entity with no id\n";
		}
	}


	/************************************************************************************************/


	void GraphicScene::SetLightNodeHandle	(SpotLightHandle Handle, NodeHandle Node)	{ FlexKit::ReleaseNode		(PLights[Handle].Position); PLights[Handle].Position = Node;	   }


	/************************************************************************************************/


	Drawable::VConsantsLayout Drawable::GetConstants()
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetTransform(Node, &WT);

		Drawable::VConsantsLayout	Constants;

		Constants.MP.Albedo = float4(0, 1, 1, 0.5f);// E->MatProperties.Albedo;
		Constants.MP.Spec	= MatProperties.Spec;
		Constants.Transform = DirectX::XMMatrixTranspose(WT);

		return Constants;
	}


	/************************************************************************************************/


	void Release(DrawablePoseState* EPS, iAllocator* allocator)
	{
		if (EPS->Joints)			allocator->free(EPS->Joints);
		if (EPS->CurrentPose)	allocator->free(EPS->CurrentPose);

		EPS->Joints = nullptr;
		EPS->CurrentPose = nullptr;
	}

}	/************************************************************************************************/