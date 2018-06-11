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


#include "Components.h"
#include "memoryutilities.h"
#include "Transforms.h"
#include "intersection.h"
#include "..\graphicsutilities\CoreSceneObjects.h"

namespace FlexKit
{
	/************************************************************************************************/


	typedef Handle_t<32, GetCRCGUID(CameraHandle)> CameraHandle;


	void InitiateCameraTable(iAllocator* Memory);
	void ReleaseCameraTable();


	CameraHandle CreateCamera(
		float	FOV			= pi/2.5,
		float	AspectRatio = 1.0f,
		float	Near		= 0.1f,
		float	Far			= 10000.0f,
		bool	Invert		= false);

	FLEXKITAPI void							SetCameraNode(CameraHandle, NodeHandle);

	FLEXKITAPI void							SetCameraAspectRatio	(CameraHandle, float);
	FLEXKITAPI void							SetCameraNode			(CameraHandle, NodeHandle);
	FLEXKITAPI void							SetCameraFOV			(CameraHandle, float);
	FLEXKITAPI void							SetCameraNear			(CameraHandle, float);
	FLEXKITAPI void							SetCameraFar			(CameraHandle, float);
	
	FLEXKITAPI float						GetCameraAspectRatio	(CameraHandle);
	FLEXKITAPI float						GetCameraFar			(CameraHandle);
	FLEXKITAPI float						GetCameraFOV			(CameraHandle);
	FLEXKITAPI NodeHandle					GetCameraNode			(CameraHandle);
	FLEXKITAPI float						GetCameraNear			(CameraHandle);
	FLEXKITAPI Camera::CameraConstantBuffer	GetCameraConstantBuffer	(CameraHandle);


	void QueueCameraUpdate(UpdateDispatcher&, UpdateTask* TransformDependency);


	class CameraBehavior
	{
	public:
		CameraBehavior(CameraHandle IN_Camera = CreateCamera()) : 
			Camera{IN_Camera}{}


		void SetCameraAspectRatio(float AspectRatio)
		{
			FlexKit::SetCameraAspectRatio(Camera, AspectRatio);
		}


		void SetCameraNode(NodeHandle Node)
		{
			FlexKit::SetCameraNode(Camera, Node);
		}


		void SetCameraFOV(float r)
		{
			FlexKit::SetCameraFOV(Camera, r);
		}


		NodeHandle GetCameraNode()
		{
			return FlexKit::GetCameraNode(Camera);
		}


		float GetCameraFov()
		{
			return FlexKit::GetCameraFOV(Camera);
		}


		Camera::CameraConstantBuffer	GetCameraConstantBuffer()
		{
			return FlexKit::GetCameraConstantBuffer(Camera);
		}


		operator CameraHandle () {
			return Camera;
		}


		CameraHandle Camera;
	};


	/************************************************************************************************/


	inline Frustum GetFrustum(CameraHandle Camera)
	{
		auto Node = GetCameraNode(Camera);

		return GetFrustum(
			GetCameraAspectRatio(Camera),
			GetCameraFOV		(Camera),
			GetCameraNear		(Camera),
			GetCameraFar		(Camera),
			GetPositionW		(Node),
			GetOrientation		(Node));
	}


	/************************************************************************************************/
}// FlexKit

#endif // GRAPHICSCOMPONENTS_H