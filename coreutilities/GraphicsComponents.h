#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

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
		float	FOV			= pi/3,
		float	AspectRatio = 1.0f,
		float	Near		= 0.1f,
		float	Far			= 10000.0f,
		bool	Invert		= false);


	FLEXKITAPI void							MarkDirty				(CameraHandle);
	FLEXKITAPI void							SetCameraNode			(CameraHandle, NodeHandle);

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


	UpdateTask* QueueCameraUpdate(UpdateDispatcher&, UpdateTask* TransformDependency);


	struct DefaultCameraInteractor
	{
		using ParentType_t = void*;

		template<typename TY>
		void OnDirty(TY* cameraBehavior, void*)
		{
			MarkDirty(cameraBehavior->camera);
		}
	};

	template<typename TY_Interactor = DefaultCameraInteractor>
	class CameraBehavior_t : public DefaultCameraInteractor
	{
	public:
		CameraBehavior_t(CameraHandle IN_camera = CreateCamera()) :
			camera{IN_camera}{}


		void SetCameraAspectRatio(float AspectRatio)
		{
			FlexKit::SetCameraAspectRatio(camera, AspectRatio);
			_CameraDirty();
		}


		void SetCameraNode(NodeHandle Node)
		{
			FlexKit::SetCameraNode(camera, Node);
			_CameraDirty();
		}


		void SetCameraFOV(float r)
		{
			FlexKit::SetCameraFOV(camera, r);
			_CameraDirty();
		}


		NodeHandle GetCameraNode()
		{
			return FlexKit::GetCameraNode(camera);
		}


		float GetCameraFov()
		{
			return FlexKit::GetCameraFOV(camera);
		}


		Camera::CameraConstantBuffer	GetCameraConstantBuffer()
		{
			return FlexKit::GetCameraConstantBuffer(camera);
		}


		operator CameraHandle () {
			return camera;
		}


		CameraHandle camera;

	protected:

		void _CameraDirty()
		{
			OnDirty(this, static_cast<TY_Interactor::ParentType_t>(this));
		}
	};


	/************************************************************************************************/


	inline Frustum GetFrustum(CameraHandle camera)
	{
		auto Node = GetCameraNode(camera);

		return GetFrustum(
			GetCameraAspectRatio(camera),
			GetCameraFOV		(camera),
			GetCameraNear		(camera),
			GetCameraFar		(camera),
			GetPositionW		(Node),
			GetOrientation		(Node));
	}


	using CameraBehavior = CameraBehavior_t<>;


	/************************************************************************************************/
}// FlexKit


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

#endif // GRAPHICSCOMPONENTS_H