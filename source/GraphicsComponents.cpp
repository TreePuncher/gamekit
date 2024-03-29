#include "Scene.h"

namespace FlexKit
{
	/************************************************************************************************/

	CameraHandle CameraComponent::CreateCamera(
		const float	FOV,			
		const float	AspectRatio,
		const float	Near,	
		const float	Far,			
		const bool	Invert)
	{
		auto handle = handles.GetNewHandle();

		Camera NewCamera;
		NewCamera.FOV			= FOV;
		NewCamera.Far			= Far;
		NewCamera.Near			= Near;
		NewCamera.invert		= Invert;
		NewCamera.AspectRatio	= AspectRatio;
		NewCamera.Node			= NodeHandle{(unsigned int)-1};

		DirtyFlags.push_back(true);
		handleRef.push_back(handle);
		handles[handle] = (index_t)Cameras.push_back(NewCamera);

		return handle;
	}


	/************************************************************************************************/


	void CameraComponent::FreeComponentView(void* _ptr)
	{
		static_cast<CameraView*>(_ptr)->Release();
	}


	/************************************************************************************************/


	void CameraComponent::Release(CameraHandle camera)
	{
		GetCamera(handleRef.back()) = Cameras.back();
		handles[handleRef.back()]	= handles[camera];

		Cameras.pop_back();
		handles.RemoveHandle(camera);
	}


	/************************************************************************************************/


	Camera& CameraComponent::GetCamera(CameraHandle handle)
	{
		const auto idx = handles[handle];
		if (DirtyFlags[idx])
		{
			Cameras[idx].UpdateMatrices();
			DirtyFlags[idx] = false;
		}
		return Cameras[handles[handle]];
	}


	/************************************************************************************************/


	void CameraComponent::SetCameraAspectRatio(CameraHandle handle, float A)
	{
		GetCamera(handle).AspectRatio = A;
	}


	/************************************************************************************************/


	void CameraComponent::MarkDirty(CameraHandle handle)
	{
		DirtyFlags[handles[handle]] = true;
	}


	/************************************************************************************************/


	void CameraComponent::SetCameraNode(CameraHandle handle, NodeHandle Node)
	{
		GetCamera(handle).Node = Node;
	}


	/************************************************************************************************/


	void CameraComponent::SetCameraFOV(CameraHandle handle, float f)
	{
		GetCamera(handle).FOV = f;
	}


	void CameraComponent::SetCameraNear(CameraHandle handle, float f)
	{
		GetCamera(handle).Near = f;
	}


	/************************************************************************************************/


	void CameraComponent::SetCameraFar(CameraHandle handle, float f)
	{
		GetCamera(handle).Far = f;
	}


	/************************************************************************************************/


	float CameraComponent::GetCameraAspectRatio(CameraHandle handle)
	{
		return GetCamera(handle).AspectRatio;
	}


	/************************************************************************************************/


	float CameraComponent::GetCameraFar(CameraHandle handle)
	{
		return GetCamera(handle).Far;
	}


	/************************************************************************************************/


	float CameraComponent::GetCameraFOV(CameraHandle handle)
	{
		return GetCamera(handle).FOV;
	}


	/************************************************************************************************/


	float CameraComponent::GetCameraNear(CameraHandle handle)
	{
		return GetCamera(handle).Near;
	}


	/************************************************************************************************/


	NodeHandle CameraComponent::GetCameraNode(CameraHandle handle)
	{
		return GetCamera(handle).Node;
	}


	/************************************************************************************************/


	Camera::ConstantBuffer CameraComponent::GetCameraPreviousConstants(CameraHandle handle)
	{
		return GetCamera(handle).GetCameraPreviousConstants();
	}


	/************************************************************************************************/


	Camera::ConstantBuffer CameraComponent::GetCameraConstants(CameraHandle handle)
	{
		return GetCamera(handle).GetConstants();
	}


	float4x4 CameraComponent::GetCameraPV(CameraHandle handle)
	{
		return GetCamera(handle).GetPV();
	}


	/************************************************************************************************/


	Ray ViewRay(CameraHandle camera, const float2 UV)
	{
		const auto cameraConstants		= FlexKit::GetCameraConstants(camera);
		const auto cameraOrientation	= FlexKit::GetOrientation(FlexKit::GetCameraNode(camera));

		const FlexKit::float3 v_dir	= cameraOrientation * (Inverse(cameraConstants.Proj) * FlexKit::float4{ UV.x, UV.y,  1.0f, 1.0f }).xyz().normal();
		const FlexKit::float3 v_o	= cameraConstants.WPOS.xyz();

		return { .D = v_dir, .O = v_o };
	}


	/************************************************************************************************/


	Ray ViewRay(GameObject& gameObject, const float2 UV)
	{
		return Apply(gameObject,
			[&](CameraView& view)
			{
				return ViewRay(view, UV);
			},
			[]() -> Ray
			{
				FK_LOG_WARNING("ViewRay Called on gameObject without camera component");
				return {};
			});
	}


	/************************************************************************************************/
}// nemespace FlexKit


/**********************************************************************

Copyright (c) 2015 - 2019 Robert May

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
