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

#ifndef GraphicScene_H
#define GraphicScene_H

#include "..\buildsettings.h"
#include "..\coreutilities\Resources.h"
#include "..\graphicsutilities\AnimationUtilities.h" 
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{
	typedef size_t EntityHandle;
	typedef size_t SpotLightHandle;

	struct FLEXKITAPI GraphicScene
	{
		EntityHandle CreateDrawable();
		void		 RemoveEntity(EntityHandle E);

		inline Drawable&	GetEntity			(EntityHandle EHandle) {return Drawables->at(EHandle);						}
		inline float4		GetMaterialColour	(EntityHandle EHandle) {return Drawables->at(EHandle).MatProperties.Albedo;	}
		inline float4		GetMaterialSpec		(EntityHandle EHandle) {return Drawables->at(EHandle).MatProperties.Spec;	}
		inline NodeHandle	GetNode				(EntityHandle EHandle) {return Drawables->at(EHandle).Node;					}
		inline float3		GetEntityPosition	(EntityHandle EHandle) {return GetPositionW(SN, Drawables->at(EHandle).Node);}

		bool isEntitySkeletonAvailable	(EntityHandle EHandle);
		bool EntityEnablePosing			(EntityHandle EHandle);
		bool EntityPlayAnimation		(EntityHandle EHandle, const char* Animation, float w = 1.0f);

		inline void SetVisability		(EntityHandle EHandle, bool Visable = true)	{Drawables->at(EHandle).Visable = Visable; }
		inline void SetMesh				(EntityHandle EHandle, TriMesh* M)			{Drawables->at(EHandle).Mesh = M;		  }
		inline void SetMaterial			(EntityHandle EHandle, ShaderSetHandle M)	{Drawables->at(EHandle).Material = M;	  }
		inline void SetMaterialParams	(EntityHandle EHandle, float4 RGBA = float4(1.0f, 1.0f, 1.0f, 0.5f), float4 Spec = float4(1.0f, 1.0f, 1.0f, 0.5f))
		{
			Drawables->at(EHandle).OverrideMaterialProperties = true;
			Drawables->at(EHandle).MatProperties.Albedo		 = RGBA;
			Drawables->at(EHandle).MatProperties.Spec		 = Spec;
		}

		Drawable& SetNode(EntityHandle EHandle, NodeHandle Node);

		EntityHandle CreateDrawableAndSetMesh(ShaderSetHandle M, GUID_t Mesh);
		EntityHandle CreateDrawableAndSetMesh(ShaderSetHandle M, char* Mesh);
		
		LightHandle		AddPointLight(float3 POS, float3 Color, float I = 10, float R = 10);
		SpotLightHandle AddSpotLight(float3 POS, float3 Color, float3 Dir, float t = pi / 4, float I = 10, float R = 10);

		void _PushEntity(Drawable E);
		void PushMesh	(TriMesh* M);

		inline void Yaw					(EntityHandle Handle, float a)			{ FlexKit::Yaw	(SN, GetEntity(Handle).Node, a); }
		inline void Pitch				(EntityHandle Handle, float a)			{ FlexKit::Pitch(SN, GetEntity(Handle).Node, a); }
		inline void Roll				(EntityHandle Handle, float a)			{ FlexKit::Roll	(SN, GetEntity(Handle).Node, a); }
		inline void TranslateEntity_LT	(EntityHandle Handle, float3 V)			{ FlexKit::TranslateLocal	(SN, GetEntity(Handle).Node, V);  GetEntity(Handle).Dirty = true;}
		inline void TranslateEntity_WT	(EntityHandle Handle, float3 V)			{ FlexKit::TranslateWorld	(SN, GetEntity(Handle).Node, V);  GetEntity(Handle).Dirty = true;}
		inline void SetPositionEntity_WT(EntityHandle Handle, float3 V)			{ FlexKit::SetPositionW		(SN, GetEntity(Handle).Node, V);  GetEntity(Handle).Dirty = true;}
		inline void SetPositionEntity_LT(EntityHandle Handle, float3 V)			{ FlexKit::SetPositionL		(SN, GetEntity(Handle).Node, V);  GetEntity(Handle).Dirty = true;}
		inline void SetOrientation		(EntityHandle Handle, Quaternion Q)		{ FlexKit::SetOrientation	(SN, GetEntity(Handle).Node, Q);  GetEntity(Handle).Dirty = true;}
		inline void SetOrientationL		(EntityHandle Handle, Quaternion Q)		{ FlexKit::SetOrientationL	(SN, GetEntity(Handle).Node, Q);  GetEntity(Handle).Dirty = true;}

		inline void SetLightNodeHandle	(SpotLightHandle Handle, NodeHandle Node)	{ FreeHandle(SN, PLights[Handle].Position); PLights[Handle].Position = Node; }

		TriMesh* FindMesh(GUID_t ID);
		TriMesh* FindMesh(const char* ID);

		TriMesh* LoadMesh(GUID_t ID, ShaderSetHandle M);
		TriMesh* LoadMesh(const char* ID, ShaderSetHandle M);

		fixed_vector<Drawable>*	Drawables;
		fixed_vector<TriMesh*>*	Geo;
		fixed_vector<size_t>*	FreeEntityList;
		iAllocator*				TempMem;
		iAllocator*				Memory;
		RenderSystem*			RS;
		ShaderTable*			ST;
		Resources*				RM;
		SceneNodes*				SN;

		PointLightBuffer		PLights;
		SpotLightBuffer			SPLights;
		PVS						_PVS;
	};


	FLEXKITAPI void InitiateGraphicScene			(GraphicScene* Out, RenderSystem* in_RS, ShaderTable* in_ST, Resources* in_RM, SceneNodes* in_SN, iAllocator* Memory, iAllocator* TempMemory);

	FLEXKITAPI void UpdateGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void UpdateAnimationsGraphicScene	(GraphicScene* SM);
	FLEXKITAPI void GetGraphicScenePVS				(GraphicScene* SM, Camera* C, PVS* __restrict out, PVS* __restrict T_out);
	FLEXKITAPI void UploadGraphicScene				(GraphicScene* SM, PVS* , PVS* );

	FLEXKITAPI void CleanUpGraphicScene				(GraphicScene* SM);
	FLEXKITAPI void CleanUpSceneSkeleton			(Skeleton* S, iAllocator* Mem);

	template<typename ALLOC_T>
	void CleanUp(DrawablePoseState* EPS, ALLOC_T* Mem)
	{
		if(EPS->Joints)			Mem->free(EPS->Joints);
		if(EPS->CurrentPose)	Mem->free(EPS->CurrentPose);

		EPS->Joints		 = nullptr;
		EPS->CurrentPose = nullptr;
	}

}

#endif