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

#include "GraphicScene.h"
#include "intersection.h"

namespace FlexKit
{
	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawable()
	{
		if (FreeEntityList&& FreeEntityList->size())
		{
			auto E = FreeEntityList->back();
			FreeEntityList->pop_back();
			Drawable& e = GetEntity(E);

			DrawableDesc	Desc;
			NodeHandle N  = GetZeroedNode(SN);
			FlexKit::CreateDrawable(RS, &e, Desc);

			e.Visable	= false;
			e.Posed		= false;
			e.Textured	= false;
			e.Node		= N;
			return E;
		}
		else
		{
			Drawable		e;
			DrawableDesc	Desc;
			NodeHandle N  = GetZeroedNode(SN);
			FlexKit::CreateDrawable(RS, &e, Desc);

			e.Visable  = false;
			e.Node	   = N;
			_PushEntity(e);
			return Drawables->size() - 1;
		}
		return -1;
	}


	/************************************************************************************************/


	void GraphicScene::RemoveEntity(EntityHandle E)
	{
		if (E + 1 == Drawables->size())
		{
			FreeHandle(SN, GetEntity(E).Node);
			CleanUpDrawable(&GetEntity(E));
			Drawables->pop_back();
		}
		else
		{
			if (!FreeEntityList)
				FreeEntityList = &FreeEntityList->Create(64, Memory);
			if (FreeEntityList->full())
			{
				auto NewList = &FreeEntityList->Create(FreeEntityList->size() * 2, Memory);
				for (auto e : *FreeEntityList)
					FreeEntityList->push_back(e);
				Memory->free(FreeEntityList);
				FreeEntityList = NewList;
			}

			FreeEntityList->push_back(E);
			FreeHandle(SN, GetEntity(E).Node);
			CleanUpDrawable(&GetEntity(E));
			GetEntity(E).VConstants.Release();
			GetEntity(E).Visable	= false;
			ReleaseMesh(GT, GetEntity(E).MeshHandle);
			GetEntity(E).MeshHandle = INVALIDMESHHANDLE;
		}
	}


	/************************************************************************************************/


