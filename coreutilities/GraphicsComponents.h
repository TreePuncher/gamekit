#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

#include "Components.h"
#include "memoryutilities.h"
#include "Transforms.h"
#include "intersection.h"
#include "CoreSceneObjects.h"


namespace FlexKit
{
	/************************************************************************************************/


	typedef Handle_t<32, GetCRCGUID(CameraHandle)> CameraHandle;

	constexpr ComponentID CameraComponentID = GetTypeGUID(CameraComponentID);

	class CameraComponent : public Component<CameraComponent, CameraComponentID>
	{
	public:
		CameraComponent(iAllocator* allocator) : 
			handles		{ allocator },
			DirtyFlags	{ allocator },
			handleRef	{ allocator },
			Cameras		{ allocator } {}

		~CameraComponent() {}

		CameraHandle CreateCamera(
			const float	FOV			= pi/3,
			const float	AspectRatio = 1.0f,
			const float	Near		= 0.1f,
			const float	Far			= 10000.0f,
			const bool	Invert		= false);

		void							ReleaseCamera(CameraHandle camera);

		Camera&							GetCamera(CameraHandle);

		void							MarkDirty				(CameraHandle);

		void							SetCameraAspectRatio	(CameraHandle, float);
		void							SetCameraNode			(CameraHandle, NodeHandle);
		void							SetCameraFOV			(CameraHandle, float);
		void							SetCameraNear			(CameraHandle, float);
		void							SetCameraFar			(CameraHandle, float);
	
		float							GetCameraAspectRatio	(CameraHandle);
		float							GetCameraFar			(CameraHandle);
		float							GetCameraFOV			(CameraHandle);
		NodeHandle						GetCameraNode			(CameraHandle);
		float							GetCameraNear			(CameraHandle);
		Camera::ConstantBuffer			GetCameraConstants		(CameraHandle);
		float4x4						GetCameraPV				(CameraHandle);

		auto&	QueueCameraUpdate(UpdateDispatcher& dispatcher)
        {
		    struct UpdateData
		    {};

		    auto& task = dispatcher.Add<UpdateData>(
			    [&](UpdateDispatcher::UpdateBuilder& Builder, auto& Data)
			    {
				    Builder.SetDebugString("QueueCameraUpdate");
			    },
			    [this](auto& Data, iAllocator& threadAllocator)
			    {
				    FK_LOG_9("Updating Cameras");

				    size_t End = Cameras.size();
				    for (size_t I = 0; I < End; ++I)
				    {
					    if (DirtyFlags[I])
					    {
						    Cameras[I].UpdateMatrices();
						    DirtyFlags[I] = false;
					    }
				    }

				    return;
			    });

		    return task;
	    }

		Vector<bool>								DirtyFlags;
		Vector<Camera>								Cameras;
		Vector<CameraHandle>						handleRef;
		HandleUtilities::HandleTable<CameraHandle>	handles;
	};

	/************************************************************************************************/


	inline void								SetCameraAspectRatio	(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraAspectRatio(camera, a);	}
	inline void								SetCameraNode			(CameraHandle camera, NodeHandle node)		{ CameraComponent::GetComponent().SetCameraNode(camera, node);		}
	inline void								SetCameraFOV			(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraFOV(camera, a);			}
	inline void								SetCameraNear			(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraNear(camera, a);			}
	inline void								SetCameraFar			(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraFar(camera, a);			}
		
	inline float							GetCameraAspectRatio	(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraAspectRatio(camera);	}
	inline float							GetCameraFar			(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraFar(camera);			}
	inline float							GetCameraFOV			(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraFOV(camera);			}
	inline NodeHandle						GetCameraNode			(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraNode(camera);			}
	inline float							GetCameraNear			(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraNear(camera);			}
	inline Camera::ConstantBuffer			GetCameraConstants		(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraConstants(camera);		}
	inline float4x4							GetCameraPV				(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraPV(camera);				}



    /************************************************************************************************/


	struct DefaultCameraInteractor
	{
		using ParentType_t = void*;

		template<typename TY>
		void OnDirty(TY* cameraBehavior, void*)
		{
			CameraComponent::GetComponent().MarkDirty(cameraBehavior->camera);
		}
	};

	template<typename TY_Interactor = DefaultCameraInteractor>
	class CameraView_t : 
		public ComponentView_t<CameraComponent>,
		public DefaultCameraInteractor
	{
	public:
		CameraView_t(CameraHandle IN_camera = CameraComponent::GetComponent().CreateCamera()) :
			camera{IN_camera}{}


		~CameraView_t()
		{
			GetComponent().ReleaseCamera(camera);
		}


		void SetCameraAspectRatio(float AspectRatio)
		{
			GetComponent().SetCameraAspectRatio(camera, AspectRatio);
			_CameraDirty();
		}


		void SetCameraNode(NodeHandle Node)
		{
			GetComponent().SetCameraNode(camera, Node);
			_CameraDirty();
		}


		void SetCameraFOV(float r)
		{
			GetComponent().SetCameraFOV(camera, r);
			_CameraDirty();
		}


		NodeHandle GetCameraNode()
		{
			return GetComponent().GetCameraNode(camera);
		}


		float GetCameraFov()
		{
			return GetComponent().GetCameraFOV();
		}


		operator CameraHandle () 
		{
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
		auto node = GetCameraNode(camera);

		return GetFrustum(
			GetCameraAspectRatio(camera),
			GetCameraFOV		(camera),
			GetCameraNear		(camera),
			GetCameraFar		(camera),
			GetPositionW		(node),
			GetOrientation		(node));
	}


	inline Frustum GetSubFrustum(CameraHandle camera, float2 UL, float2 BR)
	{
		auto& sceneNodeComponent	= SceneNodeComponent::GetComponent();
		auto node					= GetCameraNode(camera);

		return GetSubFrustum(
			GetCameraAspectRatio(camera),
			GetCameraFOV		(camera),
			GetCameraNear		(camera),
			GetCameraFar		(camera),
			GetPositionW		(node),
			GetOrientation		(node), 
			UL, 
			BR);
	}


	using CameraView = CameraView_t<>;


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
