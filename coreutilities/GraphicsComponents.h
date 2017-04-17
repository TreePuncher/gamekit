#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

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

	struct TransformComponentArgs
	{
		SceneNodeComponentSystem*	Nodes;
		NodeHandle					Node;
	};


	void CreateComponent(GameObject<>& GO, SceneNodeComponentSystem* Nodes)
	{
		if (!GO.Full())
		{
			auto NodeHandle = Nodes->GetNewNode();
			GO.Components[GO.ComponentCount++] = Component(Nodes, NodeHandle, CT_Transform);
		}
	}


	void CreateComponent(GameObject<>& GO, TransformComponentArgs Args)
	{
		if (!GO.Full())
		{
			NodeHandle Handle = Args.Node;
			if (Args.Node.INDEX == INVALIDHANDLE)
				Handle = Args.Nodes->GetNewNode();

			GO.Components[GO.ComponentCount++] = Component(Args.Nodes, Handle, CT_Transform);
		}
	}


	float3 GetWorldPosition(GameObject<>& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		return C->GetWorldPosition();
	}


	float3 GetLocalPosition(GameObject<>& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		return C->GetLocalPosition();
	}


	NodeHandle GetNodeHandle(GameObject<>& GO)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if(C)
			return C->ComponentHandle;
		return NodeHandle(-1);
	}


	void SetLocalPosition(GameObject<>& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if (C) {
			C->SetLocalPosition(XYZ);
			GO.NotifyAll(CT_Transform, GetCRCGUID(POSITION));
		}
	}


	void SetWorldPosition(GameObject<>& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		if (C) {
			C->SetWorldPosition(XYZ);
			GO.NotifyAll(CT_Transform, GetCRCGUID(POSITION));
		}
	}


	void Yaw(GameObject<>& GO, float R)
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


	inline bool SetDrawableColor(GameObject<>& GO, float3 Color)
	{
		auto C = GO.FindComponent(ComponentType::CT_Renderable);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetColor(C->ComponentHandle, Color);
			GO.NotifyAll(CT_Renderable, GetCRCGUID(MATERIAL));
		}
		return (C != nullptr);
	}

	inline bool SetDrawableMetal(GameObject<>& GO, bool M)
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

	template<typename TY_GO>
	void CreateComponent(TY_GO& GO, DrawableComponentArgs& Args)
	{
		if (!GO.Full()) {
			auto C = GO.FindComponent(ComponentType::CT_Transform);
			if (C)
				Args.System->SetNode(Args.Entity, C->ComponentHandle);
			else
				CreateComponent(GO, TransformComponentArgs{ Args.System->SceneNodes, Args.System->GetNode(Args.Entity) });

			GO.Components[GO.ComponentCount++] = Component(Args.System, Args.Entity, CT_Renderable);
		}
	}


	/************************************************************************************************/


	struct FLEXKITAPI LightComponentSystem : public ComponentSystemInterface
	{
	public:
		void InitiateSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes)
		{
			new(this) LightComponentSystem();

			Scene		= IN_Scene;
			SceneNodes	= IN_SceneNodes;
		}

		void ReleaseHandle(ComponentHandle Handle) final
		{
			Scene->RemoveEntity(Handle);
		}

		void SetNode(ComponentHandle Light, NodeHandle Node)
		{
			Scene->SetLightNodeHandle(Light, Node);
		}

		void SetColor(ComponentHandle Light, float3 NewColor)
		{
			//Scene->Set(Drawable, Node);
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


	LightComponentArgs CreateLightComponent( LightComponentSystem* LightSystem, float3 Color = float3(1.0f, 1.0f, 1.0f), float I = 100.0f, float R = 10000.0f )
	{
		return { NodeHandle(-1), LightSystem, Color, I, R};
	}


	template<typename TY_GO>
	void CreateComponent(TY_GO& GO, LightComponentArgs& Args)
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
			GO.Components[GO.ComponentCount++] = Component(Args.System, Light, CT_PointLight);
		}
	}
}//namespace FlexKit

#endif