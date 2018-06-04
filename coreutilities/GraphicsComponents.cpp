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

#include "..\graphicsutilities\graphics.h"
#include "GraphicsComponents.h"
#include "GraphicScene.h"

namespace FlexKit
{
	/********************************* Implentation of Transform Components *************************/
	/************************************************************************************************/

	void SceneNodeComponentSystem::Release()
	{
	}

	void SceneNodeComponentSystem::ReleaseHandle(ComponentHandle Handle)
	{
		FlexKit::ReleaseNode(Nodes, Handle);
	}

	NodeHandle SceneNodeComponentSystem::GetRoot() 
	{ 
		return Root; 
	}

	NodeHandle SceneNodeComponentSystem::GetNewNode() 
	{ 
		return FlexKit::GetNewNode(Nodes); 
	}


	NodeHandle SceneNodeComponentSystem::GetZeroedNode()
	{
		auto Node = FlexKit::GetZeroedNode(Nodes);
		SetParentNode(Root, Node);
		return Node;
	}

	void TansformComponent::Roll(float r)
	{
		FlexKit::Roll(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	void TansformComponent::Pitch(float r)
	{
		FlexKit::Pitch(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	void TansformComponent::Yaw(float r)
	{
		FlexKit::Yaw(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), r);
	}


	/************************************************************************************************/


	float3 TansformComponent::GetLocalPosition()
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle)));
		return LT.T;
	}


	/************************************************************************************************/


	float3 TansformComponent::GetWorldPosition()
	{
		return FlexKit::GetPositionW(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle));
	}


	/************************************************************************************************/


	void TansformComponent::Translate(const float3 xyz)
	{
		FlexKit::TranslateLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), xyz);
	}


	/************************************************************************************************/


	void TansformComponent::TranslateWorld(const float3 xyz)
	{
		FlexKit::TranslateWorld(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), xyz);
	}


	/************************************************************************************************/


	void TansformComponent::SetLocalPosition(const float3& xyz)
	{
		FlexKit::LT_Entry LT(FlexKit::GetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle)));
		LT.T = xyz.pfloats;
		FlexKit::SetLocal(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), &LT);
	}


	/************************************************************************************************/


	void TansformComponent::SetWorldPosition(const float3& xyz)
	{
		FlexKit::SetPositionW(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), xyz);
	}


	/************************************************************************************************/


	FlexKit::Quaternion TansformComponent::GetOrientation()
	{
		Quaternion Q = FlexKit::GetOrientation(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle) );
		return Q;
	}


	/************************************************************************************************/


	void TansformComponent::SetOrientation(FlexKit::Quaternion Q)
	{
		FlexKit::SetOrientation(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), static_cast<NodeHandle>(ComponentHandle), Q);
	}


	void TansformComponent::SetParentNode(NodeHandle Parent, NodeHandle Node)
	{
		FlexKit::SetParentNode(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), Parent, Node);
	}


	NodeHandle TansformComponent::GetParentNode(NodeHandle Node)
	{
		return FlexKit::GetParentNode(*static_cast<SceneNodeComponentSystem*>(ComponentSystem), Node);
	}

	NodeHandle TansformComponent::GetNode()
	{
		return ComponentHandle;
	}


	/************************************************************************************************/

	DrawableComponentArgs CreateDrawableComponent(DrawableComponentSystem*	DrawableComponent, const char* Mesh)
	{
		return { DrawableComponent->Scene->CreateDrawableAndSetMesh(Mesh), DrawableComponent };
	}

	/************************************************************************************************/

	void LightComponentSystem::ReleaseHandle(ComponentHandle Handle)  
	{ 
		ReleaseLight(&Scene->PLights, Handle); 
	}
	void LightComponentSystem::SetNode(ComponentHandle Light, NodeHandle Node) 
	{ 
		Scene->SetLightNodeHandle(Light, Node); 
	}
	void LightComponentSystem::SetColor(ComponentHandle Light, float3 NewColor) 
	{ 
		Scene->PLights[Light].K = NewColor; 
	}
	void LightComponentSystem::SetIntensity(ComponentHandle Light, float I) 
	{ 
		Scene->PLights[Light].I = I; 
	}
	void LightComponentSystem::SetRadius(ComponentHandle Light, float R) 
	{ 
		Scene->PLights[Light].R = R; 
	}

	/************************************************************************************************/

	void DrawableComponentSystem::ReleaseHandle(ComponentHandle Handle)
	{
		Scene->RemoveEntity(Handle);
	}

	void DrawableComponentSystem::SetNode(ComponentHandle Drawable, NodeHandle Node)
	{
		Scene->SetNode(Drawable, Node);
	}


	void DrawableComponentSystem::SetColor(ComponentHandle Drawable, float3 NewColor)
	{
		auto DrawableColor = Scene->GetMaterialColour(Drawable);
		auto DrawableSpec  = Scene->GetMaterialSpec(Drawable);

		DrawableColor.x = NewColor.x;
		DrawableColor.y = NewColor.y;
		DrawableColor.z = NewColor.z;
		// W cord the Metal Factor

		Scene->SetMaterialParams(Drawable, DrawableColor, DrawableSpec);
	}


	void DrawableComponentSystem::SetMetal(ComponentHandle Drawable, bool M)
	{
		auto DrawableColor = Scene->GetMaterialColour(Drawable);
		auto DrawableSpec = Scene->GetMaterialSpec(Drawable);

		DrawableColor.w = 1.0f * M;

		Scene->SetMaterialParams(Drawable, DrawableColor, DrawableSpec);
	}

	void DrawableComponentSystem::SetVisibility(ComponentHandle Drawable, bool V)
	{ 
		Scene->SetVisability(Drawable, V); 
	}

	void DrawableComponentSystem::SetRayVisibility(ComponentHandle Drawable, bool V) 
	{ 
		Scene->SetRayVisability(Drawable, V); 
	}

	NodeHandle DrawableComponentSystem::GetNode(ComponentHandle Drawable)
	{
		return Scene->GetNode(Drawable);
	}
	/*

	// TODO: Rewrite this to use Frame Graph
	void DrawableComponentSystem::DrawDebug(ImmediateRender* R, SceneNodeComponentSystem* Nodes, iAllocator* Temp)
	{
		auto& Drawables		= Scene->Drawables;
		auto& Visibility	= Scene->DrawableVisibility;

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
	*/

	RayIntesectionResults DrawableComponentSystem::RayCastBoundingSpheres(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes)// Runs RayCasts Against all object Bounding Spheres
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


	RayIntesectionResults DrawableComponentSystem::RayCastOBB(RaySet& Rays, iAllocator* Memory, SceneNodeComponentSystem* Nodes)// Runs RayCasts Against all object OBB's
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


	/************************************************************************************************/
}