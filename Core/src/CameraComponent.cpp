#include "CameraComponent.hpp"
#include "EngineCore.hpp"


namespace FlexKit
{
	/************************************************************************************************/

	/*
	void debug_DrawCameraFrustum(LineSegments* out, Camera* C, float3 Position, Quaternion Q)
	{
		auto Points = C->GetFrustumPoints(Position, Q);

		LineSegment Line;
		Line.AColour = WHITE;
		Line.BColour = WHITE;

		Line.A = Points.FTL;
		Line.B = Points.FTR;
		out->push_back(Line);

		Line.A = Points.FTL;
		Line.B = Points.FBL;
		out->push_back(Line);

		Line.A = Points.FTR;
		Line.B = Points.FBR;
		out->push_back(Line);

		Line.A = Points.FBL;
		Line.B = Points.FBR;
		out->push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.NTR;
		out->push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.NBR;
		out->push_back(Line);

		Line.A = Points.NTR;
		Line.B = Points.NBL;
		out->push_back(Line);

		Line.A = Points.NBL;
		Line.B = Points.NBR;
		out->push_back(Line);

		//*****************************************

		Line.A = Points.NBR;
		Line.B = Points.FBR;
		out->push_back(Line);

		Line.A = Points.NBL;
		Line.B = Points.FBL;
		out->push_back(Line);

		Line.A = Points.NTR;
		Line.B = Points.FTL;
		out->push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.FTR;
		out->push_back(Line);
	}
	*/

	/************************************************************************************************/


	FustrumPoints Camera::GetFrustumPoints(float3 Position, Quaternion Q)
	{
		// TODO(R.M): Optimize this maybe?
		FustrumPoints Out;

#if 0
		Out.FTL.z = -C->Far;
		Out.FTL.y = tan(C->FOV) * C->Far;
		Out.FTL.x = -Out.FTL.y * C->AspectRatio;

		Out.FTR = { -Out.FTL.x,  Out.FTL.y, Out.FTL.z };
		Out.FBL = {  Out.FTL.x, -Out.FTL.y, Out.FTL.z };
		Out.FBR = { -Out.FTL.x, -Out.FTL.y, Out.FTL.z };


		Out.NTL.z = -C->Near;
		Out.NTL.y = tan(C->FOV / 2) * C->Near;
		Out.NTL.x = -Out.NTL.y  * C->AspectRatio;

		Out.NTR = { -Out.NTL.x,  Out.NTL.y, Out.NTL.z };
		Out.NBL = { -Out.NTL.x, -Out.NTL.y, Out.NTL.z };
		Out.NBR = {  Out.NTL.x, -Out.NTL.y, Out.NTL.z };

		Out.FTL = Position + (Q * Out.FTL);
		Out.FTR = Position + (Q * Out.FTR);
		Out.FBL = Position + (Q * Out.FBL);
		Out.FBR = Position + (Q * Out.FBR);

		Out.NTL = Position + (Q * Out.NTL);
		Out.NTR = Position + (Q * Out.NTR);
		Out.NBL = Position + (Q * Out.NBL);
		Out.NBR = Position + (Q * Out.NBR);

#else
		float4x4 InverseView = IV;

		// Far Field
		{
			const float4 TopRight		{  1.0f,  1.0f, 0.0f, 1.0f };
			const float4 TopLeft		{ -1.0f,  1.0f, 0.0f, 1.0f };
			const float4 BottomRight	{  1.0f, -1.0f, 0.0f, 1.0f };
			const float4 BottomLeft	{ -1.0f, -1.0f, 0.0f, 1.0f };
			{
				FK_ASSERT(0);
				//const float4 V1 = DirectX::XMVector4Transform(TopRight,		Float4x4ToXMMATIRX(&InverseView));
				//const float4 V2 = DirectX::XMVector4Transform(TopLeft,		Float4x4ToXMMATIRX(&InverseView));
				//const float3 V3 = V1.xyz() / V1.w;
				//const float3 V4 = V2.xyz() / V2.w;
				//
				//Out.FTL	= V4;
				//Out.FTR = V3;
			}
			{
				//const float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				//const float4 V2 = DirectX::XMVector4Transform(BottomLeft,		Float4x4ToXMMATIRX(&InverseView));
				//const float3 V3 = V1.xyz() / V1.w;
				//const float3 V4 = V2.xyz() / V2.w;
				//
				//Out.FBL = V4;
				//Out.FBR = V3;
			}
		}
		// Near Field
		{
			const float4 TopRight		{  1.0f,  1.0f, 0.1f, 1.0f };
			const float4 TopLeft		{ -1.0f,  1.0f, 0.1f, 1.0f };
			const float4 BottomRight	{  1.0f, -1.0f, 0.1f, 1.0f };
			const float4 BottomLeft		{ -1.0f, -1.0f, 0.1f, 1.0f };

			{
				//const float4 V1 = DirectX::XMVector4Transform(TopRight,		Float4x4ToXMMATIRX(&InverseView));
				//const float4 V2 = DirectX::XMVector4Transform(TopLeft,		Float4x4ToXMMATIRX(&InverseView));
				//const float3 V3 = V1.xyz() / V1.w;
				//const float3 V4 = V2.xyz() / V2.w;
				//
				//Out.NTL = V4;
				//Out.NTR = V3;
			}
			{
				//const float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				//const float4 V2 = DirectX::XMVector4Transform(BottomLeft,	Float4x4ToXMMATIRX(&InverseView));
				//const float3 V3 = V1.xyz() / V1.w;
				//const float3 V4 = V2.xyz() / V2.w;
				//
				//Out.NBL = V4;
				//Out.NBR = V3;
			}
		}

#endif
		return Out;
	}


