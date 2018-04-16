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
#include "..\graphicsutilities\ImmediateRendering.h"
#include "..\graphicsutilities\CoreSceneObjects.h"

namespace FlexKit
{
	// Pre-Declarations
	struct GraphicScene;

	/************************************************************************************************/
	// Transform Functions
	// Component Derivatives 
	// only define Methods
	// All data is fetched using handle and System Pointer

	struct FLEXKITAPI SceneNodeComponentSystem : public ComponentSystemInterface
	{
	public:
		SceneNodeComponentSystem(byte* Memory, size_t BufferSize)
		{
			InitiateSceneNodeBuffer(&Nodes, Memory, BufferSize);
			Root = FlexKit::GetZeroedNode(Nodes);
		}

		// Interface Methods
		void Release();
		void ReleaseHandle(ComponentHandle Handle) final;

		NodeHandle GetRoot();
		NodeHandle GetNewNode();
		NodeHandle GetZeroedNode();

		NodeHandle Root;
		SceneNodes Nodes;

		void SetParentNode	(NodeHandle Parent, NodeHandle Node)
		{
			FlexKit::SetParentNode(Nodes, Parent, Node);
		}
		
		float3 GetLocalScale(NodeHandle Node)
		{
			return FlexKit::GetLocalScale(Nodes, Node);
		}

		float3 GetPositionW	(NodeHandle Node)
		{
			return FlexKit::GetPositionW(Nodes, Node);
		}

		float3 GetPositionL(NodeHandle Node)
		{
			return FlexKit::GetPositionL(Nodes, Node);
		}

		void SetPositionW(NodeHandle Node, float3 xyz)
		{
			FlexKit::SetPositionW(Nodes, Node, xyz);
		}

		void SetPositionL(NodeHandle Node, float3 xyz)
		{
			FlexKit::SetPositionL(Nodes, Node, xyz);
		}

		Quaternion GetOrientation(NodeHandle Node)
		{
			return FlexKit::GetOrientation(Nodes, Node);
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

		void Translate(const float3 xyz);
		void TranslateWorld(const float3 xyz);

		void SetLocalPosition(const float3&);
		void SetWorldPosition(const float3&);

		Quaternion	GetOrientation();
		void		SetOrientation(FlexKit::Quaternion Q);
		void		SetParentNode(NodeHandle Parent, NodeHandle Node);

		NodeHandle	GetParentNode(NodeHandle Node);
		NodeHandle	GetNode();
	};


	/************************************************************************************************/


	struct TransformComponentArgs
	{
		SceneNodeComponentSystem*	Nodes;
		NodeHandle					Node;
	};

