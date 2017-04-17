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

		operator SceneNodes* ()					{ return &Nodes; }
		operator ComponentSystemInterface* ()	{ return this; }
	};

	struct FLEXKITAPI TansformComponent : public Component
	{
		void Roll(float r);
		void Pitch(float r);
		void Yaw(float r);

		float3 GetLocalPosition();
		float3 GetWorldPosition();

		void SetLocalPosition(const float3&);
		void SetWorldPosition(const float3&);

		Quaternion	GetOrientation();
		void		SetOrientation(FlexKit::Quaternion Q);

		static void Release(Component* Component)
		{
			ReleaseNode((SceneNodes*)Component->ComponentSystem, Component->ComponentHandle);
			Component->ReleaseComponent = nullptr;
		}
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
			GO.Components[GO.ComponentCount++] = Component(Nodes, TansformComponent::Release, NodeHandle, CT_Transform);
		}
	}


	void CreateComponent(GameObject<>& GO, TransformComponentArgs Args)
	{
		if (!GO.Full())
		{
			NodeHandle Handle = Args.Node;
			if (Args.Node.INDEX == INVALIDHANDLE)
				Handle = Args.Nodes->GetNewNode();

			GO.Components[GO.ComponentCount++] = Component(Args.Nodes, TansformComponent::Release, Handle, CT_Transform);
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


	void SetLocalPosition(GameObject<>& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		C->SetLocalPosition(XYZ);
	}


	void SetWorldPosition(GameObject<>& GO, float3 XYZ)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		C->SetWorldPosition(XYZ);
	}


	void Yaw(GameObject<>& GO, float R)
	{
		auto C = (TansformComponent*)GO.FindComponent(CT_Transform);
		C->Yaw(R);
	}

	/************************************************************************************************/


	struct EntityComponentArgs
	{
		EntityHandle	Entity;
		GraphicScene*	GraphicScene;
	};

	EntityComponentArgs CreateEnityComponent( GraphicScene*	GraphicScene, const char* Mesh )
	{
		return { GraphicScene->CreateDrawableAndSetMesh(Mesh), GraphicScene };
	}

	struct FLEXKITAPI EntityComponent : public Component
	{
		static void Release(Component* Component)
		{
			auto* System = (GraphicScene*)Component->ComponentSystem;
			System->RemoveEntity(Component->ComponentHandle);
			Component->ReleaseComponent = nullptr;
		}
	};

	void CreateComponent(GameObject<>& GO, EntityComponentArgs& Args)
	{
		if (!GO.Full()) {
			auto C = GO.FindComponent(ComponentType::CT_Transform);
			if (C)
				Args.GraphicScene->SetNode(Args.Entity, C->ComponentHandle);
			else
				CreateComponent(GO, TransformComponentArgs{ Args.GraphicScene->SN, Args.GraphicScene->GetNode(Args.Entity) });

			//GO.Components[GO.ComponentCount++] = Component(Args.GraphicScene, &EntityComponent::Release, Args.Entity, CT_Renderable);
		}
	}

}//namespace FlexKit

#endif