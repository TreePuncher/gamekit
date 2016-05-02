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

			e.Visable  = false;
			e.Node	   = N;
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
			GetEntity(E).VConstants = nullptr;
			GetEntity(E).Visable	= false;
			GetEntity(E).Mesh		= nullptr;
		}
	}


	/************************************************************************************************/


	bool GraphicScene::isEntitySkeletonAvailable(EntityHandle EHandle)
	{
		if (Drawables->at(EHandle).Mesh)
		{
			auto ID = Drawables->at(EHandle).Mesh->TriMeshID;
			bool Available = isResourceAvailable(RM, ID, EResourceType::EResource_Skeleton);
			return Available;
		}
		return false;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityEnablePosing(EntityHandle EHandle)
	{
		bool Available			= isEntitySkeletonAvailable(EHandle);
		bool SkeletonAvailable	= isResourceAvailable(RM, GetEntity(EHandle).Mesh->TriMeshID, EResource_Skeleton);

		if (Available && SkeletonAvailable)
		{
			if(!GetEntity(EHandle).Mesh->Skeleton)
			{
				auto Handle	= LoadGameResource(RM, GetEntity(EHandle).Mesh->TriMeshID, EResource_Skeleton);
				auto S		= Resource2Skeleton(RM, Handle, Memory);
				GetEntity(EHandle).Mesh->Skeleton = S;
			}

			GetEntity(EHandle).PoseState	= CreatePoseState(&GetEntity(EHandle), Memory);
			GetEntity(EHandle).Posed		= true;
		}
		return Available;
	}


	/************************************************************************************************/


	bool GraphicScene::EntityPlayAnimation(EntityHandle EHandle, const char* Animation)
	{
		if (!GetEntity(EHandle).Mesh->Skeleton)
			return false; 
		if (GetEntity(EHandle).Mesh->Skeleton && GetEntity(EHandle).Mesh->Skeleton->Animations)
		{
			if (!strcmp(GetEntity(EHandle).Mesh->Skeleton->Animations->Clip.mID, Animation))
			{
				PlayAnimation(&GetEntity(EHandle), Animation, Memory, true);
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

				auto mesh			 = GetEntity(EHandle).Mesh;
				mesh->AnimationData |= FlexKit::TriMesh::AnimationData::EAD_Skin;
				AC.Skeleton			 = mesh->Skeleton;

				if (AC.Skeleton->Animations)
				{
					auto I = AC.Skeleton->Animations;
					while (I->Next)
						I = I->Next;

					I->Next = &Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
					I->Next->Clip = AC;
					I->Next->Next = nullptr;
				}
				else
				{
					AC.Skeleton->Animations = &Memory->allocate_aligned<Skeleton::AnimationList, 0x10>();
					AC.Skeleton->Animations->Clip = AC;
					AC.Skeleton->Animations->Next = nullptr;
				}

				PlayAnimation(&GetEntity(EHandle), Animation, Memory, true);

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


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(ShaderSetHandle M, GUID_t Mesh)
	{
		auto EHandle = CreateDrawable();

		TriMesh*  Geo = FindMesh(Mesh);
		if (!Geo) Geo = LoadMesh(Mesh, M);

		GetEntity(EHandle).Material		= M;
		GetEntity(EHandle).Mesh			= Geo;
		GetEntity(EHandle).Dirty		= true;
		GetEntity(EHandle).Visable		= true;

		return EHandle;
	}


	/************************************************************************************************/


	EntityHandle GraphicScene::CreateDrawableAndSetMesh(ShaderSetHandle M, char* Mesh)
	{
		auto EHandle = CreateDrawable();

		TriMesh* Geo = FindMesh(Mesh);
		if(!Geo) Geo = LoadMesh(Mesh, M);

		GetEntity(EHandle).Material = M;
		GetEntity(EHandle).Mesh		= Geo;
		GetEntity(EHandle).Dirty	= true;
		GetEntity(EHandle).Visable	= true;

		return EHandle;
	}


	/************************************************************************************************/


	LightHandle GraphicScene::AddPointLight(float3 POS, float3 Color, float I, float R)
	{
		auto Node = GetNewNode(SN);
		SetPositionW(SN, Node, POS);
		PLights.push_back({Color, I, R, Node});
		return LightHandle(PLights.size() - 1);
	}


	/************************************************************************************************/


	SpotLightHandle GraphicScene::AddSpotLight(float3 POS, float3 Color, float3 Dir, float t, float I, float R)
	{
		auto Node = GetNewNode(SN);
		SetPositionW(SN, Node, POS);
		SPLights.push_back({Color, I, R, Node});
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


	void GraphicScene::PushMesh(TriMesh* M)
	{
		if (!Geo)
			Geo = new(Memory->_aligned_malloc(sizeof(void*) * 64)) fixed_vector<TriMesh*>(64);

		if (Geo->full())
		{
			auto Temp		= new(TempMem->_aligned_malloc(sizeof(void*)*Geo->size())) fixed_vector<TriMesh*>(Geo->size());
			for (auto e : *Geo)
				Temp->push_back(e);

			size_t NewSize  = Geo->size() * 2;
			auto NewArray	= new(TempMem->_aligned_malloc(sizeof(void*)*NewSize)) fixed_vector<TriMesh*>(NewSize);
			for (auto e : *Temp)
				NewArray->push_back(e);

			Memory->_aligned_free(Geo);
			Geo = NewArray;
		}

		Geo->push_back(M);
	}


	/************************************************************************************************/


	TriMesh* GraphicScene::FindMesh(GUID_t ID)
	{
		if(Geo)
		for (auto M : *Geo)
			if (M->TriMeshID == ID)
				return M;

		return nullptr;
	}


	/************************************************************************************************/


	TriMesh* GraphicScene::FindMesh(const char* ID)
	{
		if(Geo)
		for (auto M : *Geo)
			if (M->ID && !strcmp(M->ID,ID))
				return M;

		return nullptr;
	}


	/************************************************************************************************/


	TriMesh* GraphicScene::LoadMesh(GUID_t ID, ShaderSetHandle M)
	{
		auto res = LoadGameResource(RM, ID, EResourceType::EResource_TriMesh);
		if (res != INVALIDHANDLE) {
			auto TriMesh = Resource2TriMesh(RS, RM, res, Memory, M, ST);
			TriMesh->TriMeshID = ID;
			PushMesh(TriMesh);
			return Geo->back();
		}
		return nullptr;
	}


	/************************************************************************************************/


	TriMesh* GraphicScene::LoadMesh(const char* ID, ShaderSetHandle M)
	{
		auto res = LoadGameResource(RM, ID);
		if (res != INVALIDHANDLE) {
			PushMesh(Resource2TriMesh(RS, RM, res, Memory, M, ST));
			return Geo->back();
		}
		return nullptr;
	}


	/************************************************************************************************/


	void InitiateGraphicScene(GraphicScene* Out, RenderSystem* in_RS, ShaderTable* in_ST, Resources* in_RM, SceneNodes* in_SN, iAllocator* Memory, iAllocator* TempMemory)
	{
		using FlexKit::CreateSpotLightBuffer;
		using FlexKit::CreatePointLightBuffer;
		using FlexKit::PointLightBufferDesc;

		Out->ST = in_ST;
		Out->RS = in_RS;
		Out->RM = in_RM;
		Out->SN = in_SN;

		Out->Drawables	= nullptr;
		Out->Geo		= nullptr;

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
					UpdateAnimation(SM->RS, &E, dt, SM->TempMem);
			}
		}
	}

	/************************************************************************************************/


	void UpdateGraphicScene(GraphicScene* SM)
	{
		for(auto& E : *SM->Drawables)
			UpdateDrawable(SM->RS, SM->SN, SM->ST, &E);
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
				if (E.Mesh && CompareAgainstFrustum(&F, FlexKit::GetPositionW(SM->SN, E.Node), E.Mesh->Info.r))
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


	void UpdateGraphicScene_PreDraw(GraphicScene* SM)
	{
		UpdatePointLightBuffer	(*SM->RS, SM->SN, &SM->PLights, SM->TempMem);
		UpdateSpotLightBuffer	(*SM->RS, SM->SN, &SM->SPLights, SM->TempMem);
	}


	/************************************************************************************************/

	void CleanUpSceneSkeleton(Skeleton* S, iAllocator* Mem)
	{
		for (size_t I = 0; I < S->JointCount; ++I)
			if(S->Joints[I].mID) Mem->free((void*)S->Joints[I].mID);

		Mem->free((void*)S->Joints);
		Mem->free((void*)S->JointPoses);
		Mem->free((void*)S->IPose);
		Mem->free(S);
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

		CleanUp(&SM->PLights, SM->Memory);
		CleanUp(&SM->SPLights, SM->Memory);

		SM->Memory->_aligned_free(SM->PLights.Lights);
		SM->PLights.Lights	= nullptr;
		SM->Drawables		= nullptr;
		SM->Geo				= nullptr;
	}


	/************************************************************************************************/
}