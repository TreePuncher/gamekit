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

namespace FlexKit
{
	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawable()
	{
		EntityHandle Out = HandleTable.GetNewHandle();

		Drawable		D;
		NodeHandle N  = GetZeroedNode(*SN);

		D.Node = N;
		_PushEntity(D, Out);

		HandleTable[Out] = Drawables.size() - 1;

		SceneManagement.CreateNode(Out);
		return Out;
	}


	/************************************************************************************************/


	void GraphicScene::RemoveEntity(EntityHandle E)
	{
		ReleaseNode(*SN, GetDrawable(E).Node);

		auto& Drawable = GetDrawable(E);
		Drawable.Release();

		ReleaseMesh(RS, GT, Drawable.MeshHandle);

		Drawable.MeshHandle		 = INVALIDMESHHANDLE;
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
		for (auto& D : this->Drawables)
		{
			ReleaseNode(*SN, D.Node);
			ReleaseMesh(RS, GT, D.MeshHandle);
		}

		SceneManagement.Release();
		Drawables.Release();
		DrawableVisibility.Release();
		DrawableRayVisibility.Release();
		DrawableHandles.Release();
		TaggedJoints.Release();
	}


	/************************************************************************************************/


	void GraphicScene::Update()
	{
		SceneManagement.Update();
	}


	/************************************************************************************************/


