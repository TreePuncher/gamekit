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
			
			GO.AddComponent(Component(Args.Nodes, Handle, TransformComponentID));
		}
	}

	bool SetParent(GameObjectInterface* GO, NodeHandle Node)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C)
		{
			C->SetParentNode(Node, C->ComponentHandle);
			return true;
		}
		return false;
	}


	float3 GetWorldPosition(GameObjectInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetWorldPosition() : 0;
	}


	float3 GetLocalPosition(GameObjectInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetLocalPosition() : 0;
	}


	Quaternion GetOrientation(GameObjectInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		return C ? C->GetOrientation() : Quaternion::Identity();
	}


	NodeHandle GetNodeHandle(GameObjectInterface*  GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		FK_ASSERT(C);

		if(C)
			return C->ComponentHandle;
		return NodeHandle(-1);
	}


	NodeHandle GetParent(GameObjectInterface* GO)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		FK_ASSERT(C);

		if (C)
			return C->GetParentNode(C->ComponentHandle);
		return NodeHandle(-1);
	}


	bool Parent(GameObjectInterface* GO, NodeHandle Node)
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

	void SetLocalPosition(GameObjectInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);

		if (C) {
			C->SetLocalPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void SetWorldPosition(GameObjectInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->SetWorldPosition(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void Translate(GameObjectInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Translate(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}

	void TranslateWorld(GameObjectInterface* GO, float3 XYZ)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->TranslateWorld(XYZ);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(POSITION));
		}
	}


	/************************************************************************************************/


	void Yaw(GameObjectInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Yaw(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}

	void Pitch(GameObjectInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Pitch(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}

	void Roll(GameObjectInterface* GO, float R)
	{
		auto C = (TansformComponent*)FindComponent(GO, TransformComponentID);
		if (C) {
			C->Roll(R);
			NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}


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

		void SetVisibility(ComponentHandle Drawable, bool V)	{ Scene->SetVisability(Drawable, V); }

		void SetRayVisibility(ComponentHandle Drawable, bool V)	{ Scene->SetRayVisability(Drawable, V);	}

		void DrawDebug(ImmediateRender* R, SceneNodeComponentSystem* Nodes, iAllocator* Temp)
		{
			auto& Drawables = Scene->Drawables;
			auto& Visibility = Scene->DrawableVisibility;

			size_t End = Scene->Drawables.size();
			for (size_t I = 0; I < End; ++I)
			{
				auto DrawableHandle = EntityHandle(I);
				if (Scene->GetVisability(DrawableHandle))
				{
					auto MeshHandle  = Scene->GetMeshHandle(DrawableHandle);
					auto Mesh        = GetMesh(Scene->GT, MeshHandle);
					auto Node        = Scene->GetNode(DrawableHandle);
					auto PositionWS  = Nodes->GetPositionW(Node);
					auto Orientation = Nodes->GetOrientation(Node);
					auto Ls			 = Nodes->GetLocalScale(Node);
					auto BS			 = Mesh->BS;
					auto AABBSpan	 = Mesh->AABB.TopRight - Mesh->AABB.BottomLeft;

					PushBox_WireFrame(R, Temp, PositionWS + Orientation * BS.xyz(), Orientation, Ls * AABBSpan, {1, 1, 0});
					//PushCircle3D(R, Temp, PositionWS + Orientation * BS.xyz(), BS.w * Ls.x);
				}
			}
		}

		RayIntesectionResults RayCastBoundingSpheres(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes)// Runs RayCasts Against all object Bounding Spheres
		{
			RayIntesectionResults Out(Memory);

			auto& Drawables  = Scene->Drawables;
			auto& Visibility = Scene->DrawableVisibility;

			size_t End = Scene->Drawables.size();
			for (size_t I = 0; I < End; ++I)
			{
				auto DrawableHandle = EntityHandle(I);
				if (Scene->GetRayVisability(DrawableHandle) && Scene->GetVisability(DrawableHandle))
				{
					auto MeshHandle  = Scene->GetMeshHandle(DrawableHandle);
					auto Mesh        = GetMesh(Scene->GT, MeshHandle);
					auto Node        = Scene->GetNode(DrawableHandle);
					auto PosWS		 = Nodes->GetPositionW(Node);
					auto Orientation = Nodes->GetOrientation(Node);
					auto Ls			 = Nodes->GetLocalScale(Node);
					auto BS			 = Mesh->BS;

					for (auto r : Rays)
					{
						auto Origin = r.O;
						auto R = BS.w * Ls.x;
						auto R2 = R * R;
						auto L = (Origin - PosWS);
						auto S = r.D.dot(L);
						auto S2 = S * S;
						auto L2 = L.dot(L);
						auto M2 = L2 - S2;

						if(S < 0 && L2 > R2)
							continue; // Miss

						if(M2 > R2)
							continue; // Miss

						auto Q = sqrt(R2 - M2);

						float T = 0;
						if(L2 > R2)
							T = S - Q;
						else 
							T = S + Q;

						RayIntesectionResult Hit;
						Hit.D      = T;
						Hit.Entity = DrawableHandle;
						Hit.Mesh   = MeshHandle;
						Hit.R      = r;
						Out.push_back(Hit);
					}
				}
			}

			return Out;
		}

		RayIntesectionResults RayCastOBB(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes)// Runs RayCasts Against all object OBB's
		{
			RayIntesectionResults Out(Memory);

			auto& Drawables  = Scene->Drawables;
			auto& Visibility = Scene->DrawableVisibility;

			size_t End = Scene->Drawables.size();
			for (size_t I = 0; I < End; ++I)
			{
				auto DrawableHandle = EntityHandle(I);
				if (Scene->GetRayVisability(DrawableHandle) && Scene->GetVisability(DrawableHandle))
				{
					auto MeshHandle  = Scene->GetMeshHandle(DrawableHandle);
					auto Mesh        = GetMesh(Scene->GT, MeshHandle);
					auto Node        = Scene->GetNode(DrawableHandle);
					auto PosWS		 = Nodes->GetPositionW(Node);
					auto Orientation = Nodes->GetOrientation(Node);
					auto Ls			 = Nodes->GetLocalScale(Node);
					auto AABB		 = Mesh->AABB;// Not Yet Orientated

					auto H = AABB.TopRight * Ls;

					auto Normals = static_vector<float3, 3>(
					{	Orientation * float3{ 1, 0, 0 },
						Orientation * float3{ 0, 1, 0 },
						Orientation * float3{ 0, 0, 1 } });

					for (auto r : Rays)
					{
						float t_min = -INFINITY;
						float t_max = +INFINITY;
						auto P = PosWS - r.O;

						[&]() {
							for (size_t I = 0; I < 3; ++I)
							{
								auto e = Normals[I].dot(P);
								auto f = Normals[I].dot(r.D);

								if (abs(f) > 0.00)
								{
									float t_1 = (e + H[I]) / f;
									float t_2 = (e - H[I]) / f;

									if (t_1 > t_2)// Swap
										std::swap(t_1, t_2);

									if (t_1 > t_min) t_min = t_1;
									if (t_2 < t_max) t_max = t_2;
									if (t_min > t_max)	return;
									if (t_max < 0)		return;
								}
								else if( -e - H[I] > 0 || 
										 -e + H[I] < 0 ) return;
							}

							RayIntesectionResult Hit;
							if (t_min > 0)
								Hit.D  = t_min;
							else
								Hit.D = t_max;

							Hit.R	   = r;
							Hit.Entity = DrawableHandle;
							Hit.Mesh   = MeshHandle;
							Out.push_back(Hit);
						}();
					}
				}
			}

			return Out;
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


	inline bool SetVisibility(GameObjectInterface* GO, bool V)
	{
		auto C = FindComponent(GO, RenderableComponentID);
		if (C) {
			auto System = (DrawableComponentSystem*)C->ComponentSystem;
			System->SetVisibility(C->ComponentHandle, V);
			NotifyAll(GO, RenderableComponentID, GetCRCGUID(RAYVISIBILITY));
		}
		return (C != nullptr);
	}

	inline bool SetRayVisibility(GameObjectInterface* GO, bool V)
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

	DrawableComponentArgs CreateEnityComponent(DrawableComponentSystem*	DrawableComponent, const char* Mesh )
	{
		return { DrawableComponent->Scene->CreateDrawableAndSetMesh(Mesh), DrawableComponent };
	}

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, DrawableComponentArgs& Args)
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


	struct FLEXKITAPI LightComponentSystem : public ComponentSystemInterface
	{
	public:
		LightComponentSystem(GraphicScene* IN_Scene, SceneNodeComponentSystem* IN_SceneNodes) :
			Scene(IN_Scene),
			SceneNodes(IN_SceneNodes) {}

		void ReleaseHandle	( ComponentHandle Handle ) final			{ ReleaseLight(&Scene->PLights, Handle); }
		void SetNode		( ComponentHandle Light, NodeHandle Node )	{ Scene->SetLightNodeHandle(Light, Node); }
		void SetColor		( ComponentHandle Light, float3 NewColor )	{ Scene->PLights[Light].K = NewColor; }
		void SetIntensity	( ComponentHandle Light, float I )			{ Scene->PLights[Light].I = I; }
		void SetRadius		( ComponentHandle Light, float R )			{ Scene->PLights[Light].R = R; }

		operator LightComponentSystem* ()	{ return this; }
		operator GraphicScene* ()			{ return *Scene; }

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

	bool SetLightColor(GameObjectInterface* GO, float3 K)
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

	bool SetLightIntensity(GameObjectInterface* GO, float I)
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
	void CreateComponent(GameObject<SIZE>& GO, LightComponentArgs& Args)
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
			InitiateCamera(RS, *Nodes, &NewCamera, AspectRatio, Near, Far, Invert);

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


		void HandleEvent(ComponentHandle Handle, ComponentType EventSource, ComponentSystemInterface* System, EventTypeID, GameObjectInterface* GO) override
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
	void CreateComponent(GameObject<SIZE>& GO, CameraComponentArgs& Args)
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
	};


	Quaternion GetCameraOrientation(GameObjectInterface* GO)
	{
		auto Camera = (CameraComponent*)FindComponent(GO, CameraComponentID);

		if (Camera)
			return Camera->GetOrientation();

		return Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	}


	/************************************************************************************************/

}//namespace FlexKit

#endif