	/************************************************************************************************/


	MinMax Camera::GetAABS_XZ(FustrumPoints Frustum)
	{
		MinMax Out;
		Out.Max = float3(0);
		Out.Min = float3(0);

		for (auto P : Frustum.Points)
		{
			Out.Min.x = Min(Out.Min.x, P.x);
			Out.Min.y = Min(Out.Min.y, P.y);
			Out.Min.z = Min(Out.Min.z, P.z);

			Out.Max.x = Max(Out.Min.x, P.x);
			Out.Max.y = Max(Out.Min.y, P.y);
			Out.Max.z = Max(Out.Min.z, P.z);
		}

		return Out;
	}



	/************************************************************************************************/


	float4x4 CreatePerspectiveRH(const Camera& camera, bool invert = false)
	{
		if (camera.FOV == 0.0f || camera.AspectRatio == 0.0f || camera.Near == 0.0f || camera.Far == 0.0f)
		{
			FK_LOG_WARNING("Invalid Args passed to CreatePerspectiveRH!");
			return float4x4::Identity();
		}

		float4x4 projection = CreatePerspectiveRH(camera.FOV, camera.Near, camera.Far, camera.AspectRatio);
		if (invert)
		{
			float4x4 invertPersepective = float4x4::Identity();

			invertPersepective(2, 2) = -1;
			invertPersepective(2, 3) = 1;
			invertPersepective(3, 2) = 1;
			projection = invertPersepective * projection;
		}

		return projection;
	}


	/************************************************************************************************/


	void Camera::UpdateMatrices()
	{
		float4x4 updatedView;
		float4x4 updatedWT;
		float4x4 updatedPV;
		float4x4 updatedIV;
		float4x4 updatedProj;

		if (Node != InvalidHandle)
			updatedWT = GetWT(Node);
		else
			updatedWT  = float4x4::Identity();

		updatedView	= Inverse(updatedWT);
		updatedProj	= CreatePerspectiveRH(*this, invert);
		updatedPV	= updatedProj * updatedView;
		updatedIV	= updatedWT;

		previous.WT     = WT;
		previous.View   = View;
		previous.PV     = PV;
		previous.Proj   = Proj;
		previous.IV     = IV;

		WT		= updatedWT;
		View	= updatedView;
		PV		= updatedPV;
		Proj	= updatedProj;
		IV		= updatedIV;
	}


	/************************************************************************************************/


	Camera::ConstantBuffer CalculateCameraConstants(const float aspectRatio, const float FOV, const float minZ, const float maxZ, const float4x4& WT)
	{
		const float4x4 view	= Inverse(WT);
		const float4x4 proj	= CreatePerspectiveRH(FOV, aspectRatio, minZ, maxZ);

		Camera::ConstantBuffer NewData;
		NewData.Proj			= proj;
		NewData.View			= view;
		NewData.ViewI			= WT;
		NewData.PV				= proj * view;
		NewData.PVI				= Inverse(NewData.PV);
		NewData.MinZ			= minZ;
		NewData.MaxZ			= maxZ;

		NewData.WPOS[0]			= WT[0][3];
		NewData.WPOS[1]			= WT[1][3];
		NewData.WPOS[2]			= WT[2][3];
		NewData.WPOS[3]			= 0;

		const float Y = tan(FOV / 2) * maxZ;
		const float X = Y * aspectRatio;

		NewData.TLCorner_VS = float3(-X, Y, -maxZ);
		NewData.TRCorner_VS = float3(X, Y, -maxZ);

		NewData.BLCorner_VS = float3(-X, -Y, -maxZ);
		NewData.BRCorner_VS = float3(X, -Y, -maxZ);

		NewData.FOV			= FOV;
		NewData.AspectRatio	= aspectRatio;

		return NewData;
	}


