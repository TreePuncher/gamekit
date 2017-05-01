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

#ifndef CAMERAUTILITIES
#define CAMERAUTILITIES

#include "Gameplay.h"
#include "GameFramework.h"
#include "InputComponent.h"

#include "../graphicsutilities/graphics.h"
#include "../coreutilities/GraphicsComponents.h"

using FlexKit::NodeHandle;
using FlexKit::SceneNodes;
using FlexKit::Camera;
using FlexKit::ComponentSystemInterface;

struct Camera3rdPersonContoller
{
	NodeHandle Yaw_Node;
	NodeHandle Pitch_Node;
	NodeHandle Roll_Node;

	float Yaw, Pitch, Roll;

	SceneNodes*	Nodes;
	Camera*		C;
};

void InitiateCamera3rdPersonContoller( SceneNodes* Nodes, Camera* C, Camera3rdPersonContoller* Out );
void UpdateCameraController( SceneNodes* Nodes, Camera3rdPersonContoller* Controller, double dT );

void SetCameraOffset	(Camera3rdPersonContoller* Controller, float3 xyz);
void SetCameraPosition	(Camera3rdPersonContoller* Controller, float3 xyz);

void TranslateCamera	(Camera3rdPersonContoller* Controller, float3 xyz);
void YawCamera			(Camera3rdPersonContoller* Controller, float Degree);
void PitchCamera		(Camera3rdPersonContoller* Controller, float Degree);
void RollCamera			(Camera3rdPersonContoller* Controller, float Degree);

float3 GetForwardVector	(Camera3rdPersonContoller* Controller);
float3 GetRightVector	(Camera3rdPersonContoller* Controller);

namespace FlexKit
{
	typedef Handle_t<16> CameraControllerHandle;

	struct OrbitCameraSystem;

	struct OrbitCameraArgs
	{
		CameraControllerHandle	Handle;
		OrbitCameraSystem*		System;
	};

	struct OrbitCameraInputState
	{
		MouseInputState Mouse;
		bool Forward;
		bool Backward;
		bool Left;
		bool Right;
	};

	struct CameraOrbitController
	{
		NodeHandle CameraNode;
		NodeHandle YawNode;
		NodeHandle PitchNode;
		NodeHandle RollNode;
	};


	struct OrbitCameraSystem : public ComponentSystemInterface
	{
		operator OrbitCameraSystem* (){return this;}
		void Initiate		(GameFramework* Framework, InputComponentSystem* Input);

		void ReleaseHandle	(ComponentHandle Handle);
		void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID);

		void Update(double dT);

		NodeHandle GetNode(CameraControllerHandle Handle)
		{
			return Controllers[Handle].YawNode;
		}

		void SetCameraNode(CameraControllerHandle Handle, NodeHandle Node)
		{
			Controllers[Handle].CameraNode = Node;
			SetParentNode(*Nodes, Controllers[Handle].CameraNode, Controllers[Handle].PitchNode);
		}

		DynArray<CameraOrbitController>	Controllers;

		// Component Data Sources
		SceneNodeComponentSystem*	Nodes;
		InputComponentSystem*		InputSystem;
	};

	const uint32_t OrbitCameraComponentID = GetTypeGUID(OrbitCamera);

	OrbitCameraArgs CreateOrbitCamera(OrbitCameraSystem* S, Camera* Cam);

	NodeHandle GetCameraSceneNode(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, OrbitCameraComponentID);
		if(C)
		{
			auto OrbitSystem = (OrbitCameraSystem*)C->ComponentSystem;
			return OrbitSystem->GetNode(C->ComponentHandle);
		}

		return NodeHandle(0);
	}


	/************************************************************************************************/


	void SetParentNode(GameObjectInterface* GO, NodeHandle Node);

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, OrbitCameraArgs& Args)
	{
		Args.System->InputSystem->BindInput(Args.Handle, Args.System);

		GO.AddComponent(Component(Args.System, Args.Handle, OrbitCameraComponentID));

		auto T			= (TansformComponent*)FindComponent(GO, TransformComponentID);
		auto CameraNode = GetCameraSceneNode(GO);

		if (T)
			Parent(GO, CameraNode);
		else
			CreateComponent(GO, TransformComponentArgs{ Args.System->Nodes, CameraNode });
	}


	struct ThirdPersonCameraComponentSystem : public ComponentSystemInterface
	{
		void Initiate(GameFramework* Framework);

		void ReleaseHandle	(ComponentHandle Handle);
		void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID);

		enum State
		{
			UNUSED,
			Dirty, 
			Clean,
		};

		struct CameraController
		{
			NodeHandle Yaw_Node;
			NodeHandle Pitch_Node;
			NodeHandle Roll_Node;

			float Yaw, Pitch, Roll;

			SceneNodes*	Nodes;
			Camera*		C;
		};

		DynArray<State>				States;
		DynArray<CameraController>	CameraControllers;
	};

	CameraControllerHandle CreateThirdPersonCamComponent(ThirdPersonCameraComponentSystem*);
}
#endif