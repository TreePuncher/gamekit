#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

/**********************************************************************

Copyright (c) 2017 Robert May

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

#include "..\coreutilities\Components.h"
#include "..\coreutilities\MathUtils.h"
#include "..\graphicsutilities\graphics.h"

namespace FlexKit
{
	/************************************************************************************************/
	// Transform Functions
	// Component Derivatives 
	// only define Methods
	// All data is fetched using handle and System Pointer

	struct FLEXKITAPI SceneNodeComponentSystem : public ComponentSystemInterface
	{
	public:
		void InitiateSystem(byte* Memory, size_t BufferSize);

		// Interface Methods
		void ReleaseHandle(ComponentHandle Handle) final;

		NodeHandle GetRoot();
		NodeHandle GetNewNode();

		NodeHandle Root;
		SceneNodes Nodes;

		void SetParentNode(NodeHandle Parent, NodeHandle Node)
		{
			FlexKit::SetParentNode(Nodes, Parent, Node);
		}

		operator SceneNodes* ()					{ return &Nodes; }
		operator ComponentSystemInterface* ()	{ return this; }
		operator SceneNodeComponentSystem* ()	{ return this; }
	};


	struct FLEXKITAPI TansformComponent : public Component
	{
		void Roll	(float r);
		void Pitch	(float r);
		void Yaw	(float r);

		float3 GetLocalPosition();
		float3 GetWorldPosition();

		void SetLocalPosition(const float3&);
		void SetWorldPosition(const float3&);

		Quaternion	GetOrientation();
		void		SetOrientation(FlexKit::Quaternion Q);
		void		SetParentNode(NodeHandle Parent, NodeHandle Node);
	};


	/************************************************************************************************/


	struct TransformComponentArgs
	{
		SceneNodeComponentSystem*	Nodes;
		NodeHandle					Node;
	};

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, SceneNodeComponentSystem* Nodes)
	{
		if (!GO.Full())
		{
			auto NodeHandle = Nodes->GetNewNode();
			GO.AddComponent(Component(Nodes, NodeHandle, CT_Transform));
		}
	}


	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, TransformComponentArgs Args)
	{
		if (!GO.Full())
		{
			NodeHandle Handle = Args.Node;
			if (Args.Node.INDEX == INVALIDHANDLE)
				Handle = Args.Nodes->GetNewNode();
			
			GO.AddComponent(Component(Args.Nodes, Handle, CT_Transform));
		}
	}


	template<typename TY_GO>
	float3 GetWorldPosition(TY_GO& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		return C->GetWorldPosition();
	}


	template<typename TY_GO>
	float3 GetLocalPosition(TY_GO& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		return C->GetLocalPosition();
	}


	template<typename TY_GO>
	NodeHandle GetNodeHandle(TY_GO& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if(C)
			return C->ComponentHandle;
		return NodeHandle(-1);
	}


	template<typename TY_GO>
	void SetLocalPosition(TY_GO& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if (C) {
			C->SetLocalPosition(XYZ);
			GO.NotifyAll(CT_Transform, GetCRCGUID(POSITION));
		}
	}


	template<typename TY_GO>
	void SetWorldPosition(TY_GO& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if (C) {
			C->SetWorldPosition(XYZ);
			GO.NotifyAll(CT_Transform, GetCRCGUID(POSITION));
		}
	}


	template<typename TY_GO>
	void Yaw(TY_GO& GO, float R)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if (C) {
			C->Yaw(R);
			GO.NotifyAll(CT_Transform, GetCRCGUID(ORIENTATION));
		}
	}


	/************************************************************************************************/


	struct FLEXKITAPI DrawableComponentSystem : public ComponentSystemInterface
	{
	public:
		void InitiateSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes)
		{
			new(this) DrawableComponentSystem();

			Scene		= IN_Scene;
			SceneNodes	= IN_SceneNodes;
		}

		void ReleaseHandle(ComponentHandle Handle) final
		{
			Scene->RemoveEntity(Handle);
		}

		void SetNode(ComponentHandle Drawable, NodeHandle Node)
		{
			Scene->SetNode(Drawable, Node);
		}

		void SetColor(ComponentHandle Drawable, float3 NewColor)
		{
			auto DrawableColor = Scene->GetMaterialColour(Drawable);
			auto DrawableSpec  = Scene->GetMaterialSpec(Drawable);

			DrawableColor.x = NewColor.x;
			DrawableColor.y = NewColor.y;
			DrawableColor.z = NewColor.z;
			// W cord the Metal Factor

			Scene->SetMaterialParams(Drawable, DrawableColor, DrawableSpec);
		}

		void SetMetal(ComponentHandle Drawable, bool M)
		{
			auto DrawableColor = Scene->GetMaterialColour(Drawable);
			auto DrawableSpec = Scene->GetMaterialSpec(Drawable);

			DrawableColor.w = 1.0f * M;

			Scene->SetMaterialParams(Drawable, DrawableColor, DrawableSpec);
		}


		NodeHandle GetNode(ComponentHandle Drawable)
		{
			return Scene->GetNode(Drawable);
		}

		operator DrawableComponentSystem* ()	{ return this;   }
		operator GraphicScene* ()				{ return *Scene; }

		GraphicScene*				Scene;
		SceneNodeComponentSystem*	SceneNodes;	// A Source System
	};


	template<typename TY_GO>
	inline bool SetDrawableColor(TY_GO& GO, float3 Color)
	{
		auto C = GO.FindComponent(ComponentType::CT_Renderable);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetColor(C->ComponentHandle, Color);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(MATERIAL));
		}
		return (C != nullptr);
	}

	template<typename TY_GO>
	inline bool SetDrawableMetal(TY_GO& GO, bool M)
	{
		auto C = GO.FindComponent(ComponentType::CT_Renderable);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetMetal(C->ComponentHandle, M);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(MATERIAL));
		}
		return (C != nullptr);
	}

	struct DrawableComponentArgs
	{
		EntityHandle				Entity;
		DrawableComponentSystem*	System;
	};

	DrawableComponentArgs CreateEnityComponent(DrawableComponentSystem*	DrawableComponent, const char* Mesh )
	{
		return { DrawableComponent->Scene->CreateDrawableAndSetMesh(Mesh), DrawableComponent };
	}

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, DrawableComponentArgs& Args)
	{
		if (!GO.Full()) {
			auto C = GO.FindComponent(ComponentType::CT_Transform);
			if (C)
				Args.System->SetNode(Args.Entity, C->ComponentHandle);
			else
				CreateComponent(GO, TransformComponentArgs{ Args.System->SceneNodes, Args.System->GetNode(Args.Entity) });

			GO.AddComponent(Component(Args.System, Args.Entity, CT_Renderable));
		}
	}


	/************************************************************************************************/


	struct FLEXKITAPI LightComponentSystem : public ComponentSystemInterface
	{
	public:
		void InitiateSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes)
		{
			new(this) LightComponentSystem();// Setups VTable

			Scene		= IN_Scene;
			SceneNodes	= IN_SceneNodes;
		}

		void ReleaseHandle(ComponentHandle Handle) final
		{
			ReleaseLight(&Scene->PLights, Handle);
		}

		void SetNode(ComponentHandle Light, NodeHandle Node)
		{
			Scene->SetLightNodeHandle(Light, Node);
		}

		void SetColor(ComponentHandle Light, float3 NewColor)
		{
			Scene->PLights[Light].K = NewColor;
		}

		void SetIntensity(ComponentHandle Light, float I)
		{
			Scene->PLights[Light].I = I;
		}

		void SetRadius(ComponentHandle Light, float R)
		{
			Scene->PLights[Light].R = R;
		}

		operator LightComponentSystem* ()	{ return this; }
		operator GraphicScene* ()			{ return *Scene; }

		GraphicScene*				Scene;
		SceneNodeComponentSystem*	SceneNodes;	// A Source System
	};

	struct LightComponentArgs
	{
		NodeHandle				Node;
		LightComponentSystem*	System;
		float3					Color;
		float					I;
		float					R;
	};

	template<typename T_GO>
	bool SetLightColor(T_GO& GO, float3 K)
	{
		auto C = GO.FindComponent(ComponentType::CT_PointLight);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetColor(C->ComponentHandle, K);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}

	template<typename T_GO>
	bool SetLightIntensity(T_GO& GO, float I)
	{
		auto C = GO.FindComponent(ComponentType::CT_PointLight);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetIntensity(C->ComponentHandle, I);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}

	template<typename T_GO>
	bool SetLightRadius(T_GO& GO, float R)
	{
		auto C = GO.FindComponent(ComponentType::CT_PointLight);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetRadius(C->ComponentHandle, R);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}


	/************************************************************************************************/


	LightComponentArgs CreateLightComponent( LightComponentSystem* LightSystem, float3 Color = float3(1.0f, 1.0f, 1.0f), float I = 100.0f, float R = 10000.0f )
	{
		return { NodeHandle(-1), LightSystem, Color, I, R};
	}


	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, LightComponentArgs& Args)
	{
		if (!GO.Full()) {
			NodeHandle Node;

			if (Args.Node == NodeHandle(-1))
			{
				auto C = GO.FindComponent(ComponentType::CT_Transform);

				if (C)
					Node = C->ComponentHandle;
				else
					Node = Args.System->SceneNodes->GetNewNode();
			}
			else
				Node = Args.Node;

			LightHandle Light = Args.System->Scene->AddPointLight(Args.Color, Node,  Args.I, Args.R);
			GO.AddComponent(Component(Args.System, Light, CT_PointLight));
		}
	}

	/************************************************************************************************/
}//namespace FlexKit

#endif