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


#include "stdafx.h"
#include "CameraUtilities.h"



namespace FlexKit
{
	/************************************************************************************************/


	struct OrbitCameraSystem
	{
		OrbitCameraSystem(iAllocator* Memory) :
			Controllers			{Memory},
			Memory				{Memory}
		{
		}

		~OrbitCameraSystem()
		{}

		Vector<Camera>					Cameras;
		Vector<CameraOrbitController>	Controllers;

		iAllocator*						Memory;
	}*OrbitCameras;


	/************************************************************************************************/


	void InitiateOrbitCameras(iAllocator* Memory)
	{
		OrbitCameras = &Memory->allocate<OrbitCameraSystem>(Memory);
	}


	/************************************************************************************************/


	void ReleaseOrbitCameras(iAllocator* Memory)
	{
		OrbitCameras->~OrbitCameraSystem();
		Memory->free(OrbitCameras);
		OrbitCameras = nullptr;
	}


	/************************************************************************************************/


	void UpdateOrbitCamera(const MouseInputState& MouseInput, double dt)
	{

	}


	/************************************************************************************************/


	OrbitCamera_Handle CreateOrbitCamera(NodeHandle Node, NodeHandle CameraNode, float MoveRate)
	{
		FK_ASSERT(OrbitCameras != nullptr);
		FK_ASSERT(Node != CameraNode);

		CameraOrbitController Controller;
		Controller.CameraNode	= CameraNode;
		Controller.YawNode		= Node;
		Controller.PitchNode	= GetZeroedNode();
		Controller.RollNode		= GetZeroedNode();
		Controller.MoveRate		= MoveRate;

		SetParentNode(Controller.YawNode, Controller.PitchNode);
		SetParentNode(Controller.PitchNode, Controller.RollNode);
		SetParentNode(Controller.RollNode, CameraNode);

		OrbitCameras->Controllers.push_back(Controller);

		return OrbitCamera_Handle(OrbitCameras->Controllers.size() - 1);
	}


	/************************************************************************************************/


	NodeHandle GetNode(OrbitCamera_Handle Handle)
	{
		FK_ASSERT(OrbitCameras != nullptr);
		return OrbitCameras->Controllers[Handle].YawNode;
	}


	/************************************************************************************************/


	CameraOrbitController GetOrbitController(OrbitCamera_Handle Handle)
	{
		FK_ASSERT(OrbitCameras != nullptr);
		return OrbitCameras->Controllers[Handle];
	}


	/************************************************************************************************/


	void SetCameraNode(OrbitCamera_Handle Handle, NodeHandle Node)
	{
		FK_ASSERT(OrbitCameras != nullptr);

		OrbitCameras->Controllers[Handle].CameraNode = Node;

		SetParentNode(
			OrbitCameras->Controllers[Handle].CameraNode,
			OrbitCameras->Controllers[Handle].PitchNode);
	}


	/************************************************************************************************/


	Quaternion GetCameraOrientation(OrbitCamera_Handle handle)
	{
		return GetOrientation(
			OrbitCameras->Controllers[handle].CameraNode);
	}


	/************************************************************************************************/


	void SetCameraPosition(OrbitCamera_Handle Handle, float3 xyz)
	{
		FK_ASSERT(OrbitCameras != nullptr);

		auto& OrbitCamera = OrbitCameras->Controllers[Handle];
		SetPositionW(OrbitCamera.YawNode, xyz);
	}


	/************************************************************************************************/


	void TranslateCamera(OrbitCamera_Handle Handle, float3 xyz)
	{
		FK_ASSERT(OrbitCameras != nullptr);

		auto& OrbitCamera = OrbitCameras->Controllers[Handle];
		TranslateWorld(OrbitCamera.YawNode, xyz);
	}


	/************************************************************************************************/


	void RotateCamera(OrbitCamera_Handle Handle, float3 xyz)
	{
		FK_ASSERT(OrbitCameras != nullptr);

		auto& OrbitCamera = OrbitCameras->Controllers[Handle];

		if(xyz[0] != 0.0f)
			Pitch(OrbitCamera.PitchNode, xyz[0]);

		if(xyz[1] != 0.0f)
			Yaw(OrbitCamera.YawNode, xyz[1]);

		if (xyz[2] != 0.0f)
			Roll(OrbitCamera.PitchNode, xyz[2]);
	}


	/************************************************************************************************/


	OrbitCameraBehavior::OrbitCameraBehavior(float MovementSpeed, float3 InitialPos)
	{

	}
	

	/************************************************************************************************/


	void OrbitCameraBehavior::Update(const MouseInputState& MouseInput)
	{

	}


	/************************************************************************************************/


	void OrbitCameraBehavior::SetCameraPosition(float3 xyz)
	{
		FlexKit::SetCameraPosition(Handle, xyz);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::TranslateWorld(float3 xyz)
	{
		FlexKit::TranslateCamera(Handle, xyz);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Rotate(float3 xyz)
	{
		FlexKit::RotateCamera(Handle, xyz);
	}

	/************************************************************************************************/


	void OrbitCameraBehavior::Yaw(float Theta)
	{
		Rotate({ 0, Theta, 0 });
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Pitch(float Theta)
	{
		Rotate({ Theta, 0, 0 });
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Roll(float Theta)
	{
		Rotate({ 0, 0, Theta });
	}

	/************************************************************************************************/


	Quaternion	OrbitCameraBehavior::GetOrientation()
	{
		return GetCameraOrientation(Handle);
	}


	/************************************************************************************************/


	float3 OrbitCameraBehavior::GetForwardVector()
	{
		Quaternion Q = GetCameraOrientation(Handle);
		return -(Q * float3(0, 0, 1)).normal();
	}


	/************************************************************************************************/


	float3 OrbitCameraBehavior::GetRightVector()
	{
		Quaternion Q = GetCameraOrientation(Handle);
		return -(Q * float3(-1, 0, 0)).normal();
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::SetCameraNode(NodeHandle Node)
	{

	}


	/************************************************************************************************/


	NodeHandle	OrbitCameraBehavior::GetCameraNode()
	{
		auto& Controller = GetControllerEntry();
		return Controller.CameraNode;
	}


	/************************************************************************************************/


	CameraOrbitController OrbitCameraBehavior::GetControllerEntry()
	{
		return GetOrbitController(Handle);
	}


}	/************************************************************************************************/