#ifndef GRAPHICSCOMPONENTS_H
#define GRAPHICSCOMPONENTS_H

#include "Components.h"
#include "CoreSceneObjects.h"
#include "intersection.h"
#include "memoryutilities.h"
#include "Transforms.h"
#include "RuntimeComponentIDs.h"


namespace FlexKit
{	/************************************************************************************************/

	struct _CameraUpdate {};

	using CameraUpdateTask = UpdateTaskTyped<_CameraUpdate>;

	class CameraComponent : public Component<CameraComponent, CameraComponentID>
	{
	public:
		CameraComponent(iAllocator* allocator) : 
			handles		{ allocator },
			DirtyFlags	{ allocator },
			handleRef	{ allocator },
			Cameras		{ allocator } {}

		~CameraComponent() {}

		void FreeComponentView(void* _ptr) final;

		CameraHandle CreateCamera(
			const float	FOV			= pi/3,
			const float	AspectRatio = 1.0f,
			const float	Near		= 0.1f,
			const float	Far			= 10000.0f,
			const bool	Invert		= false);

		void							Release(CameraHandle camera);

		Camera&							GetCamera(CameraHandle);

		void							MarkDirty				(CameraHandle);

		void							SetCameraAspectRatio	(CameraHandle, float);
		void							SetCameraNode			(CameraHandle, NodeHandle);
		void							SetCameraFOV			(CameraHandle, float);
		void							SetCameraNear			(CameraHandle, float);
		void							SetCameraFar			(CameraHandle, float);
	
		float							GetCameraAspectRatio		(CameraHandle);
		float							GetCameraFar				(CameraHandle);
		float							GetCameraFOV				(CameraHandle);
		NodeHandle						GetCameraNode				(CameraHandle);
		float							GetCameraNear				(CameraHandle);
		Camera::ConstantBuffer			GetCameraConstants			(CameraHandle);
		Camera::ConstantBuffer			GetCameraPreviousConstants	(CameraHandle);
		float4x4						GetCameraPV					(CameraHandle);

		auto&	QueueCameraUpdate(UpdateDispatcher& dispatcher)
		{
			auto& task = dispatcher.Add<_CameraUpdate>(
				[&](UpdateDispatcher::UpdateBuilder& Builder, auto& Data)
				{
					Builder.SetDebugString("QueueCameraUpdate");
				},
				[this](auto& Data, iAllocator& threadAllocator)
				{
					ProfileFunction();

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


	Ray ViewRay(CameraHandle, const float2 UV);
	Ray ViewRay(GameObject&, const float2 UV);


	inline void								SetCameraAspectRatio		(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraAspectRatio(camera, a);	}
	inline void								SetCameraNode				(CameraHandle camera, NodeHandle node)		{ CameraComponent::GetComponent().SetCameraNode(camera, node);		}
	inline void								SetCameraFOV				(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraFOV(camera, a);			}
	inline void								SetCameraNear				(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraNear(camera, a);			}
	inline void								SetCameraFar				(CameraHandle camera, float a)				{ CameraComponent::GetComponent().SetCameraFar(camera, a);			}
		
	inline float							GetCameraAspectRatio		(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraAspectRatio(camera);		}
	inline float							GetCameraFar				(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraFar(camera);				}
	inline float							GetCameraFOV				(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraFOV(camera);				}
	inline NodeHandle						GetCameraNode				(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraNode(camera);				}
	inline float							GetCameraNear				(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraNear(camera);				}
	inline Camera::ConstantBuffer			GetCameraConstants			(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraConstants(camera);			}
	inline Camera::ConstantBuffer			GetCameraPreviousConstants	(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraPreviousConstants(camera);	}
	inline float4x4							GetCameraPV					(CameraHandle camera) { return CameraComponent::GetComponent().GetCameraPV(camera);					}

	inline void								MarkCameraDirty				(CameraHandle camera) { return CameraComponent::GetComponent().MarkDirty(camera); }


	/************************************************************************************************/


	class CameraView : 
		public ComponentView_t<CameraComponent>
	{
	public:
		CameraView(GameObject& go, CameraHandle IN_camera = CameraComponent::GetComponent().CreateCamera()) :
			camera{IN_camera}{}


		void Release()
		{
			GetComponent().Release(camera);
		}


		void SetCameraAspectRatio(float AspectRatio)
		{
			GetComponent().SetCameraAspectRatio(camera, AspectRatio);
			GetComponent().MarkDirty(camera);
		}


		void SetCameraNode(NodeHandle Node)
		{
			GetComponent().SetCameraNode(camera, Node);
			GetComponent().MarkDirty(camera);
		}


		void SetCameraFOV(float r)
		{
			GetComponent().SetCameraFOV(camera, r);
			GetComponent().MarkDirty(camera);
		}


		void MarkCameraDirty()
		{
			GetComponent().MarkDirty(camera);
		}


		NodeHandle GetNode() const
		{
			return GetComponent().GetCameraNode(camera);
		}


		NodeHandle GetCameraNode() const
		{
			return GetComponent().GetCameraNode(camera);
		}


		float GetCameraFov()
		{
			return GetComponent().GetCameraFOV(camera);
		}


		operator CameraHandle () 
		{
			return camera;
		}


		CameraHandle camera;
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

	struct PassPVS
	{
		PassHandle	pass;
		PVS			pvs;
	};


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
