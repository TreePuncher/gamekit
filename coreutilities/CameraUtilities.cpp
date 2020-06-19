/**********************************************************************

Copyright (c) 2019 Robert May

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
		CameraView{ handle }
	{
		yawNode		= GetZeroedNode();
		pitchNode	= GetZeroedNode();
		rollNode	= GetZeroedNode();
		moveRate	= movementSpeed;

		SetParentNode(pitchNode, rollNode);
		SetParentNode(yawNode, pitchNode);

		CameraComponent::GetComponent().SetCameraNode(camera, rollNode);

		TranslateWorld(initialPos);
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::Update(const MouseInputState& mouseInput, double dt)
	{
		Yaw(mouseInput.Normalized_dPos[0] * dt * pi * 50);
		Pitch(mouseInput.Normalized_dPos[1] * dt * pi * 50);

		float3 movementVector	{ 0 };
		float3 forward			{ GetForwardVector() };
		float3 right			{ GetRightVector() };
        float3 up               { 0, 1, 0 };

		if (keyStates.forward)
			movementVector +=  forward;

		if (keyStates.backward)
			movementVector += -forward;

		if (keyStates.right)
			movementVector +=  right;

		if (keyStates.left)
			movementVector += -right;

        if (keyStates.up)
            movementVector += up;

        if (keyStates.down)
            movementVector += -up;


        movementVector.normalize();

		if (keyStates.KeyPressed())
			velocity += movementVector * acceleration * dt;

        if (velocity.magnitudesquared() > 0.01f) {
            velocity -= velocity * drag * dt;
            TranslateWorld(velocity * dt);
        }
        else
            velocity = 0;
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

		_CameraDirty();
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

		_CameraDirty();
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
            case OCE_MoveUp:
                keyStates.up        = state;
                break;
            case OCE_MoveDown:
                keyStates.down      = state;
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
		return CameraComponent::GetComponent().GetCameraNode(camera);
	}


	/************************************************************************************************/


	auto& QueueOrbitCameraUpdateTask(
					UpdateDispatcher&		dispatcher, 
					OrbitCameraBehavior&	orbitCamera, 
					MouseInputState			mouseState, 
					float					dt)
	{
		struct OrbitCameraUpdateData
		{
			MouseInputState			mouseState;
			float					dt;
		};

		auto& data = dispatcher.Add<OrbitCameraUpdateData>(
			[&](UpdateDispatcher::UpdateBuilder& Builder, OrbitCameraUpdateData& data)
			{
				Builder.SetDebugString("OrbitCamera Update");

				data.mouseState		= mouseState;
				data.dt				= dt;
			},
			[&orbitCamera](auto& data, iAllocator& threadAllocator)
			{
				FK_LOG_9("OrbitCamera Update");

				orbitCamera.Update(data.mouseState, data.dt);
			});

		return data;
	}


}	/************************************************************************************************/
