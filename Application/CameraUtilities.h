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

	Camera*		C;
};


void InitiateCamera3rdPersonContoller	(Camera* C, Camera3rdPersonContoller* Out);
void UpdateCameraController				(Camera3rdPersonContoller* Controller, double dT);

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
	};
	

	typedef Handle_t<32> OrbitCamera_Handle;


	struct OrbitCameraSystem : public ComponentSystemInterface
	{
		OrbitCameraSystem(GameFramework* Framework, InputComponentSystem* Input) :
			Controllers(Framework->Core->GetBlockMemory()) {}

		operator OrbitCameraSystem* (){return this;}

		void ReleaseHandle	(ComponentHandle Handle);
		void HandleEvent	(ComponentHandle Handle, ComponentType EventSource, EventTypeID);

		void Update(double dT);

		CameraControllerHandle CreateOrbitCamera(NodeHandle Node, NodeHandle CameraNode, float MoveRate)
		{	
			FK_ASSERT(Node != CameraNode);

			CameraOrbitController Controller;
			Controller.CameraNode	= CameraNode;
			Controller.YawNode		= Node;
			Controller.PitchNode	= GetZeroedNode();
			Controller.RollNode		= GetZeroedNode();
			Controller.MoveRate		= MoveRate;

			SetParentNode(Controller.YawNode,	Controller.PitchNode);
			SetParentNode(Controller.PitchNode,	Controller.RollNode);
			SetParentNode(Controller.RollNode,	CameraNode);

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
			SetParentNode(Controllers[Handle].CameraNode, Controllers[Handle].PitchNode);
		}
		
		Quaternion GetCameraOrientation(ComponentHandle handle)
		{
			return GetOrientation(Controllers[handle].CameraNode);
		}

		Vector<CameraOrbitController>	Controllers;
	};


	const uint32_t OrbitCameraComponentID = GetTypeGUID(OrbitCamera);

	/************************************************************************************************/


	typedef Handle_t<32> ThirdPersonCamera_Handle;

	struct ThirdPersonCameraComponentSystem : public ComponentSystemInterface
	{
		ThirdPersonCameraComponentSystem(GameFramework* Framework) :
			CameraControllers	(Framework->Core->GetBlockMemory()),
			States				(Framework->Core->GetBlockMemory()){}

		void OffsetYawNode(ComponentHandle Handle, float3 xyz)
		{
			SetPositionL(CameraControllers[Handle].Yaw_Node, xyz);
		}


		void SetOffset(ComponentHandle Handle, float3 Offset)
		{
			if (Handle == InvalidComponentHandle)
				return;

			auto Pitch = CameraControllers[Handle].Pitch_Node;
			SetPositionL(Pitch, Offset);
		}


		float3 GetForwardVector(ComponentHandle Handle)
		{
			if (Handle == InvalidComponentHandle)
				return float3(0);

			float3 Forward(0, 0, -1);
			Quaternion Q = GetOrientation(CameraControllers[Handle].Yaw_Node);
			return Q * Forward;
		}


		float3 GetLeftVector(ComponentHandle Handle)
		{
			if (Handle == InvalidComponentHandle)
				return float3(0);

			float3 Left(-1, 0, 0);
			Quaternion Q = GetOrientation(CameraControllers[Handle].Yaw_Node);
			return Q * Left;
		}


		CameraControllerHandle CreateController(NodeHandle Node)
		{
			CameraController Controller;

			auto Yaw	= GetZeroedNode();
			auto Pitch	= GetZeroedNode();
			auto Roll	= Node;

			SetParentNode(Node, Yaw);
			SetParentNode(Yaw, Pitch);
			SetParentNode(Pitch, Roll);
			
			SetParentNode(Node, Yaw);

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


				//Nodes->SetParentNode(GetNodeHandle(C.GO), C.Yaw_Node);
				SetOrientationL(C.Yaw_Node,		Quaternion( 0,			C.Yaw,	0));
				SetOrientationL(C.Pitch_Node,	Quaternion( C.Pitch,	0,		0));
				SetOrientationL(C.Roll_Node,	Quaternion( 0,			0,		C.Roll));
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
		};


		InputComponentSystem*		Input;

		Vector<State>				States;
		Vector<CameraController>	CameraControllers;
	};


	const uint32_t ThirdPersonCameraComponentID = GetTypeGUID(ThirdPersonCameraComponent);


	class ThirdPersonCameraBehavior
	{
	public:
		ThirdPersonCameraBehavior(ThirdPersonCamera_Handle Handle)	{}

		void PitchCamera(float R)
		{
		}


		void YawCamera(float R)
		{
		}


		void RollCamera(float R)
		{
		}


		void SetCameraOffset(float3 Offset)
		{
		}


		void OffsetYawNode(float3 Offset)
		{
		}


		float3 GetForwardVector()
		{
			return float3(0);
		}


		float3 GetLeftVector()
		{
			return float3(0);
		}

		ThirdPersonCamera_Handle Handle;
	};


	/************************************************************************************************/
}
#endif	