	/************************************************************************************************/


	Camera::ConstantBuffer Camera::GetConstants() const
	{
		const float4x4 view = Inverse(WT);
		const float4x4 PV	= Proj * view;
		const float4x4 PVI	= Inverse(PV);

		Camera::ConstantBuffer constants;
		constants.Proj		= Proj;
		constants.View		= View;
		constants.ViewI		= WT;
		constants.PV		= PV;
		constants.PVI		= PVI;
		constants.MinZ		= Near;
		constants.MaxZ		= Far;

		constants.WPOS[0]	= WT[0][3];
		constants.WPOS[1]	= WT[1][3];
		constants.WPOS[2]	= WT[2][3];
		constants.WPOS[3]	= 0;

		const float y = tan(FOV / 2) * Far;
		const float x = y * AspectRatio; 

		constants.TLCorner_VS = float3(-x, y, -Far);
		constants.TRCorner_VS = float3(x, y, -Far);

		constants.BLCorner_VS = float3(-x, -y, -Far);
		constants.BRCorner_VS = float3(x, -y, -Far);

		constants.FOV         = FOV;
		constants.AspectRatio = AspectRatio;

		return constants;
	}


	/************************************************************************************************/


	Camera::ConstantBuffer Camera::GetCameraPreviousConstants() const
	{
		const float4x4 prevWT   = previous.WT;
		const float4x4 prevView = Inverse(prevWT);

		Camera::ConstantBuffer NewData;
		NewData.Proj			= previous.Proj;
		NewData.View			= previous.View;
		NewData.ViewI			= previous.WT;
		NewData.PV				= previous.Proj * prevView;
		NewData.PVI				= Inverse(previous.Proj) * prevView;
		NewData.MinZ			= previous.nearClip;
		NewData.MaxZ			= previous.farClip;

		NewData.WPOS[0]			= prevWT[0][3];
		NewData.WPOS[1]			= prevWT[1][3];
		NewData.WPOS[2]			= prevWT[2][3];
		NewData.WPOS[3]			= 0;

		const float Y = tan(previous.FOV / 2) * Far;
		const float X = Y * previous.aspectRatio;

		NewData.TLCorner_VS = float3{ -X, Y, -Far };
		NewData.TRCorner_VS = float3{ X, Y, -Far };

		NewData.BLCorner_VS	= float3{ -X, -Y, -Far };
		NewData.BRCorner_VS = float3{ X, -Y, -Far };

		NewData.FOV			= previous.FOV;
		NewData.AspectRatio	= previous.aspectRatio;

		return NewData;
	}


	/************************************************************************************************/


	float4x4 Camera::GetPV() const noexcept
	{
		return PV;
	}


	/************************************************************************************************/


	OrbitCameraBehavior::OrbitCameraBehavior(GameObject& go, CameraHandle handle, float movementSpeed, float3 initialPos) :
		CameraView{ go, handle }
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
		float3 up				{ 0, 1, 0 };

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

		if (velocity.magnitudeSq() > 0.001f) {
			velocity -= velocity * drag * dt;
			TranslateWorld(velocity * dt);
		}
		else
			velocity = 0.0f;
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::SetCameraPosition(float3 xyz)
	{
		SetPositionW(yawNode, xyz);

		MarkCameraDirty();
	}


	/************************************************************************************************/


	void OrbitCameraBehavior::TranslateWorld(float3 xyz)
	{
		FlexKit::TranslateWorld(yawNode, xyz);

		MarkCameraDirty();
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

		MarkCameraDirty();
	}


	/************************************************************************************************/