	bool GraphicScene::isEntitySkeletonAvailable(EntityHandle EHandle)
	{
		if (Drawables->at(EHandle).MeshHandle != INVALIDMESHHANDLE)
		{
			auto Mesh		= GetMesh(GT, Drawables->at(EHandle).MeshHandle);
			auto ID			= Mesh->TriMeshID;
			bool Available	= isResourceAvailable(RM, ID);
			return Available;
		}
		return false;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityEnablePosing(EntityHandle EHandle)
	{
		bool Available			= isEntitySkeletonAvailable(EHandle);
		bool SkeletonAvailable  = false;
		auto MeshHandle			= GetEntity(EHandle).MeshHandle;

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

			GetEntity(EHandle).PoseState	= CreatePoseState(&GetEntity(EHandle), GT, Memory);
			GetEntity(EHandle).Posed		= true;
			ret = true;
		}

		return ret;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityPlayAnimation(EntityHandle EHandle, const char* Animation, float W)
	{
		auto MeshHandle = GetEntity(EHandle).MeshHandle;
		bool SkeletonLoaded = IsSkeletonLoaded(GT, MeshHandle);
		if (!SkeletonLoaded)
			return false; 

		if (SkeletonLoaded && IsAnimationsLoaded(GT, MeshHandle))
		{
			// TODO: Needs to Iterate Over Clips
			auto S = GetSkeleton(GT, MeshHandle);
			if (!strcmp(S->Animations->Clip.mID, Animation))
			{
				PlayAnimation(&GetEntity(EHandle), GT, Animation, Memory, true);
				return true;
			}
		}

		// Search Resources for Animation
		if(isResourceAvailable(RM, Animation))
		{
			auto RHndl		= LoadGameResource(RM, Animation);
			auto Resource	= GetResource(RM, RHndl);
			if (Resource->Type == EResourceType::EResource_SkeletalAnimation)
			{
				auto AC = Resource2AnimationClip(Resource, Memory);
				FreeResource(RM, RHndl);// No longer in memory once loaded

				auto mesh			 = GetMesh(GT, MeshHandle);
				mesh->AnimationData |= FlexKit::TriMesh::AnimationData::EAD_Skin;
				AC.Skeleton			 = mesh->Skeleton;

				if (AC.Skeleton->Animations)
				{
					auto I = AC.Skeleton->Animations;
					while (I->Next)
						I = I->Next;

					I->Next			= &Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
					I->Next->Clip	= AC;
					I->Next->Memory = Memory;
					I->Next->Next	= nullptr;
				}
				else
				{
					AC.Skeleton->Animations = &Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
					AC.Skeleton->Animations->Clip = AC;
					AC.Skeleton->Animations->Next = nullptr;
					AC.Skeleton->Animations->Memory = Memory;
				}

				PlayAnimation(&GetEntity(EHandle), GT, Animation, Memory, true, W);

				return true;
			}
		}
		return false;
	}


	/************************************************************************************************/


	Drawable& GraphicScene::SetNode(EntityHandle EHandle, NodeHandle Node) 
	{
		FlexKit::FreeHandle(SN, GetNode(EHandle));
		Drawables->at(EHandle).Node = Node;
		return Drawables->at(EHandle);
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(GUID_t Mesh)
	{
		auto EHandle = CreateDrawable();

		auto		Geo = FindMesh(GT, Mesh);
		if (!Geo)	Geo = LoadTriMeshIntoTable(RS, RM, GT, Mesh);

		GetEntity(EHandle).MeshHandle	= Geo;
		GetEntity(EHandle).Dirty		= true;
		GetEntity(EHandle).Visable		= true;
		GetEntity(EHandle).Textured		= false;
		GetEntity(EHandle).Textures		= nullptr;

		return EHandle;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(char* Mesh)
	{
		auto EHandle = CreateDrawable();

		TriMeshHandle MeshHandle = FindMesh(GT, Mesh);
		if (MeshHandle == INVALIDMESHHANDLE)	
			MeshHandle = LoadTriMeshIntoTable(RS, RM, GT, Mesh);

#ifdef _DEBUG
		FK_ASSERT(MeshHandle != INVALIDMESHHANDLE, "FAILED TO FIND MESH IN RESOURCES!");
#endif
		GetEntity(EHandle).MeshHandle = MeshHandle;
		GetEntity(EHandle).Dirty	  = true;
		GetEntity(EHandle).Visable	  = true;
		GetEntity(EHandle).Textured   = false;
		GetEntity(EHandle).Posed	  = false;

		return EHandle;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, NodeHandle Node, float I, float R)
	{
		PLights.push_back({Color, I, R, Node});
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 Color, float3 POS, float I, float R)
	{
		auto Node = GetNewNode(SN);
		SetPositionW(SN, Node, POS);
		PLights.push_back({ Color, I, R, Node });
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(float3 POS, float3 Color, float3 Dir, float t, float I, float R)
	{
		auto Node = GetNewNode(SN);
		SetPositionW(SN, Node, POS);
		SPLights.push_back({Color, Dir, I, R, Node});
		return PLights.size() - 1;
	}

	
	/************************************************************************************************/


	void GraphicScene::_PushEntity(Drawable E)
	{
		if (!Drawables)
			Drawables = new(Memory->_aligned_malloc(sizeof(Drawable) * 64)) fixed_vector<Drawable>(64);

		if (Drawables->full())
		{
			auto Temp		= new(TempMem->_aligned_malloc(sizeof(Drawable)*Drawables->size())) fixed_vector<Drawable>(Drawables->size());
			for (auto e : *Drawables)
				Temp->push_back(e);

			size_t NewSize  = Drawables->size() * 2;
			auto NewArray	= new(Memory->_aligned_malloc(sizeof(Drawable)*NewSize)) fixed_vector<Drawable>(NewSize);
			for (auto e : *Temp)
				NewArray->push_back(e);

			Memory->_aligned_free(Drawables);
			Drawables = NewArray;
		}

		Drawables->push_back(E);
	}


	/************************************************************************************************/


	void InitiateGraphicScene(GraphicScene* Out, RenderSystem* in_RS, Resources* in_RM, SceneNodes* in_SN, GeometryTable* GT, iAllocator* Memory, iAllocator* TempMemory)
	{
		using FlexKit::CreateSpotLightBuffer;
		using FlexKit::CreatePointLightBuffer;
		using FlexKit::PointLightBufferDesc;

		Out->RS = in_RS;
		Out->RM = in_RM;
		Out->SN = in_SN;
		Out->GT = GT;

		Out->Drawables	= nullptr;

		Out->Memory		= Memory;
		Out->TempMem	= TempMemory;

		FlexKit::PointLightBufferDesc Desc;
		Desc.MaxLightCount	= 512;


		CreatePointLightBuffer(in_RS, &Out->PLights, Desc, Memory);
		CreateSpotLightBuffer(in_RS,  &Out->SPLights, Memory);
	
		Out->_PVS.clear();
	}




	/************************************************************************************************/


	void UpdateAnimationsGraphicScene(GraphicScene* SM, double dt)
	{
		if (SM->Drawables)
		{
			for (auto E : *SM->Drawables)
			{
				if (E.AnimationState)
					UpdateAnimation(SM->RS, &E, SM->GT, dt, SM->TempMem);
			}
		}
	}


	/************************************************************************************************/


	void UpdateGraphicScene(GraphicScene* SM)
	{
	}


	/************************************************************************************************/


	void GetGraphicScenePVS(GraphicScene* SM, Camera* C, PVS* __restrict out, PVS* __restrict T_out)
	{
		FK_ASSERT(out != T_out);

		Quaternion Q;
		FlexKit::GetOrientation(SM->SN, C->Node, &Q);
		auto F = GetFrustum(C, FlexKit::GetPositionW(SM->SN, C->Node), Q);

		if (SM->Drawables)
		{
			for (auto &E : *SM->Drawables)
			{
				auto Mesh = GetMesh(SM->GT, E.MeshHandle);
				if (Mesh && CompareAgainstFrustum(&F, FlexKit::GetPositionW(SM->SN, E.Node), Mesh->Info.r))
				{
					if (!E.Transparent)
						PushPV(&E, out);
					else
						PushPV(&E, T_out);
				}
			}	
		}
	}


	/************************************************************************************************/


	void UploadGraphicScene(GraphicScene* SM, PVS* Dawables, PVS* Transparent_PVS)
	{
		UpdatePointLightBuffer	(*SM->RS, SM->SN, &SM->PLights, SM->TempMem);
		UpdateSpotLightBuffer	(*SM->RS, SM->SN, &SM->SPLights, SM->TempMem);

		for (Drawable* d : *Dawables)
			UpdateDrawable(SM->RS, SM->SN, d);

		for (Drawable* t : *Transparent_PVS)
			UpdateDrawable(SM->RS, SM->SN, t);
	}


	/************************************************************************************************/


	void CleanUpSceneAnimation(AnimationClip* AC, iAllocator* Memory)
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


	void CleanUpGraphicScene(GraphicScene* SM)
	{
		if (SM->Drawables) 
		{
			for (auto E : *SM->Drawables)
			{
				ReleaseMesh(SM->GT, E.MeshHandle);
				CleanUpDrawable(&E);

				if (E.PoseState) 
				{
					Destroy(E.PoseState);
					CleanUp(E.PoseState, SM->Memory);
					SM->Memory->_aligned_free(E.PoseState);
					SM->Memory->_aligned_free(E.AnimationState);
				}
			}
			SM->Memory->_aligned_free(SM->Drawables);
		}

		/*
		if (SM->Geo)
		{
			for (auto G : *SM->Geo) 
			{
				CleanUpTriMesh(G);
				if (G->Skeleton) 
				{
					auto I = G->Skeleton->Animations;
					while(I != nullptr)	{
						CleanUpSceneAnimation(&I->Clip, SM->Memory);
						SM->Memory->_aligned_free(I);
						I = I->Next;
					}

					CleanUpSceneSkeleton(G->Skeleton, SM->Memory);
				}
			}
		
			for (auto G : *SM->Geo)
			{
				if(G->ID)
					SM->Memory->_aligned_free((void*)G->ID);
				SM->Memory->_aligned_free(G);
			}

			SM->Memory->_aligned_free(SM->Geo);
		}
		*/

		CleanUp(&SM->PLights, SM->Memory);
		CleanUp(&SM->SPLights, SM->Memory);

		SM->Memory->_aligned_free(SM->PLights.Lights);
		SM->PLights.Lights	= nullptr;
		SM->Drawables		= nullptr;
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

			if (R != nullptr) {
				SceneResourceBlob* SceneBlob = (SceneResourceBlob*)R;
				auto& CreatedNodes = fixed_vector<NodeHandle>::Create(SceneBlob->SceneTable.NodeCount, Temp);

				{
					CompiledScene::SceneNode* Nodes = (CompiledScene::SceneNode*)(SceneBlob->Buffer + SceneBlob->SceneTable.NodeOffset);
					for (size_t I = 0; I < SceneBlob->SceneTable.NodeCount; ++I)
					{
						auto Node		= Nodes[I];
						auto NewNode	= GetNewNode(SN);

						SetOrientationL(SN, NewNode, Node.Q);
						SetPositionL(SN,	NewNode, Node.TS.xyz());

						if (Node.Parent != -1)
							SetParentNode(SN, CreatedNodes[Node.Parent], NewNode);

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
			}
		}

		return false;
	}


	/************************************************************************************************/

}