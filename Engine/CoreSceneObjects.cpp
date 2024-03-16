#include "CoreSceneObjects.h"
#include "MathUtils.h"

namespace FlexKit
{	/************************************************************************************************/


	size_t CreateSortingID(bool Posed, bool Textured, size_t Depth)
	{
		size_t DepthPart	= (Depth & 0x00ffffffffffff);
		size_t PosedBit		= (size_t(Posed)	<< (Posed		? 63 : 0));
		size_t TextureBit	= (size_t(Textured) << (Textured	? 62 : 0));

		return DepthPart | PosedBit | TextureBit;
	}

	void SortPVS(PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( C->Node );
		for( auto& v : *PVS_ )
		{
			auto b = v.brush;
			auto P = FlexKit::GetPositionW( b->Node );

			auto Depth = (size_t)abs(float3(CP - P).magnitudeSq() * 10000);
			auto SortID = CreateSortingID(false, b->Textured, Depth);
			v.SortID = SortID;
		}
		
		std::sort( PVS_->begin(), PVS_->end(), [](const PVEntry& R, const PVEntry& L ) -> bool
		{
			return ( (size_t)R.SortID < (size_t)L.SortID);
		} );
	}


	/************************************************************************************************/


	void SortPVSTransparent(PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( C->Node );
		for( auto& v : *PVS_ )
		{
			const Brush* b	= v.brush;
			const float3 P	= GetPositionW( b->Node );
			const float D	= float3{ CP - P }.magnitudeSq() * (b->DrawLast ? -1.0f : 1.0f);
			v.SortID		= (uint64_t)D;
		}

		std::sort( PVS_->begin(), PVS_->end(), []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R > (size_t)L );
		} );
	}
	

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
				const float4 V1 = DirectX::XMVector4Transform(TopRight,		Float4x4ToXMMATIRX(&InverseView));
				const float4 V2 = DirectX::XMVector4Transform(TopLeft,		Float4x4ToXMMATIRX(&InverseView));
				const float3 V3 = V1.xyz() / V1.w;
				const float3 V4 = V2.xyz() / V2.w;

				Out.FTL	= V4;
				Out.FTR = V3;
			}
			{
				const float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				const float4 V2 = DirectX::XMVector4Transform(BottomLeft,		Float4x4ToXMMATIRX(&InverseView));
				const float3 V3 = V1.xyz() / V1.w;
				const float3 V4 = V2.xyz() / V2.w;

				Out.FBL = V4;
				Out.FBR = V3;
			}
		}
		// Near Field
		{
			const float4 TopRight		{  1.0f,  1.0f, 0.1f, 1.0f };
			const float4 TopLeft		{ -1.0f,  1.0f, 0.1f, 1.0f };
			const float4 BottomRight	{  1.0f, -1.0f, 0.1f, 1.0f };
			const float4 BottomLeft		{ -1.0f, -1.0f, 0.1f, 1.0f };

			{
				const float4 V1 = DirectX::XMVector4Transform(TopRight,		Float4x4ToXMMATIRX(&InverseView));
				const float4 V2 = DirectX::XMVector4Transform(TopLeft,		Float4x4ToXMMATIRX(&InverseView));
				const float3 V3 = V1.xyz() / V1.w;
				const float3 V4 = V2.xyz() / V2.w;

				Out.NTL = V4;
				Out.NTR = V3;
			}
			{
				const float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				const float4 V2 = DirectX::XMVector4Transform(BottomLeft,	Float4x4ToXMMATIRX(&InverseView));
				const float3 V3 = V1.xyz() / V1.w;
				const float3 V4 = V2.xyz() / V2.w;

				Out.NBL = V4;
				Out.NBR = V3;
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
		updatedIV	= Inverse(updatedProj * updatedView);

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



	Camera::ConstantBuffer Camera::GetConstants() const
	{
		const float4x4 view = Inverse(WT);

		Camera::ConstantBuffer constants;
		constants.Proj		= Proj;
		constants.View		= View;
		constants.ViewI		= WT;
		constants.PV		= constants.Proj * view;
		constants.PVI		= Inverse(constants.PV);
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
}


/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

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