	bool OrbitCameraBehavior::HandleEvent(const FlexKit::Event& evt)
	{
		if (evt.InputSource == FlexKit::Event::Keyboard)
		{
			bool state = evt.Action == Event::Pressed ? true : false;

			switch (evt.mData1.mINT[0])
			{
			case KC_W:
			case OCE_MoveForward:
				keyStates.forward	= state;
				return true;
			case KC_S:
			case OCE_MoveBackward:
				keyStates.backward	= state;
				return true;
			case KC_A:
			case OCE_MoveLeft:
				keyStates.left		= state;
				return true;
			case KC_D:
			case OCE_MoveRight:
				keyStates.right		= state;
				return true;
			case KC_E:
			case OCE_MoveUp:
				keyStates.up		= state;
				return true;
			case KC_Q:
			case OCE_MoveDown:
				keyStates.down		= state;
				return true;
			default:
				return false;
			}
		}

		return false;
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


	void OrbitCameraUpdate(
		GameObject&				gameObject,
		MouseInputState			mouseState,
		float					dt)
	{
		Apply(gameObject,
			[&](OrbitCameraBehavior& orbitCamera)
			{
				orbitCamera.Update(mouseState, dt);
			});
	}


	/************************************************************************************************/


	bool OrbitCameraHandleEvent(
		GameObject&		gameObject,
		const Event&	evt)
	{
		return Apply(gameObject,
			[&](OrbitCameraBehavior& orbitCamera) -> bool
			{
				return orbitCamera.HandleEvent(evt);
			},
			[] { return false; });
	}


	/************************************************************************************************/


	void OrbitCameraTranslate(
		GameObject& gameObject,
		const float3 xyz)
	{
		Apply(gameObject,
			[&](OrbitCameraBehavior& orbitCamera)
			{
				orbitCamera.TranslateWorld(xyz);
			});
	}

	void OrbitCameraPitch(
		GameObject& gameObject,
		const float	t)
	{
		Apply(gameObject,
			[&](OrbitCameraBehavior& orbitCamera)
			{
				orbitCamera.Rotate({ t, 0, 0 });
			});
	}

	void OrbitCameraYaw(
		GameObject& gameObject,
		const float t)
	{
		Apply(gameObject,
			[&](OrbitCameraBehavior& orbitCamera)
			{
				orbitCamera.Rotate({0, t, 0});
			});
	}


	/************************************************************************************************/


	UpdateTask& QueueOrbitCameraUpdateTask(
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

		UpdateTask& data = dispatcher.Add<OrbitCameraUpdateData>(
			[&](UpdateDispatcher::UpdateBuilder& Builder, OrbitCameraUpdateData& data)
			{
				Builder.SetDebugString("OrbitCamera Update");

				data.mouseState		= mouseState;
				data.dt				= dt;
			},
			[&orbitCamera](auto& data, iAllocator& threadAllocator)
			{
				ProfileFunction();

				FK_LOG_9("OrbitCamera Update");

				orbitCamera.Update(data.mouseState, data.dt);
			});

		return data;
	}


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


	CameraView::CameraView(GameObject& go, CameraHandle IN_camera) :
		camera{ IN_camera } {
	}

    void CameraView::Release()
	{
		GetComponent().Release(camera);
	}


	/************************************************************************************************/


	void CameraView::SetCameraAspectRatio(float AspectRatio)
	{
		GetComponent().SetCameraAspectRatio(camera, AspectRatio);
		GetComponent().MarkDirty(camera);
	}


	/************************************************************************************************/


	void CameraView::SetCameraNode(NodeHandle Node)
	{
		GetComponent().SetCameraNode(camera, Node);
		GetComponent().MarkDirty(camera);
	}


	/************************************************************************************************/


	void CameraView::SetCameraFOV(float r)
	{
		GetComponent().SetCameraFOV(camera, r);
		GetComponent().MarkDirty(camera);
	}


	/************************************************************************************************/


	void CameraView::MarkCameraDirty()
	{
		GetComponent().MarkDirty(camera);
	}


	/************************************************************************************************/


	NodeHandle CameraView::GetNode() const
	{
		return GetComponent().GetCameraNode(camera);
	}


	/************************************************************************************************/


	NodeHandle CameraView::GetCameraNode() const
	{
		return GetComponent().GetCameraNode(camera);
	}


	/************************************************************************************************/


	float CameraView::GetCameraFov()
	{
		return GetComponent().GetCameraFOV(camera);
	}


	/************************************************************************************************/


	CameraView::operator CameraHandle ()
	{
		return camera;
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



	Frustum GetFrustum(CameraHandle camera)
	{
		auto node = GetCameraNode(camera);

		return GetFrustum(
			GetCameraAspectRatio(camera),
			GetCameraFOV(camera),
			GetCameraNear(camera),
			GetCameraFar(camera),
			GetPositionW(node),
			GetOrientation(node));
	}

	Frustum GetFrustumVS(CameraHandle camera)
	{
		auto node = GetCameraNode(camera);

		return GetFrustumVS(
			GetCameraAspectRatio(camera),
			GetCameraFOV(camera),
			GetCameraNear(camera),
			GetCameraFar(camera));
	}


	Frustum GetSubFrustum(CameraHandle camera, float2 UL, float2 BR)
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



}	/************************************************************************************************/


/**********************************************************************

Copyright (c) 2022 Robert May

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

