/**********************************************************************

Copyright (c) 2015 - 2017 Robert May

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

	void SortPVS( SceneNodes* Nodes, PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( Nodes, C->Node );
		for( auto& v : *PVS_ )
		{
			auto E = v.D;
			auto P = FlexKit::GetPositionW( Nodes, E->Node );

			auto Depth = (size_t)abs(float3(CP - P).magnitudesquared() * 10000);
			auto SortID = CreateSortingID(E->Posed, E->Textured, Depth);
			v.SortID = SortID;
		}
		
		std::sort( PVS_->begin(), PVS_->end(), []( PVEntry& R, PVEntry& L ) -> bool
		{
			return ( (size_t)R.SortID < (size_t)L.SortID);
		} );
	}


	/************************************************************************************************/


	void SortPVSTransparent( SceneNodes* Nodes, PVS* PVS_, Camera* C)
	{
		if(!PVS_->size())
			return;

		auto CP = FlexKit::GetPositionW( Nodes, C->Node );
		for( auto& v : *PVS_ )
		{
			auto E = v.D;
			auto P = FlexKit::GetPositionW( Nodes, E->Node );
			float D = float3( CP - P ).magnitudesquared() * ( E->DrawLast ? -1.0 : 1.0 );
			v.SortID = D;
		}

		std::sort( PVS_->begin(), PVS_->end(), []( auto& R, auto& L ) -> bool
		{
			return ( (size_t)R > (size_t)L );
		} );
	}
	

	/************************************************************************************************/

	

	void DEBUG_DrawCameraFrustum(LineSet* out, Camera* C, float3 Position, Quaternion Q)
	{
		auto Points = C->GetFrustumPoints(Position, Q);

		LineSegment Line;
		Line.AColour = WHITE;
		Line.BColour = WHITE;

		Line.A = Points.FTL;
		Line.B = Points.FTR;
		out->LineSegments.push_back(Line);

		Line.A = Points.FTL;
		Line.B = Points.FBL;
		out->LineSegments.push_back(Line);

		Line.A = Points.FTR;
		Line.B = Points.FBR;
		out->LineSegments.push_back(Line);

		Line.A = Points.FBL;
		Line.B = Points.FBR;
		out->LineSegments.push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.NTR;
		out->LineSegments.push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.NBR;
		out->LineSegments.push_back(Line);

		Line.A = Points.NTR;
		Line.B = Points.NBL;
		out->LineSegments.push_back(Line);

		Line.A = Points.NBL;
		Line.B = Points.NBR;
		out->LineSegments.push_back(Line);

		//*****************************************

		Line.A = Points.NBR;
		Line.B = Points.FBR;
		out->LineSegments.push_back(Line);

		Line.A = Points.NBL;
		Line.B = Points.FBL;
		out->LineSegments.push_back(Line);

		Line.A = Points.NTR;
		Line.B = Points.FTL;
		out->LineSegments.push_back(Line);

		Line.A = Points.NTL;
		Line.B = Points.FTR;
		out->LineSegments.push_back(Line);
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

				int x = 0;
			}
			{
				float4 V1 = DirectX::XMVector4Transform(BottomRight,	Float4x4ToXMMATIRX(&InverseView));
				float4 V2 = DirectX::XMVector4Transform(BottomLeft,		Float4x4ToXMMATIRX(&InverseView));
				float3 V3 = V1.xyz() / V1.w;
				float3 V4 = V2.xyz() / V2.w;

				Out.FBL = V4;
				Out.FBR = V3;

				int x = 0;
			}
		}
		// Near Field
		{
			float4 TopRight		(1, 1, 1, 1);
			float4 TopLeft		(-1, 1, 1, 1);
			float4 BottomRight	(1, -1, 1, 1);
			float4 BottomLeft	(-1, -1, 1, 1);
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
}