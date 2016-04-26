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

#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "buildsettings.h"
#include "coreutilities\ResourceManager.h"
#include "graphicsutilities\AnimationUtilities.h"
#include "graphicsutilities\graphics.h"

namespace FlexKit
{
	typedef size_t EntityHandle;
	typedef size_t SpotLightHandle;

	struct FLEXKITAPI SceneManager
	{
		EntityHandle CreateEntity();
		void		 RemoveEntity(EntityHandle E);

		inline Entity&		GetEntity			(EntityHandle EHandle) {return Entities->at(EHandle);						}
		inline float4		GetMaterialColour	(EntityHandle EHandle) {return Entities->at(EHandle).MatProperties.Albedo;	}
		inline float4		GetMaterialSpec		(EntityHandle EHandle) {return Entities->at(EHandle).MatProperties.Spec;	}
		inline NodeHandle	GetNode				(EntityHandle EHandle) {return Entities->at(EHandle).Node;					}
		inline float3		GetEntityPosition	(EntityHandle EHandle) {return GetPositionW(SN, Entities->at(EHandle).Node);}

		bool isEntitySkeletonAvailable	(EntityHandle EHandle);
		bool EntityEnablePosing			(EntityHandle EHandle);
		bool EntityPlayAnimation		(EntityHandle EHandle, const char* Animation);

		inline void SetVisability		(EntityHandle EHandle, bool Visable = true)	{Entities->at(EHandle).Visable = Visable; }
		inline void SetMesh				(EntityHandle EHandle, TriMesh* M)			{Entities->at(EHandle).Mesh = M;		  }
		inline void SetMaterial			(EntityHandle EHandle, ShaderSetHandle M)	{Entities->at(EHandle).Material = M;	  }
		inline void SetMaterialParams	(EntityHandle EHandle, float4 RGBA = float4(1.0f, 1.0f, 1.0f, 0.5f), float4 Spec = float4(1.0f, 1.0f, 1.0f, 0.5f))
		{
			Entities->at(EHandle).OverrideMaterialProperties = true;
			Entities->at(EHandle).MatProperties.Albedo		 = RGBA;
			Entities->at(EHandle).MatProperties.Spec		 = Spec;
		}

		Entity& SetNode(EntityHandle EHandle, NodeHandle Node);

		EntityHandle CreateEntityAndSetMesh(ShaderSetHandle M, GUID_t Mesh);
		EntityHandle CreateEntityAndSetMesh(ShaderSetHandle M, char* Mesh);
		
		LightHandle		AddPointLight(float3 POS, float3 Color, float I = 10, float R = 10);
		SpotLightHandle AddSpotLight(float3 POS, float3 Color, float3 Dir, float t = pi / 4, float I = 10, float R = 10);

		void _PushEntity(Entity E);
		void PushMesh	(TriMesh* M);

		inline void TranslateEntity_LT	(size_t EntityHandle, float3 V)			{ FlexKit::TranslateLocal	(SN, (*Entities)[EntityHandle].Node, V);}
		inline void TranslateEntity_WT	(size_t EntityHandle, float3 V)			{ FlexKit::TranslateWorld	(SN, (*Entities)[EntityHandle].Node, V);}
		inline void SetPositionEntity_WT(size_t EntityHandle, float3 V)			{ FlexKit::SetPositionW		(SN, (*Entities)[EntityHandle].Node, V);}
		inline void SetPositionEntity_LT(size_t EntityHandle, float3 V)			{ FlexKit::SetPositionL		(SN, (*Entities)[EntityHandle].Node, V);}
		inline void SetLightNodeHandle	(size_t EntityHandle, NodeHandle Node)	{ FreeHandle(SN, PLights[EntityHandle].Position); PLights[EntityHandle].Position = Node; }

		TriMesh* FindMesh(GUID_t ID);
		TriMesh* FindMesh(const char* ID);

		TriMesh* LoadMesh(GUID_t ID, ShaderSetHandle M);
		TriMesh* LoadMesh(const char* ID, ShaderSetHandle M);

		fixed_vector<Entity>*	Entities;
		fixed_vector<TriMesh*>*	Geo;
		fixed_vector<size_t>*	FreeEntityList;
		StackAllocator*			TempMem;
		BlockAllocator*			Memory;
		RenderSystem*			RS;
		ShaderTable*			ST;
		ResourceManager*		RM;
		SceneNodes*				SN;

		PointLightBuffer		PLights;
		SpotLightBuffer			SPLights;
		PVS						_PVS;
	};


	FLEXKITAPI void InitiateSceneManager(SceneManager* Out, RenderSystem* in_RS, ShaderTable* in_ST, ResourceManager* in_RM, SceneNodes* in_SN, BlockAllocator* Memory, StackAllocator* TempMemory);

	FLEXKITAPI void UpdateAnimationsSceneManager(SceneManager* SM, Camera* C, double dt);
	FLEXKITAPI void UpdateSceneManager(SceneManager* SM, Camera* C, PVS& out);
	FLEXKITAPI void UpdateSceneManager_PreDraw(SceneManager* SM);

	FLEXKITAPI void CleanUpSceneManager(SceneManager* SM);

	template<typename ALLOC_T>
	void CleanUp(EntityPoseState* EPS, ALLOC_T* Mem)
	{
		if(EPS->Joints)			Mem->free(EPS->Joints);
		if(EPS->CurrentPose)	Mem->free(EPS->CurrentPose);

		EPS->Joints		 = nullptr;
		EPS->CurrentPose = nullptr;
	}

}

#endif