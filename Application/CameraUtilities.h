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

#include "GameFramework.h"
#include "InputComponent.h"

#include "../graphicsutilities/graphics.h"
#include "../coreutilities/GraphicsComponents.h"
#include "../coreutilities/Transforms.h"

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


void InitiateCamera3rdPersonContoller	( SceneNodes* Nodes, Camera* C, Camera3rdPersonContoller* Out );
void UpdateCameraController				( SceneNodes* Nodes, Camera3rdPersonContoller* Controller, double dT );

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
		float					MoveRate;
		NodeHandle				Node;
		OrbitCameraSystem*		System;
		CameraComponentSystem*	Cameras;
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

		float MoveRate;

		GameObjectInterface* ParentGO;
	};


	struct OrbitCameraSystem : public ComponentSystemInterface
	{
		OrbitCameraSystem(GameFramework* Framework, InputComponentSystem* Input) :
			Controllers	(Framework->Core->GetBlockMemory()),
			Nodes		(Framework->Core->Nodes),
			InputSystem	(Input)	{}

		operator OrbitCameraSystem* (){return this;}

		void ReleaseHandle	(ComponentHandle Handle);
		void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID);

		void Update(double dT);

		CameraControllerHandle CreateOrbitCamera(ComponentHandle Node, ComponentHandle CameraNode, float MoveRate)
		{	
			FK_ASSERT(Node != CameraNode);

			CameraOrbitController Controller;
			Controller.CameraNode	= CameraNode;
			Controller.YawNode		= Node;
			Controller.PitchNode	= Nodes->GetZeroedNode();
			Controller.RollNode		= Nodes->GetZeroedNode();
			Controller.MoveRate		= MoveRate;

			Nodes->SetParentNode(Controller.YawNode,	Controller.PitchNode);
			Nodes->SetParentNode(Controller.PitchNode,	Controller.RollNode);
			Nodes->SetParentNode(Controller.RollNode,	CameraNode);

			Controllers.push_back(Controller);

			return CameraControllerHandle(Controllers.size() - 1);
		}

		NodeHandle GetNode(CameraControllerHandle Handle)
		{
			return Controllers[Handle].YawNode;
		}

		void SetCameraNode(CameraControllerHandle Handle, NodeHandle Node)
		{
			Controllers[Handle].CameraNode = Node;
			SetParentNode(*Nodes, Controllers[Handle].CameraNode, Controllers[Handle].PitchNode);
		}
		
		Quaternion GetOrientation(ComponentHandle handle)
		{
			return Nodes->GetOrientation(Controllers[handle].CameraNode);
		}

		Vector<CameraOrbitController>	Controllers;

		// Component Data Sources
		SceneNodeComponentSystem*	Nodes;
		InputComponentSystem*		InputSystem;
	};


	const uint32_t OrbitCameraComponentID = GetTypeGUID(OrbitCamera);

	OrbitCameraArgs CreateOrbitCamera(OrbitCameraSystem* S, NodeHandle Camera, float MoveRate = 100);
	OrbitCameraArgs CreateOrbitCamera(OrbitCameraSystem* S, CameraComponentSystem* Cameras, float MoveRate = 100);


	/************************************************************************************************/


	NodeHandle GetCameraSceneNode(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, CameraComponentID);
		if(C)
		{
			auto CameraSystem = (CameraComponentSystem*)C->ComponentSystem;
			return CameraSystem->GetNode(C->ComponentHandle);
		}

		return InvalidComponentHandle;
	}


	NodeHandle GetOrbitCameraSceneNode(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, CameraComponentID);
		if (C)
		{
			auto OrbitSystem = (OrbitCameraSystem*)C->ComponentSystem;
			return OrbitSystem->GetNode(C->ComponentHandle);
		}

		return InvalidComponentHandle;
	}

	/*
	Quaternion GetCameraOrientation(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, OrbitCameraComponentID);
		if (C)
		{
			auto OrbitSystem = (OrbitCameraSystem*)C->ComponentSystem;
			return OrbitSystem->GetOrientation(C->ComponentHandle);
		}

		return Quaternion::Identity();
	}
	*/


	/************************************************************************************************/


	void SetParentNode(GameObjectInterface* GO, NodeHandle Node);

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, OrbitCameraArgs& Args)
	{
		auto T						= (TansformComponent*)FindComponent(GO, TransformComponentID);
		auto C						= (CameraComponent*)FindComponent(GO, CameraComponentID);
		ComponentHandle Node		= InvalidComponentHandle;
		ComponentHandle CameraNode	= InvalidComponentHandle;

		CameraNode = (Args.Node != InvalidComponentHandle) ? Args.Node : C->GetNode();

		if (!T && Args.Node == InvalidComponentHandle)
			Node = Args.System->Nodes->GetZeroedNode();	// No Node Specified, make one
		else if (T)
			if (T->ComponentHandle == CameraNode)
				Node = Args.System->Nodes->GetZeroedNode();
			else
				Node = T->ComponentHandle;
		
		if (!C)
			return;// TODO: Maybe Create Camera here?

		auto ControllerHandle = Args.System->CreateOrbitCamera(Node, CameraNode, Args.MoveRate);
		GO.AddComponent(Component(Args.System, ControllerHandle, OrbitCameraComponentID));

		Args.System->Controllers[ControllerHandle].ParentGO = GO;
		Args.System->InputSystem->BindInput(ControllerHandle, Args.System);

		if (T)
			T->ComponentHandle = Node;
		else
			CreateComponent(GO, TransformComponentArgs{ Args.System->Nodes, Node });
	}


	/************************************************************************************************/


	struct ThirdPersonCameraComponentSystem : public ComponentSystemInterface
	{
		ThirdPersonCameraComponentSystem(GameFramework* Framework, InputComponentSystem* InputSystem, CameraComponentSystem* camerasystem) :
			Input				(InputSystem),
			CameraSystem		(camerasystem),
			CameraControllers	(Framework->Core->GetBlockMemory()),
			Nodes				(Framework->Core->Nodes),
			States				(Framework->Core->GetBlockMemory()){}


		void ObjectMoved	(ComponentHandle Handle, ComponentSystemInterface* System, GameObjectInterface* GO)
		{
			CameraControllers[Handle].GO = GO;
		}


		void ReleaseHandle	(ComponentHandle Handle)
		{

		}


		void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID)
		{

		}


		void Yaw(ComponentHandle Handle, float R)
		{
			CameraControllers[Handle].Yaw += R;
		}


		void Pitch(ComponentHandle Handle, float R)
		{
			CameraControllers[Handle].Pitch += R;
		}


		void Roll(ComponentHandle Handle, float R)
		{
			CameraControllers[Handle].Roll += R;
		}


		void OffsetYawNode(ComponentHandle Handle, float3 xyz)
		{
			Nodes->SetPositionL(CameraControllers[Handle].Yaw_Node, xyz);
		}


		void SetOffset(ComponentHandle Handle, float3 Offset)
		{
			if (Handle == InvalidComponentHandle)
				return;

			auto Pitch = CameraControllers[Handle].Pitch_Node;
			SetPositionL(*Nodes, Pitch, Offset);
		}


		float3 GetForwardVector(ComponentHandle Handle)
		{
			if (Handle == InvalidComponentHandle)
				return float3(0);

			float3 Forward(0, 0, -1);
			Quaternion Q = GetOrientation(*Nodes, CameraControllers[Handle].Yaw_Node);
			return Q * Forward;
		}


		float3 GetLeftVector(ComponentHandle Handle)
		{
			if (Handle == InvalidComponentHandle)
				return float3(0);

			float3 Left(-1, 0, 0);
			Quaternion Q = GetOrientation(*Nodes, CameraControllers[Handle].Yaw_Node);
			return Q * Left;
		}


		CameraControllerHandle CreateController(GameObjectInterface* GO, NodeHandle CameraNode)
		{
			CameraController Controller;

			if (CameraNode == InvalidComponentHandle)
				return InvalidComponentHandle;

			ReleaseNode(*Nodes, CameraNode);

			auto Yaw	= GetZeroedNode(*Nodes);
			auto Pitch	= GetZeroedNode(*Nodes);
			auto Roll	= CameraNode;

			Nodes->SetParentNode(GetNodeHandle(GO), Yaw);
			Nodes->SetParentNode(Yaw, Pitch);
			Nodes->SetParentNode(Pitch, Roll);
			
			//C->Node					= Roll;
			Controller.GO			= GO;

			Controller.Pitch	    = 0;
			Controller.Roll			= 0;
			Controller.Yaw		    = 0;

			Controller.Pitch_Node  = Pitch;
			Controller.Roll_Node   = Roll;
			Controller.Yaw_Node    = Yaw;

			States.push_back(Dirty);
			CameraControllers.push_back(Controller);

			return CameraControllerHandle(CameraControllers.size() - 1);
		}


		void Update(double dT)
		{
			auto MouseState = Input->GetMouseState();

			for(auto& C : CameraControllers)
			{
				C.Yaw	+= MouseState.dPos[0];
				C.Pitch += MouseState.dPos[1];

				if (C.Pitch > 75)
					C.Pitch = 75;

				if (C.Pitch < -45)
					C.Pitch = -45;

				Nodes->SetParentNode(GetNodeHandle(C.GO), C.Yaw_Node);

				SetOrientationL(*Nodes, C.Yaw_Node,		Quaternion( 0,			C.Yaw,	0));
				SetOrientationL(*Nodes, C.Pitch_Node,	Quaternion( C.Pitch,	0,		0));
				SetOrientationL(*Nodes, C.Roll_Node,	Quaternion( 0,			0,		C.Roll));
			}
		}


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

			GameObjectInterface*	GO;
		};


		CameraComponentSystem*		CameraSystem;
		SceneNodeComponentSystem*	Nodes;
		InputComponentSystem*		Input;

		Vector<State>				States;
		Vector<CameraController>	CameraControllers;
	};

	const uint32_t ThirdPersonCameraComponentID = GetTypeGUID(ThirdPersonCameraComponent);

	struct ThirdPersonCameraArgs
	{
		ThirdPersonCameraComponentSystem*	System;
		ComponentHandle						Camera;
	};

	template<size_t SIZE>
	void CreateComponent(GameObject<SIZE>& GO, ThirdPersonCameraArgs& Args)
	{
		auto T			= (TansformComponent*)FindComponent(GO, TransformComponentID);
		auto CameraNode = GetCameraSceneNode(GO);

		if (!T)
			CreateComponent(GO, TransformComponentArgs{ Args.System->Nodes, Args.System->Nodes->GetZeroedNode() });

		auto Handle = Args.System->CreateController(GO, CameraNode);
		GO.AddComponent(Component(Args.System, Handle, ThirdPersonCameraComponentID));
		Args.System->Input->BindInput(Handle, Args.System);
	}


	ThirdPersonCameraArgs CreateThirdPersonCamera(ThirdPersonCameraComponentSystem* System);


	void PitchCamera	(GameObjectInterface* GO, float R)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			Component->Pitch(C->ComponentHandle, R);
			//NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}


	void YawCamera		(GameObjectInterface* GO, float R)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			Component->Yaw(C->ComponentHandle, R);
			//NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}


	void RollCamera		(GameObjectInterface* GO, float R)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			Component->Roll(C->ComponentHandle, R);
			//NotifyAll(GO, TransformComponentID, GetCRCGUID(ORIENTATION));
		}
	}


	void SetCameraOffset(GameObjectInterface* GO, float3 Offset)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			Component->SetOffset(C->ComponentHandle, Offset);
		}
	}


	void OffsetYawNode(GameObjectInterface* GO, float3 Offset)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			Component->OffsetYawNode(C->ComponentHandle, Offset);
		}
	}


	float3 GetForwardVector(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			return Component->GetForwardVector(C->ComponentHandle);
		}
		return float3(0);
	}


	float3 GetLeftVector(GameObjectInterface* GO)
	{
		auto C = FindComponent(GO, ThirdPersonCameraComponentID);
		if (C) {
			auto Component = (ThirdPersonCameraComponentSystem*)C->ComponentSystem;
			return Component->GetLeftVector(C->ComponentHandle);
		}
		return float3(0);
	}


	/************************************************************************************************/
}
#endif	