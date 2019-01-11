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


#include "CameraUtilities.h"
#include "EngineCore.h"


namespace FlexKit
{
	/************************************************************************************************/


	OrbitCameraBehavior::OrbitCameraBehavior(CameraHandle handle, float movementSpeed, float3 initialPos) :
		CameraBehavior{ handle }
	{
		yawNode		= GetZeroedNode();
		pitchNode	= GetZeroedNode();
		rollNode	= GetZeroedNode();
		moveRate	= movementSpeed;

		SetParentNode(pitchNode, rollNode);
		SetParentNode(yawNode, pitchNode);

		FlexKit::SetCameraNode(camera, rollNode);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Update(const MouseInputState& mouseInput, double dt)
	{
		Yaw(mouseInput.Normalized_dPos[0] * dt * pi * 50);
		Pitch(mouseInput.Normalized_dPos[1] * dt * pi * 50);

		float3 movementVector	{ 0 };
		float3 forward			{ GetForwardVector() };
		float3 right			{ GetRightVector() };

		if (keyStates.forward)
			movementVector +=  forward;

		if (keyStates.backward)
			movementVector += -forward;

		if (keyStates.right)
			movementVector +=  right;

		if (keyStates.left)
			movementVector += -right;

		if (movementVector.magnitudesquared() > 0.0001f)
		{
			movementVector = movementVector.normal();
			TranslateWorld(movementVector * dt * 1000);
		}
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::SetCameraPosition(float3 xyz)
	{
		SetPositionW(yawNode, xyz);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::TranslateWorld(float3 xyz)
	{
		FlexKit::TranslateWorld(yawNode, xyz);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Rotate(float3 xyz)
	{
		if (xyz[0] != 0.0f)
			FlexKit::Pitch(pitchNode, xyz[0]);

		if (xyz[1] != 0.0f)
			FlexKit::Yaw(yawNode, xyz[1]);

		if (xyz[2] != 0.0f)
			FlexKit::Roll(pitchNode, xyz[2]);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::HandleEvent(FlexKit::Event evt)
	{
		if (evt.InputSource == FlexKit::Event::Keyboard)
		{
			bool state = evt.Action == Event::Pressed ? true : false;

			switch (evt.mData1.mINT[0])
			{
			case OCE_MoveForward:
				keyStates.forward	= state;
				break;
			case OCE_MoveBackward:
				keyStates.backward	= state;
				break;
			case OCE_MoveLeft:
				keyStates.left		= state;
				break;
			case OCE_MoveRight:
				keyStates.right		= state;
				break;
			}
		}
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
		return FlexKit::GetOrientation(pitchNode);
	}


	/************************************************************************************************/


	float3 OrbitCameraBehavior::GetForwardVector()
	{
		Quaternion Q = GetOrientation();
		return -(Q * float3(0, 0, 1)).normal();
	}


	/************************************************************************************************/


	float3 OrbitCameraBehavior::GetRightVector()
	{
		Quaternion Q = GetOrientation();
		return -(Q * float3(-1, 0, 0)).normal();
	}


	/************************************************************************************************/


	NodeHandle	OrbitCameraBehavior::GetCameraNode()
	{
		return FlexKit::GetCameraNode(camera);
	}


}	/************************************************************************************************/