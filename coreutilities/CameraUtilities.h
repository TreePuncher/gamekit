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

#include "../graphicsutilities/graphics.h"
#include "../coreutilities/GraphicsComponents.h"
#include "../coreutilities/Transforms.h"
#include "../coreutilities/MathUtils.h"


namespace FlexKit
{	/************************************************************************************************/


	enum OrbitCameraEvents
	{
		OCE_MoveForward,
		OCE_MoveBackward,
		OCE_MoveLeft,
		OCE_MoveRight
	};


	class OrbitCameraBehavior :
		public CameraBehavior
	{
	public:
		OrbitCameraBehavior(CameraHandle Handle = CreateCamera(), float MovementSpeed = 100, float3 InitialPos = {0, 0, 0});

		void Update				(const MouseInputState& MouseInput, double dt);

		void SetCameraPosition	(float3 xyz);
		void TranslateWorld		(float3 xyz);

		void Yaw				(float Degree);
		void Pitch				(float Degree);
		void Roll				(float Degree);
		void Rotate				(float3 xyz); // Three Angles

		void HandleEvent(FlexKit::Event evt);

		Quaternion	GetOrientation();
		float3		GetForwardVector();
		float3		GetRightVector();

		NodeHandle	GetCameraNode();

	private:
		NodeHandle		cameraNode;
		NodeHandle		yawNode;
		NodeHandle		pitchNode;
		NodeHandle		rollNode;
		float			moveRate;

		struct KeyStates
		{
			bool forward	= false;
			bool backward	= false;
			bool left		= false;
			bool right		= false;
		}keyStates;
	};


	/************************************************************************************************/


	inline UpdateTask*	QueueOrbitCameraUpdateTask(
		UpdateDispatcher&		dispatcher, 
		UpdateTask*				transformUpdateDependency,
		UpdateTask*				cameraUpdateDependency,
		OrbitCameraBehavior&	orbitCamera, 
		MouseInputState			mouseState, 
		float dt)
	{
		struct OrbitCameraUpdateData
		{
			MouseInputState			mouseState;
			OrbitCameraBehavior*	orbitCamera;
			float					dt;
		};

		auto& Data = dispatcher.Add<OrbitCameraUpdateData>(
			[&](auto& Builder, OrbitCameraUpdateData& Data)
			{
				Builder.SetDebugString("OrbitCamera Update");
				Builder.AddOutput(*transformUpdateDependency);
				Builder.AddOutput(*cameraUpdateDependency);
				Data.mouseState		= mouseState;
				Data.dt				= dt;
				Data.orbitCamera	= &orbitCamera;
			},
			[](auto& data)
			{
				FK_LOG_9("OrbitCamera Update");
				data.orbitCamera->Update(data.mouseState, data.dt);
			});

		return &Data;
	}


}	/************************************************************************************************/

#endif	