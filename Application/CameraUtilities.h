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
#include "../coreutilities/MathUtils.h"

using FlexKit::NodeHandle;
using FlexKit::SceneNodes;
using FlexKit::Camera;
using FlexKit::ComponentSystemInterface;


namespace FlexKit
{	/************************************************************************************************/


	struct CameraOrbitController
	{
		NodeHandle CameraNode;
		NodeHandle YawNode;
		NodeHandle PitchNode;
		NodeHandle RollNode;

		float MoveRate;
	};


	struct OrbitCameraInputState
	{
		MouseInputState Mouse;
		bool Forward;
		bool Backward;
		bool Left;
		bool Right;
	};

	
	/************************************************************************************************/


	typedef Handle_t<32> OrbitCamera_Handle;

	void InitiateOrbitCameras	(iAllocator* Memory);
	void ReleaseOrbitCameras	(iAllocator* Memory);
	void UpdateOrbitCamera		(OrbitCameraInputState& Input, double dt);

	OrbitCamera_Handle CreateOrbitCamera(NodeHandle Node, NodeHandle CameraNode, float MoveRate);

	void		SetCameraNode			(OrbitCamera_Handle Handle, NodeHandle Node);

	NodeHandle				GetNode					(OrbitCamera_Handle Handle);
	Quaternion				GetCameraOrientation	(OrbitCamera_Handle handle);
	CameraOrbitController	GetOrbitController		(OrbitCamera_Handle Handle);


	/************************************************************************************************/


	void SetCameraPosition	(OrbitCamera_Handle Handle, float3 xyz);
	void TranslateCamera	(OrbitCamera_Handle Handle, float3 xyz);
	void RotateCamera		(OrbitCamera_Handle Handle, float3 xyz); // Three Angles


	/************************************************************************************************/


	class OrbitCameraBehavior
	{
	public:
		OrbitCameraBehavior(float MovementSpeed, float3 InitialPos);

		void Update				(const OrbitCameraInputState& Input);

		void SetCameraPosition	(float3 xyz);
		void TranslateWorld		(float3 xyz);

		void Yaw				(float Degree);
		void Pitch				(float Degree);
		void Roll				(float Degree);
		void Rotate				(float3 xyz); // Three Angles

		Quaternion	GetOrientation();
		float3		GetForwardVector();
		float3		GetRightVector();

		void		SetCameraNode(NodeHandle Node);
		NodeHandle	GetCameraNode();

		CameraOrbitController	GetControllerEntry();

	private:
		OrbitCamera_Handle	Handle;
	};

}	/************************************************************************************************/

#endif	