	bool GraphicScene::isEntitySkeletonAvailable(EntityHandle EHandle)
	{
		auto Index = HandleTable[EHandle];
		if (Drawables.at(Index).MeshHandle != INVALIDMESHHANDLE)
		{
			auto Mesh		= GetMesh(GT, Drawables.at(Index).MeshHandle);
			auto ID			= Mesh->TriMeshID;
			bool Available	= isResourceAvailable(RM, ID);
			return Available;
		}
		return false;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityEnablePosing(EntityHandle EHandle)
	{
		auto Index				= HandleTable[EHandle];
		bool Available			= isEntitySkeletonAvailable(EHandle);
		bool SkeletonAvailable  = false;
		auto MeshHandle			= GetDrawable(EHandle).MeshHandle;

		if (Available) {
			auto Mesh = GetMesh(GT, MeshHandle);
			SkeletonAvailable = isResourceAvailable(RM, Mesh->TriMeshID);
		}

		bool ret = false;
		if (Available && SkeletonAvailable)
		{
			if(!IsSkeletonLoaded(GT, MeshHandle))
			{
				auto SkeletonGUID	= GetSkeletonGUID(GT, MeshHandle);
				auto Handle			= LoadGameResource(RM, SkeletonGUID);
				auto S				= Resource2Skeleton(RM, Handle, Memory);
				SetSkeleton(GT, MeshHandle, S);
			}

			auto& E = GetDrawable(EHandle);
			E.PoseState	= CreatePoseState(&E, GT, Memory);
			E.Posed		= true;
			ret = true;
		}

		return ret;
	}


	/************************************************************************************************/

	bool LoadAnimation(GraphicScene* GS, EntityHandle EHandle, ResourceHandle RHndl, TriMeshHandle MeshHandle, float w = 1.0f)
	{
		auto Resource = GetResource(GS->RM, RHndl);
		if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
		{
			auto AC = Resource2AnimationClip(Resource, GS->Memory);
			FreeResource(GS->RM, RHndl);// No longer in memory once loaded

			auto mesh = GetMesh(GS->GT, MeshHandle);
			mesh->AnimationData |= FlexKit::TriMesh::AnimationData::EAD_Skin;
			AC.Skeleton = mesh->Skeleton;

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


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(EntityHandle EHandle, GUID_t Guid, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(GT, MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded)
		{
			auto S = FlexKit::GetSkeleton(GT, MeshHandle);
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
				if (isResourceAvailable(RM, Guid))
				{
					auto RHndl = LoadGameResource(RM, Guid);
					auto Res = LoadAnimation(this, EHandle, RHndl, MeshHandle, W);
					if(!Res)
						return{ false, -1 };
				}
				else
					return{ false, -1 };
			}

			int64_t ID = INVALIDHANDLE;
			auto Res = PlayAnimation(&GetDrawable(EHandle), GT, Guid, Memory, Loop, W, ID);
			if(Res == EPLAY_ANIMATION_RES::EPLAY_SUCCESS)
				return { true, ID };

			return{ false, -1};
		}
		return{ false, -1 };
	}


	/************************************************************************************************/


	GSPlayAnimation_RES GraphicScene::EntityPlayAnimation(EntityHandle EHandle, const char* Animation, float W, bool Loop)
	{
		auto MeshHandle		= GetDrawable(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(GT, MeshHandle);
		if (!SkeletonLoaded)
			return { false, -1 };

		if (SkeletonLoaded && HasAnimationData(GT, MeshHandle))
		{
			// TODO: Needs to Iterate Over Clips
			auto S = FlexKit::GetSkeleton(GT, MeshHandle);
			if (!strcmp(S->Animations->Clip.mID, Animation))
			{
				int64_t AnimationID = -1;
				if(PlayAnimation(&GetDrawable(EHandle), GT, Animation, Memory, Loop, W, &AnimationID))
					return { true, AnimationID };
				return{ false, -1 };
			}
		}

		// Search Resources for Animation
		if(isResourceAvailable(RM, Animation))
		{
			auto RHndl = LoadGameResource(RM, Animation);
			int64_t AnimationID = -1;
			if (LoadAnimation(this, EHandle, RHndl, MeshHandle, W)) {
				if(PlayAnimation(&GetDrawable(EHandle), GT, Animation, Memory, Loop, W, &AnimationID) == EPLAY_SUCCESS)
					return { true, AnimationID };
				return{ false, -1};
			}
			else
				return { false, -1 };
		}
		return { false, -1 };
	}


	/************************************************************************************************/


	size_t	GraphicScene::GetEntityAnimationPlayCount(EntityHandle EHandle)
	{
		size_t Out = 0;
		Out = GetAnimationCount(&Drawables.at(HandleTable[EHandle]));
		return Out;
	}


	/************************************************************************************************/


	Drawable& GraphicScene::SetNode(EntityHandle EHandle, NodeHandle Node) 
	{
		FlexKit::ReleaseNode(*SN, GetNode(EHandle));
		auto& Drawable = Drawables.at(HandleTable[EHandle]);
		Drawable.Node = Node;
		return Drawable;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(GUID_t Mesh)
	{
		auto EHandle = CreateDrawable();

		auto		Geo = FindMesh(GT, Mesh);
		if (!Geo)	Geo = LoadTriMeshIntoTable(RS, RM, GT, Mesh);

		auto& Drawble       = GetDrawable(EHandle);
		SetVisability(EHandle, true);

		Drawble.MeshHandle	= Geo;
		Drawble.Dirty		= true;
		Drawble.Textured	= false;
		Drawble.Textures	= nullptr;

		SetRayVisability(EHandle, true);

		return EHandle;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(const char* Mesh)
	{
		auto EHandle = CreateDrawable();

		TriMeshHandle MeshHandle = FindMesh(GT, Mesh);
		if (MeshHandle == INVALIDMESHHANDLE)	
			MeshHandle = LoadTriMeshIntoTable(RS, RM, GT, Mesh);

#ifdef _DEBUG
		FK_ASSERT(MeshHandle != INVALIDMESHHANDLE, "FAILED TO FIND MESH IN RESOURCES!");
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
		auto Node = GetNewNode(*SN);
		SetPositionW(*SN, Node, POS);
		PLights.push_back({ Color, I, R, Node });
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(float3 POS, float3 Color, float3 Dir, float t, float I, float R)
	{
		auto Node = GetNewNode(*SN);
		SetPositionW(*SN, Node, POS);
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


	void GraphicScene::_PushEntity(Drawable E, EntityHandle Handle)
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
			if (GetFlag(*Scene->SN, D.Node, SceneNodes::StateFlags::UPDATED))
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


	void QuadTree::Release()
	{
	}


	/************************************************************************************************/


	void QuadTree::CreateNode(EntityHandle Handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::ReleaseNode(EntityHandle Handle)
	{

	}


	/************************************************************************************************/


	void QuadTree::Update()
	{
	}


	/************************************************************************************************/


	void QuadTree::Initiate(iAllocator* memory, float2 AreaDimensions)
	{
		Memory = memory;
		Area = AreaDimensions;
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
					UpdateAnimation(SM->RS, &E, SM->GT, dt, SM->TempMemory);
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

			auto WT		= GetJointPosed_WT(Tag.Joint, Entity.Node, *SM->SN, Entity.PoseState);
			auto WT_t	= Float4x4ToXMMATIRX(&WT.Transpose());

			FlexKit::SetWT		(*SM->SN,	Tag.Target, &WT_t);
			FlexKit::SetFlag	(*SM->SN,	Tag.Target, SceneNodes::StateFlags::UPDATED);
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


	void UpdateGraphicScene(GraphicScene* SM)
	{
		SM->SceneManagement.Update();
		UpdateShadowCasters(SM);
	}


	/************************************************************************************************/


	void GetGraphicScenePVS(GraphicScene* SM, Camera* C, PVS* __restrict out, PVS* __restrict T_out)
	{
		FK_ASSERT(out   != T_out);
		FK_ASSERT(C     != nullptr);
		FK_ASSERT(SM    != nullptr);
		FK_ASSERT(out   != nullptr);
		FK_ASSERT(T_out != nullptr);


		Quaternion Q = FlexKit::GetOrientation(*SM->SN, C->Node);
		auto F = GetFrustum(C, GetPositionW(*SM->SN, C->Node), Q);

		auto End = SM->Drawables.size();
		for (size_t I = 0; I < End; ++I)
		{
			auto &E = SM->Drawables[I];
			auto Mesh	= GetMesh(SM->GT, E.MeshHandle);
			auto Ls		= GetLocalScale(*SM->SN, E.Node).x;
			auto Pw		= GetPositionW(*SM->SN, E.Node);
			auto Lq		= GetOrientation(*SM->SN, E.Node);

			auto BS		= Mesh->BS;

			BoundingSphere BoundingVolume = float4((Lq * BS.xyz()) + Pw, BS.w * Ls);

			auto DrawableVisibility = SM->DrawableVisibility[I];

			if (DrawableVisibility && Mesh && CompareBSAgainstFrustum(&F, BoundingVolume))
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
		UpdatePointLightBuffer	(*SM->RS, *SM->SN, &SM->PLights, SM->TempMemory);
		UpdateSpotLightBuffer	(*SM->RS, *SM->SN, &SM->SPLights, SM->TempMemory);

		//for (auto& Caster : SM->SpotLightCasters)
		//	UploadCamera(SM->RS, *SM->SN, &Caster.C, 0, 0, 0.0f);
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
			ReleaseMesh(SM->RS, SM->GT, E.MeshHandle);

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

		Release(&SM->PLights, SM->Memory);
		Release(&SM->SPLights, SM->Memory);

		SM->TaggedJoints.Release();
		SM->PLights.Lights	= nullptr;
		SM->Drawables		= nullptr;
	}


	/************************************************************************************************/


	void BindJoint(GraphicScene* SM, JointHandle Joint, EntityHandle Entity, NodeHandle TargetNode)
	{
		SM->TaggedJoints.push_back({ Entity, Joint, TargetNode });
	}


	/************************************************************************************************/


	//bool LoadScene(RenderSystem* RS, Resources* RM, GeometryTable*, const char* ID, GraphicScene* GS_out)
	//{
	//	return false;
	//}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, SceneNodes* SN, Resources* RM, GeometryTable*, GUID_t Guid, GraphicScene* GS_out, iAllocator* Temp)
	{
		bool Available = isResourceAvailable(RM, Guid);
		if (Available)
		{
			auto RHandle = LoadGameResource(RM, Guid);
			auto R		 = GetResource(RM, RHandle);

			FINALLY
			{
				FreeResource(RM, RHandle);
			}
			FINALLYOVER

			if (R != nullptr) {
				SceneResourceBlob* SceneBlob = (SceneResourceBlob*)R;
				auto& CreatedNodes = fixed_vector<NodeHandle>::Create(SceneBlob->SceneTable.NodeCount, Temp);

				{
					CompiledScene::SceneNode* Nodes = (CompiledScene::SceneNode*)(SceneBlob->Buffer + SceneBlob->SceneTable.NodeOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.NodeCount; ++I)
					{
						auto Node		= Nodes[I];
						auto NewNode	= GetNewNode(*SN);
						
						SetOrientationL(*SN, NewNode, Node.Q );
						SetPositionL(*SN,	NewNode, Node.TS.xyz());
						SetScale(*SN, NewNode, { Node.TS.w, Node.TS.w, Node.TS.w });

						if (Node.Parent != -1)
							SetParentNode(*SN, CreatedNodes[Node.Parent], NewNode);

						CreatedNodes.push_back(NewNode);
					}
				}

				{
					auto Entities = (CompiledScene::Entity*)(SceneBlob->Buffer + SceneBlob->SceneTable.EntityOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.EntityCount; ++I)
					{
						if (Entities[I].MeshGuid != INVALIDHANDLE) {
							auto NewEntity = GS_out->CreateDrawableAndSetMesh(Entities[I].MeshGuid);
							GS_out->SetNode(NewEntity, CreatedNodes[Entities[I].Node]);
							auto Position_DEBUG = GetPositionW(*SN, CreatedNodes[Entities[I].Node]);
							SetFlag(*SN, CreatedNodes[Entities[I].Node], SceneNodes::StateFlags::SCALE);
							int x = 0;
						}
					}
				}

				{
					auto Lights = (CompiledScene::PointLight*)(SceneBlob->Buffer + SceneBlob->SceneTable.LightOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.LightCount; ++I)
					{
						auto Light		= Lights[I];
						auto NewEntity	= GS_out->AddPointLight(Light.K, CreatedNodes[Light.Node], Light.I, Light.R * 10);
					}
				}
				return true;
			}
		}

		return false;
	}


	/************************************************************************************************/


	bool LoadScene(RenderSystem* RS, SceneNodes* SN, Resources* RM, GeometryTable* GT, const char* LevelName, GraphicScene* GS_out, iAllocator* Temp)
	{
		if (isResourceAvailable(RM, LevelName))
		{
			auto RHandle = LoadGameResource(RM, LevelName);
			auto R = GetResource(RM, RHandle);

			FINALLY
			{
				FreeResource(RM, RHandle);
			}
			FINALLYOVER

			return LoadScene(RS, *SN, RM, GT, R->GUID, GS_out, Temp);
		}
		return false;
	}


	/************************************************************************************************/


	float3 GraphicScene::GetEntityPosition(EntityHandle EHandle) 
	{ 
		return GetPositionW(*SN, Drawables.at(EHandle).Node); 
	}


	/************************************************************************************************/


	Quaternion GraphicScene::GetOrientation(EntityHandle Handle)
	{
		return FlexKit::GetOrientation(*SN, GetNode(Handle));
	}


	/************************************************************************************************/

	/*
	void GraphicScene::Yaw					(EntityHandle Handle, float a)				{ FlexKit::Yaw	(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::Pitch				(EntityHandle Handle, float a)				{ FlexKit::Pitch(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::Roll					(EntityHandle Handle, float a)				{ FlexKit::Roll	(*SN, GetDrawable(Handle).Node, a);												   }
	void GraphicScene::TranslateEntity_LT	(EntityHandle Handle, float3 V)				{ FlexKit::TranslateLocal	(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::TranslateEntity_WT	(EntityHandle Handle, float3 V)				{ FlexKit::TranslateWorld	(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetPositionEntity_WT	(EntityHandle Handle, float3 V)				{ FlexKit::SetPositionW		(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetPositionEntity_LT	(EntityHandle Handle, float3 V)				{ FlexKit::SetPositionL		(*SN, GetDrawable(Handle).Node, V);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetOrientation		(EntityHandle Handle, Quaternion Q)			{ FlexKit::SetOrientation	(*SN, GetDrawable(Handle).Node, Q);  GetDrawable(Handle).Dirty = true; }
	void GraphicScene::SetOrientationL		(EntityHandle Handle, Quaternion Q)			{ FlexKit::SetOrientationL	(*SN, GetDrawable(Handle).Node, Q);  GetDrawable(Handle).Dirty = true; }
	*/

	void GraphicScene::SetLightNodeHandle	(SpotLightHandle Handle, NodeHandle Node)	{ FlexKit::ReleaseNode		(*SN, PLights[Handle].Position); PLights[Handle].Position = Node;	   }



	



	Drawable::VConsantsLayout Drawable::GetConstants(SceneNodes* Nodes)
	{
		DirectX::XMMATRIX WT;
		FlexKit::GetWT(Nodes, Node, &WT);

		Drawable::VConsantsLayout	Constants;

		Constants.MP.Albedo = float4(0, 1, 1, 0.5f);// E->MatProperties.Albedo;
		Constants.MP.Spec	= MatProperties.Spec;
		Constants.Transform = DirectX::XMMatrixTranspose(WT);

		return Constants;
	}

}	/************************************************************************************************/