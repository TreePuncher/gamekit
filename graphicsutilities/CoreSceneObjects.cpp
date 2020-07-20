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


#include "CoreSceneObjects.h"

namespace FlexKit
{
	/************************************************************************************************/


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
			auto E = v.D;
			auto P = FlexKit::GetPositionW( E->Node );

			auto Depth = (size_t)abs(float3(CP - P).magnitudesquared() * 10000);
			auto SortID = CreateSortingID(false, E->Textured, Depth);
			v.SortID = SortID;
		}
		
		std::sort( PVS_->begin(), PVS_->end(), []( PVEntry& R, PVEntry& L ) -> bool
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
			auto E = v.D;
			auto P = FlexKit::GetPositionW( E->Node );
			float D = float3( CP - P ).magnitudesquared() * ( E->DrawLast ? -1.0 : 1.0 );
			v.SortID = D;
		}

		std::sort( PVS_->begin(), PVS_->end(), []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R > (size_t)L );
		} );
	}
	

	/************************************************************************************************/

	

	void DEBUG_DrawCameraFrustum(LineSegments* out, Camera* C, float3 Position, Quaternion Q)
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
			float4 TopRight		(1, 1, 0.0f, 1);
			float4 TopLeft		(-1, 1, 0.0f, 1);
			float4 BottomRight	(1, -1, 0.0f, 1);
			float4 BottomLeft	(-1, -1, 0.0f, 1);
			{
				float4 V1 = DirectX::XMVector4Transform(TopRight,	Float4x4ToXMMATIRX(&InverseView));
				float4 V2 = DirectX::XMVector4Transform(TopLeft,	Float4x4ToXMMATIRX(&InverseView));
				float3 V3 = V1.xyz() / V1.w;
				float3 V4 = V2.xyz() / V2.w;

				Out.FTL	= V4;
				Out.FTR = V3;
			}
			{
				float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				float4 V2 = DirectX::XMVector4Transform(BottomLeft,		Float4x4ToXMMATIRX(&InverseView));
				float3 V3 = V1.xyz() / V1.w;
				float3 V4 = V2.xyz() / V2.w;

				Out.FBL = V4;
				Out.FBR = V3;
			}
		}
		// Near Field
		{
			float4 TopRight		{  1.0f,  1.0f, 0.1f, 1.0f };
			float4 TopLeft		{ -1.0f,  1.0f, 0.1f, 1.0f };
			float4 BottomRight	{  1.0f, -1.0f, 0.1f, 1.0f };
			float4 BottomLeft	{ -1.0f, -1.0f, 0.1f, 1.0f };

			{
				float4 V1 = DirectX::XMVector4Transform(TopRight, Float4x4ToXMMATIRX(&InverseView));
				float4 V2 = DirectX::XMVector4Transform(TopLeft, Float4x4ToXMMATIRX(&InverseView));
				float3 V3 = V1.xyz() / V1.w;
				float3 V4 = V2.xyz() / V2.w;

				Out.NTL = V4;
				Out.NTR = V3;
			}
			{
				float4 V1 = DirectX::XMVector4Transform(BottomRight, Float4x4ToXMMATIRX(&InverseView));
				float4 V2 = DirectX::XMVector4Transform(BottomLeft, Float4x4ToXMMATIRX(&InverseView));
				float3 V3 = V1.xyz() / V1.w;
				float3 V4 = V2.xyz() / V2.w;

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
			Out.Min.x = min(Out.Min.x, P.x);
			Out.Min.y = min(Out.Min.y, P.y);
			Out.Min.z = min(Out.Min.z, P.z);

			Out.Max.x = max(Out.Min.x, P.x);
			Out.Max.y = max(Out.Min.y, P.y);
			Out.Max.z = max(Out.Min.z, P.z);
		}

		return Out;
	}



	/************************************************************************************************/


	DirectX::XMMATRIX CreatePerspective(Camera* camera, bool Invert = false)
	{
		DirectX::XMMATRIX InvertPersepective(DirectX::XMMatrixIdentity());
		if (Invert)
		{
			InvertPersepective.r[2].m128_f32[2] = -1;
			InvertPersepective.r[2].m128_f32[3] = 1;
			InvertPersepective.r[3].m128_f32[2] = 1;
		}

		DirectX::XMMATRIX proj = InvertPersepective * XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovRH(camera->FOV, camera->AspectRatio, camera->Near, camera->Far));

		return XMMatrixTranspose(proj);
	}


	/************************************************************************************************/


	void Camera::UpdateMatrices()
	{
		using DirectX::XMMATRIX;
		using DirectX::XMMatrixTranspose;
		using DirectX::XMMatrixInverse;
		
		XMMATRIX XMView;
		XMMATRIX XMWT;
		XMMATRIX XMPV;
		XMMATRIX XMIV;
		XMMATRIX XMProj;

		if (Node != InvalidHandle_t)
			GetTransform(Node, &XMWT);
		else
			XMWT = DirectX::XMMatrixIdentity();

		XMView		= XMMatrixInverse(nullptr, XMWT);
		XMProj		= CreatePerspective(this, invert);
		XMPV		= XMMatrixTranspose(XMMatrixTranspose(XMProj) * XMView);
		XMIV		= XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixTranspose(CreatePerspective(this, invert)) * XMView));
		
		WT		= XMMatrixToFloat4x4(&XMWT);
		View	= XMMatrixToFloat4x4(&XMView);
		PV		= XMMatrixToFloat4x4(&XMPV);
		Proj	= XMMatrixToFloat4x4(&XMProj);
		IV		= XMMatrixToFloat4x4(&XMIV);
	}


	/************************************************************************************************/


	Camera::ConstantBuffer Camera::GetConstants()
	{
		DirectX::XMMATRIX XMWT   = Float4x4ToXMMATIRX(&WT);
		DirectX::XMMATRIX XMView = DirectX::XMMatrixInverse(nullptr, XMWT);

		Camera::ConstantBuffer NewData;
		NewData.Proj            = Proj;
		NewData.View			= View.Transpose();
		NewData.ViewI           = WT.Transpose();
		NewData.PV              = XMMatrixToFloat4x4(XMMatrixTranspose(XMMatrixTranspose(Float4x4ToXMMATIRX(NewData.Proj)) * XMView));
		NewData.PVI             = XMMatrixToFloat4x4(XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, XMMatrixTranspose(Float4x4ToXMMATIRX(NewData.Proj)) * XMView)));
		NewData.MinZ            = Near;
		NewData.MaxZ            = Far;

		PV = NewData.PV;

		NewData.WPOS[0]         = WT[0][3];
		NewData.WPOS[1]         = WT[1][3];
		NewData.WPOS[2]         = WT[2][3];
		NewData.WPOS[3]         = 0;

		const float Y = tan(FOV / 2);
		const float X = Y * AspectRatio;

		NewData.TLCorner_VS = float3(-X, Y, -1);
		NewData.TRCorner_VS = float3(X, Y, -1);

		NewData.BLCorner_VS = float3(-X, -Y, -1);
		NewData.BRCorner_VS = float3(X, -Y, -1);

		NewData.FOV         = FOV;
		NewData.AspectRatio = AspectRatio;

		return NewData;
	}

	float4x4 Camera::GetPV()
	{
		return PV;
	}



	/************************************************************************************************/
}
