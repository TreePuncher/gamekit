#pragma once
#include "GameFramework.hpp"
#include "Transforms.hpp"
#include "MathUtilities.hpp"


namespace FlexKit
{	/************************************************************************************************/


	class Camera
	{
	public:
		struct alignas(256) ConstantBuffer
		{
			float4x4_GPU	View;
			float4x4_GPU	ViewI;
			float4x4_GPU	Proj;
			float4x4_GPU	PV;			//  Projection x View
			float4x4_GPU	PVI;		// (Projection x View)^-1
			float4			WPOS;
			float			MinZ;
			float			MaxZ;
			float			AspectRatio;
			float			FOV;

			float3   TLCorner_VS;
			float3   TRCorner_VS;

			float3   BLCorner_VS;
			float3   BRCorner_VS;

			constexpr static const size_t GetBufferSize()
			{
				return sizeof(ConstantBuffer);
			}

		};


		static ConstantBuffer	CalculateCameraConstants(const float aspectRation, const float FOV, const float minZ, const float maxZ, const float4x4& WT, const float4x4& View);
		Camera::ConstantBuffer	CalculateCameraConstants(const float aspectRatio, const float FOV, const float minZ, const float maxZ, const float4x4& WT);

		FustrumPoints	GetFrustumPoints(float3 XYZ, Quaternion Q);
		ConstantBuffer	GetConstants() const;
		ConstantBuffer	GetCameraPreviousConstants() const;
		float4x4		GetPV() const noexcept;

		NodeHandle	Node;

		float	FOV;
		float	AspectRatio;
		float	Near;
		float	Far;
		bool	invert;
		float	fStop;	// Future
		float	ISO;	// Future

		float4x4 View;
		float4x4 Proj;
		float4x4 WT;	// World Transform
		float4x4 PV;	// Projection x View
		float4x4 IV;	// Inverse Transform


		struct PreviousFrameState
		{
			float4x4 View	= float4x4::Identity();
			float4x4 Proj	= float4x4::Identity();
			float4x4 WT		= float4x4::Identity();	// World Transform
			float4x4 PV		= float4x4::Identity();	// Projection x View
			float4x4 IV		= float4x4::Identity();	// Inverse Transform

			float FOV			= 1.0f;
			float aspectRatio	= 1.0f;
			float nearClip		= 0.01f;
			float farClip		= 10000.0f;
		}   previous;

		void UpdateMatrices();

		static MinMax	GetAABS_XZ(FustrumPoints Points);
	};


	/************************************************************************************************/


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


	class CameraView : 
		public ComponentView_t<CameraComponent>
	{
	public:
		CameraView(GameObject& go, CameraHandle IN_camera = CameraComponent::GetComponent().CreateCamera());

		void Release();

		NodeHandle	GetNode() const;
		NodeHandle	GetCameraNode() const;
		float		GetCameraFov();
		

		operator CameraHandle ();

	    void SetCameraAspectRatio(float AspectRatio);
	    void SetCameraNode(NodeHandle Node);
		void SetCameraFOV(float r);
		void SetCameraNear(float);
		void SetCameraFar(float);
		void MarkCameraDirty();

		CameraHandle camera;
	};


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

	Frustum GetFrustum		(CameraHandle camera);
	Frustum GetFrustumVS	(CameraHandle camera);
	Frustum GetSubFrustum	(CameraHandle camera, float2 UL, float2 BR);


	/************************************************************************************************/

	
	enum OrbitCameraEvents
	{
		OCE_MoveForward,
		OCE_MoveBackward,
		OCE_MoveLeft,
		OCE_MoveRight,
		OCE_MoveUp,
		OCE_MoveDown,
	};


	class OrbitCameraBehavior :
		public CameraView
	{
	public:
		OrbitCameraBehavior(GameObject&, CameraHandle Handle = CameraComponent::GetComponent().CreateCamera(), float MovementSpeed = 100, float3 InitialPos = {0, 0, 0});

		void Update				(const MouseInputState& MouseInput, double dt);

		void SetCameraPosition	(float3 xyz);
		void TranslateWorld		(float3 xyz);

		void Yaw				(float Degree);
		void Pitch				(float Degree);
		void Roll				(float Degree);
		void Rotate				(float3 xyz); // Three Angles

		bool HandleEvent(const FlexKit::Event& evt);

		Quaternion	GetOrientation();
		float3		GetForwardVector();
		float3		GetRightVector();

		NodeHandle	GetCameraNode();

		NodeHandle		cameraNode;
		NodeHandle		yawNode;
		NodeHandle		pitchNode;
		NodeHandle		rollNode;
		float			moveRate;

		float3			velocity		= 0;
		float			acceleration	= 50;
		float			drag			= 5.0;

		struct KeyStates
		{
			bool forward	= false;
			bool backward	= false;
			bool left		= false;
			bool right		= false;
			bool up         = false;
			bool down       = false;

			bool KeyPressed()
			{
				return forward | backward | left | right | up | down;
			}
		}keyStates;
	};


	/************************************************************************************************/


	void OrbitCameraUpdate(
		GameObject&		gameObject,
		MouseInputState	mouseState,
		float			dt);

	bool OrbitCameraHandleEvent(
		GameObject&		gameObject,
		const Event&	evt);

	void OrbitCameraTranslate(
		GameObject&		gameObject,
		const float3	xyz);

	void OrbitCameraPitch(
		GameObject& gameObject,
		const float	t);

	void OrbitCameraYaw(
		GameObject&		gameObject,
		const float		t);

	UpdateTask&	QueueOrbitCameraUpdateTask(
		UpdateDispatcher&		dispatcher,
		OrbitCameraBehavior&	orbitCamera,
		MouseInputState			mouseState,
		float					dt);


}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2022 - 2025 Robert May

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