	const uint32_t TransformComponentID = GetTypeGUID(TransformComponent);

	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, SceneNodeComponentSystem* Nodes)
	{
		if (!GO.Full())
		{
			auto NodeHandle = Nodes->GetNewNode();
			GO.AddComponent(Component(Nodes, NodeHandle, CT_Transform));
		}
	}


	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, TransformComponentArgs Args)
	{
		if (!GO.Full())
		{
			NodeHandle Handle = Args.Node;
			if (Args.Node.INDEX == INVALIDHANDLE)
				Handle = Args.Nodes->GetNewNode();
			
			GO.AddComponent(Component(Args.Nodes, Handle, TransformComponentID));
		}
	}

	bool SetParent(ComponentListInterface* GO, NodeHandle Node)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C)
		{
			C->SetParentNode(Node, C->ComponentHandle);
			return true;
		}
		return false;
	}


	float3 GetWorldPosition(ComponentListInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetWorldPosition() : 0;
	}


	float3 GetLocalPosition(ComponentListInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetLocalPosition() : 0;
	}


	Quaternion GetOrientation(ComponentListInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetOrientation() : Quaternion::Identity();
	}


	NodeHandle GetNodeHandle(ComponentListInterface*  GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		FK_ASSERT(C);

		if(C)
			return C->ComponentHandle;
		return NodeHandle(-1);
	}


	NodeHandle GetParent(ComponentListInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		FK_ASSERT(C);

		if (C)
			return C->GetParentNode(C->ComponentHandle);
		return NodeHandle(-1);
	}


	bool Parent(ComponentListInterface* GO, NodeHandle Node)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		FK_ASSERT(C);

		if (C) {
			auto ThisNode = GetNodeHandle(GO);
			C->SetParentNode(ThisNode, Node);
			return true;
		}
		return false;
	}

	void SetLocalPosition(ComponentListInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);

		if (C) {
			C->SetLocalPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void SetWorldPosition(ComponentListInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->SetWorldPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void Translate(ComponentListInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Translate(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void TranslateWorld(ComponentListInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->TranslateWorld(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}


	/************************************************************************************************/


	void Yaw(ComponentListInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Yaw(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}

	void Pitch(ComponentListInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Pitch(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}

	void Roll(ComponentListInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Roll(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}


	/************************************************************************************************/


	class SceneNodeBehavior :
		public BehaviorBase
	{
	public:
		SceneNodeBehavior(ComponentListInterface& GO) :
			BehaviorBase(GO, GetTypeGUID(SceneNodeBehavior)) 
		{
		}

		bool Validate() override
		{
			return	
				BehaviorBase::Validate() &
				(FindComponent(GO, TransformComponentID) != nullptr);
		}


		virtual void TranslateLocal(const float3 xyz)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->Translate(xyz);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}


		virtual void TranslateWorld(const float3 xyz)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->TranslateWorld(xyz);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}


		virtual void Yaw(float R)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->Yaw(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}


		virtual void Pitch(float R)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->Pitch(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}


		virtual void Roll(float R)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->Roll(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}


		virtual void SetLocalPosition(float3 XYZ)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->SetLocalPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}


		virtual void SetWorldPosition(float3 XYZ)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->SetWorldPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}


		virtual bool SetParent(NodeHandle Node)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			C->SetParentNode(Node, C->ComponentHandle);
			return true;
		}


		virtual float3 GetWorldPosition()
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			return C ? C->GetWorldPosition() : 0;
		}


		virtual float3 GetLocalPosition()
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			return C ? C->GetLocalPosition() : 0;
		}


		virtual Quaternion GetOrientation()
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
			return C ? C->GetOrientation() : Quaternion::Identity();
		}


		virtual NodeHandle GetNodeHandle()
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);

			if (C)
				return C->ComponentHandle;

			return NodeHandle(-1);
		}


		virtual NodeHandle GetParent(ComponentListInterface* GO)
		{
			auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);

			if (C)
				return C->GetParentNode(C->ComponentHandle);

			return NodeHandle(-1);
		}
	};


	/************************************************************************************************/

	struct RayIntesectionResult
	{
		Ray		R;				// The Ray the Intersected the Volume
		float	D;				// Distance from Origin along R
		TriMeshHandle	Mesh;	// Mesh Intersected
		EntityHandle	Entity;	// Entity Intersected
	};

	typedef Vector<RayIntesectionResult> RayIntesectionResults;

	struct FLEXKITAPI DrawableComponentSystem : public ComponentSystemInterface
	{
	public:
		DrawableComponentSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes) :
			Scene		(IN_Scene),
			SceneNodes	(IN_SceneNodes) {}

		void Release(){}

		void ReleaseHandle		(ComponentHandle Handle) final;
		void SetNode			(ComponentHandle Handle, NodeHandle Node);
		void SetColor			(ComponentHandle Handle, float3 NewColor);
		void SetMetal			(ComponentHandle Handle, bool M);
		void SetVisibility		(ComponentHandle Drawable, bool V);
		void SetRayVisibility	(ComponentHandle Drawable, bool V);

		RayIntesectionResults RayCastBoundingSpheres(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes);
		RayIntesectionResults RayCastOBB(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes);

		NodeHandle GetNode(ComponentHandle Drawable);

		operator DrawableComponentSystem* ()	{ return this;   }
		operator GraphicScene* ()				{ return Scene; }

		GraphicScene*				Scene;
		SceneNodeComponentSystem*	SceneNodes;	// A Source System
	};

	const uint32_t RenderableComponentID = GetTypeGUID(DrawableComponentSystem);

	template<typename TY_GO>
	inline bool SetDrawableColor(TY_GO& GO, float3 Color)
	{
		auto C = FindComponent(GO, RenderableComponentID);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetColor(C->ComponentHandle, Color);
			NotifyAll(GO, RenderableComponentID, GetCRCGUID(MATERIAL));
		}
		return (C != nullptr);
	}

	template<typename TY_GO>
	inline bool SetDrawableMetal(TY_GO& GO, bool M)
	{
		auto C = FindComponent(GO, RenderableComponentID);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetMetal(C->ComponentHandle, M);
			GO.NotifyAll(RenderableComponentID, GetCRCGUID(MATERIAL));
		}
		return (C != nullptr);
	}


	inline bool SetVisibility(ComponentListInterface* GO, bool V)
	{
		auto C = FindComponent(GO, RenderableComponentID);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetVisibility(C->ComponentHandle, V);
			NotifyAll(GO, RenderableComponentID, GetCRCGUID(RAYVISIBILITY));
		}
		return (C != nullptr);
	}


	inline bool SetRayVisibility(ComponentListInterface* GO, bool V)
	{
		auto C = FindComponent(GO, RenderableComponentID);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetRayVisibility(C->ComponentHandle, V);
			NotifyAll(GO, RenderableComponentID, GetCRCGUID(RAYVISIBILITY));
		}
		return (C != nullptr);
	}


	struct DrawableComponentArgs
	{
		EntityHandle				Entity;
		DrawableComponentSystem*	System;
	};

	DrawableComponentArgs CreateEnityComponent(DrawableComponentSystem*	DrawableComponent, const char* Mesh);

	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, DrawableComponentArgs& Args)
	{
		if (!GO.Full()) {
			auto C = FindComponent(GO, RenderableComponentID);
			if (C)
				Args.System->SetNode(Args.Entity, C->ComponentHandle);
			else
				CreateComponent(GO, TransformComponentArgs{ Args.System->SceneNodes, Args.System->GetNode(Args.Entity) });

			GO.AddComponent(Component(Args.System, Args.Entity, RenderableComponentID));
		}
	}


	/************************************************************************************************/


	class DrawableBehavior :
		public BehaviorBase
	{
	public:
		DrawableBehavior(ComponentListInterface& GO) :
			BehaviorBase(GO, GetTypeGUID(DrawableBehavior)) {}

		bool Validate() override
		{
			return
				BehaviorBase::Validate() &
				(FindComponent(GO, RenderableComponentID) != nullptr);
		}


		bool SetDrawableColor(float3 Color)
		{
			auto C = FindComponent(GO, RenderableComponentID);
			if (C) {
				auto System = (DrawableComponentSystem*)C->ComponentSystem;
				System->SetColor(C->ComponentHandle, Color);
				//NotifyAll(GO, RenderableComponentID, GetCRCGUID(MATERIAL));
			}
			return (C != nullptr);
		}


		bool SetDrawableMetal(bool M)
		{
			auto C = FindComponent(GO, RenderableComponentID);
			if (C) {
				auto System = (DrawableComponentSystem*)C->ComponentSystem;
				System->SetMetal(C->ComponentHandle, M);
				//GO.NotifyAll(RenderableComponentID, GetCRCGUID(MATERIAL));
			}
			return (C != nullptr);
		}


		bool SetVisibility(bool V)
		{
			auto C = FindComponent(GO, RenderableComponentID);
			if (C) {
				auto System = (DrawableComponentSystem*)C->ComponentSystem;
				System->SetVisibility(C->ComponentHandle, V);
				//NotifyAll(GO, RenderableComponentID, GetCRCGUID(RAYVISIBILITY));
			}
			return (C != nullptr);
		}


		bool SetRayVisibility(bool V)
		{
			auto C = FindComponent(GO, RenderableComponentID);
			if (C) {
				auto System = (DrawableComponentSystem*)C->ComponentSystem;
				System->SetRayVisibility(C->ComponentHandle, V);
				//NotifyAll(GO, RenderableComponentID, GetCRCGUID(RAYVISIBILITY));
			}
			return (C != nullptr);
		}
	};


	/************************************************************************************************/


	struct FLEXKITAPI LightComponentSystem : public ComponentSystemInterface
	{
	public:
		LightComponentSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes) :
			Scene(IN_Scene),
			SceneNodes(IN_SceneNodes) {}

		void ReleaseHandle	( ComponentHandle Handle ) final;	
		void SetNode		( ComponentHandle Light, NodeHandle Node );
		void SetColor		( ComponentHandle Light, float3 NewColor );
		void SetIntensity	( ComponentHandle Light, float I );
		void SetRadius		( ComponentHandle Light, float R );		

		operator LightComponentSystem* ()	{ return this; }
		operator GraphicScene* ()			{ return Scene; }

		GraphicScene*				Scene;
		SceneNodeComponentSystem*	SceneNodes;	// A Source System
	};

	const uint32_t LightComponentID = GetTypeGUID(PointLightComponentSystem);


	struct LightComponentArgs
	{
		NodeHandle				Node;
		LightComponentSystem*	System;
		float3					Color;
		float					I;
		float					R;
	};

	bool SetLightColor(ComponentListInterface* GO, float3 K)
	{
		auto C = FindComponent(GO, LightComponentID);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetColor(C->ComponentHandle, K);
			NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}

	bool SetLightIntensity(ComponentListInterface* GO, float I)
	{
		auto C = FindComponent(GO, LightComponentID);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetIntensity(C->ComponentHandle, I);
			NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}

	template<typename T_GO>
	bool SetLightRadius(T_GO& GO, float R)
	{
		auto C = FindComponent(GO, LightComponentID);
		if (C)
		{
			auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
			LightSystem->SetRadius(C->ComponentHandle, R);
			NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
		}
		return C != nullptr;
	}


	/************************************************************************************************/


	LightComponentArgs CreateLightComponent( LightComponentSystem* LightSystem, float3 Color = float3(1.0f, 1.0f, 1.0f), float I = 100.0f, float R = 10000.0f )
	{
		return { NodeHandle(-1), LightSystem, Color, I, R};
	}


	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, LightComponentArgs& Args)
	{
		if (!GO.Full()) {
			NodeHandle Node;

			if (Args.Node == NodeHandle(-1))
			{
				auto C = FindComponent(GO, TransformComponentID);

				if (C)
					Node = C->ComponentHandle;
				else
					Node = Args.System->SceneNodes->GetNewNode();
			}
			else
				Node = Args.Node;

			LightHandle Light = Args.System->Scene->AddPointLight(Args.Color, Node,  Args.I, Args.R);
			GO.AddComponent(Component(Args.System, Light, LightComponentID));
		}
	}


	/************************************************************************************************/


	class LightBehavior :
		public BehaviorBase
	{
	public:
		LightBehavior(ComponentListInterface& GO) :
			BehaviorBase(GO, GetTypeGUID(LightBehavior))
		{
			FK_ASSERT(FindComponent(GO, LightComponentID) != nullptr);
		}


		bool SetLightColor(float3 K)
		{
			auto C = FindComponent(GO, LightComponentID);

			if (C)
			{
				auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
				LightSystem->SetColor(C->ComponentHandle, K);
				NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
			}

			return C != nullptr;
		}


		bool SetLightIntensity(float I)
		{
			auto C = FindComponent(GO, LightComponentID);
			if (C)
			{
				auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
				LightSystem->SetIntensity(C->ComponentHandle, I);
				NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
			}

			return C != nullptr;
		}


		bool SetLightRadius(float R)
		{
			auto C = FindComponent(GO, LightComponentID);

			if (C)
			{
				auto LightSystem = (LightComponentSystem*)C->ComponentSystem;
				LightSystem->SetRadius(C->ComponentHandle, R);
				NotifyAll(GO, LightComponentID, GetCRCGUID(LIGHTPROPERTIES));
			}

			return C != nullptr;
		}
	};


	/************************************************************************************************/


	class FLEXKITAPI CameraComponentSystem : public ComponentSystemInterface
	{
	public:
		CameraComponentSystem(RenderSystem* rs, SceneNodeComponentSystem* nodes, iAllocator* memory) : 
			Cameras(Memory), 
			Handles(Memory), 
			Memory(memory),
			Nodes(nodes),
			RS(rs) {}


		~CameraComponentSystem() override 
		{
			for (auto& C : Cameras)
				FlexKit::ReleaseCamera(*Nodes, &C);

			Cameras.Release();
			Handles.Release();
		}


		ComponentHandle CreateCamera(float AspectRatio, float Near, float Far, NodeHandle Node = NodeHandle(-1), bool Invert = true)
		{
			Camera NewCamera;
			InitiateCamera(*Nodes, &NewCamera, AspectRatio, Near, Far, false);

			if (Node != InvalidComponentHandle) {
				//Nodes->ReleaseHandle(NewCamera.Node);
				Nodes->SetParentNode(Node, NewCamera.Node);
				NewCamera.Node = Node;
			}
			else
			{
				Node = Nodes->GetZeroedNode();
				NewCamera.Node = Node;
			}

			auto NewHandle		= Handles.GetNewHandle();
			Handles[NewHandle]	= Cameras.size();
			Cameras.push_back(NewCamera);

			return NewHandle;
		}


		void ReleaseHandle(ComponentHandle Handle) override
		{
			if (Cameras.size() - 1 > Handles[Handle]) 
			{
				for (auto& H : Handles.Indexes)
				{
					if (H == Cameras.size() - 1)
					{	
						auto index = Handles[Handle];
						Cameras[index] = Cameras.back();
						H = Handles[Handle];
						Handles.RemoveHandle(Handle);
					}
				}
			}

			ReleaseCamera(*Nodes, &Cameras.back());
			Cameras.pop_back();
		}


		void Update(double dT)
		{
			for (auto& C : Cameras)
				UpdateCamera(RS, *Nodes, &C, dT);
		}

		Camera* GetCamera(ComponentHandle Handle)
		{
			if (Handle == InvalidComponentHandle)
				return nullptr;

			return &Cameras[Handles[Handle]];
		}

		NodeHandle GetNode(ComponentHandle Handle)
		{
			auto Camera = GetCamera(Handle);
			if (Camera)
				return Camera->Node;

			return InvalidComponentHandle;
		}


		void HandleEvent(ComponentHandle Handle, ComponentType EventSource, ComponentSystemInterface* System, EventTypeID, ComponentListInterface* GO) override
		{
		}


		operator CameraComponentSystem* ()
		{
			return this;
		}

		SceneNodeComponentSystem*						Nodes;
		RenderSystem*									RS;
		iAllocator*										Memory;
		Vector<Camera>									Cameras;
		HandleUtilities::HandleTable<ComponentHandle>	Handles;
	};


	const uint32_t CameraComponentID = GetTypeGUID(CameraComponent);


	struct CameraComponentArgs
	{
		NodeHandle				Node;
		CameraComponentSystem*	System;
		float					AspectRatio;
		float					Near;
		float					Far;
		bool					InverseBuffer = true;
	};


	template<size_t SIZE>
	void CreateComponent(ComponentList<SIZE>& GO, CameraComponentArgs& Args)
	{
		if (!GO.Full()) {
			auto Transform  = (TansformComponent*)FindComponent(GO, TransformComponentID);

			auto NodeHandle = (Args.Node != InvalidComponentHandle) ? Args.Node : ((Transform != nullptr) ? Transform->GetNode() : InvalidComponentHandle);
			auto Handle		= Args.System->CreateCamera(Args.AspectRatio, Args.Near, Args.Far, NodeHandle);

			if (!Transform)
				CreateComponent(GO, TransformComponentArgs{ Args.System->Nodes, Args.System->GetNode(Handle) });

			GO.AddComponent(Component(Args.System, Handle, CameraComponentID));
		}
	}


	CameraComponentArgs CreateCameraComponent(CameraComponentSystem* System, float AspectRatio, float Near, float Far, NodeHandle Node, bool InverseBuffer = true)
	{
		CameraComponentArgs args;
		args.System			= System;
		args.InverseBuffer	= InverseBuffer;
		args.Far			= Far;
		args.Near			= Near;
		args.AspectRatio	= AspectRatio;
		args.Node			= Node;

		return args;
	}


	struct FLEXKITAPI CameraComponent : public Component
	{
		NodeHandle GetNode()
		{
			auto Cameras = static_cast<CameraComponentSystem*>(ComponentSystem);
			return Cameras->GetNode(ComponentHandle);
		}


		Quaternion GetOrientation()
		{
			auto Cameras = static_cast<CameraComponentSystem*>(ComponentSystem);
			return Cameras->Nodes->GetOrientation(Cameras->GetNode(ComponentHandle));
		}

		Camera* GetCamera_ptr()
		{
			auto Cameras = static_cast<CameraComponentSystem*>(ComponentSystem);
			return Cameras->GetCamera(ComponentHandle);
		}
	};


	/************************************************************************************************/


	Quaternion GetCameraOrientation(ComponentListInterface* GO)
	{
		auto Camera = (CameraComponent*)FindComponent(GO, CameraComponentID);

		if (Camera)
			return Camera->GetOrientation();

		return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}

	Camera* GetCamera_ptr(ComponentListInterface* GO)
	{
		auto CameraComp = (CameraComponent*)FindComponent(GO, CameraComponentID);
		return CameraComp->GetCamera_ptr();
	}

	Camera& GetCamera_ref(ComponentListInterface* GO)
	{
		auto CameraComp = (CameraComponent*)FindComponent(GO, CameraComponentID);
		FK_ASSERT(CameraComp);

		return *CameraComp->GetCamera_ptr();
	}


	/************************************************************************************************/


	class CameraBehavior :
		public BehaviorBase
	{
	public:


		CameraBehavior(ComponentListInterface& GO) :
			BehaviorBase(GO, GetTypeGUID(CameraBehavior))
		{
			//FK_ASSERT(FindComponent(GO, CameraComponentID) != nullptr);
		}

		bool Validate() override
		{
			return
				BehaviorBase::Validate() &
				(FindComponent(GO, CameraComponentID) != nullptr);
		}

		Quaternion GetCameraOrientation()
		{
			auto Camera = (CameraComponent*)FindComponent(GO, CameraComponentID);
			return Camera->GetOrientation();
		}


		Camera* GetCamera_ptr()
		{
			auto CameraComp = (CameraComponent*)FindComponent(GO, CameraComponentID);
			return CameraComp->GetCamera_ptr();
		}


		Camera& GetCamera_ref()
		{
			auto CameraComp = (CameraComponent*)FindComponent(GO, CameraComponentID);
			return *CameraComp->GetCamera_ptr();
		}
	};


	/************************************************************************************************/
}//namespace FlexKit

#endif